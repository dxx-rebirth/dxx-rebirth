/* $Id: kmine.c,v 1.2 2004-12-19 14:52:48 btb Exp $ */
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
 *
 * Functions to change entire mines.
 *
 */

#ifdef RCS
static char rcsid[] = "$Id: kmine.c,v 1.2 2004-12-19 14:52:48 btb Exp $";
#endif

#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include "error.h"

#include "inferno.h"
#include "editor.h"
#include "ui.h"
#include "texpage.h"		// For texpage_goto_first
#include "segment.h"
#include "mono.h"
#include "kdefs.h"
#include "info.h"
#include "game.h"
#include "gameseq.h"

#include "nocfile.h"

#include "object.h"

#define MINESAVE_CRIPPLED	0

char mine_filename[128] = "*.MIN";
char sit_filename[128] = "*.SIT";

#define MAX_NAME_LENGTH 128

//	See if filename f contains an extent.  If not, add extent ext.
void checkforext( char * f, char *ext )
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
void set_extension( char * f, char *ext )
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

#if MINESAVE_CRIPPLED
int SaveMine()
{
	char  ErrorMessage[200];

	sprintf( ErrorMessage, "Save Mine not available in demo version.\n");
	MessageBox( -2, -2, 1, ErrorMessage, "Ok" );
	return 1;
}
#endif

#if !MINESAVE_CRIPPLED
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
#endif

int CreateNewMine()
{
	if (SafetyCheck())  {
		texpage_goto_first();
		create_new_mine();
		LargeView.ev_matrix = vmd_identity_matrix;	//FrontView.ev_matrix;
		set_view_target_from_segment(Cursegp);
		vm_vec_make(&Seg_scale,DEFAULT_X_SIZE,DEFAULT_Y_SIZE,DEFAULT_Z_SIZE);
		Update_flags = UF_WORLD_CHANGED;
		SetPlayerFromCurseg();
		SetPlayerPosition();		//say default is permanant position
		mine_changed = 0;
		N_found_segs = 0;
		N_selected_segs = 0;
		med_compress_mine();
		gamestate_not_restored = 0;
		init_info = 1;
		ResetFilename();
		Game_mode = GM_UNKNOWN;
		Current_level_num = 0;		//0 means not a real game
	}
	return 1;
}

int MineMenu()
{
	int x;
	char * MenuItems[] = { "New mine",
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
int med_load_situation(char * filename)
{
	if (filename[0] == 97)
		Int3();
	Int3();

	return 1;
//@@	CFILE * LoadFile;
//@@	char	mine_name[MAX_NAME_LENGTH];
//@@	char	dir_name[_MAX_DIR];
//@@	char	mine_path[MAX_NAME_LENGTH];
//@@	vms_vector	pos;
//@@	fix	mat[9];
//@@
//@@	LoadFile = cfopen( filename, "rt" );
//@@	if (!LoadFile)	{
//@@		char  ErrorMessage[200];
//@@
//@@		sprintf( ErrorMessage, "ERROR: Unable to open %s\n", filename );
//@@		MessageBox( -2, -2, 1, ErrorMessage, "Ok" );
//@@		return 1;
//@@	}
//@@
//@@	fscanf(LoadFile, "%s", &mine_name);
//@@	mprintf((0, "Mine name = [%s]\n", mine_name));
//@@
//@@	_splitpath(filename,mine_path,dir_name,NULL,NULL);
//@@	strcat(mine_path,dir_name);
//@@	strcat(mine_path,mine_name);
//@@
//@@	mprintf((0, "Mine path = [%s]\n", mine_path));
//@@
//@@	med_load_mine(mine_path);
//@@
//@@	fscanf(LoadFile, "%x %x %x", &pos.x, &pos.y, &pos.z);
//@@	mprintf((0, "Load Position = %8x %8x %8x\n", pos.x, pos.y, pos.z));
//@@	mprintf((0, "\n"));
//@@
//@@	fscanf(LoadFile, "%x %x %x", &mat[0], &mat[1], &mat[2]);
//@@	mprintf((0, "%8x %8x %8x\n", mat[0], mat[1], mat[2]));
//@@
//@@	fscanf(LoadFile, "%x %x %x", &mat[3], &mat[4], &mat[5]);
//@@	mprintf((0, "%8x %8x %8x\n", mat[3], mat[4], mat[5]));
//@@
//@@	fscanf(LoadFile, "%x %x %x", &mat[6], &mat[7], &mat[8]);
//@@	mprintf((0, "%8x %8x %8x\n", mat[6], mat[7], mat[8]));
//@@	mprintf((0, "\n"));
//@@
//@@	fscanf(LoadFile, "%i\n", &ConsoleObject->segnum);
//@@
//@@	cfclose( LoadFile );
//@@
//@@	ConsoleObject->pos = pos;
//@@	ConsoleObject->orient.m1 = mat[0];	ConsoleObject->orient.m2 = mat[1];	ConsoleObject->orient.m3 = mat[2];
//@@	ConsoleObject->orient.m4 = mat[3];	ConsoleObject->orient.m5 = mat[4];	ConsoleObject->orient.m6 = mat[5];
//@@	ConsoleObject->orient.m7 = mat[6];	ConsoleObject->orient.m8 = mat[7];	ConsoleObject->orient.m9 = mat[8];
//@@
//@@	return 0;
}

//	-----------------------------------------------------------------------------
int med_save_situation(char * filename)
{
	CFILE * SaveFile;
	char	mine_name[MAX_NAME_LENGTH];

	SaveFile = cfopen( filename, "wt" );
	if (!SaveFile)	{
		char  ErrorMessage[200];

		sprintf( ErrorMessage, "ERROR: Unable to open %s\n", filename );
		MessageBox( -2, -2, 1, ErrorMessage, "Ok" );
		return 1;
	}

	//	Write mine name.
//	strcpy(mine_name, filename);
#ifndef __LINUX__
_splitpath(filename,NULL,NULL,mine_name,NULL);
#endif
	set_extension(mine_name, "min");
	fprintf(SaveFile, "%s\n", mine_name);

	//	Write player position.
        fprintf(SaveFile, "%x %x %x\n",(unsigned int) ConsoleObject->pos.x,(unsigned int) ConsoleObject->pos.y,(unsigned int) ConsoleObject->pos.z);

	//	Write player orientation.
        fprintf(SaveFile, "%8x %8x %8x\n",(unsigned int) ConsoleObject->orient.rvec.x,(unsigned int) ConsoleObject->orient.rvec.y,(unsigned int) ConsoleObject->orient.rvec.z);
        fprintf(SaveFile, "%8x %8x %8x\n",(unsigned int) ConsoleObject->orient.uvec.x,(unsigned int) ConsoleObject->orient.uvec.y,(unsigned int) ConsoleObject->orient.uvec.z);                       
        fprintf(SaveFile, "%8x %8x %8x\n",(unsigned int) ConsoleObject->orient.fvec.x,(unsigned int) ConsoleObject->orient.fvec.y,(unsigned int) ConsoleObject->orient.fvec.z);
	fprintf(SaveFile, "%i\n", ConsoleObject->segnum);

	mprintf((0, "Save Position = %8x %8x %8x\n", ConsoleObject->pos.x, ConsoleObject->pos.y, ConsoleObject->pos.z));
	mprintf((0, "\n"));

	mprintf((0, "%8x %8x %8x\n", ConsoleObject->orient.rvec.x, ConsoleObject->orient.rvec.y, ConsoleObject->orient.rvec.z));
	mprintf((0, "%8x %8x %8x\n", ConsoleObject->orient.uvec.x, ConsoleObject->orient.uvec.y, ConsoleObject->orient.uvec.z));
	mprintf((0, "%8x %8x %8x\n", ConsoleObject->orient.fvec.x, ConsoleObject->orient.fvec.y, ConsoleObject->orient.fvec.z));
	mprintf((0, "\n"));

	cfclose( SaveFile);

	return 1;
}

//	-----------------------------------------------------------------------------
int SaveSituation(void)
{
	if (ui_get_filename( sit_filename, "*.SIT", "Save Situation" )) {
		set_extension(sit_filename, "MIN");
		if (med_save_mine(sit_filename)) {
			mprintf((0, "Unable to save mine in SaveSituation.\n"));
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

