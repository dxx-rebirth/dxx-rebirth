/* $Id: newdemo.h,v 1.3 2003-03-17 07:59:11 btb Exp $ */
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

/*
 *
 * header for demo playback system
 *
 * Old Log:
 * Revision 1.1  1995/05/16  16:00:24  allender
 * Initial revision
 *
 * Revision 2.0  1995/02/27  11:27:18  john
 * New version 2.0, which has no anonymous unions, builds with
 * Watcom 10.0, and doesn't require parsing BITMAPS.TBL.
 *
 * Revision 1.43  1995/01/19  09:41:43  allender
 * prototype for laser level recording
 *
 * Revision 1.42  1995/01/18  18:48:49  allender
 * added function prototype for door_open
 *
 * Revision 1.41  1995/01/17  17:42:31  allender
 * new prototypes for ammo counts
 *
 * Revision 1.40  1995/01/04  15:04:27  allender
 * added some different prototypes for registered
 *
 * Revision 1.39  1995/01/03  11:45:11  allender
 * extern function definition
 *
 * Revision 1.38  1994/12/29  16:43:31  allender
 * new function prototype
 *
 * Revision 1.37  1994/12/28  14:15:27  allender
 * new function prototypes
 *
 * Revision 1.36  1994/12/21  12:46:41  allender
 * new functions for multiplayer deaths and kills
 *
 * Revision 1.35  1994/12/12  11:32:55  allender
 * added new record function to restore after in rearview mode
 *
 * Revision 1.34  1994/12/08  21:03:15  allender
 * added new param to record_player_flags
 *
 * Revision 1.33  1994/12/08  13:47:01  allender
 * removed function call to record_rearview
 *
 * Revision 1.32  1994/12/06  12:57:10  allender
 * added new prototype for multi decloaking
 *
 * Revision 1.31  1994/12/01  11:46:34  allender
 * added recording prototype for multi player cloak
 *
 * Revision 1.30  1994/11/27  23:04:22  allender
 * function prototype for recording new levels
 *
 * Revision 1.29  1994/11/07  08:47:43  john
 * Made wall state record.
 *
 * Revision 1.28  1994/11/05  17:22:53  john
 * Fixed lots of sequencing problems with newdemo stuff.
 *
 * Revision 1.27  1994/11/04  16:48:49  allender
 * extern Newdemo_do_interpolate variable
 *
 * Revision 1.26  1994/11/02  14:08:53  allender
 * record rearview
 *
 * Revision 1.25  1994/10/31  13:35:04  allender
 * added two record functions to save and restore cockpit state on
 * death sequence
 *
 * Revision 1.24  1994/10/29  16:01:11  allender
 * added ND_STATE_NODEMOS to indicate that there are no demos currently
 * available for playback
 *
 * Revision 1.23  1994/10/28  12:41:58  allender
 * add homing distance recording event
 *
 * Revision 1.22  1994/10/27  16:57:32  allender
 * removed VCR_MODE stuff, and added monitor blowup effects
 *
 * Revision 1.21  1994/10/26  14:44:48  allender
 * completed hacked in vcr type demo playback states
 *
 * Revision 1.20  1994/10/26  13:40:38  allender
 * more vcr demo playback defines
 *
 * Revision 1.19  1994/10/26  08:51:26  allender
 * record player weapon change
 *
 * Revision 1.18  1994/10/25  16:25:31  allender
 * prototypes for shield, energy and flags
 *
 * Revision 1.17  1994/08/15  18:05:30  john
 * *** empty log message ***
 *
 * Revision 1.16  1994/07/21  13:11:26  matt
 * Ripped out remants of old demo system, and added demo only system that
 * disables object movement and game options from menu.
 *
 * Revision 1.15  1994/07/05  12:49:02  john
 * Put functionality of New Hostage spec into code.
 *
 * Revision 1.14  1994/06/27  15:53:12  john
 * #define'd out the newdemo stuff
 *
 *
 * Revision 1.13  1994/06/24  17:01:25  john
 * Add VFX support; Took Game Sequencing, like EndGame and stuff and
 * took it out of game.c and into gameseq.c
 *
 * Revision 1.12  1994/06/21  19:46:05  john
 * Added palette effects to demo recording.
 *
 * Revision 1.11  1994/06/21  14:19:58  john
 * Put in hooks to record HUD messages.
 *
 * Revision 1.10  1994/06/20  11:50:42  john
 * Made demo record flash effect, and control center triggers.
 *
 * Revision 1.9  1994/06/17  18:01:29  john
 * A bunch of new stuff by John
 *
 * Revision 1.8  1994/06/17  12:13:34  john
 * More newdemo stuff; made editor->game transition start in slew mode.
 *
 * Revision 1.7  1994/06/16  13:02:02  john
 * Added morph hooks.
 *
 * Revision 1.6  1994/06/15  19:01:42  john
 * Added the capability to make 3d sounds play just once for the
 * laser hit wall effects.
 *
 * Revision 1.5  1994/06/15  14:57:11  john
 * Added triggers to demo recording.
 *
 * Revision 1.4  1994/06/14  20:42:19  john
 * Made robot matztn cntr not work until no robots or player are
 * in the segment.
 *
 * Revision 1.3  1994/06/14  14:43:52  john
 * Made doors work with newdemo system.
 *
 * Revision 1.2  1994/06/13  21:02:44  john
 * Initial version of new demo recording system.
 *
 * Revision 1.1  1994/06/13  15:51:09  john
 * Initial revision
 *
 *
 */


#ifndef _NEWDEMO_H
#define _NEWDEMO_H

#ifdef NEWDEMO

#define ND_STATE_NORMAL             0
#define ND_STATE_RECORDING          1
#define ND_STATE_PLAYBACK           2
#define ND_STATE_PAUSED             3
#define ND_STATE_REWINDING          4
#define ND_STATE_FASTFORWARD        5
#define ND_STATE_ONEFRAMEFORWARD    6
#define ND_STATE_ONEFRAMEBACKWARD   7
#define ND_STATE_PRINTSCREEN        8

#ifndef MACINTOSH
#define DEMO_DIR                "demos/"
#else
#define DEMO_DIR                ":Demos:"
#endif

// Gives state of recorder
extern int Newdemo_state;
extern int NewdemoFrameCount;
extern int Newdemo_game_mode;

extern int Newdemo_vcr_state;
extern byte Newdemo_do_interpolate;

//Does demo start automatically?
extern int Auto_demo;

// Functions called during recording process...
extern void newdemo_record_start_demo();
extern void newdemo_record_start_frame(int frame_number, fix frame_time );
extern void newdemo_record_render_object(object * obj);
extern void newdemo_record_viewer_object(object * obj);
extern void newdemo_record_sound_3d( int soundno, int angle, int volume );
extern void newdemo_record_sound_3d_once( int soundno, int angle, int volume );
extern void newdemo_record_sound_once( int soundno );
extern void newdemo_record_sound( int soundno );
extern void newdemo_record_wall_hit_process( int segnum, int side, int damage, int playernum );
extern void newdemo_record_trigger( int segnum, int side, int objnum,int shot );
extern void newdemo_record_hostage_rescued( int hostage_num );
extern void newdemo_record_morph_frame();
extern void newdemo_record_player_stats(int shields, int energy, int score );
extern void newdemo_record_player_afterburner(fix old_afterburner, fix afterburner);
extern void newdemo_record_wall_toggle(int segnum, int side );
extern void newdemo_record_control_center_destroyed();
extern void newdemo_record_hud_message(char *s);
extern void newdemo_record_palette_effect(short r, short g, short b);
extern void newdemo_record_player_energy(int, int);
extern void newdemo_record_player_shields(int, int);
extern void newdemo_record_player_flags(uint, uint);
extern void newdemo_record_player_weapon(int, int);
extern void newdemo_record_effect_blowup(short, int, vms_vector *);
extern void newdemo_record_homing_distance(fix);
extern void newdemo_record_letterbox(void);
extern void newdemo_record_rearview(void);
extern void newdemo_record_restore_cockpit(void);
extern void newdemo_record_wall_set_tmap_num1(short seg,ubyte side,short cseg,ubyte cside,short tmap);
extern void newdemo_record_wall_set_tmap_num2(short seg,ubyte side,short cseg,ubyte cside,short tmap);
extern void newdemo_record_multi_cloak(int pnum);
extern void newdemo_record_multi_decloak(int pnum);
extern void newdemo_set_new_level(int level_num);
extern void newdemo_record_restore_rearview(void);

extern void newdemo_record_multi_death(int pnum);
extern void newdemo_record_multi_kill(int pnum, byte kill);
extern void newdemo_record_multi_connect(int pnum, int new_player, char *new_callsign);
extern void newdemo_record_multi_reconnect(int pnum);
extern void newdemo_record_multi_disconnect(int pnum);
extern void newdemo_record_player_score(int score);
extern void newdemo_record_multi_score(int pnum, int score);
extern void newdemo_record_primary_ammo(int old_ammo, int new_ammo);
extern void newdemo_record_secondary_ammo(int old_ammo, int new_ammo);
extern void newdemo_record_door_opening(int segnum, int side);
extern void newdemo_record_laser_level(byte old_level, byte new_level);
extern void newdemo_record_cloaking_wall(int front_wall_num, int back_wall_num, ubyte type, ubyte state, fix cloak_value, fix l0, fix l1, fix l2, fix l3);
extern void newdemo_record_secret_exit_blown(int truth);


// Functions called during playback process...
extern void newdemo_object_move_all();
extern void newdemo_playback_one_frame();
extern void newdemo_goto_end();
extern void newdemo_goto_beginning();

// Interactive functions to control playback/record;
extern void newdemo_start_playback( char * filename );
extern void newdemo_stop_playback();
extern void newdemo_start_recording();
extern void newdemo_stop_recording();

extern int newdemo_get_percent_done();

extern void newdemo_record_link_sound_to_object3( int soundno, short objnum, fix max_volume, fix  max_distance, int loop_start, int loop_end );
extern int newdemo_find_object( int signature );
extern void newdemo_record_kill_sound_linked_to_object( int objnum );

#endif // NEWDEMO

#endif // _NEWDEMO_H
