// hud_message and message class definitions
#ifndef _HUD_MSG_H
#define _HUD_MSG_H

#define MSGC_GAME_CHEAT 	1    // Cheats enabled/disabled
#define MSGC_PICKUP_TOOMUCH	2    // Pickup failed: it's a powerup you have too much from
#define MSGC_PICKUP_ALREADY	4    // Pickup failed: it's a powerup you already have
#define MSGC_PICKUP_OK		8    // Pickup succeeded
#define MSGC_MULTI_USERMSG	16   // Netgame messages from other users
#define MSGC_MULTI_KILL 	32   // Netgame kill information
#define MSGC_MULTI_INFO 	64   // Netgame information (join/leave, reactor, exit)
#define MSGC_GAME_ACTION	128  // Something happened in the game (exit,hostage,ship dest)
#define MSGC_GAME_FEEDBACK	256  // User feedback (F3=Cockpit mode, can't pause, netmsg)
#define MSGC_MINE_FEEDBACK	512  // Mine feedback (can't open door, reactor invul)
#define MSGC_WEAPON_EMPTY	1024 // No weapons (no primary weapons available)
#define MSGC_WEAPON_SELECT	2048 // Manual weapon selection
#define MSGC_UNKNOWN		4096 // Unknown: External control interface message
#define MSGC_DEBUG		8192 // Unknown: External control interface message

#define MSGC_NOREDUNDANCY	(~(MSGC_PICKUP_TOOMUCH | MSGC_PICKUP_ALREADY))
#define MSGC_PLAYERMESSAGES	(~(MSGC_PICKUP_TOOMUCH | MSGC_PICKUP_ALREADY | MSGC_PICKUP_OK))

#define HUD_MESSAGE_LENGTH	150
#define HUD_MAX_NUM 80 //max to display in scrollback mode (and as such, the max to store, period)

extern int HUD_max_num_disp;

extern int MSG_Playermessages;
extern int MSG_Noredundancy;

//killed 11/01/98 -MM
//added on 10/04/98 by Matt Mueller to allow hud message logging
//extern int HUD_log_messages;
//end addition -MM
//end kill -MM

#ifdef __GNUC__
extern void hud_message(int class, char *format, ...)
 __attribute__ ((format (printf, 2, 3)));
#else
extern void hud_message(int class, char *format, ...);
#endif

extern void mekh_resend_last();
extern void mekh_hud_recall_msgs();
#endif
