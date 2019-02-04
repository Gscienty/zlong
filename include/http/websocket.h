#ifndef _ZL_HTTP_WEBSOCKET_H
#define _ZL_HTTP_WEBSOCKET_H

#include "http/request.h"
#include "http/response.h"
#include <stdbool.h>

bool zl_http_websocket_check(struct http_req_protocol * const req);

void zl_http_websocket_accept(struct http_req_protocol * const req,
                              struct http_res_protocol * const res);

#endif
