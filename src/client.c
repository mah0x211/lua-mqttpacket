/*
 *  Copyright 2016 Masatoshi Teruya. All rights reserved.
 *
 *  Permission is hereby granted, free of charge, to any person obtaining a 
 *  copy of this software and associated documentation files (the "Software"), 
 *  to deal in the Software without restriction, including without limitation 
 *  the rights to use, copy, modify, merge, publish, distribute, sublicense, 
 *  and/or sell copies of the Software, and to permit persons to whom the 
 *  Software is furnished to do so, subject to the following conditions:
 *
 *  The above copyright notice and this permission notice shall be included in
 *  all copies or substantial portions of the Software.
 *
 *  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 *  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 *  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 *  THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 *  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING 
 *  FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER 
 *  DEALINGS IN THE SOFTWARE.
 *
 *  client.c
 *  lua-mqttpacket
 *  Created by Masatoshi Teruya on 16/03/11.
 */

#include "lmqttpacket.h"


static int unsubscribe_lua( lua_State *L )
{
    const int argc = lua_gettop( L );
    MQTTString topic = MQTTString_initializer;
    size_t ntopic = 1;
    MQTTString *topics = NULL;
    size_t tlen = 0;

    // check first arguments
    topic.lenstring.data = (char*)lauxh_checklstring( L, 1, &tlen );
    // invalid topic length
    if( tlen == 0 || tlen > UINT16_MAX ){
        lauxh_argerror( L, 1, "topic length must be up to UINT16_MAX" );
    }
    topic.lenstring.len = tlen;

    // create topic container
    topics = lua_newuserdata( L, sizeof( MQTTString ) * argc );
    if( topics )
    {
        unsigned short id = 0;
        unsigned char dup = 0;
        unsigned char *buf = NULL;
        int buflen = 0;
        int idx = 2;

        // copy first topic
        topics[0] = topic;

        // get topics
        for(; idx <= argc; idx++ )
        {
            // topic
            if( lauxh_isstring( L, idx ) )
            {
                // add to topics
                topics[idx-1].cstring = NULL;
                topics[idx-1].lenstring.data = (char*)lua_tolstring(
                    L, idx, &tlen
                );
                // invalid topic length
                if( tlen == 0 || tlen > UINT16_MAX ){
                    lauxh_argerror(
                        L, idx, "topic length must be up to UINT16_MAX"
                    );
                }
                topics[idx-1].lenstring.len = tlen;
                ntopic++;
            }
            // found option at last argument
            else if( lauxh_istable( L, idx ) && idx == argc )
            {
                lua_Integer v = 0;

                // dup flag
                dup = (unsigned char)lauxh_optbooleanof( L, idx, "dup", dup );

                // id
                v = lauxh_optintegerof( L, idx, "id", 0 );
                if( v < 0 || v > UINT16_MAX ){
                    lauxh_argerror( L, idx, "id must be up to UINT16_MAX" );
                }
                id = v;
            }
            // invalid value type
            else {
                lauxh_argerror(
                    L, idx, "arg#%zd must be topic string or option table", idx
                );
            }
        }

        // create packet
        buflen = MQTTPacket_len(
            MQTTSerialize_unsubscribeLength( ntopic, topics )
        );
        if( ( buf = malloc( buflen ) ) )
        {
            int len = MQTTSerialize_unsubscribe( buf, buflen, dup, id,
                                                 ntopic, topics );

            if( len > 0 ){
                lua_pushlstring( L, (const char*)buf, len );
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


static int subscribe_lua( lua_State *L )
{
    MQTTString topic[] = { MQTTString_initializer };
    int qos[] = {0};
    size_t tlen = 0;
    unsigned char *buf = NULL;

    // check arguments
    // topic
    topic[0].lenstring.data = (char*)lauxh_checklstring( L, 1, &tlen );
    topic[0].lenstring.len = tlen;

    // topic length too large
    if( tlen > INT_MAX ){
        errno = EOVERFLOW;
    }
    else
    {
        int len = 0;
        int buflen = MQTTPacket_len(
            MQTTSerialize_subscribeLength( 1/*count*/, topic )
        );

        if( ( buf = malloc( buflen ) ) )
        {
            len = MQTTSerialize_subscribe( buf, buflen, 0/*dup*/, 0/*packetid*/,
                                           1/*count*/, topic, qos );

            if( len > 0 ){
                lua_pushlstring( L, (const char*)buf, len );
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


static int publish_lua( lua_State *L )
{
    MQTTString topic = MQTTString_initializer;
    size_t plen = 0;
    unsigned char *payload = NULL;
    unsigned char *buf = NULL;
    int buflen = 0;
    unsigned char dup = 0;
    unsigned char retain = 0;
    int qos = 0;
    unsigned short id = 0;

    // check arguments
    // topic
    topic.lenstring.data = (char*)lauxh_checklstring( L, 1, &plen );
    // topic too large
    lauxh_argcheck(
        L, plen <= UINT16_MAX, 1, "topic length must be less than UINT16_MAX"
    );
    topic.lenstring.len = plen;

    // payload
    payload = (unsigned char*)lauxh_checklstring( L, 2, &plen );
    // payload too large
    lauxh_argcheck(
        L, plen <= INT_MAX, 1, "payload length must be less than INT_MAX"
    );

    // options
    if( lua_gettop( L ) > 2 )
    {
        lua_Integer v = 0;

        lua_settop( L, 3 );
        lauxh_checktable( L, -1 );
        dup = lauxh_optbooleanof( L, 3, "dup", 0 );
        retain = lauxh_optbooleanof( L, 3, "retain", 0 );

        v = lauxh_optintegerof( L, 3, "qos", 0 );
        // invalid qos value range
        lauxh_argcheck( L, v >= 0 && v <= 2, 3, "qos must be range of 0 to 2" );
        qos = v;

        v = lauxh_optintegerof( L, 3, "id", 0 );
        // invalid qos value range
        lauxh_argcheck(
            L, v >= 0 && v <= UINT16_MAX, 3, "id must be up to UINT16_MAX"
        );
        id = v;
    }

    // create buffer
    buflen = MQTTPacket_len( MQTTSerialize_publishLength( qos, topic, plen ) );
    if( ( buf = malloc( buflen ) ) )
    {
        int len = MQTTSerialize_publish( buf, buflen, dup, qos, retain, id,
                                         topic, payload, plen );

        if( len > 0 ){
            lua_pushlstring( L, (const char*)buf, len );
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


static int disconnect_lua( lua_State *L )
{
    unsigned char buf[3] = { 0 };
    int len = MQTTSerialize_disconnect( buf, sizeof( buf ) );

    if( len > 0 ){
        lua_pushlstring( L, (const char*)buf, len );
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
            lua_pushlstring( L, (const char*)buf, len );
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


LUALIB_API int luaopen_mqttpacket_client( lua_State *L )
{
    struct luaL_Reg funcs[] = {
        { "connect", connect_lua },
        { "disconnect", disconnect_lua },
        { "publish", publish_lua },
        { "subscribe", subscribe_lua },
        { "unsubscribe", unsubscribe_lua },
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


