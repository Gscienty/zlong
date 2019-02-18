#include "debug/console.h"
#include "http/websocket.h"
#include "http/web_gateway.h"
#include "http/config.h"
#include "lua_engine/router.h"
#include "lua_engine/caller.h"
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

static void __http(struct http_session * const session)
{
    if (zl_websocket_check(&session->req_protocol)) {
        zl_websocket_accept(&session->req_protocol,
                            &session->res_protocol);
        session->is_websocket = true;
    }
    else {
        zl_http_res_protocol_init(&session->res_protocol);
        __web_resource(&session->req_protocol,
                       &session->res_protocol);
    }

    zl_http_res_protocol_default_params(&session->res_protocol);
}

static void __websocket(struct http_session * const session)
{
    (void) session;
}

void zl_webgateway_enter(struct http_session * const session)
{
    if (session->is_websocket) {
        __websocket(session);
    }
    else {
        __http(session);
    }
}
