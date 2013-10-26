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
 * Header for titles.c
 *
 */

#ifndef _TITLES_H
#define _TITLES_H

#include "pstypes.h"

#ifdef __cplusplus
extern "C" {
#endif

extern void show_titles(void);
extern void do_briefing_screens(const char *filename, int level_num);
extern void do_end_briefing_screens(const char *filename);
extern char * get_briefing_screen( int level_num );
#if defined(DXX_BUILD_DESCENT_II)
extern void show_loading_screen(ubyte *title_pal);
extern void show_endgame_briefing(void);
extern int intro_played;
#endif
extern void show_order_form(void);

#ifdef __cplusplus
}
#endif

#endif
