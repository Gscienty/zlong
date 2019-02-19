#ifndef _ZL_LUA_ENGINE_CALLER_H
#define _ZL_LUA_ENGINE_CALLER_H

#include "session/http_session.h"

struct lua_method_mapper {
    enum http_req_method method;
    const char * name;
};

void zl_lua_engine_call(const char * script_path,
                        struct http_session * const session);

#endif
