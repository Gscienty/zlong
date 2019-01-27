#ifndef _ZL_HTTP_WEB_GATEWAY_H
#define _ZL_HTTP_WEB_GATEWAY_H

#include "http/request.h"
#include "http/response.h"

void zl_webgateway_enter(struct http_req_protocol * const req,
                         struct http_res_protocol * const res);

#endif
