/* $Id: newdemo.c,v 1.12 2003-03-18 02:31:16 btb Exp $ */
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
 * Code to make a complete demo playback system.
 *
 * $Log: not supported by cvs2svn $
 * Revision 1.11  2003/03/17 07:59:11  btb
 * also look in shared data dir for demos
 *
 * Revision 1.10  2003/03/17 07:10:21  btb
 * comments/formatting
 *
 * Revision 1.12  1995/10/31  10:19:43  allender
 * shareware stuff
 *
 * Revision 1.11  1995/10/17  13:19:16  allender
 * close boxes for demo save
 *
 * Revision 1.10  1995/10/05  10:36:07  allender
 * fixed calls to digi_play_sample to account for differing volume and angle calculations
 *
 * Revision 1.9  1995/09/12  15:49:13  allender
 * define __useAppleExts__ if not already defined
 *
 * Revision 1.8  1995/09/05  14:28:32  allender
 * fixed previous N_players bug in newdemo_goto_end
 *
 * Revision 1.7  1995/09/05  14:16:51  allender
 * added space to allowable demo filename characters
 * and fixed bug with netgame demos N_players got getting
 * set correctly
 *
 * Revision 1.6  1995/09/01  16:10:47  allender
 * fixed bug with reading in N_players variable on
 * netgame demo playback
 *
 * Revision 1.5  1995/08/24  16:04:11  allender
 * fix signed byte problem
 *
 * Revision 1.4  1995/08/12  12:21:59  allender
 * made call to create_shortpos not swap the shortpos
 * elements
 *
 * Revision 1.3  1995/08/01  16:04:19  allender
 * made random picking of demo work
 *
 * Revision 1.2  1995/08/01  13:56:56  allender
 * demo system working on the mac
 *
 * Revision 1.1  1995/05/16  15:28:59  allender
 * Initial revision
 *
 * Revision 2.7  1995/05/26  16:16:06  john
 * Split SATURN into define's for requiring cd, using cd, etc.
 * Also started adding all the Rockwell stuff.
 *
 * Revision 2.6  1995/03/21  14:39:38  john
 * Ifdef'd out the NETWORK code.
 *
 * Revision 2.5  1995/03/14  18:24:31  john
 * Force Destination Saturn to use CD-ROM drive.
 *
 * Revision 2.4  1995/03/14  16:22:29  john
 * Added cdrom alternate directory stuff.
 *
 * Revision 2.3  1995/03/10  12:58:33  allender
 * only display rear view cockpit when cockpit mode was CM_FULL_COCKPIT.
 *
 * Revision 2.2  1995/03/08  16:12:15  allender
 * changes for Destination Saturn
 *
 * Revision 2.1  1995/03/08  12:11:26  allender
 * fix shortpos reading
 *
 * Revision 2.0  1995/02/27  11:29:40  john
 * New version 2.0, which has no anonymous unions, builds with
 * Watcom 10.0, and doesn't require parsing BITMAPS.TBL.
 *
 * Revision 1.189  1995/02/22  14:53:42  allender
 * missed some anonymous stuff
 *
 * Revision 1.188  1995/02/22  13:24:53  john
 * Removed the vecmat anonymous unions.
 *
 * Revision 1.187  1995/02/22  13:13:54  allender
 * remove anonymous unions from object structure
 *
 * Revision 1.186  1995/02/14  15:36:41  allender
 * fix fix for morph effect
 *
 * Revision 1.185  1995/02/14  11:25:48  allender
 * save cockpit mode and restore after playback.  get orientation for morph
 * effect when object is morph vclip
 *
 * Revision 1.184  1995/02/13  12:18:14  allender
 * change to decide about interpolating or not
 *
 * Revision 1.183  1995/02/12  00:46:23  adam
 * don't decide to skip frames until after at least 10 frames have
 * passed -- allender
 *
 * Revision 1.182  1995/02/11  22:34:01  john
 * Made textures page in for newdemos before playback time.
 *
 * Revision 1.181  1995/02/11  17:28:32  allender
 * strip frames from end of demo
 *
 * Revision 1.180  1995/02/11  16:40:35  allender
 * start of frame stripping debug code
 *
 * Revision 1.179  1995/02/10  17:40:06  allender
 * put back in wall_hit_process code to fix door problem
 *
 * Revision 1.178  1995/02/10  17:17:24  adam
 * allender } fix
 *
 * Revision 1.177  1995/02/10  17:16:24  allender
 * fix possible tmap problems
 *
 * Revision 1.176  1995/02/10  15:54:37  allender
 * changes for out of space on device.
 *
 * Revision 1.175  1995/02/09  19:55:00  allender
 * fix bug with morph recording -- force rendertype to RT_POLYOBJ when
 * playing back since it won't render until fully morphed otherwise
 *
 * Revision 1.174  1995/02/07  17:15:35  allender
 * DOH!!!!!
 *
 * Revision 1.173  1995/02/07  17:14:21  allender
 * immediately return when loading bogus level stuff when reading a frame
 *
 * Revision 1.172  1995/02/02  11:15:03  allender
 * after loading new level, read next frame (forward or back) always because
 * of co-op ships showing up when level is loaded
 *
 * Revision 1.171  1995/02/02  10:24:16  allender
 * removed cfile stuff.  Use standard FILE functions for demo playback
 *
 * Revision 1.170  1995/01/30  13:54:32  allender
 * support for missions
 *
 * Revision 1.169  1995/01/27  16:27:35  allender
 * put game mode to demo_game_mode when sorting multiplayer kill (and score)
 * list
 *
 * Revision 1.168  1995/01/27  09:52:25  allender
 * minor changes because of object/segment linking problems
 *
 * Revision 1.167  1995/01/27  09:22:28  allender
 * changed way multi-player score is recorded.  Record difference, not
 * actual
 *
 * Revision 1.166  1995/01/25  14:32:44  allender
 * changed with recorded player flags.  More checks for paused state
 * during interpolation reading of objects
 *
 * Revision 1.165  1995/01/25  11:23:32  allender
 * found bug with out of disk space problem
 *
 * Revision 1.164  1995/01/25  11:11:33  allender
 * coupla' things.  Fix problem with objects apparently being linked in
 * the wrong segment.  Put an Int3 in to check why demos will write to
 * end of space on drive. close demo file if demo doens't start playing
 * back.  Put obj->type == OBJ_ROBOT around checking for boss cloaking
 *
 * Revision 1.163  1995/01/24  19:44:30  allender
 * fix obscure problem with rewinding and having the wrong object linked
 * to the wrong segments.  will investigate further.
 *
 * Revision 1.162  1995/01/23  09:31:28  allender
 * add team score in team mode playback
 *
 * Revision 1.161  1995/01/20  22:47:39  matt
 * Mission system implemented, though imcompletely
 *
 * Revision 1.160  1995/01/20  09:30:37  allender
 * don't call LoadLevel with bogus data
 *
 * Revision 1.159  1995/01/20  09:13:23  allender
 * *&^%&*%$ typo
 *
 * Revision 1.158  1995/01/20  09:12:04  allender
 * record team names during demo recoring in GM_TEAM
 *
 * Revision 1.157  1995/01/19  16:31:09  allender
 * forgot to bump demo version for new weapon change stuff
 *
 * Revision 1.156  1995/01/19  16:29:33  allender
 * added new byte for weapon change (old weapon) so rewinding works
 * correctly for weapon changes in registered
 *
 * Revision 1.155  1995/01/19  15:00:05  allender
 * remove code to take away blastable walls in multiplayer demo playbacks
 *
 * Revision 1.154  1995/01/19  11:07:05  allender
 * put in psuedo cloaking for boss robots.  Problem is that cloaking is
 * time based, and that don't get done in demos, so bosses just disappear.
 * oh well
 *
 * Revision 1.153  1995/01/19  09:42:29  allender
 * record laser levels in demos
 *
 * Revision 1.152  1995/01/18  20:43:12  allender
 * fix laser level stuff on goto-beginning and goto-end
 *
 * Revision 1.151  1995/01/18  20:28:18  allender
 * cloak robots now cloak (except maybe for boss........)  Put in function
 * to deal with control center triggers
 *
 * Revision 1.150  1995/01/18  18:55:07  allender
 * bug fix
 *
 * Revision 1.149  1995/01/18  18:49:03  allender
 * lots 'o stuff....record laser level.  Record beginning of door opening
 * sequence.  Fix some problems with control center stuff.  Control center
 * triggers now work in reverse
 *
 * Revision 1.148  1995/01/18  08:51:40  allender
 * forgot to record ammo counts at beginning of demo
 *
 * Revision 1.147  1995/01/17  17:42:07  allender
 * added primary and secondary ammo counts.  Changed goto_end routine
 * to be more efficient
 *
 * Revision 1.146  1995/01/17  13:46:35  allender
 * fix problem with destroyed control center and rewinding a demo.
 * Save callsign and restore after demo playback
 *
 * Revision 1.145  1995/01/12  10:21:53  allender
 * fixes for 1.0 to 1.1 demo incompatibility
 *
 * Revision 1.144  1995/01/05  13:51:43  allender
 * fixed type of player num variable
 *
 * Revision 1.143  1995/01/04  16:58:28  allender
 * bumped up demo version number
 *
 * Revision 1.142  1995/01/04  14:59:02  allender
 * added more information to end of demo for registered.  Forced game mode
 * to be GM_NORMAL on demo playback
 *
 * Revision 1.141  1995/01/03  17:30:47  allender
 * fixed logic problem with cloak stuf
 *
 * Revision 1.140  1995/01/03  17:12:23  allender
 * fix for getting cloak stuff at end of demo for shareware
 *
 * Revision 1.139  1995/01/03  15:20:24  allender
 * fix goto_end for shareware -- changes to goto_end for registered
 *
 * Revision 1.138  1995/01/03  13:13:26  allender
 * add } I forgot
 *
 * Revision 1.137  1995/01/03  13:10:29  allender
 * make score work forwards and backwards
 *
 * Revision 1.136  1995/01/03  11:45:20  allender
 * added code to record players scores
 *
 * Revision 1.135  1994/12/30  10:03:57  allender
 * put cloak stuff at end of demo for fast forward to the end
 *
 * Revision 1.134  1994/12/29  17:02:55  allender
 * spelling fix on SHAREWARE
 *
 * Revision 1.133  1994/12/29  16:43:41  allender
 * lots of new multiplayer stuff.  wrapped much code with SHAREWARE defines
 *
 * Revision 1.132  1994/12/28  14:15:01  allender
 * added routines to deal with connecting and reconnecting players when
 * recording multiplayer demos
 *
 * Revision 1.131  1994/12/21  12:57:59  allender
 * bug fix
 *
 * Revision 1.130  1994/12/21  12:46:53  allender
 * record multi player deaths and kills
 *
 * Revision 1.129  1994/12/19  16:37:27  allender
 * pick good filename when trying to save in network play and player
 * gets bumped out of menu by multi-player code
 *
 * Revision 1.128  1994/12/14  10:49:01  allender
 * reset bad_read variable when starting demo playback
 *
 * Revision 1.127  1994/12/14  08:53:06  allender
 * lowered watermark for out of space
 *
 * Revision 1.126  1994/12/14  08:49:52  allender
 * put up warning when starting demo recording if not enough space and
 * not let them record
 *
 * Revision 1.125  1994/12/13  00:01:37  allender
 * CLOAK FIX -- (I'm tempted to take cloak out of game because I can't
 * seem to get it right in demo playback)
 *
 * Revision 1.124  1994/12/12  14:51:21  allender
 * more fixed to multiplayer cloak stuff
 *
 * Revision 1.123  1994/12/12  11:33:11  allender
 * fixed rearview mode to work again
 *
 * Revision 1.122  1994/12/12  11:00:16  matt
 * Added code to handle confusion with attached objects
 *
 * Revision 1.121  1994/12/12  00:31:29  allender
 * give better warning when out of space when recording.  Don't record
 * until no space left.  We have 500K watermark when we now stop recording
 *
 * Revision 1.120  1994/12/10  16:44:54  matt
 * Added debugging code to track down door that turns into rock
 *
 * Revision 1.119  1994/12/09  18:46:15  matt
 * Added code to handle odd error condition
 *
 * Revision 1.118  1994/12/09  17:27:37  allender
 * force playernum to 0 when demo is done playing
 *
 * Revision 1.117  1994/12/09  16:40:39  allender
 * yet more cloak stuff.  Assign cloak/invuln time when starting demo
 * if flags are set.  Check cloak and invuln time when demo
 * even when paused
 *
 * Revision 1.116  1994/12/09  14:59:22  matt
 * Added system to attach a fireball to another object for rendering purposes,
 * so the fireball always renders on top of (after) the object.
 *
 * Revision 1.115  1994/12/09  12:21:45  allender
 * only allow valid chars when typing in demo filename
 *
 * Revision 1.114  1994/12/08  23:19:02  allender
 * final(?) fix for getting cloak gauge to work on demo playback
 * with forward and reverse
 *
 * Revision 1.113  1994/12/08  21:34:38  allender
 * record old and new player flags to accuratedly record cloaking and
 * decloaking
 * ./
 *
 * Revision 1.112  1994/12/08  18:04:47  allender
 * bashed playernum right after reading it in demo header so shields
 * and energy are put in right place
 *
 * Revision 1.111  1994/12/08  17:10:07  allender
 * encode playernum in demo header.  Bash viewer segment to 0 if in
 * bogus segnum.  Don't link render objs for same reason
 *
 * Revision 1.110  1994/12/08  15:36:12  allender
 * cloak stuff works forwards and backwards
 *
 * Revision 1.109  1994/12/08  13:46:03  allender
 * don't record rearview anymore, but leave in case statement for playback
 * purposes.  change the way letterbox <--> cockpit transitions happen
 *
 * Revision 1.108  1994/12/08  12:36:06  matt
 * Added new object allocation & deallocation functions so other code
 * could stop messing around with internal object data structures.
 *
 * Revision 1.107  1994/12/08  11:19:04  allender
 * handle out of space (more) gracefully then before
 *
 * Revision 1.106  1994/12/08  00:29:49  allender
 * fixed bug that didn't load level on goto_beginning
 *
 * Revision 1.105  1994/12/08  00:11:51  mike
 * change matrix interpolation.
 *
 * Revision 1.104  1994/12/07  23:46:37  allender
 * changed invulnerability and cloak to work (almost) correctly both
 * in single and multi player
 *
 * Revision 1.103  1994/12/07  11:48:49  adam
 * BY ALLENDER -- added dampening of interpolation factor to 1 if greater
 * than 1 (although I have not seen this happen).  this is attempt to
 * get wobbling problem solved
 *
 * Revision 1.102  1994/12/07  11:23:56  allender
 * attempt at getting rid of wobbling on demo playback
 *
 * Revision 1.101  1994/12/06  19:31:17  allender
 * moved blastable wall stuff code to where we load level during demo
 * playback
 *
 * Revision 1.100  1994/12/06  19:21:51  allender
 * multi games, destroy blastable walls.  Do wall toggle when control center
 * destroyed
 *
 * Revision 1.99  1994/12/06  16:54:48  allender
 * fixed code so if demo automatically started from menu, don't bring up
 * message if demo is too old
 *
 * Revision 1.98  1994/12/06  13:55:15  matt
 * Use new rounding func, f2ir()
 *
 * Revision 1.97  1994/12/06  13:44:45  allender
 * suppressed compiler warnings
 *
 * Revision 1.96  1994/12/06  13:38:03  allender
 * removed recording of wall hit process.  I think that all bases are covered
 * elsewhere
 *
 * Revision 1.95  1994/12/06  12:57:35  allender
 * added recording of multi_decloaking.  Fixed some other cloaking code so
 * that cloak should last as long as player was cloaked.  We will lose the
 * guage effect, but the time is probably more important on playback
 *
 * Revision 1.94  1994/12/05  23:37:17  matt
 * Took out calls to warning() function
 *
 * Revision 1.93  1994/12/03  17:52:04  yuan
 * Localization 380ish
 *
 * Revision 1.92  1994/12/02  12:53:39  allender
 * fixed goto_beginning and goto_end on demo playback
 *
 * Revision 1.91  1994/12/01  12:01:49  allender
 * added multi player cloak stuff
 *
 * Revision 1.90  1994/11/30  09:33:58  allender
 * added field in header to tell what version (shareware or registered)
 * demo was recorded with.  Don't allow demo recorded on one to playback
 * on the other
 *
 * Revision 1.89  1994/11/29  00:31:01  allender
 * major changes -- added level recording feature which records level
 * advancement.  Changes to internal code to handle this.
 *
 * Revision 1.88  1994/11/27  23:13:54  matt
 * Made changes for new mprintf calling convention
 *
 * Revision 1.87  1994/11/27  23:07:35  allender
 * starting on code to get all level transitions recorded. not done yet
 *
 * Revision 1.86  1994/11/27  17:39:47  matt
 * Don't xlate tmap numbers when editor compiled out
 *
 * Revision 1.85  1994/11/23  09:27:21  allender
 * put up info box with message if demo version is too old or level
 * cannot be loaded
 *
 * Revision 1.84  1994/11/22  19:37:39  allender
 * fix array mistake
 *
 * Revision 1.83  1994/11/22  19:35:09  allender
 * record player ship colors in multiplayer demo recordings
 *
 * Revision 1.82  1994/11/19  15:36:42  mike
 * fix fix.
 *
 * Revision 1.81  1994/11/19  15:23:21  mike
 * rip out unused code
 *
 * Revision 1.80  1994/11/16  14:51:49  rob
 * Fixed network/demo incompatibility.
 *
 * Revision 1.79  1994/11/15  10:55:48  allender
 * made start of demo playback read initial demo information so
 * level will get loaded.  Made demo record to single file which
 * will get renamed.  Added numerics after old filename so
 * sequential filenames would be defaulted to
 *
 * Revision 1.78  1994/11/15  09:46:06  allender
 * added versioning.  Fixed problems with trying to interpolating a completely
 * 0 orientation matrix
 *
 * Revision 1.77  1994/11/14  14:34:31  matt
 * Fixed up handling when textures can't be found during remap
 *
 * Revision 1.76  1994/11/14  09:15:29  allender
 * make ESC from file save menu exit w/o saving.  Fix letterbox, rear view,
 * to normal cockpit mode transition to work correctly when skipping and
 * interpolating frames
 *
 * Revision 1.75  1994/11/11  16:22:07  allender
 * made morphing objects record only the object being morphed.
 *
 * Revision 1.74  1994/11/08  14:59:19  john
 * Added code to respond to network while in menus.
 *
 * Revision 1.73  1994/11/08  14:52:20  adam
 * *** empty log message ***
 *
 * Revision 1.72  1994/11/07  15:47:04  allender
 * prompt for filename when done recording demo
 *
 * Revision 1.71  1994/11/07  11:47:19  allender
 * when interpolating frames, delete weapon, fireball, and debris objects
 * from an inpolated frame if they don't appear in the next recorded
 * frame
 *
 * Revision 1.70  1994/11/07  11:02:41  allender
 * more with interpolation. I believe that I have it right now
 *
 * Revision 1.69  1994/11/07  08:47:40  john
 * Made wall state record.
 *
 * Revision 1.68  1994/11/05  17:22:51  john
 * Fixed lots of sequencing problems with newdemo stuff.
 *
 * Revision 1.67  1994/11/04  20:11:52  john
 * Neatening up palette stuff with demos.
 *
 * Revision 1.66  1994/11/04  16:49:44  allender
 * changed newdemo_do_interpolate to default to on
 *
 * Revision 1.65  1994/11/04  16:44:51  allender
 * added filename support for demo recording.  more auto demo stuff
 *
 * Revision 1.64  1994/11/04  13:05:31  allender
 * fixing the lifeleft variable again.  (I think I got it right this time)
 *
 * Revision 1.63  1994/11/04  11:37:37  allender
 * commented out fprintfs and fixed compiler warning
 *
 * Revision 1.62  1994/11/04  11:33:50  allender
 * added OBJ_FLARE and OBJ_LIGHT to obj->lifeleft recording
 *
 * Revision 1.61  1994/11/04  11:29:21  allender
 * more interpolation stuff -- not done yet.  Fixed so hostage vclips
 * render correctly.  Changed lifeleft to full precision, but only
 * write it when object is fireball or weapon type of object
 *
 * Revision 1.60  1994/11/03  10:00:11  allender
 * fixed divide by zero in calculating render time.  more interpolation
 * stuff which isn't quite done
 *
 * Revision 1.59  1994/11/02  17:10:59  allender
 * never play recorded frames when interpolation is occuring
 *
 * Revision 1.58  1994/11/02  14:28:58  allender
 * profile total playback time and average frame render time
 *
 * Revision 1.57  1994/11/02  14:09:03  allender
 * record rear view.  start of playback interpolation code -- this
 * is not yet done
 *
 * Revision 1.56  1994/11/01  13:25:30  allender
 * drop frames if playing back demo on slower machine
 *
 * Revision 1.55  1994/10/31  16:10:40  allender
 * record letterbox mode on death seq, and then restore
 *
 * Revision 1.54  1994/10/29  16:01:38  allender
 * added ND_STATE_NODEMOS to indicate that there are no demos currently
 * available for playback
 *
 * Revision 1.53  1994/10/29  15:38:42  allender
 * in newdemo_start_playback, make Newdemo_at_eof = 0
 *
 * Revision 1.52  1994/10/28  14:45:28  john
 * fixed typo from last checkin.
 *
 * Revision 1.51  1994/10/28  14:42:55  john
 * Added sound volumes to all sound calls.
 *
 * Revision 1.50  1994/10/28  14:31:57  allender
 * homing missle and autodemo stuff
 *
 * Revision 1.49  1994/10/28  12:42:14  allender
 * record homing distance
 *
 * Revision 1.48  1994/10/27  16:57:54  allender
 * changed demo vcr to be able to play any number of frames by storing
 * frame length (in bytes) in the demo file.  Added blowing up monitors
 *
 * Revision 1.47  1994/10/26  16:50:50  allender
 * put two functions inside of VCR_MODE ifdef
 *
 * Revision 1.46  1994/10/26  15:20:32  allender
 * added CT_REMOTE as valid control type for recording
 *
 * Revision 1.45  1994/10/26  14:45:35  allender
 * completed hacked in vcr demo playback stuff
 *
 * Revision 1.44  1994/10/26  13:40:52  allender
 * vcr playback of demo stuff
 *
 * Revision 1.43  1994/10/26  08:51:57  allender
 * record player weapon change
 *
 * Revision 1.42  1994/10/25  15:48:01  allender
 * add shields, energy, and player flags to demo recording.
 * ,
 *
 * Revision 1.41  1994/10/24  08:19:35  allender
 * fixed compilation errors
 *
 * Revision 1.40  1994/10/23  19:17:08  matt
 * Fixed bug with "no key" messages
 *
 * Revision 1.39  1994/10/22  14:15:08  mike
 * Suppress compiler warnings.
 *
 * Revision 1.38  1994/10/21  15:24:55  allender
 * compressed writing of object structures with specialized code
 * to write out only pertinent object structures.
 *
 * Revision 1.37  1994/10/20  13:03:17  matt
 * Replaced old save files (MIN/SAV/HOT) with new LVL files
 *
 * Revision 1.36  1994/09/28  23:13:10  matt
 * Macroized palette flash system
 *
 * Revision 1.35  1994/09/26  17:28:32  matt
 * Made new multiple-object morph code work with the demo system
 *
 * Revision 1.34  1994/09/10  13:31:54  matt
 * Made exploding walls a type of blastable walls.
 * Cleaned up blastable walls, making them tmap2 bitmaps.
 *
 * Revision 1.33  1994/08/15  18:05:28  john
 * *** empty log message ***
 *
 * Revision 1.32  1994/08/15  17:56:38  john
 * ,
 *
 * Revision 1.31  1994/08/10  09:44:54  john
 * *** empty log message ***
 *
 * Revision 1.30  1994/07/22  12:35:48  matt
 * Cleaned up editor/game interactions some more.
 *
 * Revision 1.29  1994/07/21  13:06:45  matt
 * Ripped out remants of old demo system, and added demo only system that
 * disables object movement and game options from menu.
 *
 * Revision 1.28  1994/07/18  16:22:44  john
 * Made all file read/writes call the same routine.
 *
 * Revision 1.27  1994/07/14  22:38:27  matt
 * Added exploding doors
 *
 * Revision 1.26  1994/07/05  12:49:04  john
 * Put functionality of New Hostage spec into code.
 *
 * Revision 1.25  1994/06/29  11:05:38  john
 * Made demos read in compressed.
 *
 * Revision 1.24  1994/06/29  09:14:06  john
 * Made files write out uncompressed and read in compressed.
 *
 * Revision 1.23  1994/06/28  11:55:28  john
 * Made newdemo system record/play directly to/from disk, so
 * we don't need the 4 MB buffer anymore.
 *
 * Revision 1.22  1994/06/27  15:52:38  john
 * #define'd out the newdemo stuff
 *
 *
 * Revision 1.21  1994/06/22  00:29:04  john
 * Fixed bug with playing demo then playing game without
 * loading new mine.
 *
 * Revision 1.20  1994/06/22  00:14:23  john
 * Attempted to fix sign.
 *
 * Revision 1.19  1994/06/21  23:57:54  john
 * Hopefully fixed bug with negative countdowns.
 *
 * Revision 1.18  1994/06/21  23:47:44  john
 * MAde Malloc always 4*1024*1024.
 *
 * Revision 1.17  1994/06/21  22:58:47  john
 * Added error if out of memory.
 *
 * Revision 1.16  1994/06/21  22:15:48  john
 * Added  % done to demo recording.
 *
 *
 * Revision 1.15  1994/06/21  19:45:55  john
 * Added palette effects to demo recording.
 *
 * Revision 1.14  1994/06/21  15:08:54  john
 * Made demo record HUD message and cleaned up the HUD code.
 *
 * Revision 1.13  1994/06/21  14:20:08  john
 * Put in hooks to record HUD messages.
 *
 * Revision 1.12  1994/06/20  11:50:15  john
 * Made demo record flash effect, and control center triggers.
 *
 * Revision 1.11  1994/06/17  18:01:33  john
 * A bunch of new stuff by John
 *
 * Revision 1.10  1994/06/17  12:13:31  john
 * More newdemo stuff; made editor->game transition start in slew mode.
 *
 * Revision 1.9  1994/06/16  13:14:36  matt
 * Fixed typo
 *
 * Revision 1.8  1994/06/16  13:02:07  john
 * Added morph hooks.
 *
 * Revision 1.7  1994/06/15  19:01:33  john
 * Added the capability to make 3d sounds play just once for the
 * laser hit wall effects.
 *
 * Revision 1.6  1994/06/15  14:56:59  john
 * Added triggers to demo recording.
 *
 * Revision 1.5  1994/06/14  20:42:15  john
 * Made robot matztn cntr not work until no robots or player are
 * in the segment.
 *
 * Revision 1.4  1994/06/14  14:43:27  john
 * Made doors work with newdemo system.
 *
 * Revision 1.3  1994/06/14  11:32:29  john
 * Made Newdemo record & restore the current mine.
 *
 * Revision 1.2  1994/06/13  21:02:43  john
 * Initial version of new demo recording system.
 *
 * Revision 1.1  1994/06/13  11:09:00  john
 * Initial revision
 *
 *
 */


#ifdef HAVE_CONFIG_H
#include <conf.h>
#endif

#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h> // for memset
#include <errno.h>
#include <ctype.h>      /* for isdigit */
#include <limits.h>
#ifdef __unix__
#include <sys/stat.h>
#include <sys/types.h>
#endif

#include "u_mem.h"
#include "inferno.h"
#include "game.h"
#include "gr.h"
#include "stdlib.h"
#include "bm.h"
//#include "error.h"
#include "mono.h"
#include "3d.h"
#include "segment.h"
#include "texmap.h"
#include "laser.h"
#include "key.h"
#include "gameseg.h"

#include "object.h"
#include "physics.h"
#include "slew.h"
#include "render.h"
#include "wall.h"
#include "vclip.h"
#include "polyobj.h"
#include "fireball.h"
#include "laser.h"
#include "error.h"
#include "ai.h"
#include "hostage.h"
#include "morph.h"

#include "powerup.h"
#include "fuelcen.h"

#include "sounds.h"
#include "collide.h"

#include "lighting.h"
#include "newdemo.h"
#include "gameseq.h"
#include "gamesave.h"
#include "gamemine.h"
#include "switch.h"
#include "gauges.h"
#include "player.h"
#include "vecmat.h"
#include "newmenu.h"
#include "args.h"
#include "palette.h"
#include "multi.h"
#ifdef NETWORK
#include "network.h"
#endif
#include "text.h"
#include "cntrlcen.h"
#include "aistruct.h"
#include "mission.h"
#include "piggy.h"
#include "controls.h"
#include "d_io.h"
#include "timer.h"

#include "findfile.h"

#ifdef EDITOR
#include "editor/editor.h"
#endif

#ifdef MACINTOSH
#pragma global_optimizer off        // pretty much sucks...need to look into this
#endif

void DoJasonInterpolate (fix recorded_time);

//#include "nocfile.h"

//Does demo start automatically?
int Auto_demo = 0;

byte WasRecorded [MAX_OBJECTS];
byte ViewWasRecorded[MAX_OBJECTS];
byte RenderingWasRecorded[32];

#define ND_EVENT_EOF                0   // EOF
#define ND_EVENT_START_DEMO         1   // Followed by 16 character, NULL terminated filename of .SAV file to use
#define ND_EVENT_START_FRAME        2   // Followed by integer frame number, then a fix FrameTime
#define ND_EVENT_VIEWER_OBJECT      3   // Followed by an object structure
#define ND_EVENT_RENDER_OBJECT      4   // Followed by an object structure
#define ND_EVENT_SOUND              5   // Followed by int soundum
#define ND_EVENT_SOUND_ONCE         6   // Followed by int soundum
#define ND_EVENT_SOUND_3D           7   // Followed by int soundum, int angle, int volume
#define ND_EVENT_WALL_HIT_PROCESS   8   // Followed by int segnum, int side, fix damage
#define ND_EVENT_TRIGGER            9   // Followed by int segnum, int side, int objnum
#define ND_EVENT_HOSTAGE_RESCUED    10  // Followed by int hostage_type
#define ND_EVENT_SOUND_3D_ONCE      11  // Followed by int soundum, int angle, int volume
#define ND_EVENT_MORPH_FRAME        12  // Followed by ? data
#define ND_EVENT_WALL_TOGGLE        13  // Followed by int seg, int side
#define ND_EVENT_HUD_MESSAGE        14  // Followed by char size, char * string (+null)
#define ND_EVENT_CONTROL_CENTER_DESTROYED 15 // Just a simple flag
#define ND_EVENT_PALETTE_EFFECT     16  // Followed by short r,g,b
#define ND_EVENT_PLAYER_ENERGY      17  // followed by byte energy
#define ND_EVENT_PLAYER_SHIELD      18  // followed by byte shields
#define ND_EVENT_PLAYER_FLAGS       19  // followed by player flags
#define ND_EVENT_PLAYER_WEAPON      20  // followed by weapon type and weapon number
#define ND_EVENT_EFFECT_BLOWUP      21  // followed by segment, side, and pnt
#define ND_EVENT_HOMING_DISTANCE    22  // followed by homing distance
#define ND_EVENT_LETTERBOX          23  // letterbox mode for death seq.
#define ND_EVENT_RESTORE_COCKPIT    24  // restore cockpit after death
#define ND_EVENT_REARVIEW           25  // going to rear view mode
#define ND_EVENT_WALL_SET_TMAP_NUM1 26  // Wall changed
#define ND_EVENT_WALL_SET_TMAP_NUM2 27  // Wall changed
#define ND_EVENT_NEW_LEVEL          28  // followed by level number
#define ND_EVENT_MULTI_CLOAK        29  // followed by player num
#define ND_EVENT_MULTI_DECLOAK      30  // followed by player num
#define ND_EVENT_RESTORE_REARVIEW   31  // restore cockpit after rearview mode
#define ND_EVENT_MULTI_DEATH        32  // with player number
#define ND_EVENT_MULTI_KILL         33  // with player number
#define ND_EVENT_MULTI_CONNECT      34  // with player number
#define ND_EVENT_MULTI_RECONNECT    35  // with player number
#define ND_EVENT_MULTI_DISCONNECT   36  // with player number
#define ND_EVENT_MULTI_SCORE        37  // playernum / score
#define ND_EVENT_PLAYER_SCORE       38  // followed by score
#define ND_EVENT_PRIMARY_AMMO       39  // with old/new ammo count
#define ND_EVENT_SECONDARY_AMMO     40  // with old/new ammo count
#define ND_EVENT_DOOR_OPENING       41  // with segment/side
#define ND_EVENT_LASER_LEVEL        42  // no data
#define ND_EVENT_PLAYER_AFTERBURNER 43  // followed by byte old ab, current ab
#define ND_EVENT_CLOAKING_WALL      44  // info changing while wall cloaking
#define ND_EVENT_CHANGE_COCKPIT     45  // change the cockpit
#define ND_EVENT_START_GUIDED       46  // switch to guided view
#define ND_EVENT_END_GUIDED         47  // stop guided view/return to ship
#define ND_EVENT_SECRET_THINGY      48  // 0/1 = secret exit functional/non-functional
#define ND_EVENT_LINK_SOUND_TO_OBJ  49  // record digi_link_sound_to_object3
#define ND_EVENT_KILL_SOUND_TO_OBJ  50  // record digi_kill_sound_linked_to_object


#define NORMAL_PLAYBACK         0
#define SKIP_PLAYBACK           1
#define INTERPOLATE_PLAYBACK    2
#define INTERPOL_FACTOR         (F1_0 + (F1_0/5))

#define DEMO_VERSION            15      // last D1 version was 13
#define DEMO_GAME_TYPE          3       // 1 was shareware, 2 registered

#define DEMO_FILENAME           DEMO_DIR "tmpdemo.dem"

#define DEMO_MAX_LEVELS         29


char nd_save_callsign[CALLSIGN_LEN+1];
int Newdemo_state = 0;
int Newdemo_vcr_state = 0;
int Newdemo_start_frame = -1;
unsigned int Newdemo_size;
int Newdemo_num_written;
int Newdemo_game_mode;
int Newdemo_old_cockpit;
byte Newdemo_no_space;
byte Newdemo_at_eof;
byte Newdemo_do_interpolate = 0; // 1
byte Newdemo_players_cloaked;
byte Newdemo_warning_given = 0;
byte Newdemo_cntrlcen_destroyed = 0;
static byte nd_bad_read;
int NewdemoFrameCount;
short frame_bytes_written = 0;
fix nd_playback_total;
fix nd_recorded_total;
fix nd_recorded_time;
byte playback_style;
byte First_time_playback=1;
fix JasonPlaybackTotal=0;


FILE *infile;
FILE *outfile=NULL;

int newdemo_get_percent_done() {
	if ( Newdemo_state == ND_STATE_PLAYBACK ) {
		return (ftell(infile)*100)/Newdemo_size;
	}
	if ( Newdemo_state == ND_STATE_RECORDING ) {
		return ftell(outfile);
	}
	return 0;
}

#define VEL_PRECISION 12

void my_extract_shortpos(object *objp, shortpos *spp)
{
	int segnum;
	byte *sp;

	sp = spp->bytemat;
	objp->orient.rvec.x = *sp++ << MATRIX_PRECISION;
	objp->orient.uvec.x = *sp++ << MATRIX_PRECISION;
	objp->orient.fvec.x = *sp++ << MATRIX_PRECISION;

	objp->orient.rvec.y = *sp++ << MATRIX_PRECISION;
	objp->orient.uvec.y = *sp++ << MATRIX_PRECISION;
	objp->orient.fvec.y = *sp++ << MATRIX_PRECISION;

	objp->orient.rvec.z = *sp++ << MATRIX_PRECISION;
	objp->orient.uvec.z = *sp++ << MATRIX_PRECISION;
	objp->orient.fvec.z = *sp++ << MATRIX_PRECISION;

	segnum = spp->segment;
	objp->segnum = segnum;

	objp->pos.x = (spp->xo << RELPOS_PRECISION) + Vertices[Segments[segnum].verts[0]].x;
	objp->pos.y = (spp->yo << RELPOS_PRECISION) + Vertices[Segments[segnum].verts[0]].y;
	objp->pos.z = (spp->zo << RELPOS_PRECISION) + Vertices[Segments[segnum].verts[0]].z;

	objp->mtype.phys_info.velocity.x = (spp->velx << VEL_PRECISION);
	objp->mtype.phys_info.velocity.y = (spp->vely << VEL_PRECISION);
	objp->mtype.phys_info.velocity.z = (spp->velz << VEL_PRECISION);
}

int newdemo_read( void *buffer, int elsize, int nelem )
{
	int num_read;
	num_read = fread( buffer,elsize,nelem, infile );
	if (ferror(infile) || feof(infile))
		nd_bad_read = -1;

	return num_read;
}

int newdemo_find_object( int signature )
{
	int i;
	object * objp;
	objp = Objects;
	for (i=0; i<=Highest_object_index; i++, objp++ ) {
		if ( (objp->type != OBJ_NONE) && (objp->signature == signature))
			return i;
	}
	return -1;
}

int newdemo_write( void *buffer, int elsize, int nelem )
{
	int num_written, total_size;

	total_size = elsize * nelem;
	frame_bytes_written += total_size;
	Newdemo_num_written += total_size;
	Assert(outfile != NULL);
	num_written = fwrite( buffer, elsize, nelem, outfile );
	//if ((Newdemo_num_written > Newdemo_size) && !Newdemo_no_space) {
	//	Newdemo_no_space=1;
	//	newdemo_stop_recording();
	//	return -1;
	//}
	if ((Newdemo_num_written > Newdemo_size) && !Newdemo_no_space)
		Newdemo_no_space=1;
	if (num_written == nelem && !Newdemo_no_space)
		return num_written;

	Newdemo_no_space=2;
	newdemo_stop_recording();
	return -1;
}

/*
 *  The next bunch of files taken from Matt's gamesave.c.  We have to modify
 *  these since the demo must save more information about objects that
 *  just a gamesave
*/

static void nd_write_byte(byte b)
{
	newdemo_write(&b, 1, 1);
}

static void nd_write_short(short s)
{
	newdemo_write(&s, 2, 1);
}

static void nd_write_int(int i)
{
	newdemo_write(&i, 4, 1);
}

static void nd_write_string(char *str)
{
	nd_write_byte(strlen(str) + 1);
	newdemo_write(str, strlen(str) + 1, 1);
}

static void nd_write_fix(fix f)
{
	newdemo_write(&f, sizeof(fix), 1);
}

static void nd_write_fixang(fixang f)
{
	newdemo_write(&f, sizeof(fixang), 1);
}

static void nd_write_vector(vms_vector *v)
{
	nd_write_fix(v->x);
	nd_write_fix(v->y);
	nd_write_fix(v->z);
}

static void nd_write_angvec(vms_angvec *v)
{
	nd_write_fixang(v->p);
	nd_write_fixang(v->b);
	nd_write_fixang(v->h);
}

void nd_write_shortpos(object *obj)
{
	int i;
	shortpos sp;
	ubyte render_type;

	create_shortpos(&sp, obj, 0);

	render_type = obj->render_type;
	if (((render_type == RT_POLYOBJ) || (render_type == RT_HOSTAGE) || (render_type == RT_MORPH)) || (obj->type == OBJ_CAMERA)) {
		for (i = 0; i < 9; i++)
			nd_write_byte(sp.bytemat[i]);
		for (i = 0; i < 9; i++) {
			if (sp.bytemat[i] != 0)
				break;
		}
		if (i == 9) {
			Int3();         // contact Allender about this.
		}
	}

	nd_write_short(sp.xo);
	nd_write_short(sp.yo);
	nd_write_short(sp.zo);
	nd_write_short(sp.segment);
	nd_write_short(sp.velx);
	nd_write_short(sp.vely);
	nd_write_short(sp.velz);
}

static void nd_read_byte(byte *b)
{
	newdemo_read(b, 1, 1);
}

static void nd_read_short(short *s)
{
	newdemo_read(s, 2, 1);
}

static void nd_read_int(int *i)
{
	newdemo_read(i, 4, 1);
}

static void nd_read_string(char *str)
{
	byte len;

	nd_read_byte(&len);
	newdemo_read(str, len, 1);
}

static void nd_read_fix(fix *f)
{
	newdemo_read(f, sizeof(fix), 1);
}

static void nd_read_fixang(fixang *f)
{
	newdemo_read(f, sizeof(fixang), 1);
}

static void nd_read_vector(vms_vector *v)
{
	nd_read_fix(&(v->x));
	nd_read_fix(&(v->y));
	nd_read_fix(&(v->z));
}

static void nd_read_angvec(vms_angvec *v)
{
	nd_read_fixang(&(v->p));
	nd_read_fixang(&(v->b));
	nd_read_fixang(&(v->h));
}

static void nd_read_shortpos(object *obj)
{
	shortpos sp;
	int i;
	ubyte render_type;

	render_type = obj->render_type;
	if (((render_type == RT_POLYOBJ) || (render_type == RT_HOSTAGE) || (render_type == RT_MORPH)) || (obj->type == OBJ_CAMERA)) {
		for (i = 0; i < 9; i++)
			nd_read_byte(&(sp.bytemat[i]));
	}

	nd_read_short(&(sp.xo));
	nd_read_short(&(sp.yo));
	nd_read_short(&(sp.zo));
	nd_read_short(&(sp.segment));
	nd_read_short(&(sp.velx));
	nd_read_short(&(sp.vely));
	nd_read_short(&(sp.velz));

	my_extract_shortpos(obj, &sp);
	if ((obj->id == VCLIP_MORPHING_ROBOT) && (render_type == RT_FIREBALL) && (obj->control_type == CT_EXPLOSION))
		extract_orient_from_segment(&obj->orient,&Segments[obj->segnum]);

}

object *prev_obj=NULL;      //ptr to last object read in

void nd_read_object(object *obj)
{
	memset(obj, 0, sizeof(object));

	/*
	 * Do render type first, since with render_type == RT_NONE, we
	 * blow by all other object information
	 */
	nd_read_byte(&(obj->render_type));
	nd_read_byte(&(obj->type));
	if ((obj->render_type == RT_NONE) && (obj->type != OBJ_CAMERA))
		return;

	nd_read_byte(&(obj->id));
	nd_read_byte(&(obj->flags));
	nd_read_short((short *)&(obj->signature));
	nd_read_shortpos(obj);

	if ((obj->type == OBJ_ROBOT) && (obj->id == SPECIAL_REACTOR_ROBOT))
		Int3();

	obj->attached_obj = -1;

	switch(obj->type) {

	case OBJ_HOSTAGE:
		obj->control_type = CT_POWERUP;
		obj->movement_type = MT_NONE;
		obj->size = HOSTAGE_SIZE;
		break;

	case OBJ_ROBOT:
		obj->control_type = CT_AI;
		// (MarkA and MikeK said we should not do the crazy last secret stuff with multiple reactors...
		// This necessary code is our vindication. --MK, 2/15/96)
		if (obj->id != SPECIAL_REACTOR_ROBOT)
			obj->movement_type = MT_PHYSICS;
		else
			obj->movement_type = MT_NONE;
		obj->size = Polygon_models[Robot_info[obj->id].model_num].rad;
		obj->rtype.pobj_info.model_num = Robot_info[obj->id].model_num;
		obj->rtype.pobj_info.subobj_flags = 0;
		obj->ctype.ai_info.CLOAKED = (Robot_info[obj->id].cloak_type?1:0);
		break;

	case OBJ_POWERUP:
		obj->control_type = CT_POWERUP;
		nd_read_byte(&(obj->movement_type));        // might have physics movement
		obj->size = Powerup_info[obj->id].size;
		break;

	case OBJ_PLAYER:
		obj->control_type = CT_NONE;
		obj->movement_type = MT_PHYSICS;
		obj->size = Polygon_models[Player_ship->model_num].rad;
		obj->rtype.pobj_info.model_num = Player_ship->model_num;
		obj->rtype.pobj_info.subobj_flags = 0;
		break;

	case OBJ_CLUTTER:
		obj->control_type = CT_NONE;
		obj->movement_type = MT_NONE;
		obj->size = Polygon_models[obj->id].rad;
		obj->rtype.pobj_info.model_num = obj->id;
		obj->rtype.pobj_info.subobj_flags = 0;
		break;

	default:
		nd_read_byte(&(obj->control_type));
		nd_read_byte(&(obj->movement_type));
		nd_read_fix(&(obj->size));
		break;
	}


	nd_read_vector(&(obj->last_pos));
	if ((obj->type == OBJ_WEAPON) && (obj->render_type == RT_WEAPON_VCLIP))
		nd_read_fix(&(obj->lifeleft));
	else {
		ubyte b;

		nd_read_byte(&b);
		obj->lifeleft = (fix)b;
		// MWA old way -- won't work with big endian machines       nd_read_byte((ubyte *)&(obj->lifeleft));
		obj->lifeleft = (fix)((int)obj->lifeleft << 12);
	}

	if (obj->type == OBJ_ROBOT) {
		if (Robot_info[obj->id].boss_flag) {
			byte cloaked;

			nd_read_byte(&cloaked);
			obj->ctype.ai_info.CLOAKED = cloaked;
		}
	}

	switch (obj->movement_type) {

	case MT_PHYSICS:
		nd_read_vector(&(obj->mtype.phys_info.velocity));
		nd_read_vector(&(obj->mtype.phys_info.thrust));
		break;

	case MT_SPINNING:
		nd_read_vector(&(obj->mtype.spin_rate));
		break;

	case MT_NONE:
		break;

	default:
		Int3();
	}

	switch (obj->control_type) {

	case CT_EXPLOSION:

		nd_read_fix(&(obj->ctype.expl_info.spawn_time));
		nd_read_fix(&(obj->ctype.expl_info.delete_time));
		nd_read_short(&(obj->ctype.expl_info.delete_objnum));

		obj->ctype.expl_info.next_attach = obj->ctype.expl_info.prev_attach = obj->ctype.expl_info.attach_parent = -1;

		if (obj->flags & OF_ATTACHED) {     //attach to previous object
			Assert(prev_obj!=NULL);
			if (prev_obj->control_type == CT_EXPLOSION) {
				if (prev_obj->flags & OF_ATTACHED && prev_obj->ctype.expl_info.attach_parent!=-1)
					obj_attach(&Objects[prev_obj->ctype.expl_info.attach_parent],obj);
				else
					obj->flags &= ~OF_ATTACHED;
			}
			else
				obj_attach(prev_obj,obj);
		}

		break;

	case CT_LIGHT:
		nd_read_fix(&(obj->ctype.light_info.intensity));
		break;

	case CT_AI:
	case CT_WEAPON:
	case CT_NONE:
	case CT_FLYING:
	case CT_DEBRIS:
	case CT_POWERUP:
	case CT_SLEW:
	case CT_CNTRLCEN:
	case CT_REMOTE:
	case CT_MORPH:
		break;

	case CT_FLYTHROUGH:
	case CT_REPAIRCEN:
	default:
		Int3();

	}

	switch (obj->render_type) {

	case RT_NONE:
		break;

	case RT_MORPH:
	case RT_POLYOBJ: {
		int i, tmo;

		if ((obj->type != OBJ_ROBOT) && (obj->type != OBJ_PLAYER) && (obj->type != OBJ_CLUTTER)) {
			nd_read_int(&(obj->rtype.pobj_info.model_num));
			nd_read_int(&(obj->rtype.pobj_info.subobj_flags));
		}

		if ((obj->type != OBJ_PLAYER) && (obj->type != OBJ_DEBRIS))
#if 0
			for (i=0;i<MAX_SUBMODELS;i++)
				nd_read_angvec(&(obj->pobj_info.anim_angles[i]));
#endif
		for (i = 0; i < Polygon_models[obj->rtype.pobj_info.model_num].n_models; i++)
			nd_read_angvec(&obj->rtype.pobj_info.anim_angles[i]);

		nd_read_int(&tmo);

#ifndef EDITOR
		obj->rtype.pobj_info.tmap_override = tmo;
#else
		if (tmo==-1)
			obj->rtype.pobj_info.tmap_override = -1;
		else {
			int xlated_tmo = tmap_xlate_table[tmo];
			if (xlated_tmo < 0) {
				//mprintf( (0, "Couldn't find texture for demo object, model_num = %d\n", obj->pobj_info.model_num));
				Int3();
				xlated_tmo = 0;
			}
			obj->rtype.pobj_info.tmap_override = xlated_tmo;
		}
#endif

		break;
	}

	case RT_POWERUP:
	case RT_WEAPON_VCLIP:
	case RT_FIREBALL:
	case RT_HOSTAGE:
		nd_read_int(&(obj->rtype.vclip_info.vclip_num));
		nd_read_fix(&(obj->rtype.vclip_info.frametime));
		nd_read_byte(&(obj->rtype.vclip_info.framenum));
		break;

	case RT_LASER:
		break;

	default:
		Int3();

	}

	prev_obj = obj;
}

void nd_write_object(object *obj)
{
	int life;

	if ((obj->type == OBJ_ROBOT) && (obj->id == SPECIAL_REACTOR_ROBOT))
		Int3();

	/*
	 * Do render_type first so on read, we can make determination of
	 * what else to read in
	 */
	nd_write_byte(obj->render_type);
	nd_write_byte(obj->type);
	if ((obj->render_type == RT_NONE) && (obj->type != OBJ_CAMERA))
		return;

	nd_write_byte(obj->id);
	nd_write_byte(obj->flags);
	nd_write_short((short)obj->signature);
	nd_write_shortpos(obj);

	if ((obj->type != OBJ_HOSTAGE) && (obj->type != OBJ_ROBOT) && (obj->type != OBJ_PLAYER) && (obj->type != OBJ_POWERUP) && (obj->type != OBJ_CLUTTER)) {
		nd_write_byte(obj->control_type);
		nd_write_byte(obj->movement_type);
		nd_write_fix(obj->size);
	}
	if (obj->type == OBJ_POWERUP)
		nd_write_byte(obj->movement_type);

	nd_write_vector(&obj->last_pos);

	if ((obj->type == OBJ_WEAPON) && (obj->render_type == RT_WEAPON_VCLIP))
		nd_write_fix(obj->lifeleft);
	else {
		life = (int)obj->lifeleft;
		life = life >> 12;
		if (life > 255)
			life = 255;
		nd_write_byte((ubyte)life);
	}

	if (obj->type == OBJ_ROBOT) {
		if (Robot_info[obj->id].boss_flag) {
			if ((GameTime > Boss_cloak_start_time) && (GameTime < Boss_cloak_end_time))
				nd_write_byte(1);
			else
				nd_write_byte(0);
		}
	}

	switch (obj->movement_type) {

	case MT_PHYSICS:
		nd_write_vector(&obj->mtype.phys_info.velocity);
		nd_write_vector(&obj->mtype.phys_info.thrust);
		break;

	case MT_SPINNING:
		nd_write_vector(&obj->mtype.spin_rate);
		break;

	case MT_NONE:
		break;

	default:
		Int3();
	}

	switch (obj->control_type) {

	case CT_AI:
		break;

	case CT_EXPLOSION:
		nd_write_fix(obj->ctype.expl_info.spawn_time);
		nd_write_fix(obj->ctype.expl_info.delete_time);
		nd_write_short(obj->ctype.expl_info.delete_objnum);
		break;

	case CT_WEAPON:
		break;

	case CT_LIGHT:

		nd_write_fix(obj->ctype.light_info.intensity);
		break;

	case CT_NONE:
	case CT_FLYING:
	case CT_DEBRIS:
	case CT_POWERUP:
	case CT_SLEW:       //the player is generally saved as slew
	case CT_CNTRLCEN:
	case CT_REMOTE:
	case CT_MORPH:
		break;

	case CT_REPAIRCEN:
	case CT_FLYTHROUGH:
	default:
		Int3();

	}

	switch (obj->render_type) {

	case RT_NONE:
		break;

	case RT_MORPH:
	case RT_POLYOBJ: {
		int i;

		if ((obj->type != OBJ_ROBOT) && (obj->type != OBJ_PLAYER) && (obj->type != OBJ_CLUTTER)) {
			nd_write_int(obj->rtype.pobj_info.model_num);
			nd_write_int(obj->rtype.pobj_info.subobj_flags);
		}

		if ((obj->type != OBJ_PLAYER) && (obj->type != OBJ_DEBRIS))
#if 0
			for (i=0;i<MAX_SUBMODELS;i++)
				nd_write_angvec(&obj->pobj_info.anim_angles[i]);
#endif
		for (i = 0; i < Polygon_models[obj->rtype.pobj_info.model_num].n_models; i++)
			nd_write_angvec(&obj->rtype.pobj_info.anim_angles[i]);

		nd_write_int(obj->rtype.pobj_info.tmap_override);

		break;
	}

	case RT_POWERUP:
	case RT_WEAPON_VCLIP:
	case RT_FIREBALL:
	case RT_HOSTAGE:
		nd_write_int(obj->rtype.vclip_info.vclip_num);
		nd_write_fix(obj->rtype.vclip_info.frametime);
		nd_write_byte(obj->rtype.vclip_info.framenum);
		break;

	case RT_LASER:
		break;

	default:
		Int3();

	}

}

int JustStartedRecording=0,JustStartedPlayback=0;

void newdemo_record_start_demo()
{
	int i;

	stop_time();
	nd_write_byte(ND_EVENT_START_DEMO);
	nd_write_byte(DEMO_VERSION);
	nd_write_byte(DEMO_GAME_TYPE);
	nd_write_fix(GameTime);

#ifdef NETWORK
	if (Game_mode & GM_MULTI)
		nd_write_int(Game_mode | (Player_num << 16));
	else
#endif
		// NOTE LINK TO ABOVE!!!
		nd_write_int(Game_mode);
#ifdef NETWORK

	if (Game_mode & GM_TEAM) {
		nd_write_byte(Netgame.team_vector);
		nd_write_string(Netgame.team_name[0]);
		nd_write_string(Netgame.team_name[1]);
	}

	if (Game_mode & GM_MULTI) {
		nd_write_byte((byte)N_players);
		for (i = 0; i < N_players; i++) {
			nd_write_string(Players[i].callsign);
			nd_write_byte(Players[i].connected);

			if (Game_mode & GM_MULTI_COOP) {
				nd_write_int(Players[i].score);
			} else {
				nd_write_short((short)Players[i].net_killed_total);
				nd_write_short((short)Players[i].net_kills_total);
			}
		}
	} else
#endif
		// NOTE LINK TO ABOVE!!!
		nd_write_int(Players[Player_num].score);

	for (i = 0; i < MAX_PRIMARY_WEAPONS; i++)
		nd_write_short((short)Players[Player_num].primary_ammo[i]);

	for (i = 0; i < MAX_SECONDARY_WEAPONS; i++)
		nd_write_short((short)Players[Player_num].secondary_ammo[i]);

	nd_write_byte((byte)Players[Player_num].laser_level);

//  Support for missions added here

	nd_write_string(Current_mission_filename);

	nd_write_byte((byte)(f2ir(Players[Player_num].energy)));
	nd_write_byte((byte)(f2ir(Players[Player_num].shields)));
	nd_write_int(Players[Player_num].flags);        // be sure players flags are set
	nd_write_byte((byte)Primary_weapon);
	nd_write_byte((byte)Secondary_weapon);
	Newdemo_start_frame = FrameCount;
	JustStartedRecording=1;

	newdemo_set_new_level(Current_level_num);
	start_time();

}

void newdemo_record_start_frame(int frame_number, fix frame_time )
{
	int i;

	if (Newdemo_no_space) {
		newdemo_stop_playback();
		return;
	}

	stop_time();

	for (i=0;i<MAX_OBJECTS;i++)
	{
		WasRecorded[i]=0;
		ViewWasRecorded[i]=0;
	}
	for (i=0;i<32;i++)
		RenderingWasRecorded[i]=0;

	frame_number -= Newdemo_start_frame;

	Assert(frame_number >= 0 );

	nd_write_byte(ND_EVENT_START_FRAME);
	nd_write_short(frame_bytes_written - 1);        // from previous frame
	frame_bytes_written=3;
	nd_write_int(frame_number);
	nd_write_int(frame_time);
	start_time();

}

void newdemo_record_render_object(object * obj)
{
	if (ViewWasRecorded[obj-Objects])
		return;

	//if (obj==&Objects[Players[Player_num].objnum] && !Player_is_dead)
	//	return;

	stop_time();
	nd_write_byte(ND_EVENT_RENDER_OBJECT);
	nd_write_object(obj);
	start_time();
}

extern ubyte RenderingType;

void newdemo_record_viewer_object(object * obj)
{

	if (ViewWasRecorded[obj-Objects] && (ViewWasRecorded[obj-Objects]-1)==RenderingType)
		return;
	//if (WasRecorded[obj-Objects])
	//	return;
	if (RenderingWasRecorded[RenderingType])
		return;

	ViewWasRecorded[obj-Objects]=RenderingType+1;
	RenderingWasRecorded[RenderingType]=1;
	stop_time();
	nd_write_byte(ND_EVENT_VIEWER_OBJECT);
	nd_write_byte(RenderingType);
	nd_write_object(obj);
	start_time();
}

void newdemo_record_sound( int soundno )
{
	stop_time();
	nd_write_byte(ND_EVENT_SOUND);
	nd_write_int( soundno );
	start_time();
}

//--unused-- void newdemo_record_sound_once( int soundno ) {
//--unused-- 	stop_time();
//--unused-- 	nd_write_byte( ND_EVENT_SOUND_ONCE );
//--unused-- 	nd_write_int( soundno );
//--unused-- 	start_time();
//--unused-- }
//--unused--

void newdemo_record_cockpit_change (int mode)
{
	stop_time();
	nd_write_byte (ND_EVENT_CHANGE_COCKPIT);
	nd_write_int(mode);
	start_time();
}


void newdemo_record_sound_3d( int soundno, int angle, int volume )
{
	stop_time();
	nd_write_byte( ND_EVENT_SOUND_3D );
	nd_write_int( soundno );
	nd_write_int( angle );
	nd_write_int( volume );
	start_time();
}

void newdemo_record_sound_3d_once( int soundno, int angle, int volume )
{
	stop_time();
	nd_write_byte( ND_EVENT_SOUND_3D_ONCE );
	nd_write_int( soundno );
	nd_write_int( angle );
	nd_write_int( volume );
	start_time();
}


void newdemo_record_link_sound_to_object3( int soundno, short objnum, fix max_volume, fix  max_distance, int loop_start, int loop_end )
{
	stop_time();
	nd_write_byte( ND_EVENT_LINK_SOUND_TO_OBJ );
	nd_write_int( soundno );
	nd_write_int( Objects[objnum].signature );
	nd_write_int( max_volume );
	nd_write_int( max_distance );
	nd_write_int( loop_start );
	nd_write_int( loop_end );
	start_time();
}

void newdemo_record_kill_sound_linked_to_object( int objnum )
{
	stop_time();
	nd_write_byte( ND_EVENT_KILL_SOUND_TO_OBJ );
	nd_write_int( Objects[objnum].signature );
	start_time();
}


void newdemo_record_wall_hit_process( int segnum, int side, int damage, int playernum )
{
	stop_time();
	//segnum = segnum;
	//side = side;
	//damage = damage;
	//playernum = playernum;
	nd_write_byte( ND_EVENT_WALL_HIT_PROCESS );
	nd_write_int( segnum );
	nd_write_int( side );
	nd_write_int( damage );
	nd_write_int( playernum );
	start_time();
}

void newdemo_record_guided_start ()
{
	nd_write_byte (ND_EVENT_START_GUIDED);
}

void newdemo_record_guided_end ()
{
	nd_write_byte (ND_EVENT_END_GUIDED);
}

void newdemo_record_secret_exit_blown(int truth)
{
	stop_time();
	nd_write_byte( ND_EVENT_SECRET_THINGY );
	nd_write_int( truth );
	start_time();
}

void newdemo_record_trigger( int segnum, int side, int objnum,int shot )
{
	stop_time();
	nd_write_byte( ND_EVENT_TRIGGER );
	nd_write_int( segnum );
	nd_write_int( side );
	nd_write_int( objnum );
	nd_write_int(shot);
	start_time();
}

void newdemo_record_hostage_rescued( int hostage_number ) {
	stop_time();
	nd_write_byte( ND_EVENT_HOSTAGE_RESCUED );
	nd_write_int( hostage_number );
	start_time();
}

void newdemo_record_morph_frame(morph_data *md)
{
	stop_time();

	nd_write_byte( ND_EVENT_MORPH_FRAME );
#if 0
	newdemo_write( md->morph_vecs, sizeof(md->morph_vecs), 1 );
	newdemo_write( md->submodel_active, sizeof(md->submodel_active), 1 );
	newdemo_write( md->submodel_startpoints, sizeof(md->submodel_startpoints), 1 );
#endif
	nd_write_object( md->obj );
	start_time();
}

void newdemo_record_wall_toggle( int segnum, int side )
{
	stop_time();
	nd_write_byte( ND_EVENT_WALL_TOGGLE );
	nd_write_int( segnum );
	nd_write_int( side );
	start_time();
}

void newdemo_record_control_center_destroyed()
{
	stop_time();
	nd_write_byte( ND_EVENT_CONTROL_CENTER_DESTROYED );
	nd_write_int( Countdown_seconds_left );
	start_time();
}

void newdemo_record_hud_message( char * message )
{
	stop_time();
	nd_write_byte( ND_EVENT_HUD_MESSAGE );
	nd_write_string(message);
	start_time();
}

void newdemo_record_palette_effect(short r, short g, short b )
{
	stop_time();
	nd_write_byte( ND_EVENT_PALETTE_EFFECT );
	nd_write_short( r );
	nd_write_short( g );
	nd_write_short( b );
	start_time();
}

void newdemo_record_player_energy(int old_energy, int energy)
{
	stop_time();
	nd_write_byte( ND_EVENT_PLAYER_ENERGY );
	nd_write_byte((byte) old_energy);
	nd_write_byte((byte) energy);
	start_time();
}

void newdemo_record_player_afterburner(fix old_afterburner, fix afterburner)
{
	stop_time();
	nd_write_byte( ND_EVENT_PLAYER_AFTERBURNER );
	nd_write_byte((byte) (old_afterburner>>9));
	nd_write_byte((byte) (afterburner>>9));
	start_time();
}

void newdemo_record_player_shields(int old_shield, int shield)
{
	stop_time();
	nd_write_byte( ND_EVENT_PLAYER_SHIELD );
	nd_write_byte((byte)old_shield);
	nd_write_byte((byte)shield);
	start_time();
}

void newdemo_record_player_flags(uint oflags, uint flags)
{
	stop_time();
	nd_write_byte( ND_EVENT_PLAYER_FLAGS );
	nd_write_int(((short)oflags << 16) | (short)flags);
	start_time();
}

void newdemo_record_player_weapon(int weapon_type, int weapon_num)
{
	stop_time();
	nd_write_byte( ND_EVENT_PLAYER_WEAPON );
	nd_write_byte((byte)weapon_type);
	nd_write_byte((byte)weapon_num);
	if (weapon_type)
		nd_write_byte((byte)Secondary_weapon);
	else
		nd_write_byte((byte)Primary_weapon);
	start_time();
}

void newdemo_record_effect_blowup(short segment, int side, vms_vector *pnt)
{
	stop_time();
	nd_write_byte (ND_EVENT_EFFECT_BLOWUP);
	nd_write_short(segment);
	nd_write_byte((byte)side);
	nd_write_vector(pnt);
	start_time();
}

void newdemo_record_homing_distance(fix distance)
{
	stop_time();
	nd_write_byte(ND_EVENT_HOMING_DISTANCE);
	nd_write_short((short)(distance>>16));
	start_time();
}

void newdemo_record_letterbox(void)
{
	stop_time();
	nd_write_byte(ND_EVENT_LETTERBOX);
	start_time();
}

void newdemo_record_rearview(void)
{
	stop_time();
	nd_write_byte(ND_EVENT_REARVIEW);
	start_time();
}

void newdemo_record_restore_cockpit(void)
{
	stop_time();
	nd_write_byte(ND_EVENT_RESTORE_COCKPIT);
	start_time();
}

void newdemo_record_restore_rearview(void)
{
	stop_time();
	nd_write_byte(ND_EVENT_RESTORE_REARVIEW);
	start_time();
}

void newdemo_record_wall_set_tmap_num1(short seg,ubyte side,short cseg,ubyte cside,short tmap)
{
	stop_time();
	nd_write_byte(ND_EVENT_WALL_SET_TMAP_NUM1);
	nd_write_short(seg);
	nd_write_byte(side);
	nd_write_short(cseg);
	nd_write_byte(cside);
	nd_write_short(tmap);
	start_time();
}

void newdemo_record_wall_set_tmap_num2(short seg,ubyte side,short cseg,ubyte cside,short tmap)
{
	stop_time();
	nd_write_byte(ND_EVENT_WALL_SET_TMAP_NUM2);
	nd_write_short(seg);
	nd_write_byte(side);
	nd_write_short(cseg);
	nd_write_byte(cside);
	nd_write_short(tmap);
	start_time();
}

void newdemo_record_multi_cloak(int pnum)
{
	stop_time();
	nd_write_byte(ND_EVENT_MULTI_CLOAK);
	nd_write_byte((byte)pnum);
	start_time();
}

void newdemo_record_multi_decloak(int pnum)
{
	stop_time();
	nd_write_byte(ND_EVENT_MULTI_DECLOAK);
	nd_write_byte((byte)pnum);
	start_time();
}

void newdemo_record_multi_death(int pnum)
{
	stop_time();
	nd_write_byte(ND_EVENT_MULTI_DEATH);
	nd_write_byte((byte)pnum);
	start_time();
}

void newdemo_record_multi_kill(int pnum, byte kill)
{
	stop_time();
	nd_write_byte(ND_EVENT_MULTI_KILL);
	nd_write_byte((byte)pnum);
	nd_write_byte(kill);
	start_time();
}

void newdemo_record_multi_connect(int pnum, int new_player, char *new_callsign)
{
	stop_time();
	nd_write_byte(ND_EVENT_MULTI_CONNECT);
	nd_write_byte((byte)pnum);
	nd_write_byte((byte)new_player);
	if (!new_player) {
		nd_write_string(Players[pnum].callsign);
		nd_write_int(Players[pnum].net_killed_total);
		nd_write_int(Players[pnum].net_kills_total);
	}
	nd_write_string(new_callsign);
	start_time();
}

void newdemo_record_multi_reconnect(int pnum)
{
	stop_time();
	nd_write_byte(ND_EVENT_MULTI_RECONNECT);
	nd_write_byte((byte)pnum);
	start_time();
}

void newdemo_record_multi_disconnect(int pnum)
{
	stop_time();
	nd_write_byte(ND_EVENT_MULTI_DISCONNECT);
	nd_write_byte((byte)pnum);
	start_time();
}

void newdemo_record_player_score(int score)
{
	stop_time();
	nd_write_byte(ND_EVENT_PLAYER_SCORE);
	nd_write_int(score);
	start_time();
}

void newdemo_record_multi_score(int pnum, int score)
{
	stop_time();
	nd_write_byte(ND_EVENT_MULTI_SCORE);
	nd_write_byte((byte)pnum);
	nd_write_int(score - Players[pnum].score);      // called before score is changed!!!!
	start_time();
}

void newdemo_record_primary_ammo(int old_ammo, int new_ammo)
{
	stop_time();
	nd_write_byte(ND_EVENT_PRIMARY_AMMO);
	if (old_ammo < 0)
		nd_write_short((short)new_ammo);
	else
		nd_write_short((short)old_ammo);
	nd_write_short((short)new_ammo);
	start_time();
}

void newdemo_record_secondary_ammo(int old_ammo, int new_ammo)
{
	stop_time();
	nd_write_byte(ND_EVENT_SECONDARY_AMMO);
	if (old_ammo < 0)
		nd_write_short((short)new_ammo);
	else
		nd_write_short((short)old_ammo);
	nd_write_short((short)new_ammo);
	start_time();
}

void newdemo_record_door_opening(int segnum, int side)
{
	stop_time();
	nd_write_byte(ND_EVENT_DOOR_OPENING);
	nd_write_short((short)segnum);
	nd_write_byte((byte)side);
	start_time();
}

void newdemo_record_laser_level(byte old_level, byte new_level)
{
	stop_time();
	nd_write_byte(ND_EVENT_LASER_LEVEL);
	nd_write_byte(old_level);
	nd_write_byte(new_level);
	start_time();
}

void newdemo_record_cloaking_wall(int front_wall_num, int back_wall_num, ubyte type, ubyte state, fix cloak_value, fix l0, fix l1, fix l2, fix l3)
{
	Assert(front_wall_num <= 255 && back_wall_num <= 255);

	stop_time();
	nd_write_byte(ND_EVENT_CLOAKING_WALL);
	nd_write_byte(front_wall_num);
	nd_write_byte(back_wall_num);
	nd_write_byte(type);
	nd_write_byte(state);
	nd_write_byte(cloak_value);
	nd_write_short(l0>>8);
	nd_write_short(l1>>8);
	nd_write_short(l2>>8);
	nd_write_short(l3>>8);
	start_time();
}

void newdemo_set_new_level(int level_num)
{
	int i;
	int side;
	segment *seg;

	stop_time();
	nd_write_byte(ND_EVENT_NEW_LEVEL);
	nd_write_byte((byte)level_num);
	nd_write_byte((byte)Current_level_num);

	if (JustStartedRecording==1)
	{
		nd_write_int(Num_walls);
		for (i=0;i<Num_walls;i++)
		{
			nd_write_byte (Walls[i].type);
			nd_write_byte (Walls[i].flags);
			nd_write_byte (Walls[i].state);

			seg = &Segments[Walls[i].segnum];
			side = Walls[i].sidenum;
			nd_write_short (seg->sides[side].tmap_num);
			nd_write_short (seg->sides[side].tmap_num2);
			JustStartedRecording=0;
		}
	}

	start_time();
}

int newdemo_read_demo_start(int rnd_demo)
{
	byte i, version, game_type, laser_level;
	char c, energy, shield;
	char text[50], current_mission[9];

	nd_read_byte(&c);
	if ((c != ND_EVENT_START_DEMO) || nd_bad_read) {
		newmenu_item m[1];

		sprintf(text, "%s %s", TXT_CANT_PLAYBACK, TXT_DEMO_CORRUPT);
		m[ 0].type = NM_TYPE_TEXT; m[ 0].text = text;
		newmenu_do( NULL, NULL, sizeof(m)/sizeof(*m), m, NULL );
		return 1;
	}
	nd_read_byte(&version);
	nd_read_byte(&game_type);
	if (game_type < DEMO_GAME_TYPE) {
		newmenu_item m[2];

		sprintf(text, "%s %s", TXT_CANT_PLAYBACK, TXT_RECORDED);
		m[ 0].type = NM_TYPE_TEXT; m[ 0].text = text;
		m[ 1].type = NM_TYPE_TEXT; m[ 1].text = "    In Descent: First Strike";

		newmenu_do( NULL, NULL, sizeof(m)/sizeof(*m), m, NULL );
		return 1;
	}
	if (game_type != DEMO_GAME_TYPE) {
		newmenu_item m[2];

		sprintf(text, "%s %s", TXT_CANT_PLAYBACK, TXT_RECORDED);
		m[ 0].type = NM_TYPE_TEXT; m[ 0].text = text;
		m[ 1].type = NM_TYPE_TEXT; m[ 1].text = "   In Unknown Descent version";

		newmenu_do( NULL, NULL, sizeof(m)/sizeof(*m), m, NULL );
		return 1;
	}
	if (version < DEMO_VERSION) {
		if (!rnd_demo) {
			newmenu_item m[1];
			sprintf(text, "%s %s", TXT_CANT_PLAYBACK, TXT_DEMO_OLD);
			m[ 0].type = NM_TYPE_TEXT; m[ 0].text = text;
			newmenu_do( NULL, NULL, sizeof(m)/sizeof(*m), m, NULL );
		}
		return 1;
	}
	nd_read_fix(&GameTime);
	Boss_cloak_start_time=Boss_cloak_end_time=GameTime;
	JasonPlaybackTotal=0;

	nd_read_int(&Newdemo_game_mode);

#ifdef NETWORK
	change_playernum_to((Newdemo_game_mode >> 16) & 0x7);
	if (Newdemo_game_mode & GM_TEAM) {
		nd_read_byte(&(Netgame.team_vector));
		nd_read_string(Netgame.team_name[0]);
		nd_read_string(Netgame.team_name[1]);
	}
	if (Newdemo_game_mode & GM_MULTI) {

		multi_new_game();
		nd_read_byte(&c);
		N_players = (int)c;
		// changed this to above two lines -- breaks on the mac because of
		// endian issues
		//		nd_read_byte((byte *)&N_players);
		for (i = 0 ; i < N_players; i++) {
			Players[i].cloak_time = 0;
			Players[i].invulnerable_time = 0;
			nd_read_string(Players[i].callsign);
			nd_read_byte(&(Players[i].connected));

			if (Newdemo_game_mode & GM_MULTI_COOP) {
				nd_read_int(&(Players[i].score));
			} else {
				nd_read_short((short *)&(Players[i].net_killed_total));
				nd_read_short((short *)&(Players[i].net_kills_total));
			}
		}
		Game_mode = Newdemo_game_mode;
		multi_sort_kill_list();
		Game_mode = GM_NORMAL;
	} else
#endif
		nd_read_int(&(Players[Player_num].score));      // Note link to above if!

	for (i = 0; i < MAX_PRIMARY_WEAPONS; i++)
		nd_read_short((short*)&(Players[Player_num].primary_ammo[i]));

	for (i = 0; i < MAX_SECONDARY_WEAPONS; i++)
			nd_read_short((short*)&(Players[Player_num].secondary_ammo[i]));

	nd_read_byte(&laser_level);
	if (laser_level != Players[Player_num].laser_level) {
		Players[Player_num].laser_level = laser_level;
		update_laser_weapon_info();
	}

	// Support for missions

	nd_read_string(current_mission);
	if (!load_mission_by_name(current_mission)) {
		if (!rnd_demo) {
			newmenu_item m[1];

			sprintf(text, TXT_NOMISSION4DEMO, current_mission);
			m[ 0].type = NM_TYPE_TEXT; m[ 0].text = text;
			newmenu_do( NULL, NULL, sizeof(m)/sizeof(*m), m, NULL );
		}
		return 1;
	}

	nd_recorded_total = 0;
	nd_playback_total = 0;
	nd_read_byte(&energy);
	nd_read_byte(&shield);

	nd_read_int((int *)&(Players[Player_num].flags));
	if (Players[Player_num].flags & PLAYER_FLAGS_CLOAKED) {
		Players[Player_num].cloak_time = GameTime - (CLOAK_TIME_MAX / 2);
		Newdemo_players_cloaked |= (1 << Player_num);
	}
	if (Players[Player_num].flags & PLAYER_FLAGS_INVULNERABLE)
		Players[Player_num].invulnerable_time = GameTime - (INVULNERABLE_TIME_MAX / 2);

	nd_read_byte((byte *)&Primary_weapon);
	nd_read_byte((byte *)&Secondary_weapon);

	// Next bit of code to fix problem that I introduced between 1.0 and 1.1
	// check the next byte -- it _will_ be a load_new_level event.  If it is
	// not, then we must shift all bytes up by one.

	Players[Player_num].energy = i2f(energy);
	Players[Player_num].shields = i2f(shield);
	JustStartedPlayback=1;
	return 0;
}

void newdemo_pop_ctrlcen_triggers()
{
	int anim_num, n, i;
	int side, cside;
	segment *seg, *csegp;

	for (i = 0; i < ControlCenterTriggers.num_links; i++) {
		seg = &Segments[ControlCenterTriggers.seg[i]];
		side = ControlCenterTriggers.side[i];
		csegp = &Segments[seg->children[side]];
		cside = find_connect_side(seg, csegp);
		anim_num = Walls[seg->sides[side].wall_num].clip_num;
		n = WallAnims[anim_num].num_frames;
		if (WallAnims[anim_num].flags & WCF_TMAP1) {
		seg->sides[side].tmap_num = csegp->sides[cside].tmap_num = WallAnims[anim_num].frames[n-1];
		} else {
			seg->sides[side].tmap_num2 = csegp->sides[cside].tmap_num2 = WallAnims[anim_num].frames[n-1];
		}
	}
}

#define N_PLAYER_SHIP_TEXTURES 6

void nd_render_extras (ubyte,object *);
extern void multi_apply_goal_textures ();
ubyte Newdemo_flying_guided=0;

int newdemo_read_frame_information()
{
	int done, segnum, side, objnum, soundno, angle, volume, i,shot;
	object *obj;
	ubyte c,WhichWindow;
	static byte saved_letter_cockpit;
	static byte saved_rearview_cockpit;
	object extraobj;
	static char LastReadValue=101;
	segment *seg;

	done = 0;

	if (Newdemo_vcr_state != ND_STATE_PAUSED)
		for (segnum=0; segnum <= Highest_segment_index; segnum++)
			Segments[segnum].objects = -1;

	reset_objects(1);
	Players[Player_num].homing_object_dist = -F1_0;

	prev_obj = NULL;

	while( !done ) {
		nd_read_byte(&c);
		if (nd_bad_read) { done = -1; break; }

		switch( c ) {

		case ND_EVENT_START_FRAME: {        // Followed by an integer frame number, then a fix FrameTime
			short last_frame_length;

			done=1;
			nd_read_short(&last_frame_length);
			nd_read_int(&NewdemoFrameCount);
			nd_read_int((int *)&nd_recorded_time);
			if (Newdemo_vcr_state == ND_STATE_PLAYBACK)
				nd_recorded_total += nd_recorded_time;
			NewdemoFrameCount--;

			if (nd_bad_read) { done = -1; break; }
			break;
		}

		case ND_EVENT_VIEWER_OBJECT:        // Followed by an object structure
			nd_read_byte (&WhichWindow);
			if (WhichWindow&15)
			{
				//mprintf ((0,"Reading extra!\n"));
				nd_read_object (&extraobj);
				if (Newdemo_vcr_state!=ND_STATE_PAUSED)
				{
					if (nd_bad_read) { done = -1; break; }

					nd_render_extras (WhichWindow,&extraobj);
				}
			}
			else
			{
				//mprintf ((0,"Reading viewer!\n"));
				//Viewer=&Objects[0];
				nd_read_object(Viewer);

				if (Newdemo_vcr_state != ND_STATE_PAUSED) {
					if (nd_bad_read) { done = -1; break; }
					segnum = Viewer->segnum;
					Viewer->next = Viewer->prev = Viewer->segnum = -1;

					// HACK HACK HACK -- since we have multiple level recording, it can be the case
					// HACK HACK HACK -- that when rewinding the demo, the viewer is in a segment
					// HACK HACK HACK -- that is greater than the highest index of segments.  Bash
					// HACK HACK HACK -- the viewer to segment 0 for bogus view.

					if (segnum > Highest_segment_index)
						segnum = 0;
					obj_link(Viewer-Objects,segnum);
				}
			}
			break;

		case ND_EVENT_RENDER_OBJECT:       // Followed by an object structure
			objnum = obj_allocate();
			if (objnum==-1)
				break;
			obj = &Objects[objnum];
			nd_read_object(obj);
			if (nd_bad_read) { done = -1; break; }
			if (Newdemo_vcr_state != ND_STATE_PAUSED) {
				segnum = obj->segnum;
				obj->next = obj->prev = obj->segnum = -1;

				// HACK HACK HACK -- don't render objects is segments greater than Highest_segment_index
				// HACK HACK HACK -- (see above)

				if (segnum > Highest_segment_index)
					break;

				obj_link(obj-Objects,segnum);
#ifdef NETWORK
				if ((obj->type == OBJ_PLAYER) && (Newdemo_game_mode & GM_MULTI)) {
					int player;

					if (Newdemo_game_mode & GM_TEAM)
						player = get_team(obj->id);
					else
						player = obj->id;
					if (player == 0)
						break;
					player--;

					for (i=0;i<N_PLAYER_SHIP_TEXTURES;i++)
						multi_player_textures[player][i] = ObjBitmaps[ObjBitmapPtrs[Polygon_models[obj->rtype.pobj_info.model_num].first_texture+i]];

					multi_player_textures[player][4] = ObjBitmaps[ObjBitmapPtrs[First_multi_bitmap_num+(player)*2]];
					multi_player_textures[player][5] = ObjBitmaps[ObjBitmapPtrs[First_multi_bitmap_num+(player)*2+1]];
					obj->rtype.pobj_info.alt_textures = player+1;
				}
#endif
			}
			break;

		case ND_EVENT_SOUND:
			nd_read_int(&soundno);
			if (nd_bad_read) {done = -1; break; }
			if (Newdemo_vcr_state == ND_STATE_PLAYBACK)
				digi_play_sample( soundno, F1_0 );
			break;

			//--unused		case ND_EVENT_SOUND_ONCE:
			//--unused			nd_read_int(&soundno);
			//--unused			if (nd_bad_read) { done = -1; break; }
			//--unused			if (Newdemo_vcr_state == ND_STATE_PLAYBACK)
			//--unused				digi_play_sample_once( soundno, F1_0 );
			//--unused			break;

		case ND_EVENT_SOUND_3D:
			nd_read_int(&soundno);
			nd_read_int(&angle);
			nd_read_int(&volume);
			if (nd_bad_read) { done = -1; break; }
			if (Newdemo_vcr_state == ND_STATE_PLAYBACK)
				digi_play_sample_3d( soundno, angle, volume, 0 );
			break;

		case ND_EVENT_SOUND_3D_ONCE:
			nd_read_int(&soundno);
			nd_read_int(&angle);
			nd_read_int(&volume);
			if (nd_bad_read) { done = -1; break; }
			if (Newdemo_vcr_state == ND_STATE_PLAYBACK)
				digi_play_sample_3d( soundno, angle, volume, 1 );
			break;

		case ND_EVENT_LINK_SOUND_TO_OBJ:
			{
				int soundno, objnum, max_volume, max_distance, loop_start, loop_end;
				int signature;
				nd_read_int( &soundno );
				nd_read_int( &signature );
				nd_read_int( &max_volume );
				nd_read_int( &max_distance );
				nd_read_int( &loop_start );
				nd_read_int( &loop_end );
				objnum = newdemo_find_object( signature );
				if ( objnum > -1 )  {   //  @mk, 2/22/96, John told me to.
					digi_link_sound_to_object3( soundno, objnum, 1, max_volume, max_distance, loop_start, loop_end );
				}
			}
			break;

		case ND_EVENT_KILL_SOUND_TO_OBJ:
			{
				int objnum, signature;
				nd_read_int( &signature );
				objnum = newdemo_find_object( signature );
				if ( objnum > -1 )  {   //  @mk, 2/22/96, John told me to.
					digi_kill_sound_linked_to_object(objnum);
				}
			}
			break;

		case ND_EVENT_WALL_HIT_PROCESS: {
			int player, segnum;
			fix damage;

			nd_read_int(&segnum);
			nd_read_int(&side);
			nd_read_fix(&damage);
			nd_read_int(&player);
			if (nd_bad_read) { done = -1; break; }
			if (Newdemo_vcr_state != ND_STATE_PAUSED)
				wall_hit_process(&Segments[segnum], side, damage, player, &(Objects[0]) );
			break;
		}

		case ND_EVENT_TRIGGER:
			nd_read_int(&segnum);
			nd_read_int(&side);
			nd_read_int(&objnum);
			nd_read_int(&shot);
			if (nd_bad_read) { done = -1; break; }
			if (Newdemo_vcr_state != ND_STATE_PAUSED)
			{
				mprintf ((0,"EVENT TRIGGER! shot=%d\n",shot));

				if (Triggers[Walls[Segments[segnum].sides[side].wall_num].trigger].type == TT_SECRET_EXIT) {
					int truth;

					nd_read_byte(&c);
					Assert(c == ND_EVENT_SECRET_THINGY);
					nd_read_int(&truth);
					if (!truth)
						check_trigger(&Segments[segnum], side, objnum,shot);
				} else
					check_trigger(&Segments[segnum], side, objnum,shot);
			}
			break;

		case ND_EVENT_HOSTAGE_RESCUED: {
			int hostage_number;

			nd_read_int(&hostage_number);
			if (nd_bad_read) { done = -1; break; }
			if (Newdemo_vcr_state != ND_STATE_PAUSED)
				hostage_rescue( hostage_number );
			break;
		}

		case ND_EVENT_MORPH_FRAME: {
#if 0
			morph_data *md;

			md = &morph_objects[0];
			if (newdemo_read( md->morph_vecs, sizeof(md->morph_vecs), 1 )!=1) { done=-1; break; }
			if (newdemo_read( md->submodel_active, sizeof(md->submodel_active), 1 )!=1) { done=-1; break; }
			if (newdemo_read( md->submodel_startpoints, sizeof(md->submodel_startpoints), 1 )!=1) { done=-1; break; }
#endif
			objnum = obj_allocate();
			if (objnum==-1)
				break;
			obj = &Objects[objnum];
			nd_read_object(obj);
			obj->render_type = RT_POLYOBJ;
			if (Newdemo_vcr_state != ND_STATE_PAUSED) {
				if (nd_bad_read) { done = -1; break; }
				if (Newdemo_vcr_state != ND_STATE_PAUSED) {
					segnum = obj->segnum;
					obj->next = obj->prev = obj->segnum = -1;
					obj_link(obj-Objects,segnum);
				}
			}
			break;
		}

		case ND_EVENT_WALL_TOGGLE:
			nd_read_int(&segnum);
			nd_read_int(&side);
			if (nd_bad_read) {done = -1; break; }
			if (Newdemo_vcr_state != ND_STATE_PAUSED)
				wall_toggle(&Segments[segnum], side);
			break;

		case ND_EVENT_CONTROL_CENTER_DESTROYED:
			nd_read_int(&Countdown_seconds_left);
			Control_center_destroyed = 1;
			if (nd_bad_read) { done = -1; break; }
			if (!Newdemo_cntrlcen_destroyed) {
				newdemo_pop_ctrlcen_triggers();
				Newdemo_cntrlcen_destroyed = 1;
				//do_controlcen_destroyed_stuff(NULL);
			}
			break;

		case ND_EVENT_HUD_MESSAGE: {
			char hud_msg[60];

			nd_read_string(&(hud_msg[0]));
			if (nd_bad_read) { done = -1; break; }
			HUD_init_message( hud_msg );
			break;
			}
		case ND_EVENT_START_GUIDED:
			Newdemo_flying_guided=1;
			if ((Newdemo_vcr_state == ND_STATE_REWINDING) || (Newdemo_vcr_state == ND_STATE_ONEFRAMEBACKWARD))
				Newdemo_flying_guided=0;
			break;
		case ND_EVENT_END_GUIDED:
			Newdemo_flying_guided=0;
			if ((Newdemo_vcr_state == ND_STATE_REWINDING) || (Newdemo_vcr_state == ND_STATE_ONEFRAMEBACKWARD))
				Newdemo_flying_guided=1;
			break;

		case ND_EVENT_PALETTE_EFFECT: {
			short r, g, b;

			nd_read_short(&r);
			nd_read_short(&g);
			nd_read_short(&b);
			if (nd_bad_read) { done = -1; break; }
			PALETTE_FLASH_SET(r,g,b);
			break;
		}

		case ND_EVENT_PLAYER_ENERGY: {
			ubyte energy;
			ubyte old_energy;

			nd_read_byte(&old_energy);
			nd_read_byte(&energy);
			if (nd_bad_read) {done = -1; break; }
			if ((Newdemo_vcr_state == ND_STATE_PLAYBACK) || (Newdemo_vcr_state == ND_STATE_FASTFORWARD) || (Newdemo_vcr_state == ND_STATE_ONEFRAMEFORWARD)) {
				Players[Player_num].energy = i2f(energy);
			} else if ((Newdemo_vcr_state == ND_STATE_REWINDING) || (Newdemo_vcr_state == ND_STATE_ONEFRAMEBACKWARD)) {
				if (old_energy != 255)
					Players[Player_num].energy = i2f(old_energy);
			}
			break;
		}

		case ND_EVENT_PLAYER_AFTERBURNER: {
			ubyte afterburner;
			ubyte old_afterburner;

			nd_read_byte(&old_afterburner);
			nd_read_byte(&afterburner);
			if (nd_bad_read) {done = -1; break; }
			if ((Newdemo_vcr_state == ND_STATE_PLAYBACK) || (Newdemo_vcr_state == ND_STATE_FASTFORWARD) || (Newdemo_vcr_state == ND_STATE_ONEFRAMEFORWARD)) {
				Afterburner_charge = afterburner<<9;
			} else if ((Newdemo_vcr_state == ND_STATE_REWINDING) || (Newdemo_vcr_state == ND_STATE_ONEFRAMEBACKWARD)) {
				if (old_afterburner != 255)
					Afterburner_charge = old_afterburner<<9;
			}
			break;
		}

		case ND_EVENT_PLAYER_SHIELD: {
			ubyte shield;
			ubyte old_shield;

			nd_read_byte(&old_shield);
			nd_read_byte(&shield);
			if (nd_bad_read) {done = -1; break; }
			if ((Newdemo_vcr_state == ND_STATE_PLAYBACK) || (Newdemo_vcr_state == ND_STATE_FASTFORWARD) || (Newdemo_vcr_state == ND_STATE_ONEFRAMEFORWARD)) {
				Players[Player_num].shields = i2f(shield);
			} else if ((Newdemo_vcr_state == ND_STATE_REWINDING) || (Newdemo_vcr_state == ND_STATE_ONEFRAMEBACKWARD)) {
				if (old_shield != 255)
					Players[Player_num].shields = i2f(old_shield);
			}
			break;
		}

		case ND_EVENT_PLAYER_FLAGS: {
			uint oflags;

			nd_read_int((int *)&(Players[Player_num].flags));
			if (nd_bad_read) {done = -1; break; }

			oflags = Players[Player_num].flags >> 16;
			Players[Player_num].flags &= 0xffff;

			if ((Newdemo_vcr_state == ND_STATE_REWINDING) || ((Newdemo_vcr_state == ND_STATE_ONEFRAMEBACKWARD) && (oflags != 0xffff)) ) {
				if (!(oflags & PLAYER_FLAGS_CLOAKED) && (Players[Player_num].flags & PLAYER_FLAGS_CLOAKED)) {
					Players[Player_num].cloak_time = 0;
					Newdemo_players_cloaked &= ~(1 << Player_num);
				}
				if ((oflags & PLAYER_FLAGS_CLOAKED) && !(Players[Player_num].flags & PLAYER_FLAGS_CLOAKED)) {
					Players[Player_num].cloak_time = GameTime - (CLOAK_TIME_MAX / 2);
					Newdemo_players_cloaked |= (1 << Player_num);
				}
				if (!(oflags & PLAYER_FLAGS_INVULNERABLE) && (Players[Player_num].flags & PLAYER_FLAGS_INVULNERABLE))
					Players[Player_num].invulnerable_time = 0;
				if ((oflags & PLAYER_FLAGS_INVULNERABLE) && !(Players[Player_num].flags & PLAYER_FLAGS_INVULNERABLE))
					Players[Player_num].invulnerable_time = GameTime - (INVULNERABLE_TIME_MAX / 2);
				Players[Player_num].flags = oflags;
			} else if ((Newdemo_vcr_state == ND_STATE_PLAYBACK) || (Newdemo_vcr_state == ND_STATE_FASTFORWARD) || (Newdemo_vcr_state == ND_STATE_ONEFRAMEFORWARD)) {
				if (!(oflags & PLAYER_FLAGS_CLOAKED) && (Players[Player_num].flags & PLAYER_FLAGS_CLOAKED)) {
					Players[Player_num].cloak_time = GameTime - (CLOAK_TIME_MAX / 2);
					Newdemo_players_cloaked |= (1 << Player_num);
				}
				if ((oflags & PLAYER_FLAGS_CLOAKED) && !(Players[Player_num].flags & PLAYER_FLAGS_CLOAKED)) {
					Players[Player_num].cloak_time = 0;
					Newdemo_players_cloaked &= ~(1 << Player_num);
				}
				if (!(oflags & PLAYER_FLAGS_INVULNERABLE) && (Players[Player_num].flags & PLAYER_FLAGS_INVULNERABLE))
					Players[Player_num].invulnerable_time = GameTime - (INVULNERABLE_TIME_MAX / 2);
				if ((oflags & PLAYER_FLAGS_INVULNERABLE) && !(Players[Player_num].flags & PLAYER_FLAGS_INVULNERABLE))
					Players[Player_num].invulnerable_time = 0;
			}
			update_laser_weapon_info();     // in case of quad laser change
			break;
		}

		case ND_EVENT_PLAYER_WEAPON: {
			byte weapon_type, weapon_num;
			byte old_weapon;

			nd_read_byte(&weapon_type);
			nd_read_byte(&weapon_num);
			nd_read_byte(&old_weapon);
			if ((Newdemo_vcr_state == ND_STATE_PLAYBACK) || (Newdemo_vcr_state == ND_STATE_FASTFORWARD) || (Newdemo_vcr_state == ND_STATE_ONEFRAMEFORWARD)) {
				if (weapon_type == 0)
					Primary_weapon = (int)weapon_num;
				else
					Secondary_weapon = (int)weapon_num;
			} else if ((Newdemo_vcr_state == ND_STATE_REWINDING) || (Newdemo_vcr_state == ND_STATE_ONEFRAMEBACKWARD)) {
				if (weapon_type == 0)
					Primary_weapon = (int)old_weapon;
				else
					Secondary_weapon = (int)old_weapon;
			}
			break;
		}

		case ND_EVENT_EFFECT_BLOWUP: {
			short segnum;
			byte side;
			vms_vector pnt;
			object dummy;

			//create a dummy object which will be the weapon that hits
			//the monitor. the blowup code wants to know who the parent of the
			//laser is, so create a laser whose parent is the player
			dummy.ctype.laser_info.parent_type = OBJ_PLAYER;

			nd_read_short(&segnum);
			nd_read_byte(&side);
			nd_read_vector(&pnt);
			if (Newdemo_vcr_state != ND_STATE_PAUSED)
				check_effect_blowup(&(Segments[segnum]), side, &pnt, &dummy, 0);
			break;
		}

		case ND_EVENT_HOMING_DISTANCE: {
			short distance;

			nd_read_short(&distance);
			Players[Player_num].homing_object_dist = i2f((int)(distance << 16));
			break;
		}

		case ND_EVENT_LETTERBOX:
			if ((Newdemo_vcr_state == ND_STATE_PLAYBACK) || (Newdemo_vcr_state == ND_STATE_FASTFORWARD) || (Newdemo_vcr_state == ND_STATE_ONEFRAMEFORWARD)) {
				saved_letter_cockpit = Cockpit_mode;
				select_cockpit(CM_LETTERBOX);
			} else if ((Newdemo_vcr_state == ND_STATE_REWINDING) || (Newdemo_vcr_state == ND_STATE_ONEFRAMEBACKWARD))
				select_cockpit(saved_letter_cockpit);
			break;

		case ND_EVENT_CHANGE_COCKPIT: {
			int dummy;

			nd_read_int (&dummy);
			select_cockpit (dummy);

			break;
		}
		case ND_EVENT_REARVIEW:
			if ((Newdemo_vcr_state == ND_STATE_PLAYBACK) || (Newdemo_vcr_state == ND_STATE_FASTFORWARD) || (Newdemo_vcr_state == ND_STATE_ONEFRAMEFORWARD)) {
				saved_rearview_cockpit = Cockpit_mode;
				if (Cockpit_mode == CM_FULL_COCKPIT)
					select_cockpit(CM_REAR_VIEW);
				Rear_view=1;
			} else if ((Newdemo_vcr_state == ND_STATE_REWINDING) || (Newdemo_vcr_state == ND_STATE_ONEFRAMEBACKWARD)) {
				if (saved_rearview_cockpit == CM_REAR_VIEW)     // hack to be sure we get a good cockpit on restore
					saved_rearview_cockpit = CM_FULL_COCKPIT;
				select_cockpit(saved_rearview_cockpit);
				Rear_view=0;
			}
			break;

		case ND_EVENT_RESTORE_COCKPIT:
			if ((Newdemo_vcr_state == ND_STATE_REWINDING) || (Newdemo_vcr_state == ND_STATE_ONEFRAMEBACKWARD)) {
				saved_letter_cockpit = Cockpit_mode;
				select_cockpit(CM_LETTERBOX);
			} else if ((Newdemo_vcr_state == ND_STATE_PLAYBACK) || (Newdemo_vcr_state == ND_STATE_FASTFORWARD) || (Newdemo_vcr_state == ND_STATE_ONEFRAMEFORWARD))
				select_cockpit(saved_letter_cockpit);
			break;


		case ND_EVENT_RESTORE_REARVIEW:
			if ((Newdemo_vcr_state == ND_STATE_REWINDING) || (Newdemo_vcr_state == ND_STATE_ONEFRAMEBACKWARD)) {
				saved_rearview_cockpit = Cockpit_mode;
				if (Cockpit_mode == CM_FULL_COCKPIT)
					select_cockpit(CM_REAR_VIEW);
				Rear_view=1;
			} else if ((Newdemo_vcr_state == ND_STATE_PLAYBACK) || (Newdemo_vcr_state == ND_STATE_FASTFORWARD) || (Newdemo_vcr_state == ND_STATE_ONEFRAMEFORWARD)) {
				if (saved_rearview_cockpit == CM_REAR_VIEW)     // hack to be sure we get a good cockpit on restore
					saved_rearview_cockpit = CM_FULL_COCKPIT;
				select_cockpit(saved_rearview_cockpit);
				Rear_view=0;
			}
			break;


		case ND_EVENT_WALL_SET_TMAP_NUM1: {
			short seg, cseg, tmap;
			ubyte side,cside;

			nd_read_short(&seg);
			nd_read_byte(&side);
			nd_read_short(&cseg);
			nd_read_byte(&cside);
			nd_read_short( &tmap );
			if ((Newdemo_vcr_state != ND_STATE_PAUSED) && (Newdemo_vcr_state != ND_STATE_REWINDING) && (Newdemo_vcr_state != ND_STATE_ONEFRAMEBACKWARD))
				Segments[seg].sides[side].tmap_num = Segments[cseg].sides[cside].tmap_num = tmap;
			break;
		}

		case ND_EVENT_WALL_SET_TMAP_NUM2: {
			short seg, cseg, tmap;
			ubyte side,cside;

			nd_read_short(&seg);
			nd_read_byte(&side);
			nd_read_short(&cseg);
			nd_read_byte(&cside);
			nd_read_short( &tmap );
			if ((Newdemo_vcr_state != ND_STATE_PAUSED) && (Newdemo_vcr_state != ND_STATE_REWINDING) && (Newdemo_vcr_state != ND_STATE_ONEFRAMEBACKWARD)) {
				Assert(tmap!=0 && Segments[seg].sides[side].tmap_num2!=0);
				Segments[seg].sides[side].tmap_num2 = Segments[cseg].sides[cside].tmap_num2 = tmap;
			}
			break;
		}

		case ND_EVENT_MULTI_CLOAK: {
			byte pnum;

			nd_read_byte(&pnum);
			if ((Newdemo_vcr_state == ND_STATE_REWINDING) || (Newdemo_vcr_state == ND_STATE_ONEFRAMEBACKWARD)) {
				Players[pnum].flags &= ~PLAYER_FLAGS_CLOAKED;
				Players[pnum].cloak_time = 0;
				Newdemo_players_cloaked &= ~(1 << pnum);
			} else if ((Newdemo_vcr_state == ND_STATE_PLAYBACK) || (Newdemo_vcr_state == ND_STATE_FASTFORWARD) || (Newdemo_vcr_state == ND_STATE_ONEFRAMEFORWARD)) {
				Players[pnum].flags |= PLAYER_FLAGS_CLOAKED;
				Players[pnum].cloak_time = GameTime  - (CLOAK_TIME_MAX / 2);
				Newdemo_players_cloaked |= (1 << pnum);
			}
			break;
		}

		case ND_EVENT_MULTI_DECLOAK: {
			byte pnum;

			nd_read_byte(&pnum);

			if ((Newdemo_vcr_state == ND_STATE_REWINDING) || (Newdemo_vcr_state == ND_STATE_ONEFRAMEBACKWARD)) {
				Players[pnum].flags |= PLAYER_FLAGS_CLOAKED;
				Players[pnum].cloak_time = GameTime  - (CLOAK_TIME_MAX / 2);
				Newdemo_players_cloaked |= (1 << pnum);
			} else if ((Newdemo_vcr_state == ND_STATE_PLAYBACK) || (Newdemo_vcr_state == ND_STATE_FASTFORWARD) || (Newdemo_vcr_state == ND_STATE_ONEFRAMEFORWARD)) {
				Players[pnum].flags &= ~PLAYER_FLAGS_CLOAKED;
				Players[pnum].cloak_time = 0;
				Newdemo_players_cloaked &= ~(1 << pnum);
			}
			break;
		}

		case ND_EVENT_MULTI_DEATH: {
			byte pnum;

			nd_read_byte(&pnum);
			if ((Newdemo_vcr_state == ND_STATE_REWINDING) || (Newdemo_vcr_state == ND_STATE_ONEFRAMEBACKWARD))
				Players[pnum].net_killed_total--;
			else if ((Newdemo_vcr_state == ND_STATE_PLAYBACK) || (Newdemo_vcr_state == ND_STATE_FASTFORWARD) || (Newdemo_vcr_state == ND_STATE_ONEFRAMEFORWARD))
				Players[pnum].net_killed_total++;
			break;
		}

#ifdef NETWORK
		case ND_EVENT_MULTI_KILL: {
			byte pnum, kill;

			nd_read_byte(&pnum);
			nd_read_byte(&kill);
			if ((Newdemo_vcr_state == ND_STATE_REWINDING) || (Newdemo_vcr_state == ND_STATE_ONEFRAMEBACKWARD)) {
				Players[pnum].net_kills_total -= kill;
				if (Newdemo_game_mode & GM_TEAM)
					team_kills[get_team(pnum)] -= kill;
			} else if ((Newdemo_vcr_state == ND_STATE_PLAYBACK) || (Newdemo_vcr_state == ND_STATE_FASTFORWARD) || (Newdemo_vcr_state == ND_STATE_ONEFRAMEFORWARD)) {
				Players[pnum].net_kills_total += kill;
				if (Newdemo_game_mode & GM_TEAM)
					team_kills[get_team(pnum)] += kill;
			}
			Game_mode = Newdemo_game_mode;
			multi_sort_kill_list();
			Game_mode = GM_NORMAL;
			break;
		}

		case ND_EVENT_MULTI_CONNECT: {
			byte pnum, new_player;
			int killed_total, kills_total;
			char new_callsign[CALLSIGN_LEN+1], old_callsign[CALLSIGN_LEN+1];

			nd_read_byte(&pnum);
			nd_read_byte(&new_player);
			if (!new_player) {
				nd_read_string(old_callsign);
				nd_read_int(&killed_total);
				nd_read_int(&kills_total);
			}
			nd_read_string(new_callsign);
			if ((Newdemo_vcr_state == ND_STATE_REWINDING) || (Newdemo_vcr_state == ND_STATE_ONEFRAMEBACKWARD)) {
				Players[pnum].connected = 0;
				if (!new_player) {
					memcpy(Players[pnum].callsign, old_callsign, CALLSIGN_LEN+1);
					Players[pnum].net_killed_total = killed_total;
					Players[pnum].net_kills_total = kills_total;
				} else {
					N_players--;
				}
			} else if ((Newdemo_vcr_state == ND_STATE_PLAYBACK) || (Newdemo_vcr_state == ND_STATE_FASTFORWARD) || (Newdemo_vcr_state == ND_STATE_ONEFRAMEFORWARD)) {
				Players[pnum].connected = 1;
				Players[pnum].net_kills_total = 0;
				Players[pnum].net_killed_total = 0;
				memcpy(Players[pnum].callsign, new_callsign, CALLSIGN_LEN+1);
				if (new_player)
					N_players++;
			}
			break;
		}

		case ND_EVENT_MULTI_RECONNECT: {
			byte pnum;

			nd_read_byte(&pnum);
			if ((Newdemo_vcr_state == ND_STATE_REWINDING) || (Newdemo_vcr_state == ND_STATE_ONEFRAMEBACKWARD))
				Players[pnum].connected = 0;
			else if ((Newdemo_vcr_state == ND_STATE_PLAYBACK) || (Newdemo_vcr_state == ND_STATE_FASTFORWARD) || (Newdemo_vcr_state == ND_STATE_ONEFRAMEFORWARD))
				Players[pnum].connected = 1;
			break;
		}

		case ND_EVENT_MULTI_DISCONNECT: {
			byte pnum;

			nd_read_byte(&pnum);
			if ((Newdemo_vcr_state == ND_STATE_REWINDING) || (Newdemo_vcr_state == ND_STATE_ONEFRAMEBACKWARD))
				Players[pnum].connected = 1;
			else if ((Newdemo_vcr_state == ND_STATE_PLAYBACK) || (Newdemo_vcr_state == ND_STATE_FASTFORWARD) || (Newdemo_vcr_state == ND_STATE_ONEFRAMEFORWARD))
				Players[pnum].connected = 0;
			break;
		}

		case ND_EVENT_MULTI_SCORE: {
			int score;
			byte pnum;

			nd_read_byte(&pnum);
			nd_read_int(&score);
			if ((Newdemo_vcr_state == ND_STATE_REWINDING) || (Newdemo_vcr_state == ND_STATE_ONEFRAMEBACKWARD))
				Players[pnum].score -= score;
			else if ((Newdemo_vcr_state == ND_STATE_PLAYBACK) || (Newdemo_vcr_state == ND_STATE_FASTFORWARD) || (Newdemo_vcr_state == ND_STATE_ONEFRAMEFORWARD))
				Players[pnum].score += score;
			Game_mode = Newdemo_game_mode;
			multi_sort_kill_list();
			Game_mode = GM_NORMAL;
			break;
		}

#endif // NETWORK
		case ND_EVENT_PLAYER_SCORE: {
			int score;

			nd_read_int(&score);
			if ((Newdemo_vcr_state == ND_STATE_REWINDING) || (Newdemo_vcr_state == ND_STATE_ONEFRAMEBACKWARD))
				Players[Player_num].score -= score;
			else if ((Newdemo_vcr_state == ND_STATE_PLAYBACK) || (Newdemo_vcr_state == ND_STATE_FASTFORWARD) || (Newdemo_vcr_state == ND_STATE_ONEFRAMEFORWARD))
				Players[Player_num].score += score;
			break;
		}


		case ND_EVENT_PRIMARY_AMMO: {
			short old_ammo, new_ammo;

			nd_read_short(&old_ammo);
			nd_read_short(&new_ammo);

			if ((Newdemo_vcr_state == ND_STATE_REWINDING) || (Newdemo_vcr_state == ND_STATE_ONEFRAMEBACKWARD))
				Players[Player_num].primary_ammo[Primary_weapon] = old_ammo;
			else if ((Newdemo_vcr_state == ND_STATE_PLAYBACK) || (Newdemo_vcr_state == ND_STATE_FASTFORWARD) || (Newdemo_vcr_state == ND_STATE_ONEFRAMEFORWARD))
				Players[Player_num].primary_ammo[Primary_weapon] = new_ammo;
			break;
		}

		case ND_EVENT_SECONDARY_AMMO: {
			short old_ammo, new_ammo;

			nd_read_short(&old_ammo);
			nd_read_short(&new_ammo);

			if ((Newdemo_vcr_state == ND_STATE_REWINDING) || (Newdemo_vcr_state == ND_STATE_ONEFRAMEBACKWARD))
				Players[Player_num].secondary_ammo[Secondary_weapon] = old_ammo;
			else if ((Newdemo_vcr_state == ND_STATE_PLAYBACK) || (Newdemo_vcr_state == ND_STATE_FASTFORWARD) || (Newdemo_vcr_state == ND_STATE_ONEFRAMEFORWARD))
				Players[Player_num].secondary_ammo[Secondary_weapon] = new_ammo;
			break;
		}

		case ND_EVENT_DOOR_OPENING: {
			short segnum;
			byte side;

			nd_read_short(&segnum);
			nd_read_byte(&side);
			if ((Newdemo_vcr_state == ND_STATE_REWINDING) || (Newdemo_vcr_state == ND_STATE_ONEFRAMEBACKWARD)) {
				int anim_num;
				int cside;
				segment *segp, *csegp;

				segp = &Segments[segnum];
				csegp = &Segments[segp->children[side]];
				cside = find_connect_side(segp, csegp);
				anim_num = Walls[segp->sides[side].wall_num].clip_num;

				if (WallAnims[anim_num].flags & WCF_TMAP1) {
					segp->sides[side].tmap_num = csegp->sides[cside].tmap_num = WallAnims[anim_num].frames[0];
				} else {
					segp->sides[side].tmap_num2 = csegp->sides[cside].tmap_num2 = WallAnims[anim_num].frames[0];
				}
			}
			break;
		}

		case ND_EVENT_LASER_LEVEL: {
			byte old_level, new_level;

			nd_read_byte(&old_level);
			nd_read_byte(&new_level);
			if ((Newdemo_vcr_state == ND_STATE_REWINDING) || (Newdemo_vcr_state == ND_STATE_ONEFRAMEBACKWARD)) {
				Players[Player_num].laser_level = old_level;
				update_laser_weapon_info();
			} else if ((Newdemo_vcr_state == ND_STATE_PLAYBACK) || (Newdemo_vcr_state == ND_STATE_FASTFORWARD) || (Newdemo_vcr_state == ND_STATE_ONEFRAMEFORWARD)) {
				Players[Player_num].laser_level = new_level;
				update_laser_weapon_info();
			}
			break;
		}

		case ND_EVENT_CLOAKING_WALL: {
			ubyte back_wall_num,front_wall_num,type,state,cloak_value;
			short l0,l1,l2,l3;
			segment *segp;
			int sidenum;

			nd_read_byte(&front_wall_num);
			nd_read_byte(&back_wall_num);
			nd_read_byte(&type);
			nd_read_byte(&state);
			nd_read_byte(&cloak_value);
			nd_read_short(&l0);
			nd_read_short(&l1);
			nd_read_short(&l2);
			nd_read_short(&l3);

			Walls[front_wall_num].type = type;
			Walls[front_wall_num].state = state;
			Walls[front_wall_num].cloak_value = cloak_value;
			segp = &Segments[Walls[front_wall_num].segnum];
			sidenum = Walls[front_wall_num].sidenum;
			segp->sides[sidenum].uvls[0].l = ((int) l0) << 8;
			segp->sides[sidenum].uvls[1].l = ((int) l1) << 8;
			segp->sides[sidenum].uvls[2].l = ((int) l2) << 8;
			segp->sides[sidenum].uvls[3].l = ((int) l3) << 8;

			Walls[back_wall_num].type = type;
			Walls[back_wall_num].state = state;
			Walls[back_wall_num].cloak_value = cloak_value;
			segp = &Segments[Walls[back_wall_num].segnum];
			sidenum = Walls[back_wall_num].sidenum;
			segp->sides[sidenum].uvls[0].l = ((int) l0) << 8;
			segp->sides[sidenum].uvls[1].l = ((int) l1) << 8;
			segp->sides[sidenum].uvls[2].l = ((int) l2) << 8;
			segp->sides[sidenum].uvls[3].l = ((int) l3) << 8;

			break;
		}

		case ND_EVENT_NEW_LEVEL: {
			byte new_level, old_level, loaded_level;

			nd_read_byte (&new_level);
			nd_read_byte (&old_level);
			if (Newdemo_vcr_state == ND_STATE_PAUSED)
				break;

			stop_time();
			if ((Newdemo_vcr_state == ND_STATE_REWINDING) || (Newdemo_vcr_state == ND_STATE_ONEFRAMEBACKWARD))
				loaded_level = old_level;
			else {
				loaded_level = new_level;
				for (i = 0; i < MAX_PLAYERS; i++) {
					Players[i].cloak_time = 0;
					Players[i].flags &= ~PLAYER_FLAGS_CLOAKED;
				}
			}
			if ((loaded_level < Last_secret_level) || (loaded_level > Last_level)) {
				newmenu_item m[3];

				m[ 0].type = NM_TYPE_TEXT; m[ 0].text = TXT_CANT_PLAYBACK;
				m[ 1].type = NM_TYPE_TEXT; m[ 1].text = TXT_LEVEL_CANT_LOAD;
				m[ 2].type = NM_TYPE_TEXT; m[ 2].text = TXT_DEMO_OLD_CORRUPT;
				newmenu_do( NULL, NULL, sizeof(m)/sizeof(*m), m, NULL );
				return -1;
			}

			LoadLevel((int)loaded_level,1);
			Newdemo_cntrlcen_destroyed = 0;

			if (JustStartedPlayback)
			{
				nd_read_int (&Num_walls);
				for (i=0;i<Num_walls;i++)    // restore the walls
				{
					nd_read_byte (&Walls[i].type);
					nd_read_byte (&Walls[i].flags);
					nd_read_byte (&Walls[i].state);

					seg = &Segments[Walls[i].segnum];
					side = Walls[i].sidenum;
					nd_read_short (&seg->sides[side].tmap_num);
					nd_read_short (&seg->sides[side].tmap_num2);
				}
#ifdef NETWORK
				if (Newdemo_game_mode & GM_CAPTURE)
					multi_apply_goal_textures ();
#endif
				JustStartedPlayback=0;
			}


			// so says Rob H.!!!			if (Newdemo_game_mode & GM_MULTI) {
			// so says Rob H.!!!				for (i = 0; i < Num_walls; i++) {
			// so says Rob H.!!!					if (Walls[i].type == WALL_BLASTABLE)
			// so says Rob H.!!!					{
			// so says Rob H.!!!						int a, n;
			// so says Rob H.!!!						int side;
			// so says Rob H.!!!						segment *seg;
			// so says Rob H.!!!
			// so says Rob H.!!!						seg = &Segments[Walls[i].segnum];
			// so says Rob H.!!!						side = Walls[i].sidenum;
			// so says Rob H.!!!						a = Walls[i].clip_num;
			// so says Rob H.!!!						n = WallAnims[a].num_frames;
			// so says Rob H.!!!						seg->sides[side].tmap_num = WallAnims[a].frames[n-1];
			// so says Rob H.!!!						Walls[i].flags |= WALL_BLASTED;
			// so says Rob H.!!!					}
			// so says Rob H.!!!				}
			// so says Rob H.!!!			}

			reset_palette_add();                // get palette back to normal
			start_time();
			break;
		}

		case ND_EVENT_EOF: {
			done=-1;
			fseek(infile, -1, SEEK_CUR);        // get back to the EOF marker
			Newdemo_at_eof = 1;
			NewdemoFrameCount++;
			break;
		}

		default:
			Int3();
		}
	}

	LastReadValue=c;

	if (nd_bad_read) {
		newmenu_item m[2];

		m[ 0].type = NM_TYPE_TEXT; m[ 0].text = TXT_DEMO_ERR_READING;
		m[ 1].type = NM_TYPE_TEXT; m[ 1].text = TXT_DEMO_OLD_CORRUPT;
		newmenu_do( NULL, NULL, sizeof(m)/sizeof(*m), m, NULL );
	}

	return done;
}

void newdemo_goto_beginning()
{
	//if (NewdemoFrameCount == 0)
	//	return;
	fseek(infile, 0, SEEK_SET);
	Newdemo_vcr_state = ND_STATE_PLAYBACK;
	if (newdemo_read_demo_start(0))
		newdemo_stop_playback();
	if (newdemo_read_frame_information() == -1)
		newdemo_stop_playback();
	if (newdemo_read_frame_information() == -1)
		newdemo_stop_playback();
	Newdemo_vcr_state = ND_STATE_PAUSED;
	Newdemo_at_eof = 0;
}

void newdemo_goto_end()
{
	short frame_length, byte_count, bshort;
	byte level, bbyte, laser_level;
	ubyte energy, shield, c;
	int i, loc, bint;

	fseek(infile, -2, SEEK_END);
	nd_read_byte(&level);

	if ((level < Last_secret_level) || (level > Last_level)) {
		newmenu_item m[3];

		m[ 0].type = NM_TYPE_TEXT; m[ 0].text = TXT_CANT_PLAYBACK;
		m[ 1].type = NM_TYPE_TEXT; m[ 1].text = TXT_LEVEL_CANT_LOAD;
		m[ 2].type = NM_TYPE_TEXT; m[ 2].text = TXT_DEMO_OLD_CORRUPT;
		newmenu_do( NULL, NULL, sizeof(m)/sizeof(*m), m, NULL );
		newdemo_stop_playback();
		return;
	}
	if (level != Current_level_num)
		LoadLevel(level,1);

	fseek(infile, -4, SEEK_END);
	nd_read_short(&byte_count);
	fseek(infile, -2 - byte_count, SEEK_CUR);

	nd_read_short(&frame_length);
	loc = ftell(infile);
	if (Newdemo_game_mode & GM_MULTI)
		nd_read_byte(&Newdemo_players_cloaked);
	else
		nd_read_byte(&bbyte);
	nd_read_byte(&bbyte);
	nd_read_short(&bshort);
	nd_read_int(&bint);

	nd_read_byte(&energy);
	nd_read_byte(&shield);
	Players[Player_num].energy = i2f(energy);
	Players[Player_num].shields = i2f(shield);
	nd_read_int((int *)&(Players[Player_num].flags));
	if (Players[Player_num].flags & PLAYER_FLAGS_CLOAKED) {
		Players[Player_num].cloak_time = GameTime - (CLOAK_TIME_MAX / 2);
		Newdemo_players_cloaked |= (1 << Player_num);
	}
	if (Players[Player_num].flags & PLAYER_FLAGS_INVULNERABLE)
		Players[Player_num].invulnerable_time = GameTime - (INVULNERABLE_TIME_MAX / 2);
	nd_read_byte((byte *)&Primary_weapon);
	nd_read_byte((byte *)&Secondary_weapon);
	for (i = 0; i < MAX_PRIMARY_WEAPONS; i++)
		nd_read_short((short *)&(Players[Player_num].primary_ammo[i]));
	for (i = 0; i < MAX_SECONDARY_WEAPONS; i++)
		nd_read_short((short *)&(Players[Player_num].secondary_ammo[i]));
	nd_read_byte(&laser_level);
	if (laser_level != Players[Player_num].laser_level) {
		Players[Player_num].laser_level = laser_level;
		update_laser_weapon_info();
	}

	if (Newdemo_game_mode & GM_MULTI) {
		nd_read_byte(&c);
		N_players = (int)c;
		// see newdemo_read_start_demo for explanation of
		// why this is commented out
		//		nd_read_byte((byte *)&N_players);
		for (i = 0; i < N_players; i++) {
			nd_read_string(Players[i].callsign);
			nd_read_byte(&(Players[i].connected));
			if (Newdemo_game_mode & GM_MULTI_COOP) {
				nd_read_int(&(Players[i].score));
			} else {
				nd_read_short((short *)&(Players[i].net_killed_total));
				nd_read_short((short *)&(Players[i].net_kills_total));
			}
		}
	} else {
		nd_read_int(&(Players[Player_num].score));
	}

	fseek(infile, loc, SEEK_SET);
	fseek(infile, -frame_length, SEEK_CUR);
	nd_read_int(&NewdemoFrameCount);            // get the frame count
	NewdemoFrameCount--;
	fseek(infile, 4, SEEK_CUR);
	Newdemo_vcr_state = ND_STATE_PLAYBACK;
	newdemo_read_frame_information();           // then the frame information
	Newdemo_vcr_state = ND_STATE_PAUSED;
	return;
}

void newdemo_back_frames(int frames)
{
	short last_frame_length;
	int i;

	for (i = 0; i < frames; i++)
	{
		fseek(infile, -10, SEEK_CUR);
		nd_read_short(&last_frame_length);
		fseek(infile, 8 - last_frame_length, SEEK_CUR);

		if (!Newdemo_at_eof && newdemo_read_frame_information() == -1) {
			newdemo_stop_playback();
			return;
		}
		if (Newdemo_at_eof)
			Newdemo_at_eof = 0;

		fseek(infile, -10, SEEK_CUR);
		nd_read_short(&last_frame_length);
		fseek(infile, 8 - last_frame_length, SEEK_CUR);
	}

}

/*
 *  routine to interpolate the viewer position.  the current position is
 *  stored in the Viewer object.  Save this position, and read the next
 *  frame to get all objects read in.  Calculate the delta playback and
 *  the delta recording frame times between the two frames, then intepolate
 *  the viewers position accordingly.  nd_recorded_time is the time that it
 *  took the recording to render the frame that we are currently looking
 *  at.
*/

void interpolate_frame(fix d_play, fix d_recorded)
{
	int i, j, num_cur_objs;
	fix factor;
	object *cur_objs;

	factor = fixdiv(d_play, d_recorded);
	if (factor > F1_0)
		factor = F1_0;

	num_cur_objs = Highest_object_index;
	cur_objs = (object *)d_malloc(sizeof(object) * (num_cur_objs + 1));
	if (cur_objs == NULL) {
		mprintf((0,"Couldn't get %d bytes for cur_objs in interpolate_frame\n", sizeof(object) * num_cur_objs));
		Int3();
		return;
	}
	for (i = 0; i <= num_cur_objs; i++)
		memcpy(&(cur_objs[i]), &(Objects[i]), sizeof(object));

	Newdemo_vcr_state = ND_STATE_PAUSED;
	if (newdemo_read_frame_information() == -1) {
		d_free(cur_objs);
		newdemo_stop_playback();
		return;
	}

	for (i = 0; i <= num_cur_objs; i++) {
		for (j = 0; j <= Highest_object_index; j++) {
			if (cur_objs[i].signature == Objects[j].signature) {
				ubyte render_type = cur_objs[i].render_type;
				//fix delta_p, delta_h, delta_b;
				fix delta_x, delta_y, delta_z;
				//vms_angvec cur_angles, dest_angles;

				//  Extract the angles from the object orientation matrix.
				//  Some of this code taken from ai_turn_towards_vector
				//  Don't do the interpolation on certain render types which don't use an orientation matrix

				if (!((render_type == RT_LASER) || (render_type == RT_FIREBALL) || (render_type == RT_POWERUP))) {

					vms_vector  fvec1, fvec2, rvec1, rvec2;
					fix         mag1;

					fvec1 = cur_objs[i].orient.fvec;
					vm_vec_scale(&fvec1, F1_0-factor);
					fvec2 = Objects[j].orient.fvec;
					vm_vec_scale(&fvec2, factor);
					vm_vec_add2(&fvec1, &fvec2);
					mag1 = vm_vec_normalize_quick(&fvec1);
					if (mag1 > F1_0/256) {
						rvec1 = cur_objs[i].orient.rvec;
						vm_vec_scale(&rvec1, F1_0-factor);
						rvec2 = Objects[j].orient.rvec;
						vm_vec_scale(&rvec2, factor);
						vm_vec_add2(&rvec1, &rvec2);
						vm_vec_normalize_quick(&rvec1); // Note: Doesn't matter if this is null, if null, vm_vector_2_matrix will just use fvec1
						vm_vector_2_matrix(&cur_objs[i].orient, &fvec1, NULL, &rvec1);
					}

					//--old new way --	vms_vector fvec1, fvec2, rvec1, rvec2;
					//--old new way --
					//--old new way --	fvec1 = cur_objs[i].orient.fvec;
					//--old new way --	vm_vec_scale(&fvec1, F1_0-factor);
					//--old new way --	fvec2 = Objects[j].orient.fvec;
					//--old new way --	vm_vec_scale(&fvec2, factor);
					//--old new way --	vm_vec_add2(&fvec1, &fvec2);
					//--old new way --	vm_vec_normalize_quick(&fvec1);
					//--old new way --
					//--old new way --	rvec1 = cur_objs[i].orient.rvec;
					//--old new way --	vm_vec_scale(&rvec1, F1_0-factor);
					//--old new way --	rvec2 = Objects[j].orient.rvec;
					//--old new way --	vm_vec_scale(&rvec2, factor);
					//--old new way --	vm_vec_add2(&rvec1, &rvec2);
					//--old new way --	vm_vec_normalize_quick(&rvec1);
					//--old new way --
					//--old new way --	vm_vector_2_matrix(&cur_objs[i].orient, &fvec1, NULL, &rvec1);

					// -- old fashioned way --					vm_extract_angles_matrix(&cur_angles, &(cur_objs[i].orient));
					// -- old fashioned way --					vm_extract_angles_matrix(&dest_angles, &(Objects[j].orient));
					// -- old fashioned way --
					// -- old fashioned way --					delta_p = (dest_angles.p - cur_angles.p);
					// -- old fashioned way --					delta_h = (dest_angles.h - cur_angles.h);
					// -- old fashioned way --					delta_b = (dest_angles.b - cur_angles.b);
					// -- old fashioned way --
					// -- old fashioned way --					if (delta_p != 0) {
					// -- old fashioned way --						if (delta_p > F1_0/2) delta_p = dest_angles.p - cur_angles.p - F1_0;
					// -- old fashioned way --						if (delta_p < -F1_0/2) delta_p = dest_angles.p - cur_angles.p + F1_0;
					// -- old fashioned way --						delta_p = fixmul(delta_p, factor);
					// -- old fashioned way --						cur_angles.p += delta_p;
					// -- old fashioned way --					}
					// -- old fashioned way --					if (delta_h != 0) {
					// -- old fashioned way --						if (delta_h > F1_0/2) delta_h = dest_angles.h - cur_angles.h - F1_0;
					// -- old fashioned way --						if (delta_h < -F1_0/2) delta_h = dest_angles.h - cur_angles.h + F1_0;
					// -- old fashioned way --						delta_h = fixmul(delta_h, factor);
					// -- old fashioned way --						cur_angles.h += delta_h;
					// -- old fashioned way --					}
					// -- old fashioned way --					if (delta_b != 0) {
					// -- old fashioned way --						if (delta_b > F1_0/2) delta_b = dest_angles.b - cur_angles.b - F1_0;
					// -- old fashioned way --						if (delta_b < -F1_0/2) delta_b = dest_angles.b - cur_angles.b + F1_0;
					// -- old fashioned way --						delta_b = fixmul(delta_b, factor);
					// -- old fashioned way --						cur_angles.b += delta_b;
					// -- old fashioned way --					}
				}

				// Interpolate the object position.  This is just straight linear
				// interpolation.

				delta_x = Objects[j].pos.x - cur_objs[i].pos.x;
				delta_y = Objects[j].pos.y - cur_objs[i].pos.y;
				delta_z = Objects[j].pos.z - cur_objs[i].pos.z;

				delta_x = fixmul(delta_x, factor);
				delta_y = fixmul(delta_y, factor);
				delta_z = fixmul(delta_z, factor);

				cur_objs[i].pos.x += delta_x;
				cur_objs[i].pos.y += delta_y;
				cur_objs[i].pos.z += delta_z;

				// -- old fashioned way --// stuff the new angles back into the object structure
				// -- old fashioned way --				vm_angles_2_matrix(&(cur_objs[i].orient), &cur_angles);
			}
		}
	}

	// get back to original position in the demo file.  Reread the current
	// frame information again to reset all of the object stuff not covered
	// with Highest_object_index and the object array (previously rendered
	// objects, etc....)

	newdemo_back_frames(1);
	newdemo_back_frames(1);
	if (newdemo_read_frame_information() == -1)
		newdemo_stop_playback();
	Newdemo_vcr_state = ND_STATE_PLAYBACK;

	for (i = 0; i <= num_cur_objs; i++)
		memcpy(&(Objects[i]), &(cur_objs[i]), sizeof(object));
	Highest_object_index = num_cur_objs;
	d_free(cur_objs);
}

void newdemo_playback_one_frame()
{
	int frames_back, i, level;
	static fix base_interpol_time = 0;
	static fix d_recorded = 0;

	for (i = 0; i < MAX_PLAYERS; i++)
		if (Newdemo_players_cloaked & (1 << i))
			Players[i].cloak_time = GameTime - (CLOAK_TIME_MAX / 2);

	if (Players[Player_num].flags & PLAYER_FLAGS_INVULNERABLE)
		Players[Player_num].invulnerable_time = GameTime - (INVULNERABLE_TIME_MAX / 2);

	if (Newdemo_vcr_state == ND_STATE_PAUSED)       // render a frame or not
		return;

	if (Newdemo_vcr_state == ND_STATE_PLAYBACK)
		DoJasonInterpolate(nd_recorded_time);

	Control_center_destroyed = 0;
	Countdown_seconds_left = -1;
	PALETTE_FLASH_SET(0,0,0);       //clear flash

	if ((Newdemo_vcr_state == ND_STATE_REWINDING) || (Newdemo_vcr_state == ND_STATE_ONEFRAMEBACKWARD))
	{
		level = Current_level_num;
		if (NewdemoFrameCount == 0)
			return;
		else if ((Newdemo_vcr_state == ND_STATE_REWINDING) && (NewdemoFrameCount < 10)) {
			newdemo_goto_beginning();
			return;
		}
		if (Newdemo_vcr_state == ND_STATE_REWINDING)
			frames_back = 10;
		else
			frames_back = 1;
		if (Newdemo_at_eof) {
			fseek(infile, 11, SEEK_CUR);
		}
		newdemo_back_frames(frames_back);

		if (level != Current_level_num)
			newdemo_pop_ctrlcen_triggers();

		if (Newdemo_vcr_state == ND_STATE_ONEFRAMEBACKWARD) {
			if (level != Current_level_num)
				newdemo_back_frames(1);
			Newdemo_vcr_state = ND_STATE_PAUSED;
		}
	}
	else if (Newdemo_vcr_state == ND_STATE_FASTFORWARD) {
		if (!Newdemo_at_eof)
		{
			for (i = 0; i < 10; i++)
			{
				if (newdemo_read_frame_information() == -1)
				{
					if (Newdemo_at_eof)
						Newdemo_vcr_state = ND_STATE_PAUSED;
					else
						newdemo_stop_playback();
					break;
				}
			}
		}
		else
			Newdemo_vcr_state = ND_STATE_PAUSED;
	}
	else if (Newdemo_vcr_state == ND_STATE_ONEFRAMEFORWARD) {
		if (!Newdemo_at_eof) {
			level = Current_level_num;
			if (newdemo_read_frame_information() == -1) {
				if (!Newdemo_at_eof)
					newdemo_stop_playback();
			}
			if (level != Current_level_num) {
				if (newdemo_read_frame_information() == -1) {
					if (!Newdemo_at_eof)
						newdemo_stop_playback();
				}
			}
			Newdemo_vcr_state = ND_STATE_PAUSED;
		} else
			Newdemo_vcr_state = ND_STATE_PAUSED;
	}
	else {

		//  First, uptate the total playback time to date.  Then we check to see
		//  if we need to change the playback style to interpolate frames or
		//  skip frames based on where the playback time is relative to the
		//  recorded time.

		if (NewdemoFrameCount <= 0)
			nd_playback_total = nd_recorded_total;      // baseline total playback time
		else
			nd_playback_total += FrameTime;
		if ((playback_style == NORMAL_PLAYBACK) && (NewdemoFrameCount > 10))
			if ((nd_playback_total * INTERPOL_FACTOR) < nd_recorded_total) {
				playback_style = INTERPOLATE_PLAYBACK;
				nd_playback_total = nd_recorded_total + FrameTime;  // baseline playback time
				base_interpol_time = nd_recorded_total;
				d_recorded = nd_recorded_time;                      // baseline delta recorded
			}
		if ((playback_style == NORMAL_PLAYBACK) && (NewdemoFrameCount > 10))
			if (nd_playback_total > nd_recorded_total)
				playback_style = SKIP_PLAYBACK;


		if ((playback_style == INTERPOLATE_PLAYBACK) && Newdemo_do_interpolate) {
			fix d_play = 0;

			if (nd_recorded_total - nd_playback_total < FrameTime) {
				d_recorded = nd_recorded_total - nd_playback_total;

				while (nd_recorded_total - nd_playback_total < FrameTime) {
					object *cur_objs;
					int i, j, num_objs, level;

					num_objs = Highest_object_index;
					cur_objs = (object *)d_malloc(sizeof(object) * (num_objs + 1));
					if (cur_objs == NULL) {
						Warning ("Couldn't get %d bytes for objects in interpolate playback\n", sizeof(object) * num_objs);
						break;
					}
					for (i = 0; i <= num_objs; i++)
						memcpy(&(cur_objs[i]), &(Objects[i]), sizeof(object));

					level = Current_level_num;
					if (newdemo_read_frame_information() == -1) {
						d_free(cur_objs);
						newdemo_stop_playback();
						return;
					}
					if (level != Current_level_num) {
						d_free(cur_objs);
						if (newdemo_read_frame_information() == -1)
							newdemo_stop_playback();
						break;
					}

					//  for each new object in the frame just read in, determine if there is
					//  a corresponding object that we have been interpolating.  If so, then
					//  copy that interpolated object to the new Objects array so that the
					//  interpolated position and orientation can be preserved.

					for (i = 0; i <= num_objs; i++) {
						for (j = 0; j <= Highest_object_index; j++) {
							if (cur_objs[i].signature == Objects[j].signature) {
								memcpy(&(Objects[j].orient), &(cur_objs[i].orient), sizeof(vms_matrix));
								memcpy(&(Objects[j].pos), &(cur_objs[i].pos), sizeof(vms_vector));
								break;
							}
						}
					}
					d_free(cur_objs);
					d_recorded += nd_recorded_time;
					base_interpol_time = nd_playback_total - FrameTime;
				}
			}

			d_play = nd_playback_total - base_interpol_time;
			interpolate_frame(d_play, d_recorded);
			return;
		}
		else {
			//mprintf ((0, "*"));
			if (newdemo_read_frame_information() == -1) {
				newdemo_stop_playback();
				return;
			}
			if (playback_style == SKIP_PLAYBACK) {
				//mprintf ((0, "."));
				while (nd_playback_total > nd_recorded_total) {
					if (newdemo_read_frame_information() == -1) {
						newdemo_stop_playback();
						return;
					}
				}
			}
		}
	}
}

void newdemo_start_recording()
{
#ifdef WINDOWS
	Newdemo_size=GetFreeDiskSpace();
	mprintf((0, "Free space = %d\n", Newdemo_size));
#else
	Newdemo_size = GetDiskFree();
#endif

	Newdemo_size -= 100000;

	if ((Newdemo_size+100000) <  2000000000) {
		if (((int)(Newdemo_size)) < 500000) {
#ifndef MACINTOSH
			nm_messagebox(NULL, 1, TXT_OK, TXT_DEMO_NO_SPACE);
#else
			nm_messagebox(NULL, 1, TXT_OK, "Not enough space on current\ndrive to start demo recording.");
#endif
			return;
		}
	}

	Newdemo_num_written = 0;
	Newdemo_no_space=0;
	Newdemo_state = ND_STATE_RECORDING;
	outfile = fopen( DEMO_FILENAME, "wb" );

#ifndef MACINTOSH
	if (outfile == NULL && errno == ENOENT) {   //dir doesn't exist?
#else
	if (outfile == NULL) {                      //dir doesn't exist and no errno on mac!
#endif
		d_mkdir(DEMO_DIR); //try making directory
		outfile = fopen( DEMO_FILENAME, "wb" );
	}

	if (outfile == NULL)
	{
		nm_messagebox(NULL, 1, TXT_OK, "Cannot open demo temp file");
		Newdemo_state = ND_STATE_NORMAL;
	}
	else
		newdemo_record_start_demo();

}

char demoname_allowed_chars[] = "azAZ09__--";
void newdemo_stop_recording()
{
	newmenu_item m[6];
	int l, exit;
	static char filename[15] = "", *s;
	static ubyte tmpcnt = 0;
	ubyte cloaked = 0;
	char fullname[15+FILENAME_LEN] = DEMO_DIR;
	unsigned short byte_count = 0;

	exit = 0;

	nd_write_byte(ND_EVENT_EOF);
	nd_write_short(frame_bytes_written - 1);
	if (Game_mode & GM_MULTI) {
		for (l = 0; l < N_players; l++) {
			if (Players[l].flags & PLAYER_FLAGS_CLOAKED)
				cloaked |= (1 << l);
		}
		nd_write_byte(cloaked);
		nd_write_byte(ND_EVENT_EOF);
	} else {
		nd_write_short(ND_EVENT_EOF);
	}
	nd_write_short(ND_EVENT_EOF);
	nd_write_int(ND_EVENT_EOF);

	byte_count += 10;       // from frame_bytes_written

	nd_write_byte((byte)(f2ir(Players[Player_num].energy)));
	nd_write_byte((byte)(f2ir(Players[Player_num].shields)));
	nd_write_int(Players[Player_num].flags);        // be sure players flags are set
	nd_write_byte((byte)Primary_weapon);
	nd_write_byte((byte)Secondary_weapon);
	byte_count += 8;

	for (l = 0; l < MAX_PRIMARY_WEAPONS; l++)
		nd_write_short((short)Players[Player_num].primary_ammo[l]);

	for (l = 0; l < MAX_SECONDARY_WEAPONS; l++)
		nd_write_short((short)Players[Player_num].secondary_ammo[l]);
	byte_count += (sizeof(short) * (MAX_PRIMARY_WEAPONS + MAX_SECONDARY_WEAPONS));

	nd_write_byte(Players[Player_num].laser_level);
	byte_count++;

	if (Game_mode & GM_MULTI) {
		nd_write_byte((byte)N_players);
		byte_count++;
		for (l = 0; l < N_players; l++) {
			nd_write_string(Players[l].callsign);
			byte_count += (strlen(Players[l].callsign) + 2);
			nd_write_byte(Players[l].connected);
			if (Game_mode & GM_MULTI_COOP) {
				nd_write_int(Players[l].score);
				byte_count += 5;
			} else {
				nd_write_short((short)Players[l].net_killed_total);
				nd_write_short((short)Players[l].net_kills_total);
				byte_count += 5;
			}
		}
	} else {
		nd_write_int(Players[Player_num].score);
		byte_count += 4;
	}
	nd_write_short(byte_count);

	nd_write_byte(Current_level_num);
	nd_write_byte(ND_EVENT_EOF);

	l = ftell(outfile);
	fclose(outfile);
	outfile = NULL;
	Newdemo_state = ND_STATE_NORMAL;
	gr_palette_load( gr_palette );

	if (filename[0] != '\0') {
		int num, i = strlen(filename) - 1;
		char newfile[15];

		while (isdigit(filename[i])) {
			i--;
			if (i == -1)
				break;
		}
		i++;
		num = atoi(&(filename[i]));
		num++;
		filename[i] = '\0';
		sprintf (newfile, "%s%d", filename, num);
		strncpy(filename, newfile, 8);
		filename[8] = '\0';
	}

try_again:
	;

	Newmenu_allowed_chars = demoname_allowed_chars;
	if (!Newdemo_no_space) {
		m[0].type=NM_TYPE_INPUT; m[0].text_len = 8; m[0].text = filename;
		exit = newmenu_do( NULL, TXT_SAVE_DEMO_AS, 1, &(m[0]), NULL );
	} else if (Newdemo_no_space == 1) {
		m[ 0].type = NM_TYPE_TEXT; m[ 0].text = TXT_DEMO_SAVE_BAD;
		m[ 1].type = NM_TYPE_INPUT;m[ 1].text_len = 8; m[1].text = filename;
		exit = newmenu_do( NULL, NULL, 2, m, NULL );
	} else if (Newdemo_no_space == 2) {
		m[ 0].type = NM_TYPE_TEXT; m[ 0].text = TXT_DEMO_SAVE_NOSPACE;
		m[ 1].type = NM_TYPE_INPUT;m[ 1].text_len = 8; m[1].text = filename;
		exit = newmenu_do( NULL, NULL, 2, m, NULL );
	}
	Newmenu_allowed_chars = NULL;

	if (exit == -2) {                   // got bumped out from network menu
		char save_file[7+FILENAME_LEN];

		if (filename[0] != '\0') {
			strcpy(save_file, DEMO_DIR);
			strcat(save_file, filename);
			strcat(save_file, ".dem");
		} else
			sprintf (save_file, "%stmp%d.dem", DEMO_DIR, tmpcnt++);
		remove(save_file);
		rename(DEMO_FILENAME, save_file);
		return;
	}
	if (exit == -1) {               // pressed ESC
		remove(DEMO_FILENAME);      // might as well remove the file
		return;                     // return without doing anything
	}

	if (filename[0]==0) //null string
		goto try_again;

	//check to make sure name is ok
	for (s=filename;*s;s++)
		if (!isalnum(*s) && *s!='_') {
			nm_messagebox1(NULL, NULL,1,TXT_CONTINUE, TXT_DEMO_USE_LETTERS);
			goto try_again;
		}

	if (Newdemo_no_space)
		strcat(fullname, m[1].text);
	else
		strcat(fullname, m[0].text);
	strcat(fullname, ".dem");
	remove(fullname);
	rename(DEMO_FILENAME, fullname);
}

//returns the number of demo files on the disk
int newdemo_count_demos()
{
	FILEFINDSTRUCT find;
	int NumFiles=0;

	if( !FileFindFirst( DEMO_DIR "*.dem", &find ) ) {
		do {
			NumFiles++;
		} while( !FileFindNext( &find ) );
		FileFindClose();
	}

	if ( AltHogdir_initialized ) {
		char search_name[PATH_MAX + 5];
		strcpy(search_name, AltHogDir);
		strcat(search_name, "/" DEMO_DIR "*.dem");
		if( !FileFindFirst( search_name, &find ) ) {
			do {
				NumFiles++;
			} while( !FileFindNext( &find ) );
			FileFindClose();
		}
	}

	return NumFiles;
}

void newdemo_start_playback(char * filename)
{
	FILEFINDSTRUCT find;
	int rnd_demo = 0;
	char filename2[PATH_MAX+FILENAME_LEN] = DEMO_DIR;

#ifdef NETWORK
	change_playernum_to(0);
#endif
	First_time_playback=1;
	JasonPlaybackTotal=0;

	if (filename==NULL) {
		// Randomly pick a filename
		int NumFiles = 0, RandFileNum;
		rnd_demo = 1;

		NumFiles = newdemo_count_demos();

		if ( NumFiles == 0 ) {
			return;     // No files found!
		}
		RandFileNum = d_rand() % NumFiles;
		NumFiles = 0;
		if( !FileFindFirst( DEMO_DIR "*.dem", &find ) ) {
			do {
				if ( NumFiles==RandFileNum ) {
					filename = (char *)&find.name;
					break;
				}
				NumFiles++;
			} while( !FileFindNext( &find ) );
			FileFindClose();
		}

		if ( filename == NULL && AltHogdir_initialized ) {
			char search_name[PATH_MAX + 5];
			strcpy(search_name, AltHogDir);
			strcat(search_name, "/" DEMO_DIR "*.dem");
			if( !FileFindFirst( search_name, &find ) ) {
				do {
					if ( NumFiles==RandFileNum ) {
						filename = (char *)&find.name;
						break;
					}
					NumFiles++;
				} while( !FileFindNext( &find ) );
				FileFindClose();
			}
		}

		if ( filename==NULL) return;
	}

	if (!filename)
		return;

	strcat(filename2,filename);

	infile = fopen( filename2, "rb" );

	if (infile==NULL && AltHogdir_initialized) {
		strcpy(filename2, AltHogDir);
		strcat(filename2, "/" DEMO_DIR);
		strcat(filename2, filename);
		infile = fopen( filename2, "rb" );
	}

	if (infile==NULL) {
		mprintf( (0, "Error reading '%s'\n", filename ));
		return;
	}

	nd_bad_read = 0;
#ifdef NETWORK
	change_playernum_to(0);                 // force playernum to 0
#endif
	strncpy(nd_save_callsign, Players[Player_num].callsign, CALLSIGN_LEN);
	Viewer = ConsoleObject = &Objects[0];   // play properly as if console player
	if (newdemo_read_demo_start(rnd_demo)) {
		fclose(infile);
		return;
	}

	Game_mode = GM_NORMAL;
	Newdemo_state = ND_STATE_PLAYBACK;
	Newdemo_vcr_state = ND_STATE_PLAYBACK;
	Newdemo_old_cockpit = Cockpit_mode;
	Newdemo_size = filelength(fileno(infile));
	nd_bad_read = 0;
	Newdemo_at_eof = 0;
	NewdemoFrameCount = 0;
	Newdemo_players_cloaked = 0;
	playback_style = NORMAL_PLAYBACK;
	Function_mode = FMODE_GAME;
	Cockpit_3d_view[0] = CV_NONE;       //turn off 3d views on cockpit
	Cockpit_3d_view[1] = CV_NONE;       //turn off 3d views on cockpit
	newdemo_playback_one_frame();       // this one loads new level
	newdemo_playback_one_frame();       // get all of the objects to renderb game
}

void newdemo_stop_playback()
{
	fclose( infile );
	Newdemo_state = ND_STATE_NORMAL;
#ifdef NETWORK
	change_playernum_to(0);             //this is reality
#endif
	strncpy(Players[Player_num].callsign, nd_save_callsign, CALLSIGN_LEN);
	Cockpit_mode = Newdemo_old_cockpit;
	Game_mode = GM_GAME_OVER;
	Function_mode = FMODE_MENU;
	longjmp(LeaveGame,0);               // Exit game loop
}


#ifndef NDEBUG

#define BUF_SIZE 16384

void newdemo_strip_frames(char *outname, int bytes_to_strip)
{
	FILE *outfile;
	char *buf;
	int total_size, bytes_done, read_elems, bytes_back;
	int trailer_start, loc1, loc2, stop_loc, bytes_to_read;
	short last_frame_length;

	bytes_done = 0;
	total_size = filelength(fileno(infile));
	outfile = fopen(outname, "wb");
	if (outfile == NULL) {
		newmenu_item m[1];

		m[ 0].type = NM_TYPE_TEXT; m[ 0].text = "Can't open output file";
		newmenu_do( NULL, NULL, 1, m, NULL );
		newdemo_stop_playback();
		return;
	}
	buf = d_malloc(BUF_SIZE);
	if (buf == NULL) {
		newmenu_item m[1];

		m[ 0].type = NM_TYPE_TEXT; m[ 0].text = "Can't malloc output buffer";
		newmenu_do( NULL, NULL, 1, m, NULL );
		fclose(outfile);
		newdemo_stop_playback();
		return;
	}
	newdemo_goto_end();
	trailer_start = ftell(infile);
	fseek(infile, 11, SEEK_CUR);
	bytes_back = 0;
	while (bytes_back < bytes_to_strip) {
		loc1 = ftell(infile);
		//fseek(infile, -10, SEEK_CUR);
		//nd_read_short(&last_frame_length);
		//fseek(infile, 8 - last_frame_length, SEEK_CUR);
		newdemo_back_frames(1);
		loc2 = ftell(infile);
		bytes_back += (loc1 - loc2);
	}
	fseek(infile, -10, SEEK_CUR);
	nd_read_short(&last_frame_length);
	fseek(infile, -3, SEEK_CUR);
	stop_loc = ftell(infile);
	fseek(infile, 0, SEEK_SET);
	while (stop_loc > 0) {
		if (stop_loc < BUF_SIZE)
			bytes_to_read = stop_loc;
		else
			bytes_to_read = BUF_SIZE;
		read_elems = fread(buf, 1, bytes_to_read, infile);
		fwrite(buf, 1, read_elems, outfile);
		stop_loc -= read_elems;
	}
	stop_loc = ftell(outfile);
	fseek(infile, trailer_start, SEEK_SET);
	while ((read_elems = fread(buf, 1, BUF_SIZE, infile)) != 0)
		fwrite(buf, 1, read_elems, outfile);
	fseek(outfile, stop_loc, SEEK_SET);
	fseek(outfile, 1, SEEK_CUR);
	fwrite(&last_frame_length, 2, 1, outfile);
	fclose(outfile);
	newdemo_stop_playback();

}

#endif

object DemoRightExtra,DemoLeftExtra;
ubyte DemoDoRight=0,DemoDoLeft=0;

void nd_render_extras (ubyte which,object *obj)
{
	ubyte w=which>>4;
	ubyte type=which&15;

	if (which==255)
	{
		Int3(); // how'd we get here?
		do_cockpit_window_view(w,NULL,0,WBU_WEAPON,NULL);
		return;
	}

	if (w)
	{
		memcpy (&DemoRightExtra,obj,sizeof(object));  DemoDoRight=type;
	}
	else
	{
		memcpy (&DemoLeftExtra,obj,sizeof(object)); DemoDoLeft=type;
	}

}

void DoJasonInterpolate (fix recorded_time)
{
	fix the_delay;

	JasonPlaybackTotal+=FrameTime;

	if (!First_time_playback)
	{
		// get the difference between the recorded time and the playback time
		the_delay=(recorded_time - FrameTime);
		//mprintf ((0,"The delay=%d\n", f2i(the_delay)));
		if (the_delay >= f0_0)
		{
			stop_time();
			timer_delay(the_delay);
			start_time();
		}
		else
		{
			while (JasonPlaybackTotal > nd_recorded_total)
				if (newdemo_read_frame_information() == -1)
				{
					newdemo_stop_playback();
					return;
				}

			//the_delay = nd_recorded_total - JasonPlaybackTotal;
			//if (the_delay > f0_0)
			//	timer_delay(the_delay);
		}

	}

	First_time_playback=0;
}

#ifdef MACINTOSH
#pragma global_optimizer reset
#endif

