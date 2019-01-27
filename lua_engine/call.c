#include "debug/console.h"
#include "lua_engine/caller.h"
#include <lua.h>
#include <lualib.h>
#include <lauxlib.h>

static struct lua_method_mapper __func_names[] = {
    { HTTP_REQ_METHOD_GET, "get" },
    { HTTP_REQ_METHOD_POST, "post" },
};

const char * __func_name(struct http_req_protocol * const req)
{
    int i;
    int len = sizeof(__func_names) / sizeof(struct lua_method_mapper);

    for (i = 0; i < len; i++) {
        if (req->method == __func_names[i].method)
            return __func_names[i].name;
    }

    return "default_method";
}

static struct lua_State * __open_lua_state(const char * script_path)
{
    lua_State * lua = luaL_newstate();
    luaL_openlibs(lua);
    luaL_dofile(lua, script_path);

    return lua;
}

static void __wrap_http_protocol(struct lua_State * lua,
                                 struct http_req_protocol * const req,
                                 struct http_res_protocol * const res)
{
    struct lua_http_protocol * req_wrap =
        lua_newuserdata(lua, sizeof(struct lua_http_protocol));
    struct lua_http_protocol * res_wrap = 
        lua_newuserdata(lua, sizeof(struct lua_http_protocol));

    req_wrap->type = LUA_HTTP_PROTOCOL_TYPE_REQ;
    req_wrap->ptr  = req;
    res_wrap->type = LUA_HTTP_PROTOCOL_TYPE_RES;
    res_wrap->ptr  = res;
}

static void __call_func(struct lua_State * lua,
                        struct http_req_protocol * const req,
                        struct http_res_protocol * const res)
{
    lua_settop(lua, 0);

    lua_getglobal(lua, __func_name(req));
    __wrap_http_protocol(lua, req, res);
    lua_pcall(lua, 2, 0, 0);
}

void zl_lua_engine_call(const char * script_path,
                        struct http_req_protocol * const req,
                        struct http_res_protocol * const res)
{
    struct lua_State * lua;
    lua = __open_lua_state(script_path);
    __call_func(lua, req, res);
    lua_close(lua);
}
