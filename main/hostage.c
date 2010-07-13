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
 * Code to render and manipulate hostages
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "error.h"
#include "3d.h"
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
#include "playsave.h"

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
	if ((PlayerCfg.CockpitMode[1]!=CM_FULL_COCKPIT && PlayerCfg.CockpitMode[1]!=CM_STATUS_BAR) || N_hostage_clips>=MAX_HOSTAGE_CLIPS)
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

	HUD_init_message(HM_DEFAULT, TXT_HOSTAGE_RESCUED);

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
				Hostages[newslot] = Hostages[i];
				Objects[Hostages[newslot].objnum].id = newslot;
				Hostages[i].objnum = -1;
				i = 0;		// start over
			}
		}
	}

	for (i=0; i<MAX_HOSTAGES; i++ )	{
		if ( hostage_is_valid(i) )	
			;
	}
}

#endif
