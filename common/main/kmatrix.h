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
 * Kill matrix.
 *
 */

#pragma once

#include "maths.h"

#ifdef __cplusplus

enum class kmatrix_result
{
	abort,
	proceed,
};

enum class kmatrix_network : uint8_t
{
	offline,
	/* This has the same numeric value as GM_NETWORK, but currently,
	 * that is not required for correct operation.
	 */
	multiplayer = 4,
};

kmatrix_result kmatrix_view(kmatrix_network network); // shows matrix screen. Retruns 0 if aborted (quitting game) and 1 if proceeding to next level if applicable.
#ifdef dsx
namespace dsx {
kmatrix_result multi_endlevel_score();
}
#endif

#endif
