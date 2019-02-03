#include "http/web_gateway.h"
#include "debug/console.h"
#include "lua_engine/router.h"
#include "lua_engine/caller.h"
#include "http/config.h"
#include <string.h>
#include <malloc.h>
#include <unistd.h>

static void __web_resource(struct http_req_protocol * const req,
                           struct http_res_protocol * const res)
{
    const char * script_path = zl_lua_engine_get_script_path(req);
    if (script_path == NULL) {
        error("script_path cannot got");
        zl_lua_engine_notfound(res);
    }
    else {
        if (access(script_path, F_OK) == 0) {
            zl_lua_engine_call(script_path, req, res);
        }
        else {
            zl_lua_engine_notfound(res);
        }
        free((void *) script_path);
    }
}

void zl_webgateway_enter(struct http_req_protocol * const req,
                         struct http_res_protocol * const res)
{
    zl_http_res_protocol_init(res);
    info("enter resource");
    __web_resource(req, res);
    zl_http_res_protocol_default_params(res);
}
