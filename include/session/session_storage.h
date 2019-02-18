#ifndef _ZL_SESSION_SESSION_STORAGE_H
#define _ZL_SESSION_SESSION_STORAGE_H

#include "utils/rbtree.h"
#include "http/request.h"
#include "http/response.h"
#include "http/websocket.h"
#include <netinet/in.h>
#include <uv.h>

struct http {
    union {
        struct sockaddr_in v4;
        struct sockaddr_in6 v6;
    } addr;
    unsigned int delay;
    uv_loop_t event_looper;
    uv_tcp_t tcp_handler;
    int backlog;
};

struct http_session {
    struct rbnode node;
    uv_tcp_t tcp_sock;
    struct http_req_parser parser;
    struct http_req_protocol req_protocol;
    struct http_res_protocol res_protocol;
    uv_write_t writer;

    bool is_websocket;
    struct websocket_frame_parser ws_parser;
    struct websocket_frame ws_cli_frame;
    struct websocket_frame ws_ser_frame;

    void * buf;
    size_t buf_size;
};

typedef void (*zl_webgateway_enter_fptr) (struct http_session * const session);
typedef size_t
(*zl_websocket_frame_parse_fptr) (struct websocket_frame_parser * const parser,
                                  struct websocket_frame * const frame,
                                  const unsigned char * data,
                                  size_t len);
typedef size_t
(*zl_http_req_protocol_parse_fptr) (struct http_req_parser * const parser,
                                    struct http_req_protocol * const req,
                                    const char * data,
                                    size_t len);

void zl_sessions_add(struct http_session * const session);

struct http_session * zl_sessions_find(uv_tcp_t * const tcp_sock);

void zl_session_destory(struct http_session * const session);

void zl_session_readbuf_alloc(uv_handle_t * handle,
                              size_t suggested_size,
                              uv_buf_t * buf);

void zl_session_close(uv_handle_t * handle);

void zl_session_write(uv_write_t *req, int status);

bool zl_session_http_respond(uv_stream_t * stream,
                             struct http_session * const session);

void zl_session_http_reset(uv_stream_t * stream,
                           struct http_session * session);

void zl_session_websocket_reset(struct http_session * session);

void zl_session_read(uv_stream_t * stream,
                     ssize_t nread,
                     const uv_buf_t * buf);

void zl_session_new(uv_stream_t *server, int status);

void zl_session_register_webgetway_enter(zl_webgateway_enter_fptr fptr);

void zl_session_register_websocket_parser(zl_websocket_frame_parse_fptr fptr);

void zl_session_register_req_protocol_parser(zl_http_req_protocol_parse_fptr fptr);


#endif
