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
 * $Source: /cvsroot/dxx-rebirth/d1x-rebirth/main/piggy.h,v $
 * $Revision: 1.1.1.1 $
 * $Author: zicodxx $
 * $Date: 2006/03/17 19:44:33 $
 * 
 * Interface to piggy functions.
 * 
 * $Log: piggy.h,v $
 * Revision 1.1.1.1  2006/03/17 19:44:33  zicodxx
 * initial import
 *
 * Revision 1.3  2000/04/18 01:17:58  sekmu
 * Changed/fixed altsounds (mostly done)
 *
 * Revision 1.2  1999/09/22 02:04:09  donut
 * include settings.h in piggy.h
 *
 * Revision 1.1.1.1  1999/06/14 22:12:58  donut
 * Import of d1x 1.37 source.
 *
 * Revision 2.0  1995/02/27  11:31:21  john
 * New version 2.0, which has no anonymous unions, builds with
 * Watcom 10.0, and doesn't require parsing BITMAPS.TBL.
 * 
 * Revision 1.10  1995/02/03  17:08:29  john
 * Changed sound stuff to allow low memory usage.
 * Also, changed so that Sounds isn't an array of digi_sounds, it
 * is a ubyte pointing into GameSounds, this way the digi.c code that
 * locks sounds won't accidentally unlock a sound that is already playing, but
 * since it's Sounds[soundno] is different, it would erroneously be unlocked.
 * 
 * Revision 1.9  1995/01/24  14:33:49  john
 * *** empty log message ***
 * 
 * Revision 1.8  1995/01/24  14:32:35  john
 * Took out paging in code.
 * 
 * Revision 1.7  1995/01/23  12:30:17  john
 * Made debug code that mprintf what bitmap gets paged in.
 * 
 * Revision 1.6  1995/01/17  14:11:37  john
 * Added function that is called after level loaded.
 * 
 * Revision 1.5  1995/01/14  19:16:58  john
 * First version of new bitmap paging code.
 * 
 * Revision 1.4  1994/10/27  18:51:57  john
 * Added -piglet option that only loads needed textures for a 
 * mine.  Only saved ~1MB, and code still doesn't free textures
 * before you load a new mine.
 * 
 * Revision 1.3  1994/06/08  14:20:47  john
 * Made piggy dump before going into game.
 * 
 * Revision 1.2  1994/05/06  13:02:40  john
 * Added piggy stuff; worked on supertransparency
 * 
 * Revision 1.1  1994/05/06  11:47:46  john
 * Initial revision
 * 
 * 
 */ 



#ifndef _PIGGY_H
#define _PIGGY_H

#include "digi.h"
#include "sounds.h"
#include "settings.h"

#define D1_SHARE_BIG_PIGSIZE    5092871 // v1.0 - 1.4 before RLE compression
#define D1_SHARE_10_PIGSIZE     2529454 // v1.0 - 1.2
#define D1_SHARE_PIGSIZE        2509799 // v1.4
#define D1_10_BIG_PIGSIZE       7640220 // v1.0 before RLE compression
#define D1_10_PIGSIZE           4520145 // v1.0
#define D1_PIGSIZE              4920305 // v1.4 - 1.5 (Incl. OEM v1.4a)
#define D1_OEM_PIGSIZE          5039735 // v1.0
#define D1_MAC_PIGSIZE          3975533
#define D1_MAC_SHARE_PIGSIZE    2714487

typedef struct bitmap_index
  {
    ushort index;
  }
bitmap_index;


extern ubyte bogus_data[64 * 64];

extern grs_bitmap bogus_bitmap;

extern ubyte bogus_bitmap_initialized;

extern digi_sound bogus_sound;


int properties_init ();

void piggy_close ();

void piggy_dump_all ();

bitmap_index piggy_register_bitmap (grs_bitmap * bmp, char *name, int in_file);

int piggy_register_sound (digi_sound * snd, char *name, int in_file);

bitmap_index piggy_find_bitmap (char *name);

int piggy_find_sound (char *name);

void piggy_read_bitmap_data (grs_bitmap * bmp);

void piggy_read_sound_data (digi_sound * snd);


void piggy_load_level_data ();


#ifdef SHAREWARE
#define MAX_BITMAP_FILES	1500
#define MAX_SOUND_FILES		MAX_SOUNDS
#else	/* 
 */
#define MAX_BITMAP_FILES	1800
#define MAX_SOUND_FILES		MAX_SOUNDS
#endif	/* 
 */

#define PIGGY_PC_SHAREWARE 2

//moved to sounds.h - extern digi_sound GameSounds[MAX_SOUND_FILES];

extern grs_bitmap GameBitmaps[MAX_BITMAP_FILES];


#ifdef PIGGY_USE_PAGING
extern void piggy_bitmap_page_in (bitmap_index bmp);
extern void piggy_bitmap_page_out_all ();

extern int piggy_page_flushed;

// DPH (17/9/98): Mod to use static inline function rather than #define under
// linux, as the #define has problems with linefeeds.
#ifdef __LINUX__
#define PIGGY_PAGE_IN(bmp) _piggy_page_in(bmp)
static inline void _piggy_page_in(bitmap_index bmp)
{
 do {
 	if ( GameBitmaps[(bmp).index].bm_flags & BM_FLAG_PAGED_OUT )	{
 		piggy_bitmap_page_in( bmp );
 	}
 } while(0);
}
#else // __GNUC__
#define PIGGY_PAGE_IN(bmp)                                                      \
do {                                                                            \
	if ( GameBitmaps[(bmp).index].bm_flags & BM_FLAG_PAGED_OUT )	{       \
		piggy_bitmap_page_in( bmp );                                    \
	}                                                                       \
} while(0)

//              mprintf(( 0, "Paging in '%s' from file '%s', line %d\n", #bmp, __FILE__,__LINE__ ));    
#endif // __GNUC__
// DPH (17/9/98): End mod.

#else
#define PIGGY_PAGE_IN(bmp)
#endif // PIGGY_USE_PAGING

#ifdef FAST_FILE_IO
#define bitmap_index_read(bi, fp) cfread(bi, sizeof(bitmap_index), 1, fp)
#define bitmap_index_read_n(bi, n, fp) cfread(bi, sizeof(bitmap_index), n, fp)
#else
/*
 * reads a bitmap_index structure from a CFILE
 */
void bitmap_index_read(bitmap_index *bi, CFILE *fp);

/*
 * reads n bitmap_index structs from a CFILE
 */
int bitmap_index_read_n(bitmap_index *bi, int n, CFILE *fp);
#endif // FAST_FILE_IO

#endif // _PIGGY_H

