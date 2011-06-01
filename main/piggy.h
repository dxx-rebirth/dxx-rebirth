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
 *
 * Interface to piggy functions.
 *
 */

#ifndef _PIGGY_H
#define _PIGGY_H

#include "physfsx.h"
#include "digi.h"
#include "sounds.h"

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

extern int MacPig;
extern int PCSharePig;

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

#endif // __GNUC__
// DPH (17/9/98): End mod.

void piggy_read_sounds(int pc_shareware);

/*
 * reads a bitmap_index structure from a PHYSFS_file
 */
void bitmap_index_read(bitmap_index *bi, PHYSFS_file *fp);

/*
 * reads n bitmap_index structs from a PHYSFS_file
 */
int bitmap_index_read_n(bitmap_index *bi, int n, PHYSFS_file *fp);

#endif // _PIGGY_H

