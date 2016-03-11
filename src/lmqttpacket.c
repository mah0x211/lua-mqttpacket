#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <stdint.h>
// lualib
#include <lauxhlib.h>
#include <MQTTPacket.h>


LUALIB_API int luaopen_mqttpacket( lua_State *L )
{
    struct luaL_Reg funcs[] = {
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


