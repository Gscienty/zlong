#include "debug/console.h"
#include "lua_engine/obj_wraper.h"
#include "session/http_session.h"

static int __version(lua_State * lua)
{
    lua_pushstring(lua, "Version 0.1");
    return 1;
}

static int __get_request(lua_State * lua)
{
    struct http_session ** session_wrapper;
    session_wrapper = lua_touserdata(lua, 1);

    struct http_req_protocol ** req_wrapper
        = lua_newuserdata(lua, sizeof(struct http_req_protocol *));
    luaL_getmetatable(lua, "zl_request");
    lua_setmetatable(lua, -2);


    *req_wrapper = &(*session_wrapper)->req_protocol;

    return 1;
}

static int __get_response(lua_State * lua)
{
    struct http_session ** session_wrapper;
    session_wrapper = lua_touserdata(lua, 1);

    struct http_res_protocol ** res_wrapper
        = lua_newuserdata(lua, sizeof(struct http_res_protocol *));
    luaL_getmetatable(lua, "zl_response");
    lua_setmetatable(lua, -2);

    *res_wrapper = &(*session_wrapper)->res_protocol;

    return 1;
}

static int __get_cli_websocket_frame(lua_State * lua)
{
    struct http_session ** session_wrapper;
    session_wrapper = lua_touserdata(lua, 1);

    struct websocket_frame ** frame_wrapper
        = lua_newuserdata(lua, sizeof(struct websocket_frame *));
    luaL_getmetatable(lua, "zl_websocket_frame");
    lua_setmetatable(lua, -2);

    *frame_wrapper = &(*session_wrapper)->ws_cli_frame;

    return 1;
}

static int __get_ser_websocket_frame(lua_State * lua)
{
    struct http_session ** session_wrapper;
    session_wrapper = lua_touserdata(lua, 1);

    struct websocket_frame ** frame_wrapper
        = lua_newuserdata(lua, sizeof(struct websocket_frame *));
    luaL_getmetatable(lua, "zl_websocket_frame");
    lua_setmetatable(lua, -2);

    *frame_wrapper = &(*session_wrapper)->ws_ser_frame;

    return 1;
}

static luaL_Reg __methods[] = {
    { "version", __version },
    { "request", __get_request },
    { "response", __get_response },
    { "recv_websocket", __get_cli_websocket_frame },
    { "send_websocket", __get_ser_websocket_frame },
    { NULL, NULL }
};

void zl_lua_register_session_metatable(lua_State * lua)
{
    luaL_newmetatable(lua, "zl_session");
    lua_pushvalue(lua, -1);
    lua_setfield(lua, -2, "__index");
    luaL_setfuncs(lua, __methods, 0);
}
