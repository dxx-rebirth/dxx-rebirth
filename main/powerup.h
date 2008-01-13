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
 * $Source: /cvsroot/dxx-rebirth/d1x-rebirth/main/powerup.h,v $
 * $Revision: 1.1.1.1 $
 * $Author: zicodxx $
 * $Date: 2006/03/17 19:41:45 $
 * 
 * Powerup header file.
 * 
 * $Log: powerup.h,v $
 * Revision 1.1.1.1  2006/03/17 19:41:45  zicodxx
 * initial import
 *
 * Revision 1.1.1.1  1999/06/14 22:13:01  donut
 * Import of d1x 1.37 source.
 *
 * Revision 2.0  1995/02/27  11:27:35  john
 * New version 2.0, which has no anonymous unions, builds with
 * Watcom 10.0, and doesn't require parsing BITMAPS.TBL.
 * 
 * Revision 1.34  1995/02/06  15:52:37  mike
 * add mini megawow powerup for giving reasonable weapons.
 * 
 * Revision 1.33  1995/01/30  17:14:11  mike
 * halve rate of vulcan ammo consumption.
 * 
 * Revision 1.32  1995/01/15  20:47:56  mike
 * add lighting field to powerups.
 * 
 * Revision 1.31  1994/12/12  21:39:58  matt
 * Changed vulcan ammo: 10K max, 5K w/weapon, 1250 per powerup
 * 
 * Revision 1.30  1994/12/07  12:55:26  mike
 * tweak vulcan amounts.
 * 
 * Revision 1.29  1994/12/02  20:06:46  matt
 * Made vulcan ammo print at approx 25 times actual
 * 
 * Revision 1.28  1994/10/26  15:56:27  yuan
 * Made vulcan cannon give 100 ammo if it has less than that.
 * 
 * Revision 1.27  1994/10/15  19:07:10  mike
 * Define constants for amount of vulcan ammo per powerup.
 * 
 * Revision 1.26  1994/09/26  13:29:38  matt
 * Added extra life each 100,000 points, and show icons on HUD for num lives
 * 
 * Revision 1.25  1994/09/22  19:00:25  mike
 * Kill constants ENERGY_BOOST and SHIELD_BOOST: it's now difficulty level dependent.
 * 
 * Revision 1.24  1994/09/20  19:46:11  mike
 * Fix powerup number assignments.
 * 
 * Revision 1.23  1994/09/02  11:53:34  mike
 * Add the megawow powerup.  If you don't know about it, that's because it's a secret.
 * 
 * Revision 1.22  1994/09/01  10:41:35  matt
 * Sizes for powerups now specified in bitmaps.tbl; blob bitmaps now plot
 * correctly if width & height of bitmap are different.
 * 
 * Revision 1.21  1994/08/31  19:26:14  mike
 * Start adding new pile of powerups.
 * 
 * Revision 1.20  1994/08/25  17:56:08  matt
 * Added quad laser powerup
 * 
 * Revision 1.19  1994/08/18  15:11:50  mike
 * missile powerups.
 * 
 * Revision 1.18  1994/08/09  17:54:33  adam
 * upped no. of powerup types
 * 
 * Revision 1.17  1994/08/09  17:53:39  adam
 * *** empty log message ***
 * 
 * Revision 1.16  1994/07/27  19:44:16  mike
 * Objects containing objects.
 * 
 * Revision 1.15  1994/07/26  18:31:32  mike
 * Move some constants here from eobject.c.
 * 
 * Revision 1.14  1994/07/20  17:35:03  yuan
 * Some minor bug fixes and new key gauges...
 * 
 * Revision 1.13  1994/07/12  15:53:23  john
 * *** empty log message ***
 * 
 * Revision 1.12  1994/07/12  15:30:47  mike
 * Prototype diminish_towards_max.
 * 
 * Revision 1.11  1994/07/07  14:59:04  john
 * Made radar powerups.
 * 
 * 
 * Revision 1.10  1994/07/01  16:35:40  yuan
 * Added key system
 * 
 * Revision 1.9  1994/06/29  19:43:33  matt
 * Made powerup animation not happen in render routine
 * 
 * Revision 1.8  1994/06/21  18:54:03  matt
 * Added support for powerups that don't get picked up if not needed, but this
 * feature is commented out at the end of do_powerup(), since the physics gave 
 * me all sorts of problems, with the player getting stuck on a powerup.
 * 
 * Revision 1.7  1994/06/08  18:16:32  john
 * Bunch of new stuff that basically takes constants out of the code
 * and puts them into bitmaps.tbl.
 * 
 * Revision 1.6  1994/05/18  13:26:30  yuan
 * *** empty log message ***
 * 
 * Revision 1.5  1994/05/17  17:01:48  yuan
 * Added constant for boosts.
 * 
 * Revision 1.4  1994/04/06  14:42:50  yuan
 * Adding new powerups.
 * 
 * Revision 1.3  1994/04/01  14:36:59  yuan
 * John's head is an extra life...
 * 
 * Revision 1.2  1994/04/01  11:15:22  yuan
 * Added multiple bitmap functionality to all objects...
 * (hostages, powerups, lasers, etc.)  
 * Hostages and powerups are implemented in the object system,
 * just need to finish function call to "affect" player.
 * 
 * Revision 1.1  1994/03/31  17:01:43  yuan
 * Initial revision
 * 
 * 
 */



#ifndef _POWERUP_H
#define _POWERUP_H

#include "vclip.h"
#include "player.h"

#define	POW_EXTRA_LIFE 			0
#define	POW_ENERGY					1
#define	POW_SHIELD_BOOST			2
#define	POW_LASER					3

#define	POW_KEY_BLUE				4
#define	POW_KEY_RED					5
#define	POW_KEY_GOLD				6

#define	POW_RADAR_ROBOTS			7
#define	POW_RADAR_POWERUPS		8
#define	POW_FULL_MAP				9

#define	POW_MISSILE_1				10
#define	POW_MISSILE_4				11

#define	POW_QUAD_FIRE				12

//--#define	POW_VULCAN_WEAPON			13
//--#define	POW_SPREADFIRE_WEAPON	14
//--#define	POW_PLASMA_WEAPON			15
//--#define	POW_FUSION_WEAPON			16
//--#define	POW_PROXIMITY_WEAPON		17
//--#define	POW_SMARTBOMB_WEAPON		18
//--#define	POW_MEGA_WEAPON			19
//--#define	POW_VULCAN_AMMO			20
//--#define	POW_HOMING_AMMO_1			21
//--#define	POW_HOMING_AMMO_4			22
//--#define	POW_CLOAK					23
//--#define	POW_TURBO					24
//--#define	POW_INVULNERABILITY		25
//--#define	POW_HEADLIGHT				26
//--#define	POW_MEGAWOW					27

#define	POW_VULCAN_WEAPON			13
#define	POW_SPREADFIRE_WEAPON	14
#define	POW_PLASMA_WEAPON			15
#define	POW_FUSION_WEAPON			16
#define	POW_PROXIMITY_WEAPON		17
#define	POW_HOMING_AMMO_1 		18 //21
#define  POW_HOMING_AMMO_4       19 //22
#define  POW_SMARTBOMB_WEAPON    20 //18
#define	POW_MEGA_WEAPON			21	//19
#define	POW_VULCAN_AMMO			22	//20
#define	POW_CLOAK					23
#define	POW_TURBO					24
#define	POW_INVULNERABILITY		25
#define	POW_HEADLIGHT				26
#define	POW_MEGAWOW					27

#define	VULCAN_AMMO_MAX				(392*2)
#define	VULCAN_WEAPON_AMMO_AMOUNT	196
#define	VULCAN_AMMO_AMOUNT			(49*2)

// What I picked up        What it said I picked up
// ----------------        ------------------------
// vulcan ammo             4 homing missiles
// mega missile            1 homing missile
// smart missile           vulcan ammo
// 4 homing missiles       mega missile
// 1 homing missile        smart missile
// 
// The rest were correct.  I can help you with this whenever you're free.

#define MAX_POWERUP_TYPES			29

#define	POWERUP_NAME_LENGTH	16		//	Length of a robot or powerup name.
extern char	Powerup_names[MAX_POWERUP_TYPES][POWERUP_NAME_LENGTH];

typedef struct powerup_type_info {
	int	vclip_num;
	int	hit_sound;
	fix	size;			//3d size of longest dimension
	fix	light;		//	amount of light cast by this powerup, set in bitmaps.tbl
} __pack__ powerup_type_info;

extern int N_powerup_types;
extern powerup_type_info Powerup_info[MAX_POWERUP_TYPES];

void draw_powerup(object *obj);

//returns true if powerup consumed
int do_powerup(object *obj);

//process (animate) a powerup for one frame
void do_powerup_frame(object *obj);

//	Diminish shields and energy towards max in case they exceeded it.
extern void diminish_towards_max(void);

extern void do_megawow_powerup(int quantity);

extern void powerup_basic(int redadd, int greenadd, int blueadd, int score, char *format, ...);

extern void count_powerup_start_level();

extern int powerup_start_level[MAX_POWERUP_TYPES];

extern void pow_count_level();

extern void pow_add_random();

// add pow_count to powerup_start_level
extern void pow_add_level_pow_count(int *pow_count);

// fill pow_count with items that would be dropped if player dies
extern void player_to_pow_count(player *player, int *pow_count);

#define NUM_PLAYER_DROP_POWERUPS 16
extern int player_drop_powerups[NUM_PLAYER_DROP_POWERUPS];

// reduce player drop powerups
extern void clip_player_pow_count(int *pow_count);

// may this powerup be added to the level?
// returns number of powerups left if true, otherwise 0.
extern int may_create_powerup(int powerup);

#ifdef FAST_FILE_IO
#define powerup_type_info_read_n(pti, n, fp) cfread(pti, sizeof(powerup_type_info), n, fp)
#else
/*
 * reads n powerup_type_info structs from a CFILE
 */
extern int powerup_type_info_read_n(powerup_type_info *pti, int n, CFILE *fp);
#endif

#endif /* _POWERUP_H */
 
