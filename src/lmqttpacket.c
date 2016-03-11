#include "lmqttpacket.h"

LUALIB_API int luaopen_mqttpacket( lua_State *L )
{
    lua_createtable( L, 0, 1 );

    lua_pushstring( L, "client" );
    luaopen_mqttpacket_client( L );
    lua_rawset( L, -3 );

    return 1;
}


