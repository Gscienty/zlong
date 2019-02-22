#include "debug/console.h"
#include "utils/kv_param.h"
#include "http/response.h"
#include <stdio.h>
#include <malloc.h>
#include <string.h>

void zl_http_res_protocol_init(struct http_res_protocol * const res)
{
    res->payload              = NULL;
    res->payload_size         = 0;
    res->status_code          = 200;
    res->description          = NULL;
    res->version              = HTTP_VERSION_1_1;
    res->tcp_payload          = NULL;
    res->tcp_payload_size     = 0;
    res->tcp_payload_writable = 0;
    rbtree_root_init(&res->params);
}

void zl_http_res_protocol_clear(struct http_res_protocol * const res)
{
    if (res->payload != NULL) {
        free(res->payload);
        res->payload = NULL;
    }
    if (res->tcp_payload != NULL) {
        free(res->tcp_payload);
        res->tcp_payload = NULL;
    }
    info("res clear param");
    zl_kv_param_dict_clear(&res->params);

    zl_http_res_protocol_init(res);
}

void zl_http_res_protocol_add_param(struct http_res_protocol * const res,
                                    char * key,
                                    char * val)
{
    struct kv_param * param = 
        (struct kv_param *) malloc(sizeof(struct kv_param));
    if (param == NULL)
        return;

    zl_kv_param_init(param, &res->params);
    info("res append param %s: %s", key, val);
    zl_kv_param_set(param, key, val);

    zl_kv_param_dict_add(&res->params, param);
}

static void
__default_param_content_length(struct http_res_protocol * const res)
{
    char * val =
        zl_kv_param_dict_find(&res->params, "Content-Length");
    if (val != NULL)
        return;
    val = zl_kv_param_dict_find(&res->params, "Transfer-Encoding");
    if (val != NULL && strcmp(val, "chunked") == 0)
        return;

    snprintf(res->tmp, HTTP_RES_TMP_MAX_SIZE, "%ld", res->payload_size);

    zl_http_res_protocol_add_param(res,
                                   strdup("Content-Length"),
                                   strdup(res->tmp));
}

static void __default_param_connection(struct http_res_protocol * const res)
{
    char * val =
        zl_kv_param_dict_find(&res->params, "Connection");
    if (val != NULL)
        return;

    zl_http_res_protocol_add_param(res,
                                   strdup("Connection"),
                                   strdup("keep-alive"));
}

void zl_http_res_protocol_default_params(struct http_res_protocol * const res)
{
    __default_param_connection(res);
    __default_param_content_length(res);
}

static bool __tcp_payload_realloc(struct http_res_protocol * const res)
{
    if (res->tcp_payload == NULL) {
        res->tcp_payload = malloc(HTTP_RES_TCP_PAYLOAD_BLOCK);
        if (res->tcp_payload == NULL)
            return false;
        res->tcp_payload_size = HTTP_RES_TCP_PAYLOAD_BLOCK;
        res->tcp_payload_writable = 0;
    }
    else {
        res->tcp_payload = realloc(res->tcp_payload,
                                   res->tcp_payload_size
                                   + HTTP_RES_TCP_PAYLOAD_BLOCK);
        if (res->tcp_payload == NULL)
            return false;
        res->tcp_payload_size += HTTP_RES_TCP_PAYLOAD_BLOCK;
    }
    return true;
}

inline static size_t __tcp_payload_remain(struct http_res_protocol * const res)
{
    if (res->tcp_payload_size >= res->tcp_payload_writable)
        return res->tcp_payload_size - res->tcp_payload_writable;
    return 0;
}

inline static char * __tcp_payload_position(struct http_res_protocol * const res)
{
    return res->tcp_payload + res->tcp_payload_writable;
}

static bool __status_code_serialize(struct http_res_protocol * const res)
{
    size_t remain_size = __tcp_payload_remain(res);
    while (remain_size < 4) {
        if (!__tcp_payload_realloc(res))
            return false;
        remain_size = __tcp_payload_remain(res);
    }

    snprintf(__tcp_payload_position(res), 5, "%d ", res->status_code);
    res->tcp_payload_writable += 4;
    return true;
}

static struct http_res_status_code_desc __desc[] = {
    { 100, "Continue" },
    { 101, "Switching Protocols" },
    { 102, "Processing" },
    { 200, "OK" },
    { 201, "Created" },
    { 202, "Accepted" },
    { 203, "Non-Authoriative Information" },
    { 204, "No Content" },
    { 205, "Reset Content" },
    { 206, "Partial Content" },
    { 207, "Multi-Status" },
    { 300, "Multiple Choices" },
    { 301, "Moved Permanently" },
    { 302, "Move temporarily" },
    { 303, "See Other" },
    { 304, "Not Modified" },
    { 305, "Use Proxy" },
    { 306, "Switch Proxy" },
    { 307, "Temporary Redirect" },
    { 400, "Bad Request" },
    { 401, "Unauthorized" },
    { 402, "Payment Required" },
    { 403, "Forbidden" },
    { 404, "Not Found" },
    { 405, "Method Not Allowed" },
    { 406, "Not Acceptable" },
    { 500, "Internal Server Error" },
    { 501, "Not Implemented" },
    { 502, "Bad Gateway" }
};

static bool
__status_code_auto_desc_serialize(struct http_res_protocol * const res)
{
    int end = sizeof(__desc) / sizeof(struct http_res_status_code_desc);
    int start = 0;
    int mid;
    const char * desc;
    size_t desc_size;
    size_t remain_size;

    while (start <= end) {
        mid = (start + end) / 2;
        if (res->status_code == __desc[mid].status_code) {
            break;
        }
        else if (res->status_code < __desc[mid].status_code) {
            if (end == mid) {
                mid = -1;
                break;
            }
            end = mid;
        }
        else {
            if (start == mid) {
                mid = -1;
                break;
            }
            start = end;
        }
    }

    if (mid == -1) {
        return false;
    }
    else {
        desc = __desc[mid].desc;
    }
    desc_size = strlen(desc) + 2;

    remain_size = __tcp_payload_remain(res);
    while (desc_size > remain_size) {
        if (!__tcp_payload_realloc(res))
            return false;
        remain_size = __tcp_payload_remain(res);
    }

    snprintf(__tcp_payload_position(res), desc_size + 1, "%s\r\n", desc);
    res->tcp_payload_writable += desc_size;

    return true;

}

static bool
__status_code_spec_desc_serialize(struct http_res_protocol * const res)
{
    size_t desc_size = strlen(res->description) + 2;
    size_t remain_size = __tcp_payload_remain(res);
    while (desc_size > remain_size) {
        if (!__tcp_payload_realloc(res))
            return false;
        remain_size = __tcp_payload_remain(res);
    }

    snprintf(__tcp_payload_position(res),
             desc_size + 1,
             "%s\r\n",
             res->description);
    res->tcp_payload_writable += desc_size;

    return true;
}

static bool __status_code_desc_serialize(struct http_res_protocol * const res)
{
    if (res->description == NULL) {
        return __status_code_auto_desc_serialize(res);
    }
    else {
        return __status_code_spec_desc_serialize(res);
    }
}

static struct http_res_version __versions[] = {
    { HTTP_VERSION_1_0, "HTTP/1.0" },
    { HTTP_VERSION_1_1, "HTTP/1.1" }
};

static bool __version_serialize(struct http_res_protocol * const res)
{
    int end = sizeof(__versions) / sizeof(struct http_res_version);
    int start = 0;
    int mid;
    const char * desc;
    size_t desc_size;
    size_t remain_size;

    while (start <= end) {
        mid = (start + end) / 2;
        if (res->version == __versions[mid].version) {
            break;
        }
        else if (res->version < __versions[mid].version) {
            if (end == mid) {
                mid = -1;
                break;
            }
            end = mid;
        }
        else {
            if (start == mid) {
                mid = -1;
                break;
            }
            start = end;
        }
    }

    if (mid == -1) {
        return false;
    }
    else {
        desc = __versions[mid].desc;
    }
    desc_size = strlen(desc) + 1;

    remain_size = __tcp_payload_remain(res);
    while (desc_size > remain_size) {
        if (!__tcp_payload_realloc(res))
            return false;
        remain_size = __tcp_payload_remain(res);
    }

    snprintf(__tcp_payload_position(res), desc_size + 1, "%s ", desc);
    res->tcp_payload_writable += desc_size;

    return true;
}

static bool __param_serialize(struct http_res_protocol * const res,
                              struct kv_param * const param)
{
    size_t param_size = strlen(param->key) + 2 + strlen(param->val) + 2;
    size_t remain_size = __tcp_payload_remain(res);
    while (param_size > remain_size) {
        if (!__tcp_payload_realloc(res))
            return false;
        remain_size = __tcp_payload_remain(res);
    }

    snprintf(__tcp_payload_position(res), param_size + 1, "%s: %s\r\n",
             param->key, param->val);
    res->tcp_payload_writable += param_size;
    
    return true;
}

static bool __params_serialize(struct http_res_protocol * const res)
{
    struct llnode stack;
    struct http_param_llnode * newly;
    struct http_param_llnode * top;

    ll_head_init(&stack);

    if (res->params.root != &res->params.nil) {
        newly = (struct http_param_llnode *)
            malloc(sizeof(struct http_param_llnode));
        newly->param = container_of(res->params.root, struct kv_param, node);
        ll_insert_before(&stack, &newly->node);
    }

    while (!ll_empty(&stack)) {
        top = container_of(stack.next, struct http_param_llnode, node);
        __param_serialize(res, top->param);

        if (top->param->node.left != &res->params.nil) {
            newly = (struct http_param_llnode *)
                malloc(sizeof(struct http_param_llnode));
            newly->param = container_of(top->param->node.left,
                                        struct kv_param,
                                        node);
            ll_insert_before(&stack, &newly->node);
        }

        if (top->param->node.right != &res->params.nil) {
            newly = (struct http_param_llnode *)
                malloc(sizeof(struct http_param_llnode));
            newly->param = container_of(top->param->node.right,
                                        struct kv_param,
                                        node);
            ll_insert_before(&stack, &newly->node);
        }

        ll_remove(&top->node);
        free(top);
    }
    return true;
}

static bool __crlf_serialize(struct http_res_protocol * const res)
{
    size_t remain_size = __tcp_payload_remain(res);
    while (2 > remain_size) {
        if (!__tcp_payload_realloc(res))
            return false;
        remain_size = __tcp_payload_remain(res);
    }

    snprintf(__tcp_payload_position(res), 3, "\r\n");
    res->tcp_payload_writable += 2;
    
    return true;
}

static bool __content_serialize(struct http_res_protocol * const res)
{
    size_t remain_size = __tcp_payload_remain(res);
    while (res->payload_size > remain_size) {
        if (!__tcp_payload_realloc(res))
            return false;
        remain_size = __tcp_payload_remain(res);
    }
    
    memcpy(__tcp_payload_position(res), res->payload, res->payload_size);

    res->tcp_payload_writable += res->payload_size;
    
    return true;
}

bool zl_http_res_protocol_serialize(struct http_res_protocol * const res)
{
    if (!__tcp_payload_realloc(res))
        return false;

    if (!__version_serialize(res))
        return false;
    if (!__status_code_serialize(res))
        return false;
    if (!__status_code_desc_serialize(res))
        return false;
    if (!__params_serialize(res))
        return false;
    if (!__crlf_serialize(res))
        return false;
    if (!__content_serialize(res))
        return false;

    return true;
}

