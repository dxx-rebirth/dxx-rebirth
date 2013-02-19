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
