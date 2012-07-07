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
COPYRIGHT 1993-1999 PARALLAX SOFTWARE CORPORATION.  ALL RIGHTS RESERVED.
*/


#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>
#include <string.h>
#include <ctype.h>

#include "fix.h"
#include "object.h"
#include "timer.h"
#include "joy.h"
#include "digi.h"
#include "sounds.h"
#include "args.h"
#include "key.h"
#include "newdemo.h"
#include "game.h"
#include "dxxerror.h"
#include "wall.h"
#include "piggy.h"
#include "text.h"
#include "kconfig.h"
#include "config.h"

#define SOF_USED				1 		// Set if this sample is used
#define SOF_PLAYING			2		// Set if this sample is playing on a channel
#define SOF_LINK_TO_OBJ		4		// Sound is linked to a moving object. If object dies, then finishes play and quits.
#define SOF_LINK_TO_POS		8		// Sound is linked to segment, pos
#define SOF_PLAY_FOREVER	16		// Play forever (or until level is stopped), otherwise plays once
#define SOF_PERMANENT		32		// Part of the level, like a waterfall or fan

typedef struct sound_object {
	short			signature;		// A unique signature to this sound
	ubyte			flags;			// Used to tell if this slot is used and/or currently playing, and how long.
	ubyte			pad;				//	Keep alignment
	fix			max_volume;		// Max volume that this sound is playing at
	fix			max_distance;	// The max distance that this sound can be heard at...
	int			volume;			// Volume that this sound is playing at
	int			pan;				// Pan value that this sound is playing at
	int			channel;			// What channel this is playing on, -1 if not playing
	short			soundnum;		// The sound number that is playing
	int			loop_start;		// The start point of the loop. -1 means no loop
	int			loop_end;		// The end point of the loop
	union {
		struct {
			short			segnum;				// Used if SOF_LINK_TO_POS field is used
			short			sidenum;
			vms_vector	position;
		} pos;
		struct {
			short			objnum;				// Used if SOF_LINK_TO_OBJ field is used
			short			objsignature;
		} obj;
	} link_type;
} sound_object;

#define MAX_SOUND_OBJECTS 150
sound_object SoundObjects[MAX_SOUND_OBJECTS];
short next_signature=0;

int N_active_sound_objects=0;

int digi_sounds_initialized=0;

/* Find the sound which actually equates to a sound number */
int digi_xlat_sound(int soundno)
{
	if (soundno < 0)
		return -1;

	if (GameArg.SysLowMem)
	{
		soundno = AltSounds[soundno];
		if (soundno == 255)
			return -1;
	}

	//Assert(Sounds[soundno] != 255);	//if hit this, probably using undefined sound
	if (Sounds[soundno] == 255)
		return -1;

	return Sounds[soundno];
}

int digi_unxlat_sound(int soundno)
{
	int i;
	ubyte *table = (GameArg.SysLowMem?AltSounds:Sounds);

	if ( soundno < 0 ) return -1;

	for (i=0;i<MAX_SOUNDS;i++)
		if (table[i] == soundno)
			return i;

	Int3();
	return 0;
}


void digi_get_sound_loc( vms_matrix * listener, vms_vector * listener_pos, int listener_seg, vms_vector * sound_pos, int sound_seg, fix max_volume, int *volume, int *pan, fix max_distance )
{

	vms_vector	vector_to_sound;
	fix angle_from_ear, cosang,sinang;
	fix distance;
	fix path_distance;

	*volume = 0;
	*pan = 0;

	max_distance = (max_distance*5)/4;		// Make all sounds travel 1.25 times as far.

	//	Warning: Made the vm_vec_normalized_dir be vm_vec_normalized_dir_quick and got illegal values to acos in the fang computation.
	distance = vm_vec_normalized_dir_quick( &vector_to_sound, sound_pos, listener_pos );

	if (distance < max_distance )	{
		int num_search_segs = f2i(max_distance/20);
		if ( num_search_segs < 1 ) num_search_segs = 1;

		path_distance = find_connected_distance(listener_pos, listener_seg, sound_pos, sound_seg, num_search_segs, WID_RENDPAST_FLAG+WID_FLY_FLAG );
		if ( path_distance > -1 )	{
			*volume = max_volume - fixdiv(path_distance,max_distance);
			if (*volume > 0 )	{
				angle_from_ear = vm_vec_delta_ang_norm(&listener->rvec,&vector_to_sound,&listener->uvec);
				fix_sincos(angle_from_ear,&sinang,&cosang);
				if (GameCfg.ReverseStereo) cosang *= -1;
				*pan = (cosang + F1_0)/2;
			} else {
				*volume = 0;
			}
		}
	}

}

void digi_play_sample_once( int soundno, fix max_volume )
{
	if ( Newdemo_state == ND_STATE_RECORDING )
		newdemo_record_sound( soundno );

	soundno = digi_xlat_sound(soundno);

	if (soundno < 0 ) return;

   // start the sample playing
	digi_start_sound( soundno, max_volume, 0xffff/2, 0, -1, -1, -1 );
}


void digi_play_sample( int soundno, fix max_volume )
{
	if ( Newdemo_state == ND_STATE_RECORDING )
		newdemo_record_sound( soundno );

	soundno = digi_xlat_sound(soundno);

	if (soundno < 0 ) return;

   // start the sample playing
	digi_start_sound( soundno, max_volume, 0xffff/2, 0, -1, -1, -1 );
}


void digi_play_sample_3d( int soundno, int angle, int volume, int no_dups )
{

	no_dups = 1;

	if ( Newdemo_state == ND_STATE_RECORDING )		{
		if ( no_dups )
			newdemo_record_sound_3d_once( soundno, angle, volume );
		else
			newdemo_record_sound_3d( soundno, angle, volume );
	}

	soundno = digi_xlat_sound(soundno);

	if (soundno < 0 ) return;

	if (volume < 10 ) return;

   // start the sample playing
	digi_start_sound( soundno, volume, angle, 0, -1, -1, -1 );
}


void SoundQ_init();
void SoundQ_process();
void SoundQ_pause();

void digi_init_sounds()
{
	int i;

	SoundQ_init();

	digi_stop_all_channels();

	digi_stop_looping_sound();
	for (i=0; i<MAX_SOUND_OBJECTS; i++ )	{
		SoundObjects[i].channel = -1;
		SoundObjects[i].flags = 0;	// Mark as dead, so some other sound can use this sound
	}
	N_active_sound_objects = 0;
	digi_sounds_initialized = 1;
}

extern int digi_max_channels;

// plays a sample that loops forever.
// Call digi_stop_channe(channel) to stop it.
// Call digi_set_channel_volume(channel, volume) to change volume.
// if loop_start is -1, entire sample loops
// Returns the channel that sound is playing on, or -1 if can't play.
// This could happen because of no sound drivers loaded or not enough channels.
int digi_looping_sound = -1;
int digi_looping_volume = 0;
int digi_looping_start = -1;
int digi_looping_end = -1;
int digi_looping_channel = -1;

void digi_play_sample_looping_sub()
{
	if ( digi_looping_sound > -1 )
		digi_looping_channel  = digi_start_sound( digi_looping_sound, digi_looping_volume, 0xFFFF/2, 1, digi_looping_start, digi_looping_end, -1 );
}

void digi_play_sample_looping( int soundno, fix max_volume,int loop_start, int loop_end )
{
	soundno = digi_xlat_sound(soundno);

	if (soundno < 0 ) return;

	if (digi_looping_channel>-1)
		digi_stop_sound( digi_looping_channel );

	digi_looping_sound = soundno;
	digi_looping_volume = max_volume;
	digi_looping_start = loop_start;
	digi_looping_end = loop_end;
	digi_play_sample_looping_sub();
}

void digi_change_looping_volume( fix volume )
{
	if ( digi_looping_channel > -1 )
		digi_set_channel_volume( digi_looping_channel, volume );
	digi_looping_volume = volume;
}

void digi_stop_looping_sound()
{
	if ( digi_looping_channel > -1 )
		digi_stop_sound( digi_looping_channel );
	digi_looping_channel = -1;
	digi_looping_sound = -1;
}

void digi_pause_looping_sound()
{
	if ( digi_looping_channel > -1 )
		digi_stop_sound( digi_looping_channel );
	digi_looping_channel = -1;
}

void digi_unpause_looping_sound()
{
	digi_play_sample_looping_sub();
}

//hack to not start object when loading level
int Dont_start_sound_objects = 0;

void digi_start_sound_object(int i)
{
	// start sample structures
	SoundObjects[i].channel =  -1;

	if ( SoundObjects[i].volume <= 0 )
		return;

	if ( Dont_start_sound_objects )
		return;

// -- MK, 2/22/96 -- 	if ( Newdemo_state == ND_STATE_RECORDING )
// -- MK, 2/22/96 -- 		newdemo_record_sound_3d_once( digi_unxlat_sound(SoundObjects[i].soundnum), SoundObjects[i].pan, SoundObjects[i].volume );

	// only use up to half the sound channels for "permanent" sounts
	if ((SoundObjects[i].flags & SOF_PERMANENT) && (N_active_sound_objects >= max(1, digi_max_channels / 4)))
		return;

	// start the sample playing

	SoundObjects[i].channel = digi_start_sound( SoundObjects[i].soundnum,
										SoundObjects[i].volume,
										SoundObjects[i].pan,
										SoundObjects[i].flags & SOF_PLAY_FOREVER,
										SoundObjects[i].loop_start,
										SoundObjects[i].loop_end, i );

	if (SoundObjects[i].channel > -1 )
		N_active_sound_objects++;
}

//sounds longer than this get their 3d aspects updated
#define SOUND_3D_THRESHHOLD  (GameArg.SndDigiSampleRate * 3 / 2)	//1.5 seconds

int digi_link_sound_to_object3( int org_soundnum, short objnum, int forever, fix max_volume, fix  max_distance, int loop_start, int loop_end )
{

	int i,volume,pan;
	object * objp;
	int soundnum;

	soundnum = digi_xlat_sound(org_soundnum);

	if ( max_volume < 0 ) return -1;
//	if ( max_volume > F1_0 ) max_volume = F1_0;

	if (soundnum < 0 ) return -1;
	if (GameSounds[soundnum].data==NULL) {
		Int3();
		return -1;
	}
	if ((objnum<0)||(objnum>Highest_object_index))
		return -1;

	if ( !forever ) { 		// && GameSounds[soundnum - SOUND_OFFSET].length < SOUND_3D_THRESHHOLD)	{
		// Hack to keep sounds from building up...
		digi_get_sound_loc( &Viewer->orient, &Viewer->pos, Viewer->segnum, &Objects[objnum].pos, Objects[objnum].segnum, max_volume,&volume, &pan, max_distance );
		digi_play_sample_3d( org_soundnum, pan, volume, 0 );
		return -1;
	}

	if ( Newdemo_state == ND_STATE_RECORDING )		{
		newdemo_record_link_sound_to_object3( org_soundnum, objnum, max_volume, max_distance, loop_start, loop_end );
	}

	for (i=0; i<MAX_SOUND_OBJECTS; i++ )
		if (SoundObjects[i].flags==0)
			break;

	if (i==MAX_SOUND_OBJECTS) {
		return -1;
	}

	SoundObjects[i].signature=next_signature++;
	SoundObjects[i].flags = SOF_USED | SOF_LINK_TO_OBJ;
	if ( forever )
		SoundObjects[i].flags |= SOF_PLAY_FOREVER;
	SoundObjects[i].link_type.obj.objnum = objnum;
	SoundObjects[i].link_type.obj.objsignature = Objects[objnum].signature;
	SoundObjects[i].max_volume = max_volume;
	SoundObjects[i].max_distance = max_distance;
	SoundObjects[i].volume = 0;
	SoundObjects[i].pan = 0;
	SoundObjects[i].soundnum = soundnum;
	SoundObjects[i].loop_start = loop_start;
	SoundObjects[i].loop_end = loop_end;

	if (Dont_start_sound_objects) { 		//started at level start

		SoundObjects[i].flags |= SOF_PERMANENT;
		SoundObjects[i].channel =  -1;
	}
	else {
		objp = &Objects[SoundObjects[i].link_type.obj.objnum];
		digi_get_sound_loc( &Viewer->orient, &Viewer->pos, Viewer->segnum,
                       &objp->pos, objp->segnum, SoundObjects[i].max_volume,
                       &SoundObjects[i].volume, &SoundObjects[i].pan, SoundObjects[i].max_distance );

		digi_start_sound_object(i);

		// If it's a one-shot sound effect, and it can't start right away, then
		// just cancel it and be done with it.
		if ( (SoundObjects[i].channel < 0) && (!(SoundObjects[i].flags & SOF_PLAY_FOREVER)) )    {
			SoundObjects[i].flags = 0;
			return -1;
		}
	}

	return SoundObjects[i].signature;
}

int digi_link_sound_to_object2( int org_soundnum, short objnum, int forever, fix max_volume, fix  max_distance )
{
	return digi_link_sound_to_object3( org_soundnum, objnum, forever, max_volume, max_distance, -1, -1 );
}


int digi_link_sound_to_object( int soundnum, short objnum, int forever, fix max_volume )
{
	return digi_link_sound_to_object2( soundnum, objnum, forever, max_volume, 256*F1_0  );
}

int digi_link_sound_to_pos2( int org_soundnum, short segnum, short sidenum, vms_vector * pos, int forever, fix max_volume, fix max_distance )
{

	int i, volume, pan;
	int soundnum;

	soundnum = digi_xlat_sound(org_soundnum);

	if ( max_volume < 0 ) return -1;
//	if ( max_volume > F1_0 ) max_volume = F1_0;

	if (soundnum < 0 ) return -1;
	if (GameSounds[soundnum].data==NULL) {
		Int3();
		return -1;
	}

	if ((segnum<0)||(segnum>Highest_segment_index))
		return -1;

	if ( !forever ) { 	//&& GameSounds[soundnum - SOUND_OFFSET].length < SOUND_3D_THRESHHOLD)	{
		// Hack to keep sounds from building up...
		digi_get_sound_loc( &Viewer->orient, &Viewer->pos, Viewer->segnum, pos, segnum, max_volume, &volume, &pan, max_distance );
		digi_play_sample_3d( org_soundnum, pan, volume, 0 );
		return -1;
	}

	for (i=0; i<MAX_SOUND_OBJECTS; i++ )
		if (SoundObjects[i].flags==0)
			break;

	if (i==MAX_SOUND_OBJECTS) {
		return -1;
	}


	SoundObjects[i].signature=next_signature++;
	SoundObjects[i].flags = SOF_USED | SOF_LINK_TO_POS;
	if ( forever )
		SoundObjects[i].flags |= SOF_PLAY_FOREVER;
	SoundObjects[i].link_type.pos.segnum = segnum;
	SoundObjects[i].link_type.pos.sidenum = sidenum;
	SoundObjects[i].link_type.pos.position = *pos;
	SoundObjects[i].soundnum = soundnum;
	SoundObjects[i].max_volume = max_volume;
	SoundObjects[i].max_distance = max_distance;
	SoundObjects[i].volume = 0;
	SoundObjects[i].pan = 0;
	SoundObjects[i].loop_start = SoundObjects[i].loop_end = -1;

	if (Dont_start_sound_objects) {		//started at level start

		SoundObjects[i].flags |= SOF_PERMANENT;

		SoundObjects[i].channel =  -1;
	}
	else {

		digi_get_sound_loc( &Viewer->orient, &Viewer->pos, Viewer->segnum,
                       &SoundObjects[i].link_type.pos.position, SoundObjects[i].link_type.pos.segnum, SoundObjects[i].max_volume,
                       &SoundObjects[i].volume, &SoundObjects[i].pan, SoundObjects[i].max_distance );

		digi_start_sound_object(i);

		// If it's a one-shot sound effect, and it can't start right away, then
		// just cancel it and be done with it.
		if ( (SoundObjects[i].channel < 0) && (!(SoundObjects[i].flags & SOF_PLAY_FOREVER)) )    {
			SoundObjects[i].flags = 0;
			return -1;
		}
	}

	return SoundObjects[i].signature;
}

int digi_link_sound_to_pos( int soundnum, short segnum, short sidenum, vms_vector * pos, int forever, fix max_volume )
{
	return digi_link_sound_to_pos2( soundnum, segnum, sidenum, pos, forever, max_volume, F1_0 * 256 );
}

//if soundnum==-1, kill any sound
void digi_kill_sound_linked_to_segment( int segnum, int sidenum, int soundnum )
{
	int i,killed;

	if (soundnum != -1)
		soundnum = digi_xlat_sound(soundnum);


	killed = 0;

	for (i=0; i<MAX_SOUND_OBJECTS; i++ )	{
		if ( (SoundObjects[i].flags & SOF_USED) && (SoundObjects[i].flags & SOF_LINK_TO_POS) )	{
			if ((SoundObjects[i].link_type.pos.segnum == segnum) && (SoundObjects[i].link_type.pos.sidenum==sidenum) && (soundnum==-1 || SoundObjects[i].soundnum==soundnum ))	{
				if ( SoundObjects[i].channel > -1 )	{
					digi_stop_sound( SoundObjects[i].channel );
					N_active_sound_objects--;
				}
				SoundObjects[i].channel = -1;
				SoundObjects[i].flags = 0;	// Mark as dead, so some other sound can use this sound
				killed++;
			}
		}
	}
}

void digi_kill_sound_linked_to_object( int objnum )
{

	int i,killed;

	killed = 0;

	if ( Newdemo_state == ND_STATE_RECORDING )		{
		newdemo_record_kill_sound_linked_to_object( objnum );
	}

	for (i=0; i<MAX_SOUND_OBJECTS; i++ )	{
		if ( (SoundObjects[i].flags & SOF_USED) && (SoundObjects[i].flags & SOF_LINK_TO_OBJ ) )	{
			if (SoundObjects[i].link_type.obj.objnum == objnum)	{
				if ( SoundObjects[i].channel > -1 )	{
					digi_stop_sound( SoundObjects[i].channel );
					N_active_sound_objects--;
				}
				SoundObjects[i].channel = -1;
				SoundObjects[i].flags = 0;	// Mark as dead, so some other sound can use this sound
				killed++;
			}
		}
	}
}

//	John's new function, 2/22/96.
void digi_record_sound_objects()
{
	int i;

	for (i=0; i<MAX_SOUND_OBJECTS; i++ )	{
		if ( (SoundObjects[i].flags & SOF_USED)&&
		     (SoundObjects[i].flags & SOF_LINK_TO_OBJ)&&
		     (SoundObjects[i].flags & SOF_PLAY_FOREVER)
		   )
		{

			newdemo_record_link_sound_to_object3( digi_unxlat_sound(SoundObjects[i].soundnum), SoundObjects[i].link_type.obj.objnum,
				SoundObjects[i].max_volume, SoundObjects[i].max_distance, SoundObjects[i].loop_start, SoundObjects[i].loop_end );
		}
	}
}

int was_recording = 0;

void digi_sync_sounds()
{
	int i;
	int oldvolume, oldpan;

	if ( Newdemo_state == ND_STATE_RECORDING)	{
		if ( !was_recording )	{
			digi_record_sound_objects();
		}
		was_recording = 1;
	} else {
		was_recording = 0;
	}

	SoundQ_process();

	for (i=0; i<MAX_SOUND_OBJECTS; i++ )	{
		if ( SoundObjects[i].flags & SOF_USED )	{
			oldvolume = SoundObjects[i].volume;
			oldpan = SoundObjects[i].pan;

			if ( !(SoundObjects[i].flags & SOF_PLAY_FOREVER) )	{
			 	// Check if its done.
				if (SoundObjects[i].channel > -1 ) {
					if ( !digi_is_channel_playing(SoundObjects[i].channel) )	{
						digi_end_sound( SoundObjects[i].channel );
						SoundObjects[i].flags = 0;	// Mark as dead, so some other sound can use this sound
						N_active_sound_objects--;
						continue;		// Go on to next sound...
					}
				}
			}

			if ( SoundObjects[i].flags & SOF_LINK_TO_POS )	{
				digi_get_sound_loc( &Viewer->orient, &Viewer->pos, Viewer->segnum,
                                &SoundObjects[i].link_type.pos.position, SoundObjects[i].link_type.pos.segnum, SoundObjects[i].max_volume,
                                &SoundObjects[i].volume, &SoundObjects[i].pan, SoundObjects[i].max_distance );

			} else if ( SoundObjects[i].flags & SOF_LINK_TO_OBJ )	{
				object * objp;


				if ( Newdemo_state == ND_STATE_PLAYBACK )	{
					int objnum;
					objnum = newdemo_find_object( SoundObjects[i].link_type.obj.objsignature );
					if ( objnum > -1 )	{
						objp = &Objects[objnum];
					} else {
						objp = &Objects[0];
					}
				} else {
					objp = &Objects[SoundObjects[i].link_type.obj.objnum];
				}

				if ((objp->type==OBJ_NONE) || (objp->signature!=SoundObjects[i].link_type.obj.objsignature))	{
					// The object that this is linked to is dead, so just end this sound if it is looping.
					if ( SoundObjects[i].channel>-1 )	{
						if (SoundObjects[i].flags & SOF_PLAY_FOREVER)
							digi_stop_sound( SoundObjects[i].channel );
						else
							digi_end_sound( SoundObjects[i].channel );
						N_active_sound_objects--;
					}
					SoundObjects[i].flags = 0;	// Mark as dead, so some other sound can use this sound
					continue;		// Go on to next sound...
				} else {
					digi_get_sound_loc( &Viewer->orient, &Viewer->pos, Viewer->segnum,
	                                &objp->pos, objp->segnum, SoundObjects[i].max_volume,
                                   &SoundObjects[i].volume, &SoundObjects[i].pan, SoundObjects[i].max_distance );
				}
			}

			if (oldvolume != SoundObjects[i].volume) 	{
				if ( SoundObjects[i].volume < 1 )	{
					// Sound is too far away, so stop it from playing.

					if ( SoundObjects[i].channel>-1 )	{
						if (SoundObjects[i].flags & SOF_PLAY_FOREVER)
							digi_stop_sound( SoundObjects[i].channel );
						else
							digi_end_sound( SoundObjects[i].channel );
						N_active_sound_objects--;
						SoundObjects[i].channel = -1;
					}

					if (! (SoundObjects[i].flags & SOF_PLAY_FOREVER)) {
						SoundObjects[i].flags = 0;	// Mark as dead, so some other sound can use this sound
						continue;
					}

				} else {
					if (SoundObjects[i].channel<0)	{
						digi_start_sound_object(i);
					} else {
						digi_set_channel_volume( SoundObjects[i].channel, SoundObjects[i].volume );
					}
				}
			}

			if (oldpan != SoundObjects[i].pan) 	{
				if (SoundObjects[i].channel>-1)
					digi_set_channel_pan( SoundObjects[i].channel, SoundObjects[i].pan );
			}

		}
	}

#ifndef NDEBUG
//	digi_sound_debug();
#endif
}

void digi_pause_digi_sounds()
{

	int i;

	digi_pause_looping_sound();

	for (i=0; i<MAX_SOUND_OBJECTS; i++ )	{
		if ( (SoundObjects[i].flags & SOF_USED) && (SoundObjects[i].channel>-1) )	{
			digi_stop_sound( SoundObjects[i].channel );
			if (! (SoundObjects[i].flags & SOF_PLAY_FOREVER))
				SoundObjects[i].flags = 0;	// Mark as dead, so some other sound can use this sound
			N_active_sound_objects--;
			SoundObjects[i].channel = -1;
		}
	}

	digi_stop_all_channels();
	SoundQ_pause();
}

void digi_resume_digi_sounds()
{
	digi_sync_sounds();	//don't think we really need to do this, but can't hurt
	digi_unpause_looping_sound();
}

// Called by the code in digi.c when another sound takes this sound object's
// slot because the sound was done playing.
void digi_end_soundobj(int i)
{
	Assert( SoundObjects[i].flags & SOF_USED );
	Assert( SoundObjects[i].channel > -1 );

	N_active_sound_objects--;
	SoundObjects[i].channel = -1;
}

void digi_stop_digi_sounds()
{
	int i;

	digi_stop_looping_sound();

	for (i=0; i<MAX_SOUND_OBJECTS; i++ )	{
		if ( SoundObjects[i].flags & SOF_USED )	{
			if ( SoundObjects[i].channel > -1 )	{
				digi_stop_sound( SoundObjects[i].channel );
				N_active_sound_objects--;
			}
			SoundObjects[i].flags = 0;	// Mark as dead, so some other sound can use this sound
		}
	}

	digi_stop_all_channels();
	SoundQ_init();
}

#ifndef NDEBUG
int verify_sound_channel_free( int channel )
{
	int i;
	for (i=0; i<MAX_SOUND_OBJECTS; i++ )	{
		if ( SoundObjects[i].flags & SOF_USED )	{
			if ( SoundObjects[i].channel == channel )	{
				Int3();	// Get John!
			}
		}
	}
	return 0;
}

void digi_sound_debug()
{
	int i;
	int n_active_sound_objs=0;
	int n_sound_objs=0;

	for (i=0; i<MAX_SOUND_OBJECTS; i++ )	{
		if ( SoundObjects[i].flags & SOF_USED ) 	{
			n_sound_objs++;
			if ( SoundObjects[i].channel > -1 )
				n_active_sound_objs++;
		}
	}
	digi_debug();
}
#endif

typedef struct sound_q {
	fix64 time_added;
	int soundnum;
} sound_q;

#define MAX_Q 32
#define MAX_LIFE F1_0*30		// After being queued for 30 seconds, don't play it
sound_q SoundQ[MAX_Q];
int SoundQ_head, SoundQ_tail, SoundQ_num;
int SoundQ_channel;

void SoundQ_init()
{
	SoundQ_head = SoundQ_tail = 0;
	SoundQ_num = 0;
	SoundQ_channel = -1;
}

void SoundQ_pause()
{
	SoundQ_channel = -1;
}

void SoundQ_end()
{
	// Current playing sound is stopped, so take it off the Queue
	SoundQ_head = (SoundQ_head+1);
	if ( SoundQ_head >= MAX_Q ) SoundQ_head = 0;
	SoundQ_num--;
	SoundQ_channel = -1;
}

void SoundQ_process()
{
	if ( SoundQ_channel > -1 )	{
		if ( digi_is_channel_playing(SoundQ_channel) )
			return;
		SoundQ_end();
	}

	while ( SoundQ_head != SoundQ_tail )	{
		sound_q * q = &SoundQ[SoundQ_head];

		if ( q->time_added+MAX_LIFE > timer_query() )	{
			SoundQ_channel = digi_start_sound(q->soundnum, F1_0+1, 0xFFFF/2, 0, -1, -1, -1 );
			return;
		} else {
			// expired; remove from Queue
	  		SoundQ_end();
		}
	}
}


void digi_start_sound_queued( short soundnum, fix volume )
{
	int i;

	soundnum = digi_xlat_sound(soundnum);

	if (soundnum < 0 ) return;

	i = SoundQ_tail+1;
	if ( i>=MAX_Q ) i = 0;

	// Make sure its loud so it doesn't get cancelled!
	if ( volume < F1_0+1 )
		volume = F1_0 + 1;

	if ( i != SoundQ_head )	{
		SoundQ[SoundQ_tail].time_added = timer_query();
		SoundQ[SoundQ_tail].soundnum = soundnum;
		SoundQ_num++;
		SoundQ_tail = i;
	}

	// Try to start it!
	SoundQ_process();
}

