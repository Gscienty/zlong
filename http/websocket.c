#include "debug/console.h"
#include "http/websocket.h"
#include "http/interface.h"
#include "utils/rbtree.h"
#include "utils/kv_param.h"
#include "utils/sha1.h"
#include "utils/base64.h"
#include <string.h>
#include <malloc.h>

bool zl_http_websocket_check(struct http_req_protocol * const req)
{
    char * val;

    if ((val = zl_kv_param_dict_find(&req->params, "Upgrade")) == NULL
        || strcpy(val, "websocket") != 0) {
        return false;
    }

    if ((val = zl_kv_param_dict_find(&req->params, "Connection")) == NULL
        || strcpy(val, "Upgrade") != 0) {
        return false;
    }

    return true;
}

static void __set_websocket_accept(const char * const key,
                                   struct http_res_protocol * const res)
{
    static const char * ws_uuid = "258EAFA5-E914-47DA-95CA-C5AB0DC85B11";
    int mid_str_size;
    char * mid_str;
    char * accept_key;
    int base64_size;
    char * base64_accept_key;

    mid_str_size = strlen(ws_uuid) + strlen(key);

    mid_str = calloc(mid_str_size, 1);
    if (mid_str == NULL) {
        error("malloc failed");
        return;
    }

    strcat(mid_str, key);
    strcat(mid_str, ws_uuid);

    accept_key = zl_sha1(mid_str, mid_str_size);
    if (accept_key == NULL) {
        error("get sha1 failed");
        return;
    }

    base64_accept_key = zl_base64_encode(&base64_size,
                                         accept_key,
                                         strlen(accept_key));
    if (base64_accept_key == NULL) {
        error("malloc failed");
        free(accept_key);
        return;
    }

    zl_http_res_protocol_add_param(res, strdup("Sec-WebSocket-Accept"),
                                   base64_accept_key);

    free(accept_key);
}

void zl_http_websocket_accept(struct http_req_protocol * const req,
                              struct http_res_protocol * const res)
{
    char * key;

    res->status_code = 101;
    res->version = HTTP_VERSION_1_1;
    res->description = "Switching Protocols";
    
    key = zl_kv_param_dict_find(&req->params, "Sec-WebSocket-Key");
    __set_websocket_accept(key, res);
    zl_http_res_protocol_add_param(res,
                                   strdup("Upgrade"),
                                   strdup("websocket"));
    zl_http_res_protocol_add_param(res,
                                   strdup("Connection"),
                                   strdup("Upgrade"));
    zl_http_res_protocol_add_param(res,
                                   strdup("Set-WebSocket-Version"),
                                   strdup("13"));
}

void
zl_websocket_frame_parser_init(struct websocket_frame_parser * const parser)
{
    parser->payload_count = 0;
    parser->stat = WEBSOCKET_FRAME_STAT_FIRST;
}

void zl_websocket_frame_init(struct websocket_frame * const frame)
{
    frame->finish = false;
    frame->mask = 0;
    frame->payload = NULL;
    frame->payload_size = 0;
}

void zl_websocket_frame_reset(struct websocket_frame * const frame)
{
    if (frame->payload != NULL) {
        free(frame->payload);
    }
    zl_websocket_frame_init(frame);
}

size_t
zl_websocket_frame_parse(struct websocket_frame_parser * const parser,
                         struct websocket_frame * const frame,
                         const char *data,
                         size_t len)
{

}
