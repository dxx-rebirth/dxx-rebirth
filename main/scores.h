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
 * Scores header.
 *
 */


#ifndef _SCORES_H
#define _SCORES_H

#define ROBOT_SCORE             1000
#define HOSTAGE_SCORE           1000
#define CONTROL_CEN_SCORE       5000
#define ENERGY_SCORE            0
#define SHIELD_SCORE            0
#define LASER_SCORE             0
#define DEBRIS_SCORE            0
#define CLUTTER_SCORE           0
#define MISSILE1_SCORE          0
#define MISSILE4_SCORE          0
#define KEY_SCORE               0
#define QUAD_FIRE_SCORE         0

#define VULCAN_AMMO_SCORE       0
#define CLOAK_SCORE             0
#define TURBO_SCORE             0
#define INVULNERABILITY_SCORE   0
#define HEADLIGHT_SCORE         0

struct stats_info;

extern void scores_view(struct stats_info *last_game, int citem);

// If player has a high score, adds you to file and returns.
// If abort_flag set, only show if player has gotten a high score.
extern void scores_maybe_add_player(int abort_flag);

#endif /* _SCORES_H */
