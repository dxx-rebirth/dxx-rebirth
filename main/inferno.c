/* $Id: inferno.c,v 1.71 2004-04-15 07:34:28 btb Exp $ */
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
 * inferno.c: Entry point of program (main procedure)
 *
 * After main initializes everything, most of the time is spent in the loop
 * while (Function_mode != FMODE_EXIT)
 * In this loop, the main menu is brought up first.
 *
 * main() for Inferno
 *
 * Old Log:
 * Revision 1.1  1995/12/05  15:57:49  allender
 * Initial revision
 *
 * Revision 1.19  1995/11/07  17:03:12  allender
 * added splash screen for descent contest
 *
 * Revision 1.18  1995/10/31  10:22:22  allender
 * shareware stuff
 *
 * Revision 1.17  1995/10/18  01:53:07  allender
 * mouse click to leave do not distribute screen
 *
 * Revision 1.16  1995/10/17  12:00:12  allender
 * mouse click gets past endgame screen
 *
 * Revision 1.15  1995/10/12  17:40:12  allender
 * read config file after digi initialized
 *
 * Revision 1.14  1995/10/05  10:38:22  allender
 * changed key_getch at exit to be key_inkey and moved
 * mouse init until after macintosh windowing init call
 *
 * Revision 1.13  1995/09/18  17:01:04  allender
 * put gr_init call before render buffer stuff
 *
 * Revision 1.12  1995/08/31  15:50:53  allender
 * call init for appletalk, and change name of intro screens
 *
 * Revision 1.11  1995/08/26  16:26:19  allender
 * whole bunch 'o stuff!!
 *
 * Revision 1.10  1995/07/12  21:48:18  allender
 * removed Int3 from beginning of program
 *
 * Revision 1.9  1995/07/05  16:45:48  allender
 * removed hide_cursor call
 *
 * Revision 1.8  1995/06/20  16:44:57  allender
 * game now renders in 640x480 at all times.  Changed code
 * to call game_init_render_buffers with right params
 *
 * Revision 1.7  1995/06/13  13:07:55  allender
 * change macintosh initialzation.  Mac windows now init'ed through gr_init.
 *
 * Revision 1.6  1995/06/08  16:36:53  allender
 * "ifdef" profile include
 *
 * Revision 1.5  1995/06/07  08:08:18  allender
 * dont' make memory info show at end of program
 *
 * Revision 1.4  1995/06/02  07:47:40  allender
 * removed network initialzation for now
 *
 * Revision 1.3  1995/05/26  06:54:52  allender
 * put digi_init after timer and key stuff since I was testing stuff
 * that needed the keyboard handler installed
 *
 * Revision 1.2  1995/05/19  11:28:09  allender
 * removed printf
 *
 * Revision 1.1  1995/05/16  15:26:39  allender
 * Initial revision
 *
 * Revision 2.36  1996/01/05  16:52:16  john
 * Improved 3d stuff.
 *
 * Revision 2.35  1995/10/07  13:20:06  john
 * Added new modes for LCDBIOS, also added support for -JoyNice,
 * and added Shift+F1-F4 to controls various stereoscopic params.
 *
 * Revision 2.34  1995/06/26  11:30:57  john
 * Made registration/copyright screen go away after 5 minutes.
 *
 * Revision 2.33  1995/05/31  14:26:55  unknown
 * Fixed ugly spacing.
 *
 * Revision 2.32  1995/05/26  16:15:28  john
 * Split SATURN into define's for requiring cd, using cd, etc.
 * Also started adding all the Rockwell stuff.
 *
 * Revision 2.31  1995/05/11  13:30:01  john
 * Changed 3dbios detection to work like Didde Kim wanted it to.
 *
 * Revision 2.30  1995/05/08  13:53:50  john
 * Added code to read vipport environemnt variable.
 *
 * Revision 2.29  1995/05/08  11:26:18  john
 * Reversed eyes in 3dmax mode.
 *
 * Revision 2.28  1995/05/08  11:24:06  john
 * Made 3dmax work like Kasan wants it to.
 *
 * Revision 2.27  1995/04/23  16:06:25  john
 * Moved rinvul into modem/null modem menu.
 *
 * Revision 2.26  1995/04/12  13:39:26  john
 * Fixed bug with -lowmem not working.
 *
 * Revision 2.25  1995/04/09  14:43:00  john
 * Made Dynamic sockets not print Msockets for help.
 *
 * Revision 2.24  1995/04/07  16:11:33  john
 * Fixed problem with VFX display when using setup.
 *
 * Revision 2.23  1995/04/06  15:40:51  john
 * Synced VFX with setup #'s.
 *
 * Revision 2.22  1995/04/06  12:12:53  john
 * Fixed some bugs with 3dmax.
 *
 * Revision 2.21  1995/03/30  16:36:51  mike
 * text localization.
 *
 * Revision 2.20  1995/03/29  15:33:52  john
 * Added code to parse descent.net file.
 *
 * Revision 2.19  1995/03/28  20:08:21  john
 * Took away alternate server thing.
 *
 * Revision 2.18  1995/03/27  09:43:08  john
 * Added VR Settings in config file.
 *
 * Revision 2.17  1995/03/23  19:02:21  john
 * Added descent.net file use.
 *
 * Revision 2.16  1995/03/23  12:25:11  john
 * Moved IPX stuff into BIOS lib.
 *
 * Revision 2.15  1995/03/21  16:52:34  john
 * Added 320x100.
 *
 * Revision 2.14  1995/03/21  14:40:33  john
 * Ifdef'd out the NETWORK code.
 *
 * Revision 2.13  1995/03/16  23:13:35  john
 * Fixed bug with piggy paging in bitmap not checking for disk
 * error, hence bogifying textures if you pull the CD out.
 *
 * Revision 2.12  1995/03/16  21:45:22  john
 * Made all paged modes have incompatible menus!
 *
 * Revision 2.11  1995/03/15  15:19:34  john
 * Took out code that changes to exe dir.
 *
 * Revision 2.10  1995/03/15  14:33:37  john
 * Added code to force the Descent CD-rom in the drive.
 *
 * Revision 2.9  1995/03/15  11:41:27  john
 * Better Saturn CD-ROM support.
 *
 * Revision 2.8  1995/03/14  18:24:46  john
 * Force Destination Saturn to use CD-ROM drive.
 *
 * Revision 2.7  1995/03/14  16:22:35  john
 * Added cdrom alternate directory stuff.
 *
 * Revision 2.6  1995/03/13  15:17:19  john
 * Added alternate hogfile directory.
 *
 * Revision 2.5  1995/03/10  13:05:35  john
 * Added code so that palette is correct for VFX1 helmets.
 *
 * Revision 2.4  1995/03/07  15:12:43  john
 * Fixed VFX,3dmax support.
 *
 * Revision 2.3  1995/03/07  14:19:35  mike
 * More destination saturn stuff.
 *
 * Revision 2.2  1995/03/06  16:47:34  mike
 * destination saturn
 *
 * Revision 2.1  1995/03/06  15:24:06  john
 * New screen techniques.
 *
 * Revision 2.0  1995/02/27  11:31:29  john
 * New version 2.0, which has no anonymous unions, builds with
 * Watcom 10.0, and doesn't require parsing BITMAPS.TBL.
 *
 * Revision 1.295  1995/02/23  12:02:14  john
 * Made mono  windows smaller.
 *
 * Revision 1.294  1995/02/16  17:35:00  john
 * Added code to allow dynamic socket changing.
 *
 * Revision 1.293  1995/02/14  19:29:29  john
 * Locked down critical error handler.
 *
 * Revision 1.292  1995/02/14  15:29:20  john
 * Added CR-LF to last line of menu help text.
 *
 * Revision 1.291  1995/02/14  11:39:01  john
 * Added polled/bios joystick readers.
 *
 * Revision 1.290  1995/02/13  20:35:03  john
 * Lintized
 *
 * Revision 1.289  1995/02/11  16:20:02  john
 * Added code to make the default mission be the one last played.
 *
 * Revision 1.288  1995/02/11  15:54:13  rob
 * changed cinvul to rinvul.
 *
 * Revision 1.287  1995/02/11  14:48:43  rob
 * Added max of 314 seconds to control invul. times
 *
 * Revision 1.286  1995/02/11  12:42:01  john
 * Added new song method, with FM bank switching..
 *
 * Revision 1.285  1995/02/11  11:36:11  rob
 * Added cinvul option.
 *
 * Revision 1.284  1995/02/10  16:07:45  matt
 * Took 'registered' out of printed info at startup
 *
 * Revision 1.283  1995/02/09  22:00:59  john
 * Added i-glasses tracking.
 *
 * Revision 1.282  1995/02/02  11:11:27  john
 * Added -nocyberman switch.
 *
 * Revision 1.281  1995/02/01  16:35:14  john
 * Linted.
 *
 * Revision 1.280  1995/01/31  02:04:25  matt
 * Fixed up cmdline help
 *
 * Revision 1.279  1995/01/30  16:25:55  john
 * Put back in graphical screen at program end.
 *
 * Revision 1.278  1995/01/28  17:05:50  matt
 * Changed imbedded copyright to use comma instead of hyphen
 *
 * Revision 1.277  1995/01/28  15:57:26  john
 * Made joystick calibration be only when wrong detected in
 * menu or joystick axis changed.
 *
 * Revision 1.276  1995/01/25  14:37:49  john
 * Made joystick only prompt for calibration once...
 *
 * Revision 1.275  1995/01/24  18:21:00  john
 * Added Adam's text warning.
 *
 * Revision 1.274  1995/01/22  15:57:20  john
 * Took out code that printed warning out as game exited.
 *
 * Revision 1.273  1995/01/22  13:31:35  matt
 * Added load of mission 0, so there's always a default mission
 *
 * Revision 1.272  1995/01/19  17:00:41  john
 * Made save game work between levels.
 *
 * Revision 1.271  1995/01/18  11:47:57  adam
 * changed copyright notice
 *
 * Revision 1.270  1995/01/15  13:42:42  john
 * Moved low_mem cutoff higher.
 *
 * Revision 1.269  1995/01/12  18:53:50  john
 * Put ifdef EDITOR around the code that checked for
 * a 800x600 mode, because this trashed some people's
 * computers (maybe) causing the mem allocation error in
 * mouse.c that many users reported.
 *
 * Revision 1.268  1995/01/12  11:41:42  john
 * Added external control reading.
 *
 * Revision 1.267  1995/01/06  10:26:55  john
 * Added -nodoscheck command line switch.
 *
 * Revision 1.266  1995/01/05  16:59:30  yuan
 * Don't show orderform in editor version.
 *
 * Revision 1.265  1994/12/28  15:33:51  john
 * Added -slowjoy option.
 *
 * Revision 1.264  1994/12/15  16:44:15  matt
 * Added trademark notice
 *
 * Revision 1.263  1994/12/14  20:13:59  john
 * Reduced physical mem requments to 2 MB.
 *
 * Revision 1.262  1994/12/14  19:06:17  john
 * Lowered physical memory requments to 2 MB.
 *
 * Revision 1.261  1994/12/14  09:41:29  allender
 * change to drive and directory (if needed) of command line invocation
 * so descent can be started from anywhere
 *
 * Revision 1.260  1994/12/13  19:08:59  john
 * Updated memory requirements.
 *
 * Revision 1.259  1994/12/13  17:30:33  john
 * Made the timer rate be changed right after initializing it.
 *
 * Revision 1.258  1994/12/13  02:46:25  matt
 * Added imbedded copyright
 *
 * Revision 1.257  1994/12/13  02:06:46  john
 * Added code to check stack used by descent... initial
 * check showed stack used 35k/50k, so we decided it wasn't
 * worth pursuing any more.
 *
 * Revision 1.256  1994/12/11  23:17:54  john
 * Added -nomusic.
 * Added RealFrameTime.
 * Put in a pause when sound initialization error.
 * Made controlcen countdown and framerate use RealFrameTime.
 *
 * Revision 1.255  1994/12/10  00:56:51  matt
 * Added -nomusic to command-line help
 *
 * Revision 1.254  1994/12/08  11:55:11  john
 * Took out low memory print.
 *
 * Revision 1.253  1994/12/08  11:51:00  john
 * Made strcpy only copy corect number of chars,.
 *
 * Revision 1.252  1994/12/08  00:38:29  matt
 * Cleaned up banner messages
 *
 * Revision 1.251  1994/12/07  19:14:52  matt
 * Cleaned up command-line options and command-line help message
 *
 * Revision 1.250  1994/12/06  19:33:28  john
 * Fixed text of message to make more sense.
 *
 * Revision 1.249  1994/12/06  16:30:55  john
 * Neatend mem message,..
 *
 * Revision 1.248  1994/12/06  16:17:35  john
 * Added better mem checking/printing.
 *
 * Revision 1.247  1994/12/06  14:14:37  john
 * Added code to set low mem based on memory.
 *
 * Revision 1.246  1994/12/05  12:29:09  allender
 * removed ifdefs around -norun option
 *
 * Revision 1.245  1994/12/05  00:03:30  matt
 * Added -norun option to exit after writing pig
 *
 * Revision 1.244  1994/12/04  14:47:01  john
 * MAde the intro and menu be the same song.
 *
 * Revision 1.243  1994/12/04  14:36:42  john
 * Added menu music.
 *
 * Revision 1.242  1994/12/02  13:50:17  yuan
 * Localization.
 *
 * Revision 1.241  1994/12/01  17:28:30  adam
 * added end-shareware stuff
 *
 * Revision 1.240  1994/11/30  12:10:57  adam
 * added support for PCX titles/brief screens
 *
 * Revision 1.239  1994/11/29  15:47:33  matt
 * Moved error_init to start of game, so error message prints last
 *
 * Revision 1.238  1994/11/29  14:19:22  jasen
 * reduced dos mem requirments.
 *
 * Revision 1.237  1994/11/29  03:46:35  john
 * Added joystick sensitivity; Added sound channels to detail menu.  Removed -maxchannels
 * command line arg.
 *
 * Revision 1.236  1994/11/29  02:50:18  john
 * Increased the amount a joystick has to be off before
 * asking if they want to recalibrate their joystick.
 *
 * Revision 1.235  1994/11/29  02:01:29  john
 * Corrected some of the Descent command line help items.
 *
 * Revision 1.234  1994/11/29  01:39:56  john
 * Fixed minor bug with vfx_light help not wrapping correctly.
 *
 * Revision 1.233  1994/11/28  21:34:17  john
 * Reduced dos mem rqment to 70k.
 *
 * Revision 1.232  1994/11/28  21:20:38  john
 * First version with memory checking.
 *
 * Revision 1.231  1994/11/28  20:06:21  rob
 * Removed old serial param command line options.
 * Added -noserial and -nonetwork to help listing.
 *
 * Revision 1.230  1994/11/27  23:15:24  matt
 * Made changes for new mprintf calling convention
 *
 * Revision 1.229  1994/11/27  20:50:51  matt
 * Don't set mem stuff if no debug
 *
 * Revision 1.228  1994/11/27  18:46:21  matt
 * Cleaned up command-line switches a little
 *
 * Revision 1.227  1994/11/21  17:48:00  matt
 * Added text to specifiy whether shareware or registered version
 *
 * Revision 1.226  1994/11/21  14:44:20  john
 * Fixed some bugs with setting volumes even when -nosound was used. Duh!
 *
 * Revision 1.225  1994/11/21  13:53:42  matt
 * Took out dos extender copyright
 *
 * Revision 1.224  1994/11/21  09:46:54  john
 * Added -showmeminfo parameter.
 *
 * Revision 1.223  1994/11/20  22:12:05  mike
 * Make some stuff dependent on SHAREWARE.
 *
 * Revision 1.222  1994/11/20  21:14:09  john
 * Changed -serial to -noserial.  MAde a 1 sec delay
 * before leaving title screen.  Clear keyboard buffer
 * before asking for player name.
 *
 * Revision 1.221  1994/11/19  15:20:20  mike
 * rip out unused code and data
 *
 * Revision 1.220  1994/11/17  19:14:29  adam
 * prevented order screen from coming up when -notitles is used
 *
 * Revision 1.219  1994/11/16  11:34:39  john
 * Added -nottitle switch.
 *
 * Revision 1.218  1994/11/16  10:05:53  john
 * Added verbose messages.
 *
 * Revision 1.217  1994/11/15  20:12:34  john
 * Added back in inferno and parallax screens.
 *
 * Revision 1.216  1994/11/15  18:35:30  john
 * Added verbose setting.
 *
 * Revision 1.215  1994/11/15  17:47:44  john
 * Added ordering info screen.
 *
 * Revision 1.214  1994/11/15  08:57:44  john
 * Added MS-DOS version checking and -nonetwork option.
 *
 * Revision 1.213  1994/11/15  08:34:32  john
 * Added better error messages for IPX init.
 *
 * Revision 1.212  1994/11/14  20:14:18  john
 * Fixed some warnings.
 *
 * Revision 1.211  1994/11/14  19:50:49  john
 * Added joystick cal values to descent.cfg.
 *
 * Revision 1.210  1994/11/14  17:56:44  allender
 * make call to ReadConfigFile at startup
 *
 * Revision 1.209  1994/11/14  11:41:55  john
 * Fixed bug with editor/game sequencing.
 *
 * Revision 1.208  1994/11/13  17:05:11  john
 * Made the callsign entry be a list box and gave the ability
 * to delete players.
 *
 * Revision 1.207  1994/11/13  15:39:22  john
 * Added critical error handler to game.  Took out -editor command line
 * option because it didn't work anymore and wasn't worth fixing.  Made scores
 * not use MINER enviroment variable on release version, and made scores
 * not print an error if there is no descent.hi.
 *
 * Revision 1.206  1994/11/10  20:53:29  john
 * Used new sound install parameters.
 *
 * Revision 1.205  1994/11/10  11:07:52  mike
 * Set default detail level.
 *
 * Revision 1.204  1994/11/09  13:45:43  matt
 * Made -? work again for help
 *
 * Revision 1.203  1994/11/09  10:55:58  matt
 * Cleaned up initialization for editor -> game transitions
 *
 * Revision 1.202  1994/11/07  21:35:47  matt
 * Use new function iff_read_into_bitmap()
 *
 * Revision 1.201  1994/11/05  17:22:16  john
 * Fixed lots of sequencing problems with newdemo stuff.
 *
 * Revision 1.200  1994/11/05  14:05:44  john
 * Fixed fade transitions between all screens by making
 * gr_palette_fade_in and out keep track of whether the palette is
 * faded in or not.  Then, wherever the code needs to fade out, it
 * just calls gr_palette_fade_out and it will fade out if it isn't
 * already.  The same with fade_in.
 * This eliminates the need for all the flags like Menu_fade_out,
 * game_fade_in palette, etc.
 *
 * Revision 1.199  1994/11/04  14:36:30  allender
 * change Auto_demo meaning to mean autostart from menu only.  Use
 * FindArgs when searching for AutoDemo from command line.  also,
 * set N_Players to 1 when starting in editor mode.
 *
 * Revision 1.198  1994/11/02  11:59:49  john
 * Moved menu out of game into inferno main loop.
 *
 * Revision 1.197  1994/11/01  17:57:39  mike
 * -noscreens option to bypass all screens.
 *
 * Revision 1.196  1994/10/28  15:42:34  allender
 * don't register player if Autodemo is on
 *
 * Revision 1.195  1994/10/28  10:58:01  matt
 * Added copyright notice for DOS4GW
 *
 * Revision 1.194  1994/10/20  21:26:48  matt
 * Took out old serial name/number code, and put up message if this
 * is a marked version.
 *
 * Revision 1.193  1994/10/19  09:52:14  allender
 * Print out who descent.exe belongs to if descent.exe is stamped.
 *
 * Revision 1.192  1994/10/18  16:43:05  allender
 * Added check for identifier stamp and time after which descent will
 * no longer run.
 *
 * Revision 1.191  1994/10/17  13:07:17  john
 * Moved the descent.cfg info into the player config file.
 *
 * Revision 1.190  1994/10/04  10:26:31  matt
 * Support new menu fade in
 *
 * Revision 1.189  1994/10/03  22:58:46  matt
 * Changed some values of game_mode
 *
 * Revision 1.188  1994/10/03  18:55:39  rob
 * Changed defaults for com port settings.
 *
 * Revision 1.187  1994/10/03  13:34:47  matt
 * Added new (and hopefully better) game sequencing functions
 *
 * Revision 1.186  1994/09/30  12:37:28  john
 * Added midi,digi volume to configuration.
 *
 * Revision 1.185  1994/09/30  10:08:48  john
 * Changed sound stuff... made it so the reseting card doesn't hang,
 * made volume change only if sound is installed.
 *
 * Revision 1.184  1994/09/28  17:25:00  matt
 * Added first draft of game save/load system
 *
 * Revision 1.183  1994/09/28  16:18:23  john
 * Added capability to play midi song.
 *
 * Revision 1.182  1994/09/28  11:31:18  john
 * Made text output unbuffered.
 *
 * Revision 1.181  1994/09/27  19:23:44  john
 * Added -nojoystick and -nomouse
 *
 * Revision 1.180  1994/09/24  16:55:29  rob
 * No longer open COM port immediately upon program start.
 * No longer set Network_active is serial_active is set.
 *
 * Revision 1.179  1994/09/24  14:16:30  mike
 * Support new game mode constants.
 *
 * Revision 1.178  1994/09/22  17:52:31  rob
 * Added Findargs hooks for -serial, -speed, and -com.
 *
 * Revision 1.177  1994/09/22  16:14:11  john
 * Redid intro sequecing.
 *
 * Revision 1.176  1994/09/21  16:32:58  john
 * Made mouse and keyboard init after bm_init. Why?
 * Because it seems to work better under virtual
 * memory.
 *
 * Revision 1.175  1994/09/21  16:27:52  john
 * Added mouse_init
 *
 * Revision 1.174  1994/09/20  15:14:10  matt
 * New message for new VFX switches
 *
 * Revision 1.173  1994/09/16  16:14:27  john
 * Added acrade sequencing.
 *
 * Revision 1.172  1994/09/16  11:49:52  john
 * Added first version of arcade joystick support;
 * Also fixed some bugs in kconfig.c, such as reading non-present
 * joysticks, which killed frame rate, and not reading key_down_time
 * when in slide mode or bank mode.
 *
 * Revision 1.171  1994/09/15  16:11:35  john
 * Added support for VFX1 head tracking. Fixed bug with memory over-
 * write when using stereo mode.
 *
 * Revision 1.170  1994/09/12  19:38:23  john
 * Made some stuff that prints to the DOS screen go to the
 * mono instead, since it really is debugging info.
 *
 * Revision 1.169  1994/08/29  21:18:28  john
 * First version of new keyboard/oystick remapping stuff.
 *
 * Revision 1.168  1994/08/26  13:02:00  john
 * Put high score system in.
 *
 * Revision 1.167  1994/08/24  19:00:23  john
 * Changed key_down_time to return fixed seconds instead of
 * milliseconds.
 *
 * Revision 1.166  1994/08/18  16:24:20  john
 * changed socket to channel in text.
 *
 * Revision 1.165  1994/08/18  16:16:51  john
 * Added support for different sockets.
 *
 * Revision 1.164  1994/08/18  10:47:53  john
 * *** empty log message ***
 *
 * Revision 1.163  1994/08/12  09:15:54  john
 * *** empty log message ***
 *
 * Revision 1.162  1994/08/12  03:11:19  john
 * Made network be default off; Moved network options into
 * main menu.  Made starting net game check that mines are the
 * same.
 *
 * Revision 1.161  1994/08/10  19:57:05  john
 * Changed font stuff; Took out old menu; messed up lots of
 * other stuff like game sequencing messages, etc.
 *
 * Revision 1.160  1994/08/05  16:30:23  john
 * Added capability to turn off network.
 *
 * Revision 1.159  1994/08/04  19:42:51  matt
 * Moved serial number & name (and version name) from inferno.c to inferno.ini
 *
 * Revision 1.158  1994/08/03  10:30:23  matt
 * Change cybermaxx switches, updated command-line help, and added serial number system
 *
 * Revision 1.157  1994/07/29  18:30:10  matt
 * New parms (lack of parms, actually) for g3_init()
 *
 * Revision 1.156  1994/07/24  00:39:25  matt
 * Added more text to TEX file; make NewGame() take a start level; made game
 * load/save menus use open/close window funcs.
 *
 * Revision 1.155  1994/07/21  21:31:27  john
 * First cheapo version of VictorMaxx tracking.
 *
 * Revision 1.154  1994/07/21  18:15:34  matt
 * Ripped out a bunch of unused stuff
 *
 * Revision 1.153  1994/07/21  17:59:10  matt
 * Cleaned up initial mode game/editor code
 *
 * Revision 1.152  1994/07/21  13:11:19  matt
 * Ripped out remants of old demo system, and added demo only system that
 * disables object movement and game options from menu.
 *
 * Revision 1.151  1994/07/20  15:58:27  john
 * First installment of ipx stuff.
 *
 * Revision 1.150  1994/07/15  16:04:24  matt
 * Changed comment for milestone 3 version
 *
 * Revision 1.149  1994/07/15  13:59:24  matt
 * Fixed stupid mistake I make in the last revision
 *
 * Revision 1.148  1994/07/15  13:20:15  matt
 * Updated comand-line help
 *
 * Revision 1.147  1994/07/14  23:29:43  matt
 * Open two mono debug messages, one for errors & one for spew
 *
 * Revision 1.146  1994/07/09  22:48:05  matt
 * Added localizable text
 *
 * Revision 1.145  1994/07/02  13:49:47  matt
 * Cleaned up includes
 *
 * Revision 1.144  1994/06/30  20:04:43  john
 * Added -joydef support.
 *
 * Revision 1.143  1994/06/24  17:01:44  john
 * Add VFX support; Took Game Sequencing, like EndGame and stuff and
 * took it out of game.c and into gameseq.c
 *
 */

#ifdef HAVE_CONFIG_H
#include <conf.h>
#endif

char copyright[] = "DESCENT II  COPYRIGHT (C) 1994-1996 PARALLAX SOFTWARE CORPORATION";

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>

#ifdef __unix__
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#endif

#include "pstypes.h"
#include "strutil.h"
#include "console.h"
#include "pa_enabl.h"       //$$POLY_ACC
#include "gr.h"
#include "fix.h"
#include "vecmat.h"
#include "mono.h"
#include "key.h"
#include "timer.h"
#include "3d.h"
#include "bm.h"
#include "inferno.h"
#include "error.h"
#include "game.h"
#include "segment.h"		//for Side_to_verts
#include "u_mem.h"
#include "segpoint.h"
#include "screens.h"
#include "texmap.h"
#include "texmerge.h"
#include "menu.h"
#include "wall.h"
#include "polyobj.h"
#include "effects.h"
#include "digi.h"
#include "iff.h"
#include "pcx.h"
#include "palette.h"
#include "args.h"
#include "sounds.h"
#include "titles.h"
#include "player.h"
#include "text.h"
#include "newdemo.h"
#ifdef NETWORK
#include "network.h"
#include "modem.h"
#endif
#include "gamefont.h"
#include "kconfig.h"
#include "mouse.h"
#include "joy.h"
#include "newmenu.h"
#include "desc_id.h"
#include "config.h"
#include "joydefs.h"
#include "multi.h"
#include "songs.h"
#include "cfile.h"
#include "gameseq.h"
#include "gamepal.h"
#include "mission.h"
#include "movie.h"
#include "compbit.h"
#include "d_io.h"

// #  include "3dfx_des.h"

//added on 9/30/98 by Matt Mueller for selectable automap modes
#include "automap.h"
//end addition -MM

#include "../texmap/scanline.h" //for select_tmap -MM

#if defined(POLY_ACC)
#include "poly_acc.h"
extern int Current_display_mode;        //$$ there's got to be a better way than hacking this.
#endif

#ifdef EDITOR
#include "editor/editor.h"
#include "editor/kdefs.h"
#include "ui.h"
#endif

#ifndef __MSDOS__
#include <SDL.h>
#endif

#include "vers_id.h"

void mem_init(void);
void arch_init(void);
void arch_init_start(void);

//Current version number

ubyte Version_major = 1;		//FULL VERSION
ubyte Version_minor = 2;

//static const char desc_id_checksum_str[] = DESC_ID_CHKSUM_TAG "0000"; // 4-byte checksum
char desc_id_exit_num = 0;

int Function_mode=FMODE_MENU;		//game or editor?
int Screen_mode=-1;					//game screen or editor screen?

//--unused-- grs_bitmap Inferno_bitmap_title;

int WVIDEO_running=0;		//debugger can set to 1 if running

#ifdef EDITOR
int Inferno_is_800x600_available = 0;
#endif

//--unused-- int Cyberman_installed=0;			// SWIFT device present
ubyte CybermouseActive=0;

int __far descent_critical_error_handler( unsigned deverr, unsigned errcode, unsigned __far * devhdr );

void check_joystick_calibration(void);

void show_order_form(void);

//--------------------------------------------------------------------------

extern int piggy_low_memory;


int descent_critical_error = 0;
unsigned descent_critical_deverror = 0;
unsigned descent_critical_errcode = 0;

extern int Network_allow_socket_changes;

extern void vfx_set_palette_sub(ubyte *);

extern int VR_low_res;

extern int Config_vr_type;
extern int Config_vr_resolution;
extern int Config_vr_tracking;

#define LINE_LEN	100

//read help from a file & print to screen
void print_commandline_help()
{
	CFILE *ifile;
	int have_binary=0;
	char line[LINE_LEN];

	ifile = cfopen("help.tex","rb");
	if (!ifile) {
		ifile = cfopen("help.txb","rb");
		if (!ifile)
			Warning("Cannot load help text file.");
		have_binary = 1;
	}

	if (ifile)
	{

		while (cfgets(line,LINE_LEN,ifile)) {

			if (have_binary) {
				int i;
				for (i = 0; i < strlen(line) - 1; i++) {
					encode_rotate_left(&(line[i]));
					line[i] = line[i] ^ BITMAP_TBL_XOR;
					encode_rotate_left(&(line[i]));
				}
			}

			if (line[0] == ';')
				continue;		//don't show comments

			printf("%s",line);

		}

		cfclose(ifile);

	}

//	printf( " Diagnostic:\n\n");
//	printf( "  -emul           %s\n", "Certain video cards need this option in order to run game");
//	printf(	"  -ddemul         %s\n", "If -emul doesn't work, use this option");
//	printf( "\n");
#ifdef EDITOR
	printf( " Editor Options:\n\n");
	printf( "  -autoload <file>%s\n", "Autoload a level in the editor");
	printf( "  -hoarddata      %s\n","FIXME: Undocumented");
//	printf( "  -nobm           %s\n","FIXME: Undocumented");
	printf( "\n");
#endif
	printf( " D2X Options:\n\n");
	printf( "  -noredundancy   %s\n", "Do not send messages when picking up redundant items in multi");
	printf( "  -shortpackets   %s\n", "Set shortpackets to default as on");
	printf( "  -maxfps <n>     %s\n", "Set maximum framerate (1-100)");
	printf( "  -notitles       %s\n", "Do not show titlescreens on startup");
	printf( "  -hogdir <dir>   %s\n", "set shared data directory to <dir>");
#ifdef __unix__
	printf( "  -nohogdir       %s\n", "don't try to use shared data directory");
	printf( "  -userdir <dir>  %s\n", "set user dir to <dir> instead of $HOME/.d2x");
#endif
	printf( "  -ini <file>     %s\n", "option file (alternate to command line), defaults to d2x.ini");
	printf( "  -autodemo       %s\n", "Start in demo mode");
	printf( "  -bigpig         %s\n","FIXME: Undocumented");
	printf( "  -bspgen         %s\n","FIXME: Undocumented");
//	printf( "  -cdproxy        %s\n","FIXME: Undocumented");
#ifndef NDEBUG
	printf( "  -checktime      %s\n","FIXME: Undocumented");
	printf( "  -showmeminfo    %s\n","FIXME: Undocumented");
#endif
//	printf( "  -codereadonly   %s\n","FIXME: Undocumented");
//	printf( "  -cyberimpact    %s\n","FIXME: Undocumented");
	printf( "  -debug          %s\n","Enable very verbose output");
//	printf( "  -debugmode      %s\n","FIXME: Undocumented");
//	printf( "  -disallowgfx    %s\n","FIXME: Undocumented");
//	printf( "  -disallowreboot %s\n","FIXME: Undocumented");
//	printf( "  -dynamicsockets %s\n","FIXME: Undocumented");
//	printf( "  -forcegfx       %s\n","FIXME: Undocumented");
#ifdef SDL_INPUT
	printf( "  -grabmouse      %s\n","Keeps the mouse from wandering out of the window");
#endif
//	printf( "  -hw_3dacc       %s\n","FIXME: Undocumented");
#ifndef RELEASE
	printf( "  -invulnerability %s\n","Make yourself invulnerable");
#endif
	printf( "  -ipxnetwork <num> %s\n","Use IPX network number <num>");
	printf( "  -jasen          %s\n","FIXME: Undocumented");
	printf( "  -joyslow        %s\n","FIXME: Undocumented");
#ifdef NETWORK
	printf( "  -kali           %s\n","use Kali for networking");
#endif
//	printf( "  -logfile        %s\n","FIXME: Undocumented");
	printf( "  -lowresmovies   %s\n","Play low resolution movies if available (for slow machines)");
#if defined(EDITOR) || !defined(MACDATA)
	printf( "  -macdata        %s\n","Read (and, for editor, write) mac data files (swap colors)");
#endif
//	printf( "  -memdbg         %s\n","FIXME: Undocumented");
//	printf( "  -monodebug      %s\n","FIXME: Undocumented");
	printf( "  -nocdrom        %s\n","FIXME: Undocumented");
#ifdef __DJGPP__
	printf( "  -nocyberman     %s\n","FIXME: Undocumented");
#endif
#ifndef NDEBUG
	printf( "  -nofade         %s\n","Disable fades");
#endif
#ifdef NETWORK
	printf( "  -nomatrixcheat  %s\n","FIXME: Undocumented");
	printf( "  -norankings     %s\n","Disable multiplayer ranking system");
	printf( "  -packets <num>  %s\n","Specifies the number of packets per second\n");
//	printf( "  -showaddress    %s\n","FIXME: Undocumented");
	printf( "  -socket         %s\n","FIXME: Undocumented");
#endif
#if !defined(MACINTOSH) && !defined(WINDOWS)
	printf( "  -nomixer        %s\n","Don't crank music volume");
//	printf( "  -superhires     %s\n","Allow higher-resolution modes");
#endif
//	printf( "  -nomodex        %s\n","FIXME: Undocumented");
#ifndef RELEASE
	printf( "  -nomovies       %s\n","Don't play movies");
	printf( "  -noscreens      %s\n","Skip briefing screens");
#endif
#if !defined(SHAREWARE) || ( defined(SHAREWARE) && defined(APPLE_DEMO) )
	printf( "  -noredbook      %s\n","Disable redbook audio");
#endif
	printf( "  -norun          %s\n","Bail out after initialization");
//	printf( "  -ordinaljoy     %s\n","FIXME: Undocumented");
//	printf( "  -rtscts         %s\n","Same as -ctsrts");
//	printf( "  -semiwin        %s\n","Use non-fullscreen mode");
//	printf( "  -specialdevice  %s\n","FIXME: Undocumented");
#ifdef TACTILE
	printf( "  -stickmag       %s\n","FIXME: Undocumented");
#endif
//	printf( "  -stopwatch      %s\n","FIXME: Undocumented");
	printf( "  -subtitles      %s\n","Turn on movie subtitles (English-only)");
//	printf( "  -sysram         %s\n","FIXME: Undocumented");
	printf( "  -text <file>    %s\n","Specify alternate .tex file");
//	printf( "  -tsengdebug1    %s\n","FIXME: Undocumented");
//	printf( "  -tsengdebug2    %s\n","FIXME: Undocumented");
//	printf( "  -tsengdebug3    %s\n","FIXME: Undocumented");
#ifdef NETWORK
	printf( "  -udp            %s\n","Use TCP/UDP for networking");
#endif
//	printf( "  -vidram         %s\n","FIXME: Undocumented");
	printf( "  -xcontrol       %s\n","FIXME: Undocumented");
	printf( "  -xname          %s\n","FIXME: Undocumented");
	printf( "  -xver           %s\n","FIXME: Undocumented");
	printf( "  -tmap <t>       %s\n","select texmapper to use (c,fp,i386,pent,ppro)");
	printf( "  -automap<X>x<Y> %s\n","Set automap resolution to <X> by <Y>");
	printf( "  -automap_gameres %s\n","Set automap to use the same resolution as in game");
//	printf( "  -menu<X>x<Y>    %s\n","Set menu resolution to <X> by <Y>");
//	printf( "  -menu_gameres   %s\n","Set menus to use the same resolution as in game");
	printf( "\n");

	printf( "D2X System Options:\n\n");
#ifdef __MSDOS__
	printf( "  -joy209         %s\n", "Use alternate port 209 for joystick");
#endif
#ifdef GR_SUPPORTS_FULLSCREEN_TOGGLE
	printf( "  -fullscreen     %s\n", "Use fullscreen mode if available");
#endif
#ifdef OGL
	printf( "  -gl_texmagfilt <f> %s\n","set GL_TEXTURE_MAG_FILTER (see readme.d1x)");
	printf( "  -gl_texminfilt <f> %s\n","set GL_TEXTURE_MIN_FILTER (see readme.d1x)");
	printf( "  -gl_mipmap      %s\n","set gl texture filters to \"standard\" options for mipmapping");
	printf( "  -gl_simple      %s\n","set gl texture filters to gl_nearest for \"original\" look. (default)");
	printf( "  -gl_alttexmerge %s\n","use new texmerge, usually uses less ram (default)");
	printf( "  -gl_stdtexmerge %s\n","use old texmerge, uses more ram, but _might_ be a bit faster");
#ifdef GR_SUPPORTS_FULLSCREEN_TOGGLE
	printf( "  -gl_voodoo      %s\n","force fullscreen mode only");
#endif
	printf( "  -gl_16bittextures %s\n","attempt to use 16bit textures");
	printf( "  -gl_reticle <r> %s\n","use OGL reticle 0=never 1=above 320x* 2=always");
	printf( "  -gl_intensity4_ok %s\n","FIXME: Undocumented");
	printf( "  -gl_luminance4_alpha4_ok %s\n","FIXME: Undocumented");
	printf( "  -gl_readpixels_ok %s\n","FIXME: Undocumented");
	printf( "  -gl_rgba2_ok    %s\n","FIXME: Undocumented");
//	printf( "  -gl_test1       %s\n","FIXME: Undocumented");
	printf( "  -gl_test2       %s\n","FIXME: Undocumented");
	printf( "  -gl_vidmem      %s\n","FIXME: Undocumented");
#ifdef OGL_RUNTIME_LOAD
	printf( "  -gl_library <l> %s\n","use alternate opengl library");
#endif
#endif
#ifdef SDL_VIDEO
	printf( "  -nosdlvidmodecheck %s\n", "Some X servers don't like checking vidmode first, so just switch");
	printf( "  -hwsurface      %s\n","FIXME: Undocumented");
#endif
#ifdef __unix__
	printf( "  -serialdevice <s> %s\n", "Set serial/modem device to <s>");
	printf( "  -serialread <r> %s\n", "Set serial/modem to read from <r>");
#endif
	printf( "\n Help:\n\n");
	printf( "  -help, -h, -?, ? %s\n", "View this help screen");
	printf( "\n");
}

void do_joystick_init()
{

	if (!FindArg( "-nojoystick" ))	{
		con_printf(CON_VERBOSE, "\n%s", TXT_VERBOSE_6);
		joy_init();
		if ( FindArg( "-joyslow" ))	{
			con_printf(CON_VERBOSE, "\n%s", TXT_VERBOSE_7);
			joy_set_slow_reading(JOY_SLOW_READINGS);
		}
		if ( FindArg( "-joypolled" ))	{
			con_printf(CON_VERBOSE, "\n%s", TXT_VERBOSE_8);
			joy_set_slow_reading(JOY_POLLED_READINGS);
		}
		if ( FindArg( "-joybios" ))	{
			con_printf(CON_VERBOSE, "\n%s", TXT_VERBOSE_9);
			joy_set_slow_reading(JOY_BIOS_READINGS);
		}

	//	Added from Descent v1.5 by John.  Adapted by Samir.
	} else {
		con_printf(CON_VERBOSE, "\n%s", TXT_VERBOSE_10);
	}
}

//set this to force game to run in low res
int disable_high_res=0;

void do_register_player(ubyte *title_pal)
{
	Players[Player_num].callsign[0] = '\0';

	if (!Auto_demo) 	{

		key_flush();

		//now, before we bring up the register player menu, we need to
		//do some stuff to make sure the palette is ok.  First, we need to
		//get our current palette into the 2d's array, so the remapping will
		//work.  Second, we need to remap the fonts.  Third, we need to fill
		//in part of the fade tables so the darkening of the menu edges works

		memcpy(gr_palette,title_pal,sizeof(gr_palette));
		remap_fonts_and_menus(1);
		RegisterPlayer();		//get player's name
	}

}

#define PROGNAME argv[0]

extern char Language[];

//can we do highres menus?
extern int MenuHiresAvailable;

int intro_played = 0;

int Inferno_verbose = 0;

//added on 11/18/98 by Victor Rachels to add -mission and -startgame
int start_net_immediately = 0;
//int start_with_mission = 0;
//char *start_with_mission_name;
//end this section addition

int open_movie_file(char *filename,int must_have);

#if defined(POLY_ACC)
#define MENU_HIRES_MODE SM_640x480x15xPA
#else
#define MENU_HIRES_MODE SM(640,480)
#endif

//	DESCENT II by Parallax Software
//		Descent Main

//extern ubyte gr_current_pal[];

#ifdef	EDITOR
int	Auto_exit = 0;
char	Auto_file[128] = "";
#endif

int main(int argc, char *argv[])
{
	int i, t;
	ubyte title_pal[768];
	int screen_width = 640;
	int screen_height = 480;
	u_int32_t screen_mode = SM(screen_width,screen_height);

	con_init();  // Initialise the console
	mem_init();

	error_init(NULL, NULL);

	InitArgs( argc,argv );

#ifdef __unix__
	{
		char *home = getenv("HOME");

		if ((t = FindArg("-userdir")))
			chdir(Args[t+1]);

		else if (home) {
			char buf[PATH_MAX + 5];

			strcpy(buf, home);
			strcat(buf, "/.d2x");
			if (chdir(buf)) {
				mkdir(buf, 0755);
				if (chdir(buf))
					fprintf(stderr, "Cannot change to $HOME/.d2x\n");
			}
		}
	}
#endif

	if (FindArg("-debug"))
		con_threshold.value = (float)2;
	else if (FindArg("-verbose"))
		con_threshold.value = (float)1;

	//tell cfile where hogdir is
	if ((t=FindArg("-hogdir")))
		cfile_use_alternate_hogdir(Args[t+1]);
#ifdef __unix__
	else if (!FindArg("-nohogdir"))
		cfile_use_alternate_hogdir(SHAREPATH);
#endif

	//tell cfile about our counter
	cfile_set_critical_error_counter_ptr(&descent_critical_error);

	if (! cfile_init("descent2.hog"))
		if (! cfile_init("d2demo.hog"))
			Warning("Could not find a valid hog file (descent2.hog or d2demo.hog)\nPossible locations are:\n"
#ifdef __unix__
			      "\t$HOME/.d2x\n"
			      "\t" SHAREPATH "\n"
#else
				  "\tCurrent directory\n"
#endif
				  "Or use the -hogdir option to specify an alternate location.");
	load_text();

	//print out the banner title
	con_printf(CON_NORMAL, "\nDESCENT 2 %s v%d.%d",VERSION_TYPE,Version_major,Version_minor);
	#ifdef VERSION_NAME
	con_printf(CON_NORMAL, "  %s", VERSION_NAME);
	#endif
	if (cfexist(MISSION_DIR "d2x.hog"))
		con_printf(CON_NORMAL, "  Vertigo Enhanced");

	con_printf(CON_NORMAL, "  %s %s\n", __DATE__,__TIME__);
	con_printf(CON_NORMAL, "%s\n%s\n",TXT_COPYRIGHT,TXT_TRADEMARK);
	con_printf(CON_NORMAL, "This is a MODIFIED version of Descent 2. Copyright (c) 1999 Peter Hawkins\n");
	con_printf(CON_NORMAL, "                                         Copyright (c) 2002 Bradley Bell\n");


	if (FindArg( "-?" ) || FindArg( "-help" ) || FindArg( "?" ) || FindArg( "-h" ) ) {
		print_commandline_help();
		set_exit_message("");
#ifdef __MINGW32__
		exit(0);  /* mingw hangs on this return.  dunno why */
#endif
		return(0);
	}

	con_printf(CON_NORMAL, "\n");
	con_printf(CON_NORMAL, TXT_HELP, PROGNAME);		//help message has %s for program name
	con_printf(CON_NORMAL, "\n");

	//(re)added Mar 30, 2003 Micah Lieske - Allow use of 22K sound samples again.
	if(FindArg("-sound22k"))
	{
		digi_sample_rate = SAMPLE_RATE_22K;
	}

	if(FindArg("-sound11k"))
	{
		digi_sample_rate = SAMPLE_RATE_11K;
	}

	arch_init_start();

	arch_init();

	//con_printf(CON_VERBOSE, "\n%s...", "Checking for Descent 2 CD-ROM");

	//added/edited 8/18/98 by Victor Rachels to set maximum fps <= 100
	if ((t = FindArg( "-maxfps" ))) {
		t=atoi(Args[t+1]);
		if (t>0&&t<=80)
			maxfps=t;
	}
	//end addition - Victor Rachels

	if ( FindArg( "-autodemo" ))
		Auto_demo = 1;

#ifndef RELEASE
	if ( FindArg( "-noscreens" ) )
		Skip_briefing_screens = 1;
#endif

	if ((t=FindArg("-tmap"))){
		select_tmap(Args[t+1]);
	}else
		select_tmap(NULL);

	Lighting_on = 1;

//	if (init_graphics()) return 1;

	#ifdef EDITOR
	if (!Inferno_is_800x600_available)	{
		con_printf(CON_NORMAL, "The editor will not be available, press any key to start game...\n" );
		Function_mode = FMODE_MENU;
	}
	#endif

	if (!WVIDEO_running)
		con_printf(CON_DEBUG,"WVIDEO_running = %d\n",WVIDEO_running);

	con_printf (CON_VERBOSE, "%s", TXT_VERBOSE_1);
	ReadConfigFile();

	do_joystick_init();

#if defined(POLY_ACC)
    Current_display_mode = -1;
    game_init_render_buffers(SM_640x480x15xPA, 640, 480, VR_NONE, VRF_COMPATIBLE_MENUS+VRF_ALLOW_COCKPIT );
#else

	if (!VR_offscreen_buffer)	//if hasn't been initialied (by headset init)
		set_display_mode(0);		//..then set default display mode
#endif

	{
//added on 12/14/98 by Matt Mueller - override res in d1x.ini with command line args
		int i, argnum=INT_MAX;
//end addition -MM
		int vr_mode = VR_NONE;
		int screen_compatible = 1;
		int use_double_buffer = 0;

//added/edited on 12/14/98 by Matt Mueller - override res in d1x.ini with command line args
//added on 9/30/98 by Matt Mueller clean up screen mode code, and add higher resolutions
#define SCREENMODE(X,Y,C) if ( (i=FindArg( "-" #X "x" #Y ))&&(i<argnum))  {argnum=i; screen_mode = SM( X , Y ); con_printf(CON_VERBOSE, "Using " #X "x" #Y " ...\n" );screen_width = X;screen_height = Y;use_double_buffer = 1;screen_compatible = C;}
//aren't #defines great? :)

		SCREENMODE(320,100,0);
		SCREENMODE(320,200,1);
//end addition/edit -MM
		SCREENMODE(320,240,0);
		SCREENMODE(320,400,0);
		SCREENMODE(640,400,0);
		SCREENMODE(640,480,0);
		SCREENMODE(800,600,0);
		SCREENMODE(1024,768,0);
		SCREENMODE(1152,864,0);
		SCREENMODE(1280,960,0);
		SCREENMODE(1280,1024,0);
		SCREENMODE(1600,1200,0);
//end addition -MM
		
//added ifdefs on 9/30/98 by Matt Mueller to fix high res in linux
#ifdef __MSDOS__
		if ( FindArg( "-nodoublebuffer" ) )	{
			con_printf(CON_VERBOSE, "Double-buffering disabled...\n" );
#endif
			use_double_buffer = 0;
#ifdef __MSDOS__
		}
#endif
//end addition -MM

		//added 3/24/99 by Owen Evans for screen res changing
//		Game_Screen_mode = screen_mode;
		//end added -OE
		game_init_render_buffers(screen_mode, screen_width, screen_height, vr_mode, screen_compatible);
               
	}
	{
//added/edited on 12/14/98 by Matt Mueller - override res in d1x.ini with command line args
		int i, argnum=INT_MAX;
//added on 9/30/98 by Matt Mueller for selectable automap modes - edited 11/21/99 whee, more fun with defines.
#define SMODE(V,VV,VG,X,Y) if ( (i=FindArg( "-" #V #X "x" #Y )) && (i<argnum))  {argnum=i; VV = SM( X , Y );VG=0;}
#define SMODE_GR(V,VG) if ((i=FindArg("-" #V "_gameres"))){if (i<argnum) VG=1;}
#define SMODE_PRINT(V,VV,VG) if (VG) con_printf(CON_VERBOSE, #V " using game resolution ...\n"); else con_printf(CON_VERBOSE, #V " using %ix%i ...\n",SM_W(VV),SM_H(VV) );
//aren't #defines great? :)
//end addition/edit -MM
#define S_MODE(V,VV,VG) argnum=INT_MAX;SMODE(V,VV,VG,320,200);SMODE(V,VV,VG,320,240);SMODE(V,VV,VG,320,400);SMODE(V,VV,VG,640,400);SMODE(V,VV,VG,640,480);SMODE(V,VV,VG,800,600);SMODE(V,VV,VG,1024,768);SMODE(V,VV,VG,1280,1024);SMODE(V,VV,VG,1600,1200);SMODE_GR(V,VG);SMODE_PRINT(V,VV,VG);

		S_MODE(automap,automap_mode,automap_use_game_res);
//		S_MODE(menu,menu_screen_mode,menu_use_game_res);
	 }
//end addition -MM

	i = FindArg( "-xcontrol" );
	if ( i > 0 )	{
		kconfig_init_external_controls( strtol(Args[i+1], NULL, 0), strtol(Args[i+2], NULL, 0) );
	}

	con_printf(CON_VERBOSE, "\n%s\n\n", TXT_INITIALIZING_GRAPHICS);
	if (FindArg("-nofade"))
		grd_fades_disabled=1;

	//determine whether we're using high-res menus & movies
#if !defined(POLY_ACC)
	if (FindArg("-nohires") || FindArg("-nohighres") || (gr_check_mode(MENU_HIRES_MODE) != 0) || disable_high_res)
		MovieHires = MenuHires = MenuHiresAvailable = 0;
	else
#endif
		//NOTE LINK TO ABOVE!
		MenuHires = MenuHiresAvailable = 1;

	if (FindArg( "-lowresmovies" ))
		MovieHires = 0;

	if ((t=gr_init())!=0)				//doesn't do much
		Error(TXT_CANT_INIT_GFX,t);

   #ifdef _3DFX
   _3dfx_Init();
   #endif

	// Load the palette stuff. Returns non-zero if error.
	con_printf(CON_DEBUG, "Initializing palette system...\n" );
	gr_use_palette_table(DEFAULT_PALETTE );

	con_printf(CON_DEBUG, "Initializing font system...\n" );
	gamefont_init();	// must load after palette data loaded.

	con_printf( CON_DEBUG, "Initializing movie libraries...\n" );
	init_movies();		//init movie libraries

#if 0
	con_printf(CON_VERBOSE, "Going into graphics mode...\n");
#if defined(POLY_ACC)
	gr_set_mode(SM_640x480x15xPA);
#else
	gr_set_mode(MovieHires?SM(640,480):SM(320,200));
#endif
#endif

	#ifndef RELEASE
	if ( FindArg( "-notitles" ) )
		songs_play_song( SONG_TITLE, 1);
	else
	#endif
	{	//NOTE LINK TO ABOVE!
#ifndef SHAREWARE
		int played=MOVIE_NOT_PLAYED;	//default is not played
#endif
		int song_playing = 0;

		#ifdef D2_OEM
		#define MOVIE_REQUIRED 0
		#else
		#define MOVIE_REQUIRED 1
		#endif

#ifdef D2_OEM   //$$POLY_ACC, jay.
		{	//show bundler screens
			char filename[FILENAME_LEN];

			played=MOVIE_NOT_PLAYED;	//default is not played

            played = PlayMovie("pre_i.mve",0);

			if (!played) {
                strcpy(filename,MenuHires?"pre_i1b.pcx":"pre_i1.pcx");

				while (cfexist(filename))
				{
					show_title_screen( filename, 1, 0 );
                    filename[5]++;
				}
			}
		}
#endif

		#ifndef SHAREWARE
			init_subtitles("intro.tex");
			played = PlayMovie("intro.mve",MOVIE_REQUIRED);
			close_subtitles();
		#endif

		#ifdef D2_OEM
		if (played != MOVIE_NOT_PLAYED)
			intro_played = 1;
		else {						//didn't get intro movie, try titles

			played = PlayMovie("titles.mve",MOVIE_REQUIRED);

			if (played == MOVIE_NOT_PLAYED) {
#if defined(POLY_ACC)
				gr_set_mode(SM_640x480x15xPA);
#else
				gr_set_mode(MenuHires?SM_640x480V:SM_320x200C);
#endif
				con_printf( CON_DEBUG, "\nPlaying title song..." );
				songs_play_song( SONG_TITLE, 1);
				song_playing = 1;
				con_printf( CON_DEBUG, "\nShowing logo screens..." );
				show_title_screen( MenuHires?"iplogo1b.pcx":"iplogo1.pcx", 1, 1 );
				show_title_screen( MenuHires?"logob.pcx":"logo.pcx", 1, 1 );
			}
		}

		{	//show bundler movie or screens

			char filename[FILENAME_LEN];
			int movie_handle;

			played=MOVIE_NOT_PLAYED;	//default is not played

			//check if OEM movie exists, so we don't stop the music if it doesn't
			movie_handle = open_movie_file("oem.mve",0);
			if (movie_handle != -1) {
				close(movie_handle);
				played = PlayMovie("oem.mve",0);
				song_playing = 0;		//movie will kill sound
			}

			if (!played) {
				strcpy(filename,MenuHires?"oem1b.pcx":"oem1.pcx");

				while (cfexist(filename))
				{
					show_title_screen( filename, 1, 0 );
					filename[3]++;
				}
			}
		}
		#endif

		if (!song_playing)
			songs_play_song( SONG_TITLE, 1);
			
	}

	PA_DFX (pa_splash());

	con_printf( CON_DEBUG, "\nShowing loading screen..." );
	{
		//grs_bitmap title_bm;
		int pcx_error;
		char filename[14];

		strcpy(filename, MenuHires?"descentb.pcx":"descent.pcx");
		if (! cfexist(filename))
			strcpy(filename, MenuHires?"descntob.pcx":"descento.pcx"); // OEM
		if (! cfexist(filename))
			strcpy(filename, "descentd.pcx"); // SHAREWARE
		if (! cfexist(filename))
			strcpy(filename, "descentb.pcx"); // MAC SHAREWARE

#if defined(POLY_ACC)
		gr_set_mode(SM_640x480x15xPA);
#else
		gr_set_mode(MenuHires?SM(640,480):SM(320,200));
#endif
#ifdef OGL
		set_screen_mode(SCREEN_MENU);
#endif

		FontHires = FontHiresAvailable && MenuHires;

		if ((pcx_error=pcx_read_fullscr( filename, title_pal ))==PCX_ERROR_NONE)	{
			//vfx_set_palette_sub( title_pal );
			gr_palette_clear();
			gr_palette_fade_in( title_pal, 32, 0 );
		} else
			Error( "Couldn't load pcx file '%s', PCX load error: %s\n",filename, pcx_errormsg(pcx_error));
	}

	con_printf( CON_DEBUG , "\nDoing bm_init..." );
	#ifdef EDITOR
		bm_init_use_tbl();
	#else
		bm_init();
	#endif

	#ifdef EDITOR
	if (FindArg("-hoarddata") != 0) {
		#define MAX_BITMAPS_PER_BRUSH 30
		grs_bitmap * bm[MAX_BITMAPS_PER_BRUSH];
		grs_bitmap icon;
		int nframes;
		short nframes_short;
		ubyte palette[256*3];
		FILE *ofile;
		int iff_error,i;
		char *sounds[] = {"selforb.raw","selforb.r22",		//SOUND_YOU_GOT_ORB			
								"teamorb.raw","teamorb.r22",		//SOUND_FRIEND_GOT_ORB			
								"enemyorb.raw","enemyorb.r22",	//SOUND_OPPONENT_GOT_ORB	
								"OPSCORE1.raw","OPSCORE1.r22"};	//SOUND_OPPONENT_HAS_SCORED

		ofile = fopen("hoard.ham","wb");

	   iff_error = iff_read_animbrush("orb.abm",bm,MAX_BITMAPS_PER_BRUSH,&nframes,palette);
		Assert(iff_error == IFF_NO_ERROR);
		nframes_short = nframes;
		fwrite(&nframes_short,sizeof(nframes_short),1,ofile);
		fwrite(&bm[0]->bm_w,sizeof(short),1,ofile);
		fwrite(&bm[0]->bm_h,sizeof(short),1,ofile);
		fwrite(palette,3,256,ofile);
		for (i=0;i<nframes;i++)
			fwrite(bm[i]->bm_data,1,bm[i]->bm_w*bm[i]->bm_h,ofile);

		iff_error = iff_read_animbrush("orbgoal.abm",bm,MAX_BITMAPS_PER_BRUSH,&nframes,palette);
		Assert(iff_error == IFF_NO_ERROR);
		Assert(bm[0]->bm_w == 64 && bm[0]->bm_h == 64);
		nframes_short = nframes;
		fwrite(&nframes_short,sizeof(nframes_short),1,ofile);
		fwrite(palette,3,256,ofile);
		for (i=0;i<nframes;i++)
			fwrite(bm[i]->bm_data,1,bm[i]->bm_w*bm[i]->bm_h,ofile);

		for (i=0;i<2;i++) {
			iff_error = iff_read_bitmap(i?"orbb.bbm":"orb.bbm",&icon,BM_LINEAR,palette);
			Assert(iff_error == IFF_NO_ERROR);
			fwrite(&icon.bm_w,sizeof(short),1,ofile);
			fwrite(&icon.bm_h,sizeof(short),1,ofile);
			fwrite(palette,3,256,ofile);
			fwrite(icon.bm_data,1,icon.bm_w*icon.bm_h,ofile);
		}

		for (i=0;i<sizeof(sounds)/sizeof(*sounds);i++) {
			FILE *ifile;
			int size;
			ubyte *buf;
			ifile = fopen(sounds[i],"rb");
			Assert(ifile != NULL);
			size = ffilelength(ifile);
			buf = d_malloc(size);
			fread(buf,1,size,ifile);
			fwrite(&size,sizeof(size),1,ofile);
			fwrite(buf,1,size,ofile);
			d_free(buf);
			fclose(ifile);
		}

		fclose(ofile);

		exit(1);
	}
	#endif

	//the bitmap loading code changes gr_palette, so restore it
	memcpy(gr_palette,title_pal,sizeof(gr_palette));

	if ( FindArg( "-norun" ) )
		return(0);

	con_printf( CON_DEBUG, "\nInitializing 3d system..." );
	g3_init();

	con_printf( CON_DEBUG, "\nInitializing texture caching system..." );
	texmerge_init( 10 );		// 10 cache bitmaps

	con_printf( CON_DEBUG, "\nRunning game...\n" );
	set_screen_mode(SCREEN_MENU);

	init_game();

	//	If built with editor, option to auto-load a level and quit game
	//	to write certain data.
	#ifdef	EDITOR
	{	int t;
	if ( (t = FindArg( "-autoload" )) ) {
		Auto_exit = 1;
		strcpy(Auto_file, Args[t+1]);
	}
		
	}

	if (Auto_exit) {
		strcpy(Players[0].callsign, "dummy");
	} else
	#endif
		do_register_player(title_pal);

	gr_palette_fade_out( title_pal, 32, 0 );

	Game_mode = GM_GAME_OVER;

	if (Auto_demo)	{
		newdemo_start_playback("descent.dem");		
		if (Newdemo_state == ND_STATE_PLAYBACK )
			Function_mode = FMODE_GAME;
	}

	//do this here because the demo code can do a longjmp when trying to
	//autostart a demo from the main menu, never having gone into the game
	setjmp(LeaveGame);

	while (Function_mode != FMODE_EXIT)
	{
		switch( Function_mode )	{
		case FMODE_MENU:
			set_screen_mode(SCREEN_MENU);
			if ( Auto_demo )	{
				newdemo_start_playback(NULL);		// Randomly pick a file
				if (Newdemo_state != ND_STATE_PLAYBACK)	
					Error("No demo files were found for autodemo mode!");
			} else {
				#ifdef EDITOR
				if (Auto_exit) {
					strcpy((char *)&Level_names[0], Auto_file);
					LoadLevel(1, 1);
					Function_mode = FMODE_EXIT;
					break;
				}
				#endif

				check_joystick_calibration();
				gr_palette_clear();		//I'm not sure why we need this, but we do
				DoMenu();									 	
				#ifdef EDITOR
				if ( Function_mode == FMODE_EDITOR )	{
					create_new_mine();
					SetPlayerFromCurseg();
					load_palette(NULL,1,0);
				}
				#endif
			}
			break;
		case FMODE_GAME:
			#ifdef EDITOR
				keyd_editor_mode = 0;
			#endif

#ifdef SDL_INPUT
			/* keep the mouse from wandering in SDL */
			if (FindArg("-grabmouse"))
			    SDL_WM_GrabInput(SDL_GRAB_ON);
#endif

			game();

#ifdef SDL_INPUT
			/* give control back to the WM */
			if (FindArg("-grabmouse"))
			    SDL_WM_GrabInput(SDL_GRAB_OFF);
#endif

			if ( Function_mode == FMODE_MENU )
				songs_play_song( SONG_TITLE, 1 );
			break;
		#ifdef EDITOR
		case FMODE_EDITOR:
			keyd_editor_mode = 1;
			editor();
#ifdef __WATCOMC__
			_harderr( (void*)descent_critical_error_handler );		// Reinstall game error handler
#endif
			if ( Function_mode == FMODE_GAME ) {
				Game_mode = GM_EDITOR;
				editor_reset_stuff_on_level();
				N_players = 1;
			}
			break;
		#endif
		default:
			Error("Invalid function mode %d",Function_mode);
		}
	}

	WriteConfigFile();

	#ifndef RELEASE
	if (!FindArg( "-notitles" ))
	#endif
		show_order_form();

	#ifndef NDEBUG
	if ( FindArg( "-showmeminfo" ) )
		show_mem_info = 1;		// Make memory statistics show
	#endif

	return(0);		//presumably successful exit
}


void check_joystick_calibration()	{
	int x1, y1, x2, y2, c;
	fix t1;

	if ( (Config_control_type!=CONTROL_JOYSTICK) &&
		  (Config_control_type!=CONTROL_FLIGHTSTICK_PRO) &&
		  (Config_control_type!=CONTROL_THRUSTMASTER_FCS) &&
		  (Config_control_type!=CONTROL_GRAVIS_GAMEPAD)
		) return;

	joy_get_pos( &x1, &y1 );

	t1 = timer_get_fixed_seconds();
	while( timer_get_fixed_seconds() < t1 + F1_0/100 )
		;

	joy_get_pos( &x2, &y2 );

	// If joystick hasn't moved...
	if ( (abs(x2-x1)<30) &&  (abs(y2-y1)<30) )	{
		if ( (abs(x1)>30) || (abs(x2)>30) ||  (abs(y1)>30) || (abs(y2)>30) )	{
			c = nm_messagebox( NULL, 2, TXT_CALIBRATE, TXT_SKIP, TXT_JOYSTICK_NOT_CEN );
			if ( c==0 )	{
				joydefs_calibrate();
			}
		}
	}

}

void show_order_form()
{
#ifndef EDITOR

	int pcx_error;
	char title_pal[768];
	char	exit_screen[16];

	gr_set_current_canvas( NULL );
	gr_palette_clear();

	key_flush();

	strcpy(exit_screen, MenuHires?"ordrd2ob.pcx":"ordrd2o.pcx"); // OEM
	if (! cfexist(exit_screen))
		strcpy(exit_screen, MenuHires?"orderd2b.pcx":"orderd2.pcx"); // SHAREWARE, prefer mac if hires
	if (! cfexist(exit_screen))
		strcpy(exit_screen, MenuHires?"orderd2.pcx":"orderd2b.pcx"); // SHAREWARE, have to rescale
	if (! cfexist(exit_screen))
		strcpy(exit_screen, MenuHires?"warningb.pcx":"warning.pcx"); // D1
	if (! cfexist(exit_screen))
		return; // D2 registered

	if ((pcx_error=pcx_read_fullscr( exit_screen, title_pal ))==PCX_ERROR_NONE) {
		//vfx_set_palette_sub( title_pal );
		gr_palette_fade_in( title_pal, 32, 0 );
		gr_update();
		while (!key_inkey() && !mouse_button_state(0)) {} //key_getch();
		gr_palette_fade_out( title_pal, 32, 0 );
	}
	else
		Int3();		//can't load order screen

	key_flush();

#endif
}

void quit_request()
{
#ifdef NETWORK
//	void network_abort_game();
//	if(Network_status)
//		network_abort_game();
#endif
	exit(0);
}
