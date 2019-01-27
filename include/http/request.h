#ifndef _ZL_HTTP_REQUEST_H
#define _ZL_HTTP_REQUEST_H

#include "utils/version.h"
#include "utils/rbtree.h"
#include <stddef.h>

enum http_req_method {
    HTTP_REQ_METHOD_GET,
    HTTP_REQ_METHOD_POST,
    HTTP_REQ_METHOD_PUT,
    HTTP_REQ_METHOD_DELETE,
    HTTP_REQ_METHOD_HEAD,
    HTTP_REQ_METHOD_CONNECT,
    HTTP_REQ_METHOD_OPTIONS,
    HTTP_REQ_METHOD_TRACE,
};

enum http_req_stat {
    HTTP_REQ_STAT_HEADER_METHOD,
    HTTP_REQ_STAT_HEADER_URI,
    HTTP_REQ_STAT_HEADER_VERSION,
    HTTP_REQ_STAT_HEADER_REQ_END,
    HTTP_REQ_STAT_HEADER_PARAM,
    HTTP_REQ_STAT_HEADER_PARAM_END1,
    HTTP_REQ_STAT_HEADER_PARAM_END2,
    HTTP_REQ_STAT_HEADER_PARAM_END3,
    HTTP_REQ_STAT_HEADER_END,
    HTTP_REQ_STAT_RAW,
    HTTP_REQ_STAT_END,
    HTTP_REQ_STAT_ERROR
};

#define HTTP_REQ_METHOD_MAX_SIZE 8
#define HTTP_REQ_URI_MAX_SIZE 255
#define HTTP_REQ_VERSION_MAX_SIZE 16
#define HTTP_REQ_PARAM_MAX_SIZE 512

struct http_req_parser {
    enum http_req_stat stat;
    char method[HTTP_REQ_METHOD_MAX_SIZE];
    int method_used;
    char uri[HTTP_REQ_URI_MAX_SIZE];
    int uri_used;
    char version[HTTP_REQ_VERSION_MAX_SIZE];
    int version_used;
    char param[HTTP_REQ_PARAM_MAX_SIZE];
    int param_used;

    int content_length;
    int content_received;
};

struct http_req_protocol {
    enum http_req_method method;
    char * uri;
    enum http_version version;
    struct rbroot params;

    void * payload;
    size_t payload_size;
};

/**
 * init parser
 * @param parser: parser
 * 
 */
void zl_http_req_parser_init(struct http_req_parser * const parser);

/**
 * init req protocol
 * @param req
 * 
 */
void zl_http_req_protocol_init(struct http_req_protocol * const req);

/**
 * reset req protocol
 * 
 */
void zl_http_req_protocol_reset(struct http_req_protocol * const req);

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
                                  size_t len);

#endif
