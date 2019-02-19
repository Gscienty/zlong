#include "session/http_session.h"
#include "lua_engine/caller.h"
#include "utils/kv_param.h"
#include <string.h>
#include <stdbool.h>
#include <malloc.h>
#include <lua.h>
#include <lualib.h>
#include <lauxlib.h>

static const char * __uri(struct http_session * const session)
{
    return session->req_protocol.uri;
}

static const char * __param(struct http_session * const session,
                          const char * const key)
{
    return zl_kv_param_dict_find(&session->req_protocol.params, key);
}

static int __lua_get_request_uri(struct lua_State * lua)
{
    struct http_session ** session_wrapper;
    session_wrapper = lua_touserdata(lua, 1);

    lua_pushstring(lua, __uri(*session_wrapper));

    return 1;
}

static int __lua_set_request_uri(struct lua_State * lua)
{
    struct http_session ** session_wrapper;
    session_wrapper = lua_touserdata(lua, 1);

    free((*session_wrapper)->req_protocol.uri);
    (*session_wrapper)->req_protocol.uri = strdup(lua_tostring(lua, 2));

    return 1;
}

static int __lua_get_request_param(struct lua_State * lua)
{
    const char * val;
    const char * key;
    struct http_session ** session_wrapper;
    session_wrapper = lua_touserdata(lua, 1);
    
    session_wrapper = lua_touserdata(lua, 1);
    key = lua_tostring(lua, 2);

    val = __param(*session_wrapper , key);

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
    struct http_session ** session_wrapper;
    struct kv_param * newly_param;
    session_wrapper = lua_touserdata(lua, 1);
    key = lua_tostring(lua, 2);

    zl_kv_param_dict_delete(&(*session_wrapper)->req_protocol.params, key);
    newly_param = (struct kv_param *) malloc(sizeof(struct kv_param));
    zl_kv_param_set(newly_param, (char *) key, (char *) lua_tostring(lua, 3));
    zl_kv_param_dict_add(&(*session_wrapper)->req_protocol.params, newly_param);

    return 1;
}

static int __lua_get_request_body(struct lua_State * lua)
{
    struct http_session ** session_wrapper;
    session_wrapper = lua_touserdata(lua, 1);

    lua_pushstring(lua, (*session_wrapper)->req_protocol.payload);

    return 1;
}

static int __lua_get_request_querystring(struct lua_State * lua)
{
    struct http_session ** session_wrapper;
    char * delimiter;
    session_wrapper = lua_touserdata(lua, 1);

    delimiter = strchr((*session_wrapper)->req_protocol.uri, '?');
    if (delimiter == NULL) {
        lua_pushstring(lua, "null");
    }
    else {
        lua_pushstring(lua, delimiter);
    }

    return 1;
}

static struct luaL_Reg __methods[] = {
    { "get_uri", __lua_get_request_uri },
    { "set_uri", __lua_set_request_uri },
    { "get_querystring", __lua_get_request_querystring },
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

