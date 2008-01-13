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
 * $Source: /cvsroot/dxx-rebirth/d1x-rebirth/main/switch.h,v $
 * $Revision: 1.1.1.1 $
 * $Author: zicodxx $
 * $Date: 2006/03/17 19:42:24 $
 * 
 * Triggers and Switches.
 * 
 * $Log: switch.h,v $
 * Revision 1.1.1.1  2006/03/17 19:42:24  zicodxx
 * initial import
 *
 * Revision 1.1.1.1  1999/06/14 22:13:12  donut
 * Import of d1x 1.37 source.
 *
 * Revision 2.0  1995/02/27  11:26:52  john
 * New version 2.0, which has no anonymous unions, builds with
 * Watcom 10.0, and doesn't require parsing BITMAPS.TBL.
 * 
 * Revision 1.19  1995/01/12  17:00:36  rob
 * Fixed a problem with switches and secret levels.
 * 
 * Revision 1.18  1994/10/06  21:24:40  matt
 * Added switch for exit to secret level
 * 
 * Revision 1.17  1994/09/29  17:05:52  matt
 * Removed unused constant
 * 
 * Revision 1.16  1994/09/24  17:10:07  yuan
 * Added Matcen triggers.
 * 
 * Revision 1.15  1994/08/15  18:06:39  yuan
 * Added external trigger.
 * 
 * Revision 1.14  1994/06/16  16:20:52  john
 * Made player start out in physics mode; Neatend up game loop a bit.
 * 
 * Revision 1.13  1994/05/30  20:22:08  yuan
 * New triggers.
 * 
 * Revision 1.12  1994/05/27  10:32:44  yuan
 * New dialog boxes (Walls and Triggers) added.
 * 
 * 
 * Revision 1.11  1994/05/25  18:06:32  yuan
 * Making new dialog box controls for walls and triggers.
 * 
 * Revision 1.10  1994/04/28  18:04:40  yuan
 * Gamesave added.
 * Trigger problem fixed (seg pointer is replaced by index now.)
 * 
 * Revision 1.9  1994/04/26  11:19:01  yuan
 * Make it so a trigger can only be triggered every 5 seconds.
 * 
 */

#ifndef _SWITCH_H
#define _SWITCH_H

#include "inferno.h"
#include "segment.h"

#define	MAX_WALLS_PER_LINK		10

#define	TRIGGER_DEFAULT			2*F1_0

#define	MAX_TRIGGERS				100
#define	MAX_WALL_SWITCHES			50
#define	MAX_WALL_LINKS				100

// Trigger flags	  
#define	TRIGGER_CONTROL_DOORS		1	// Control Trigger
#define	TRIGGER_SHIELD_DAMAGE		2	// Shield Damage Trigger
#define	TRIGGER_ENERGY_DRAIN	   	4	// Energy Drain Trigger
#define	TRIGGER_EXIT					8	// End of level Trigger
#define	TRIGGER_ON					  16	// Whether Trigger is active
#define	TRIGGER_ONE_SHOT			  32	// If Trigger can only be triggered once
#define	TRIGGER_MATCEN				  64	// Trigger for materialization centers
#define	TRIGGER_ILLUSION_OFF		 128	// Switch Illusion OFF trigger
#define	TRIGGER_ILLUSION_ON		 512	// Switch Illusion ON trigger
#define	TRIGGER_SECRET_EXIT		 256	// Exit to secret level

// Trigger delay times before they can be retriggered (Recharge time)
#define	TRIGGER_DELAY_DOOR		F1_0*1	// 1 second for doors
#define	TRIGGER_DELAY_ZAPS		F1_0/10	// 1/10 second for quickie stuff

// New unimplemented trigger ideas 
#define	TRIGGER_CONTROL_ROBOTS	  64	// If Trigger is a Door control trigger (Linked)
#define	CONTROL_ROBOTS					8	// If Trigger modifies robot behavior
#define	CONTROL_LIGHTS_ON			  16	// If Trigger turns on lights in a certain area
#define	CONTROL_LIGHTS_OFF		  32	// If Trigger turns off lights in a certain area

typedef struct trigger {
	sbyte		type;
	short		flags;
	fix		value;
	fix		time;
	sbyte		link_num;
	short 	num_links;
	short 	seg[MAX_WALLS_PER_LINK];
	short		side[MAX_WALLS_PER_LINK];
	} __pack__ trigger;

//typedef struct link {
//	short 	num_walls;
//	short 	seg[MAX_WALLS_PER_LINK];
//	short		side[MAX_WALLS_PER_LINK];
//	} link;

extern trigger Triggers[MAX_TRIGGERS];
//extern link Links[MAX_WALL_LINKS];

extern int Num_triggers;
//extern int Num_links;

extern void trigger_init();

extern void check_trigger(segment *seg, short side, short objnum);
extern int check_trigger_sub(int trigger_num, int player_num);

extern void triggers_frame_process();

#ifdef FAST_FILE_IO
#define trigger_read(t, fp) cfread(t, sizeof(trigger), 1, fp)
#else
/*
 * reads a trigger structure from a CFILE
 */
extern void trigger_read(trigger *t, CFILE *fp);
#endif

#endif
 
