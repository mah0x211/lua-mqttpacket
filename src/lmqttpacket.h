
#ifndef lmqttpacket_h
#define lmqttpacket_h

#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <stdint.h>
// lualib
#include "lauxhlib.h"
#include "MQTTPacket.h"


LUALIB_API int luaopen_mqttpacket( lua_State *L );
LUALIB_API int luaopen_mqttpacket_client( lua_State *L );


#endif
