/* $Id: newmenu.c,v 1.26 2004-05-21 00:33:03 btb Exp $ */
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
 * Routines for menus.
 *
 * Old Log:
 * Revision 1.26  1996/04/14  21:07:35  allender
 * some change for update
 *
 * Revision 1.25  1995/10/31  10:19:25  allender
 * shareware stuff
 *
 * Revision 1.24  1995/10/27  11:09:04  allender
 * always cd to descent dir before any menu processing --
 * sanity check because of way mac does volume mounting and
 * other file stuff
 *
 * Revision 1.23  1995/10/24  18:11:19  allender
 * do_appl_quit saves and restores background -- don't
 * do any special processing.  fixed newmenu_do3 to pass width
 * and height to ...do_4
 *
 * Revision 1.22  1995/10/21  23:26:25  allender
 * can't do cmd-q in multiplayer games
 *
 * Revision 1.21  1995/10/21  22:53:25  allender
 * menus need to be 8 byte multiple wide -- printscreen stuff
 *
 * Revision 1.20  1995/10/20  00:50:15  allender
 * redbook stuff and make esc leave menu always
 *
 * Revision 1.19  1995/10/17  13:14:52  allender
 * mouse support for other menus now operational -- dragging
 * mouse will also move sliders
 *
 * Revision 1.18  1995/10/12  17:35:19  allender
 * mouse support for pilot and demos
 *
 * Revision 1.17  1995/10/11  12:17:47  allender
 * removed event loop processing
 *
 * Revision 1.16  1995/10/10  11:51:29  allender
 * align menus to 8 byte boundry
 *
 * Revision 1.15  1995/10/05  14:09:36  allender
 * lowered starting x location on menus to fixup network
 * status screen chopping off characters
 *
 * Revision 1.14  1995/09/24  10:52:27  allender
 * made close boxes work
 * maybe some other minor stuff?
 *
 * Revision 1.13  1995/09/15  09:02:04  allender
 * started putting mouse support in listboxes
 *
 * Revision 1.12  1995/09/13  08:50:14  allender
 * close box in this rev?  Worked on getting list box better
 * looking
 *
 * Revision 1.11  1995/09/01  10:52:37  allender
 * fixed up list boxes
 *
 * Revision 1.10  1995/08/24  16:23:08  allender
 * mouse support for most menus
 *
 * Revision 1.9  1995/08/08  13:46:56  allender
 * added macsys header file
 *
 * Revision 1.8  1995/08/01  16:03:54  allender
 * added file list with Desalvo's directory stuff
 *
 * Revision 1.7  1995/07/26  17:03:37  allender
 * show then hide cursor during newmenu operations -- not quite
 * right yet
 *
 * Revision 1.6  1995/07/17  08:56:26  allender
 * fixed up menus to look better with new high res fonts
 *
 * Revision 1.5  1995/07/12  10:35:52  allender
 * removed old debug thing
 *
 * Revision 1.4  1995/06/23  10:23:21  allender
 * menus should work in any resolution now
 *
 * Revision 1.3  1995/06/20  09:11:19  allender
 * changed menu system to support 640x480 instead of 320x200
 *
 * Revision 1.2  1995/06/13  13:07:30  allender
 * added special key to move 320x200 window to upper left corner of screen
 *
 * Revision 1.1  1995/05/16  15:29:13  allender
 * Initial revision
 *
 * Revision 2.8  1995/05/26  16:16:28  john
 * Split SATURN into define's for requiring cd, using cd, etc.
 * Also started adding all the Rockwell stuff.
 *
 * Revision 2.7  1995/04/23  14:54:17  john
 * Fixed bug with background breaking in first menu.
 *
 * Revision 2.6  1995/03/21  14:38:46  john
 * Ifdef'd out the NETWORK code.
 *
 * Revision 2.5  1995/03/15  14:33:29  john
 * Added code to force the Descent CD-rom in the drive.
 *
 * Revision 2.4  1995/03/14  18:24:28  john
 * Force Destination Saturn to use CD-ROM drive.
 *
 * Revision 2.3  1995/03/14  16:22:23  john
 * Added cdrom alternate directory stuff.
 *
 * Revision 2.2  1995/03/06  18:30:51  john
 * Fixed bug with newmenu trashing editor font.
 *
 * Revision 2.1  1995/03/06  15:23:17  john
 * New screen techniques.
 *
 * Revision 2.0  1995/02/27  11:27:55  john
 * New version 2.0, which has no anonymous unions, builds with
 * Watcom 10.0, and doesn't require parsing BITMAPS.TBL.
 *
 * Revision 1.129  1995/02/11  16:19:56  john
 * Added code to make the default mission be the one last played.
 *
 * Revision 1.128  1995/02/02  19:41:33  john
 * Added 10 save game slots.
 *
 * Revision 1.127  1995/02/01  18:13:52  john
 * Fixed some constants.
 *
 * Revision 1.126  1995/02/01  18:04:01  yuan
 * Added 50 characters to list.
 *
 * Revision 1.125  1995/02/01  13:39:35  john
 * Made menu text that changes not overwrite.
 *
 * Revision 1.124  1995/01/31  10:47:57  john
 * Added menu that you can specify the width of the menu.
 *
 * Revision 1.123  1995/01/28  17:18:12  john
 * Added file list box.
 *
 * Revision 1.122  1995/01/27  17:15:55  john
 * Made prev comment actually work.
 *
 * Revision 1.121  1995/01/27  16:49:03  john
 * *** empty log message ***
 *
 * Revision 1.120  1995/01/27  16:46:40  john
 * Made so that input_menu only clears text if -empty-.
 *
 * Revision 1.119  1995/01/27  15:25:04  john
 * Delete saved game when deleteing a pilot file.
 *
 * Revision 1.118  1995/01/25  16:36:09  john
 * Made so that when you hit enter during
 * game save, -empty- goes away.
 *
 * Revision 1.117  1995/01/24  16:59:30  john
 * took out mono debugging text.
 *
 * Revision 1.116  1995/01/24  16:23:16  john
 * Fixed some bugs with listboxes.
 *
 * Revision 1.115  1995/01/23  23:47:36  matt
 * Made keypad enter work in menus
 *
 * Revision 1.114  1995/01/23  19:29:59  john
 * Added scrolling listbox menus.
 *
 * Revision 1.113  1995/01/19  12:33:34  john
 * Made keys jump to sliders, etc in menus.
 *
 * Revision 1.112  1995/01/15  14:33:08  rob
 * Fixed problem with nested nm_messageboxes.
 *
 * Revision 1.111  1995/01/03  17:33:40  john
 * Made scrolling textbox. Used it for scores cool saying.
 *
 * Revision 1.110  1994/12/28  10:42:58  john
 * More VFX tweaking.
 *
 * Revision 1.109  1994/12/28  10:26:39  john
 * Fixed some VFX problems.
 *
 * Revision 1.108  1994/12/15  23:18:10  john
 * Added fix so that VFX mode doesn't hang.
 *
 * Revision 1.107  1994/12/15  12:19:55  john
 * Made menu use clipped bitblt functions.
 *
 * Revision 1.106  1994/12/09  00:41:30  mike
 * fix hang in automap print screen
 *
 * Revision 1.105  1994/12/08  10:01:34  john
 * Changed the way the player callsign stuff works.
 *
 * Revision 1.104  1994/12/04  15:34:30  john
 * Fixed bug with newmenu not restoring font properly.
 *
 * Revision 1.103  1994/12/03  17:47:09  john
 * Fixed bug that didn't free filename mem when not .plr files found.
 *
 * Revision 1.102  1994/12/03  15:06:15  john
 * If no pilot exists, bring up box asking for name.
 *
 * Revision 1.101  1994/12/03  11:04:02  john
 * Changed newmenu code a bit to fix bug with bogus
 * backgrounds occcasionally.
 *
 * Revision 1.100  1994/12/01  20:15:48  yuan
 * Localization.
 *
 * Revision 1.99  1994/12/01  10:33:28  john
 * Fixed bug with large menu backgrounds not drawing correctly.
 *
 * Revision 1.98  1994/12/01  02:41:56  john
 * Fixed warnining.
 *
 * Revision 1.97  1994/11/30  22:52:43  john
 * Fixed bug in code that made the backgrounds behind menus.
 *
 * Revision 1.96  1994/11/30  19:47:42  john
 * Added a check for out o' memory when mallocing background.
 *
 * Revision 1.95  1994/11/30  19:39:10  john
 * ..
 *
 * Revision 1.94  1994/11/30  19:38:27  john
 * fixed bug with previous.
 *
 * Revision 1.93  1994/11/30  19:36:47  john
 * Made Gravis Ultrasound work again.  Made the scores blink
 * at a constant rate.  Revamped the newmenu background storage,
 * which hopefully fixed some bugs.  Made menus in ame not pause
 * sound, except for the pause key.               ^== Game!
 *
 * Revision 1.92  1994/11/30  18:06:05  matt
 * When player types space in callsign, comes up as underscore
 *
 * Revision 1.91  1994/11/30  12:28:22  adam
 * added PCX support
 *
 * Revision 1.90  1994/11/30  12:10:59  adam
 * added support for PCX titles/brief screens
 *
 * Revision 1.89  1994/11/29  00:59:12  allender
 * change newmenu_get_filename so demo files can be deleted too
 *
 * Revision 1.88  1994/11/27  21:16:18  allender
 * made some return values in newmenu_get_filename 0 instead of -1
 *
 * Revision 1.87  1994/11/27  16:58:17  matt
 * Made printscreen work all the time (not just when no ndebug) and made it
 * work when getting a filename.
 *
 * Revision 1.86  1994/11/27  16:47:51  john
 * Made the call to fade in palette only happen if it needs to be, just
 * because I thought it might reduce code paging with vm in menus.
 *
 * Revision 1.85  1994/11/26  15:30:16  matt
 * Allow escape out of change pilot menu
 *
 * Revision 1.84  1994/11/26  14:17:26  matt
 * Player can now only enter valid chars for his name
 *
 * Revision 1.83  1994/11/23  14:13:17  allender
 * if no demo files, displays less "techy" message
 *
 * Revision 1.82  1994/11/21  11:55:52  john
 * Fixed some sound pausing in menus bugs.
 *
 * Revision 1.81  1994/11/19  15:14:58  mike
 * remove unused code and data
 *
 * Revision 1.80  1994/11/18  23:37:54  john
 * Changed some shorts to ints.
 *
 * Revision 1.79  1994/11/15  09:29:21  john
 * Made it so that pressing a letter when selecting players moves to
 * a matching choice.
 *
 * Revision 1.78  1994/11/14  17:12:28  adam
 * *** empty log message ***
 *
 * Revision 1.77  1994/11/14  16:58:31  rob
 * Tried to fix a problem with save demo dialog.
 *
 * Revision 1.76  1994/11/14  16:13:46  matt
 * Fixed handling of players with DOS device names
 *
 * Revision 1.75  1994/11/13  18:12:53  matt
 * Fixed handling of filenames that are the same as DOS devices
 *
 * Revision 1.74  1994/11/13  17:20:44  john
 * Fixed text a bit.
 *
 * Revision 1.73  1994/11/13  17:18:22  john
 * Changed wording of new pilot.
 *
 * Revision 1.72  1994/11/13  17:14:21  john
 * Fixed bug with player list box.
 *
 * Revision 1.71  1994/11/13  17:12:48  john
 * Fixed broken demo file list.
 *
 * Revision 1.70  1994/11/13  17:04:49  john
 * Made the callsign entry be a list box and gave the ability
 * to delete players.
 *
 * Revision 1.69  1994/11/13  15:38:03  john
 * Added critical error handler to game.  Took out -editor command line
 * option because it didn't work anymore and wasn't worth fixing.  Made scores
 * not use MINER enviroment variable on release version, and made scores
 * not print an error if there is no descent.hi.
 *
 * Revision 1.68  1994/11/11  18:17:03  rob
 * Made multi_menu_poll return a value to exit menus.
 *
 * Revision 1.67  1994/11/11  11:07:06  rob
 * Added include of multi.h
 *
 * Revision 1.66  1994/11/10  20:25:16  rob
 * John's stuff to make network menus work.
 *
 * Revision 1.65  1994/11/08  14:51:39  john
 * Added nm_messagebox1, (like the original, only you can pass a function).
 *
 * Revision 1.64  1994/11/08  08:30:39  john
 * Fixed bug with centering titles.
 *
 * Revision 1.63  1994/11/08  08:27:00  john
 * Made titles and subtitles center.
 *
 * Revision 1.62  1994/11/07  09:40:48  john
 * Neatend file list box some.
 *
 * Revision 1.61  1994/11/05  17:22:41  john
 * Fixed lots of sequencing problems with newdemo stuff.
 *
 * Revision 1.60  1994/11/05  15:04:08  john
 * Added non-popup menu for the main menu, so that scores and credits don't have to save
 * the background.
 *
 * Revision 1.59  1994/11/05  14:03:52  john
 * Fixed fade transitions between all screens by making gr_palette_fade_in and out keep
 * track of whether the palette is faded in or not.  Then, wherever the code needs to fade out,
 * it just calls gr_palette_fade_out and it will fade out if it isn't already.  The same with fade_in.
 * This eliminates the need for all the flags like Menu_fade_out, game_fade_in palette, etc.
 *
 * Revision 1.58  1994/11/04  20:11:50  john
 * Neatening up palette stuff with demos.
 *
 * Revision 1.57  1994/11/04  13:49:24  allender
 * fixed newmenu_get_filename to work with less than 10 files
 *
 * Revision 1.56  1994/11/03  19:37:44  john
 * Added scrolling file list box
 *
 * Revision 1.55  1994/10/31  18:16:42  john
 * Made Pad arrows work with menus.
 *
 * Revision 1.54  1994/10/28  14:54:25  john
 * Added forward dec. for newmenu_close.
 * .\
 *
 * Revision 1.53  1994/10/28  14:53:00  john
 * Fixed hideous bug that would bomb if you called newmenu_draw_background
 * before any menus were ever displayed.
 *
 * Revision 1.52  1994/10/24  19:56:53  john
 * Made the new user setup prompt for config options.
 *
 * Revision 1.51  1994/10/24  15:15:49  john
 * Made Esc exit nm_messagebox's,
 * ,
 *
 * Revision 1.50  1994/10/21  15:20:20  john
 * Made PrtScr do screen dump, not F2.
 *
 * Revision 1.49  1994/10/18  12:33:38  john
 * Only used copy the item text into the saved_text field
 * if it is an inputbox or inputbox_menu.
 *
 * Revision 1.48  1994/10/17  11:04:01  john
 * Made backtab work also.
 *
 * Revision 1.47  1994/10/17  10:47:49  john
 * MAde Tab work like down arrow.
 *
 * Revision 1.46  1994/10/17  10:45:10  john
 * Made the player able to abort death by pressing any button or key.
 *
 * Revision 1.45  1994/10/13  21:52:02  john
 * Made it so that if a messagebox has 1 choice, then
 * Esc will return -1.
 *
 * Revision 1.44  1994/10/13  11:35:38  john
 * Made Thrustmaster FCS Hat work.  Put a background behind the
 * keyboard configure.  Took out turn_sensitivity.  Changed sound/config
 * menu to new menu. Made F6 be calibrate joystick.
 *
 * Revision 1.43  1994/10/11  17:18:52  john
 * Fixed bug with sliders always starting at -1.
 *
 * Revision 1.42  1994/10/11  17:08:29  john
 * Added sliders for volume controls.
 *
 * Revision 1.41  1994/10/06  16:04:40  john
 * Made text items color differently than others. Adam
 * is gonna make a diff colored font for these.
 *
 * Revision 1.40  1994/10/06  15:08:23  rob
 * Allowed any negative key value to abort the menu and return.
 *
 * Revision 1.39  1994/10/04  10:26:06  matt
 * Changed fade in to happen every time a global var is set
 *
 * Revision 1.38  1994/10/04  09:16:08  john
 * If you pass -1 as choice in newmenu_do1, then
 * no item is highlighted until you press up or
 * down arrows.
 *
 * Revision 1.37  1994/10/03  23:44:37  matt
 * Save & restore palette effect around menus & pause message
 *
 * Revision 1.36  1994/10/03  22:59:40  matt
 * Re-enabled backspace to generate Int3()
 *
 * Revision 1.35  1994/10/03  19:11:21  matt
 * Changed string input cursor to blinking underscore
 *
 * Revision 1.34  1994/10/03  14:44:15  john
 * Added newmenu_do1, which allows you to pass the starting
 * item to the menu system
 *
 * Revision 1.33  1994/09/30  11:51:21  john
 * Added Matt's NM_TYPE_INPUT_MENU
 *
 * Revision 1.32  1994/09/28  17:22:56  matt
 * Added extra space between subtitle and menu items
 * Made shortcut key check ignore leading spaces in text
 *
 * Revision 1.31  1994/09/15  16:11:22  john
 * Added support for VFX1 head tracking. Fixed bug with memory over-
 * write when using stereo mode.
 *
 * Revision 1.30  1994/09/12  09:52:59  john
 * Made global flush function that flushes keyboard,mouse, and joystick.
 *
 * Revision 1.29  1994/09/10  19:10:54  matt
 * Fixed a few things (like arrow key handling) for menus with all
 * text items, such as the key help message.
 *
 * Revision 1.28  1994/09/01  18:55:38  john
 * freed scores.lbm
 *
 * Revision 1.27  1994/09/01  18:03:50  john
 * Neatened up scores a bit.
 *
 * Revision 1.26  1994/08/30  20:38:13  john
 * Passed citem in newmenu sub.
 *
 * Revision 1.25  1994/08/30  11:13:01  john
 * Added beveled edges to menus.
 *
 * Revision 1.24  1994/08/26  13:01:58  john
 * Put high score system in.
 *
 * Revision 1.23  1994/08/16  00:18:44  john
 * Made pressing the first letter of a menu
 * item move to it.
 *
 * Revision 1.22  1994/08/15  23:17:43  john
 * *** empty log message ***
 *
 * Revision 1.21  1994/08/15  23:15:28  john
 * Made 1 menu/checkbox return with any keypress.
 *
 * Revision 1.20  1994/08/12  10:18:23  john
 * *** empty log message ***
 *
 * Revision 1.19  1994/08/12  10:09:11  john
 * Made borders better.
 *
 * Revision 1.18  1994/08/12  03:11:16  john
 * Made network be default off; Moved network options into
 * main menu.  Made starting net game check that mines are the
 * same.
 *
 * Revision 1.17  1994/08/11  22:14:43  john
 * Free'd up some memory that I forgot to free.
 *
 * Revision 1.16  1994/08/11  19:27:35  john
 * Made the Backspace drop into the debugger only
 * if you're not in an inputbox.
 *
 * Revision 1.15  1994/08/11  18:01:49  matt
 * Added F2 and BACKSPACE keys to new menu system
 *
 * Revision 1.14  1994/08/11  14:25:58  john
 * *** empty log message ***
 *
 * Revision 1.13  1994/08/11  14:25:40  john
 * *** empty log message ***
 *
 * Revision 1.12  1994/08/11  13:47:02  john
 * Made newmenu have subtitles, passed key through to
 * the newmenu subfunctions.
 *
 * Revision 1.11  1994/08/11  12:45:08  john
 * *** empty log message ***
 *
 * Revision 1.10  1994/08/11  12:25:45  john
 * Made right_offset spacing different
 *
 * Revision 1.9  1994/08/11  12:09:49  john
 * Made work with bitmapped fonts.
 *
 * Revision 1.8  1994/08/10  19:56:16  john
 * Changed font stuff; Took out old menu; messed up lots of
 * other stuff like game sequencing messages, etc.
 *
 * Revision 1.7  1994/07/27  16:12:23  john
 * Changed newmenu system to have a callback function.
 * /.
 *
 * Revision 1.6  1994/07/25  15:10:23  john
 * *** empty log message ***
 *
 * Revision 1.5  1994/07/25  12:33:35  john
 * Network "pinging" in.
 *
 * Revision 1.4  1994/07/24  18:21:27  john
 * Took out first time stuff.
 *
 * Revision 1.3  1994/07/24  17:32:47  john
 * Added percent item.  Also neatend up a bit.
 *
 * Revision 1.2  1994/07/22  17:48:13  john
 * Added new menuing system.
 *
 * Revision 1.1  1994/07/22  13:55:38  john
 * Initial revision
 *
 *
 */

#ifdef HAVE_CONFIG_H
#include <conf.h>
#endif

#ifdef WINDOWS
#include "desw.h"
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <ctype.h>
#ifndef _MSC_VER
#include <unistd.h>
#endif
#include <limits.h>

#include "pa_enabl.h"                   //$$POLY_ACC
#include "error.h"
#include "pstypes.h"
#include "gr.h"
#include "mono.h"
#include "songs.h"
#include "key.h"
#include "palette.h"
#include "game.h"
#include "text.h"
#include "findfile.h"

#include "menu.h"
#include "newmenu.h"
#include "gamefont.h"
#include "gamepal.h"
#ifdef NETWORK
#include "network.h"
#endif
#include "iff.h"
#include "pcx.h"
#include "u_mem.h"
#include "mouse.h"
#include "joy.h"
#include "digi.h"

#include "multi.h"
#include "endlevel.h"
#include "screens.h"
#include "config.h"
#include "player.h"
#include "newdemo.h"
#include "kconfig.h"
#include "strutil.h"

#ifdef MACINTOSH
#include <Events.h>
#endif

#if defined (TACTILE)
 #include "tactile.h"
#endif

#if defined(POLY_ACC)
#include "poly_acc.h"
#endif

#define MAXDISPLAYABLEITEMS 15

#define LHX(x)      ((x)*(MenuHires?2:1))
#define LHY(y)      ((y)*(MenuHires?2.4:1))

#define TITLE_FONT      HUGE_FONT
#define NORMAL_FONT     MEDIUM1_FONT    //normal, non-highlighted item
#define SELECTED_FONT   MEDIUM2_FONT    //highlighted item
#define SUBTITLE_FONT   MEDIUM3_FONT

#define NORMAL_CHECK_BOX    "Å"
#define CHECKED_CHECK_BOX   "Ç"

#define NORMAL_RADIO_BOX    ""
#define CHECKED_RADIO_BOX   "Ä"
#define CURSOR_STRING       "_"
#define SLIDER_LEFT         "É"  // 131
#define SLIDER_RIGHT        "Ñ"  // 132
#define SLIDER_MIDDLE       "Ö"  // 133
#define SLIDER_MARKER       "Ü"  // 134
#define UP_ARROW_MARKER     "á"  // 135
#define DOWN_ARROW_MARKER   "à"  // 136

int newmenu_do4( char * title, char * subtitle, int nitems, newmenu_item * item, void (*subfunction)(int nitems,newmenu_item * items, int * last_key, int citem), int citem, char * filename, int width, int height, int TinyMode );
void show_extra_netgame_info(int choice);


int Newmenu_first_time = 1;
//--unused-- int Newmenu_fade_in = 1;

typedef struct bkg {
	WINDOS (dd_grs_canvas *menu_canvas, grs_canvas * menu_canvas);
	grs_bitmap * saved;			// The background under the menu.
	grs_bitmap * background;
} bkg;

grs_bitmap nm_background,nm_background_save;

#define MESSAGEBOX_TEXT_SIZE 2176   // How many characters in messagebox (changed form 300 (fixes crash from show_game_score and friends) - 2000/01/18 Matt Mueller)
#define MAX_TEXT_WIDTH 	200				// How many pixels wide a input box can be

ubyte MenuReordering=0;
ubyte SurfingNet=0;
char Pauseable_menu=0;
char already_showing_info=0;


void newmenu_close()	{
	if ( nm_background.bm_data )
		d_free(nm_background.bm_data);

	if ( nm_background_save.bm_data )
		d_free(nm_background_save.bm_data);
	Newmenu_first_time = 1;
}

ubyte background_palette[768];

//should be called whenever the palette changes
void nm_remap_background()
{
	if (!Newmenu_first_time) {
		if (!nm_background.bm_data)
			nm_background.bm_data = d_malloc(nm_background.bm_w * nm_background.bm_h);

		memcpy(nm_background.bm_data,nm_background_save.bm_data,nm_background.bm_w * nm_background.bm_h);

		gr_remap_bitmap_good( &nm_background, background_palette, -1, -1 );
	}
}

#include <math.h>

extern char last_palette_loaded[];

void nm_draw_background1(char * filename)
{
	int pcx_error;
	grs_bitmap *bmp;
	ubyte pal[256*3];
	int width, height;

	//@@//I think this only gets called to fill the whole screen
	//@@Assert(grd_curcanv->cv_bitmap.bm_w == 320);
	//@@Assert(grd_curcanv->cv_bitmap.bm_h == 200);

	pcx_error = pcx_get_dimensions(filename, &width, &height);
	if (pcx_error != PCX_ERROR_NONE)
		Error("Could not open pcx file <%s>\n", filename);

	bmp = gr_create_bitmap(width, height);

	pcx_error = pcx_read_bitmap(filename,bmp,bmp->bm_type,pal);
	Assert(pcx_error == PCX_ERROR_NONE);

	//@@gr_remap_bitmap_good( bmp, pal, -1, -1 );


	{	//remap stuff. this code is kindof a hack

		//now, before we bring up the menu, we need to
		//do some stuff to make sure the palette is ok.  First, we need to
		//get our current palette into the 2d's array, so the remapping will
		//work.  Second, we need to remap the fonts.  Third, we need to fill
		//in part of the fade tables so the darkening of the menu edges works

		gr_copy_palette(gr_palette, pal, sizeof(gr_palette));
#ifdef OGL
		gr_palette_load(gr_palette);
#endif
		remap_fonts_and_menus(1);

	}

WIN(DDGRLOCK(dd_grd_curcanv));
#if defined(POLY_ACC)
    pa_save_clut();
    pa_update_clut(gr_palette, 0, 256, 0);
#endif
	show_fullscr(bmp);
#if defined(POLY_ACC)
    pa_restore_clut();
#endif
WIN(DDGRUNLOCK(dd_grd_curcanv));

	gr_free_bitmap(bmp);

	strcpy(last_palette_loaded,"");		//force palette load next time

}

#define MENU_BACKGROUND_BITMAP_HIRES (cfexist("scoresb.pcx")?"scoresb.pcx":"scores.pcx")
#define MENU_BACKGROUND_BITMAP_LORES (cfexist("scores.pcx")?"scores.pcx":"scoresb.pcx") // Mac datafiles only have scoresb.pcx

#define MENU_BACKGROUND_BITMAP (MenuHires?MENU_BACKGROUND_BITMAP_HIRES:MENU_BACKGROUND_BITMAP_LORES)

int Background_hires;
int No_darkening=0;

void nm_draw_background(int x1, int y1, int x2, int y2 )
{
	int w,h;

	if (Newmenu_first_time || MenuHires!=Background_hires)	{
		int pcx_error;

		if (Newmenu_first_time) {
			atexit( newmenu_close );
			Newmenu_first_time = 0;
			nm_background_save.bm_data=NULL;		
		}
		else {
			if (nm_background_save.bm_data)
				d_free(nm_background_save.bm_data);
			if (nm_background.bm_data)
				d_free(nm_background.bm_data);
		}

		pcx_error = pcx_read_bitmap(MENU_BACKGROUND_BITMAP,&nm_background_save,BM_LINEAR,background_palette);
		Assert(pcx_error == PCX_ERROR_NONE);

		nm_background = nm_background_save;
		nm_background.bm_data=NULL;		
		nm_remap_background();

		Background_hires = MenuHires;
	}

	if ( x1 < 0 ) x1 = 0;
	if ( y1 < 0 ) y1 = 0;

	w = x2-x1+1;
	h = y2-y1+1;

	//if ( w > nm_background.bm_w ) w = nm_background.bm_w;
	//if ( h > nm_background.bm_h ) h = nm_background.bm_h;
	
	x2 = x1 + w - 1;
	y2 = y1 + h - 1;

#if 0
	{
		grs_bitmap *tmp = gr_create_bitmap(w, h);

		gr_bitmap_scale_to(&nm_background, tmp);

		WIN(DDGRLOCK(dd_grd_curcanv));
		if (No_darkening)
			gr_bm_bitblt(w, h, x1, y1, LHX(10), LHY(10), tmp, &(grd_curcanv->cv_bitmap) );
		else
			gr_bm_bitblt(w, h, x1, y1, 0, 0, tmp, &(grd_curcanv->cv_bitmap) );
		gr_free_bitmap(tmp);
	}
#else
	WIN(DDGRLOCK(dd_grd_curcanv));
	if (No_darkening)
		gr_bm_bitblt(w, h, x1, y1, LHX(10), LHY(10), &nm_background, &(grd_curcanv->cv_bitmap) );
	else
		gr_bm_bitblt(w, h, x1, y1, 0, 0, &nm_background, &(grd_curcanv->cv_bitmap) );
#endif

	if (!No_darkening) {
		Gr_scanline_darkening_level = 2*7;

		gr_setcolor( BM_XRGB(0,0,0) );
		gr_urect( x2-5, y1+5, x2-5, y2-5 );
		gr_urect( x2-4, y1+4, x2-4, y2-5 );
		gr_urect( x2-3, y1+3, x2-3, y2-5 );
		gr_urect( x2-2, y1+2, x2-2, y2-5 );
		gr_urect( x2-1, y1+1, x2-1, y2-5 );
		gr_urect( x2+0, y1+0, x2-0, y2-5 );

		gr_urect( x1+5, y2-5, x2, y2-5 );
		gr_urect( x1+4, y2-4, x2, y2-4 );
		gr_urect( x1+3, y2-3, x2, y2-3 );
		gr_urect( x1+2, y2-2, x2, y2-2 );
		gr_urect( x1+1, y2-1, x2, y2-1 );
		gr_urect( x1+0, y2, x2, y2-0 );
	}
	WIN(DDGRUNLOCK(dd_grd_curcanv));

	Gr_scanline_darkening_level = GR_FADE_LEVELS;
}

void nm_restore_background( int x, int y, int w, int h )
{
	int x1, x2, y1, y2;

	x1 = x; x2 = x+w-1;
	y1 = y; y2 = y+h-1;

	if ( x1 < 0 ) x1 = 0;
	if ( y1 < 0 ) y1 = 0;

	if ( x2 >= nm_background.bm_w ) x2=nm_background.bm_w-1;
	if ( y2 >= nm_background.bm_h ) y2=nm_background.bm_h-1;

	w = x2 - x1 + 1;
	h = y2 - y1 + 1;

	WIN(DDGRLOCK(dd_grd_curcanv));
		gr_bm_bitblt(w, h, x1, y1, x1, y1, &nm_background, &(grd_curcanv->cv_bitmap) );
	WIN(DDGRUNLOCK(dd_grd_curcanv));
}

// Draw a left justfied string
void nm_string( bkg * b, int w1,int x, int y, char * s)
{
	int w,h,aw,tx=0,t=0,i;
	char *p,*s1,*s2,measure[2];
	int XTabs[]={15,87,124,162,228,253};
   
	p=s1=NULL;
	s2 = d_strdup(s);

	for (i=0;i<6;i++) {
		XTabs[i]=(LHX(XTabs[i]));
		XTabs[i]+=x;
	}
 
	measure[1]=0;

	if (!SurfingNet) {
		p = strchr( s2, '\t' );
		if (p && (w1>0) ) {
			*p = '\0';
			s1 = p+1;
		}
	}

	gr_get_string_size(s2, &w, &h, &aw  );

	if (w1 > 0)
		w = w1;

	// CHANGED
	gr_bm_bitblt(b->background->bm_w-15, h+2, 5, y-1, 5, y-1, b->background, &(grd_curcanv->cv_bitmap) );
	//gr_bm_bitblt(w, h, x, y, x, y, b->background, &(grd_curcanv->cv_bitmap) );

	if (SurfingNet) {
		for (i=0;i<strlen(s2);i++) {
			if (s2[i]=='\t' && SurfingNet) {
				x=XTabs[t];
				t++;
				continue;
			}
			measure[0]=s2[i];
			gr_get_string_size(measure,&tx,&h,&aw);
			gr_string(x,y,measure);
			x+=tx;
		}
	}
	else
		gr_string (x,y,s2);
         
	if (!SurfingNet && p && (w1>0) ) {
		gr_get_string_size(s1, &w, &h, &aw  );

		gr_string( x+w1-w, y, s1 );

		*p = '\t';
	}
	d_free(s2);
}

// Draw a slider and it's string
void nm_string_slider( bkg * b, int w1,int x, int y, char * s )
{
	int w,h,aw;
	char *p,*s1;

	s1=NULL;

	p = strchr( s, '\t' );
	if (p)	{
		*p = '\0';
		s1 = p+1;
	}

	gr_get_string_size(s, &w, &h, &aw  );
	// CHANGED

		gr_bm_bitblt(b->background->bm_w-15, h, 5, y, 5, y, b->background, &(grd_curcanv->cv_bitmap) );
		//gr_bm_bitblt(w, h, x, y, x, y, b->background, &(grd_curcanv->cv_bitmap) );

		gr_string( x, y, s );

		if (p)	{
			gr_get_string_size(s1, &w, &h, &aw  );

			// CHANGED
			gr_bm_bitblt(w, 1, x+w1-w, y, x+w1-w, y, b->background, &(grd_curcanv->cv_bitmap) );
			// CHANGED
			gr_bm_bitblt(w, 1, x+w1-w, y+h-1, x+w1-w, y, b->background, &(grd_curcanv->cv_bitmap) );

			gr_string( x+w1-w, y, s1 );

			*p = '\t';
		}
}


// Draw a left justfied string with black background.
void nm_string_black( bkg * b, int w1,int x, int y, char * s )
{
	int w,h,aw;
	gr_get_string_size(s, &w, &h, &aw  );
	b = b;					
	if (w1 == 0) w1 = w;

	WIN(DDGRLOCK(dd_grd_curcanv));
		gr_setcolor( BM_XRGB(2,2,2) );
  		gr_rect( x-1, y-1, x-1, y+h-1 );
		gr_rect( x-1, y-1, x+w1-1, y-1 );

	 
		gr_setcolor( BM_XRGB(5,5,5) );
		gr_rect( x, y+h, x+w1, y+h);
		gr_rect( x+w1, y-1, x+w1, y+h );
     
		gr_setcolor( BM_XRGB(0,0,0) );
		gr_rect( x, y, x+w1-1, y+h-1 );
	
		gr_string( x+1, y+1, s );
	WIN(DDGRUNLOCK(dd_grd_curcanv));
}


// Draw a right justfied string
void nm_rstring( bkg * b,int w1,int x, int y, char * s )
{
	int w,h,aw;
	gr_get_string_size(s, &w, &h, &aw  );
	x -= 3;

	if (w1 == 0) w1 = w;

	//mprintf( 0, "Width = %d, string='%s'\n", w, s );

	// CHANGED
	WIN(DDGRLOCK(dd_grd_curcanv));
		gr_bm_bitblt(w1, h, x-w1, y, x-w1, y, b->background, &(grd_curcanv->cv_bitmap) );
		gr_string( x-w, y, s );
	WIN(DDGRUNLOCK(dd_grd_curcanv));
}

#include "timer.h"

//for text items, constantly redraw cursor (to achieve flash)
void update_cursor( newmenu_item *item)
{
	int w,h,aw;
	fix time = timer_get_approx_seconds();
	int x,y;
	char * text = item->text;

	Assert(item->type==NM_TYPE_INPUT_MENU || item->type==NM_TYPE_INPUT);

	while( *text )	{
		gr_get_string_size(text, &w, &h, &aw  );
		if ( w > item->w-10 )
			text++;
		else
			break;
	}
	if (*text==0) 
		w = 0;
	x = item->x+w; y = item->y;

WIN(DDGRLOCK(dd_grd_curcanv));
	if (time & 0x8000)
		gr_string( x, y, CURSOR_STRING );
	else {
		gr_setcolor( BM_XRGB(0,0,0) );
		gr_rect( x, y, x+grd_curcanv->cv_font->ft_w-1, y+grd_curcanv->cv_font->ft_h-1 );
	}
WIN(DDGRUNLOCK(dd_grd_curcanv));
}

void nm_string_inputbox( bkg *b, int w, int x, int y, char * text, int current )
{
	int w1,h1,aw;

	while( *text )	{
		gr_get_string_size(text, &w1, &h1, &aw  );
		if ( w1 > w-10 )
			text++;
		else
			break;
	}
	if ( *text == 0 )
		w1 = 0;

   nm_string_black( b, w, x, y, text );
		
	if ( current )	{
		gr_string( x+w1+1, y, CURSOR_STRING );
	}
}

void draw_item( bkg * b, newmenu_item *item, int is_current,int tiny )
{
       if (tiny)
        {
         if (is_current)
          gr_set_fontcolor(gr_find_closest_color_current(57,49,20),-1);
         else
          gr_set_fontcolor(gr_find_closest_color_current(29,29,47),-1);

         if (item->text[0]=='\t')
          gr_set_fontcolor (gr_find_closest_color_current(63,63,63),-1);
        }
       else
        {
         if (is_current)
          grd_curcanv->cv_font = SELECTED_FONT;
         else
          grd_curcanv->cv_font = NORMAL_FONT;

		#ifdef WINDOWS
			if (is_current && item->type == NM_TYPE_TEXT) 
				grd_curcanv->cv_font = NORMAL_FONT;
		#endif
        }

WIN(DDGRLOCK(dd_grd_curcanv));	
	switch( item->type )	{
	case NM_TYPE_TEXT:
      // grd_curcanv->cv_font=TEXT_FONT;
		// fall through on purpose

	case NM_TYPE_MENU:
		nm_string( b, item->w, item->x, item->y, item->text );
		break;
	case NM_TYPE_SLIDER:	{
		int j;
		if (item->value < item->min_value) item->value=item->min_value;
		if (item->value > item->max_value) item->value=item->max_value;
		sprintf( item->saved_text, "%s\t%s", item->text, SLIDER_LEFT );
		for (j=0; j<(item->max_value-item->min_value+1); j++ )	{
			sprintf( item->saved_text, "%s%s", item->saved_text,SLIDER_MIDDLE );
		}
		sprintf( item->saved_text, "%s%s", item->saved_text,SLIDER_RIGHT );
		
		item->saved_text[item->value+1+strlen(item->text)+1] = SLIDER_MARKER[0];
		
		nm_string_slider( b, item->w, item->x, item->y, item->saved_text );
		}
		break;
	case NM_TYPE_INPUT_MENU:
		if ( item->group==0 )		{
			nm_string( b, item->w, item->x, item->y, item->text );
		} else {
			nm_string_inputbox( b, item->w, item->x, item->y, item->text, is_current );
		}
		break;
	case NM_TYPE_INPUT:
		nm_string_inputbox( b, item->w, item->x, item->y, item->text, is_current );
		break;
	case NM_TYPE_CHECK:
		nm_string( b, item->w, item->x, item->y, item->text );
		if (item->value)
			nm_rstring( b,item->right_offset,item->x, item->y, CHECKED_CHECK_BOX );
		else														  
			nm_rstring( b,item->right_offset,item->x, item->y, NORMAL_CHECK_BOX );
		break;
	case NM_TYPE_RADIO:
		nm_string( b, item->w, item->x, item->y, item->text );
		if (item->value)
			nm_rstring( b,item->right_offset, item->x, item->y, CHECKED_RADIO_BOX );
		else
			nm_rstring( b,item->right_offset, item->x, item->y, NORMAL_RADIO_BOX );
		break;
	case NM_TYPE_NUMBER:	{
		char text[10];
		if (item->value < item->min_value) item->value=item->min_value;
		if (item->value > item->max_value) item->value=item->max_value;
		nm_string( b, item->w, item->x, item->y, item->text );
		sprintf( text, "%d", item->value );
		nm_rstring( b,item->right_offset,item->x, item->y, text );
		}
		break;
	}
WIN(DDGRUNLOCK(dd_grd_curcanv));

}

char *Newmenu_allowed_chars=NULL;

//returns true if char is allowed
int char_allowed(char c)
{
	char *p = Newmenu_allowed_chars;

	if (!p)
		return 1;

	while (*p) {
		Assert(p[1]);

		if (c>=p[0] && c<=p[1])
			return 1;

		p += 2;
	}

	return 0;
}

void strip_end_whitespace( char * text )
{
	int i,l;
	l = strlen( text );
	for (i=l-1; i>=0; i-- )	{
		if ( isspace(text[i]) )
			text[i] = 0;
		else
			return;
	}
}

int newmenu_do( char * title, char * subtitle, int nitems, newmenu_item * item, void (*subfunction)(int nitems,newmenu_item * items, int * last_key, int citem) )
{
	return newmenu_do3( title, subtitle, nitems, item, subfunction, 0, NULL, -1, -1 );
}
int newmenu_dotiny( char * title, char * subtitle, int nitems, newmenu_item * item, void (*subfunction)(int nitems,newmenu_item * items, int * last_key, int citem) )
{
        return newmenu_do4( title, subtitle, nitems, item, subfunction, 0, NULL, LHX(310), -1, 1 );
}


int newmenu_dotiny2( char * title, char * subtitle, int nitems, newmenu_item * item, void (*subfunction)(int nitems,newmenu_item * items, int * last_key, int citem) )
{
        return newmenu_do4( title, subtitle, nitems, item, subfunction, 0, NULL, -1, -1, 1 );
}


int newmenu_do1( char * title, char * subtitle, int nitems, newmenu_item * item, void (*subfunction)(int nitems,newmenu_item * items, int * last_key, int citem), int citem )
{
	return newmenu_do3( title, subtitle, nitems, item, subfunction, citem, NULL, -1, -1 );
}


int newmenu_do2( char * title, char * subtitle, int nitems, newmenu_item * item, void (*subfunction)(int nitems,newmenu_item * items, int * last_key, int citem), int citem, char * filename )
{
	return newmenu_do3( title, subtitle, nitems, item, subfunction, citem, filename, -1, -1 );
}
int newmenu_do3( char * title, char * subtitle, int nitems, newmenu_item * item, void (*subfunction)(int nitems,newmenu_item * items, int * last_key, int citem), int citem, char * filename, int width, int height )
 {
  return newmenu_do4( title, subtitle, nitems, item, subfunction, citem, filename, width, height,0 );
 }

int newmenu_do_fixedfont( char * title, char * subtitle, int nitems, newmenu_item * item, void (*subfunction)(int nitems,newmenu_item * items, int * last_key, int citem), int citem, char * filename, int width, int height){
	set_screen_mode(SCREEN_MENU);//hafta set the screen mode before calling or fonts might get changed/freed up if screen res changes
//	return newmenu_do3_real( title, subtitle, nitems, item, subfunction, citem, filename, width,height, GAME_FONT, GAME_FONT, GAME_FONT, GAME_FONT);
	return newmenu_do4( title, subtitle, nitems, item, subfunction, citem, filename, width,height, 0);
}

//returns 1 if a control device button has been pressed
int check_button_press()
{
	int i;

	switch (Config_control_type) {
	case	CONTROL_JOYSTICK:
	case	CONTROL_FLIGHTSTICK_PRO:
	case	CONTROL_THRUSTMASTER_FCS:
	case	CONTROL_GRAVIS_GAMEPAD:
		for (i=0; i<4; i++ )	
	 		if (joy_get_button_down_cnt(i)>0) return 1;
		break;
	case	CONTROL_MOUSE:
	case	CONTROL_CYBERMAN:
#ifndef NEWMENU_MOUSE   // don't allow mouse to continue from menu
		for (i=0; i<3; i++ )	
			if (mouse_button_down_count(i)>0) return 1;
		break;
#endif
	case	CONTROL_WINJOYSTICK:
	#ifdef WINDOWS	
		for (i=0; i<4; i++ )	
	 		if (joy_get_button_down_cnt(i)>0) return 1;
	#endif	
		break;
	case	CONTROL_NONE:		//keyboard only
		#ifdef APPLE_DEMO
			if (key_checkch())	return 1;			
		#endif

		break;
	default:
		Error("Bad control type (Config_control_type):%i",Config_control_type);
	}

	return 0;
}

extern int network_request_player_names(int);
extern int RestoringMenu;

#ifdef NEWMENU_MOUSE
ubyte Hack_DblClick_MenuMode=0;
#endif

#ifdef MACINTOSH
extern ubyte joydefs_calibrating;
#else
# define joydefs_calibrating 0
#endif

#define CLOSE_X     (MenuHires?15:7)
#define CLOSE_Y     (MenuHires?15:7)
#define CLOSE_SIZE  (MenuHires?10:5)

void draw_close_box(int x,int y)
{
	WIN (DDGRLOCK(dd_grd_curcanv));
	gr_setcolor( BM_XRGB(0, 0, 0) );
	gr_rect(x + CLOSE_X, y + CLOSE_Y, x + CLOSE_X + CLOSE_SIZE, y + CLOSE_Y + CLOSE_SIZE);
	gr_setcolor( BM_XRGB(21, 21, 21) );
	gr_rect(x + CLOSE_X + LHX(1), y + CLOSE_Y + LHX(1), x + CLOSE_X + CLOSE_SIZE - LHX(1), y + CLOSE_Y + CLOSE_SIZE - LHX(1));
	WIN (DDGRUNLOCK(dd_grd_curcanv));
}

int newmenu_do4( char * title, char * subtitle, int nitems, newmenu_item * item, void (*subfunction)(int nitems,newmenu_item * items, int * last_key, int citem), int citem, char * filename, int width, int height, int TinyMode )
{
	int old_keyd_repeat, done;
	int  choice,old_choice,i,j,x,y,w,h,aw, tw, th, twidth,fm,right_offset;
	int k, nmenus, nothers,ScrollOffset=0,LastScrollCheck=-1,MaxDisplayable,sx,sy;
	grs_font * save_font;
	int string_width, string_height, average_width;
	int ty;
	bkg bg;
	int all_text=0;		//set true if all text items
	int sound_stopped=0,time_stopped=0;
   int TopChoice,IsScrollBox=0;   // Is this a scrolling box? Set to false at init
   char *Temp,TempVal;
	int dont_restore=0;
   int MaxOnMenu=MAXDISPLAYABLEITEMS;
	WINDOS(dd_grs_canvas *save_canvas, grs_canvas *save_canvas );	
#ifdef NEWMENU_MOUSE
	int mouse_state, omouse_state, dblclick_flag=0;
	int mx=0, my=0, x1 = 0, x2, y1, y2;
	int close_box=0;
#endif
#ifdef MACINTOSH
	EventRecord event;		// looking for disk inserted events for CD mounts
#endif

	PA_DFX (pa_set_frontbuffer_current());
	PA_DFX (pa_set_front_to_read());

	WIN(if (!_AppActive) return -1);		// Don't draw message if minimized!
	newmenu_hide_cursor();

	if (nitems < 1 )
    {
		return -1;
    } 

	WIN(mouse_set_mode(0));		//disable centering mode

        MaxDisplayable=nitems;

	//set_screen_mode(SCREEN_MENU);
	set_popup_screen();

	if ( Function_mode == FMODE_GAME && !(Game_mode & GM_MULTI)) {
		digi_pause_digi_sounds();
		sound_stopped = 1;
	}

	if (!((Game_mode & GM_MULTI) && (Function_mode == FMODE_GAME) && (!Endlevel_sequence)) )
	{
		time_stopped = 1;
		stop_time();
		#ifdef TACTILE 
		  if (TactileStick)	
			  DisableForces();	
		#endif
	}

#ifdef WINDOWS
RePaintNewmenu4:
#endif
	WINDOS( save_canvas = dd_grd_curcanv, save_canvas = grd_curcanv );

	WINDOS( dd_gr_set_current_canvas(NULL), gr_set_current_canvas(NULL) );

	save_font = grd_curcanv->cv_font;

	tw = th = 0;

	if ( title )	{
		grd_curcanv->cv_font = TITLE_FONT;
		gr_get_string_size(title,&string_width,&string_height,&average_width );
		tw = string_width;
		th = string_height;
	}
	if ( subtitle )	{
		grd_curcanv->cv_font = SUBTITLE_FONT;
		gr_get_string_size(subtitle,&string_width,&string_height,&average_width );
		if (string_width > tw )
			tw = string_width;
		th += string_height;
	}

	th += LHY(8);		//put some space between titles & body

  	if (TinyMode)
    	grd_curcanv->cv_font = SMALL_FONT;
  	else 
    	grd_curcanv->cv_font = NORMAL_FONT;

	w = aw = 0;
	h = th;
	nmenus = nothers = 0;

	// Find menu height & width (store in w,h)
	for (i=0; i<nitems; i++ )	{
		item[i].redraw=1;
		item[i].y = h;
		gr_get_string_size(item[i].text,&string_width,&string_height,&average_width );
		item[i].right_offset = 0;
		
		if (SurfingNet)
			string_height+=LHY(3);

		item[i].saved_text[0] = '\0';

		if ( item[i].type == NM_TYPE_SLIDER )	{
			int w1,h1,aw1;
			nothers++;
			sprintf( item[i].saved_text, "%s", SLIDER_LEFT );
			for (j=0; j<(item[i].max_value-item[i].min_value+1); j++ )	{
				sprintf( item[i].saved_text, "%s%s", item[i].saved_text,SLIDER_MIDDLE );
			}
			sprintf( item[i].saved_text, "%s%s", item[i].saved_text,SLIDER_RIGHT );
			gr_get_string_size(item[i].saved_text,&w1,&h1,&aw1 );
			string_width += w1 + aw;
		}

		if ( item[i].type == NM_TYPE_MENU )	{
			nmenus++;
		}

		if ( item[i].type == NM_TYPE_CHECK )	{
			int w1,h1,aw1;
			nothers++;
			gr_get_string_size(NORMAL_CHECK_BOX, &w1, &h1, &aw1  );
			item[i].right_offset = w1;
			gr_get_string_size(CHECKED_CHECK_BOX, &w1, &h1, &aw1  );
			if (w1 > item[i].right_offset)
				item[i].right_offset = w1;
		}
		
		if (item[i].type == NM_TYPE_RADIO ) {
			int w1,h1,aw1;
			nothers++;
			gr_get_string_size(NORMAL_RADIO_BOX, &w1, &h1, &aw1  );
			item[i].right_offset = w1;
			gr_get_string_size(CHECKED_RADIO_BOX, &w1, &h1, &aw1  );
			if (w1 > item[i].right_offset)
				item[i].right_offset = w1;
		}

		if  (item[i].type==NM_TYPE_NUMBER )	{
			int w1,h1,aw1;
			char test_text[20];
			nothers++;
			sprintf( test_text, "%d", item[i].max_value );
			gr_get_string_size( test_text, &w1, &h1, &aw1 );
			item[i].right_offset = w1;
			sprintf( test_text, "%d", item[i].min_value );
			gr_get_string_size( test_text, &w1, &h1, &aw1 );
			if ( w1 > item[i].right_offset)
				item[i].right_offset = w1;
		}

		if ( item[i].type == NM_TYPE_INPUT )	{
			Assert( strlen(item[i].text) < NM_MAX_TEXT_LEN );
			strcpy(item[i].saved_text, item[i].text );
			nothers++;
			string_width = item[i].text_len*grd_curcanv->cv_font->ft_w+((MenuHires?3:1)*item[i].text_len);
			if ( string_width > MAX_TEXT_WIDTH ) 
				string_width = MAX_TEXT_WIDTH;
			item[i].value = -1;
		}

		if ( item[i].type == NM_TYPE_INPUT_MENU )	{
			Assert( strlen(item[i].text) < NM_MAX_TEXT_LEN );
			strcpy(item[i].saved_text, item[i].text );
			nmenus++;
			string_width = item[i].text_len*grd_curcanv->cv_font->ft_w+((MenuHires?3:1)*item[i].text_len);
			item[i].value = -1;
			item[i].group = 0;
		}

		item[i].w = string_width;
		item[i].h = string_height;

		if ( string_width > w )
			w = string_width;		// Save maximum width
		if ( average_width > aw )
			aw = average_width;
		h += string_height+1;		// Find the height of all strings
	}

   // Big hack for allowing the netgame options menu to spill over

   MaxOnMenu=MAXDISPLAYABLEITEMS;
   if (ExtGameStatus==GAMESTAT_NETGAME_OPTIONS || ExtGameStatus==GAMESTAT_MORE_NETGAME_OPTIONS)
		MaxOnMenu++;

   if (!TinyMode && (h>((MaxOnMenu+1)*(string_height+1))+(LHY(8))))
    {
     IsScrollBox=1;
     h=(MaxOnMenu*(string_height+1)+LHY(8));
     MaxDisplayable=MaxOnMenu;
     mprintf ((0,"Hey, this is a scroll box!\n"));
    }
   else
    IsScrollBox=0;

	right_offset=0;

	if ( width > -1 )
		w = width;

	if ( height > -1 )
		h = height;

	for (i=0; i<nitems; i++ )	{
		item[i].w = w;
		if (item[i].right_offset > right_offset )
			right_offset = item[i].right_offset;
	}
	if (right_offset > 0 )
		right_offset += 3;

	//gr_get_string_size("",&string_width,&string_height,&average_width );

	w += right_offset;


	twidth = 0;
	if ( tw > w )	{
		twidth = ( tw - w )/2;
		w = tw;
	}

   if (RestoringMenu)
	 { right_offset=0; twidth=0;}

	mprintf(( 0, "Right offset = %d\n", right_offset ));


	// Find min point of menu border
//	x = (grd_curscreen->sc_w-w)/2;
//	y = (grd_curscreen->sc_h-h)/2;

	w += MenuHires?60:30;
	h += MenuHires?60:30;

	if ( w > grd_curcanv->cv_bitmap.bm_w ) w = grd_curcanv->cv_bitmap.bm_w;
	if ( h > grd_curcanv->cv_bitmap.bm_h ) h = grd_curcanv->cv_bitmap.bm_h;

	x = (grd_curcanv->cv_bitmap.bm_w-w)/2;
	y = (grd_curcanv->cv_bitmap.bm_h-h)/2;

	if ( x < 0 ) x = 0;
	if ( y < 0 ) y = 0;

	if ( filename != NULL )	{
		nm_draw_background1( filename );
		gr_palette_load(gr_palette);
	}

// Save the background of the display
//		Win95 must refer to the screen as a dd_grs_canvas, so...
	WINDOS (	bg.menu_canvas = dd_gr_create_sub_canvas( dd_grd_screencanv, x, y, w, h ),
			bg.menu_canvas = gr_create_sub_canvas( &grd_curscreen->sc_canvas, x, y, w, h )
	);
	WINDOS (	dd_gr_set_current_canvas( bg.menu_canvas ), 
			gr_set_current_canvas(bg.menu_canvas)	);

	if ( filename == NULL )	{
		// Save the background under the menu...
		#ifdef TACTILE
			if (TactileStick)
				DisableForces();
		#endif
		
#if defined(POLY_ACC)
		bg.saved = gr_create_bitmap2( w, h, grd_curcanv->cv_bitmap.bm_type, NULL );
#else
		bg.saved = gr_create_bitmap( w, h );
#endif
		Assert( bg.saved != NULL );

		WIN (DDGRLOCK(dd_grd_curcanv));
			gr_bm_bitblt(w, h, 0, 0, 0, 0, &grd_curcanv->cv_bitmap, bg.saved );
		WIN (DDGRUNLOCK(dd_grd_curcanv));

		WINDOS (	dd_gr_set_current_canvas(NULL), 
					gr_set_current_canvas( NULL ) 
		);

		nm_draw_background(x,y,x+w-1,y+h-1);

		WINDOS (	dd_gr_set_current_canvas(bg.menu_canvas),
					gr_set_current_canvas( bg.menu_canvas )
		);

		bg.background = gr_create_sub_bitmap(&nm_background,0,0,w,h);

	} else {
		bg.saved = NULL;
#if defined(POLY_ACC)
		bg.background = gr_create_bitmap2( w, h, grd_curcanv->cv_bitmap.bm_type, NULL );
#else
		bg.background = gr_create_bitmap( w, h );
#endif
		Assert( bg.background != NULL );
		
		WIN (DDGRLOCK(dd_grd_curcanv));
			gr_bm_bitblt(w, h, 0, 0, 0, 0, &grd_curcanv->cv_bitmap, bg.background );
		WIN (DDGRUNLOCK(dd_grd_curcanv));
	}

// ty = 15 + (yborder/4);

	ty = MenuHires?30:15;

	if ( title )	{
		grd_curcanv->cv_font = TITLE_FONT;
		gr_set_fontcolor( GR_GETCOLOR(31,31,31), -1 );
		gr_get_string_size(title,&string_width,&string_height,&average_width );
		tw = string_width;
		th = string_height;
		WIN (DDGRLOCK(dd_grd_curcanv));
			gr_printf( 0x8000, ty, title );
		WIN (DDGRUNLOCK(dd_grd_curcanv));
		ty += th;
	}

	if ( subtitle )	{
		grd_curcanv->cv_font = SUBTITLE_FONT;
		gr_set_fontcolor( GR_GETCOLOR(21,21,21), -1 );
		gr_get_string_size(subtitle,&string_width,&string_height,&average_width );
		tw = string_width;
		th = string_height;
		WIN (DDGRLOCK(dd_grd_curcanv));
			gr_printf( 0x8000, ty, subtitle );
		WIN (DDGRUNLOCK(dd_grd_curcanv));
		ty += th;
	}

  	if (TinyMode)
    	grd_curcanv->cv_font = SMALL_FONT;
 	else 
    	grd_curcanv->cv_font = NORMAL_FONT;
	
	// Update all item's x & y values.
	for (i=0; i<nitems; i++ )	{
		item[i].x = (MenuHires?30:15) + twidth + right_offset;
		item[i].y += (MenuHires?30:15);
		if ( item[i].type==NM_TYPE_RADIO )	{
			fm = -1;	// find first marked one
			for ( j=0; j<nitems; j++ )	{
				if ( item[j].type==NM_TYPE_RADIO && item[j].group==item[i].group )	{
					if (fm==-1 && item[j].value)
						fm = j;
					item[j].value = 0;
				}
			}
			if ( fm>=0 )	
				item[fm].value=1;
			else
				item[i].value=1;
		}
	}

	old_keyd_repeat = keyd_repeat;
	keyd_repeat = 1;

	if (citem==-1)	{
		choice = -1;
	} else {
		if (citem < 0 ) citem = 0;
		if (citem > nitems-1 ) citem = nitems-1;
		choice = citem;

#ifdef NEWMENU_MOUSE
		dblclick_flag = 1;
#endif

		while ( item[choice].type==NM_TYPE_TEXT )	{
			choice++;
			if (choice >= nitems ) {
				choice=0; 
			}
			if (choice == citem ) {
				choice=0; 
				all_text=1;
				break; 
			}
		}
	} 
	done = 0;
   TopChoice=choice;

	gr_update();
	// Clear mouse, joystick to clear button presses.
	game_flush_inputs();

#ifdef NEWMENU_MOUSE
	mouse_state = omouse_state = 0;
	if (filename == NULL && !MenuReordering) {
		draw_close_box(0,0);
		close_box = 1;
	}

	if (!MenuReordering && !joydefs_calibrating)
	{
		newmenu_show_cursor();
# ifdef WINDOWS
		SetCursor(LoadCursor(NULL,IDC_ARROW));
# endif
	}
#endif

   mprintf ((0,"Set to true!\n"));

	while(!done)	{
	#ifdef WINDOWS
		MSG msg;

		DoMessageStuff(&msg);

		if (_RedrawScreen) {
			_RedrawScreen = FALSE;
		
			if (!filename) {
				gr_free_bitmap(bg.saved);
				d_free( bg.background );
			}
			else 	
				gr_free_bitmap(bg.background);

			dd_gr_free_sub_canvas( bg.menu_canvas );
			grd_curcanv->cv_font = save_font;
			dd_grd_curcanv = save_canvas;

			goto RePaintNewmenu4;
		}

		DDGRRESTORE;

	#endif

#ifdef NEWMENU_MOUSE
		if (!joydefs_calibrating)
			newmenu_show_cursor();      // possibly hidden
		omouse_state = mouse_state;
		if (!MenuReordering)
			mouse_state = mouse_button_state(0);
//@@      mprintf ((0,"mouse state:%d\n",mouse_state));
#endif

		//see if redbook song needs to be restarted
		songs_check_redbook_repeat();

		//network_listen();

		k = key_inkey();

    	if (subfunction)
      	(*subfunction)(nitems,item,&k,choice);

#ifdef NETWORK
		if (!time_stopped)	{
			// Save current menu box
			if (multi_menu_poll() == -1)
				k = -2;
		}
#endif

		if ( k<-1 ) {
			dont_restore = (k == -3);		//-3 means don't restore
			choice = k;
			k = -1;
			done = 1;
		}
#ifndef WINDOWS
		if (check_button_press())
			done = 1;
#endif

//		if ( (nmenus<2) && (k>0) && (nothers==0) )
//			done=1;

		old_choice = choice;
	
		switch( k )	{

#ifdef NETWORK
		case KEY_I:
		 if (SurfingNet && !already_showing_info)
		   {
			 show_extra_netgame_info(choice-2);
		   }
		 if (SurfingNet && already_showing_info)
			{
			 done=1;
			 choice=-1;
			}
		 break;
		case KEY_U:
		 if (SurfingNet && !already_showing_info)
		   {
			 network_request_player_names(choice-2);
		   }
		 if (SurfingNet && already_showing_info)
			{
			 done=1;
			 choice=-1;
			}
		 break;
#endif
		case KEY_PAUSE:
		 if (Pauseable_menu)
		   {	
			 Pauseable_menu=0;
			 done=1;
		 	 choice=-1;
		   }
		 break;
		case KEY_TAB + KEY_SHIFTED:
		case KEY_UP:
		case KEY_PAD8:
			if (all_text) break;
			do {
				choice--;

      		if (IsScrollBox)
          	{
           		LastScrollCheck=-1;
            	mprintf ((0,"Scrolling! Choice=%d\n",choice));
                                   
              	if (choice<TopChoice)
                 	{ choice=TopChoice; break; }

             	if (choice<ScrollOffset)
               {
             		for (i=0;i<nitems;i++)
                 		item[i].redraw=1;
                     ScrollOffset--;
                     mprintf ((0,"ScrollOffset=%d\n",ScrollOffset));
               }
           	}
           	else
          	{
           		if (choice >= nitems ) choice=0;
            	if (choice < 0 ) choice=nitems-1;
           	}
			} while ( item[choice].type==NM_TYPE_TEXT );
			if ((item[choice].type==NM_TYPE_INPUT) && (choice!=old_choice))	
				item[choice].value = -1;
			if ((old_choice>-1) && (item[old_choice].type==NM_TYPE_INPUT_MENU) && (old_choice!=choice))	{
				item[old_choice].group=0;
				strcpy(item[old_choice].text, item[old_choice].saved_text );
				item[old_choice].value = -1;
			}
			if (old_choice>-1) 
				item[old_choice].redraw = 1;
			item[choice].redraw=1;
			break;
		case KEY_TAB:
		case KEY_DOWN:
		case KEY_PAD2:
      	// ((0,"Pressing down! IsScrollBox=%d",IsScrollBox));
     		if (all_text) break;
			do {
				choice++;

        		if (IsScrollBox)
          	{
            	LastScrollCheck=-1;
            	mprintf ((0,"Scrolling! Choice=%d\n",choice));
                                   
             	if (choice==nitems)
               	{ choice--; break; }

              	if (choice>=MaxOnMenu+ScrollOffset)
              	{
               	for (i=0;i<nitems;i++)
              			item[i].redraw=1;
                  ScrollOffset++;
                  mprintf ((0,"ScrollOffset=%d\n",ScrollOffset));
              	}
           	}
            else
            {
         		if (choice < 0 ) choice=nitems-1;
           		if (choice >= nitems ) choice=0;
           	}

			} while ( item[choice].type==NM_TYPE_TEXT );
                                                      
			if ((item[choice].type==NM_TYPE_INPUT) && (choice!=old_choice))	
				item[choice].value = -1;
			if ( (old_choice>-1) && (item[old_choice].type==NM_TYPE_INPUT_MENU) && (old_choice!=choice))	{
				item[old_choice].group=0;
				strcpy(item[old_choice].text, item[old_choice].saved_text );	
				item[old_choice].value = -1;
			}
			if (old_choice>-1)
				item[old_choice].redraw=1;
			item[choice].redraw=1;
			break;
		case KEY_SPACEBAR:
			if ( choice > -1 )	{
				switch( item[choice].type )	{
				case NM_TYPE_MENU:
				case NM_TYPE_INPUT:
				case NM_TYPE_INPUT_MENU:
					break;
				case NM_TYPE_CHECK:
					if ( item[choice].value )
						item[choice].value = 0;
					else
						item[choice].value = 1;
					mprintf ((0,"ISB=%d MDI=%d SO=%d choice=%d\n",IsScrollBox,MAXDISPLAYABLEITEMS,ScrollOffset,choice));
					if (IsScrollBox)
					 {
						if (choice==(MaxOnMenu+ScrollOffset-1) || choice==ScrollOffset)
						 {
						   mprintf ((0,"Special redraw!\n"));
							LastScrollCheck=-1;					
						 }
					 }
				
					item[choice].redraw=1;
					break;
				case NM_TYPE_RADIO:
					for (i=0; i<nitems; i++ )	{
						if ((i!=choice) && (item[i].type==NM_TYPE_RADIO) && (item[i].group==item[choice].group) && (item[i].value) )	{
							item[i].value = 0;
							item[i].redraw = 1;
						}
					}
					item[choice].value = 1;
					item[choice].redraw = 1;
					break;
				}	
			}
			break;

                case KEY_SHIFTED+KEY_UP:
                 if (MenuReordering && choice!=TopChoice)
                  {
                   Temp=item[choice].text;
                   TempVal=item[choice].value;
                   item[choice].text=item[choice-1].text;
                   item[choice].value=item[choice-1].value;
                   item[choice-1].text=Temp;
                   item[choice-1].value=TempVal;
                   item[choice].redraw=1;
                   item[choice-1].redraw=1;
                   choice--;
                  }
                 break;
                case KEY_SHIFTED+KEY_DOWN:
                 if (MenuReordering && choice!=(nitems-1))
                  {
                   Temp=item[choice].text;
                   TempVal=item[choice].value;
                   item[choice].text=item[choice+1].text;
                   item[choice].value=item[choice+1].value;
                   item[choice+1].text=Temp;
                   item[choice+1].value=TempVal;
                   item[choice].redraw=1;
                   item[choice+1].redraw=1;
                   choice++;
                  }
                 break;
                
		case KEY_ENTER:
		case KEY_PADENTER:
			if ( (choice>-1) && (item[choice].type==NM_TYPE_INPUT_MENU) && (item[choice].group==0))	{
				item[choice].group = 1;
				item[choice].redraw = 1;
				if ( !strnicmp( item[choice].saved_text, TXT_EMPTY, strlen(TXT_EMPTY) ) )	{
					item[choice].text[0] = 0;
					item[choice].value = -1;
				} else {	
					strip_end_whitespace(item[choice].text);
				}
			} else
				done = 1;
			break;

		case KEY_ESC:
			if ( (choice>-1) && (item[choice].type==NM_TYPE_INPUT_MENU) && (item[choice].group==1))	{
				item[choice].group=0;
				strcpy(item[choice].text, item[choice].saved_text );	
				item[choice].redraw=1;
				item[choice].value = -1;
			} else {
				done = 1;
				choice = -1;
			}
			break;

		MAC(case KEY_COMMAND+KEY_SHIFTED+KEY_3:)
		case KEY_PRINT_SCREEN:
			MAC(newmenu_hide_cursor());
			save_screen_shot(0);
			PA_DFX (pa_set_frontbuffer_current());
			PA_DFX (pa_set_front_to_read());
			for (i=0;i<nitems;i++)
				item[i].redraw=1;
			
			MAC(newmenu_show_cursor());
			MAC(key_flush());
			break;

		#ifdef MACINTOSH

		case KEY_COMMAND+KEY_RIGHT:
			songs_goto_next_song();
			break;
		case KEY_COMMAND+KEY_LEFT:
			songs_goto_prev_song();
			break;
		case KEY_COMMAND+KEY_UP:
			songs_play_level_song(1);
			break;
		case KEY_COMMAND+KEY_DOWN:
			songs_stop_redbook();
			break;

		case KEY_COMMAND+KEY_M:
			k = -1;
			#if !defined(SHAREWARE) || defined(APPLE_DEMO)
			if ( (Game_mode & GM_MULTI) )		// don't process in multiplayer games
				break;

			key_close();		// no processing of keys with keyboard handler.. jeez				
			stop_time();
			newmenu_hide_cursor();
			show_boxed_message ("Mounting CD\nESC to quit");	
			RBAMountDisk();		// OS has totaly control of the CD.
			if (Function_mode == FMODE_MENU)
				songs_play_song(SONG_TITLE,1);
			else if (Function_mode == FMODE_GAME)
				songs_play_level_song( Current_level_num );
			clear_boxed_message();
			newmenu_show_cursor();
			key_init();
			key_flush();
			start_time();
			#endif
			
			break;

		case KEY_COMMAND+KEY_E:
			songs_stop_redbook();
			RBAEjectDisk();
			k = -1;		// force key not to register
			break;
			
		case KEY_COMMAND+KEY_Q: {
			extern void macintosh_quit();
			
			if ( !(Game_mode & GM_MULTI) )
				macintosh_quit();
			if (!joydefs_calibrating)
				newmenu_show_cursor();
			k = -1;		// force key not to register
			break;
		}
		#endif

		#ifndef NDEBUG
		case KEY_BACKSP:	
			if ( (choice>-1) && (item[choice].type!=NM_TYPE_INPUT)&&(item[choice].type!=NM_TYPE_INPUT_MENU))
				Int3(); 
			break;
		#endif

		}

#ifdef NEWMENU_MOUSE // for mouse selection of menu's etc.
		WIN(Sleep(100));
		if ( !done && mouse_state && !omouse_state && !all_text ) {
			mouse_get_pos(&mx, &my);
			for (i=0; i<nitems; i++ )	{
				x1 = grd_curcanv->cv_bitmap.bm_x + item[i].x - item[i].right_offset - 6;
				x2 = x1 + item[i].w;
				y1 = grd_curcanv->cv_bitmap.bm_y + item[i].y;
				y2 = y1 + item[i].h;
				if (((mx > x1) && (mx < x2)) && ((my > y1) && (my < y2))) {
					if (i+ScrollOffset != choice) {
						if(Hack_DblClick_MenuMode) dblclick_flag = 0; 
					}
					
					choice = i + ScrollOffset;

					switch( item[choice].type )	{
					case NM_TYPE_CHECK:
						if ( item[choice].value )
							item[choice].value = 0;
						else
							item[choice].value = 1;
						item[choice].redraw=1;

						if (IsScrollBox)
							LastScrollCheck=-1;
#if 0
						if (IsScrollBox)
						 {
							if (choice==(MaxOnMenu+ScrollOffset-1) || choice==ScrollOffset)
							 {
							   mprintf ((0,"Special redraw!\n"));
								LastScrollCheck=-1;					
							 }
						 }
#endif
						break;
					case NM_TYPE_RADIO:
						for (i=0; i<nitems; i++ )	{
							if ((i!=choice) && (item[i].type==NM_TYPE_RADIO) && (item[i].group==item[choice].group) && (item[i].value) )	{
								item[i].value = 0;
								item[i].redraw = 1;
							}
						}
						item[choice].value = 1;
						item[choice].redraw = 1;
						break;
					}
					item[old_choice].redraw=1;
					break;
				}
			}
		}

		if (mouse_state && all_text)
			done = 1;
		
		if ( !done && mouse_state && !all_text ) {
			mouse_get_pos(&mx, &my);
			
			// check possible scrollbar stuff first
			if (IsScrollBox) {
				int arrow_width, arrow_height, aw;
				
				if (ScrollOffset != 0) {
					gr_get_string_size(UP_ARROW_MARKER, &arrow_width, &arrow_height, &aw);
					x2 = grd_curcanv->cv_bitmap.bm_x + item[ScrollOffset].x-(MenuHires?24:12);
		          	y1 = grd_curcanv->cv_bitmap.bm_y + item[ScrollOffset].y-((string_height+1)*ScrollOffset);
					x1 = x1 - arrow_width;
					y2 = y1 + arrow_height;
					if (((mx > x1) && (mx < x2)) && ((my > y1) && (my < y2)) ) {
						choice--;
		           		LastScrollCheck=-1;
		            	mprintf ((0,"Scrolling! Choice=%d\n",choice));
		                                   
		             	if (choice<ScrollOffset)
		               {
		             		for (i=0;i<nitems;i++)
		                 		item[i].redraw=1;
		                     ScrollOffset--;
		                     mprintf ((0,"ScrollOffset=%d\n",ScrollOffset));
		               }
					}
				}
				if (ScrollOffset+MaxDisplayable<nitems) {
					gr_get_string_size(DOWN_ARROW_MARKER, &arrow_width, &arrow_height, &aw);
					x2 = grd_curcanv->cv_bitmap.bm_x + item[ScrollOffset+MaxDisplayable-1].x-(MenuHires?24:12);
					y1 = grd_curcanv->cv_bitmap.bm_y + item[ScrollOffset+MaxDisplayable-1].y-((string_height+1)*ScrollOffset);
					x1 = x1 - arrow_width;
					y2 = y1 + arrow_height;
					if (((mx > x1) && (mx < x2)) && ((my > y1) && (my < y2)) ) {
						choice++;
		            	LastScrollCheck=-1;
		            	mprintf ((0,"Scrolling! Choice=%d\n",choice));
		                                   
		              	if (choice>=MaxOnMenu+ScrollOffset)
		              	{
		               	for (i=0;i<nitems;i++)
		              			item[i].redraw=1;
		                  ScrollOffset++;
		                  mprintf ((0,"ScrollOffset=%d\n",ScrollOffset));
		              	}
					}
				}
			}
			
			for (i=0; i<nitems; i++ )	{
				x1 = grd_curcanv->cv_bitmap.bm_x + item[i].x - item[i].right_offset - 6;
				x2 = x1 + item[i].w;
				y1 = grd_curcanv->cv_bitmap.bm_y + item[i].y;
				y2 = y1 + item[i].h;
				if (((mx > x1) && (mx < x2)) && ((my > y1) && (my < y2)) && (item[i].type != NM_TYPE_TEXT) ) {
					if (i+ScrollOffset != choice) {
						if(Hack_DblClick_MenuMode) dblclick_flag = 0; 
					}

					choice = i + ScrollOffset;

					if ( item[choice].type == NM_TYPE_SLIDER ) {
						char slider_text[NM_MAX_TEXT_LEN+1], *p, *s1;
						int slider_width, height, aw, sleft_width, sright_width, smiddle_width;
						
						strcpy(slider_text, item[choice].saved_text);
						p = strchr(slider_text, '\t');
						if (p) {
							*p = '\0';
							s1 = p+1;
						}
						if (p) {
							gr_get_string_size(s1, &slider_width, &height, &aw);
							gr_get_string_size(SLIDER_LEFT, &sleft_width, &height, &aw);
							gr_get_string_size(SLIDER_RIGHT, &sright_width, &height, &aw);
							gr_get_string_size(SLIDER_MIDDLE, &smiddle_width, &height, &aw);

							x1 = grd_curcanv->cv_bitmap.bm_x + item[choice].x + item[choice].w - slider_width;
							x2 = x1 + slider_width + sright_width;
							if ( (mx > x1) && (mx < (x1 + sleft_width)) && (item[choice].value != item[choice].min_value) ) {
								item[choice].value = item[choice].min_value;
								item[choice].redraw = 2;
							} else if ( (mx < x2) && (mx > (x2 - sright_width)) && (item[choice].value != item[choice].max_value) ) {
								item[choice].value = item[choice].max_value;
								item[choice].redraw = 2;
							} else if ( (mx > (x1 + sleft_width)) && (mx < (x2 - sright_width)) ) {
								int num_values, value_width, new_value;
								
								num_values = item[choice].max_value - item[choice].min_value + 1;
								value_width = (slider_width - sleft_width - sright_width) / num_values;
								new_value = (mx - x1 - sleft_width) / value_width;
								if ( item[choice].value != new_value ) {
									item[choice].value = new_value;
									item[choice].redraw = 2;
								}
							}
							*p = '\t';
						}
					}
					if (choice == old_choice)
						break;
					if ((item[choice].type==NM_TYPE_INPUT) && (choice!=old_choice))	
						item[choice].value = -1;
					if ((old_choice>-1) && (item[old_choice].type==NM_TYPE_INPUT_MENU) && (old_choice!=choice))	{
						item[old_choice].group=0;
						strcpy(item[old_choice].text, item[old_choice].saved_text );
						item[old_choice].value = -1;
					}
					if (old_choice>-1) 
						item[old_choice].redraw = 1;
					item[choice].redraw=1;
					break;
				}
			}
		}
		
		if ( !done && !mouse_state && omouse_state && !all_text && (choice != -1) && (item[choice].type == NM_TYPE_MENU) ) {
			mouse_get_pos(&mx, &my);
			x1 = grd_curcanv->cv_bitmap.bm_x + item[choice].x;
			x2 = x1 + item[choice].w;
			y1 = grd_curcanv->cv_bitmap.bm_y + item[choice].y;
			y2 = y1 + item[choice].h;
			if (((mx > x1) && (mx < x2)) && ((my > y1) && (my < y2))) {
				if (Hack_DblClick_MenuMode) {
					if (dblclick_flag) done = 1;
					else dblclick_flag = 1;
				}
				else done = 1;
			}
		}
		
		if ( !done && !mouse_state && omouse_state && (choice>-1) && (item[choice].type==NM_TYPE_INPUT_MENU) && (item[choice].group==0))	{
			item[choice].group = 1;
			item[choice].redraw = 1;
			if ( !strnicmp( item[choice].saved_text, TXT_EMPTY, strlen(TXT_EMPTY) ) )	{
				item[choice].text[0] = 0;
				item[choice].value = -1;
			} else {	
				strip_end_whitespace(item[choice].text);
			}
		}
		
		if ( !done && !mouse_state && omouse_state && close_box ) {
			mouse_get_pos(&mx, &my);
			x1 = grd_curcanv->cv_bitmap.bm_x + CLOSE_X;
			x2 = x1 + CLOSE_SIZE;
			y1 = grd_curcanv->cv_bitmap.bm_y + CLOSE_Y;
			y2 = y1 + CLOSE_SIZE;
			if ( ((mx > x1) && (mx < x2)) && ((my > y1) && (my < y2)) ) {
				choice = -1;
				done = 1;
			}
		}

//	 HACK! Don't redraw loadgame preview
		if (RestoringMenu) item[0].redraw = 0;
#endif // NEWMENU_MOUSE

		if ( choice > -1 )	{
			int ascii;

			if ( ((item[choice].type==NM_TYPE_INPUT)||((item[choice].type==NM_TYPE_INPUT_MENU)&&(item[choice].group==1)) )&& (old_choice==choice) )	{
				if ( k==KEY_LEFT || k==KEY_BACKSP || k==KEY_PAD4 )	{
					if (item[choice].value==-1) item[choice].value = strlen(item[choice].text);
					if (item[choice].value > 0)
						item[choice].value--;
					item[choice].text[item[choice].value] = 0;
					item[choice].redraw = 1;	
				} else {
					ascii = key_to_ascii(k);
					if ((ascii < 255 ) && (item[choice].value < item[choice].text_len ))
					{
						int allowed;

						if (item[choice].value==-1) {
							item[choice].value = 0;
						}

						allowed = char_allowed(ascii);

						if (!allowed && ascii==' ' && char_allowed('_')) {
							ascii = '_';
							allowed=1;
						}

						if (allowed) {
							item[choice].text[item[choice].value++] = ascii;
							item[choice].text[item[choice].value] = 0;
							item[choice].redraw=1;	
						}
					}
				}
			} else if ((item[choice].type!=NM_TYPE_INPUT) && (item[choice].type!=NM_TYPE_INPUT_MENU) ) {
				ascii = key_to_ascii(k);
				if (ascii < 255 ) {
					int choice1 = choice;
					ascii = toupper(ascii);
					do {
						int i,ch;
						choice1++;
						if (choice1 >= nitems ) choice1=0;
						for (i=0;(ch=item[choice1].text[i])!=0 && ch==' ';i++);
						if ( ( (item[choice1].type==NM_TYPE_MENU) ||
								 (item[choice1].type==NM_TYPE_CHECK) ||
								 (item[choice1].type==NM_TYPE_RADIO) ||
								 (item[choice1].type==NM_TYPE_NUMBER) ||
								 (item[choice1].type==NM_TYPE_SLIDER) )
								&& (ascii==toupper(ch)) )	{
							k = 0;
							choice = choice1;
							if (old_choice>-1)
								item[old_choice].redraw=1;
							item[choice].redraw=1;
						}
					} while (choice1 != choice );
				}	
			}

			if ( (item[choice].type==NM_TYPE_NUMBER) || (item[choice].type==NM_TYPE_SLIDER)) 	{
				int ov=item[choice].value;
				switch( k ) {
				case KEY_PAD4:
			  	case KEY_LEFT:
			  	case KEY_MINUS:
				case KEY_MINUS+KEY_SHIFTED:
				case KEY_PADMINUS:
					item[choice].value -= 1;
					break;
			  	case KEY_RIGHT:
				case KEY_PAD6:
			  	case KEY_EQUAL:
				case KEY_EQUAL+KEY_SHIFTED:
				case KEY_PADPLUS:
					item[choice].value++;
					break;
				case KEY_PAGEUP:
				case KEY_PAD9:
				case KEY_SPACEBAR:
					item[choice].value += 10;
					break;
				case KEY_PAGEDOWN:
				case KEY_BACKSP:
				case KEY_PAD3:
					item[choice].value -= 10;
					break;
				}
				if (ov!=item[choice].value)
					item[choice].redraw=1;
			}
	
		}

		WINDOS (	dd_gr_set_current_canvas(bg.menu_canvas),
				gr_set_current_canvas(bg.menu_canvas));

    	// Redraw everything...
     	for (i=ScrollOffset; i<MaxDisplayable+ScrollOffset; i++ )
     	{
      	if (item[i].redraw) // warning! ugly hack below                  
        	{
         	item[i].y-=((string_height+1)*ScrollOffset);
			newmenu_hide_cursor();
           	draw_item( &bg, &item[i], (i==choice && !all_text),TinyMode );
				item[i].redraw=0;
#ifdef NEWMENU_MOUSE
				if (!MenuReordering && !joydefs_calibrating)
					newmenu_show_cursor();
#endif
            item[i].y+=((string_height+1)*ScrollOffset);
        	}   
         if (i==choice && (item[i].type==NM_TYPE_INPUT || (item[i].type==NM_TYPE_INPUT_MENU && item[i].group)))
				update_cursor( &item[i]);
		}
	gr_update();

      if (IsScrollBox)
     	{
      	//grd_curcanv->cv_font = NORMAL_FONT;
      	
        	if (LastScrollCheck!=ScrollOffset)
         {
          	LastScrollCheck=ScrollOffset;
          	grd_curcanv->cv_font = SELECTED_FONT;
		  	 	
          	sy=item[ScrollOffset].y-((string_height+1)*ScrollOffset);
          	sx=item[ScrollOffset].x-(MenuHires?24:12);
				
          
          	if (ScrollOffset!=0)
           		nm_rstring( &bg, (MenuHires?20:10), sx, sy, UP_ARROW_MARKER );
          	else
           		nm_rstring( &bg, (MenuHires?20:10), sx, sy, "  " );

          	sy=item[ScrollOffset+MaxDisplayable-1].y-((string_height+1)*ScrollOffset);
          	sx=item[ScrollOffset+MaxDisplayable-1].x-(MenuHires?24:12);
          
          	if (ScrollOffset+MaxDisplayable<nitems)
           		nm_rstring( &bg, (MenuHires?20:10), sx, sy, DOWN_ARROW_MARKER );
          	else
           	nm_rstring( &bg, (MenuHires?20:10), sx, sy, "  " );

       	}

     	}   

		if ( !dont_restore && gr_palette_faded_out )	{
			gr_palette_fade_in( gr_palette, 32, 0 );
		}
	}

	newmenu_hide_cursor();

	// Restore everything...

	WINDOS (	dd_gr_set_current_canvas(bg.menu_canvas),
			gr_set_current_canvas(bg.menu_canvas));

	if ( filename == NULL )	{
		// Save the background under the menu...
		WIN (DDGRLOCK(dd_grd_curcanv));
			gr_bitmap(0, 0, bg.saved); 	
		WIN (DDGRUNLOCK(dd_grd_curcanv));
		gr_free_bitmap(bg.saved);
		d_free( bg.background );
	} else {
		if (!dont_restore)	//info passed back from subfunction
		{
			WIN (DDGRLOCK(dd_grd_curcanv));
			gr_bitmap(0, 0, bg.background);
			WIN (DDGRUNLOCK(dd_grd_curcanv)); 	
		}
		gr_free_bitmap(bg.background);
	}

	WINDOS (	dd_gr_free_sub_canvas(bg.menu_canvas),
			gr_free_sub_canvas( bg.menu_canvas ) );

	WINDOS (dd_gr_set_current_canvas(NULL), gr_set_current_canvas( NULL ));			
	grd_curcanv->cv_font	= save_font;
	WINDOS (dd_gr_set_current_canvas(NULL), gr_set_current_canvas( save_canvas ));			
	keyd_repeat = old_keyd_repeat;

	game_flush_inputs();

	if (time_stopped) 
     {
		start_time();
		#ifdef TACTILE
			if (TactileStick)
				EnableForces();
		#endif
	  }

	if ( sound_stopped )
		digi_resume_digi_sounds();

	WIN(mouse_set_mode(1));				//re-enable centering mode

	return choice;
	
}


int nm_messagebox1( char *title, void (*subfunction)(int nitems,newmenu_item * items, int * last_key, int citem), int nchoices, ... )
{
	int i;
	char * format;
	va_list args;
	char *s;
	char nm_text[MESSAGEBOX_TEXT_SIZE];
	newmenu_item nm_message_items[5];

	va_start(args, nchoices );

	Assert( nchoices <= 5 );

	for (i=0; i<nchoices; i++ )	{
                s = va_arg( args, char * );
                nm_message_items[i].type = NM_TYPE_MENU; nm_message_items[i].text = s;
	}
	format = va_arg( args, char * );
	strcpy( nm_text, "" );
	vsprintf(nm_text,format,args);
	va_end(args);

	Assert(strlen(nm_text) < MESSAGEBOX_TEXT_SIZE);

	return newmenu_do( title, nm_text, nchoices, nm_message_items, subfunction );
}

int nm_messagebox( char *title, int nchoices, ... )
{
	int i;
	char * format;
	va_list args;
	char *s;
	char nm_text[MESSAGEBOX_TEXT_SIZE];
	newmenu_item nm_message_items[5];

	va_start(args, nchoices );

	Assert( nchoices <= 5 );

	for (i=0; i<nchoices; i++ )	{
		s = va_arg( args, char * );
		nm_message_items[i].type = NM_TYPE_MENU; nm_message_items[i].text = s;
	}
	format = va_arg( args, char * );
	strcpy( nm_text, "" );
	vsprintf(nm_text,format,args);
	va_end(args);

	Assert(strlen(nm_text) < MESSAGEBOX_TEXT_SIZE );

	return newmenu_do( title, nm_text, nchoices, nm_message_items, NULL );
}




void newmenu_file_sort( int n, char *list )
{
	int i, j, incr;
	char t[14];

	incr = n / 2;
	while( incr > 0 )		{
		for (i=incr; i<n; i++ )		{
			j = i - incr;
			while (j>=0 )			{
				if (strncmp(&list[j*14], &list[(j+incr)*14], 12) > 0)				{
                                        memcpy( t, &list[j*14], FILENAME_LEN );
                                        memcpy( &list[j*14], &list[(j+incr)*14], FILENAME_LEN );
                                        memcpy( &list[(j+incr)*14], t, FILENAME_LEN );
					j -= incr;
				}
				else
					break;
			}
		}
		incr = incr / 2;
	}
}

void delete_player_saved_games(char * name)
{
	int i;
	char filename[16];
	
	for (i=0;i<10; i++)	{
#ifndef MACINTOSH
		sprintf( filename, "%s.sg%d", name, i );
#else
		sprintf( filename, ":Players:%s.sg%d", name, i );
#endif
		cfile_delete(filename);
	}
}

#define MAX_FILES 300

int MakeNewPlayerFile(int allow_abort);

extern char AltHogDir[64];
extern char AltHogdir_initialized;

int newmenu_get_filename( char * title, char * filespec, char * filename, int allow_abort_flag )
{
	int i;
	FILEFINDSTRUCT find;
	int NumFiles=0, key,done, citem, ocitem;
	char * filenames = NULL;
	int NumFiles_displayed = 8;
	int first_item = -1, ofirst_item;
	int old_keyd_repeat = keyd_repeat;
	int player_mode=0;
	int demo_mode=0;
	int demos_deleted=0;
	int initialized = 0;
	int exit_value = 0;
	int w_x, w_y, w_w, w_h, title_height;
	int box_x, box_y, box_w, box_h;
	bkg bg;		// background under listbox
#ifdef NEWMENU_MOUSE
	int mx, my, x1, x2, y1, y2, mouse_state, omouse_state;
	int mouse2_state, omouse2_state;
	int dblclick_flag=0;
# ifdef WINDOWS
	int simukey=0;
	int show_up_arrow=0,show_down_arrow=0;
# endif
#endif
WIN(int win_redraw=0);

	w_x=w_y=w_w=w_h=title_height=0;
	box_x=box_y=box_w=box_h=0;

	filenames = d_malloc( MAX_FILES * 14 );
	if (filenames==NULL) return 0;

	citem = 0;
	keyd_repeat = 1;

	WIN(mouse_set_mode(0));				//disable centering mode

	if (strstr( filespec, "*.plr" ))
		player_mode = 1;
	else if (strstr( filespec, "*.dem" ))
		demo_mode = 1;

ReadFileNames:
	done = 0;
	NumFiles=0;
	
#if !defined(APPLE_DEMO)		// no new pilots for special apple oem version
	if (player_mode)	{
		strncpy( &filenames[NumFiles*14], TXT_CREATE_NEW, FILENAME_LEN );
		NumFiles++;
	}
#endif

	if( !FileFindFirst( filespec, &find ) )	{
		do	{
			if (NumFiles<MAX_FILES)	{
                                strncpy( &filenames[NumFiles*14], find.name, FILENAME_LEN );
				if ( player_mode )	{
					char * p;
					p = strchr(&filenames[NumFiles*14],'.');
					if (p) *p = '\0';
				}
				NumFiles++;
			} else {
				break;
			}
		} while( !FileFindNext( &find ) );
		FileFindClose();
	}

	if (demo_mode && AltHogdir_initialized) {
		char filespec2[PATH_MAX + FILENAME_LEN];
		strcpy(filespec2, AltHogDir);
		strcat(filespec2, "/");
		strcat(filespec2, filespec);
		if ( !FileFindFirst( filespec2, &find ) ) {
			do {
				if (NumFiles<MAX_FILES)	{
					strncpy( &filenames[NumFiles*14], find.name, FILENAME_LEN );
					NumFiles++;
				} else {
					break;
				}
			} while( !FileFindNext( &find ) );
			FileFindClose();
		}
	}

	if ( (NumFiles < 1) && demos_deleted )	{
		exit_value = 0;
		goto ExitFileMenu;
	}
	if ( (NumFiles < 1) && demo_mode ) {
		nm_messagebox( NULL, 1, TXT_OK, "%s %s\n%s", TXT_NO_DEMO_FILES, TXT_USE_F5, TXT_TO_CREATE_ONE);
		exit_value = 0;
		goto ExitFileMenu;
	}

	#ifndef APPLE_DEMO
	if ( (NumFiles < 2) && player_mode ) {
		citem = 0;
		goto ExitFileMenuEarly;
	}
	#endif


	if ( NumFiles<1 )	{
		#ifndef APPLE_DEMO
			nm_messagebox( NULL, 1, "Ok", "%s\n '%s' %s", TXT_NO_FILES_MATCHING, filespec, TXT_WERE_FOUND);
		#endif
		exit_value = 0;
		goto ExitFileMenu;
	}

	if (!initialized) {	
//		set_screen_mode(SCREEN_MENU);
		set_popup_screen();

        #ifdef WINDOWS
RePaintNewmenuFile:

		dd_gr_set_current_canvas(NULL);
	#else
		gr_set_current_canvas(NULL);
	#endif

		WIN(DDGRLOCK(dd_grd_curcanv))					//mwa put these here -- are these needed Samir???
		{
			grd_curcanv->cv_font = SUBTITLE_FONT;
		}
		WIN(DDGRUNLOCK(dd_grd_curcanv));

		w_w = 0;
		w_h = 0;

		for (i=0; i<NumFiles; i++ ) {
			int w, h, aw;
			gr_get_string_size( &filenames[i*14], &w, &h, &aw );		
			if ( w > w_w )
				w_w = w;
		}
		if ( title ) {
			int w, h, aw;
			gr_get_string_size( title, &w, &h, &aw );		
			if ( w > w_w )
				w_w = w;
			title_height = h + (grd_curfont->ft_h*2);		// add a little space at the bottom of the title
		}

		box_w = w_w;
		box_h = ((grd_curfont->ft_h + 2) * NumFiles_displayed);

		w_w += (grd_curfont->ft_w * 4);
		w_h = title_height + box_h + (grd_curfont->ft_h * 2);		// more space at bottom

		if ( w_w > grd_curcanv->cv_w ) w_w = grd_curcanv->cv_w;
		if ( w_h > grd_curcanv->cv_h ) w_h = grd_curcanv->cv_h;
	
		w_x = (grd_curcanv->cv_w-w_w)/2;
		w_y = (grd_curcanv->cv_h-w_h)/2;
	
		if ( w_x < 0 ) w_x = 0;
		if ( w_y < 0 ) w_y = 0;

		box_x = w_x + (grd_curfont->ft_w*2);			// must be in sync with w_w!!!
		box_y = w_y + title_height;

// save the screen behind the menu.

		bg.saved = NULL;

	#if !defined(WINDOWS)
		if ( (VR_offscreen_buffer->cv_w >= w_w) && (VR_offscreen_buffer->cv_h >= w_h) ) 
			bg.background = &VR_offscreen_buffer->cv_bitmap;
		else
	#endif
#if defined(POLY_ACC)
			bg.background = gr_create_bitmap2( w_w, w_h, grd_curcanv->cv_bitmap.bm_type, NULL );
#else
			bg.background = gr_create_bitmap( w_w, w_h );
#endif

		Assert( bg.background != NULL );


		WIN(DDGRLOCK(dd_grd_curcanv));
		gr_bm_bitblt(w_w, w_h, 0, 0, w_x, w_y, &grd_curcanv->cv_bitmap, bg.background );
		WIN(DDGRUNLOCK(dd_grd_curcanv));

#if 0
		WINDOS(
	 		dd_gr_blt_notrans(dd_grd_curcanv, 0, 0, 
				_DDModeList[W95DisplayMode].rw, _DDModeList[W95DisplayMode].rh, 
				dd_VR_offscreen_buffer, 0, 0, 
				_DDModeList[W95DisplayMode].rw, _DDModeList[W95DisplayMode].rh),
			gr_bm_bitblt(grd_curcanv->cv_w, grd_curcanv->cv_h, 0, 0, 0, 0, &(grd_curcanv->cv_bitmap), &(VR_offscreen_buffer->cv_bitmap) )
		);
#endif

		nm_draw_background( w_x,w_y,w_x+w_w-1,w_y+w_h-1 );
		
		WIN(DDGRLOCK(dd_grd_curcanv))
		{	
			gr_string( 0x8000, w_y+10, title );
		}
		WIN(DDGRUNLOCK(dd_grd_curcanv));

		WIN(DDGRRESTORE);
	 
		initialized = 1;
	}

	if ( !player_mode )	{
		newmenu_file_sort( NumFiles, filenames );
	} else {
		#if defined(MACINTOSH) && defined(APPLE_DEMO)
		newmenu_file_sort( NumFiles, filenames );
		#else
		newmenu_file_sort( NumFiles-1, &filenames[14] );		// Don't sort first one!
		#endif
		for ( i=0; i<NumFiles; i++ )	{
			if (!stricmp(Players[Player_num].callsign, &filenames[i*14]) )	{
#ifdef NEWMENU_MOUSE
				dblclick_flag = 1;
#endif
				citem = i;
			}
	 	}
	}

#ifdef NEWMENU_MOUSE
	mouse_state = omouse_state = 0;
	mouse2_state = omouse2_state = 0;
	draw_close_box(w_x,w_y);
	newmenu_show_cursor();
#endif

	while(!done)	{
	#ifdef WINDOWS
		MSG msg;

		DoMessageStuff(&msg);

		if (_RedrawScreen) {
			_RedrawScreen = FALSE;

			if ( bg.background != &VR_offscreen_buffer->cv_bitmap )
				gr_free_bitmap(bg.background);
	
			win_redraw = 1;		
			goto RePaintNewmenuFile;
		}

		DDGRRESTORE
	#endif

		ocitem = citem;
		ofirst_item = first_item;
		gr_update();

#ifdef NEWMENU_MOUSE
		omouse_state = mouse_state;
		omouse2_state = mouse2_state;
		mouse_state = mouse_button_state(0);
		mouse2_state = mouse_button_state(1);
#endif

		//see if redbook song needs to be restarted
		songs_check_redbook_repeat();

		#ifdef WINDOWS
		if (!mouse2_state && omouse2_state)
			key = KEY_CTRLED+KEY_D;		//fake ctrl-d
		else
		#endif
			//NOTE LINK TO ABOVE ELSE
			key = key_inkey();

	#ifdef WINDOWS
		if (simukey==-1)
			key=KEY_UP;
		else if (simukey==1)
		   key=KEY_DOWN;
		simukey=0;
	#endif
			
		switch(key)	{
		MAC(case KEY_COMMAND+KEY_SHIFTED+KEY_3:)
		case KEY_PRINT_SCREEN:
			MAC(newmenu_hide_cursor());
			save_screen_shot(0);
			PA_DFX (pa_set_frontbuffer_current());
			PA_DFX (pa_set_front_to_read());
			
			MAC(newmenu_show_cursor());
			MAC(key_flush());
			break;

		case KEY_CTRLED+KEY_D:
			#if defined(MACINTOSH) && defined(APPLE_DEMO)
			break;
			#endif

			if ( ((player_mode)&&(citem>0)) || ((demo_mode)&&(citem>=0)) )	{
				int x = 1;
				#ifdef WINDOWS
				mouse_set_mode(1);				//re-enable centering mode
				#endif
				newmenu_hide_cursor();
				if (player_mode)
					x = nm_messagebox( NULL, 2, TXT_YES, TXT_NO, "%s %s?", TXT_DELETE_PILOT, &filenames[citem*14]+((player_mode && filenames[citem*14]=='$')?1:0) );
				else if (demo_mode)
					x = nm_messagebox( NULL, 2, TXT_YES, TXT_NO, "%s %s?", TXT_DELETE_DEMO, &filenames[citem*14]+((demo_mode && filenames[citem*14]=='$')?1:0) );
				#ifdef WINDOWS
				mouse_set_mode(0);				//disenable centering mode
				#endif
				newmenu_show_cursor();
 				if (x==0)	{
					char * p;
					int ret;
					char name[_MAX_PATH],dir[_MAX_DIR];

					p = &filenames[(citem*14)+strlen(&filenames[citem*14])];
					if (player_mode)
						*p = '.';

					_splitpath(filespec,name,dir,NULL,NULL);
					strcat(name,dir);
					strcat(name,&filenames[citem*14]);
					
					#ifdef MACINTOSH
					{
						int i;
						char *p;
						
						if ( !strncmp(name, ".\\", 2) )
							for (i = 0; i < strlen(name); i++)		// don't subtract 1 from strlen to get the EOS marker
								name[i] = name[i+1];
						while ( (p = strchr(name, '\\')) )
							*p = ':';
					}
					#endif
				
					ret = cfile_delete(name);
					if (player_mode)
						*p = 0;

					if ((!ret) && player_mode)	{
						delete_player_saved_games( &filenames[citem*14] );
					}

					if (ret) {
						if (player_mode)
							nm_messagebox( NULL, 1, TXT_OK, "%s %s %s", TXT_COULDNT, TXT_DELETE_PILOT, &filenames[citem*14]+((player_mode && filenames[citem*14]=='$')?1:0) );
						else if (demo_mode)
							nm_messagebox( NULL, 1, TXT_OK, "%s %s %s", TXT_COULDNT, TXT_DELETE_DEMO, &filenames[citem*14]+((demo_mode && filenames[citem*14]=='$')?1:0) );
					} else if (demo_mode)
						demos_deleted = 1;
					first_item = -1;
					goto ReadFileNames;
				}
			}
			break;
		case KEY_HOME:
		case KEY_PAD7:
			citem = 0;
			break;
		case KEY_END:
		case KEY_PAD1:
			citem = NumFiles-1;
			break;
		case KEY_UP:
		case KEY_PAD8:
			citem--;			
			break;
		case KEY_DOWN:
		case KEY_PAD2:
			citem++;			
			break;
 		case KEY_PAGEDOWN:
		case KEY_PAD3:
			citem += NumFiles_displayed;
			break;
		case KEY_PAGEUP:
		case KEY_PAD9:
			citem -= NumFiles_displayed;
			break;
		case KEY_ESC:
			if (allow_abort_flag) {
				citem = -1;
				done = 1;
			}
			break;
		case KEY_ENTER:
		case KEY_PADENTER:
			done = 1;
			break;
			
		#ifdef MACINTOSH
		case KEY_COMMAND+KEY_Q: {
			extern void macintosh_quit();
			
			if ( !(Game_mode & GM_MULTI) )
				macintosh_quit();
			newmenu_show_cursor();
			key_flush();
			break;
		}
		#endif
		
		default:	
			{

				int ascii = key_to_ascii(key);
				if ( ascii < 255 )	{
					int cc,cc1;
					cc=cc1=citem+1;
					if (cc1 < 0 )  cc1 = 0;
					if (cc1 >= NumFiles )  cc1 = 0;
					while(1) {
						if ( cc < 0 ) cc = 0;
						if ( cc >= NumFiles ) cc = 0;
						if ( citem == cc ) break;
	
						if ( toupper(filenames[cc*14]) == toupper(ascii) )	{
							citem = cc;
							break;
						}
						cc++;
					}
				}
			}
		}
		if ( done ) break;


		if (citem<0)
			citem=0;

		if (citem>=NumFiles)
			citem = NumFiles-1;

		if (citem< first_item)
			first_item = citem;

		if (citem>=( first_item+NumFiles_displayed))
		{
			first_item = citem-NumFiles_displayed+1;
		}

#ifdef WINDOWS
		if (NumFiles>first_item+NumFiles_displayed)
			show_down_arrow=1;
		else 
			show_down_arrow=0;
		if (first_item>0)
			show_up_arrow=1;
		else	
			show_up_arrow=0;
#endif
			

		if (NumFiles <= NumFiles_displayed )
			 first_item = 0;

		if (first_item>NumFiles-NumFiles_displayed)
		{
			first_item = NumFiles-NumFiles_displayed;
		}

		if (first_item < 0 ) first_item = 0;

#ifdef NEWMENU_MOUSE
		WIN(Sleep(100));
		if (mouse_state || mouse2_state) {
			int w, h, aw;

			mouse_get_pos(&mx, &my);
			for (i=first_item; i<first_item+NumFiles_displayed; i++ )	{
				gr_get_string_size(&filenames[i*14], &w, &h, &aw  );
				x1 = box_x;
				x2 = box_x + box_w - 1;
				y1 = (i-first_item)*(grd_curfont->ft_h + 2) + box_y;
				y2 = y1+h+1;
				if ( ((mx > x1) && (mx < x2)) && ((my > y1) && (my < y2)) ) {
					if (i == citem && !mouse2_state) {
						break;
					}
					citem = i;
					dblclick_flag = 0;
					break;
				}
			}
		}
		
		if (!mouse_state && omouse_state) {
			int w, h, aw;

			gr_get_string_size(&filenames[citem*14], &w, &h, &aw  );
			mouse_get_pos(&mx, &my);
			x1 = box_x;
			x2 = box_x + box_w - 1;
			y1 = (citem-first_item)*(grd_curfont->ft_h + 2) + box_y;
			y2 = y1+h+1;
			if ( ((mx > x1) && (mx < x2)) && ((my > y1) && (my < y2)) ) {
				if (dblclick_flag) done = 1;
				else dblclick_flag = 1;
			}
		}

		if ( !mouse_state && omouse_state ) {
			mouse_get_pos(&mx, &my);
			x1 = w_x + CLOSE_X + 2;
			x2 = x1 + CLOSE_SIZE - 2;
			y1 = w_y + CLOSE_Y + 2;
			y2 = y1 + CLOSE_SIZE - 2;
			if ( ((mx > x1) && (mx < x2)) && ((my > y1) && (my < y2)) ) {
				citem = -1;
				done = 1;
			}
		   #ifdef WINDOWS
			x1 = box_x-LHX(10);
			x2 = x1 + LHX(10);
			y1 = box_y;
			y2 = box_y+LHY(7);
			if ( ((mx > x1) && (mx < x2)) && ((my > y1) && (my < y2)) && show_up_arrow ) 
				simukey = -1;
			y1 = box_y+box_h-LHY(7);
			y2 = box_y+box_h;
			if ( ((mx > x1) && (mx < x2)) && ((my > y1) && (my < y2)) && show_down_arrow) 
				simukey = 1;
		   #endif
		}

#endif
  
	WIN(DDGRLOCK(dd_grd_curcanv));
		gr_setcolor( BM_XRGB(2,2,2));
		//gr_rect( box_x - 1, box_y-2, box_x + box_w, box_y-2 );
		gr_setcolor( BM_XRGB( 0,0,0)  );

	#ifdef WINDOWS
		if (ofirst_item != first_item || win_redraw)	{
			win_redraw = 0;
	#else
		if (ofirst_item != first_item)	{
	#endif
			newmenu_hide_cursor();
			gr_setcolor( BM_XRGB( 0,0,0)  );
			for (i=first_item; i<first_item+NumFiles_displayed; i++ )	{
				int w, h, aw, y;
				y = (i-first_item)*(grd_curfont->ft_h + 2) + box_y;
			
				if ( i >= NumFiles )	{

					gr_setcolor( BM_XRGB(5,5,5));
					gr_rect( box_x + box_w, y-1, box_x + box_w, y + grd_curfont->ft_h + 1);
					//gr_rect( box_x, y + grd_curfont->ft_h + 2, box_x + box_w, y + grd_curfont->ft_h + 2);
					
					gr_setcolor( BM_XRGB(2,2,2));
					gr_rect( box_x - 1, y - 1, box_x - 1, y + grd_curfont->ft_h + 2 );
					
					gr_setcolor( BM_XRGB(0,0,0));
					gr_rect( box_x, y - 1, box_x + box_w - 1, y + grd_curfont->ft_h + 1);
					
				} else {
					if ( i == citem )	
						grd_curcanv->cv_font = SELECTED_FONT;
					else	
						grd_curcanv->cv_font = NORMAL_FONT;
					gr_get_string_size(&filenames[i*14], &w, &h, &aw  );

					gr_setcolor( BM_XRGB(5,5,5));
				  //	gr_rect( box_x, y + h + 2, box_x + box_w, y + h + 2);
					gr_rect( box_x + box_w, y - 1, box_x + box_w, y + h + 1);
					
					gr_setcolor( BM_XRGB(2,2,2));
					gr_rect( box_x - 1, y - 1, box_x - 1, y + h + 1);
					gr_setcolor( BM_XRGB(0,0,0));
							
					gr_rect( box_x, y-1, box_x + box_w - 1, y + h + 1 );
					gr_string( box_x + 5, y, (&filenames[i*14])+((player_mode && filenames[i*14]=='$')?1:0)  );
				}
			}	 
			newmenu_show_cursor();
		} else if ( citem != ocitem )	{
			int w, h, aw, y;

			newmenu_hide_cursor();
			i = ocitem;
			if ( (i>=0) && (i<NumFiles) )	{
				y = (i-first_item)*(grd_curfont->ft_h+2)+box_y;
				if ( i == citem )	
					grd_curcanv->cv_font = SELECTED_FONT;
				else	
					grd_curcanv->cv_font = NORMAL_FONT;
				gr_get_string_size(&filenames[i*14], &w, &h, &aw  );
				gr_rect( box_x, y-1, box_x + box_w - 1, y + h + 1 );
				gr_string( box_x + 5, y, (&filenames[i*14])+((player_mode && filenames[i*14]=='$')?1:0)  );
			}
			i = citem;
			if ( (i>=0) && (i<NumFiles) )	{
				y = (i-first_item)*(grd_curfont->ft_h+2)+box_y;
				if ( i == citem )	
					grd_curcanv->cv_font = SELECTED_FONT;
				else	
					grd_curcanv->cv_font = NORMAL_FONT;
				gr_get_string_size(&filenames[i*14], &w, &h, &aw  );
				gr_rect( box_x, y-1, box_x + box_x - 1, y + h + 1 );
				gr_string( box_x + 5, y, (&filenames[i*14])+((player_mode && filenames[i*14]=='$')?1:0)  );
			}
			newmenu_show_cursor();
		}

	#ifdef WINDOWS   
			grd_curcanv->cv_font = NORMAL_FONT;
			if (show_up_arrow)
	 	  		gr_string( box_x-LHX(10), box_y ,UP_ARROW_MARKER );
			else
			{
				No_darkening=1;
				nm_draw_background (box_x-LHX(10),box_y,box_x-2,box_y+LHY(7));
				No_darkening=0;
			}

			if (show_down_arrow)
	     		gr_string( box_x-LHX(10), box_y+box_h-LHY(7) ,DOWN_ARROW_MARKER );
			else
			{
				No_darkening=1;
				nm_draw_background (box_x-LHX(10),box_y+box_h-LHY(7),box_x-2,box_y+box_h);
				No_darkening=0;
			}

	#endif



	WIN(DDGRUNLOCK(dd_grd_curcanv));
	}

ExitFileMenuEarly:
	MAC(newmenu_hide_cursor());
	if ( citem > -1 )	{
		strncpy( filename, (&filenames[citem*14])+((player_mode && filenames[citem*14]=='$')?1:0), FILENAME_LEN );
		exit_value = 1;
	} else {
		exit_value = 0;
	}											 

ExitFileMenu:
	keyd_repeat = old_keyd_repeat;

	if ( initialized )	{
			if (Newdemo_state != ND_STATE_PLAYBACK)	//horrible hack to prevent restore when screen has been cleared
			{
			WIN (DDGRLOCK(dd_grd_curcanv));
				gr_bm_bitblt(w_w, w_h, w_x, w_y, 0, 0, bg.background, &grd_curcanv->cv_bitmap );
			WIN (DDGRUNLOCK(dd_grd_curcanv)); 	
			}
			if ( bg.background != &VR_offscreen_buffer->cv_bitmap )
				gr_free_bitmap(bg.background);
#if 0
		WINDOS(
			dd_gr_blt_notrans(dd_VR_offscreen_buffer,
				0,0,_DDModeList[W95DisplayMode].rw, _DDModeList[W95DisplayMode].rh,
				dd_grd_curcanv,
				0,0,_DDModeList[W95DisplayMode].rw, _DDModeList[W95DisplayMode].rh),
			gr_bm_bitblt(grd_curcanv->cv_w, grd_curcanv->cv_h, 0, 0, 0, 0, &(VR_offscreen_buffer->cv_bitmap), &(grd_curcanv->cv_bitmap) )
		);
#endif

		WIN(DDGRRESTORE);
	}

	if ( filenames )
		d_free(filenames);

	WIN(mouse_set_mode(1));				//re-enable centering mode
	WIN(newmenu_hide_cursor());

	return exit_value;

}


// Example listbox callback function...
// int lb_callback( int * citem, int *nitems, char * items[], int *keypress )
// {
// 	int i;
// 
// 	if ( *keypress = KEY_CTRLED+KEY_D )	{
// 		if ( *nitems > 1 )	{
// 			cfile_delete(items[*citem]);    // Delete the file
// 			for (i=*citem; i<*nitems-1; i++ )	{
// 				items[i] = items[i+1];
// 			}
// 			*nitems = *nitems - 1;
// 			d_free( items[*nitems] );
// 			items[*nitems] = NULL;
// 			return 1;	// redraw;
// 		}
//			*keypress = 0;
// 	}			
// 	return 0;
// }

#define LB_ITEMS_ON_SCREEN 8

int newmenu_listbox( char * title, int nitems, char * items[], int allow_abort_flag, int (*listbox_callback)( int * citem, int *nitems, char * items[], int *keypress ) )
{
	return newmenu_listbox1( title, nitems, items, allow_abort_flag, 0, listbox_callback );
}

int newmenu_listbox1( char * title, int nitems, char * items[], int allow_abort_flag, int default_item, int (*listbox_callback)( int * citem, int *nitems, char * items[], int *keypress ) )
{
	int i;
	int done, ocitem,citem, ofirst_item, first_item, key, redraw;
	int old_keyd_repeat = keyd_repeat;
	int width, height, wx, wy, title_height, border_size;
	int total_width,total_height;
	bkg bg;
#ifdef NEWMENU_MOUSE
	int mx, my, x1, x2, y1, y2, mouse_state, omouse_state;	//, dblclick_flag;
	int close_x,close_y;
# ifdef WINDOWS
   int simukey=0,show_up_arrow=0,show_down_arrow=0;
# endif
#endif
WIN(int win_redraw=0);

	keyd_repeat = 1;

   PA_DFX (pa_set_frontbuffer_current());
	PA_DFX (pa_set_front_to_read());
	WIN(mouse_set_mode(0));				//disable centering mode

//	set_screen_mode(SCREEN_MENU);
	set_popup_screen();

#ifdef WINDOWS
RePaintNewmenuListbox:
 
	dd_gr_set_current_canvas(NULL);
#else
	gr_set_current_canvas(NULL);
#endif

	grd_curcanv->cv_font = SUBTITLE_FONT;

	width = 0;
	for (i=0; i<nitems; i++ )	{
		int w, h, aw;
		gr_get_string_size( items[i], &w, &h, &aw );		
		if ( w > width )
			width = w;
	}
	height = (grd_curfont->ft_h + 2) * LB_ITEMS_ON_SCREEN;

	{
		int w, h, aw;
		gr_get_string_size( title, &w, &h, &aw );		
		if ( w > width )
			width = w;
		title_height = h + 5;
	}

	border_size = grd_curfont->ft_w;
   WIN (border_size=grd_curfont->ft_w*2);
		
	width += (grd_curfont->ft_w);
	if ( width > grd_curcanv->cv_w - (grd_curfont->ft_w * 3) )
		width = grd_curcanv->cv_w - (grd_curfont->ft_w * 3);

	wx = (grd_curcanv->cv_bitmap.bm_w-width)/2;
	wy = (grd_curcanv->cv_bitmap.bm_h-(height+title_height))/2 + title_height;
	if ( wy < title_height )
		wy = title_height;

	total_width = width+2*border_size;
	total_height = height+2*border_size+title_height;

	bg.saved = NULL;

#if !defined(WINDOWS)
	if ( (VR_offscreen_buffer->cv_w >= total_width) && (VR_offscreen_buffer->cv_h >= total_height) )
		bg.background = &VR_offscreen_buffer->cv_bitmap;
	else
#endif
		//bg.background = gr_create_bitmap( width, (height + title_height) );
#if defined(POLY_ACC)
		bg.background = gr_create_bitmap2(total_width, total_height, grd_curcanv->cv_bitmap.bm_type, NULL);
#else
		bg.background = gr_create_bitmap(total_width,total_height);
#endif
	Assert( bg.background != NULL );
		
	WIN (DDGRLOCK(dd_grd_curcanv));
		//gr_bm_bitblt(wx+width+border_size, wy+height+border_size, 0, 0, wx-border_size, wy-title_height-border_size, &grd_curcanv->cv_bitmap, bg.background );
		gr_bm_bitblt(total_width,total_height, 0, 0, wx-border_size, wy-title_height-border_size, &grd_curcanv->cv_bitmap, bg.background );
	WIN (DDGRUNLOCK(dd_grd_curcanv));

#if 0
	WINDOS(
 		dd_gr_blt_notrans(dd_grd_curcanv, 0, 0, 
				_DDModeList[W95DisplayMode].rw, _DDModeList[W95DisplayMode].rh, 
				dd_VR_offscreen_buffer, 0, 0, 
				_DDModeList[W95DisplayMode].rw, _DDModeList[W95DisplayMode].rh),
		gr_bm_bitblt(grd_curcanv->cv_w, grd_curcanv->cv_h, 0, 0, 0, 0, &(grd_curcanv->cv_bitmap), &(VR_offscreen_buffer->cv_bitmap) )
	);
#endif

	nm_draw_background( wx-border_size,wy-title_height-border_size,wx+width+border_size-1,wy+height+border_size-1 );

	WIN(DDGRLOCK(dd_grd_curcanv));
		gr_string( 0x8000, wy - title_height, title );
	WIN(DDGRUNLOCK(dd_grd_curcanv));	

	WIN(DDGRRESTORE);

	done = 0;
	citem = default_item;
	if ( citem < 0 ) citem = 0;
	if ( citem >= nitems ) citem = 0;

	first_item = -1;

#ifdef NEWMENU_MOUSE
	mouse_state = omouse_state = 0;	//dblclick_flag = 0;
	close_x = wx-border_size;
	close_y = wy-title_height-border_size;
	draw_close_box(close_x,close_y);
	newmenu_show_cursor();
#endif

	while(!done)	{
	#ifdef WINDOWS
		MSG msg;

		DoMessageStuff(&msg);

		if (_RedrawScreen) {
			_RedrawScreen = FALSE;

			if ( bg.background != &VR_offscreen_buffer->cv_bitmap )
				gr_free_bitmap(bg.background);
			win_redraw = 1;			
			goto RePaintNewmenuListbox;
		}

	  	DDGRRESTORE;
	#endif
  
		ocitem = citem;
		ofirst_item = first_item;
#ifdef NEWMENU_MOUSE
		omouse_state = mouse_state;
		mouse_state = mouse_button_state(0);
#endif
		//see if redbook song needs to be restarted
		songs_check_redbook_repeat();

		key = key_inkey();

		if ( listbox_callback )
			redraw = (*listbox_callback)(&citem, &nitems, items, &key );
		else
			redraw = 0;

	#ifdef WINDOWS
		if (win_redraw) {
			redraw = 1;
			win_redraw = 0;
		}
	#endif

		if ( key<-1 ) {
			citem = key;
			key = -1;
			done = 1;
		}


	#ifdef WINDOWS
		if (simukey==-1)
			key=KEY_UP;
		else if (simukey==1)
		   key=KEY_DOWN;
		simukey=0;
	#endif
		
		switch(key)	{
		MAC(case KEY_COMMAND+KEY_SHIFTED+KEY_3:)
		case KEY_PRINT_SCREEN: 		
			MAC(newmenu_hide_cursor());
			save_screen_shot(0); 
			PA_DFX (pa_set_frontbuffer_current());
			PA_DFX (pa_set_front_to_read());
			
			MAC(newmenu_show_cursor());
			MAC(key_flush());
			break;
		case KEY_HOME:
		case KEY_PAD7:
			citem = 0;
			break;
		case KEY_END:
		case KEY_PAD1:
			citem = nitems-1;
			break;
		case KEY_UP:
		case KEY_PAD8:
			citem--;			
			break;
		case KEY_DOWN:
		case KEY_PAD2:
			citem++;			
			break;
 		case KEY_PAGEDOWN:
		case KEY_PAD3:
			citem += LB_ITEMS_ON_SCREEN;
			break;
		case KEY_PAGEUP:
		case KEY_PAD9:
			citem -= LB_ITEMS_ON_SCREEN;
			break;
		case KEY_ESC:
			if (allow_abort_flag) {
				citem = -1;
				done = 1;
			}
			break;
		case KEY_ENTER:
		case KEY_PADENTER:
			done = 1;
			break;

		#ifdef MACINTOSH
		case KEY_COMMAND+KEY_Q: {
			extern void macintosh_quit();
			
			if ( !(Game_mode & GM_MULTI) )
				macintosh_quit();
			newmenu_show_cursor();
			key_flush();
			break;
		}
		#endif

		default:	
			if ( key > 0 )	{
				int ascii = key_to_ascii(key);
				if ( ascii < 255 )	{
					int cc,cc1;
					cc=cc1=citem+1;
					if (cc1 < 0 )  cc1 = 0;
					if (cc1 >= nitems )  cc1 = 0;
					while(1) {
						if ( cc < 0 ) cc = 0;
						if ( cc >= nitems ) cc = 0;
						if ( citem == cc ) break;
	
						if ( toupper( items[cc][0] ) == toupper(ascii) )	{
							citem = cc;
							break;
						}
						cc++;
					}
				}
			}
		}
		if ( done ) break;

		if (citem<0)
			citem=0;

		if (citem>=nitems)
			citem = nitems-1;

		if (citem< first_item)
			first_item = citem;

		if (citem>=( first_item+LB_ITEMS_ON_SCREEN))
			first_item = citem-LB_ITEMS_ON_SCREEN+1;

		if (nitems <= LB_ITEMS_ON_SCREEN )
			 first_item = 0;

		if (first_item>nitems-LB_ITEMS_ON_SCREEN)
			first_item = nitems-LB_ITEMS_ON_SCREEN;
		if (first_item < 0 ) first_item = 0;

#ifdef WINDOWS
		if (nitems>first_item+LB_ITEMS_ON_SCREEN)
			show_down_arrow=1;
		else 
			show_down_arrow=0;
		if (first_item>0)
			show_up_arrow=1;
		else	
			show_up_arrow=0;
#endif


#ifdef NEWMENU_MOUSE
		WIN(Sleep(100));
		if (mouse_state) {
			int w, h, aw;

			mouse_get_pos(&mx, &my);
			for (i=first_item; i<first_item+LB_ITEMS_ON_SCREEN; i++ )	{
				if (i > nitems)
					break;
				gr_get_string_size(items[i], &w, &h, &aw  );
				x1 = wx;
				x2 = wx + width;
				y1 = (i-first_item)*(grd_curfont->ft_h+2)+wy;
				y2 = y1+h+1;
				if ( ((mx > x1) && (mx < x2)) && ((my > y1) && (my < y2)) ) {
					//if (i == citem) {
					//	break;
					//}
					//dblclick_flag= 0;
					citem = i;
					done = 1;
					break;
				}
			}
		}

		//no double-click stuff for listbox
		//@@if (!mouse_state && omouse_state) {
		//@@	int w, h, aw;
		//@@
		//@@	gr_get_string_size(items[citem], &w, &h, &aw  );
		//@@	mouse_get_pos(&mx, &my);
		//@@	x1 = wx;
		//@@	x2 = wx + width;
		//@@	y1 = (citem-first_item)*(grd_curfont->ft_h+2)+wy;
		//@@	y2 = y1+h+1;
		//@@	if ( ((mx > x1) && (mx < x2)) && ((my > y1) && (my < y2)) ) {
		//@@		if (dblclick_flag) done = 1;
		//@@	}
		//@@}

		//check for close box clicked
		if ( !mouse_state && omouse_state ) {
			mouse_get_pos(&mx, &my);
			x1 = close_x + CLOSE_X + 2;
			x2 = x1 + CLOSE_SIZE - 2;
			y1 = close_y + CLOSE_Y + 2;
			y2 = y1 + CLOSE_SIZE - 2;
			if ( ((mx > x1) && (mx < x2)) && ((my > y1) && (my < y2)) ) {
				citem = -1;
				done = 1;
			}

		   #ifdef WINDOWS
			x1 = wx-LHX(10);
			x2 = x1 + LHX(10);
			y1 = wy;
			y2 = wy+LHY(7);
			if ( ((mx > x1) && (mx < x2)) && ((my > y1) && (my < y2)) && show_up_arrow) 
				simukey = -1;
			y1 = total_height-LHY(7);
			y2 = total_height;
			if ( ((mx > x1) && (mx < x2)) && ((my > y1) && (my < y2)) && show_down_arrow ) 
				simukey = 1;
		   #endif

		 	
		}
#endif

		if ( (ofirst_item != first_item) || redraw)	{
			newmenu_hide_cursor();
			WIN(DDGRLOCK(dd_grd_curcanv));

			gr_setcolor( BM_XRGB( 0,0,0)  );
			for (i=first_item; i<first_item+LB_ITEMS_ON_SCREEN; i++ )	{
				int w, h, aw, y;
				y = (i-first_item)*(grd_curfont->ft_h+2)+wy;
				if ( i >= nitems )	{
					gr_setcolor( BM_XRGB(0,0,0));
					gr_rect( wx, y-1, wx+width-1, y+grd_curfont->ft_h + 1 );
				} else {
					if ( i == citem )	
						grd_curcanv->cv_font = SELECTED_FONT;
					else	
						grd_curcanv->cv_font = NORMAL_FONT;
					gr_get_string_size(items[i], &w, &h, &aw  );
					gr_rect( wx, y-1, wx+width-1, y+h+1 );
					gr_string( wx+5, y, items[i]  );
				}
			}		

				
			// If Win95 port, draw up/down arrows on left side of menu
			#ifdef WINDOWS   
				grd_curcanv->cv_font = NORMAL_FONT;
			if (show_up_arrow)
	 	  		gr_string( wx-LHX(10), wy ,UP_ARROW_MARKER );
			else
			{
				No_darkening=1;
				nm_draw_background (wx-LHX(10),wy,wx-2,wy+LHY(7));
				No_darkening=0;
			}

			if (show_down_arrow)
	     		gr_string( wx-LHX(10), wy+total_height-LHY(7) ,DOWN_ARROW_MARKER );
			else
			{
				No_darkening=1;
				nm_draw_background (wx-LHX(10),wy+total_height-LHY(7),wx-2,wy+total_height);
				No_darkening=0;
			}

			#endif


			WIN(DDGRUNLOCK(dd_grd_curcanv));
			newmenu_show_cursor();
			gr_update();
		} else if ( citem != ocitem )	{
			int w, h, aw, y;

			newmenu_hide_cursor();

			WIN(DDGRLOCK(dd_grd_curcanv));

			i = ocitem;
			if ( (i>=0) && (i<nitems) )	{
				y = (i-first_item)*(grd_curfont->ft_h+2)+wy;
				if ( i == citem )	
					grd_curcanv->cv_font = SELECTED_FONT;
				else	
					grd_curcanv->cv_font = NORMAL_FONT;
				gr_get_string_size(items[i], &w, &h, &aw  );
				gr_rect( wx, y-1, wx+width-1, y+h+1 );
				gr_string( wx+5, y, items[i]  );

			}
			i = citem;
			if ( (i>=0) && (i<nitems) )	{
				y = (i-first_item)*(grd_curfont->ft_h+2)+wy;
				if ( i == citem )	
					grd_curcanv->cv_font = SELECTED_FONT;
				else	
					grd_curcanv->cv_font = NORMAL_FONT;
				gr_get_string_size( items[i], &w, &h, &aw  );
				gr_rect( wx, y-1, wx+width-1, y+h );
				gr_string( wx+5, y, items[i]  );
			}
			WIN(DDGRUNLOCK(dd_grd_curcanv));

			newmenu_show_cursor();
			gr_update();
		}
	}
	newmenu_hide_cursor();

	keyd_repeat = old_keyd_repeat;

	WIN (DDGRLOCK(dd_grd_curcanv));
	gr_bm_bitblt(total_width,total_height, wx-border_size, wy-title_height-border_size, 0, 0, bg.background, &grd_curcanv->cv_bitmap );
	WIN (DDGRUNLOCK(dd_grd_curcanv)); 	

	if ( bg.background != &VR_offscreen_buffer->cv_bitmap )
		gr_free_bitmap(bg.background);

#if 0
	WINDOS(
		dd_gr_blt_notrans(dd_VR_offscreen_buffer,
				0,0,_DDModeList[W95DisplayMode].rw, _DDModeList[W95DisplayMode].rh,
				dd_grd_curcanv,
				0,0,_DDModeList[W95DisplayMode].rw, _DDModeList[W95DisplayMode].rh),
		gr_bm_bitblt(grd_curcanv->cv_w, grd_curcanv->cv_h, 0, 0, 0, 0, &(VR_offscreen_buffer->cv_bitmap), &(grd_curcanv->cv_bitmap) )
	);
#endif

	WIN(DDGRRESTORE);

	WIN(mouse_set_mode(1));				//re-enable centering mode

	return citem;
}

int newmenu_filelist( char * title, char * filespec, char * filename )
{
	int i, NumFiles;
	char * Filenames[MAX_FILES];
	char FilenameText[MAX_FILES][14];
	FILEFINDSTRUCT find;

	NumFiles = 0;
	if( !FileFindFirst( filespec, &find ) )	{
		do	{
			if (NumFiles<MAX_FILES)	{
                                strncpy( FilenameText[NumFiles], find.name, FILENAME_LEN);
				Filenames[NumFiles] = FilenameText[NumFiles];
				NumFiles++;
			} else {
				break;
			}
		} while( !FileFindNext( &find ) );
		FileFindClose();
	}

	i = newmenu_listbox( title, NumFiles, Filenames, 1, NULL );
	if ( i > -1 )	{
		strcpy( filename, Filenames[i] );
		return 1;
	} 
	return 0;
}

//added on 10/14/98 by Victor Rachels to attempt a fixedwidth font messagebox
int nm_messagebox_fixedfont( char *title, int nchoices, ... )
{
	int i;
	char * format;
	va_list args;
	char *s;
	char nm_text[MESSAGEBOX_TEXT_SIZE];
	newmenu_item nm_message_items[5];

	va_start(args, nchoices );

	Assert( nchoices <= 5 );

	for (i=0; i<nchoices; i++ )	{
		s = va_arg( args, char * );
		nm_message_items[i].type = NM_TYPE_MENU; nm_message_items[i].text = s;
	}
	format = va_arg( args, char * );
	//sprintf(	  nm_text, "" ); // adb: ?
	vsprintf(nm_text,format,args);
	va_end(args);

	Assert(strlen(nm_text) < MESSAGEBOX_TEXT_SIZE );

        return newmenu_do_fixedfont( title, nm_text, nchoices, nm_message_items, NULL, 0, NULL, -1, -1 );
}
//end this section addition - Victor Rachels

#ifdef NETWORK
extern netgame_info Active_games[];
extern int NumActiveNetgames;

void show_extra_netgame_info(int choice)
 {
	newmenu_item m[5];
   char mtext[5][50];
	int i,num=0;

	if (choice>=NumActiveNetgames)
		return;
	
   for (i=0;i<5;i++)
	{
	 m[i].text=(char *)&mtext[i];
    m[i].type=NM_TYPE_TEXT;		
	}

   sprintf (mtext[num],"Game: %s",Active_games[choice].game_name); num++;
   sprintf (mtext[num],"Mission: %s",Active_games[choice].mission_title); num++;
	sprintf (mtext[num],"Current Level: %d",Active_games[choice].levelnum); num++;
	sprintf (mtext[num],"Difficulty: %s",MENU_DIFFICULTY_TEXT(Active_games[choice].difficulty)); num++;

	already_showing_info=1;	
	newmenu_dotiny2( NULL, "Netgame Information", num, m, NULL);
	already_showing_info=0;	
 }

#endif // NETWORK

/* Spiffy word wrap string formatting function */

void nm_wrap_text(char *dbuf, char *sbuf, int line_length)
{
	int col;
	char *wordptr;
	char *tbuf;

	tbuf = (char *)d_malloc(strlen(sbuf)+1);
	strcpy(tbuf, sbuf);

	wordptr = strtok(tbuf, " ");
	if (!wordptr) return;
	col = 0;
	dbuf[0] = 0;

	while (wordptr)
	{
		col = col+strlen(wordptr)+1;
		if (col >=line_length) {
			col = 0;
			sprintf(dbuf, "%s\n%s ", dbuf, wordptr);
		}
		else {
			sprintf(dbuf, "%s%s ", dbuf, wordptr);
		}
		wordptr = strtok(NULL, " ");
	}

	d_free(tbuf);
}
