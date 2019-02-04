#ifndef _ZL_HTTP_RESPONSE_H
#define _ZL_HTTP_RESPONSE_H

#include "utils/linked_list.h"
#include "utils/version.h"
#include "utils/rbtree.h"
#include <stdbool.h>

#define HTTP_RES_TMP_MAX_SIZE 32
#define HTTP_RES_TCP_PAYLOAD_BLOCK 4096

struct http_res_protocol {
    short status_code;
    char * description;
    enum http_version version;
    struct rbroot params;    

    void * payload;
    size_t payload_size;

    void * tcp_payload;
    size_t tcp_payload_writable;
    size_t tcp_payload_size;

    char tmp[HTTP_RES_TMP_MAX_SIZE];
};

struct http_res_status_code_desc {
    short status_code;
    const char * desc;
};

struct http_res_version {
    enum http_version version;
    const char * desc;
};

struct http_param_llnode {
    struct llnode node;
    struct kv_param * param;
};

void zl_http_res_protocol_init(struct http_res_protocol * const res);

void zl_http_res_protocol_clear(struct http_res_protocol * const res);

void zl_http_res_protocol_add_param(struct http_res_protocol * const res,
                                    char * key,
                                    char * val);

void
zl_http_res_protocol_default_params(struct http_res_protocol * const res);

bool zl_http_res_protocol_serialize(struct http_res_protocol * const res);

#endif
