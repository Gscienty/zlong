#include "lua_engine/caller.h"
#include "utils/kv_param.h"
#include <string.h>
#include <stdbool.h>
#include <malloc.h>
#include <lua.h>
#include <lualib.h>
#include <lauxlib.h>

static bool __protocol_legal(struct lua_http_protocol * const protocol)
{
    return protocol != NULL || protocol->type == LUA_HTTP_PROTOCOL_TYPE_RES;
}

static int __lua_set_response_status_code(struct lua_State * lua)
{
    struct lua_http_protocol * protocol;
    struct http_res_protocol * res;
    short status_code;

    protocol = lua_touserdata(lua, 1);
    if (!__protocol_legal(protocol))
        return 0;
    res = (struct http_res_protocol *) protocol->ptr;
    status_code = lua_tointeger(lua, 2);

    res->status_code = status_code;

    return 1;
}

static const char * __param(struct http_res_protocol * const res,
                          const char * const key)
{
    return zl_kv_param_dict_find(&res->params, key);
}

static int __lua_get_response_param(struct lua_State * lua)
{
    const char * val;
    const char * key;
    struct lua_http_protocol * protocol;
    
    protocol = lua_touserdata(lua, 1);
    if (!__protocol_legal(protocol))
        return 0;
    key = lua_tostring(lua, 2);

    val = __param((struct http_res_protocol *) protocol->ptr, key);

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
    struct lua_http_protocol * protocol;
    struct http_res_protocol * req;
    struct kv_param * newly_param;

    protocol = lua_touserdata(lua, 1);
    if (!__protocol_legal(protocol))
        return 0;
    req = (struct http_res_protocol *) protocol->ptr;
    key = lua_tostring(lua, 2);

    zl_kv_param_dict_delete(&req->params, key);

    newly_param = (struct kv_param *) malloc(sizeof(struct kv_param));
    zl_kv_param_set(newly_param, (char *) key, (char *) lua_tostring(lua, 3));
    zl_kv_param_dict_add(&req->params, newly_param);

    return 1;
}

static int __lua_set_response_string_body(struct lua_State * lua)
{
    struct lua_http_protocol * protocol;
    struct http_res_protocol * req;
    const char * val;

    protocol = lua_touserdata(lua, 1);
    if (!__protocol_legal(protocol))
        return 0;
    req = (struct http_res_protocol *) protocol->ptr;
    val = lua_tostring(lua, 2);

    req->payload_size = strlen(val);
    req->payload = strdup(val);


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
