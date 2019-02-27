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
    struct http_session_writable_node * writable;
    zl_http_req_protocol_reset(&session->req_protocol);
    zl_http_res_protocol_clear(&session->res_protocol);
    zl_websocket_frame_reset(&session->ws_cli_frame);
    zl_websocket_frame_reset(&session->ws_ser_frame);

    if (session->buf != NULL) {
        free(session->buf);
    }

    if (session->security && session->ssl != NULL) {
        SSL_free(session->ssl);
        session->ssl = NULL;
    }

    while (!ll_empty(&session->writable_queue)) {
        writable = container_of(session->writable_queue.next,
                                struct http_session_writable_node,
                                node);
        
        ll_remove(&writable->node);
        if (writable->buf.base != NULL) {
            free(writable->buf.base);
        }
        free(writable);
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

struct http_session_writable_node * __new_session_writable_node()
{
    struct http_session_writable_node * ret =
        calloc(1, sizeof(struct http_session_writable_node));

    return ret;
}

void zl_session_close(uv_handle_t * handle)
{
    struct http_session * session =
        container_of(handle, struct http_session, tcp_sock);
    info("close session[%x]", session);
    zl_session_destory(session);
}

static void __session_write(uv_write_t * req, int status)
{
    struct http_session_writable_node * node =
        container_of(req, struct http_session_writable_node, writer);

    info("writed stream_write_queue_size: %d", node->writer.handle->write_queue_size);
    ll_remove(&node->node);
    if (node->buf.base != NULL) {
        free(node->buf.base);
    }
    free(node);
    if (status < 0) {
        warn("write error %d", status);
        return;
    }
}

static void __security_respond(struct http_session * const session)
{
    uv_buf_t buf;
    int readed_size;
    struct http_session_writable_node * writable;
    buf.base = malloc(4096);
    buf.len = 4096;

    while ((readed_size = BIO_read(session->write_bio, buf.base, buf.len)) > 0) {
        writable =__new_session_writable_node();
        ll_insert_before(&session->writable_queue, &writable->node);

        writable->buf.len = readed_size;
        info("security respond: %d", readed_size);
        writable->buf.base = malloc(writable->buf.len);
        memcpy(writable->buf.base, buf.base, readed_size);

        info("stream_write_queue_size: %d", session->tcp_sock.write_queue_size);
        uv_write(&writable->writer,
                 (uv_stream_t *) &session->tcp_sock,
                 &writable->buf, 1,
                 __session_write);
    }
    BIO_flush(session->write_bio);

    free(buf.base);
}

bool zl_session_http_respond(uv_stream_t * stream,
                             struct http_session * const session)
{
    struct http_session_writable_node * writable;

    if (!zl_http_res_protocol_serialize(&session->res_protocol)) {
        return false;
    }

    if (session->security) {
        SSL_write(session->ssl,
                  session->res_protocol.tcp_payload,
                  session->res_protocol.tcp_payload_size);

        __security_respond(session);
    }
    else {
        writable = __new_session_writable_node();
        ll_insert_before(&session->writable_queue, &writable->node);

        writable->buf.base = session->res_protocol.tcp_payload;
        writable->buf.len  = session->res_protocol.tcp_payload_writable;
        uv_write(&writable->writer,
                 stream,
                 &writable->buf, 1,
                 __session_write);
    }

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

    if (!zl_session_http_respond(stream, session)) {
        uv_close((uv_handle_t *) stream, zl_session_close);
        return;
    }

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
                time(&session->last_active);
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
                time(&session->last_active);
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

static bool __session_security_init(struct http_session * const session)
{
    if (session->ssl_state == HTTP_SESSION_TLS_STATE_INIT
        || session->ssl_state == HTTP_SESSION_TLS_STATE_HANDSHAKE) {
        __session_security_handshake(session);
        if(session->ssl_state == HTTP_SESSION_TLS_STATE_HANDSHAKE
           || session->ssl_state == HTTP_SESSION_TLS_STATE_IO) {
            __security_respond(session);
        }
        if(session->ssl_state != HTTP_SESSION_TLS_STATE_IO) {
            if (session->ssl_state == HTTP_SESSION_TLS_STATE_CLOSING) {
                uv_close((uv_handle_t *) &session->tcp_sock, zl_session_close);
            }
            return false;
        }
    }
    return true;
}

static void __session_security_read_data(struct http_session * const session,
                                         uv_buf_t * buf)
{
    int rc;
    rc = SSL_read(session->ssl, buf->base, buf->len);
    info("readed size: %d", rc);
    if (rc <= 0) {
        if (rc == 0) {
            uv_close((uv_handle_t *) &session->tcp_sock, zl_session_close);
        }
        else if (SSL_get_error(session->ssl, rc) != SSL_ERROR_WANT_READ) {
            error("session[%x] tls read error (part 2)", session);
            uv_close((uv_handle_t *) &session->tcp_sock, zl_session_close);
        }
        return;
    }
    __session_enter_app(session, rc, buf);
}

static void __session_security_read(struct http_session * const session,
                                    ssize_t nread,
                                    uv_buf_t * buf)
{
    int rc;
    rc = BIO_write(session->read_bio, buf->base, nread);
    if (!__session_security_init(session)) {
        return;
    }

    __session_security_read_data(session, buf);
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

    ll_head_init(&session->writable_queue);

    time(&session->last_active);
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

void __thread_work(uv_work_t * req)
{
    struct http_session * session =
        container_of(req, struct http_session, worker);

    uv_read_start((uv_stream_t *) &session->tcp_sock,
                  zl_session_readbuf_alloc,
                  zl_session_read);
}

void __thread_after_work(uv_work_t * req, int status)
{
    (void) req;
    (void) status;
}

void zl_session_new(uv_stream_t * server, int status)
{
    struct config * config = zl_config_get();
    /*struct http * http = container_of(server, struct http, tcp_handler);*/
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
    /*uv_queue_work(&http->event_looper,*/
                  /*&session->worker,*/
                  /*__thread_work,*/
                  /*__thread_after_work);*/
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

