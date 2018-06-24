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
COPYRIGHT 1993-1999 PARALLAX SOFTWARE CORPORATION.  ALL RIGHTS RESERVED.
*/


#include <algorithm>
#include <stdexcept>
#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>
#include <string.h>
#include <ctype.h>

#include "maths.h"
#include "object.h"
#include "timer.h"
#include "joy.h"
#include "digi.h"
#include "digi_audio.h"
#include "sounds.h"
#include "args.h"
#include "key.h"
#include "newdemo.h"
#include "game.h"
#include "gameseg.h"
#include "dxxerror.h"
#include "wall.h"
#include "piggy.h"
#include "text.h"
#include "kconfig.h"
#include "config.h"

#include "compiler-begin.h"
#include "compiler-exchange.h"
#include "compiler-range_for.h"

using std::max;

namespace dcx {

constexpr std::integral_constant<unsigned, 1> SOF_USED{};		// Set if this sample is used
constexpr std::integral_constant<unsigned, 4> SOF_LINK_TO_OBJ{};		// Sound is linked to a moving object. If object dies, then finishes play and quits.
constexpr std::integral_constant<unsigned, 8> SOF_LINK_TO_POS{};		// Sound is linked to segment, pos
constexpr std::integral_constant<unsigned, 16> SOF_PLAY_FOREVER{};		// Play forever (or until level is stopped), otherwise plays once
constexpr std::integral_constant<unsigned, 32> SOF_PERMANENT{};		// Part of the level, like a waterfall or fan

constexpr std::integral_constant<int, -1> RAIIdigi_sound::invalid_channel;
constexpr std::integral_constant<unsigned, 150> MAX_SOUND_OBJECTS{};

struct sound_object
{
	short			signature;		// A unique signature to this sound
	ubyte			flags;			// Used to tell if this slot is used and/or currently playing, and how long.
	ubyte			pad;				//	Keep alignment
	fix			max_volume;		// Max volume that this sound is playing at
	vm_distance max_distance;	// The max distance that this sound can be heard at...
	int			volume;			// Volume that this sound is playing at
	int			pan;				// Pan value that this sound is playing at
	int			channel;			// What channel this is playing on, -1 if not playing
	short			soundnum;		// The sound number that is playing
	int			loop_start;		// The start point of the loop. -1 means no loop
	int			loop_end;		// The end point of the loop
	union {
		struct {
			segnum_t			segnum;				// Used if SOF_LINK_TO_POS field is used
			uint8_t sidenum;
			vms_vector	position;
		} pos;
		struct {
			objnum_t			objnum;				// Used if SOF_LINK_TO_OBJ field is used
			object_signature_t			objsignature;
		} obj;
	} link_type;
};

using sound_objects_t = array<sound_object, MAX_SOUND_OBJECTS>;
static sound_objects_t SoundObjects;
static short next_signature=0;

static int N_active_sound_objects;

static void digi_kill_sound(sound_object &s)
{
	s.flags = 0;	// Mark as dead, so some other sound can use this sound
	if (s.channel > -1)	{
		N_active_sound_objects--;
		digi_stop_sound(exchange(s.channel, -1));
	}
}

static std::pair<sound_objects_t::iterator, sound_objects_t::iterator> find_sound_object_flags0(sound_objects_t &SoundObjects)
{
	const auto eso = SoundObjects.end();
	const auto i = std::find_if(SoundObjects.begin(), eso, [](sound_object &so) { return so.flags == 0; });
	return {i, eso};
}

static std::pair<sound_objects_t::iterator, sound_objects_t::iterator> find_sound_object(sound_objects_t &SoundObjects, const unsigned soundnum, const vcobjidx_t obj, const sound_stack once)
{
	if (once == sound_stack::allow_stacking)
		return find_sound_object_flags0(SoundObjects);
	const auto eso = SoundObjects.end();
	sound_objects_t::iterator free_sound_object = eso;
	for (auto i = SoundObjects.begin(); i != eso; ++i)
	{
		constexpr uint8_t used_obj = SOF_USED | SOF_LINK_TO_OBJ;
		auto &so = *i;
		const auto flags = so.flags;
		if (flags == 0)
			free_sound_object = i;
		else if (so.soundnum == soundnum && (flags & used_obj) == used_obj)
		{
			/* No check for a signature match here.  If the signature
			 * matches, this sound will be reclaimed to prevent
			 * stacking.  If the signature does not match, then the old
			 * sound is tied to a dead object and needs to be removed.
			 */
			if (so.link_type.obj.objnum == obj)
			{
				digi_kill_sound(so);
				return {i, eso};
			}
		}
	}
	return {free_sound_object, eso};
}

}

namespace dsx {

/* Find the sound which actually equates to a sound number */
int digi_xlat_sound(int soundno)
{
	if (soundno < 0)
		return -1;

	if (CGameArg.SysLowMem)
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

static int digi_unxlat_sound(int soundno)
{
	if ( soundno < 0 ) return -1;

	auto &table = (CGameArg.SysLowMem ? AltSounds : Sounds);
	auto b = begin(table);
	auto e = end(table);
	auto i = std::find(b, e, soundno);
	if (i != e)
		return std::distance(b, i);
	throw std::invalid_argument("sound not loaded");
}

static void digi_get_sound_loc(const vms_matrix &listener, const vms_vector &listener_pos, const vcsegptridx_t listener_seg, const vms_vector &sound_pos, const vcsegptridx_t sound_seg, fix max_volume, int *volume, int *pan, vm_distance max_distance)
{

	vms_vector	vector_to_sound;
	fix angle_from_ear;
	*volume = 0;
	*pan = 0;

	max_distance = (max_distance*5)/4;		// Make all sounds travel 1.25 times as far.

	//	Warning: Made the vm_vec_normalized_dir be vm_vec_normalized_dir_quick and got illegal values to acos in the fang computation.
	auto distance = vm_vec_normalized_dir_quick( vector_to_sound, sound_pos, listener_pos );

	if (distance < max_distance )	{
		int num_search_segs = f2i(max_distance/20);
		if ( num_search_segs < 1 ) num_search_segs = 1;

		auto path_distance = find_connected_distance(listener_pos, listener_seg, sound_pos, sound_seg, num_search_segs, WID_RENDPAST_FLAG|WID_FLY_FLAG);
		if ( path_distance > -1 )	{
			*volume = max_volume - fixdiv(path_distance,max_distance);
			if (*volume > 0 )	{
				angle_from_ear = vm_vec_delta_ang_norm(listener.rvec,vector_to_sound,listener.uvec);
				auto cosang = fix_cos(angle_from_ear);
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
	digi_start_sound( soundno, max_volume, 0xffff/2, 0, -1, -1, sound_object_none);
}

void digi_play_sample( int soundno, fix max_volume )
{
	if ( Newdemo_state == ND_STATE_RECORDING )
		newdemo_record_sound( soundno );

	soundno = digi_xlat_sound(soundno);

	if (soundno < 0 ) return;

   // start the sample playing
	digi_start_sound( soundno, max_volume, 0xffff/2, 0, -1, -1, sound_object_none);
}

void digi_play_sample_3d(int soundno, int angle, int volume)
{
	if ( Newdemo_state == ND_STATE_RECORDING )		{
			newdemo_record_sound_3d_once( soundno, angle, volume );
	}

	soundno = digi_xlat_sound(soundno);

	if (soundno < 0 ) return;

	if (volume < 10 ) return;

   // start the sample playing
	digi_start_sound( soundno, volume, angle, 0, -1, -1, sound_object_none);
}

static void SoundQ_init();
static void SoundQ_process();
static void SoundQ_pause();

void digi_init_sounds()
{
	SoundQ_init();

	digi_stop_all_channels();

	digi_stop_looping_sound();
	range_for (auto &i, SoundObjects)
	{
		i.channel = -1;
		i.flags = 0;	// Mark as dead, so some other sound can use this sound
	}
	N_active_sound_objects = 0;
}

// plays a sample that loops forever.
// Call digi_stop_channe(channel) to stop it.
// Call digi_set_channel_volume(channel, volume) to change volume.
// if loop_start is -1, entire sample loops
// Returns the channel that sound is playing on, or -1 if can't play.
// This could happen because of no sound drivers loaded or not enough channels.
static int digi_looping_sound = -1;
static int digi_looping_volume;
static int digi_looping_start = -1;
static int digi_looping_end = -1;
static int digi_looping_channel = -1;

static void digi_play_sample_looping_sub()
{
	if ( digi_looping_sound > -1 )
		digi_looping_channel  = digi_start_sound( digi_looping_sound, digi_looping_volume, 0xFFFF/2, 1, digi_looping_start, digi_looping_end, sound_object_none);
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
	digi_looping_volume = volume;
	if ( digi_looping_channel > -1 )
		digi_set_channel_volume( digi_looping_channel, volume );
}

static void digi_pause_looping_sound()
{
	const auto c = digi_looping_channel;
	if (c > -1)
	{
		digi_looping_channel = -1;
		digi_stop_sound(c);
	}
}

void digi_stop_looping_sound()
{
	digi_looping_sound = -1;
	digi_pause_looping_sound();
}

static void digi_unpause_looping_sound()
{
	digi_play_sample_looping_sub();
}

#if defined(DXX_BUILD_DESCENT_II)
//hack to not start object when loading level
int Dont_start_sound_objects = 0;
#endif

static void digi_start_sound_object(sound_object &s)
{
	// start sample structures
	s.channel =  -1;

	if ( s.volume <= 0 )
		return;

#if defined(DXX_BUILD_DESCENT_II)
	if ( Dont_start_sound_objects )
		return;
#endif

	// only use up to half the sound channels for "permanent" sounts
	if ((s.flags & SOF_PERMANENT) && (N_active_sound_objects >= max(1, digi_max_channels / 4)))
		return;

	// start the sample playing

	s.channel = digi_start_sound( s.soundnum,
										s.volume,
										s.pan,
										s.flags & SOF_PLAY_FOREVER,
										s.loop_start,
										s.loop_end, &s);

	if (s.channel > -1 )
		N_active_sound_objects++;
}

static void digi_link_sound_common(const object_base &viewer, sound_object &so, const vms_vector &pos, const int forever, const fix max_volume, const vm_distance max_distance, const int soundnum, const vcsegptridx_t segnum)
{
	so.signature=next_signature++;
	if ( forever )
		so.flags |= SOF_PLAY_FOREVER;
	so.soundnum = soundnum;
	so.max_volume = max_volume;
	so.max_distance = max_distance;
	so.volume = 0;
	so.pan = 0;
#if defined(DXX_BUILD_DESCENT_II)
	if (Dont_start_sound_objects) {		//started at level start
		so.flags |= SOF_PERMANENT;
		so.channel =  -1;
	}
	else
#endif
	{
		digi_get_sound_loc(viewer.orient, viewer.pos, segnum.absolute_sibling(viewer.segnum),
                       pos, segnum, so.max_volume,
                       &so.volume, &so.pan, so.max_distance);
		digi_start_sound_object(so);
		// If it's a one-shot sound effect, and it can't start right away, then
		// just cancel it and be done with it.
		if ( (so.channel < 0) && (!(so.flags & SOF_PLAY_FOREVER)) )    {
			so.flags = 0;
			return;
		}
	}
}

void digi_link_sound_to_object3(const unsigned org_soundnum, const vcobjptridx_t objnum, const uint8_t forever, const fix max_volume, const sound_stack once, const vm_distance max_distance, const int loop_start, const int loop_end)
{
	const auto &&viewer = vcobjptr(Viewer);
	int soundnum;

	soundnum = digi_xlat_sound(org_soundnum);

	if (max_volume < 0)
		return;
//	if ( max_volume > F1_0 ) max_volume = F1_0;

	if (soundnum < 0)
		return;
	if (GameSounds[soundnum].data==NULL) {
		Int3();
		return;
	}

	if ( Newdemo_state == ND_STATE_RECORDING )		{
		newdemo_record_link_sound_to_object3( org_soundnum, objnum, max_volume, max_distance, loop_start, loop_end );
	}

	const auto &&f = find_sound_object(SoundObjects, soundnum, objnum, once);
	if (f.first == f.second)
		return;
	auto &so = *f.first;
	so.flags = SOF_USED | SOF_LINK_TO_OBJ;
	so.link_type.obj.objnum = objnum;
	so.link_type.obj.objsignature = objnum->signature;
	so.loop_start = loop_start;
	so.loop_end = loop_end;
	digi_link_sound_common(viewer, so, objnum->pos, forever, max_volume, max_distance, soundnum, vcsegptridx(objnum->segnum));
}

void digi_link_sound_to_object2(const unsigned soundnum, const vcobjptridx_t objnum, const uint8_t forever, const fix max_volume, const sound_stack once, const vm_distance max_distance)
{
	digi_link_sound_to_object3(soundnum, objnum, forever, max_volume, once, max_distance, -1, -1);
}

void digi_link_sound_to_object(const unsigned soundnum, const vcobjptridx_t objnum, const uint8_t forever, const fix max_volume, const sound_stack once)
{
	digi_link_sound_to_object2(soundnum, objnum, forever, max_volume, once, vm_distance{256*F1_0});
}

static void digi_link_sound_to_pos2(fvcobjptr &vcobjptr, const int org_soundnum, const vcsegptridx_t segnum, short sidenum, const vms_vector &pos, int forever, fix max_volume, const vm_distance max_distance)
{
	const auto &&viewer = vcobjptr(Viewer);
	int volume, pan;
	int soundnum;

	soundnum = digi_xlat_sound(org_soundnum);

	if (max_volume < 0)
		return;
//	if ( max_volume > F1_0 ) max_volume = F1_0;

	if (soundnum < 0)
		return;
	if (GameSounds[soundnum].data==NULL) {
		Int3();
		return;
	}
	if (!forever)
	{
		// Hack to keep sounds from building up...
		digi_get_sound_loc(viewer->orient, viewer->pos, segnum.absolute_sibling(viewer->segnum), pos, segnum, max_volume, &volume, &pan, max_distance);
		digi_play_sample_3d(org_soundnum, pan, volume);
		return;
	}

	auto f = find_sound_object_flags0(SoundObjects);
	if (f.first == f.second)
		return;
	auto &so = *f.first;
	so.flags = SOF_USED | SOF_LINK_TO_POS;
	so.link_type.pos.segnum = segnum;
	so.link_type.pos.sidenum = sidenum;
	so.link_type.pos.position = pos;
	so.loop_start = so.loop_end = -1;
	digi_link_sound_common(viewer, so, pos, forever, max_volume, max_distance, soundnum, segnum);
}

void digi_link_sound_to_pos(int soundnum, const vcsegptridx_t segnum, short sidenum, const vms_vector &pos, int forever, fix max_volume)
{
	digi_link_sound_to_pos2(vcobjptr, soundnum, segnum, sidenum, pos, forever, max_volume, vm_distance{F1_0 * 256});
}

//if soundnum==-1, kill any sound
void digi_kill_sound_linked_to_segment( segnum_t segnum, int sidenum, int soundnum )
{
	if (soundnum != -1)
		soundnum = digi_xlat_sound(soundnum);
	range_for (auto &i, SoundObjects)
	{
		if ( (i.flags & SOF_USED) && (i.flags & SOF_LINK_TO_POS) )	{
			if ((i.link_type.pos.segnum == segnum) && (i.link_type.pos.sidenum==sidenum) && (soundnum==-1 || i.soundnum==soundnum ))	{
				digi_kill_sound(i);
			}
		}
	}
}

void digi_kill_sound_linked_to_object(const vcobjptridx_t objnum)
{
	if ( Newdemo_state == ND_STATE_RECORDING )		{
		newdemo_record_kill_sound_linked_to_object( objnum );
	}
	range_for (auto &i, SoundObjects)
	{
		if ( (i.flags & SOF_USED) && (i.flags & SOF_LINK_TO_OBJ ) )	{
			if (i.link_type.obj.objnum == objnum)	{
				digi_kill_sound(i);
			}
		}
	}
}

//	John's new function, 2/22/96.
static void digi_record_sound_objects()
{
	range_for (auto &s, SoundObjects)
	{
		if ((s.flags & SOF_USED) && (s.flags & SOF_LINK_TO_OBJ) && (s.flags & SOF_PLAY_FOREVER))
		{
			newdemo_record_link_sound_to_object3( digi_unxlat_sound(s.soundnum), s.link_type.obj.objnum,
				s.max_volume, s.max_distance, s.loop_start, s.loop_end );
		}
	}
}

static int was_recording = 0;

void digi_sync_sounds()
{
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
	if (!Viewer)
		return;
	const auto &&viewer = vcobjptr(Viewer);
	range_for (auto &s, SoundObjects)
	{
		if (s.flags & SOF_USED)
		{
			oldvolume = s.volume;
			oldpan = s.pan;

			if ( !(s.flags & SOF_PLAY_FOREVER) )	{
			 	// Check if its done.
				if (s.channel > -1 ) {
					if ( !digi_is_channel_playing(s.channel) )	{
						digi_end_sound( s.channel );
						s.flags = 0;	// Mark as dead, so some other sound can use this sound
						N_active_sound_objects--;
						continue;		// Go on to next sound...
					}
				}
			}

			if ( s.flags & SOF_LINK_TO_POS )	{
				digi_get_sound_loc(viewer->orient, viewer->pos, vcsegptridx(viewer->segnum), s.link_type.pos.position, vcsegptridx(s.link_type.pos.segnum), s.max_volume,
                                &s.volume, &s.pan, s.max_distance );

			} else if ( s.flags & SOF_LINK_TO_OBJ )	{
				const auto objp = [&s]{
					if (Newdemo_state != ND_STATE_PLAYBACK)
						return vcobjptr(s.link_type.obj.objnum);
					auto objnum = newdemo_find_object(s.link_type.obj.objsignature);
					if (objnum != object_none)
						return static_cast<vcobjptr_t>(objnum);
					return vcobjptr(object_first);
				}();

				if ((objp->type==OBJ_NONE) || (objp->signature!=s.link_type.obj.objsignature))	{
					// The object that this is linked to is dead, so just end this sound if it is looping.
					if ( s.channel>-1 )	{
						if (s.flags & SOF_PLAY_FOREVER)
							digi_stop_sound( s.channel );
						else
							digi_end_sound( s.channel );
						N_active_sound_objects--;
					}
					s.flags = 0;	// Mark as dead, so some other sound can use this sound
					continue;		// Go on to next sound...
				} else {
					digi_get_sound_loc(viewer->orient, viewer->pos, vcsegptridx(viewer->segnum), objp->pos, vcsegptridx(objp->segnum), s.max_volume,
                                   &s.volume, &s.pan, s.max_distance );
				}
			}

			if (oldvolume != s.volume) 	{
				if ( s.volume < 1 )	{
					// Sound is too far away, so stop it from playing.

					const auto c = s.channel;
					if (c > -1)
					{
						s.channel = -1;
						if (s.flags & SOF_PLAY_FOREVER)
							digi_stop_sound(c);
						else
							digi_end_sound(c);
						N_active_sound_objects--;
					}

					if (! (s.flags & SOF_PLAY_FOREVER)) {
						s.flags = 0;	// Mark as dead, so some other sound can use this sound
						continue;
					}

				} else {
					if (s.channel<0)	{
						digi_start_sound_object(s);
					} else {
						digi_set_channel_volume( s.channel, s.volume );
					}
				}
			}

			if (oldpan != s.pan) 	{
				if (s.channel>-1)
					digi_set_channel_pan( s.channel, s.pan );
			}

		}
	}
}

void digi_pause_digi_sounds()
{
	digi_pause_looping_sound();
	range_for (auto &s, SoundObjects)
	{
		if (!(s.flags & SOF_USED))
			continue;
		const auto c = s.channel;
		if (c > -1)
		{
			s.channel = -1;
			if (! (s.flags & SOF_PLAY_FOREVER))
				s.flags = 0;	// Mark as dead, so some other sound can use this sound
			N_active_sound_objects--;
			digi_stop_sound(c);
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
void digi_end_soundobj(sound_object &s)
{
	Assert(s.flags & SOF_USED);
	Assert(s.channel > -1);

	N_active_sound_objects--;
	s.channel = -1;
}

void digi_stop_digi_sounds()
{
	digi_stop_looping_sound();
	range_for (auto &s, SoundObjects)
	{
		if (s.flags & SOF_USED)
		{
			if ( s.channel > -1 )	{
				digi_stop_sound( s.channel );
				N_active_sound_objects--;
			}
			s.flags = 0;	// Mark as dead, so some other sound can use this sound
		}
	}

	digi_stop_all_channels();
	SoundQ_init();
}

#ifndef NDEBUG
int verify_sound_channel_free( int channel )
{
	const auto predicate = [channel](const sound_object &s) {
		return (s.flags & SOF_USED) && s.channel == channel;
	};
	if (std::any_of(SoundObjects.begin(), SoundObjects.end(), predicate))
		throw std::runtime_error("sound busy");
	return 0;
}
#endif

struct sound_q
{
	fix64 time_added;
	int soundnum;
};

#define MAX_LIFE F1_0*30		// After being queued for 30 seconds, don't play it
static int SoundQ_head, SoundQ_tail, SoundQ_num;
int SoundQ_channel;
static array<sound_q, 32> SoundQ;

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
	if (SoundQ_head >= SoundQ.size())
		SoundQ_head = 0;
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
			SoundQ_channel = digi_start_sound(q->soundnum, F1_0+1, 0xFFFF/2, 0, -1, -1, sound_object_none);
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
	if (i >= SoundQ.size())
		i = 0;

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

}
