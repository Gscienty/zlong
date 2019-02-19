#include "debug/console.h"
#include "http/websocket.h"
#include "http/web_gateway.h"
#include "http/config.h"
#include "lua_engine/router.h"
#include "lua_engine/caller.h"
#include <string.h>
#include <malloc.h>
#include <unistd.h>

static inline const char * __get_script(struct http_req_protocol * const req)
{
    const char * script_path = zl_lua_engine_get_script_path(req);
    if (script_path != NULL && access(script_path, F_OK) == 0) {
        return script_path;
    }
    if (script_path != NULL) {
        free((void *) script_path);
    }

    return NULL;
}

static void __http(struct http_session * const session)
{
    const char * script_path = __get_script(&session->req_protocol);
    if (script_path == NULL) {
        zl_lua_engine_notfound(&session->res_protocol);
    }
    else if (zl_websocket_check(&session->req_protocol)) {
        zl_websocket_accept(&session->req_protocol,
                            &session->res_protocol);
        session->is_websocket = true;
    }
    else {
        zl_http_res_protocol_init(&session->res_protocol);
        zl_lua_engine_call(script_path, session);
    }

    zl_http_res_protocol_default_params(&session->res_protocol);
    free((void *) script_path);
}

static void __websocket(struct http_session * const session)
{
    const char * script_path = __get_script(&session->req_protocol);

    zl_lua_engine_call(script_path, session);

    free((void *) script_path);
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
