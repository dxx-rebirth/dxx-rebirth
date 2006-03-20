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
COPYRIGHT 1993-1998 PARALLAX SOFTWARE CORPORATION.  ALL RIGHTS RESERVED.
*/
/*
 * $Source: /cvsroot/dxx-rebirth/d1x-rebirth/main/gauges.h,v $
 * $Revision: 1.1.1.1 $
 * $Author: zicodxx $
 * $Date: 2006/03/17 19:44:52 $
 * 
 * Prototypes and defines for gauges
 * 
 * $Log: gauges.h,v $
 * Revision 1.1.1.1  2006/03/17 19:44:52  zicodxx
 * initial import
 *
 * Revision 1.1.1.1  1999/06/14 22:12:27  donut
 * Import of d1x 1.37 source.
 *
 * Revision 2.0  1995/02/27  11:28:45  john
 * New version 2.0, which has no anonymous unions, builds with
 * Watcom 10.0, and doesn't require parsing BITMAPS.TBL.
 * 
 * Revision 1.27  1994/12/14  18:06:39  matt
 * Added prototype
 * 
 * Revision 1.26  1994/12/09  16:19:52  yuan
 * kill matrix stuff.
 * 
 * Revision 1.25  1994/10/25  11:07:34  mike
 * Prototype play_homing_warning.
 * 
 * Revision 1.24  1994/10/24  16:34:39  mike
 * Increase MAX_GAUGE_BMS from 56 to 80...
 * 
 * Revision 1.23  1994/10/21  20:43:47  mike
 * Prototype add_bonus_points_to_score.
 * 
 * Revision 1.22  1994/10/14  15:56:33  mike
 * Prototype update_laser_weapon_info.
 * 
 * Revision 1.21  1994/10/13  15:17:26  mike
 * Remove afterburner references.
 * 
 * Revision 1.20  1994/10/05  17:09:46  matt
 * Added functional reticle
 * 
 * Revision 1.19  1994/10/04  21:41:29  matt
 * Added cloaked player gauge effect, and different ship bitmap for each player
 * 
 * Revision 1.18  1994/09/26  13:29:40  matt
 * Added extra life each 100,000 points, and show icons on HUD for num lives
 * 
 * Revision 1.17  1994/09/20  11:56:08  matt
 * Added prototype
 * 
 * Revision 1.16  1994/09/20  00:11:03  matt
 * Finished gauges for Status Bar, including hostage video display.
 * 
 * Revision 1.15  1994/09/17  23:57:18  matt
 * Got some, but not all, off the status bar gauges working
 * 
 * Revision 1.14  1994/09/16  13:08:46  mike
 * Prototype say_afterburner_status.
 * 
 * Revision 1.13  1994/09/14  16:27:03  mike
 * Prototype player_dead_message();
 * 
 * 
 * Revision 1.12  1994/07/20  17:34:43  yuan
 * Some minor bug fixes and new key gauges...
 * 
 * Revision 1.11  1994/07/14  14:46:02  yuan
 * Added score effect.
 * 
 * Revision 1.10  1994/07/12  16:22:00  yuan
 * Increased number of maximum gauges.
 * 
 * Revision 1.9  1994/07/11  20:10:36  yuan
 * Numerical gauges.
 * 
 * Revision 1.8  1994/07/10  18:01:28  yuan
 * Added new gauges.
 * 
 * Revision 1.7  1994/06/21  15:08:22  john
 * Made demo record HUD message and cleaned up the HUD code.
 * 
 * Revision 1.6  1994/06/21  12:40:46  yuan
 * Fixing HUD message.
 * 
 * Revision 1.5  1994/06/21  12:11:56  yuan
 * Fixed up menus and added HUDisplay messages.
 * 
 * Revision 1.4  1994/04/28  21:34:24  mike
 * prototype check_erase_gauge
 * 
 * Revision 1.3  1994/04/06  14:42:46  yuan
 * Adding new powerups.
 * 
 * Revision 1.2  1993/12/05  22:48:58  matt
 * Reworked include files in an attempt to cut down on build times
 * 
 * Revision 1.1  1993/12/05  21:07:55  matt
 * Initial revision
 * 
 * 
 */



#ifndef _GAUGES_H
#define _GAUGES_H

#include "fix.h"
#include "gr.h"
#include "piggy.h"
#include "hudmsg.h"

//from gauges.c

#define MAX_GAUGE_BMS 80	//	increased from 56 to 80 by a very unhappy MK on 10/24/94.

extern bitmap_index Gauges[MAX_GAUGE_BMS];   // Array of all gauge bitmaps.

extern void init_gauge_canvases();
extern void close_gauge_canvases();

extern void show_score();
extern void show_score_added();
extern void add_points_to_score();
extern void add_bonus_points_to_score();

void render_gauges(void);
void init_gauges(void);
extern void check_erase_message(void);

// Call to flash a message on the HUD
extern void HUD_render_message_frame();
//extern void HUD_init_message(char * format, ... );
extern void HUD_clear_messages();

//#define gauge_message HUD_init_message


extern void draw_hud();		//draw all the HUD stuff

extern void player_dead_message(void);
// extern void say_afterburner_status(void);

//fills in the coords of the hostage video window
void get_hostage_window_coords(int *x,int *y,int *w,int *h);

//from testgaug.c

void gauge_frame(void);
extern void update_laser_weapon_info(void);
extern void play_homing_warning(void);

typedef struct {
	ubyte r,g,b;
} rgb;

extern rgb player_rgb[];

//added 02/07/99 Matt Mueller
extern int Gauge_hud_mode;
//edited 02/10/99 Matt Mueller - add another mode
#define GAUGE_HUD_NUMMODES 4
//end edit -MM
//end addition -MM
//added 2/8/99 Victor Rachels
extern int gauge_update_hud_mode;
//end addition -VR

#endif
 
