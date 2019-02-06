#ifndef _ZL_HTTP_WEBSOCKET_H
#define _ZL_HTTP_WEBSOCKET_H

#include "http/request.h"
#include "http/response.h"
#include "utils/rbtree.h"
#include <stdbool.h>
#include <stdint.h>

enum zl_websocket_opcode {
    ZL_WEBSOCKET_CONTINUE_OPCODE = 0x00,
    ZL_WEBSOCKET_TEXT_OPCODE     = 0x01,
    ZL_WEBSOCKET_BIN_OPCODE      = 0x02,
    ZL_WEBSOCKET_CLOSE_OPCODE    = 0x08,
    ZL_WEBSOCKET_PING_OPCODE     = 0x09,
    ZL_WEBSOCKET_PONG_OPCODE     = 0x0A
};

enum websocket_frame_stat {
    WEBSOCKET_FRAME_STAT_FIRST,
    WEBSOCKET_FRAME_STAT_SECOND,
    WEBSOCKET_FRAME_STAT_PAYLOAD_2_1,
    WEBSOCKET_FRAME_STAT_PAYLOAD_2_2,
    WEBSOCKET_FRAME_STAT_PAYLOAD_8_1,
    WEBSOCKET_FRAME_STAT_PAYLOAD_8_2,
    WEBSOCKET_FRAME_STAT_PAYLOAD_8_3,
    WEBSOCKET_FRAME_STAT_PAYLOAD_8_4,
    WEBSOCKET_FRAME_STAT_PAYLOAD_8_5,
    WEBSOCKET_FRAME_STAT_PAYLOAD_8_6,
    WEBSOCKET_FRAME_STAT_PAYLOAD_8_7,
    WEBSOCKET_FRAME_STAT_PAYLOAD_8_8,
    WEBSOCKET_FRAME_STAT_MASK_1,
    WEBSOCKET_FRAME_STAT_MASK_2,
    WEBSOCKET_FRAME_STAT_MASK_3,
    WEBSOCKET_FRAME_STAT_MASK_4,
    WEBSOCKET_FRAME_STAT_PAYLOAD,
    WEBSOCKET_FRAME_STAT_END,
    WEBSOCKET_FRAME_STAT_ERROR
};

struct websocket_frame_parser {
    enum websocket_frame_stat stat;
    size_t payload_count;
};

struct websocket_frame {
    bool finish;
    bool mask;
    char mask_key[4];
    enum zl_websocket_opcode opcode;
    char * payload;
    size_t payload_size;

    char * tcp_payload;
    size_t tcp_payload_writable;
    size_t tcp_payload_size;
};

#define WEBSOCKET_TCP_PAYLOAD_BLOCK 4096

bool zl_websocket_check(struct http_req_protocol * const req);

void zl_websocket_accept(struct http_req_protocol * const req,
                         struct http_res_protocol * const res);

void
zl_websocket_frame_parser_init(struct websocket_frame_parser * const parser);

void zl_websocket_frame_init(struct websocket_frame * const frame);

void zl_websocket_frame_reset(struct websocket_frame * const frame);

size_t
zl_websocket_frame_parse(struct websocket_frame_parser * const parser,
                         struct websocket_frame * const frame,
                         const unsigned char *data,
                         size_t len);

bool zl_websocket_frame_serialize(struct websocket_frame * const frame);

#endif
