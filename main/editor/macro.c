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
 * $Source: /cvs/cvsroot/d2x/main/editor/macro.c,v $
 * $Revision: 1.1 $
 * $Author: btb $
 * $Date: 2004-12-19 13:54:27 $
 * 
 * Routines for recording/playing/saving macros
 * 
 * $Log: not supported by cvs2svn $
 * Revision 1.1.1.1  1999/06/14 22:03:35  donut
 * Import of d1x 1.37 source.
 *
 * Revision 2.0  1995/02/27  11:35:09  john
 * Version 2.0! No anonymous unions, Watcom 10.0, with no need
 * for bitmaps.tbl.
 * 
 * Revision 1.12  1993/11/15  14:46:37  john
 * Changed Menu to MenuX
 * 
 * Revision 1.11  1993/11/05  17:32:44  john
 * added funcs
 * .,
 * 
 * Revision 1.10  1993/10/28  16:23:20  john
 * *** empty log message ***
 * 
 * Revision 1.9  1993/10/28  13:03:12  john
 * ..
 * 
 * Revision 1.8  1993/10/25  16:02:35  john
 * *** empty log message ***
 * 
 * Revision 1.7  1993/10/22  13:35:29  john
 * *** empty log message ***
 * 
 * Revision 1.6  1993/10/21  17:10:09  john
 * Fixed bug w/ load macro.
 * 
 * Revision 1.5  1993/10/19  12:58:47  john
 * *** empty log message ***
 * 
 * Revision 1.4  1993/10/19  12:55:02  john
 * *** empty log message ***
 * 
 * Revision 1.3  1993/10/19  12:49:49  john
 * made EventBuffer dynamic, use ReadFile, WriteFile
 * 
 * Revision 1.2  1993/10/15  17:42:20  john
 * *** empty log message ***
 * 
 * Revision 1.1  1993/10/15  17:28:06  john
 * Initial revision
 * 
 * 
 */


#pragma off (unreferenced)
static char rcsid[] = "$Id: macro.c,v 1.1 2004-12-19 13:54:27 btb Exp $";
#pragma on (unreferenced)

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
#include "mono.h"
#include "3d.h"
#include "mouse.h"
#include "bm.h"
#include "error.h"
#include "medlisp.h"
#include "cflib.h"

#include "kdefs.h"

#include "u_mem.h"

#define MAX_NUM_EVENTS 10000

UI_EVENT * RecordBuffer;

int MacroNumEvents = 0;
int MacroStatus = 0;

static char filename[128] = "*.MIN";

int MacroRecordAll()
{
	if ( MacroStatus== UI_STATUS_NORMAL )
	{
		if (RecordBuffer) free( RecordBuffer );
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
		if (RecordBuffer) free( RecordBuffer );
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
		ui_mouse_hide();
		ui_play_events_fast(MacroNumEvents, RecordBuffer);
		MacroStatus = UI_STATUS_FASTPLAY;
	}
	return 1;
}

int MacroSave()
{

	if (MacroNumEvents < 1 )
	{
		MessageBox( -2, -2, 1, "No macro has been defined to save!", "Oops" );
		return 1;
	}

	if (ui_get_filename( filename, "*.MAC", "SAVE MACRO" ))   {
		RecordBuffer[0].type = 7;
		RecordBuffer[0].frame = 0;
		RecordBuffer[0].data = MacroNumEvents;
		WriteFile(  filename, RecordBuffer, sizeof(UI_EVENT)*MacroNumEvents );
	}
	return 1;
}

int MacroLoad()
{
	int length;

	if (ui_get_filename( filename, "*.MAC", "LOAD MACRO" ))   {
		if (RecordBuffer) free( RecordBuffer );
		RecordBuffer = (UI_EVENT *)ReadFile( filename, &length );
		MacroNumEvents = RecordBuffer[0].data;
	}
	return 1;
}

void macro_free_buffer()
{
	if ( RecordBuffer ) 
		free(RecordBuffer);
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

