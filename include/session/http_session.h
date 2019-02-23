#ifndef _ZL_SESSION_HTTP_SESSION_H
#define _ZL_SESSION_HTTP_SESSION_H

#include "utils/rbtree.h"
#include "utils/linked_list.h"
#include "http/request.h"
#include "http/response.h"
#include "http/websocket.h"
#include <openssl/ssl.h>
#include <uv.h>
#include <time.h>

enum http_session_tls_state {
    HTTP_SESSION_TLS_STATE_INIT,
    HTTP_SESSION_TLS_STATE_HANDSHAKE,
    HTTP_SESSION_TLS_STATE_IO,
    HTTP_SESSION_TLS_STATE_CLOSING
};

struct http_session_writable_node {
    struct llnode node;
    uv_write_t writer;
    uv_buf_t buf;
};

struct http_session {
    struct rbnode node;
    uv_tcp_t tcp_sock;
    struct http_req_parser parser;
    struct http_req_protocol req_protocol;
    struct http_res_protocol res_protocol;

    bool is_websocket;
    struct websocket_frame_parser ws_parser;
    struct websocket_frame ws_cli_frame;
    struct websocket_frame ws_ser_frame;

    void * buf;
    size_t buf_size;

    bool security;
    SSL * ssl;
    enum http_session_tls_state ssl_state;
    BIO * write_bio;
    BIO * read_bio;

    struct llnode writable_queue;

    uv_work_t worker;
    time_t last_active;
};

#endif
