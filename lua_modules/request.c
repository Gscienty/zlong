#include "lua_engine/caller.h"
#include "utils/kv_param.h"
#include <string.h>
#include <stdbool.h>
#include <malloc.h>
#include <lua.h>
#include <lualib.h>
#include <lauxlib.h>

static const char * __uri(struct http_req_protocol * const req)
{
    return req->uri;
}

static const char * __param(struct http_req_protocol * const req,
                          const char * const key)
{
    return zl_kv_param_dict_find(&req->params, key);
}

static bool __protocol_legal(struct lua_http_protocol * const protocol)
{
    return protocol != NULL || protocol->type == LUA_HTTP_PROTOCOL_TYPE_REQ;
}

static int __lua_get_request_uri(struct lua_State * lua)
{
    struct lua_http_protocol * protocol;
    
    protocol = lua_touserdata(lua, 1);
    if (!__protocol_legal(protocol))
        return 0;

    lua_pushstring(lua, __uri((struct http_req_protocol *) protocol->ptr));

    return 1;
}

static int __lua_set_request_uri(struct lua_State * lua)
{
    struct lua_http_protocol * protocol;
    struct http_req_protocol * req;

    protocol = lua_touserdata(lua, 1);
    if (!__protocol_legal(protocol))
        return 0;
    req = (struct http_req_protocol *) protocol->ptr;

    free(req->uri);
    req->uri = strdup(lua_tostring(lua, 2));

    return 1;
}

static int __lua_get_request_param(struct lua_State * lua)
{
    const char * val;
    const char * key;
    struct lua_http_protocol * protocol;
    
    protocol = lua_touserdata(lua, 1);
    if (!__protocol_legal(protocol))
        return 0;
    key = lua_tostring(lua, 2);

    val = __param((struct http_req_protocol *) protocol->ptr, key);

    if (val == NULL) {
        lua_pushnil(lua);
    }
    else {
        lua_pushstring(lua, val);
    }
    
    return 1;
}

static int __lua_set_request_param(struct lua_State * lua)
{
    const char * key;
    struct lua_http_protocol * protocol;
    struct http_req_protocol * req;
    struct kv_param * newly_param;

    protocol = lua_touserdata(lua, 1);
    if (__protocol_legal(protocol))
        return 0;
    req = (struct http_req_protocol *) protocol->ptr;
    key = lua_tostring(lua, 2);

    zl_kv_param_dict_delete(&req->params, key);

    newly_param = (struct kv_param *) malloc(sizeof(struct kv_param));
    zl_kv_param_set(newly_param, (char *) key, (char *) lua_tostring(lua, 3));
    zl_kv_param_dict_add(&req->params, newly_param);

    return 1;
}

static int __lua_get_request_body(struct lua_State * lua)
{
    struct lua_http_protocol * protocol;
    struct http_req_protocol * req;

    protocol = lua_touserdata(lua, 1);
    if (__protocol_legal(protocol))
        return 0;
    req = (struct http_req_protocol *) protocol->ptr;

    lua_pushstring(lua, req->payload);

    return 1;
}

static struct luaL_Reg __methods[] = {
    { "get_uri", __lua_get_request_uri },
    { "set_uri", __lua_set_request_uri },
    { "get_param", __lua_get_request_param },
    { "set_param", __lua_set_request_param },
    { "get_body", __lua_get_request_body },
    { NULL, NULL }
};

int luaopen_request(lua_State * lua)
{
    luaL_newlib(lua, __methods);

    return 1;
}

