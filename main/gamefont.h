/* $Id: gamefont.h,v 1.3 2003-10-10 09:36:35 btb Exp $ */
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
 * Font declarations for the game,.
 *
 * Old Log:
 * Revision 1.2  1995/08/18  10:23:54  allender
 * removed large font -- added PC small font
 *
 * Revision 1.1  1995/05/16  15:56:55  allender
 * Initial revision
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

#include "gr.h"

// When adding a new font, don't forget to change the filename in
// gamefont.c!!!!!!!!!!!!!!!!!!!!!!!!!!!

// We are interleaving low & high resolution fonts, so to access a
// font you say fontnum+flag where flag is 0 for lowres, 1 for hires

#define GFONT_BIG_1     0
#define GFONT_MEDIUM_1  2
#define GFONT_MEDIUM_2  4
#define GFONT_MEDIUM_3  6
#define GFONT_SMALL     8

#define SMALL_FONT      (Gamefonts[GFONT_SMALL + FontHires])
#define MEDIUM1_FONT    (Gamefonts[GFONT_MEDIUM_1 + FontHires])
#define MEDIUM2_FONT    (Gamefonts[GFONT_MEDIUM_2 + FontHires])
#define MEDIUM3_FONT    (Gamefonts[GFONT_MEDIUM_3 + FontHires])
#define HUGE_FONT       (Gamefonts[GFONT_BIG_1 + FontHires])

#define GAME_FONT       SMALL_FONT

#define MAX_FONTS 10

extern grs_font *Gamefonts[MAX_FONTS];

void gamefont_init();
void gamefont_close();

extern int FontHires;
extern int FontHiresAvailable;

#endif /* _GAMEFONT_H */
