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
COPYRIGHT 1993-1998 PARALLAX SOFTWARE CORPORATION.  ALL RIGHTS RESERVED.
*/

/*
 *
 * Game Loading editor functions
 *
 */


#include <string.h>
#include <stdio.h>

#include "inferno.h"
#include "editor.h"
#include "ui.h"
#include "gamesave.h"
#include "object.h"
#include "gameseq.h"
#include "gameseg.h"
#include "kdefs.h"
#include "d_levelstate.h"

namespace {

static std::array<char, PATH_MAX> game_filename{"*." DXX_LEVEL_FILE_EXTENSION};

static void checkforgamext(std::array<char, PATH_MAX> &f)
{
	int i;

	for (i=1; f[i]; i++ )
	{
		if (f[i]=='.') return;

		if ((f[i]==' '||f[i]==0) )
		{
			f[i]='.';
			f[i+1]='L';
			f[i+2]= 'V';
			f[i+3]= 'L';
			f[i+4]=0;
			return;
		}
	}

	if (i < 123)
	{
		f[i]='.';
		f[i+1]='L';
		f[i+2]= 'V';
		f[i+3]= 'L';
		f[i+4]=0;
		return;
	}
}

//these variables store the "permanant" player position, which overrides
//whatever the player's position happens to be when the game is saved
segnum_t Perm_player_segnum=segment_none;		//-1 means position not set
vms_vector Perm_player_position;
vms_matrix Perm_player_orient;

}

//set the player's "permanant" position from the current position
int SetPlayerPosition()
{
	Perm_player_position = ConsoleObject->pos;
	Perm_player_orient = ConsoleObject->orient;
	Perm_player_segnum = ConsoleObject->segnum;

	editor_status("Player initial position set");
	return 0;
}

// Save game
// returns 1 if successful
//	returns 0 if unsuccessful
int SaveGameData()
{
	auto &LevelSharedVertexState = LevelSharedSegmentState.get_vertex_state();
	auto &Objects = LevelUniqueObjectState.Objects;
	auto &Vertices = LevelSharedVertexState.get_vertices();
	auto &vmobjptr = Objects.vmptr;
	auto &vmobjptridx = Objects.vmptridx;
	if (gamestate == editor_gamestate::unsaved) {
		if (ui_messagebox(-2, -2, 2, "Game State has not been saved...\nContinue?\n", "NO", "Yes") == 1)
			return 0;
		}
		
   if (ui_get_filename( game_filename, "*." DXX_LEVEL_FILE_EXTENSION, "Save Level" )) {
		int saved_flag;
		vms_vector save_pos = ConsoleObject->pos;
		vms_matrix save_orient = ConsoleObject->orient;
		auto save_segnum = ConsoleObject->segnum;

      checkforgamext(game_filename);

		if (Perm_player_segnum > Highest_segment_index)
			Perm_player_segnum = segment_none;

		auto &vcvertptr = Vertices.vcptr;
		if (Perm_player_segnum!=segment_none) {
			if (get_seg_masks(vcvertptr, Perm_player_position, vcsegptr(Perm_player_segnum), 0).centermask == 0)
			{
				ConsoleObject->pos = Perm_player_position;
				ConsoleObject->orient = Perm_player_orient;
				obj_relink(vmobjptr, vmsegptr, vmobjptridx(ConsoleObject), vmsegptridx(Perm_player_segnum));
			}
			else
				Perm_player_segnum=segment_none;		//position was bogus
		}
		saved_flag = save_level(
#if defined(DXX_BUILD_DESCENT_II)
			LevelSharedSegmentState.DestructibleLights,
#endif
			game_filename.data());
		if (Perm_player_segnum!=segment_none) {

			if (save_segnum > Highest_segment_index)
				save_segnum = 0;

			ConsoleObject->pos = save_pos;
			const auto &&save_segp = vmsegptridx(save_segnum);
			auto found_save_segnum = find_point_seg(LevelSharedSegmentState, LevelUniqueSegmentState, save_pos, save_segp);
			if (found_save_segnum == segment_none) {
				found_save_segnum = save_segp;
				auto &LevelSharedVertexState = LevelSharedSegmentState.get_vertex_state();
				auto &Vertices = LevelSharedVertexState.get_vertices();
				auto &vcvertptr = Vertices.vcptr;
				compute_segment_center(vcvertptr, save_pos, save_segp);
			}

			obj_relink(vmobjptr, vmsegptr, vmobjptridx(ConsoleObject), found_save_segnum);
			ConsoleObject->orient = save_orient;
		}
		if (saved_flag)
			return 0;
		mine_changed = 0;
		gamestate = editor_gamestate::none;
	}
	return 1;
}

// returns 1 if successful
//	returns 0 if unsuccessful
int LoadGameData()
{
if (SafetyCheck())  {
	if (ui_get_filename( game_filename, "*." DXX_LEVEL_FILE_EXTENSION, "Load Level" ))
		{
		checkforgamext(game_filename);
		if (load_level(
#if defined(DXX_BUILD_DESCENT_II)
				LevelSharedSegmentState.DestructibleLights,
#endif
				game_filename.data()))
			return 0;
		Current_level_num = 1;			// assume level 1
		gamestate = editor_gamestate::none;
		Update_flags = UF_WORLD_CHANGED;
		Perm_player_position = ConsoleObject->pos;
		Perm_player_orient = ConsoleObject->orient;
		Perm_player_segnum = ConsoleObject->segnum;
		}
	}
	return 1;
}

//called whenever a new mine is created, so new mine doesn't get name
//of last saved mine as default
void ResetFilename()
{
	strcpy(game_filename.data(), "*.LVL");
}

