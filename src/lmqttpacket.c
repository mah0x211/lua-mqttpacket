#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <stdint.h>
// lualib
#include <lauxhlib.h>
#include <MQTTPacket.h>


static int disconnect_lua( lua_State *L )
{
    unsigned char *buf[3] = { 0 };
    int len = MQTTSerialize_disconnect( buf, sizeof( buf ) );

    if( len > 0 ){
        lua_pushlstring( L, buf, len );
        return 1;
    }

    // got error
    lua_pushnil( L );
    lua_pushstring( L, strerror( ENOBUFS ) );

    return 2;
}


static int connect_lua( lua_State *L )
{
    unsigned char *buf = NULL;
    int buflen = 0;
    // MQTTPacket_connectData at MQTTConnect.h
    MQTTPacket_connectData opts = MQTTPacket_connectData_initializer;

    // alloc buffer
    buflen = MQTTPacket_len( MQTTSerialize_connectLength( &opts ) );
    if( ( buf = malloc( buflen ) ) )
    {
        // create connect packet
        int len = MQTTSerialize_connect( buf, buflen, &opts );

        if( len > 0 ){
            lua_pushlstring( L, buf, len );
            free( buf );
            return 1;
        }

        errno = ENOBUFS;
        free( buf );
    }

    // got error
    lua_pushnil( L );
    lua_pushstring( L, strerror( errno ) );

    return 2;
}


LUALIB_API int luaopen_mqttpacket( lua_State *L )
{
    struct luaL_Reg funcs[] = {
        { "connect", connect_lua },
        { "disconnect", disconnect_lua },
        { NULL, NULL }
    };
    struct luaL_Reg *ptr = funcs;
    
    // create table
    lua_newtable( L );
    while( ptr->name ){
        lauxh_pushfn2tbl( L, ptr->name, ptr->func );
        ptr++;
    }

    return 1;
}


