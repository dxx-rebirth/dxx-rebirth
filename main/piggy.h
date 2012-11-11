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
 * Interface to piggy functions.
 *
 */

#ifndef _PIGGY_H
#define _PIGGY_H

#include "physfsx.h"
#include "digi.h"
#include "sounds.h"
#include "inferno.h"

#define D1_PIGFILE              "descent.pig"

#define D1_SHARE_BIG_PIGSIZE    5092871 // v1.0 - 1.4 before RLE compression
#define D1_SHARE_10_PIGSIZE     2529454 // v1.0 - 1.2
#define D1_SHARE_PIGSIZE        2509799 // v1.4
#define D1_10_BIG_PIGSIZE       7640220 // v1.0 before RLE compression
#define D1_10_PIGSIZE           4520145 // v1.0
#define D1_PIGSIZE              4920305 // v1.4 - 1.5 (Incl. OEM v1.4a)
#define D1_OEM_PIGSIZE          5039735 // v1.0
#define D1_MAC_PIGSIZE          3975533
#define D1_MAC_SHARE_PIGSIZE    2714487

#define MAX_ALIASES 20

typedef struct alias {
	char alias_name[FILENAME_LEN];
	char file_name[FILENAME_LEN];
} alias;

extern alias alias_list[MAX_ALIASES];
extern int Num_aliases;

extern int Piggy_hamfile_version;

// an index into the bitmap collection of the piggy file
typedef struct bitmap_index {
	ushort index;
} __pack__ bitmap_index;

int properties_init();
void piggy_close();
void piggy_dump_all();
bitmap_index piggy_register_bitmap( grs_bitmap * bmp, char * name, int in_file );
int piggy_register_sound( digi_sound * snd, char * name, int in_file );
bitmap_index piggy_find_bitmap( char * name );
int piggy_find_sound( char * name );

extern int Pigfile_initialized;

void piggy_read_bitmap_data(grs_bitmap * bmp);
void piggy_read_sound_data(digi_sound *snd);

void piggy_load_level_data();

char* piggy_game_bitmap_name(grs_bitmap *bmp);

#define MAX_BITMAP_FILES    2620 // Upped for CD Enhanced
#define MAX_SOUND_FILES     MAX_SOUNDS

extern digi_sound GameSounds[MAX_SOUND_FILES];
extern grs_bitmap GameBitmaps[MAX_BITMAP_FILES];


extern void piggy_bitmap_page_in( bitmap_index bmp );
extern void piggy_bitmap_page_out_all();
extern int piggy_page_flushed;

/* Make GNUC use static inline function as #define with backslash continuations causes problems with dos linefeeds */
# ifdef __GNUC__
#  define  PIGGY_PAGE_IN(bmp) _piggy_page_in(bmp)
static inline void _piggy_page_in(bitmap_index bmp) {
    if ( GameBitmaps[(bmp).index].bm_flags & BM_FLAG_PAGED_OUT ) {
        piggy_bitmap_page_in( bmp );
    }
}

# else /* __GNUC__ */

	#define PIGGY_PAGE_IN(bmp)	\
do {					\
	if ( GameBitmaps[(bmp).index].bm_flags & BM_FLAG_PAGED_OUT )	{\
		piggy_bitmap_page_in( bmp ); \
	}				\
} while(0)
# endif /* __GNUC__ */

void piggy_read_sounds();

//reads in a new pigfile (for new palette)
//returns the size of all the bitmap data
void piggy_new_pigfile(char *pigname);

//loads custom bitmaps for current level
void load_bitmap_replacements(char *level_name);
//if descent.pig exists, loads descent 1 texture bitmaps
void load_d1_bitmap_replacements();

/*
 * reads a bitmap_index structure from a PHYSFS_file
 */
void bitmap_index_read(bitmap_index *bi, PHYSFS_file *fp);

/*
 * reads n bitmap_index structs from a PHYSFS_file
 */
int bitmap_index_read_n(bitmap_index *bi, int n, PHYSFS_file *fp);

/*
 * Find and load the named bitmap from descent.pig
 */
bitmap_index read_extra_bitmap_d1_pig(char *name);

extern void remove_char( char * s, char c );	// in piggy.c
#define REMOVE_EOL(s)		remove_char((s),'\n')
#define REMOVE_COMMENTS(s)	remove_char((s),';')
#define REMOVE_DOTS(s)  	remove_char((s),'.')

extern ubyte bogus_bitmap_initialized;
extern digi_sound bogus_sound;
extern const char space[3];
extern const char equal_space[4];

#endif //_PIGGY_H
