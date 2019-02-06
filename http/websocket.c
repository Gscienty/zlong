#include "debug/console.h"
#include "http/websocket.h"
#include "http/interface.h"
#include "utils/rbtree.h"
#include "utils/kv_param.h"
#include "utils/sha1.h"
#include "utils/base64.h"
#include <string.h>
#include <malloc.h>

bool zl_websocket_check(struct http_req_protocol * const req)
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

void zl_websocket_accept(struct http_req_protocol * const req,
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
    frame->finish               = false;
    frame->mask                 = 0;
    frame->opcode               = ZL_WEBSOCKET_TEXT_OPCODE;
    frame->payload              = NULL;
    frame->payload_size         = 0;
    frame->tcp_payload          = NULL;
    frame->tcp_payload_size     = 0;
    frame->tcp_payload_writable = 0;
}

void zl_websocket_frame_reset(struct websocket_frame * const frame)
{
    if (frame->payload != NULL) {
        free(frame->payload);
    }
    if (frame->tcp_payload != NULL) {
        free(frame->tcp_payload);
    }
    zl_websocket_frame_init(frame);
}

static void __parse_first_byte(struct websocket_frame_parser * const parser,
                               struct websocket_frame * const frame,
                               uint8_t byte)
{
    frame->finish = (byte & 0x80) != 0;
    frame->opcode = (enum zl_websocket_opcode) (byte & 0x0F);

    parser->stat = WEBSOCKET_FRAME_STAT_SECOND;
}

static void __parse_second_byte(struct websocket_frame_parser * const parser,
                                struct websocket_frame * const frame,
                                uint8_t byte)
{
    frame->mask = (byte & 0x80) != 0;
    frame->payload_size = (byte & 0x7F);

    if (frame->payload_size == 126) {
        frame->payload_size = 0;
        parser->stat = WEBSOCKET_FRAME_STAT_PAYLOAD_2_1;
    }
    else if (frame->payload_size == 127) {
        frame->payload_size = 0;
        parser->stat = WEBSOCKET_FRAME_STAT_PAYLOAD_8_1;
    }
    else {
        frame->payload = malloc(frame->payload_size);
        parser->payload_count = 0;
        if (frame->mask)
            parser->stat = WEBSOCKET_FRAME_STAT_MASK_1;
        else
            parser->stat = WEBSOCKET_FRAME_STAT_PAYLOAD;
    }
}

size_t
zl_websocket_frame_parse(struct websocket_frame_parser * const parser,
                         struct websocket_frame * const frame,
                         const unsigned char *data,
                         size_t len)
{
    size_t used_size = 0;    
    size_t readable;
    char c;

    if (parser->stat == WEBSOCKET_FRAME_STAT_END
        || parser->stat == WEBSOCKET_FRAME_STAT_ERROR) {
        return 0;
    }

    while (used_size < len
           && parser->stat != WEBSOCKET_FRAME_STAT_END
           && parser->stat != WEBSOCKET_FRAME_STAT_ERROR) {
        if (parser->stat == WEBSOCKET_FRAME_STAT_PAYLOAD) {
            readable = frame->payload_size - parser->payload_count;
            if (readable > len - used_size)
                readable = len - used_size;

            memcpy(frame->payload + parser->payload_count,
                   data + used_size,
                   readable);
            parser->payload_count += readable;
            if (parser->payload_count == frame->payload_size)
                parser->stat = WEBSOCKET_FRAME_STAT_END;

            break;
        }

        c = data[used_size++];
        switch (parser->stat) {
        case WEBSOCKET_FRAME_STAT_FIRST:
            __parse_first_byte(parser, frame, c);
            break;
        case WEBSOCKET_FRAME_STAT_SECOND:
            __parse_second_byte(parser, frame, c);
        case WEBSOCKET_FRAME_STAT_PAYLOAD_2_1:
            frame->payload_size = ((size_t) c) << 8;
            parser->stat = WEBSOCKET_FRAME_STAT_PAYLOAD_2_2;
            break;
        case WEBSOCKET_FRAME_STAT_PAYLOAD_2_2:
            frame->payload_size |= ((size_t) c);
            if (frame->mask)
                parser->stat = WEBSOCKET_FRAME_STAT_MASK_1;
            else
                parser->stat = WEBSOCKET_FRAME_STAT_PAYLOAD;
            break;
        case WEBSOCKET_FRAME_STAT_PAYLOAD_8_1:
            frame->payload_size = ((size_t) c) << 56;
            parser->stat = WEBSOCKET_FRAME_STAT_PAYLOAD_8_2;
            break;
        case WEBSOCKET_FRAME_STAT_PAYLOAD_8_2:
            frame->payload_size |= ((size_t) c) << 48;
            parser->stat = WEBSOCKET_FRAME_STAT_PAYLOAD_8_3;
            break;
        case WEBSOCKET_FRAME_STAT_PAYLOAD_8_3:
            frame->payload_size |= ((size_t) c) << 40;
            parser->stat = WEBSOCKET_FRAME_STAT_PAYLOAD_8_4;
            break;
        case WEBSOCKET_FRAME_STAT_PAYLOAD_8_4:
            frame->payload_size |= ((size_t) c) << 32;
            parser->stat = WEBSOCKET_FRAME_STAT_PAYLOAD_8_5;
            break;
        case WEBSOCKET_FRAME_STAT_PAYLOAD_8_5:
            frame->payload_size |= ((size_t) c) << 24;
            parser->stat = WEBSOCKET_FRAME_STAT_PAYLOAD_8_6;
            break;
        case WEBSOCKET_FRAME_STAT_PAYLOAD_8_6:
            frame->payload_size |= ((size_t) c) << 16;
            parser->stat = WEBSOCKET_FRAME_STAT_PAYLOAD_8_7;
            break;
        case WEBSOCKET_FRAME_STAT_PAYLOAD_8_7:
            frame->payload_size |= ((size_t) c) << 8;
            parser->stat = WEBSOCKET_FRAME_STAT_PAYLOAD_8_8;
            break;
        case WEBSOCKET_FRAME_STAT_PAYLOAD_8_8:
            frame->payload_size |= ((size_t) c);
            if (frame->mask)
                parser->stat = WEBSOCKET_FRAME_STAT_MASK_1;
            else
                parser->stat = WEBSOCKET_FRAME_STAT_PAYLOAD;
            break;
        case WEBSOCKET_FRAME_STAT_MASK_1:
            frame->mask_key[0] = c;
            parser->stat = WEBSOCKET_FRAME_STAT_MASK_2;
            break;
        case WEBSOCKET_FRAME_STAT_MASK_2:
            frame->mask_key[1] = c;
            parser->stat = WEBSOCKET_FRAME_STAT_MASK_3;
            break;
        case WEBSOCKET_FRAME_STAT_MASK_3:
            frame->mask_key[2] = c;
            parser->stat = WEBSOCKET_FRAME_STAT_MASK_4;
            break;
        case WEBSOCKET_FRAME_STAT_MASK_4:
            frame->mask_key[3] = c;
            parser->stat = WEBSOCKET_FRAME_STAT_PAYLOAD;
            break;
        default:
            parser->stat = WEBSOCKET_FRAME_STAT_ERROR;
            break;
        }
    }

    return used_size;
}

inline static size_t __tcp_payload_remain(struct websocket_frame * const frame)
{
    if (frame->tcp_payload_size >= frame->tcp_payload_writable)
        return frame->tcp_payload_size - frame->tcp_payload_writable;
    return 0;
}

inline static char *
__tcp_payload_position(struct websocket_frame * const frame)
{
    return frame->tcp_payload + frame->tcp_payload_writable;
}

static bool __tcp_payload_realloc(struct websocket_frame * const frame)
{
    if (frame->tcp_payload == NULL) {
        frame->tcp_payload = malloc(WEBSOCKET_TCP_PAYLOAD_BLOCK);
        if (frame->tcp_payload == NULL)
            return false;
        frame->tcp_payload_size = WEBSOCKET_TCP_PAYLOAD_BLOCK;
        frame->tcp_payload_writable = 0;
    }
    else {
        frame->tcp_payload = realloc(frame->tcp_payload,
                                     frame->tcp_payload_size
                                     + WEBSOCKET_TCP_PAYLOAD_BLOCK);
        if (frame->tcp_payload == NULL)
            return false;
        frame->tcp_payload_size += WEBSOCKET_TCP_PAYLOAD_BLOCK;
    }
    return true;
}

static bool __first_serialize(struct websocket_frame * const frame)
{
    size_t remain_size = __tcp_payload_remain(frame);
    unsigned char byte = 0x00;
    while (1 > remain_size) {
        if (!__tcp_payload_realloc(frame))
            return false;
        remain_size = __tcp_payload_remain(frame);
    }

    if (frame->finish)
        byte |= 0x80;
    byte |= frame->opcode;

    frame->tcp_payload[0] = byte;
    frame->tcp_payload_writable++;

    return true;
}

static bool
__mask_and_payload_len_serialize(struct websocket_frame * const frame)
{
    size_t remain_size = __tcp_payload_remain(frame);
    size_t need_size;
    unsigned char byte;

    if (frame->payload_size < 126) {
        need_size = 1;
    }
    else if (frame->payload_size <= 0xFFFF) {
        need_size = 3;
    }
    else {
        need_size = 9;
    }
    while (need_size > remain_size) {
        if (!__tcp_payload_realloc(frame))
            return false;
        remain_size = __tcp_payload_remain(frame);
    }

    byte = 0x00;
    if (frame->mask)
        byte |= 0x80;
    if (frame->payload_size < 126) {
        byte |= frame->payload_size;
        *__tcp_payload_position(frame) = byte;
        frame->tcp_payload_writable++;
    }
    else if (frame->payload_size <= 0xFFFF) {
        byte |= 126;
        *__tcp_payload_position(frame) = byte;
        frame->tcp_payload_writable++;
        
        *__tcp_payload_position(frame) = (frame->payload_size & 0xFF00) >> 8;
        frame->tcp_payload_writable++;
        *__tcp_payload_position(frame) = frame->payload_size & 0x00FF;
        frame->tcp_payload_writable++;
    }
    else {
        byte |= 127;
        *__tcp_payload_position(frame) = byte;
        frame->tcp_payload_writable++;

        *__tcp_payload_position(frame) = (frame->payload_size & 0xFF00000000000000) >> 56;
        frame->tcp_payload_writable++;
        *__tcp_payload_position(frame) = (frame->payload_size & 0x00FF000000000000) >> 48;
        frame->tcp_payload_writable++;
        *__tcp_payload_position(frame) = (frame->payload_size & 0x0000FF0000000000) >> 40;
        frame->tcp_payload_writable++;
        *__tcp_payload_position(frame) = (frame->payload_size & 0x000000FF00000000) >> 32;
        frame->tcp_payload_writable++;
        *__tcp_payload_position(frame) = (frame->payload_size & 0x00000000FF000000) >> 24;
        frame->tcp_payload_writable++;
        *__tcp_payload_position(frame) = (frame->payload_size & 0x0000000000FF0000) >> 16;
        frame->tcp_payload_writable++;
        *__tcp_payload_position(frame) = (frame->payload_size & 0x000000000000FF00) >> 8;
        frame->tcp_payload_writable++;
        *__tcp_payload_position(frame) = frame->payload_size & 0x00000000000000FF;
        frame->tcp_payload_writable++;
    }

    return true;
}

static bool __mask_serialize(struct websocket_frame * const frame)
{
    size_t remain_size;
    if (!frame->mask) {
        return true;
    }

    remain_size = __tcp_payload_remain(frame);
    while (4 > remain_size) {
        if (!__tcp_payload_realloc(frame))
            return false;
        remain_size = __tcp_payload_remain(frame);
    }

    memmove(__tcp_payload_position(frame), frame->mask_key, 4);
    frame->tcp_payload_writable += 4;

    return true;
}

static bool __payload_serialize(struct websocket_frame * const frame)
{
    size_t remain_size = __tcp_payload_remain(frame);
    while (frame->payload_size > remain_size) {
        if (!__tcp_payload_realloc(frame))
            return false;
        remain_size = __tcp_payload_remain(frame);
    }

    memmove(__tcp_payload_position(frame), frame->payload, frame->payload_size);
    frame->tcp_payload_writable += frame->payload_size;

    return true;
}

bool zl_websocket_frame_serialize(struct websocket_frame * const frame)
{
    if (!__tcp_payload_realloc(frame))
        return false;

    if (!__mask_and_payload_len_serialize(frame))
        return false;
    if (!__mask_serialize(frame))
        return false;
    if (!__payload_serialize(frame))
        return false;

    return true;
}
