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
 * $Source: /cvs/cvsroot/d2x/main/editor/kmine.c,v $
 * $Revision: 1.1 $
 * $Author: btb $
 * $Date: 2004-12-19 13:54:27 $
 *
 * Functions to change entire mines.
 *
 * $Log: not supported by cvs2svn $
 * Revision 1.1.1.1  1999/06/14 22:03:28  donut
 * Import of d1x 1.37 source.
 *
 * Revision 2.0  1995/02/27  11:34:22  john
 * Version 2.0! No anonymous unions, Watcom 10.0, with no need
 * for bitmaps.tbl.
 * 
 * Revision 1.53  1995/02/22  15:04:52  allender
 * remove anonymous unions from vecmat stuff
 * 
 * Revision 1.52  1994/11/27  23:17:13  matt
 * Made changes for new mprintf calling convention
 * 
 * Revision 1.51  1994/11/17  14:48:05  mike
 * validation functions moved from editor to game.
 * 
 * Revision 1.50  1994/11/17  11:38:49  matt
 * Ripped out code to load old mines
 * 
 * Revision 1.49  1994/10/08  17:10:22  matt
 * Correctly set current_level_num when loading/creating mine in editor
 * 
 * Revision 1.48  1994/10/03  11:30:45  matt
 * Fixed problem with permanant player position when creating a new mine
 * 
 * Revision 1.47  1994/09/29  17:42:19  matt
 * Cleaned up game_mode a little
 * 
 * Revision 1.46  1994/08/18  10:48:21  john
 * Cleaned up game sequencing.
 * 
 * Revision 1.45  1994/08/09  16:05:36  john
 * Added the ability to place players.  Made old
 * Player variable be ConsoleObject.
 * 
 * Revision 1.44  1994/07/22  12:37:06  matt
 * Cleaned up editor/game interactions some more.
 * 
 * Revision 1.43  1994/07/21  17:26:50  matt
 * When new mine created, the default save filename is now reset
 * 
 * Revision 1.42  1994/06/08  14:29:25  matt
 * Took out support for old mine versions
 * 
 * Revision 1.41  1994/06/03  12:28:04  yuan
 * Fixed game restore state.
 * 
 * Revision 1.40  1994/05/19  12:10:29  matt
 * Use new vecmat macros and globals
 * 
 * Revision 1.39  1994/05/14  17:17:56  matt
 * Got rid of externs in source (non-header) files
 * 
 * Revision 1.38  1994/05/12  14:47:47  mike
 * New previous mine structure and object structure.
 * 
 * Revision 1.37  1994/05/06  12:52:12  yuan
 * Adding some gamesave checks...
 * 
 * Revision 1.36  1994/05/05  20:37:02  yuan
 * Added gamesave checks when entering and leaving the game.
 * 
 * Removed Load Game Save Game functions...
 * Now there is only Load/Save Mine... (equivalent to old Load/Save Game)
 * 
 * Revision 1.35  1994/04/27  22:57:54  matt
 * Made sit mine load from path of sit file
 * 
 * Revision 1.34  1994/04/21  18:29:55  matt
 * Don't use same variable for mine filename & sit filename
 * 
 * Revision 1.33  1994/04/21  18:21:43  matt
 * Strip path from mine filename in sit file
 * 
 * Revision 1.32  1994/04/18  10:54:35  mike
 * Add situation save/load
 * 
 * Revision 1.31  1994/02/16  16:47:54  yuan
 * Removed temp.min.
 * 
 * Revision 1.30  1994/02/16  15:22:51  yuan
 * Checking in for editor make.
 * 
 * Revision 1.29  1994/02/09  15:04:23  yuan
 * brought back save ability
 * 
 * Revision 1.28  1994/02/08  12:42:45  yuan
 * fixed log.
 * 
 * Revision 1.27  1994/02/08  12:41:47  yuan
 *	Crippled save mine function from demo version.
 * 
 * Revision 1.26  1994/01/13  13:26:05  yuan
 * Added med_compress_mine when creating new mine or
 * when loading mine
 * 
 * Revision 1.25  1994/01/11  12:03:23  yuan
 * Fixed so that when old mine implementation not in,
 * message is displayed when you try to load an old mine
 * 
 * Revision 1.24  1994/01/11  11:47:57  yuan
 * *** empty log message ***
 * 
 * Revision 1.23  1994/01/05  09:59:56  yuan
 * Added load old mine funciton
 * 
 * Revision 1.22  1993/12/16  15:58:08  john
 * moved texture selection page to texpage.c
 * ,
 * 
 * Revision 1.21  1993/12/10  14:48:55  mike
 * Kill orthogonal views.
 * 
 * Revision 1.20  1993/12/03  16:44:06  yuan
 * Changed some 0.0 return values to 0
 * 
 * 
 * Revision 1.19  1993/12/02  12:39:34  matt
 * Removed extra includes
 * 
 * Revision 1.18  1993/11/17  13:14:48  yuan
 * Moved Save Group to group.c
 * 
 * Revision 1.17  1993/11/16  17:25:48  yuan
 * Unworking group function added... 
 * 
 * Revision 1.16  1993/11/15  14:46:25  john
 * Changed Menu to MenuX
 * 
 * Revision 1.15  1993/11/08  19:13:45  yuan
 * Added Undo command (not working yet)
 * 
 */

#ifdef RCS
static char rcsid[] = "$Id: kmine.c,v 1.1 2004-12-19 13:54:27 btb Exp $";
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

