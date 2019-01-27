#include "http/web_gateway.h"
#include "debug/console.h"
#include "lua_engine/caller.h"
#include <string.h>
#include <malloc.h>

static void __web_resource(struct http_req_protocol * const req,
                           struct http_res_protocol * const res)
{
    char * script_path = (char *) malloc(256);
    strcpy(script_path, "./");
    strcat(script_path, req->uri);

    zl_lua_engine_call(script_path, req, res);

    free(script_path);
}

void zl_webgateway_enter(struct http_req_protocol * const req,
                         struct http_res_protocol * const res)
{
    zl_http_res_protocol_init(res);
    __web_resource(req, res);
    zl_http_res_protocol_default_params(res);
}
