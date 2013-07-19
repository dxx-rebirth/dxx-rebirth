#ifndef _HUD_MSG_H
#define _HUD_MSG_H

#include <stdarg.h>

#define HUD_MESSAGE_LENGTH	150
#define HUD_MAX_NUM_DISP	4
#define HUD_MAX_NUM_STOR	20

// classes - expanded whenever needed
#define HM_DEFAULT		1 // just some normal message
#define HM_MULTI		2 // a message related to multiplayer (game and player messages)
#define HM_REDUNDANT		4 // "you already have..."-type messages. stuff a player is able to supress
#define HM_MAYDUPL		8 // messages that might appear once per frame. for these we want to check all messages we have  in queue and supress it if so

extern int HUD_toolong;
extern void HUD_clear_messages();
extern void HUD_render_message_frame();
int HUD_init_message(int class_flag, const char * format, ... );
int HUD_init_message_va(int class_flag, const char * format, va_list args);
int HUD_init_message_literal(int class_flag, const char *str);

#endif
