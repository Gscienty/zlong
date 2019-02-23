#include "debug/console.h"
#include "lua_engine/obj_wraper.h"
#include "session/http_session.h"
#include "utils/kv_param.h"
#include <string.h>
#include <stdbool.h>
#include <malloc.h>
#include <lua.h>
#include <lualib.h>
#include <lauxlib.h>

static int __set_status_code(struct lua_State * lua)
{
    struct http_res_protocol ** res_wrapper;
    short status_code;
    res_wrapper = lua_touserdata(lua, 1);
    status_code = lua_tointeger(lua, 2);

    (*res_wrapper)->status_code = status_code;

    return 1;
}

static const char * __param(struct http_res_protocol * const res,
                          const char * const key)
{
    return zl_kv_param_dict_find(&(res)->params, key);
}

static int __get_param(struct lua_State * lua)
{
    const char * val;
    const char * key;
    struct http_res_protocol ** res_wrapper;
    res_wrapper = lua_touserdata(lua, 1);
    key = lua_tostring(lua, 2);

    val = __param(*res_wrapper, key);
    if (val == NULL) {
        lua_pushnil(lua);
    }
    else {
        lua_pushstring(lua, val);
    }
    
    return 1;
}

static int __set_param(struct lua_State * lua)
{
    char * key;
    char * val;
    struct http_res_protocol ** res_wrapper;
    res_wrapper = lua_touserdata(lua, 1);
    key = strdup(lua_tostring(lua, 2));
    val = strdup(lua_tostring(lua, 3));

    zl_http_res_protocol_add_param(*res_wrapper, key, val);

    return 1;
}

static int __append_string_body(struct lua_State * lua)
{
    struct http_res_protocol ** res_wrapper;
    const char * val;
    size_t val_size;
    res_wrapper = lua_touserdata(lua, 1);
    val = lua_tostring(lua, 2);
    val_size = strlen(val);

    if ((*res_wrapper)->payload_size == 0) {
        (*res_wrapper)->payload = strdup(val);
        (*res_wrapper)->payload_size = val_size;
    }
    else {
        (*res_wrapper)->payload =
            realloc((*res_wrapper)->payload,
                    (*res_wrapper)->payload_size + val_size);
        memcpy((*res_wrapper)->payload + (*res_wrapper)->payload_size,
               val,
               val_size);
        (*res_wrapper)->payload_size += val_size;
    }

    return 1;
}

static struct luaL_Reg __methods[] = {
    { "set_status_code", __set_status_code },
    { "get_param", __get_param },
    { "set_param", __set_param },
    { "write", __append_string_body },
    { NULL, NULL }
};

void zl_lua_register_response_metatable(lua_State * lua)
{
    luaL_newmetatable(lua, "zl_response");
    lua_pushvalue(lua, -1);
    lua_setfield(lua, -2, "__index");
    luaL_setfuncs(lua, __methods, 0);
}
