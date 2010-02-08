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
 * $Source: /cvsroot/dxx-rebirth/d1x-rebirth/main/scores.h,v $
 * $Revision: 1.1.1.1 $
 * $Author: zicodxx $
 * $Date: 2006/03/17 19:42:40 $
 * 
 * Scores header.
 * 
 * $Log: scores.h,v $
 * Revision 1.1.1.1  2006/03/17 19:42:40  zicodxx
 * initial import
 *
 * Revision 1.1.1.1  1999/06/14 22:13:05  donut
 * Import of d1x 1.37 source.
 *
 * Revision 2.0  1995/02/27  11:31:53  john
 * New version 2.0, which has no anonymous unions, builds with
 * Watcom 10.0, and doesn't require parsing BITMAPS.TBL.
 * 
 * Revision 1.13  1994/12/07  00:36:36  mike
 * scores sequencing stuff.
 * 
 * Revision 1.12  1994/11/28  11:26:09  matt
 * Took out scores for weapons, which are no longer used
 * 
 * Revision 1.11  1994/10/24  18:20:03  john
 * Made the current high score flash.
 * 
 * Revision 1.10  1994/10/18  18:21:36  john
 * NEw score system.
 * 
 * Revision 1.9  1994/10/03  23:01:58  matt
 * New parms for scores_view()
 * 
 * 
 * Revision 1.8  1994/09/27  16:10:37  adam
 * changed scores of course
 * 
 * Revision 1.7  1994/08/31  19:25:46  mike
 * Add score values for new powerups.
 * 
 * Revision 1.6  1994/08/26  16:00:18  mike
 * enhanced (?) scoring.
 * 
 * Revision 1.5  1994/08/26  13:01:45  john
 * Put high score system in.
 * 
 * Revision 1.4  1994/05/30  16:33:21  yuan
 * Revamping high scores.
 * 
 * Revision 1.3  1994/05/14  17:15:07  matt
 * Got rid of externs in source (non-header) files
 * 
 * Revision 1.2  1994/05/13  13:13:57  yuan
 * Added player death...
 * 
 * When you die, if just pops up a a message.
 * When game is over, a message is popped up, and if you have a high score,
 * you get to enter it.
 * 
 * Revision 1.1  1994/05/13  10:22:16  yuan
 * Initial revision
 * 
 * 
 */



#ifndef _SCORES_H
#define _SCORES_H

#define	ROBOT_SCORE				1000
#define	HOSTAGE_SCORE			1000
#define	CONTROL_CEN_SCORE		5000
#define	ENERGY_SCORE			0
#define	SHIELD_SCORE			0
#define	LASER_SCORE				0
#define	DEBRIS_SCORE			0
#define	CLUTTER_SCORE			0
#define	MISSILE1_SCORE			0
#define	MISSILE4_SCORE			0
#define	KEY_SCORE				0
#define	QUAD_FIRE_SCORE		0

#define	VULCAN_AMMO_SCORE				0
#define	CLOAK_SCORE						0
#define	TURBO_SCORE						0
#define	INVULNERABILITY_SCORE		0
#define	HEADLIGHT_SCORE				0

struct stats_info;

extern void scores_view(struct stats_info *last_game, int citem);

// If player has a high score, adds you to file and returns.
//	If abort_flag set, only show if player has gotten a high score.
extern void scores_maybe_add_player(int abort_flag);

#endif
