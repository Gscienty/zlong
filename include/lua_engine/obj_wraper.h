#ifndef _ZL_LUA_ENGINE_OBJ_WRAPER_H
#define _ZL_LUA_ENGINE_OBJ_WRAPER_H

#include <lua.h>
#include <lualib.h>
#include <lauxlib.h>

void zl_lua_register_session_metatable(lua_State * lua);

void zl_lua_register_request_metatable(lua_State * lua);

void zl_lua_register_response_metatable(lua_State * lua);

#endif
