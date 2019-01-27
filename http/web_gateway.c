#include "http/web_gateway.h"
#include "debug/console.h"
#include "lua_engine/caller.h"

static void __web_resource(struct http_req_protocol * const req,
                           struct http_res_protocol * const res)
{
    zl_lua_engine_call("test/test.lua", req, res);
}

void zl_webgateway_enter(struct http_req_protocol * const req,
                         struct http_res_protocol * const res)
{
    (void) req;
    zl_http_res_protocol_init(res);

    __web_resource(req, res);

    /*zl_http_res_protocol_default_params(res);*/
    info("enter web gateway");
}
