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
 * $Source: /cvs/cvsroot/d2x/main/digi.h,v $
 * $Revision: 1.1.1.2 $
 * $Author: bradleyb $
 * $Date: 2001-01-19 03:33:43 $
 * 
 * Include file for sound hardware.
 * 
 * $Log: not supported by cvs2svn $
 * Revision 1.2  1999/11/15 10:43:15  sekmu
 * added freq/br to digi_sound struct for alt sounds
 *
 * Revision 1.1.1.1  1999/06/14 22:12:14  donut
 * Import of d1x 1.37 source.
 *
 * Revision 2.0  1995/02/27  11:28:40  john
 * New version 2.0, which has no anonymous unions, builds with
 * Watcom 10.0, and doesn't require parsing BITMAPS.TBL.
 * 
 * Revision 1.29  1995/02/11  12:42:00  john
 * Added new song method, with FM bank switching..
 * 
 * Revision 1.28  1995/02/03  17:08:26  john
 * Changed sound stuff to allow low memory usage.
 * Also, changed so that Sounds isn't an array of digi_sounds, it
 * is a ubyte pointing into GameSounds, this way the digi.c code that
 * locks sounds won't accidentally unlock a sound that is already playing, but
 * since it's Sounds[soundno] is different, it would erroneously be unlocked.
 * 
 * Revision 1.27  1995/02/01  22:20:31  john
 * Added digi_is_sound_playing.
 * 
 * Revision 1.26  1994/12/20  18:03:51  john
 * Added loop midi flag.
 * 
 * Revision 1.25  1994/12/13  00:46:14  john
 * Split digi and midi volume into 2 seperate functions.
 * 
 * Revision 1.24  1994/12/10  20:34:53  john
 * Added digi_kill_sound_linked_to_object.
 * 
 * Revision 1.23  1994/12/10  15:59:39  mike
 * Fixed bug.
 * 
 * Revision 1.22  1994/12/10  15:44:35  john
 * Added max_distance passing for sound objects.
 * 
 * Revision 1.21  1994/12/05  12:17:40  john
 * Added code that locks/unlocks digital sounds on demand.
 * 
 * Revision 1.20  1994/11/28  18:34:57  john
 * Made the digi_max_channels cut of an old sound instead of
 * not playing a new sound.
 * 
 * Revision 1.19  1994/11/14  17:53:56  allender
 * made some digi variables extern
 * 
 * Revision 1.18  1994/10/28  14:42:58  john
 * Added sound volumes to all sound calls.
 * 
 * Revision 1.17  1994/10/11  15:25:37  john
 * Added new function to play a sound once...
 * 
 * Revision 1.16  1994/10/03  20:51:44  john
 * Started added pause sound function; for the network I changed to 
 * packet structure a bit; never tested, though.
 * 
 * 
 * Revision 1.15  1994/10/03  13:09:43  john
 * Added Pause function, but never tested it yet.
 * 
 * Revision 1.14  1994/09/30  10:09:24  john
 * Changed sound stuff... made it so the reseting card doesn't hang, 
 * made volume change only if sound is installed.
 * 
 * Revision 1.13  1994/09/29  21:13:43  john
 * Added Master volumes for digi and midi. Also took out panning,
 * because it doesn't work with MasterVolume stuff. 
 * 
 * Revision 1.12  1994/09/29  12:42:34  john
 * Added sidenum to keep track of sound pos. Made sound functions
 * not do anything if nosound. Made sounds_init delete currently
 * playing sounds.
 * 
 * Revision 1.11  1994/09/29  12:23:42  john
 * Added digi_kill_sound_linked_to_segment function.
 * 
 * Revision 1.10  1994/09/29  11:59:04  john
 * Added digi_kill_sound
 * 
 * Revision 1.9  1994/09/29  10:37:38  john
 * Added sound objects that dynamicaly change volume,pan.
 * 
 * Revision 1.8  1994/09/28  16:18:37  john
 * Added capability to play midi song.
 * 
 * Revision 1.7  1994/06/17  18:01:41  john
 * A bunch of new stuff by John
 * 
 * Revision 1.6  1994/06/15  19:00:58  john
 * Added the capability to make 3d sounds play just once for the
 * laser hit wall effects.
 * 
 * Revision 1.5  1994/06/07  10:54:30  john
 * Made key S reinit the sound system.
 * 
 * Revision 1.4  1994/05/09  21:11:39  john
 * Sound changes; pass index instead of pointer to digi routines.
 * Made laser sound cut off the last laser sound.
 * 
 * Revision 1.3  1994/04/27  11:44:25  john
 * First version of sound! Yay!
 * 
 * Revision 1.2  1994/04/20  21:58:50  john
 * First version of sound stuff... hopefully everything
 * is commented out because it hangs..
 * 
 * Revision 1.1  1994/04/15  14:25:02  john
 * Initial revision
 * 
 * 
 */



#ifndef _DIGI_H
#define _DIGI_H

#include "pstypes.h"
#include "vecmat.h"

/*
#ifdef __DJGPP__
#define ALLEGRO
#endif
*/

#ifdef ALLEGRO
#include "allg_snd.h"
typedef SAMPLE digi_sound;
#else
typedef struct digi_sound       {
        int bits;
        int freq;
	int length;
	ubyte * data;
} digi_sound;
#endif

#define SAMPLE_RATE_11K         11025
#define SAMPLE_RATE_22K         22050



#ifdef __ENV_DJGPP__
extern int digi_driver_board;
extern int digi_driver_port;
extern int digi_driver_irq;
extern int digi_driver_dma;
extern int digi_midi_type;
extern int digi_midi_port;
#endif

extern int digi_sample_rate;

extern int digi_get_settings();
extern int digi_init();
extern void digi_reset();
extern void digi_close();

int digi_xlat_sound(int sound);
// Volume is max at F1_0.
extern void digi_play_sample( int sndnum, fix max_volume );
extern void digi_play_sample_once( int sndnum, fix max_volume );
extern int digi_link_sound_to_object( int soundnum, short objnum, int forever, fix max_volume );
extern int digi_link_sound_to_pos( int soundnum, short segnum, short sidenum, vms_vector * pos, int forever, fix max_volume );
// Same as above, but you pass the max distance sound can be heard.  The old way uses f1_0*256 for max_distance.
extern int digi_link_sound_to_object2( int soundnum, short objnum, int forever, fix max_volume, fix  max_distance );
extern int digi_link_sound_to_pos2( int soundnum, short segnum, short sidenum, vms_vector * pos, int forever, fix max_volume, fix max_distance );

extern int digi_link_sound_to_object3( int org_soundnum, short objnum, int forever, fix max_volume, fix  max_distance, int loop_start, int loop_end );

extern void digi_play_midi_song( char * filename, char * melodic_bank, char * drum_bank, int loop );

extern void digi_play_sample_3d( int soundno, int angle, int volume, int no_dups ); // Volume from 0-0x7fff

extern void digi_init_sounds();
extern void digi_sync_sounds();
extern void digi_kill_sound_linked_to_segment( int segnum, int sidenum, int soundnum );
extern void digi_kill_sound_linked_to_object( int objnum );

extern void digi_set_midi_volume( int mvolume );
extern void digi_set_digi_volume( int dvolume );
extern void digi_set_volume( int dvolume, int mvolume );

extern int digi_is_sound_playing(int soundno);

extern void digi_pause_all();
extern void digi_resume_all();
extern void digi_stop_all();

extern void digi_set_max_channels(int n);
extern int digi_get_max_channels();

extern int digi_lomem;

extern void digi_pause_digi_sounds();
extern void digi_resume_digi_sounds();

int digi_start_sound(int soundnum, fix volume, fix pan, int unknown1, int unknown2, int unknown3, int unknown4);
void digi_stop_sound(int channel);
void digi_start_sound_queued( short soundnum, fix volume );
void digi_play_sample_looping( int soundno, fix max_volume,int loop_start, int loop_end );
void digi_stop_looping_sound(void);
void digi_change_looping_volume(fix volume);

#endif
 
