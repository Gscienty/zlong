#include "debug/console.h"
#include "http/config.h"
#include "http/interface.h"
#include "session/session_handler.h"
#include "utils/define.h"
#include <openssl/ssl.h>
#include <openssl/err.h>
#include <malloc.h>
#include <string.h>

static zl_webgateway_enter_fptr __webgateway_fptr = NULL;
static zl_websocket_frame_parse_fptr __websocket_frame_parse_fptr = NULL;
static zl_http_req_protocol_parse_fptr __http_req_protocol_parse_fptr = NULL;

void zl_session_destory(struct http_session * const session)
{
    if (session->buf != NULL)
        free(session->buf);

    zl_http_req_protocol_reset(&session->req_protocol);
    zl_http_res_protocol_clear(&session->res_protocol);
    zl_websocket_frame_reset(&session->ws_cli_frame);
    zl_websocket_frame_reset(&session->ws_ser_frame);

    if (session->security && session->ssl != NULL) {
        SSL_free(session->ssl);
        session->ssl = NULL;
    }

    zl_sessions_remove(session);
    free(session);
}

void zl_session_readbuf_alloc(uv_handle_t * handle,
                              size_t suggested_size,
                              uv_buf_t * buf)
{
    struct http_session * session =
        container_of(handle, struct http_session, tcp_sock);

    if (session->buf_size < suggested_size) {
        if (session->buf == NULL)
            session->buf = malloc(suggested_size);
        else
            session->buf = realloc(session->buf, suggested_size);
        session->buf_size = suggested_size;
    }
    buf->base = session->buf;
    buf->len = session->buf_size;
}

void zl_session_close(uv_handle_t * handle)
{
    struct http_session * session;

    session = zl_sessions_find((uv_tcp_t *) handle);
    if (session == NULL)
        return;
    info("close session[%x]", session);

    zl_session_destory(session);
}

void zl_session_write(uv_write_t *req, int status)
{
    (void) req;
    if (status < 0)
        return;
}

bool zl_session_http_respond(uv_stream_t * stream,
                             struct http_session * const session)
{
    uv_buf_t buf;
    
    if (!zl_http_res_protocol_serialize(&session->res_protocol)) {
        return false;
    }

    if (session->security) {
        SSL_write(session->ssl,
                  session->res_protocol.tcp_payload,
                  session->res_protocol.tcp_payload_size);
        buf.len = session->res_protocol.tcp_payload_size + 1024;
        buf.base = malloc(buf.len);
        BIO_read(session->write_bio, buf.base, buf.len);
    }
    else {
        buf.base = session->res_protocol.tcp_payload;
        buf.len  = session->res_protocol.tcp_payload_writable;
    }

    uv_write(&session->writer, stream, &buf, 1, zl_session_write);

    session->res_protocol.tcp_payload          = NULL;
    session->res_protocol.tcp_payload_size     = 0;
    session->res_protocol.tcp_payload_writable = 0;

    return true;
}

static void __http_flush(uv_stream_t * stream,
                           struct http_session * session)
{
    if (!session->is_websocket) {
        zl_http_req_protocol_reset(&session->req_protocol);
    }
    zl_http_req_parser_init(&session->parser);

    if (!zl_session_http_respond(stream, session))
        uv_close((uv_handle_t *) stream, zl_session_close);

    zl_http_res_protocol_clear(&session->res_protocol);
}

void zl_session_websocket_reset(struct http_session * session)
{
    zl_websocket_frame_reset(&session->ws_cli_frame);
    zl_websocket_frame_parser_init(&session->ws_parser);
}

void __session_enter_app(struct http_session * const session,
                         ssize_t nread,
                         const uv_buf_t * buf)
{
    if (session->is_websocket) {
        if (__webgateway_fptr != NULL
            && __websocket_frame_parse_fptr != NULL) {
            __websocket_frame_parse_fptr(&session->ws_parser,
                                         &session->ws_cli_frame,
                                         (unsigned char *) buf->base,
                                         nread);
            if (session->ws_parser.stat == WEBSOCKET_FRAME_STAT_END) {
                info("parsed websocket frame");
                __webgateway_fptr(session);
                zl_session_websocket_reset(session);
            }
        }
        else {
            uv_close((uv_handle_t *) &session->tcp_sock, zl_session_close);
        }
    }
    else {
        if (__webgateway_fptr != NULL
            && __http_req_protocol_parse_fptr != NULL) {
            __http_req_protocol_parse_fptr(&session->parser,
                                           &session->req_protocol,
                                           (unsigned char *) buf->base,
                                           nread);
            if (session->parser.stat == HTTP_REQ_STAT_END) {
                info("parsed http req protocol");
                __webgateway_fptr(session);
                __http_flush((uv_stream_t *) &session->tcp_sock, session);
            }
        }
        else {
            uv_close((uv_handle_t *) &session->tcp_sock, zl_session_close);
        }
    }
}


static bool __session_security_handshake(struct http_session * const session)
{
    int ret;

    session->ssl_state = HTTP_SESSION_TLS_STATE_HANDSHAKE;
    ret = SSL_do_handshake(session->ssl);

    switch (ret) {
    case 1:
        session->ssl_state = HTTP_SESSION_TLS_STATE_IO;
        info("tls handshake completed");
        return true;
    case 0:
        session->ssl_state = HTTP_SESSION_TLS_STATE_CLOSING;
        return false;
    case -1:
        ret = SSL_get_error(session->ssl, ret);
        switch (ret) {
        case SSL_ERROR_WANT_READ:
            return false;
        default:
            info("closing error", ret);
            session->ssl_state = HTTP_SESSION_TLS_STATE_CLOSING;
            return false;
        }
    default:
        session->ssl_state = HTTP_SESSION_TLS_STATE_CLOSING;
        return false;
    }
}

static void __session_security_read(struct http_session * const session,
                                    ssize_t nread,
                                    uv_buf_t * buf)
{
    int rc;
    uv_buf_t handshake_buf;
    rc = BIO_write(session->read_bio, buf->base, nread);

    if (session->ssl_state == HTTP_SESSION_TLS_STATE_INIT
        || session->ssl_state == HTTP_SESSION_TLS_STATE_HANDSHAKE) {
        __session_security_handshake(session);
        if(session->ssl_state == HTTP_SESSION_TLS_STATE_HANDSHAKE
           || session->ssl_state == HTTP_SESSION_TLS_STATE_IO) {

            handshake_buf.base = malloc(4096);
            handshake_buf.len = 4096;

            rc = BIO_read(session->write_bio,
                          handshake_buf.base,
                          handshake_buf.len);
            handshake_buf.len = rc;

            uv_write(&session->writer,
                     (uv_stream_t *) &session->tcp_sock,
                     &handshake_buf, 1,
                     zl_session_write);
        }
        if(session->ssl_state != HTTP_SESSION_TLS_STATE_IO) {
            if (session->ssl_state == HTTP_SESSION_TLS_STATE_CLOSING) {
                zl_session_close((uv_handle_t *) &session->tcp_sock);
            }
            return;
        }
    }

    rc = SSL_read(session->ssl, buf->base, buf->len);
    info("readed size: %d", rc);
    if (rc <= 0) {
        if (rc == 0) {
            zl_session_close((uv_handle_t *) &session->tcp_sock);
        }
        else if (SSL_get_error(session->ssl, rc) != SSL_ERROR_WANT_READ) {
            error("session[%x] tls read error (part 2)", session);
            zl_session_close((uv_handle_t *) &session->tcp_sock);
        }
        return;
    }
    __session_enter_app(session, rc, buf);
}

void zl_session_read(uv_stream_t * stream,
                     ssize_t nread,
                     const uv_buf_t * buf)
{
    struct http_session * session =
        container_of(stream, struct http_session, tcp_sock);

    if (nread <= 0 && nread != EAGAIN) {
        uv_close((uv_handle_t *) stream, zl_session_close);
        return;
    }

    if (session->security) {
        info("session[%x] use tls/ssl", session);
        __session_security_read(session, nread, (uv_buf_t *) buf);
    }
    else {
        info("session[%x] use tcp", session);
        __session_enter_app(session, nread, buf);
    }
}

static void __session_init(struct http_session * const session,
                              uv_loop_t * loop)
{
    zl_sessions_rbnode_init(&session->node);
    zl_http_req_parser_init(&session->parser);
    zl_http_req_protocol_init(&session->req_protocol);
    zl_http_res_protocol_init(&session->res_protocol);
    uv_tcp_init(loop, &session->tcp_sock);

    session->is_websocket = false;
    zl_websocket_frame_init(&session->ws_cli_frame);
    zl_websocket_frame_init(&session->ws_ser_frame);
    zl_websocket_frame_parser_init(&session->ws_parser);

    session->buf = NULL;
    session->buf_size = 0;

    session->security = false;
    session->ssl = NULL;
    session->ssl_state = HTTP_SESSION_TLS_STATE_INIT;
    session->write_bio = NULL;
    session->read_bio = NULL;
}

static void __session_ssl_init(uv_stream_t * server,
                               struct http_session * const session)
{
    struct http * const http =
        container_of(server, struct http, tcp_handler);

    session->security = true;
    session->ssl = SSL_new(http->ctx);
    SSL_set_accept_state(session->ssl);

    session->read_bio = BIO_new(BIO_s_mem());
    session->write_bio = BIO_new(BIO_s_mem());
    BIO_set_nbio(session->read_bio, 1);
    BIO_set_nbio(session->write_bio, 1);
    SSL_set_bio(session->ssl, session->read_bio, session->write_bio);
}

void zl_session_new(uv_stream_t * server, int status)
{
    struct config * config = zl_config_get();
    int ret;
    if (status < 0) {
        error("new session error: %s", uv_strerror(status));
        return;
    }
    zl_sessions_init();

    struct http_session * session =
        (struct http_session *) malloc(sizeof(struct http_session));
    if (session == NULL) {
        error("new session error: malloc failed");
        return;
    }
    __session_init(session, server->loop);

    ret = uv_accept(server, (uv_stream_t *) &session->tcp_sock);
    if (ret < 0) {
        error("new session error: accept failed");
        return;
    }
    if (config->security) {
        info("session[%x] use security", session);
        __session_ssl_init(server, session);
    }

    info("accept a newly session[%x]", session);
    zl_sessions_add(session);

    uv_read_start((uv_stream_t *) &session->tcp_sock,
                  zl_session_readbuf_alloc,
                  zl_session_read);
}

void zl_session_register_webgetway_enter(zl_webgateway_enter_fptr fptr)
{
    __webgateway_fptr = fptr;
}

void zl_session_register_websocket_parser(zl_websocket_frame_parse_fptr fptr)
{
    __websocket_frame_parse_fptr = fptr;
}

void zl_session_register_req_protocol_parser(zl_http_req_protocol_parse_fptr fptr)
{
    __http_req_protocol_parse_fptr = fptr;
}

