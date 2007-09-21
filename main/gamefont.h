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
 * $Source: /cvsroot/dxx-rebirth/d1x-rebirth/main/gamefont.h,v $
 * $Revision: 1.1.1.1 $
 * $Author: zicodxx $
 * $Date: 2006/03/17 19:43:53 $
 * 
 * Font declarations for the game,.
 * 
 * $Log: gamefont.h,v $
 * Revision 1.1.1.1  2006/03/17 19:43:53  zicodxx
 * initial import
 *
 * Revision 1.3  1999/11/20 10:05:17  donut
 * variable size menu patch from Jan Bobrowski.  Variable menu font size support and a bunch of fixes for menus that didn't work quite right, by me (MPM).
 *
 * Revision 1.2  1999/10/08 00:57:03  donut
 * variable GAME_FONT
 *
 * Revision 1.1.1.1  1999/06/14 22:12:22  donut
 * Import of d1x 1.37 source.
 *
 * Revision 2.0  1995/02/27  11:31:09  john
 * New version 2.0, which has no anonymous unions, builds with
 * Watcom 10.0, and doesn't require parsing BITMAPS.TBL.
 * 
 * Revision 1.7  1994/11/18  16:41:28  adam
 * trimmed meat
 * 
 * Revision 1.6  1994/11/17  13:07:00  adam
 * removed unused font
 * 
 * Revision 1.5  1994/11/03  21:36:02  john
 * Added code for credit fonts.
 * 
 * Revision 1.4  1994/08/17  20:20:25  matt
 * Took out alternate-color versions of font3, since this is a mono font
 * 
 * Revision 1.3  1994/08/11  12:44:32  adam
 * killed a #define
 * 
 * Revision 1.2  1994/08/10  19:57:16  john
 * Changed font stuff; Took out old menu; messed up lots of
 * other stuff like game sequencing messages, etc.
 * 
 * Revision 1.1  1994/08/10  17:20:22  john
 * Initial revision
 * 
 * 
 */



#ifndef _GAMEFONT_H
#define _GAMEFONT_H

// When adding a new font, don't forget to change the
// filename in gamefont.c!!!!!!!!!!!!!!!!!!!!!!!!!!!

#define GFONT_BIG_1 		0
#define GFONT_MEDIUM_1	1
#define GFONT_MEDIUM_2	2
#define GFONT_MEDIUM_3	3
#define GFONT_SMALL		4

#define GAME_FONT		(Gamefonts[GFONT_SMALL])
#define HELP_FONT		(Gamefonts[GFONT_MEDIUM_1])
#define MENU_FONT		(Gamefonts[GFONT_MEDIUM_1])
#define SCORES_FONT	(Gamefonts[GFONT_MEDIUM_1])

#define MAX_FONTS 5

extern grs_font *Gamefonts[MAX_FONTS];

void gamefont_init();
void gamefont_close();

char *gamefont_curfontname(int gf);

void gamefont_choose_game_font(int scrx,int scry);

#endif
 
