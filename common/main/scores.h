/*
 * Portions of this file are copyright Rebirth contributors and licensed as
 * described in COPYING.txt.
 * Portions of this file are copyright Parallax Software and licensed
 * according to the Parallax license below.
 * See COPYING.txt for license details.

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

#include "dsx-ns.h"
#include "fwd-gr.h"

#define HOSTAGE_SCORE           1000
#define CONTROL_CEN_SCORE       5000
#define ENERGY_SCORE            0
#define SHIELD_SCORE            0
#define LASER_SCORE             0
#define KEY_SCORE               0
#define QUAD_FIRE_SCORE         0

#define VULCAN_AMMO_SCORE       0
#define CLOAK_SCORE             0
#define INVULNERABILITY_SCORE   0

#ifdef DXX_BUILD_DESCENT
namespace dsx {

void scores_view_menu(grs_canvas &);

// If player has a high score, adds you to file and returns.
// If abort_flag set, only show if player has gotten a high score.
void scores_maybe_add_player();

}
#endif

#endif /* _SCORES_H */
