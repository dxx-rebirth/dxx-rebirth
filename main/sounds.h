/* $Id: sounds.h,v 1.5 2003-10-10 09:36:35 btb Exp $ */
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

/*
 *
 * Numbering system for the sounds.
 *
 * Old Log:
 * Revision 1.1  1995/05/16  16:03:32  allender
 * Initial revision
 *
 * Revision 2.0  1995/02/27  11:27:32  john
 * New version 2.0, which has no anonymous unions, builds with
 * Watcom 10.0, and doesn't require parsing BITMAPS.TBL.
 *
 * Revision 1.41  1995/02/03  17:08:28  john
 * Changed sound stuff to allow low memory usage.
 * Also, changed so that Sounds isn't an array of digi_sounds, it
 * is a ubyte pointing into GameSounds, this way the digi.c code that
 * locks sounds won't accidentally unlock a sound that is already playing, but
 * since it's Sounds[soundno] is different, it would erroneously be unlocked.
 *
 * Revision 1.40  1995/01/30  21:45:17  adam
 * added weapon change sounds
 *
 * Revision 1.39  1995/01/30  21:11:57  mike
 * Use new weapon selection sounds, different for primary and secondary.
 *
 * Revision 1.38  1995/01/26  17:02:58  mike
 * make fusion cannon have more chrome, make fusion, mega rock you!
 *
 * Revision 1.37  1995/01/18  19:46:15  matt
 * Added sound for invulnerability wearing off, and voice message for cheating
 *
 * Revision 1.36  1995/01/18  19:05:04  john
 * Increased MAX_SOUNDS.
 *
 * Revision 1.35  1994/12/14  16:57:08  adam
 * *** empty log message ***
 *
 * Revision 1.34  1994/12/08  21:31:40  adam
 * *** empty log message ***
 *
 * Revision 1.33  1994/12/08  12:33:01  mike
 * make boss dying more interesting.
 *
 * Revision 1.32  1994/12/04  21:39:40  matt
 * Added sound constants for endlevel explosions
 *
 * Revision 1.31  1994/11/30  14:02:58  mike
 * see/claw/attack sounds.
 *
 * Revision 1.30  1994/11/29  20:43:54  matt
 * Deleted a bunch of unused constants
 *
 * Revision 1.29  1994/11/29  15:48:11  matt
 * Cleaned up, & added new sounds
 *
 * Revision 1.28  1994/11/29  14:35:36  adam
 * moved lava noise index
 *
 * Revision 1.27  1994/11/29  13:23:30  matt
 * Cleaned up sound constants
 *
 * Revision 1.26  1994/11/29  13:01:04  rob
 * ADded badass explosion define.
 *
 * Revision 1.25  1994/11/29  11:34:23  rob
 * Added new HUD sounds.
 *
 * Revision 1.24  1994/11/15  16:52:01  mike
 * hiss sound placeholder.
 *
 * Revision 1.23  1994/10/25  16:21:45  adam
 * changed homing sound
 *
 * Revision 1.22  1994/10/23  00:27:34  matt
 * Made exploding wall do one long sound, instead of lots of small ones
 *
 * Revision 1.21  1994/10/22  14:12:35  mike
 * Add sound for missile tracking player.
 *
 * Revision 1.20  1994/10/11  12:25:21  matt
 * Added "hot rocks" that create badass explosion when hit by weapons
 *
 * Revision 1.19  1994/10/10  20:57:50  matt
 * Added sound for exploding wall (hostage door)
 *
 * Revision 1.18  1994/10/04  15:33:31  john
 * Took out the old PLAY_SOUND??? code and replaced it
 * with direct calls into digi_link_??? so that all sounds
 * can be made 3d.
 *
 * Revision 1.17  1994/09/29  21:13:41  john
 * Added Master volumes for digi and midi. Also took out panning,
 * because it doesn't work with MasterVolume stuff.
 *
 * Revision 1.16  1994/09/29  00:42:29  matt
 * Made hitting a locked door play a sound
 *
 * Revision 1.15  1994/09/20  19:14:34  john
 * Massaged the sound system; used a better formula for determining
 * which l/r balance, also, put in Mike's stuff that searches for a connection
 * between the 2 sounds' segments, stopping for closed doors, etc.
 *
 * Revision 1.14  1994/07/06  15:23:59  john
 * Revamped hostage sound.
 *
 * Revision 1.13  1994/06/21  19:13:27  john
 * *** empty log message ***
 *
 * Revision 1.12  1994/06/21  12:09:54  adam
 * *** empty log message ***
 *
 * Revision 1.11  1994/06/21  12:03:15  john
 * Added more sounds.
 *
 * Revision 1.10  1994/06/20  22:01:54  matt
 * Added prototype for Play3D()
 *
 * Revision 1.9  1994/06/20  21:06:06  yuan
 * Fixed up menus.
 *
 * Revision 1.8  1994/06/17  12:21:54  mike
 * Hook for robot hits player.
 *
 * Revision 1.7  1994/06/15  19:01:35  john
 * Added the capability to make 3d sounds play just once for the
 * laser hit wall effects.
 *
 * Revision 1.6  1994/06/08  11:43:03  john
 * Enable 3D sound.
 *
 * Revision 1.5  1994/06/07  18:21:20  john
 * Start changing sound to the new 3D system.
 *
 * Revision 1.4  1994/05/16  16:17:41  john
 * Bunch of stuff on my Inferno Task list May16-23
 *
 * Revision 1.3  1994/05/09  21:11:38  john
 * Sound changes; pass index instead of pointer to digi routines.
 * Made laser sound cut off the last laser sound.
 *
 * Revision 1.2  1994/04/27  11:47:46  john
 * First version.
 *
 * Revision 1.1  1994/04/26  10:44:36  john
 * Initial revision
 *
 *
 */


#ifndef _SOUNDS_H
#define _SOUNDS_H

#include "vecmat.h"
#include "digi.h"

//------------------- List of sound effects --------------------

#define SOUND_LASER_FIRED                       10

#define SOUND_WEAPON_HIT_DOOR                   27
#define SOUND_WEAPON_HIT_BLASTABLE              11
#define SOUND_BADASS_EXPLOSION                  11  // need something different for this if possible

#define SOUND_ROBOT_HIT_PLAYER                  17
#define SOUND_ROBOT_SUCKED_PLAYER               SOUND_ROBOT_HIT_PLAYER // Robot sucked energy from player.

#define SOUND_ROBOT_HIT                         20
#define SOUND_ROBOT_DESTROYED                   21
#define SOUND_VOLATILE_WALL_HIT                 21
#define SOUND_LASER_HIT_WATER                   232
#define SOUND_MISSILE_HIT_WATER                 233

#define SOUND_LASER_HIT_CLUTTER                 30
#define SOUND_CONTROL_CENTER_HIT                30
#define SOUND_EXPLODING_WALL                    31  // one long sound
#define SOUND_CONTROL_CENTER_DESTROYED          31

#define SOUND_CONTROL_CENTER_WARNING_SIREN      32
#define SOUND_MINE_BLEW_UP                      33

#define SOUND_FUSION_WARMUP                     34
#define SOUND_DROP_WEAPON                       39

#define SOUND_FORCEFIELD_BOUNCE_PLAYER          40
#define SOUND_FORCEFIELD_BOUNCE_WEAPON          41
#define SOUND_FORCEFIELD_HUM                    42
#define SOUND_FORCEFIELD_OFF                    43

#define SOUND_MARKER_HIT                        50
#define SOUND_BUDDY_MET_GOAL                    51

#define SOUND_REFUEL_STATION_GIVING_FUEL        62

#define SOUND_PLAYER_HIT_WALL                   70
#define SOUND_PLAYER_GOT_HIT                    71

#define SOUND_HOSTAGE_RESCUED                   91

#define SOUND_BRIEFING_HUM                      94
#define SOUND_BRIEFING_PRINTING                 95

#define SOUND_COUNTDOWN_0_SECS                  100 // countdown 100..114
#define SOUND_COUNTDOWN_13_SECS                 113
#define SOUND_COUNTDOWN_29_SECS                 114

#define SOUND_HUD_MESSAGE                       117
#define SOUND_HUD_KILL                          118

#define SOUND_HOMING_WARNING                    122 // Warning beep: You are being tracked by a missile! Borrowed from old repair center sounds.

#define SOUND_HUD_JOIN_REQUEST                  123
#define SOUND_HUD_BLUE_GOT_FLAG                 124
#define SOUND_HUD_RED_GOT_FLAG                  125
#define SOUND_HUD_YOU_GOT_FLAG                  126
#define SOUND_HUD_BLUE_GOT_GOAL                 127
#define SOUND_HUD_RED_GOT_GOAL                  128
#define SOUND_HUD_YOU_GOT_GOAL                  129

#define SOUND_LAVAFALL_HISS                     150 // under a lavafall
#define SOUND_VOLATILE_WALL_HISS                151 // need a hiss sound here.
#define SOUND_SHIP_IN_WATER                     152 // sitting (or moving though) water
#define SOUND_SHIP_IN_WATERFALL                 158 // under a waterfall

#define SOUND_GOOD_SELECTION_PRIMARY            153
#define SOUND_BAD_SELECTION                     156

#define SOUND_GOOD_SELECTION_SECONDARY          154 // Adam: New sound number here! MK, 01/30/95
#define SOUND_ALREADY_SELECTED                  155 // Adam: New sound number here! MK, 01/30/95

#define SOUND_CLOAK_ON                          160 // USED FOR WALL CLOAK
#define SOUND_CLOAK_OFF                         161 // sound when cloak goes away
#define SOUND_INVULNERABILITY_OFF               163 // sound when invulnerability goes away

#define SOUND_BOSS_SHARE_SEE                    183
#define SOUND_BOSS_SHARE_DIE                    185

#define ROBOT_SEE_SOUND_DEFAULT                 170
#define ROBOT_ATTACK_SOUND_DEFAULT              171
#define ROBOT_CLAW_SOUND_DEFAULT                190

#define SOUND_BIG_ENDLEVEL_EXPLOSION            SOUND_EXPLODING_WALL
#define SOUND_TUNNEL_EXPLOSION                  SOUND_EXPLODING_WALL

#define SOUND_DROP_BOMB                         26

#define SOUND_CHEATER                           200

#define SOUND_AMBIENT_LAVA                      222
#define SOUND_AMBIENT_WATER                     223

#define SOUND_CONVERT_ENERGY                    241
#define SOUND_WEAPON_STOLEN                     244

#define SOUND_LIGHT_BLOWNUP                     157

#define SOUND_WALL_REMOVED                      246 // Wall removed, probably due to a wall switch.
#define SOUND_AFTERBURNER_IGNITE                247
#define SOUND_AFTERBURNER_PLAY                  248

#define SOUND_SECRET_EXIT                       249

#define SOUND_WALL_CLOAK_ON                     SOUND_CLOAK_ON
#define SOUND_WALL_CLOAK_OFF                    SOUND_CLOAK_OFF

#define SOUND_SEISMIC_DISTURBANCE_START         251

#define SOUND_YOU_GOT_ORB                       84
#define SOUND_FRIEND_GOT_ORB                    85
#define SOUND_OPPONENT_GOT_ORB                  86
#define SOUND_OPPONENT_HAS_SCORED               87

//--------------------------------------------------------------
#define MAX_SOUNDS  254     // bad to have sound 255!

// I think it would be nice to have a scrape sound...
//#define SOUND_PLAYER_SCRAPE_WALL                72

extern ubyte Sounds[MAX_SOUNDS];
extern ubyte AltSounds[MAX_SOUNDS];

#endif /* _SOUNDS_H */
