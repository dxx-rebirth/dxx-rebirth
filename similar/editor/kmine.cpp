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
 * Functions to change entire mines.
 *
 */

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include "dxxerror.h"
#include "strutil.h"
#include "inferno.h"
#include "editor.h"
#include "editor/esegment.h"
#include "editor/medmisc.h"
#include "ui.h"
#include "texpage.h"		// For texpage_goto_first
#include "segment.h"
#include "kdefs.h"
#include "info.h"
#include "game.h"
#include "physfsx.h"
#include "gameseq.h"
#include "object.h"

char mine_filename[PATH_MAX] = "*.MIN";
static char sit_filename[PATH_MAX] = "*.SIT";

#define MAX_NAME_LENGTH PATH_MAX

//	See if filename f contains an extent.  If not, add extent ext.
static void checkforext( char * f, const char *ext )
{
	int i;

	for (i=1; i<MAX_NAME_LENGTH; i++ ) {
		if (f[i]=='.')
			return;

		if ((f[i] == ' ') || (f[i]==0) ) {
			f[i] = '.';
			f[i+1] = ext[0];
			f[i+2] = ext[1];
			f[i+3] = ext[2];
			f[i+4] = 0;
			return;
		}
	}

	if (i < 123) {
		f[i] = '.';
		f[i+1] = ext[0];
		f[i+2] = ext[1];
		f[i+3] = ext[2];
		f[i+4] = 0;
		return;
	}
}

//	See if filename f contains an extent.  If not, add extent ext.
static void set_extension( char * f, const char *ext )
{
	int i;

	for (i=1; i<MAX_NAME_LENGTH-4; i++ ) {
		if ((f[i]=='.') || (f[i] == ' ') || (f[i]==0) ) {
			f[i] = '.';
			f[i+1] = ext[0];
			f[i+2] = ext[1];
			f[i+3] = ext[2];
			f[i+4] = 0;
			return;
		}
	}
}

int SaveMine()
{
	// Save mine
//	med_save_mine("TEMP.MIN");
    if (ui_get_filename( mine_filename, "*.MIN", "SAVE MINE" ))
	{
        checkforext(mine_filename, "MIN");
        if (med_save_mine(mine_filename))
			return 0;
		mine_changed = 0;
	}
	
	return 1;
}

int CreateNewMine()
{
	if (SafetyCheck())  {
		texpage_goto_first();
		create_new_mine();
		LargeView.ev_matrix = vmd_identity_matrix;	//FrontView.ev_matrix;
		set_view_target_from_segment(vcvertptr, Cursegp);
		Update_flags = UF_WORLD_CHANGED;
		SetPlayerFromCurseg();
		SetPlayerPosition();		//say default is permanant position
		mine_changed = 0;
		Found_segs.clear();
		Selected_segs.clear();
		med_compress_mine();
		gamestate = editor_gamestate::none;
		init_info = 1;
		ResetFilename();
		Game_mode = GM_UNKNOWN;
		Current_level_num = 1;		// make level 1
	}
	return 1;
}

int MineMenu()
{
	int x;
	static const char *const MenuItems[] = { "New mine",
					   "Load mine",
					   "Save mine",
					   "Print mine",
					   "Redraw mine" };

	x = MenuX( -1, -1, 5, MenuItems );

	switch( x )
	{
	case 1:     // New
		CreateNewMine();
		break;
	case 2:     // Load
		//@@LoadMine();
		break;
	case 3:     // Save
		SaveMine();
		break;
	case 4:     // Print
		break;
	case 5:     // Redraw
		Update_flags = UF_ALL;
		break;
	}
	return 1;
}

// -----------------------------------------------------------------------------
// returns 1 if error, else 0
static int med_load_situation(char * filename)
{
	if (filename[0] == 97)
		Int3();
	Int3();

	return 1;
}

//	-----------------------------------------------------------------------------
static int med_save_situation(char * filename)
{
	auto SaveFile = PHYSFSX_openWriteBuffered(filename);
	if (!SaveFile)	{
		char  ErrorMessage[512];

		snprintf(ErrorMessage, sizeof(ErrorMessage), "ERROR: Unable to open %.480s", filename);
		ui_messagebox( -2, -2, 1, ErrorMessage, "Ok" );
		return 1;
	}

	//	Write mine name.
	struct splitpath_t path;
	d_splitpath(filename, &path);
	PHYSFSX_printf(SaveFile, "%.*s.min\n", DXX_ptrdiff_cast_int(path.base_end - path.base_start), path.base_start);

	//	Write player position.
	PHYSFSX_printf(SaveFile, "%x %x %x\n",static_cast<unsigned>(ConsoleObject->pos.x),static_cast<unsigned>(ConsoleObject->pos.y),static_cast<unsigned>(ConsoleObject->pos.z));

	//	Write player orientation.
	PHYSFSX_printf(SaveFile, "%8x %8x %8x\n",static_cast<unsigned>(ConsoleObject->orient.rvec.x),static_cast<unsigned>(ConsoleObject->orient.rvec.y),static_cast<unsigned>(ConsoleObject->orient.rvec.z));
	PHYSFSX_printf(SaveFile, "%8x %8x %8x\n",static_cast<unsigned>(ConsoleObject->orient.uvec.x),static_cast<unsigned>(ConsoleObject->orient.uvec.y),static_cast<unsigned>(ConsoleObject->orient.uvec.z));
	PHYSFSX_printf(SaveFile, "%8x %8x %8x\n",static_cast<unsigned>(ConsoleObject->orient.fvec.x),static_cast<unsigned>(ConsoleObject->orient.fvec.y),static_cast<unsigned>(ConsoleObject->orient.fvec.z));
	PHYSFSX_printf(SaveFile, "%i\n", ConsoleObject->segnum);
	return 1;
}

//	-----------------------------------------------------------------------------
int SaveSituation(void)
{
	if (ui_get_filename( sit_filename, "*.SIT", "Save Situation" )) {
		set_extension(sit_filename, "MIN");
		if (med_save_mine(sit_filename)) {
			return 0;
		}

		set_extension(sit_filename, "SIT");
		if (med_save_situation(sit_filename))
			return 0;
	}
	
	return 1;
}

//	-----------------------------------------------------------------------------
//	Load a situation file which consists of x,y,z, orientation matrix, mine name.
int LoadSituation(void)
{
	if (SafetyCheck())  {
		if (ui_get_filename( sit_filename, "*.sit", "Load Situation" ))	{
         checkforext(sit_filename, "SIT");
         if (med_load_situation(sit_filename))
 				return 0;
			// set_view_target_from_segment(Cursegp);
			Update_flags = UF_WORLD_CHANGED;
			// SetPlayerFromCurseg();
			med_compress_mine();
			init_info = 1;
			mine_changed = 0;
		}
	}

	return 1;
}

