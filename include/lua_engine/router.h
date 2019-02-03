#ifndef _ZL_LUA_ENGINE_ROUTER_H
#define _ZL_LUA_ENGINE_ROUTER_H

#include "http/request.h"
#include "http/response.h"

const char * zl_lua_engine_get_script_path(struct http_req_protocol * const req);

void zl_lua_engine_notfound(struct http_res_protocol * const res);

#endif
