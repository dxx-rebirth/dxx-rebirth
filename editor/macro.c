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
 * Routines for recording/playing/saving macros
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <math.h>
#include <string.h>
#include "inferno.h"
#include "segment.h"
#include "editor.h"
#include "gr.h"
#include "ui.h"
#include "key.h"
#include "fix.h"
#include "3d.h"
#include "mouse.h"
#include "bm.h"
#include "error.h"
#include "medlisp.h"
#include "kdefs.h"
#include "u_mem.h"

#define MAX_NUM_EVENTS 10000

UI_EVENT * RecordBuffer;

int MacroNumEvents = 0;
int MacroStatus = 0;

static char filename[PATH_MAX] = "*.MIN";

int MacroRecordAll()
{
	if ( MacroStatus== UI_STATUS_NORMAL )
	{
		if (RecordBuffer) d_free( RecordBuffer );
		MALLOC( RecordBuffer, UI_EVENT, MAX_NUM_EVENTS );
		ui_record_events( MAX_NUM_EVENTS, RecordBuffer, UI_RECORD_MOUSE | UI_RECORD_KEYS );
		MacroStatus = UI_STATUS_RECORDING;
	}
	return 1;
}

int MacroRecordKeys()
{
	if ( MacroStatus== UI_STATUS_NORMAL )
	{
		if (RecordBuffer) d_free( RecordBuffer );
		MALLOC( RecordBuffer, UI_EVENT, MAX_NUM_EVENTS );
		ui_record_events( MAX_NUM_EVENTS, RecordBuffer, UI_RECORD_KEYS );
		MacroStatus = UI_STATUS_RECORDING;
	}
	return 1;
}

int MacroPlayNormal()
{
	if (MacroStatus== UI_STATUS_NORMAL && MacroNumEvents > 0 && RecordBuffer )
	{
		ui_set_playback_speed( 1 );
		ui_play_events_realtime(MacroNumEvents, RecordBuffer);
		MacroStatus = UI_STATUS_PLAYING;
	}
	return 1;
}

int MacroPlayFast()
{
	if (MacroStatus== UI_STATUS_NORMAL && MacroNumEvents > 0 && RecordBuffer )
	{
		mouse_toggle_cursor(0);
		ui_play_events_fast(MacroNumEvents, RecordBuffer);
		MacroStatus = UI_STATUS_FASTPLAY;
	}
	return 1;
}

int MacroSave()
{

	if (MacroNumEvents < 1 )
	{
		ui_messagebox( -2, -2, 1, "No macro has been defined to save!", "Oops" );
		return 1;
	}

	if (ui_get_filename( filename, "*.MAC", "SAVE MACRO" ))
	{
		PHYSFS_file *fp = PHYSFS_openWrite(filename);

		if (!fp) return 0;
		RecordBuffer[0].type = 7;
		RecordBuffer[0].frame = 0;
		RecordBuffer[0].data = MacroNumEvents;
		PHYSFS_write(fp, RecordBuffer, sizeof(UI_EVENT), MacroNumEvents);
		PHYSFS_close(fp);
	}
	return 1;
}

int MacroLoad()
{
	if (ui_get_filename( filename, "*.MAC", "LOAD MACRO" ))
	{
		PHYSFS_file *fp = PHYSFS_openRead(filename);
		int length;

		if (!fp)
			return 0;
		if (RecordBuffer)
			d_free( RecordBuffer );
		length = PHYSFS_fileLength(fp);
		RecordBuffer = d_malloc(length);
		if (!RecordBuffer)
			return 0;

		PHYSFS_read(fp, RecordBuffer, length, 1);
		PHYSFS_close(fp);
		MacroNumEvents = RecordBuffer[0].data;
	}
	return 1;
}

void macro_free_buffer()
{
	if ( RecordBuffer ) 
		d_free(RecordBuffer);
}

int MacroMenu()
{
	int x;
	char * MenuItems[] = { "Play fast",
					   "Play normal",
					   "Record all",
					   "Record keys",
					   "Save macro",
					   "Load macro" };

	x = MenuX( -1, -1, 6, MenuItems );

	switch( x )
	{
	case 1:
		MacroPlayFast();
		break;
	case 2:
		MacroPlayNormal();
		break;
	case 3:
		MacroRecordAll();
		break;
	case 4:
		MacroRecordKeys();
		break;
	case 5:     // Save
		MacroSave();
		break;
	case 6:     // Load
		MacroLoad();
		break;
	}
	return 1;
}

