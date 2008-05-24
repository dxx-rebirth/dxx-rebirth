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
 * $Source: /cvsroot/dxx-rebirth/d1x-rebirth/main/songs.h,v $
 * $Revision: 1.1.1.1 $
 * $Author: zicodxx $
 * $Date: 2006/03/17 19:42:37 $
 * 
 * .
 * 
 * $Log: songs.h,v $
 * Revision 1.1.1.1  2006/03/17 19:42:37  zicodxx
 * initial import
 *
 * Revision 1.1.1.1  1999/06/14 22:13:09  donut
 * Import of d1x 1.37 source.
 *
 * Revision 2.0  1995/02/27  11:30:52  john
 * New version 2.0, which has no anonymous unions, builds with
 * Watcom 10.0, and doesn't require parsing BITMAPS.TBL.
 * 
 * Revision 1.6  1995/02/11  22:21:44  adam
 * *** empty log message ***
 * 
 * Revision 1.5  1995/02/11  19:10:49  adam
 * *** empty log message ***
 * 
 * Revision 1.4  1995/02/11  18:34:40  adam
 * *** empty log message ***
 * 
 * Revision 1.3  1995/02/11  18:04:51  adam
 * upped songs
 * 
 * Revision 1.2  1995/02/11  12:42:12  john
 * Added new song method, with FM bank switching..
 * 
 * Revision 1.1  1995/02/11  10:20:18  john
 * Initial revision
 * 
 * 
 */

#ifndef _SONGS_H
#define _SONGS_H

typedef struct song_info {
	char	filename[16];
	char	melodic_bank_file[16];
	char	drum_bank_file[16];
} song_info;

#define MAX_SONGS 27

extern song_info Songs[];

#define SONG_TITLE			0
#define SONG_BRIEFING		1
#define SONG_ENDLEVEL		2
#define SONG_ENDGAME			3
#define SONG_CREDITS			4
#define SONG_LEVEL_MUSIC	5
#ifdef SHAREWARE
#define NUM_GAME_SONGS		5
#else
#define NUM_GAME_SONGS          22
#endif

void songs_play_song( int songnum, int repeat );
void songs_play_level_song( int levelnum );

// stop the redbook, so we can read off the CD
void songs_stop_redbook(void);

// stop any songs - midi or redbook - that are currently playing
void songs_stop_all(void);

// this should be called regularly to check for redbook restart
void songs_check_redbook_repeat(void);

#endif
 
