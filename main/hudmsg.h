#ifndef _HUD_MSG_H
#define _HUD_MSG_H

#define HUD_MESSAGE_LENGTH	150
#define HUD_MAX_NUM_DISP	4
#define HUD_MAX_NUM_STOR	20

// classes - expanded whenever needed
#define HM_DEFAULT		1
#define HM_REDUNDANT		2
#define HM_MULTI		4

extern int HUD_toolong;
extern void HUD_clear_messages();
extern void HUD_render_message_frame();
extern int HUD_init_message(int class_flag, char * format, ... );

#endif
