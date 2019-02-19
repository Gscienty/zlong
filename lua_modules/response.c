#include "session/http_session.h"
#include "lua_engine/caller.h"
#include "utils/kv_param.h"
#include <string.h>
#include <stdbool.h>
#include <malloc.h>
#include <lua.h>
#include <lualib.h>
#include <lauxlib.h>

static int __lua_set_response_status_code(struct lua_State * lua)
{
    struct http_session ** session_wrapper;
    short status_code;
    session_wrapper = lua_touserdata(lua, 1);
    status_code = lua_tointeger(lua, 2);

    (*session_wrapper)->res_protocol.status_code = status_code;

    return 1;
}

static const char * __param(struct http_session * const session,
                          const char * const key)
{
    return zl_kv_param_dict_find(&(session)->res_protocol.params, key);
}

static int __lua_get_response_param(struct lua_State * lua)
{
    const char * val;
    const char * key;
    struct http_session ** session_wrapper;
    session_wrapper = lua_touserdata(lua, 1);
    key = lua_tostring(lua, 2);

    val = __param(*session_wrapper, key);
    if (val == NULL) {
        lua_pushnil(lua);
    }
    else {
        lua_pushstring(lua, val);
    }
    
    return 1;
}

static int __lua_set_response_param(struct lua_State * lua)
{
    const char * key;
    struct http_session ** session_wrapper;
    struct kv_param * newly_param;
    session_wrapper = lua_touserdata(lua, 1);
    key = lua_tostring(lua, 2);

    zl_kv_param_dict_delete(&(*session_wrapper)->res_protocol.params, key);
    newly_param = (struct kv_param *) malloc(sizeof(struct kv_param));
    zl_kv_param_set(newly_param, (char *) key, (char *) lua_tostring(lua, 3));
    zl_kv_param_dict_add(&(*session_wrapper)->res_protocol.params, newly_param);

    return 1;
}

static int __lua_set_response_string_body(struct lua_State * lua)
{
    struct http_session ** session_wrapper;
    const char * val;
    session_wrapper = lua_touserdata(lua, 1);
    val = lua_tostring(lua, 2);

    (*session_wrapper)->res_protocol.payload_size = strlen(val);
    (*session_wrapper)->res_protocol.payload = strdup(val);

    return 1;
}

static struct luaL_Reg __methods[] = {
    { "set_status_code", __lua_set_response_status_code },
    { "get_param", __lua_get_response_param },
    { "set_param", __lua_set_response_param },
    { "set_strbody", __lua_set_response_string_body },
    { NULL, NULL }
};

int luaopen_response(lua_State * lua)
{
    luaL_newlib(lua, __methods);

    return 1;
}
