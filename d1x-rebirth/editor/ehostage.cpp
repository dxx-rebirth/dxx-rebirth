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
 * Routines for placing hostages, etc...
 *
 */


#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>

#include "dxxerror.h"
#include "screens.h"
#include "editor/esegment.h"
#include "ehostage.h"

#include "timer.h"
#include "objpage.h"
#include "maths.h"
#include "gameseg.h"
#include "kdefs.h"
#include	"object.h"
#include "polyobj.h"
#include "game.h"
#include "powerup.h"
#include "ai.h"
#include "hostage.h"
#include "key.h"
#include "bm.h"
#include "sounds.h"
#include "centers.h"
#include "piggy.h"
#include "u_mem.h"
#include "event.h"

#include "compiler-make_unique.h"

//-------------------------------------------------------------------------
// Variables for this module...
//-------------------------------------------------------------------------
static UI_DIALOG 				*MainWindow = NULL;
static int						CurrentHostageIndex = -1;
static int						LastHostageIndex = -1;

namespace {

struct hostage_dialog
{
	std::unique_ptr<UI_GADGET_USERBOX> hostageViewBox;
	std::unique_ptr<UI_GADGET_BUTTON> quitButton, next, prev, compress, delete_object, new_object;

	vclip			*vclip_ptr;				// Used for the vclip on monitor
	fix64			time;
	fix 			vclip_animation_time;			// How long the rescue sequence has been playing
	fix 			vclip_playback_speed;				// Calculated internally.  Frames/second of vclip.
};

}

static int SelectPrevHostage()	{
	int start=0;

	do	{
		CurrentHostageIndex--;
		if ( CurrentHostageIndex < 0 ) CurrentHostageIndex = MAX_HOSTAGES-1;
		start++;
		if ( start > MAX_HOSTAGES ) break;
	} while ( !hostage_is_valid( CurrentHostageIndex ) );

	if (hostage_is_valid( CurrentHostageIndex ) )	{
		Cur_object_index = Hostages[CurrentHostageIndex].objnum;
	} else {
		CurrentHostageIndex =-1;
	}
	
	return CurrentHostageIndex;
}


static int SelectNextHostage()	{
	int start=0;

	do	{
		CurrentHostageIndex++;
		if ( CurrentHostageIndex >= MAX_HOSTAGES ) CurrentHostageIndex = 0;
		start++;
		if ( start > MAX_HOSTAGES ) break;
	} while ( !hostage_is_valid( CurrentHostageIndex ) );

	if (hostage_is_valid( CurrentHostageIndex ) )	{
		Cur_object_index = Hostages[CurrentHostageIndex].objnum;
	} else {
		CurrentHostageIndex =-1;
	}
	
	return CurrentHostageIndex;
}


static int SelectClosestHostage()	{
	int start=0;

	while ( !hostage_is_valid( CurrentHostageIndex ) )	{
		CurrentHostageIndex++;
		if ( CurrentHostageIndex >= MAX_HOSTAGES ) CurrentHostageIndex = 0;
		start++;
		if ( start > MAX_HOSTAGES ) break;
	}

	if (hostage_is_valid( CurrentHostageIndex ) )	{
		Cur_object_index = Hostages[CurrentHostageIndex].objnum;
	} else {
		CurrentHostageIndex =-1;
	}
	
	return CurrentHostageIndex;
}


static int PlaceHostage()	{
	int ctype,i;
	//update_due_to_new_segment();
	const auto cur_object_loc = compute_segment_center(Cursegp);

	ctype = -1;
	for (i=0; i<Num_total_object_types; i++ )	{
		if (ObjType[i] == OL_HOSTAGE )	{
			ctype = i;	
			break;
		}
	}

	Assert( ctype != -1 );

	if (place_object(Cursegp, cur_object_loc, ctype, 0 )==0)	{
		Int3();		// Debug below
		i=place_object(Cursegp, cur_object_loc, ctype, 0 );
		return 1;
	}

	const auto &&objp = vobjptridx(Cur_object_index);
	if (hostage_object_is_valid(objp))
	{
		CurrentHostageIndex	= get_hostage_id(objp);
	} else {
		Int3();		// Get John! (Object should be valid)
		hostage_object_is_valid(objp);	// For debugging only
	}

	return 0;
}

static int CompressHostages()
{
	hostage_compress_all();

	return 0;
}

//@@int SelectPrevVclip()	{
//@@	if (!hostage_is_valid( CurrentHostageIndex ) )	
//@@		return 0;
//@@
//@@	if ( Hostages[CurrentHostageIndex].type == 0 )
//@@		Hostages[CurrentHostageIndex].type = N_hostage_types-1;
//@@	else
//@@		Hostages[CurrentHostageIndex].type--;
//@@	
//@@	if ( Hostages[CurrentHostageIndex].type >= N_hostage_types )
//@@		Hostages[CurrentHostageIndex].type = 0;
//@@	
//@@	return 1;
//@@}
//@@
//@@int SelectNextVclip()	{
//@@	if (!hostage_is_valid( CurrentHostageIndex ) )	
//@@		return 0;
//@@
//@@	Hostages[CurrentHostageIndex].type++;
//@@	if ( Hostages[CurrentHostageIndex].type >= N_hostage_types )
//@@		Hostages[CurrentHostageIndex].type = 0;
//@@
//@@	return 1;
//@@}

//@@int find_next_hostage_sound()	{
//@@	int start=0,n;
//@@
//@@	n = Hostages[CurrentHostageIndex].sound_num;
//@@	do	{
//@@		n++;
//@@		if ( n < SOUND_HOSTAGE_VOICES ) n = SOUND_HOSTAGE_VOICES+MAX_HOSTAGE_SOUNDS-1;
//@@		if ( n >= SOUND_HOSTAGE_VOICES+MAX_HOSTAGE_SOUNDS ) n = SOUND_HOSTAGE_VOICES;
//@@		start++;
//@@		if ( start > MAX_HOSTAGE_SOUNDS ) break;
//@@	} while ( Sounds[n] == NULL );
//@@
//@@	if ( Sounds[n] == NULL )
//@@		Hostages[CurrentHostageIndex].sound_num = -1;
//@@	else	{
//@@		Hostages[CurrentHostageIndex].sound_num = n;
//@@		PlayHostageSound();
//@@	}
//@@	return 1;
//@@}
//@@
//@@int find_prev_hostage_sound()	{
//@@	int start=0,n;
//@@
//@@	n = Hostages[CurrentHostageIndex].sound_num;
//@@	do	{
//@@		n--;
//@@		if ( n < SOUND_HOSTAGE_VOICES ) n = SOUND_HOSTAGE_VOICES+MAX_HOSTAGE_SOUNDS-1;
//@@		if ( n >= SOUND_HOSTAGE_VOICES+MAX_HOSTAGE_SOUNDS ) n = SOUND_HOSTAGE_VOICES;
//@@		start++;
//@@		if ( start > MAX_HOSTAGE_SOUNDS ) break;
//@@	} while ( Sounds[n] == NULL );
//@@
//@@	if ( Sounds[n] == NULL )
//@@		Hostages[CurrentHostageIndex].sound_num = -1;
//@@	else	{
//@@		Hostages[CurrentHostageIndex].sound_num = n;
//@@		PlayHostageSound();
//@@	}
//@@	return 1;
//@@}


static window_event_result hostage_dialog_handler(UI_DIALOG *dlg,const d_event &event, hostage_dialog *h);

//-------------------------------------------------------------------------
// Called from the editor... does one instance of the hostage dialog box
//-------------------------------------------------------------------------
int do_hostage_dialog()
{
	// Only open 1 instance of this window...
	if ( MainWindow != NULL ) return 0;
	
	// Close other windows
	close_all_windows();
	
	auto h = make_unique<hostage_dialog>();
	h->vclip_animation_time = 0;
	h->vclip_playback_speed = 0;
	h->vclip_ptr = NULL;

	CurrentHostageIndex = 0;
	SelectClosestHostage();

	// Open a window with a quit button
	MainWindow = ui_create_dialog(TMAPBOX_X+10, TMAPBOX_Y+20, 765-TMAPBOX_X, 545-TMAPBOX_Y, DF_DIALOG, hostage_dialog_handler, std::move(h));
	return 1;
}

static window_event_result hostage_dialog_created(UI_DIALOG *const w, hostage_dialog *const h)
{
	h->quitButton = ui_add_gadget_button(w, 20, 222, 48, 40, "Done", NULL);
	// The little box the hostage vclip will play in.
	h->hostageViewBox = ui_add_gadget_userbox(w, 10, 90+10, 64, 64);
	// A bunch of buttons...
	int i = 90;
	h->next = ui_add_gadget_button(w, 155, i, 140, 26, "Next Hostage", SelectNextHostage);	i += 29;		
	h->prev = ui_add_gadget_button(w, 155, i, 140, 26, "Prev Hostage", SelectPrevHostage); i += 29;		
	h->compress = ui_add_gadget_button(w, 155, i, 140, 26, "Compress All", CompressHostages); i += 29;		
	h->delete_object = ui_add_gadget_button(w, 155, i, 140, 26, "Delete", ObjectDelete);	i += 29;		
	h->new_object = ui_add_gadget_button(w, 155, i, 140, 26, "Create New", PlaceHostage);	i += 29;		
	h->time = timer_query();
	LastHostageIndex = -2;		// Set to some dummy value so everything works ok on the first frame.
	return window_event_result::ignored;
}

void hostage_close_window()
{
	if ( MainWindow!=NULL )	{
		ui_close_dialog( MainWindow );
		MainWindow = NULL;
	}
}

static window_event_result hostage_dialog_handler(UI_DIALOG *dlg,const d_event &event, hostage_dialog *h)
{
	switch(event.type)
	{
		case EVENT_WINDOW_CREATED:
			return hostage_dialog_created(dlg, h);
		case EVENT_WINDOW_CLOSE:
			std::default_delete<hostage_dialog>()(h);
			MainWindow = nullptr;
			return window_event_result::ignored;
		default:
			break;
	}
	fix64 Temp;
	int keypress = 0;
	if (event.type == EVENT_KEY_COMMAND)
		keypress = event_key_get(event);

	Assert(MainWindow != NULL);

	SelectClosestHostage();

	//------------------------------------------------------------
	// Call the ui code..
	//------------------------------------------------------------
	ui_button_any_drawn = 0;

	//------------------------------------------------------------
	// If we change objects, we need to reset the ui code for all
	// of the radio buttons that control the ai mode.  Also makes
	// the current AI mode button be flagged as pressed down.
	//------------------------------------------------------------
	//------------------------------------------------------------
	// If any of the radio buttons that control the mode are set, then
	// update the cooresponding AI state.
	//------------------------------------------------------------

	//------------------------------------------------------------
	// Redraw the object in the little 64x64 box
	//------------------------------------------------------------
	if (event.type == EVENT_UI_DIALOG_DRAW)
	{
		ui_dprintf_at( MainWindow, 10, 32,"&Message:" );

		// A simple frame time counter for spinning the objects...
		Temp = timer_query();
		h->time = Temp;
		
		if (CurrentHostageIndex > -1 )	{
			gr_set_current_canvas( h->hostageViewBox->canvas );

				gr_clear_canvas( CGREY );
		} else {
			// no hostage, so just blank out
			gr_set_current_canvas( h->hostageViewBox->canvas );
			gr_clear_canvas( CGREY );
		}
	}

	//------------------------------------------------------------
	// If anything changes in the ui system, redraw all the text that
	// identifies this robot.
	//------------------------------------------------------------
	if (event.type == EVENT_UI_DIALOG_DRAW)
	{
		if ( CurrentHostageIndex > -1 )	{
			ui_dprintf_at( MainWindow, 10, 15, "Hostage: %d   Object: %d", CurrentHostageIndex, Hostages[CurrentHostageIndex].objnum );
			//@@ui_dprintf_at( MainWindow, 10, 73, "Type: %d   Sound: %d   ", Hostages[CurrentHostageIndex].type, Hostages[CurrentHostageIndex].sound_num );
		}	else {
			ui_dprintf_at( MainWindow, 10, 15, "Hostage: none " );
			//@@ui_dprintf_at( MainWindow, 10, 73, "Type:    Sound:       " );
			ui_dprintf_at( MainWindow, 10, 73, "Face:         " );
		}
	}
	
	if (ui_button_any_drawn || (LastHostageIndex != CurrentHostageIndex))
		Update_flags |= UF_WORLD_CHANGED;
	if (GADGET_PRESSED(h->quitButton.get()) || keypress==KEY_ESC)
	{
		return window_event_result::close;
	}		

	LastHostageIndex = CurrentHostageIndex;
	return window_event_result::ignored;
}
