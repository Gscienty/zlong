#include "utils/kv_param.h"
#include "http/parser.h"
#include "debug/console.h"
#include <stdbool.h>
#include <string.h>
#include <malloc.h>
#include <stdlib.h>

/**
 * init parser
 * @param parser: parser
 * 
 */
void zl_http_req_parser_init(struct http_req_parser * const parser)
{
    parser->method_used      = 0;
    parser->uri_used         = 0;
    parser->version_used     = 0;
    parser->stat             = HTTP_REQ_STAT_HEADER_METHOD;
    parser->content_length   = 0;
    parser->content_received = 0;
}

/**
 * init req protocol
 * @param req
 * 
 */
void zl_http_req_protocol_init(struct http_req_protocol * const req)
{
    req->method                = 0;
    req->uri                   = NULL;
    req->version               = 0;
    req->payload               = NULL;
    req->payload_size          = 0;

    rbtree_root_init(&req->params);
}


/**
 * reset req protocol
 * 
 */
void zl_http_req_protocol_reset(struct http_req_protocol * const req)
{
    if (req->uri != NULL)
        free(req->uri);
    if (req->payload != NULL)
        free(req->payload);

    zl_kv_param_dict_clear(&req->params);

    zl_http_req_protocol_init(req);
}

static bool __parse_header_method(struct http_req_parser * const parser,
                                  const char c)
{
    if (parser->method_used >= HTTP_REQ_METHOD_MAX_SIZE - 1)
        return false;
    parser->method[parser->method_used++] = c;
    parser->method[parser->method_used]   = 0;
    return true;
}

static bool __set_method(struct http_req_protocol * const req,
                         struct http_req_parser * const parser)
{
    // ugly
    if (strcmp(parser->method, "GET") == 0)
        req->method = HTTP_REQ_METHOD_GET;
    else if (strcmp(parser->method, "POST") == 0)
        req->method = HTTP_REQ_METHOD_GET;

    return true;
}

static bool __parse_header_uri(struct http_req_parser * const parser,
                               const char c)
{
    if (parser->uri_used >= HTTP_REQ_URI_MAX_SIZE - 1)
        return false;
    parser->uri[parser->uri_used++] = c;
    parser->uri[parser->uri_used]   = 0;
    return true;
}

static bool __set_uri(struct http_req_protocol * const req,
                      struct http_req_parser * const parser)
{
    req->uri = strdup(parser->uri);
    return true;
}

static bool __parse_header_version(struct http_req_parser * const parser,
                                   const char c)
{
    if (parser->version_used >= HTTP_REQ_VERSION_MAX_SIZE - 1)
        return false;
    parser->version[parser->version_used++] = c;
    parser->version[parser->version_used]   = 0;
    return true;
}

static bool __set_version(struct http_req_protocol * const req,
                          struct http_req_parser * const parser)
{
    (void) parser;
    req->version = HTTP_VERSION_1_1;
    return true;
}

static bool __req_param_append(struct http_req_protocol * const req,
                               struct http_req_parser * const parser)
{
    char * delimiter = strchr(parser->param, ':');
    struct kv_param * param;

    if (delimiter == NULL)
        return false;
    param = (struct kv_param *) malloc(sizeof(struct kv_param));
    if (param == NULL)
        return false;
    zl_kv_param_init(param, &req->params);

    *delimiter = 0;
    delimiter += 2;

    zl_kv_param_set(param, strdup(parser->param), strdup(delimiter));

    if (!zl_kv_param_dict_add(&req->params, param)) {
        zl_kv_param_clear(param);
        free(param);

        return false;
    }

    parser->param_used = 0;
    return true;
}

static bool __parse_header_param(struct http_req_parser * const parser,
                                 const char c)
{
    if (parser->param_used >= HTTP_REQ_PARAM_MAX_SIZE - 1)
        return false;
    parser->param[parser->param_used++] = c;
    parser->param[parser->param_used]   = 0;
    return true;
}

static size_t __parse_header(struct http_req_parser * const parser,
                             struct http_req_protocol * const req,
                             const char * data,
                             size_t len)
{
    size_t used_size = 0;
    char c;
    if (parser->stat == HTTP_REQ_STAT_END
        || parser->stat == HTTP_REQ_STAT_HEADER_END
        || parser->stat == HTTP_REQ_STAT_ERROR) {
        return 0;
    }

    while (used_size < len
           && parser->stat != HTTP_REQ_STAT_END
           && parser->stat != HTTP_REQ_STAT_HEADER_END
           && parser->stat != HTTP_REQ_STAT_ERROR) {

        c = data[used_size++];
        switch (parser->stat) {
        default:
            break;
        case HTTP_REQ_STAT_HEADER_METHOD:
            if (c == ' ') {
                parser->stat = HTTP_REQ_STAT_HEADER_URI;
                __set_method(req, parser);
            }
            else if (!__parse_header_method(parser, c))
                parser->stat = HTTP_REQ_STAT_ERROR;
            break;
        case HTTP_REQ_STAT_HEADER_URI:
            if (c == ' ') {
                parser->stat = HTTP_REQ_STAT_HEADER_VERSION;
                __set_uri(req, parser);
            }
            else if (!__parse_header_uri(parser, c))
                parser->stat = HTTP_REQ_STAT_ERROR;
            break;
        case HTTP_REQ_STAT_HEADER_VERSION:
            if (c == '\r') {
                parser->stat = HTTP_REQ_STAT_HEADER_REQ_END;
                __set_version(req, parser);
            }
            else if (!__parse_header_version(parser, c))
                parser->stat = HTTP_REQ_STAT_ERROR;
            break;
        case HTTP_REQ_STAT_HEADER_REQ_END:
            if (c == '\n')
                parser->stat = HTTP_REQ_STAT_HEADER_PARAM;
            else
                parser->stat = HTTP_REQ_STAT_ERROR;
            break;
        case HTTP_REQ_STAT_HEADER_PARAM:
            if (c == '\r') {
                if (!__req_param_append(req, parser))
                    parser->stat = HTTP_REQ_STAT_ERROR;
                else
                    parser->stat = HTTP_REQ_STAT_HEADER_PARAM_END1;
            }
            else
                __parse_header_param(parser, c);
            break;
        case HTTP_REQ_STAT_HEADER_PARAM_END1:
            if (c == '\n')
                parser->stat = HTTP_REQ_STAT_HEADER_PARAM_END2;
            else
                parser->stat = HTTP_REQ_STAT_ERROR;
            break;
        case HTTP_REQ_STAT_HEADER_PARAM_END2:
            if (c == '\r')
                parser->stat = HTTP_REQ_STAT_HEADER_PARAM_END3;
            else {
                if (!__parse_header_param(parser, c))
                    parser->stat = HTTP_REQ_STAT_ERROR;
                else
                    parser->stat = HTTP_REQ_STAT_HEADER_PARAM;
            }
            break;
        case HTTP_REQ_STAT_HEADER_PARAM_END3:
            if (c == '\n')
                parser->stat = HTTP_REQ_STAT_HEADER_END;
            else
                parser->stat = HTTP_REQ_STAT_ERROR;
            break;
        }
    }

    return used_size;
}

static size_t __parse_raw(struct http_req_parser * const parser,
                          struct http_req_protocol * const req,
                          const char *data,
                          size_t len)
{
    size_t readable_size;
    if (req->payload == NULL) {
        req->payload = malloc(parser->content_length);
        if (req->payload == NULL)
            return 0;
        req->payload_size = parser->content_length;
    }
    
    readable_size = parser->content_length - parser->content_received;
    if (len < readable_size)
        readable_size = len;

    memmove(req->payload + parser->content_received,
            data,
            readable_size);
    return readable_size;
}

/**
 * parse http request
 * @param parser: parser
 * @param req: http req
 * @param data: data
 * @param len: data len
 * 
 */
size_t zl_http_req_protocol_parse(struct http_req_parser * const parser,
                                  struct http_req_protocol * const req,
                                  const char *data,
                                  size_t len)
{
    const char * content_length;
    size_t used_size;

    if (parser->stat == HTTP_REQ_STAT_RAW) {
        if (parser->content_length - parser->content_received == 0) {
            parser->stat = HTTP_REQ_STAT_END;
            return 0;
        }
        used_size = __parse_raw(parser, req, data, len);
        parser->content_received += used_size;
        return used_size;
    }

    used_size = __parse_header(parser, req, data, len);

    if (parser->stat == HTTP_REQ_STAT_HEADER_END) {
        content_length = zl_kv_param_dict_find(&req->params, "Content-Length");
        if (content_length == NULL) {
            parser->stat = HTTP_REQ_STAT_END;
        }
        else {
            parser->content_length = atoi(content_length);
            parser->content_received = 0;
            parser->content_received +=
                __parse_raw(parser, req, data + used_size, len - used_size);
            used_size += parser->content_received;
        }
    }

    return used_size;
}
