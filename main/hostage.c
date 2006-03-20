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
 * $Source: /cvsroot/dxx-rebirth/d1x-rebirth/main/hostage.c,v $
 * $Revision: 1.1.1.1 $
 * $Author: zicodxx $
 * $Date: 2006/03/17 19:44:57 $
 * 
 * Code to render and manipulate hostages
 * 
 * $Log: hostage.c,v $
 * Revision 1.1.1.1  2006/03/17 19:44:57  zicodxx
 * initial import
 *
 * Revision 1.1.1.1  1999/06/14 22:07:48  donut
 * Import of d1x 1.37 source.
 *
 * Revision 2.0  1995/02/27  11:28:36  john
 * New version 2.0, which has no anonymous unions, builds with
 * Watcom 10.0, and doesn't require parsing BITMAPS.TBL.
 * 
 * Revision 1.65  1995/02/22  13:45:54  allender
 * remove anonymous unions from object structure
 * 
 * Revision 1.64  1995/02/13  20:34:57  john
 * Lintized
 * 
 * Revision 1.63  1995/01/15  19:41:48  matt
 * Ripped out hostage faces for registered version
 * 
 * Revision 1.62  1995/01/14  19:16:53  john
 * First version of new bitmap paging code.
 * 
 * Revision 1.61  1994/12/19  16:35:09  john
 * Made hoastage playback end when ship dies.
 * 
 * Revision 1.60  1994/12/06  16:30:41  yuan
 * Localization
 * 
 * Revision 1.59  1994/11/30  17:32:46  matt
 * Put hostage_face_clip array back in so editor would work
 * 
 * Revision 1.58  1994/11/30  17:22:13  matt
 * Ripped out hostage faces in shareware version
 * 
 * Revision 1.57  1994/11/30  16:11:25  matt
 * Use correct constant for hostage voice
 * 
 * Revision 1.56  1994/11/27  23:15:19  matt
 * Made changes for new mprintf calling convention
 * 
 * Revision 1.55  1994/11/19  19:53:44  matt
 * Added code to full support different hostage head clip & message for
 * each hostage.
 * 
 * Revision 1.54  1994/11/19  16:35:15  matt
 * Got rid of unused code, & made an array smaller
 * 
 * Revision 1.53  1994/11/14  12:42:03  matt
 * Increased palette flash when hostage rescued
 * 
 * Revision 1.52  1994/10/28  14:43:09  john
 * Added sound volumes to all sound calls.
 * 
 * Revision 1.51  1994/10/23  02:10:57  matt
 * Got rid of obsolete hostage_info stuff
 * 
 * Revision 1.50  1994/10/22  00:08:44  matt
 * Fixed up problems with bonus & game sequencing
 * Player doesn't get credit for hostages unless he gets them out alive
 * 
 * Revision 1.49  1994/10/20  22:52:49  matt
 * Fixed compiler warnings
 * 
 * Revision 1.48  1994/10/20  21:25:44  matt
 * Took out silly scale down/scale up code for hostage anim
 * 
 * Revision 1.47  1994/10/20  12:47:28  matt
 * Replace old save files (MIN/SAV/HOT) with new LVL files
 * 
 * Revision 1.46  1994/10/04  15:33:33  john
 * Took out the old PLAY_SOUND??? code and replaced it
 * with direct calls into digi_link_??? so that all sounds
 * can be made 3d.
 * 
 * Revision 1.45  1994/09/28  23:10:46  matt
 * Made hostage rescue do palette flash
 * 
 * Revision 1.44  1994/09/20  00:11:00  matt
 * Finished gauges for Status Bar, including hostage video display.
 * 
 * Revision 1.43  1994/09/15  21:24:19  matt
 * Changed system to keep track of whether & what cockpit is up
 * Made hostage clip not queue when no cockpit
 * 
 * 
 * Revision 1.42  1994/08/25  13:45:19  matt
 * Made hostage vclips queue
 * 
 * Revision 1.41  1994/08/14  23:15:06  matt
 * Added animating bitmap hostages, and cleaned up vclips a bit
 * 
 * Revision 1.40  1994/08/12  22:41:11  john
 * Took away Player_stats; add Players array.
 * 
 * Revision 1.39  1994/07/14  22:06:35  john
 * Fix radar/hostage vclip conflict.
 * 
 * Revision 1.38  1994/07/12  18:40:21  yuan
 * Tweaked location of radar and hostage screen... 
 * Still needs work.
 * 
 * 
 * Revision 1.37  1994/07/07  09:52:17  john
 * Moved hostage screen.
 * 
 * Revision 1.36  1994/07/06  15:23:52  john
 * Revamped hostage sound.
 * 
 * Revision 1.35  1994/07/06  15:14:54  john
 * Added hostage sound effect picking.
 * 
 * Revision 1.34  1994/07/06  13:25:33  john
 * Added compress hostages functions.
 * 
 * Revision 1.33  1994/07/06  12:52:59  john
 * Fixed compiler warnings.
 * 
 * Revision 1.32  1994/07/06  12:43:50  john
 * Made generic messages for hostages.
 * 
 * Revision 1.31  1994/07/06  10:55:07  john
 * New structures for hostages.
 * 
 * Revision 1.30  1994/07/05  12:49:09  john
 * Put functionality of New Hostage spec into code.
 * 
 * Revision 1.29  1994/07/02  13:08:47  matt
 * Increment stats when hostage rescued
 * 
 * Revision 1.28  1994/07/01  18:07:46  john
 * y
 * 
 * Revision 1.27  1994/07/01  18:07:03  john
 * *** empty log message ***
 * 
 * Revision 1.26  1994/07/01  17:55:26  john
 * First version of not-working hostage system.
 * 
 * Revision 1.25  1994/06/27  15:53:21  john
 * #define'd out the newdemo stuff
 * 
 * 
 * Revision 1.24  1994/06/20  16:08:52  john
 * Added volume control; made doors 3d sounds.
 * 
 * Revision 1.23  1994/06/16  10:15:32  yuan
 * Fixed location of face.
 * 
 * Revision 1.22  1994/06/15  15:05:33  john
 * *** empty log message ***
 * 
 * Revision 1.21  1994/06/14  21:15:20  matt
 * Made rod objects draw lighted or not depending on a parameter, so the
 * materialization effect no longer darkens.
 * 
 * Revision 1.20  1994/06/08  18:16:26  john
 * Bunch of new stuff that basically takes constants out of the code
 * and puts them into bitmaps.tbl.
 * 
 * Revision 1.19  1994/06/02  19:30:08  matt
 * Moved texture-mapped rod drawing stuff (used for hostage & now for the
 * materialization center) to object.c
 * 
 * 
 */

#ifdef RCS
#pragma off (unreferenced)
static char rcsid[] = "$Id: hostage.c,v 1.1.1.1 2006/03/17 19:44:57 zicodxx Exp $";
#pragma on (unreferenced)
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "error.h"
#include "3d.h"
#include "mono.h"

#include "inferno.h"
#include "object.h"
#include "game.h"
#include "player.h"

#include "fireball.h"
#include "gauges.h"
#include "hostage.h"

#include "lighting.h"

#include "sounds.h"
#include "vclip.h"
#include "newdemo.h"

#include "text.h"
#include "piggy.h"

//------------- Globaly used hostage variables --------------------------------------------------
int 					N_hostage_types = 0;			  				// Number of hostage types
int 					Hostage_vclip_num[MAX_HOSTAGE_TYPES];	//vclip num for each tpye of hostage
hostage_data 		Hostages[MAX_HOSTAGES];						// Data for each hostage in mine

//------------- Internally used hostage variables --------------------------------------------------
static fix 			Hostage_animation_time=-1;			// How long the rescue sequence has been playing - units are frames, not seconds - -1 means not playing

vclip Hostage_face_clip[MAX_HOSTAGES];

#ifdef HOSTAGE_FACES

static fix 			HostagePlaybackSpeed=0;				// Calculated internally.  Frames/second of vclip.
static int 		Hostage_monitor_x = 204; 			// X location of monitor where hostage face appears
static int 		Hostage_monitor_y = 152;			// Y
static int 		Hostage_monitor_w = 55;				// Width of monitor
static int 		Hostage_monitor_h = 41;				// Height of monitor

char 					Hostage_global_message[HOSTAGE_MAX_GLOBALS][HOSTAGE_MESSAGE_LEN];
int 					Hostage_num_globals=0;

#define MAX_HOSTAGE_CLIPS 5

static int 			Hostage_queue[MAX_HOSTAGE_CLIPS];
static vclip 		*Hostage_vclip=NULL;		// Used for the vclip on monitor
static int			N_hostage_clips=0;

#define RESCUED_VCLIP_NUM	8
#define RESCUED_SOUND_NUM	91

//starts next clip in queue
void start_hostage_clip()
{
	int i,vclip_num,hostage_number;

	Assert(Hostage_animation_time==-1);		//none should be playing

	get_hostage_window_coords(&Hostage_monitor_x,&Hostage_monitor_y,&Hostage_monitor_w,&Hostage_monitor_h);

	hostage_number = Hostage_queue[0];			//get first in queue

	//drop the queue
	N_hostage_clips--;
	for (i=0;i<N_hostage_clips;i++)
		Hostage_queue[i] = Hostage_queue[i+1];

	//get vclip num - either special one, or randomly-selected generic one
	if ( Hostages[hostage_number].vclip_num > -1 )
		vclip_num = Hostages[hostage_number].vclip_num;
	else
		vclip_num = RESCUED_VCLIP_NUM;

	Hostage_vclip = &Hostage_face_clip[vclip_num];

	// Set the time to be zero to start hostage vclip display sequence
	Hostage_animation_time = 0;

	// Calculate the frame/second of the playback
	HostagePlaybackSpeed = fixdiv(F1_0,Hostage_vclip->frame_time);

	// Start the sound for this hostage
	if ( Hostage_vclip->sound_num > -1 )
		digi_play_sample( Hostage_vclip->sound_num, F1_0 );

}

//add this hostage's clip to the queue
void queue_hostage_clip(int hostage_num)
{
	if ((Cockpit_mode!=CM_FULL_COCKPIT && Cockpit_mode!=CM_STATUS_BAR) || N_hostage_clips>=MAX_HOSTAGE_CLIPS)
		return;		//no cockpit, or queue is full

	Hostage_queue[N_hostage_clips] = hostage_num;

	N_hostage_clips++;

	if (Hostage_animation_time<=0)		//none playing?
		start_hostage_clip();				//..start this one!

}

//current clip is done, stop it
void stop_hostage_clip()
{
	get_hostage_window_coords(&Hostage_monitor_x,&Hostage_monitor_y,&Hostage_monitor_w,&Hostage_monitor_h);

	nosound();								// Turn off sound		
	Hostage_animation_time = -1;		// Consider this vclip done

	if (N_hostage_clips)			//more in queue?
		start_hostage_clip();

	return;
}

void stop_all_hostage_clips()
{
	N_hostage_clips = 0;
	Hostage_animation_time=-1;
}

int hostage_is_vclip_playing()
{
	if (Hostage_animation_time>=0)
		return 1;
	else
		return 0;
}

#endif

//---------------- Initializes the hostage system ----------------------------------------------------
void hostage_init()
{
	Hostage_animation_time=-1;
}

//-------------- Renders a hostage ----------------------------------------------------------------
void draw_hostage(object *obj)
{
	Assert( obj->id < MAX_HOSTAGES );

	draw_object_tmap_rod(obj,Vclip[obj->rtype.vclip_info.vclip_num].frames[obj->rtype.vclip_info.framenum],1);

}

//------------- Called once when a hostage is rescued ------------------------------------------
void hostage_rescue( int hostage_number )
{
	//mprintf( (0, "Rescued hostage %d", hostage_number ));

	if ( (hostage_number<0) || (hostage_number>=MAX_HOSTAGES) )	{
			Int3();		// Get John!
			return;
	}

	PALETTE_FLASH_ADD(0,0,25);		//small blue flash

	Players[Player_num].hostages_on_board++;

	// Do an audio effect
	if ( Newdemo_state != ND_STATE_PLAYBACK )
		digi_play_sample( SOUND_HOSTAGE_RESCUED, F1_0 );

 #ifndef HOSTAGE_FACES

	hud_message(MSGC_GAME_ACTION, TXT_HOSTAGE_RESCUED);

 #else

	// Show the text message
	if ( strlen(Hostages[hostage_number].text) ) 
		gauge_message("%s", Hostages[hostage_number].text );
	else	{
		if ( Hostage_num_globals > 0 )	{
			int mn;
                        mn = (d_rand()*Hostage_num_globals)/D_RAND_MAX;
			if ( mn>=0 && mn < Hostage_num_globals )
				gauge_message("%s", &Hostage_global_message[mn][0] );
		}
	}

	if ( Newdemo_state == ND_STATE_RECORDING )
		newdemo_record_hostage_rescued( hostage_number );

	queue_hostage_clip(hostage_number);
 #endif
}

#ifdef HOSTAGE_FACES

//------------- Called once per frame to do the hostage effects --------------------------------
//returns true if something drew
int do_hostage_effects()
{
	int bitmapnum;

	// Don't do the effect if the time is <= 0
	if ( Hostage_animation_time < 0 )
		return 0;

	// Find next bitmap in the vclip
	bitmapnum = f2i(Hostage_animation_time);
	//mprintf( (0, " Time: %x  Bitmap: %d\n", Hostage_animation_time, bitmapnum  ));

	// Check if vclip is done playing.
	if (bitmapnum >= Hostage_vclip->num_frames)		{
		stop_hostage_clip();

		if (Hostage_animation_time >= 0)		//new clip
			bitmapnum = f2i(Hostage_animation_time);
		else
			return 0;		//no new one
	}

	get_hostage_window_coords(&Hostage_monitor_x,&Hostage_monitor_y,&Hostage_monitor_w,&Hostage_monitor_h);

	PIGGY_PAGE_IN(Hostage_vclip->frames[bitmapnum]);	
	gr_bitmap(Hostage_monitor_x,Hostage_monitor_y,Hostage_vclip->frames[bitmapnum]);

	// Increment the hostage rescue time scaled to playback speed.
	// This means that the integer part of the fix is the frame number
	// of the animation.

	Hostage_animation_time += fixmul(FrameTime,HostagePlaybackSpeed);

	return 1;
}

#endif

#define LINEBUF_SIZE 100

//------------------- Useful macros and variables ---------------
#define REMOVE_EOL(s)		hostage_remove_char((s),'\n')
#define REMOVE_COMMENTS(s)	hostage_remove_char((s),';')

void hostage_remove_char( char * s, char c )
{
	char *p;
	p = strchr(s,c);
	if (p) *p = '\0';
}

int hostage_is_valid( int hostage_num )	{
	if ( hostage_num < 0 ) return 0;
	if ( hostage_num >= MAX_HOSTAGES ) return 0;
	if ( Hostages[hostage_num].objnum < 0 ) return 0;
	if ( Hostages[hostage_num].objnum > Highest_object_index ) return 0;
	if ( Objects[Hostages[hostage_num].objnum].type != OBJ_HOSTAGE ) return 0;
	if ( Objects[Hostages[hostage_num].objnum].signature != Hostages[hostage_num].objsig ) return 0;
	if ( Objects[Hostages[hostage_num].objnum].id != hostage_num) return 0;
	return 1;
}

int hostage_object_is_valid( int objnum )	{
	if ( objnum < 0 ) return 0;
	if ( objnum > Highest_object_index ) return 0;
	if ( Objects[objnum].type != OBJ_HOSTAGE ) return 0;
	return hostage_is_valid(Objects[objnum].id);
}


int hostage_get_next_slot()	{
	int i;
	for (i=0; i<MAX_HOSTAGES; i++ )	{
		if (!hostage_is_valid(i))
			return i;
	}
	return MAX_HOSTAGES;
}

void hostage_init_info( int objnum )	{
	int i;

	i = hostage_get_next_slot();
	Assert( i > -1 );
	Hostages[i].objnum = objnum;
	Hostages[i].objsig = Objects[objnum].signature;
	//Hostages[i].type = 0;
	Hostages[i].vclip_num = -1;
	//Hostages[i].sound_num = -1;
	strcpy( Hostages[i].text, "\0" );
	Objects[objnum].id = i;
}

void hostage_init_all()
{
	int i;

	// Initialize all their values...
	for (i=0; i<MAX_HOSTAGES; i++ )	{
		Hostages[i].objnum = -1;
		Hostages[i].objsig = -1;
		//Hostages[i].type = 0;
		Hostages[i].vclip_num = -1;
		//Hostages[i].sound_num = -1;
		strcpy( Hostages[i].text, "\0" );
	}

	//@@hostage_read_global_messages();
}


#ifdef EDITOR

void hostage_compress_all()	{
	int i,newslot;
	
	for (i=0; i<MAX_HOSTAGES; i++ )	{
		if ( hostage_is_valid(i) )	{
			newslot = hostage_get_next_slot();
			if ( newslot < i )	{
				mprintf( (0, "Moved hostage %d to %d\n", i, newslot ));
				Hostages[newslot] = Hostages[i];
				Objects[Hostages[newslot].objnum].id = newslot;
				Hostages[i].objnum = -1;
				i = 0;		// start over
			}
		}
	}

	mprintf( (0, "Valid hostages:\n" ));
	for (i=0; i<MAX_HOSTAGES; i++ )	{
		if ( hostage_is_valid(i) )	
			mprintf( (0, "%d\n", i ));
	}
}

#endif

//@@#include "nocfile.h"
//@@
//@@void hostage_read_global_messages()	{
//@@	char *text_string;
//@@	FILE * fp;
//@@	char	inputline[LINEBUF_SIZE];
//@@
//@@	Hostage_num_globals=0;
//@@
//@@	fp = fopen( "GENERIC.HOT", "rt" );
//@@	if ( fp == NULL ) return;
//@@
//@@	while (fgets(inputline, LINEBUF_SIZE, fp )) {
//@@		
//@@		REMOVE_EOL(inputline);
//@@		REMOVE_COMMENTS(inputline);
//@@
//@@		text_string = strchr( inputline, '.' );
//@@
//@@		if ( text_string )	{
//@@			*text_string = 0;	// turn period into end of string
//@@
//@@			text_string++;	// skip over period
//@@			// Remove white space from beginning of string.
//@@			while( (*text_string == ' ') || (*text_string == '\t') )
//@@				text_string++;
//@@			
//@@			mprintf( (0, "Generic hostage message %d, text is '%s'\n", Hostage_num_globals, text_string ));
//@@			strncpy( &Hostage_global_message[Hostage_num_globals][0], text_string, HOSTAGE_MESSAGE_LEN );
//@@			Hostage_num_globals++;
//@@
//@@			Assert( Hostage_num_globals <= HOSTAGE_MAX_GLOBALS );
//@@		}
//@@	}
//@@	fclose(fp);
//@@}
























