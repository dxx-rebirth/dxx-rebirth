/*
THE COMPUTER CODE CONTAINED HEREIN IS THE SOLE PROPERTY OF PARALLAX
SOFTWARE CORPORATION ("PARALLAX").  PARALLAX, IN DISTRIBUTING THE CODE TO
END-USERS, AND SUBJECT TO ALL OF THE TERMS AND CONDITIONS HEREIN, GRANTS A
ROYALTY-FREE, PERPETUAL LICENSE TO SUCH END-USERS FOR USE BY SUCH END-USERS
IN USING, DISPLAYING,  AND CREATING DERIVATIVE WORKS THEREOF, SO LONG AS
SUCH USE, DISPLAY OR CREATION IS FOR NON-COMMERCIAL, ROYALTY OR REVENUE
FREE PURPOSES.  IN NO EVENT SHALL THE END-USER USE THE COMPUTER CODE
CONTAINED HEREIN FOR REVENUE-BEARING PURPOSES.  THE END-USER UNDERSTANDS
AND AGREES TO THE TERMS HEREIN AND ACCEPTS THE SAME BY USE OF THIS FILE.  
COPYRIGHT 1993-1999 PARALLAX SOFTWARE CORPORATION.  ALL RIGHTS RESERVED.
*/



#ifndef _AUTOMAP_H
#define _AUTOMAP_H

#include "player.h"

extern void do_automap(int key_code);
extern void automap_clear_visited();
extern ubyte Automap_visited[MAX_SEGMENTS];
extern void modex_print_message(int x, int y, char *str);
void DropBuddyMarker(object *objp);

extern int Automap_active;

#define NUM_MARKERS 				16
#define MARKER_MESSAGE_LEN		40

extern char MarkerMessage[NUM_MARKERS][MARKER_MESSAGE_LEN];
extern char MarkerOwner[NUM_MARKERS][CALLSIGN_LEN+1];
extern int	MarkerObject[NUM_MARKERS];

//added on 9/30/98 by Matt Mueller for selectable automap modes
extern u_int32_t automap_mode ; 
#define AUTOMAP_MODE (automap_use_game_res?grd_curscreen->sc_mode:automap_mode)
//extern int automap_width ;
//extern int automap_height ;
//end addition -MM
extern int automap_use_game_res;

#endif
