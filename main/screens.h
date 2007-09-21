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
 * $Source: /cvsroot/dxx-rebirth/d1x-rebirth/main/screens.h,v $
 * $Revision: 1.1.1.1 $
 * $Author: zicodxx $
 * $Date: 2006/03/17 19:44:20 $
 * 
 * Info on canvases, screens, etc.
 * 
 * $Log: screens.h,v $
 * Revision 1.1.1.1  2006/03/17 19:44:20  zicodxx
 * initial import
 *
 * Revision 1.4  1999/11/21 13:00:08  donut
 * Changed screen_mode format.  Now directly encodes res into a 32bit int, rather than using arbitrary values.
 *
 * Revision 1.3  1999/11/20 10:05:18  donut
 * variable size menu patch from Jan Bobrowski.  Variable menu font size support and a bunch of fixes for menus that didn't work quite right, by me (MPM).
 *
 * Revision 1.2  1999/08/14 15:49:51  donut
 * moved MENU_SCREEN_MODE to main/screens.h so that it can be used for the startup screen mode in inferno.c
 *
 * Revision 1.1.1.1  1999/06/14 22:13:05  donut
 * Import of d1x 1.37 source.
 *
 * Revision 2.2  1995/03/14  12:14:00  john
 * Made VR helmets have 4 resolutions to choose from.
 * 
 * Revision 2.1  1995/03/06  15:24:09  john
 * New screen techniques.
 * 
 * Revision 2.0  1995/02/27  11:31:40  john
 * New version 2.0, which has no anonymous unions, builds with
 * Watcom 10.0, and doesn't require parsing BITMAPS.TBL.
 * 
 * Revision 1.15  1994/08/10  19:56:45  john
 * Changed font stuff; Took out old menu; messed up lots of
 * other stuff like game sequencing messages, etc.
 * 
 * Revision 1.14  1994/07/20  21:04:26  john
 * Add VictorMax VR helment support.
 * 
 * Revision 1.13  1994/06/24  17:01:28  john
 * Add VFX support; Took Game Sequencing, like EndGame and stuff and
 * took it out of game.c and into gameseq.c
 * 
 * Revision 1.12  1994/04/20  20:30:03  john
 * *** empty log message ***
 * 
 * Revision 1.11  1994/03/30  21:12:05  yuan
 * Use only 119 lines (saves 3 scanlines)
 * 
 * Revision 1.10  1994/03/17  16:49:37  john
 * *** empty log message ***
 * 
 * Revision 1.9  1994/02/11  15:07:44  matt
 * Added extern of Canv_game_offscrn
 * 
 * Revision 1.8  1994/01/31  16:52:43  john
 * redid cockpit bounds.
 * 
 * Revision 1.7  1994/01/26  18:13:53  john
 * Changed 3d constants..
 * 
 * Revision 1.6  1994/01/25  17:11:46  john
 * *** empty log message ***
 * 
 * Revision 1.5  1994/01/25  11:43:25  john
 * Changed game window size.
 * 
 * Revision 1.4  1993/12/13  16:32:39  yuan
 * Fixed menu system memory errors, and other bugs.
 * 
 * Revision 1.3  1993/12/10  16:07:23  yuan
 * Working on menu system.  Updated the title screen.
 * 
 * Revision 1.2  1993/12/09  21:27:46  matt
 * Added 3d window sizing constants
 * 
 * Revision 1.1  1993/12/06  09:50:33  matt
 * Initial revision
 * 
 * 
 * 
 */

#ifndef _SCREEN_H
#define _SCREEN_H

#include "gr.h"

//What graphics modes the game & editor open

//for Screen_mode variable
#define SCREEN_MENU				0	//viewing the menu screen
#define SCREEN_GAME				1	//viewing the menu screen
#define SCREEN_EDITOR			2	//viewing the editor screen

extern grs_canvas       Screen_3d_window;                               // The rectangle for rendering the mine to

//from editor.c
extern grs_canvas *Canv_editor;			//the full on-scrren editor canvas
extern grs_canvas *Canv_editor_game;	//the game window on the editor screen

//from game.c
extern int set_screen_mode(int sm);		// True = editor screen

extern u_int32_t menu_screen_mode;
extern int menu_use_game_res;
#define MENU_SCREEN_MODE (menu_use_game_res?Game_screen_mode:menu_screen_mode)

#endif
 
