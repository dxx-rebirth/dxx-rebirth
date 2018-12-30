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
 * Headers for gamesave.c
 *
 */


#ifndef _GAMESAVE_H
#define _GAMESAVE_H

#include "pstypes.h"

#ifdef __cplusplus
#include "fwd-segment.h"

#define D1X_LEVEL_FILE_EXTENSION	"RDL"
#define D2X_LEVEL_FILE_EXTENSION	"RL2"

#if defined(DXX_BUILD_DESCENT_I)
#define	NUM_SHAREWARE_LEVELS	7
#define	NUM_REGISTERED_LEVELS	23

extern const char Shareware_level_names[NUM_SHAREWARE_LEVELS][12];
extern const char Registered_level_names[NUM_REGISTERED_LEVELS][12];

int convert_tmap(int tmap);	// for gamemine.c
#define DXX_LEVEL_FILE_EXTENSION	D1X_LEVEL_FILE_EXTENSION
#elif defined(DXX_BUILD_DESCENT_II)
#define DXX_LEVEL_FILE_EXTENSION	D2X_LEVEL_FILE_EXTENSION
#endif
void LoadGame(void);
void SaveGame(void);
int get_level_name(void);

#ifdef dsx
namespace dsx {
int load_level(
#if defined(DXX_BUILD_DESCENT_II)
	d_level_shared_destructible_light_state &LevelSharedDestructibleLightState,
#endif
	const char *filename);
int save_level(
#if defined(DXX_BUILD_DESCENT_II)
	const d_level_shared_destructible_light_state &LevelSharedDestructibleLightState,
#endif
	const char *filename);
}
#endif

extern char Gamesave_current_filename[PATH_MAX];

extern int Gamesave_current_version;

extern int Gamesave_num_org_robots;

// In dumpmine.c
extern void write_game_text_file(const char *filename);

extern int Errors_in_mine;

#endif

#endif /* _GAMESAVE_H */
