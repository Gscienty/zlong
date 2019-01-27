#ifndef _ZL_LUA_ENGINE_CALLER_H
#define _ZL_LUA_ENGINE_CALLER_H

#include "http/request.h"
#include "http/response.h"

struct lua_method_mapper {
    enum http_req_method method;
    const char * name;
};

enum lua_http_protocol_type {
    LUA_HTTP_PROTOCOL_TYPE_REQ,
    LUA_HTTP_PROTOCOL_TYPE_RES
};

struct lua_http_protocol {
    enum lua_http_protocol_type type;
    void *ptr;
};

void zl_lua_engine_call(const char * script_path,
                        struct http_req_protocol * const req,
                        struct http_res_protocol * const res);

#endif
