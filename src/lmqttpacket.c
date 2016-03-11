#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <stdint.h>
// lualib
#include <lauxhlib.h>
#include <MQTTPacket.h>



static int publish_lua( lua_State *L )
{
    MQTTString topic = MQTTString_initializer;
    size_t tlen = 0;
    size_t plen = 0;
    const char *payload = NULL;
    unsigned char *buf = NULL;

    // check arguments
    // topic
    topic.lenstring.data = lauxh_checklstring( L, 1, &tlen );
    topic.lenstring.len = tlen;
    // payload
    payload = lauxh_checklstring( L, 2, &plen );

    // topic/payload too large
    if( tlen > INT_MAX || plen > INT_MAX ){
        errno = EOVERFLOW;
    }
    else
    {
        int buflen = MQTTPacket_len(
            MQTTSerialize_publishLength( 0, topic, plen )
        );

        if( ( buf = malloc( buflen ) ) )
        {
            int len = MQTTSerialize_publish( buf, buflen, 0/*dup*/, 0/*qos*/,
                                         0/*retained*/, 0/*packetid*/, topic,
                                         payload, plen );

            if( len > 0 ){
                lua_pushlstring( L, buf, len );
                free( buf );
                return 1;
            }

            errno = ENOBUFS;
            free( buf );
        }
    }

    // got error
    lua_pushnil( L );
    lua_pushstring( L, strerror( errno ) );

    return 2;
}


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
        { "publish", publish_lua },
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


