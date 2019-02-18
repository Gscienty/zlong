#include "session/session_handler.h"
#include "http/websocket.h"
#include "http/web_gateway.h"
#include "http/interface.h"
#include "debug/console.h"
#include <malloc.h>

/**
 * init http
 * @param http
 * 
 */
int zl_http_init(struct http * const http)
{
    int ret;
    ret = uv_loop_init(&http->event_looper);
    if (ret < 0) {
        error("loop init error");
        return -1;
    }

    zl_session_register_webgetway_enter(zl_webgateway_enter);
    zl_session_register_websocket_parser(zl_websocket_frame_parse);
    zl_session_register_req_protocol_parser(zl_http_req_protocol_parse);

    ret = uv_tcp_init(&http->event_looper, &http->tcp_handler);
    if (ret < 0) {
        error("tcp server init error");
        return ret;
    }
    if (http->delay != 0) {
        ret = uv_tcp_keepalive(&http->tcp_handler, 1, http->delay);
        if (ret < 0) {
            error("keepalive error");
            return ret;
        }
    }
    ret = uv_tcp_bind(&http->tcp_handler, (struct sockaddr *) &http->addr, 0);
    if (ret < 0) {
        error("tcp bind failed");
        return ret;
    }
    ret = uv_listen((uv_stream_t *) &http->tcp_handler,
                    http->backlog,
                    zl_session_new);
    if (ret < 0) {
        error("listen failed");
        return ret;
    }

    info("http init success.");

    return ret;
}

/**
 * set no delay
 * @param http
 * 
 */
int zl_http_nodelay(struct http * const http)
{
    return uv_tcp_nodelay(&http->tcp_handler, 1);
}

/**
 * simultaneous accepts
 * @param http
 * 
 */
int zl_http_simultaneous(struct http * const http)
{
    return uv_tcp_simultaneous_accepts(&http->tcp_handler, 1);
}

/**
 * run
 * @param http
 * 
 */
int zl_http_run(struct http * const http)
{
    return uv_run(&http->event_looper, UV_RUN_DEFAULT);
}
