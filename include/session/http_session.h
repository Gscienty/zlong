#ifndef _ZL_SESSION_HTTP_SESSION_H
#define _ZL_SESSION_HTTP_SESSION_H

#include "utils/rbtree.h"
#include "http/request.h"
#include "http/response.h"
#include "http/websocket.h"

#include <uv.h>

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

#endif
