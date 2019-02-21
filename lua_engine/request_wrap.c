#include "session/http_session.h"
#include "utils/kv_param.h"
#include <string.h>
#include <stdbool.h>
#include <malloc.h>
#include <lua.h>
#include <lualib.h>
#include <lauxlib.h>

static const char * __param(struct http_req_protocol * const req,
                          const char * const key)
{
    return zl_kv_param_dict_find(&req->params, key);
}

static int __get_uri(struct lua_State * lua)
{
    struct http_req_protocol ** req_wrapper;
    req_wrapper = lua_touserdata(lua, 1);

    lua_pushstring(lua, (*req_wrapper)->uri);

    return 1;
}

static int __set_uri(struct lua_State * lua)
{
    struct http_req_protocol ** req_wrapper;
    req_wrapper = lua_touserdata(lua, 1);

    free((*req_wrapper)->uri);
    (*req_wrapper)->uri = strdup(lua_tostring(lua, 2));

    return 1;
}

static int __get_param(struct lua_State * lua)
{
    const char * val;
    const char * key;
    struct http_req_protocol ** req_wrapper;
    req_wrapper = lua_touserdata(lua, 1);
    key = lua_tostring(lua, 2);

    val = __param(*req_wrapper, key);

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
    const char * key;
    struct http_req_protocol ** req_wrapper;
    struct kv_param * newly_param;
    req_wrapper = lua_touserdata(lua, 1);
    key = lua_tostring(lua, 2);

    zl_kv_param_dict_delete(&(*req_wrapper)->params, key);
    newly_param = (struct kv_param *) malloc(sizeof(struct kv_param));
    zl_kv_param_set(newly_param, (char *) key, (char *) lua_tostring(lua, 3));
    zl_kv_param_dict_add(&(*req_wrapper)->params, newly_param);

    return 1;
}

static int __get_body(struct lua_State * lua)
{
    struct http_req_protocol ** req_wrapper;
    req_wrapper = lua_touserdata(lua, 1);

    lua_pushstring(lua, (*req_wrapper)->payload);

    return 1;
}

static int __get_querystring(struct lua_State * lua)
{
    struct http_req_protocol ** req_wrapper;
    char * delimiter;
    req_wrapper = lua_touserdata(lua, 1);

    delimiter = strchr((*req_wrapper)->uri, '?');
    if (delimiter == NULL) {
        lua_pushstring(lua, "null");
    }
    else {
        lua_pushstring(lua, delimiter);
    }

    return 1;
}

static luaL_Reg __methods[] = {
    { "get_uri", __get_uri },
    { "set_uri", __set_uri },
    { "get_param", __get_param },
    { "set_param", __set_param },
    { "get_body", __get_body },
    { "get_querystring", __get_querystring },
    { NULL, NULL }
};

void zl_lua_register_request_metatable(lua_State * lua)
{
    luaL_newmetatable(lua, "zl_request");
    lua_pushvalue(lua, -1);
    lua_setfield(lua, -2, "__index");
    luaL_setfuncs(lua, __methods, 0);
}
