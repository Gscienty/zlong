#include "debug/console.h"
#include "session/session_handler.h"
#include <malloc.h>

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

    zl_sessions_remove(session);
    free(session);
}

void zl_session_readbuf_alloc(uv_handle_t * handle,
                              size_t suggested_size,
                              uv_buf_t * buf)
{
    struct http_session * session = zl_sessions_find((uv_tcp_t *) handle);
    if (session == NULL) {
        buf->base = NULL;
        buf->len = 0;
        return;
    }

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

    buf.base = session->res_protocol.tcp_payload;
    buf.len  = session->res_protocol.tcp_payload_writable;

    session->res_protocol.tcp_payload          = NULL;
    session->res_protocol.tcp_payload_size     = 0;
    session->res_protocol.tcp_payload_writable = 0;

    uv_write(&session->writer, stream, &buf, 1, zl_session_write);
    return true;
}

void zl_session_http_reset(uv_stream_t * stream,
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

void zl_session_read(uv_stream_t * stream,
                     ssize_t nread,
                     const uv_buf_t * buf)
{
    struct http_session * session;

    session = zl_sessions_find((uv_tcp_t *) stream);
    if (session == NULL) {
        error("session not found");
        return;
    }

    if (nread <= 0 && nread != EAGAIN) {
        uv_close((uv_handle_t *) stream, zl_session_close);
        return;
    }

    info("parse http request: %x", session);

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
            uv_close((uv_handle_t *) stream, zl_session_close);
        }
    }
    else {
        if (__webgateway_fptr != NULL
            && __http_req_protocol_parse_fptr != NULL) {
            __http_req_protocol_parse_fptr(&session->parser,
                                           &session->req_protocol,
                                           buf->base,
                                           nread);
            if (session->parser.stat == HTTP_REQ_STAT_END) {
                info("parsed http req protocol");
                __webgateway_fptr(session);
                zl_session_http_reset(stream, session);
            }
        }
        else {
            uv_close((uv_handle_t *) stream, zl_session_close);
        }
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
}

void zl_session_new(uv_stream_t *server, int status)
{
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
