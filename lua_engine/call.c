#include "debug/console.h"
#include "lua_engine/obj_wraper.h"
#include "lua_engine/caller.h"
#include <lua.h>
#include <lualib.h>
#include <lauxlib.h>

static struct lua_method_mapper __func_names[] = {
    { HTTP_REQ_METHOD_GET, "get" },
    { HTTP_REQ_METHOD_POST, "post" },
    { HTTP_REQ_METHOD_PUT, "put" },
    { HTTP_REQ_METHOD_DELETE, "delete" }
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

    luaL_newmetatable(lua, "zl_session");
    lua_pushvalue(lua, -1);
    lua_setfield(lua, -2, "__index");

    zl_lua_register_request_metatable(lua);
    zl_lua_register_response_metatable(lua);
    zl_lua_register_session_metatable(lua);

    return lua;
}

static void __wrap_session(struct lua_State * lua,
                           struct http_session * const session)
{
    struct http_session ** session_wrapper =
        lua_newuserdata(lua, sizeof(struct http_session *));
    luaL_getmetatable(lua, "zl_session");
    lua_setmetatable(lua, -2);

    *session_wrapper = session;
}

static void __call_func(struct lua_State * lua,
                        struct http_session * const session)
{
    int ret;
    lua_settop(lua, 0);
    if (session->is_websocket) {
        lua_getglobal(lua, "websocket");
    }
    else {
        lua_getglobal(lua, __func_name(&session->req_protocol));
    }
    __wrap_session(lua, session);
    ret = lua_pcall(lua, 1, 0, 0);
    info("lua exec ret: %d", ret);
}

void zl_lua_engine_call(const char * script_path,
                             struct http_session * const session)
{
    struct lua_State * lua;
    lua = __open_lua_state(script_path);
    __call_func(lua, session);
    lua_close(lua);
}
