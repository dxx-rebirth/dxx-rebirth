/* $Id: titles.h,v 1.5 2004-10-24 12:46:49 schaffner Exp $ */
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

#ifndef RELEASE
extern int	Skip_briefing_screens;
#else
#define Skip_briefing_screens 0
#endif

extern int show_title_screen( char * filename, int allow_keys, int from_hog_only );
extern int show_briefing_screen( char * filename, int allow_keys );
extern void show_title_flick(char *name, int allow_keys );
extern void do_briefing_screens(char *filename,int level_num);
extern char * get_briefing_screen( int level_num );

extern void show_endgame_briefing(void);

#endif
