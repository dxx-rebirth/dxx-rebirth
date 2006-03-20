/* reconfig.c  -  ingame reconfig by Victor Rachels */

#ifndef _RECONFIG_H
#define _RECONFIG_H
#define RECONFIG_VERSION             0
/*#define RECONFIG_GAMEMODE             1
#define RECONFIG_GAME_FLAGS           2
#define RECONFIG_DIFFICULTY           4
#define RECONFIG_CONTROL_INVUL_TIME   8
#define RECONFIG_RESTRICT_MODE        16
#define RECONFIG_FLAGS                32
#define RECONFIG_PPS                  64
#define RECONFIG_PROTOCOL_VERSION     128
#define RECONFIG_MAX_NUMPLAYERS       256*/

void reconfig_send_gameinfo();
void reconfig_receive(ubyte *values,int len);

#endif
