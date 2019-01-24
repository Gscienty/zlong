#include "http/web_gateway.h"
#include "debug/console.h"

static void __web_resource(struct http_req_protocol * const req,
                           struct http_res_protocol * const res)
{
    (void) req;
    (void) res;
}

void zl_webgateway_enter(struct http_req_protocol * const req,
                         struct http_res_protocol * const res)
{
    (void) req;
    zl_http_res_protocol_init(res);

    __web_resource(req, res);

    zl_http_res_protocol_default_params(res);
    info("enter web gateway");
}
