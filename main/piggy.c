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
 * $Source: /cvsroot/dxx-rebirth/d1x-rebirth/main/piggy.c,v $
 * $Revision: 1.2 $
 * $Author: michaelstather $
 * $Date: 2006/03/18 23:08:13 $
 * 
 * Functions for managing the pig files.
 * 
 * $Log: piggy.c,v $
 * Revision 1.2  2006/03/18 23:08:13  michaelstather
 * New build system by KyroMaster
 *
 * Revision 1.1.1.1  2006/03/17 19:44:33  zicodxx
 * initial import
 *
 * Revision 1.9  2000/04/18 01:17:58  sekmu
 * Changed/fixed altsounds (mostly done)
 *
 * Revision 1.8  2000/02/07 07:27:04  donut
 * killed ifndef linux around some free(), it doesn't seem to crash anymore
 *
 * Revision 1.7  2000/01/27 07:40:05  sekmu
 * removed error
 *
 * Revision 1.6  2000/01/23 02:59:29  sekmu
 * mostly completed alternate sound loading
 *
 * Revision 1.5  1999/12/17 02:22:18  donut
 * Tim Riker's shareware sound fix
 *
 * Revision 1.4  1999/11/20 10:05:18  donut
 * variable size menu patch from Jan Bobrowski.  Variable menu font size support and a bunch of fixes for menus that didn't work quite right, by me (MPM).
 *
 * Revision 1.3  1999/11/15 10:44:42  sekmu
 * added calls to altsound.c for loading new sounds
 *
 * Revision 1.2  1999/10/07 22:40:29  donut
 * shareware fixes
 *
 * Revision 1.1.1.1  1999/06/14 22:10:55  donut
 * Import of d1x 1.37 source.
 *
 * Revision 2.10  1995/10/07  13:17:26  john
 * Made all bitmaps paged out by default.
 * 
 * Revision 2.9  1995/04/14  14:05:24  john
 * *** empty log message ***
 * 
 * Revision 2.8  1995/04/12  13:39:37  john
 * Fixed bug with -lowmem not working.
 * 
 * Revision 2.7  1995/03/29  23:23:17  john
 * Fixed major bug with sounds not building into pig right.
 * 
 * Revision 2.6  1995/03/28  18:05:00  john
 * Fixed it so you don't have to delete pig after changing bitmaps.tbl
 * 
 * Revision 2.5  1995/03/16  23:13:06  john
 * Fixed bug with piggy paging in bitmap not checking for disk
 * error, hence bogifying textures if you pull the CD out.
 * 
 * Revision 2.4  1995/03/14  16:22:27  john
 * Added cdrom alternate directory stuff.
 * 
 * Revision 2.3  1995/03/06  15:23:20  john
 * New screen techniques.
 * 
 * Revision 2.2  1995/02/27  13:13:40  john
 * Removed floating point.
 * 
 * Revision 2.1  1995/02/27  12:31:25  john
 * Made work without editor.
 * 
 * Revision 2.0  1995/02/27  11:28:02  john
 * New version 2.0, which has no anonymous unions, builds with
 * Watcom 10.0, and doesn't require parsing BITMAPS.TBL.
 * 
 * Revision 1.85  1995/02/09  12:54:24  john
 * Made paged out bitmaps have bm_data be a valid pointer
 * instead of NULL, in case anyone accesses it.
 * 
 * Revision 1.84  1995/02/09  12:50:59  john
 * Bullet-proofed the piggy loading code.
 * 
 * Revision 1.83  1995/02/07  17:08:51  john
 * Added some error handling stuff instead of asserts.
 * 
 * Revision 1.82  1995/02/03  17:06:48  john
 * Changed sound stuff to allow low memory usage.
 * Also, changed so that Sounds isn't an array of digi_sounds, it
 * is a ubyte pointing into GameSounds, this way the digi.c code that
 * locks sounds won't accidentally unlock a sound that is already playing, but
 * since it's Sounds[soundno] is different, it would erroneously be unlocked.
 * 
 * Revision 1.81  1995/02/02  21:56:39  matt
 * Added data for new gauge bitmaps
 * 
 * Revision 1.80  1995/02/01  23:31:57  john
 * Took out loading bar.
 * 
 * Revision 1.79  1995/01/28  15:13:18  allender
 * bumped up Piggy_bitmap_cache_size
 * 
 * Revision 1.78  1995/01/26  12:30:43  john
 * Took out prev.
 * 
 * Revision 1.77  1995/01/26  12:12:17  john
 * Made buffer be big for bitmaps.
 * 
 * Revision 1.76  1995/01/25  20:15:38  john
 * Made editor allocate all mem.
 * 
 * Revision 1.75  1995/01/25  14:52:56  john
 * Made bitmap buffer be 1.5 MB.
 * 
 * Revision 1.74  1995/01/22  16:03:19  mike
 * localization.
 * 
 * Revision 1.73  1995/01/22  15:58:36  mike
 * localization
 * 
 * Revision 1.72  1995/01/18  20:51:20  john
 * Took out warnings.
 * 
 * Revision 1.71  1995/01/18  20:47:21  john
 * Added code to allocate sounds & bitmaps into diff
 * buffers, also made sounds not be compressed for registered.
 * 
 * Revision 1.70  1995/01/18  15:08:41  john
 * Added start/stop time around paging.
 * Made paging clear screen around globe.
 * 
 * Revision 1.69  1995/01/18  10:07:51  john
 * 
 * Took out debugging mprintfs.
 * 
 * Revision 1.68  1995/01/17  14:27:42  john
 * y
 * 
 * Revision 1.67  1995/01/17  12:14:39  john
 * Made walls, object explosion vclips load at level start.
 * 
 * Revision 1.66  1995/01/15  13:15:44  john
 * Made so that paging always happens, lowmem just loads less.
 * Also, make KB load print to hud.
 * 
 * Revision 1.65  1995/01/15  11:56:28  john
 * Working version of paging.
 * 
 * Revision 1.64  1995/01/14  19:17:07  john
 * First version of new bitmap paging code.
 * 
 * Revision 1.63  1994/12/15  12:26:44  john
 * Added -nolowmem function.
 * 
 * Revision 1.62  1994/12/14  21:12:26  john
 * Fixed bug with page fault when exiting and using
 * -nosound.
 * 
 * Revision 1.61  1994/12/14  11:35:31  john
 * Evened out thermometer for pig read.
 * 
 * Revision 1.60  1994/12/14  10:51:00  john
 * Sped up sound loading.
 * 
 * Revision 1.59  1994/12/14  10:12:08  john
 * Sped up pig loading.
 * 
 * Revision 1.58  1994/12/13  09:14:47  john
 * *** empty log message ***
 * 
 * Revision 1.57  1994/12/13  09:12:57  john
 * Made the bar always fill up.
 * 
 * Revision 1.56  1994/12/13  03:49:08  john
 * Made -lowmem not load the unnecessary bitmaps.
 * 
 * Revision 1.55  1994/12/06  16:06:35  john
 * Took out piggy sorting.
 * 
 * Revision 1.54  1994/12/06  15:11:14  john
 * Fixed bug with reading pigs.
 * 
 * Revision 1.53  1994/12/06  14:14:47  john
 * Added code to set low mem based on memory.
 * 
 * Revision 1.52  1994/12/06  14:01:10  john
 * Fixed bug that was causing -lowmem all the time..
 * 
 * Revision 1.51  1994/12/06  13:33:48  john
 * Added lowmem option.
 * 
 * Revision 1.50  1994/12/05  19:40:10  john
 * If -nosound or no sound card selected, don't load sounds from pig.
 * 
 * Revision 1.49  1994/12/05  12:17:44  john
 * Added code that locks/unlocks digital sounds on demand.
 * 
 * Revision 1.48  1994/12/05  11:39:03  matt
 * Fixed little mistake
 * 
 * Revision 1.47  1994/12/05  09:29:22  john
 * Added clength to the sound field.
 * 
 * Revision 1.46  1994/12/04  15:27:15  john
 * Fixed my stupid bug that looked at -nosound instead of digi_driver_card
 * to see whether or not to lock down sound memory.
 * 
 * Revision 1.45  1994/12/03  14:17:00  john
 * Took out my debug mprintf.
 * 
 * Revision 1.44  1994/12/03  13:32:37  john
 * Fixed bug with offscreen bitmap.
 * 
 * Revision 1.43  1994/12/03  13:07:13  john
 * Made the pig read/write compressed sounds.
 * 
 * Revision 1.42  1994/12/03  11:48:51  matt
 * Added option to not dump sounds to pigfile
 * 
 * Revision 1.41  1994/12/02  20:02:20  matt
 * Made sound files constant match constant for table
 * 
 * Revision 1.40  1994/11/29  11:03:09  adam
 * upped # of sounds
 * 
 * Revision 1.39  1994/11/27  23:13:51  matt
 * Made changes for new mprintf calling convention
 * 
 * Revision 1.38  1994/11/20  18:40:34  john
 * MAde the piggy.lst and piggy.all not dump for release.
 * 
 * Revision 1.37  1994/11/19  23:54:45  mike
 * up number of bitmaps for shareware version.
 * 
 * Revision 1.36  1994/11/19  19:53:05  mike
 * change MAX_BITMAP_FILES
 * 
 * Revision 1.35  1994/11/19  10:42:56  matt
 * Increased number of bitmaps for non-shareware version
 * 
 * Revision 1.34  1994/11/19  09:11:52  john
 * Added avg_color to bitmaps saved in pig.
 * 
 * Revision 1.33  1994/11/19  00:07:05  john
 * Fixed bug with 8 char sound filenames not getting read from pig.
 * 
 * Revision 1.32  1994/11/18  22:24:54  john
 * Added -bigpig command line that doesn't rle your pig.
 * 
 * Revision 1.31  1994/11/18  21:56:53  john
 * Added a better, leaner pig format.
 * 
 * Revision 1.30  1994/11/16  12:06:16  john
 * Fixed bug with calling .bbms abms.
 * 
 * Revision 1.29  1994/11/16  12:00:56  john
 * Added piggy.all dump.
 * 
 * Revision 1.28  1994/11/10  21:16:02  adam
 * nothing important
 * 
 * Revision 1.27  1994/11/10  13:42:00  john
 * Made sounds not lock down if using -nosound.
 * 
 * Revision 1.26  1994/11/09  19:55:40  john
 * Added full rle support with texture rle caching.
 * 
 * Revision 1.25  1994/11/09  16:36:42  john
 * First version with RLE bitmaps in Pig.
 * 
 * Revision 1.24  1994/10/27  19:42:59  john
 * Disable the piglet option.
 * 
 * Revision 1.23  1994/10/27  18:51:40  john
 * Added -piglet option that only loads needed textures for a 
 * mine.  Only saved ~1MB, and code still doesn't free textures
 * before you load a new mine.
 * 
 * Revision 1.22  1994/10/25  13:11:42  john
 * Made the sounds sort. Dumped piggy.lst.
 * 
 * Revision 1.21  1994/10/06  17:06:23  john
 * Took out rle stuff.
 * 
 * Revision 1.20  1994/10/06  15:45:36  adam
 * bumped MAX_BITMAP_FILES again!
 * 
 * Revision 1.19  1994/10/06  11:01:17  yuan
 * Upped MAX_BITMAP_FILES
 * 
 * Revision 1.18  1994/10/06  10:44:45  john
 * Added diagnostic message and psuedo run-length-encoder
 * to see how much memory we would save by storing bitmaps
 * in a RLE method.  Also, I commented out the code that
 * stores 4K bitmaps on a 4K boundry to reduce pig size 
 * a bit.
 * 
 * Revision 1.17  1994/10/04  20:03:13  matt
 * Upped maximum number of bitmaps
 * 
 * Revision 1.16  1994/10/03  18:04:20  john
 * Fixed bug with data_offset not set right for bitmaps
 * that are 64x64 and not aligned on a 4k boundry.
 * 
 * Revision 1.15  1994/09/28  11:30:55  john
 * changed inferno.pig to descent.pig, changed the way it
 * is read.
 * 
 * Revision 1.14  1994/09/22  16:14:17  john
 * Redid intro sequecing.
 * 
 * Revision 1.13  1994/09/19  14:42:47  john
 * Locked down sounds with Virtual memory.
 * 
 * Revision 1.12  1994/09/10  17:31:52  mike
 * Increase number of loadable bitmaps.
 * 
 * Revision 1.11  1994/09/01  19:32:49  mike
 * Boost texture map allocation.
 * 
 * Revision 1.10  1994/08/16  11:51:02  john
 * Added grwased pigs.
 * 
 * Revision 1.9  1994/07/06  09:18:03  adam
 * upped bitmap #s
 * 
 * Revision 1.8  1994/06/20  22:02:15  matt
 * Fixed bug from last change
 * 
 * Revision 1.7  1994/06/20  21:33:18  matt
 * Made bm.h not include sounds.h, to reduce dependencies
 * 
 * Revision 1.6  1994/06/20  16:52:19  john
 * cleaned up init output a bit.
 * 
 * Revision 1.5  1994/06/08  14:20:57  john
 * Made piggy dump before going into game.
 * 
 * Revision 1.4  1994/06/02  18:59:22  matt
 * Clear selector field of bitmap loaded from pig file
 * 
 * Revision 1.3  1994/05/06  15:31:41  john
 * Made name field a bit longer.
 * 
 * Revision 1.2  1994/05/06  13:02:44  john
 * Added piggy stuff; worked on supertransparency
 * 
 * Revision 1.1  1994/05/06  11:47:26  john
 * Initial revision
 * 
 * 
 */


#ifdef RCS
static char rcsid[] = "$Id: piggy.c,v 1.2 2006/03/18 23:08:13 michaelstather Exp $";
#endif

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>

#include "types.h"
#include "inferno.h"
#include "gr.h"
#include "u_mem.h"
#include "mono.h"
#include "error.h"
#include "sounds.h"
#include "bm.h"
#include "hash.h"
#include "args.h"
#include "palette.h"
#include "gamefont.h"
#include "rle.h"
#include "screens.h"
#ifdef BITMAP_SELECTOR
#include "u_dpmi.h"
#endif

#ifdef SHAREWARE
#include "snddecom.h"
#endif

#include "piggy.h"
#include "texmerge.h"
#include "paging.h"
#include "game.h"
#include "text.h"
#include "cfile.h"
#include "newmenu.h"
#include "custom.h"

int piggy_is_substitutable_bitmap( char * name, char * subst_name );

//#define NO_DUMP_SOUNDS	1		//if set, dump bitmaps but not sounds

ubyte *BitmapBits = NULL;
ubyte *SoundBits = NULL;

typedef struct BitmapFile	{
	char			name[15];
} BitmapFile;

typedef struct SoundFile	{
	char			name[15];
} SoundFile;

hashtable AllBitmapsNames;
hashtable AllDigiSndNames;

int Num_bitmap_files = 0;
int Num_sound_files = 0;

digi_sound GameSounds[MAX_SOUND_FILES];
int SoundOffset[MAX_SOUND_FILES];
grs_bitmap GameBitmaps[MAX_BITMAP_FILES];

int Num_bitmap_files_new = 0;
int Num_sound_files_new = 0;
static BitmapFile AllBitmaps[ MAX_BITMAP_FILES ];
static SoundFile AllSounds[ MAX_SOUND_FILES ];

#define DBM_FLAG_LARGE 	128		// Flags added onto the flags struct in b
#define DBM_FLAG_ABM		64

int Piggy_bitmap_cache_size = 0;
int Piggy_bitmap_cache_next = 0;
ubyte * Piggy_bitmap_cache_data = NULL;
/*static*/ int GameBitmapOffset[MAX_BITMAP_FILES];
/*static*/ ubyte GameBitmapFlags[MAX_BITMAP_FILES];
ushort GameBitmapXlat[MAX_BITMAP_FILES];

#define PIGGY_BUFFER_SIZE (2048*1024)

int piggy_page_flushed = 0;

typedef struct DiskBitmapHeader {
	char name[8];
	ubyte dflags;
	ubyte	width;	
	ubyte height;
	ubyte flags;
	ubyte avg_color;
	int offset;
} __pack__ DiskBitmapHeader;

typedef struct DiskSoundHeader {
	char name[8];
	int length;
	int data_length;
	int offset;
} __pack__ DiskSoundHeader;

ubyte BigPig = 0;

#ifdef SHAREWARE
static int SoundCompressed[ MAX_SOUND_FILES ];
#endif

//#define BUILD_PSX_DATA 1

#ifdef BUILD_PSX_DATA
FILE * count_file = NULL;

int num_good=0,num_bad=0;
int num_good64=0,num_bad64=0;

void close_count_file()
{
 	if ( count_file )	{
		fprintf( count_file,"Good = %d\n", num_good );
		fprintf( count_file,"Bad = %d\n", num_bad );
		fprintf( count_file,"64Good = %d\n", num_good64 );
		fprintf( count_file,"64Bad = %d\n", num_bad64 );
		fclose(count_file);
		count_file=NULL;
	}
}

void count_colors( int bnum, grs_bitmap * bmp )
{
	int i,j,colors;
	ushort n[256];

	quantize_colors( bnum, bmp );

	if ( count_file == NULL )	{
		atexit( close_count_file );
		count_file = fopen( "bitmap.cnt", "wt" );
	}
	for (i=0; i<256; i++ )
		n[i] = 0;

	
	for (j = 0; j < bmp->bm_h; j++)
		for (i = 0; i < bmp->bm_w; i++)
			n [gr_gpixel (bmp, i, j)] ++;

	colors = 0;
	for (i=0; i<256; i++ )
		if (n[i]) colors++;

	if ( colors > 16 )	{
		if ( (bmp->bm_w==64) && (bmp->bm_h==64) ) num_bad64++;
		num_bad++;
		fprintf( count_file, "Bmp has %d colors (%d x %d)\n", colors, bmp->bm_w, bmp->bm_h );
	} else {
		if ( (bmp->bm_w==64) && (bmp->bm_h==64) ) num_good64++;
		num_good++;
	}
}
#endif

void piggy_get_bitmap_name( int i, char * name )
{
	strncpy( name, AllBitmaps[i].name, 12 );
	name[12] = 0;
}

bitmap_index piggy_register_bitmap( grs_bitmap * bmp, char * name, int in_file )
{
	bitmap_index temp;
	Assert( Num_bitmap_files < MAX_BITMAP_FILES );
#ifdef D1XD3D
	Assert (bmp->iMagic == BM_MAGIC_NUMBER);
#endif

	temp.index = Num_bitmap_files;


	if (!in_file)	{
		if ( !BigPig )	gr_bitmap_rle_compress( bmp );
		Num_bitmap_files_new++;
	}

	strncpy( AllBitmaps[Num_bitmap_files].name, name, 12 );
	hashtable_insert( &AllBitmapsNames, AllBitmaps[Num_bitmap_files].name, Num_bitmap_files );
	GameBitmaps[Num_bitmap_files] = *bmp;
	if ( !in_file )	{
		GameBitmapOffset[Num_bitmap_files] = 0;
		GameBitmapFlags[Num_bitmap_files] = bmp->bm_flags;
	}
	Num_bitmap_files++;

	return temp;
}

int piggy_register_sound( digi_sound * snd, char * name, int in_file )
{
	int i;

	Assert( Num_sound_files < MAX_SOUND_FILES );

	strncpy( AllSounds[Num_sound_files].name, name, 12 );
	hashtable_insert( &AllDigiSndNames, AllSounds[Num_sound_files].name, Num_sound_files );
	GameSounds[Num_sound_files] = *snd;

//added/moved on 11/13/99 by Victor Rachels to ready for changing freq
//#ifdef ALLEGRO
        GameSounds[Num_sound_files].bits = snd->bits;
        GameSounds[Num_sound_files].freq = snd->freq;

#ifdef ALLEGRO
//end this section move - VR
	GameSounds[Num_sound_files].priority = 128;
	GameSounds[Num_sound_files].loop_start = 0;
	GameSounds[Num_sound_files].loop_end = GameSounds[Num_sound_files].len;
	GameSounds[Num_sound_files].param = -1;
#endif
	if ( !in_file )	{
		SoundOffset[Num_sound_files] = 0;	
	}

	i = Num_sound_files;
   
	if (!in_file)
		Num_sound_files_new++;

	Num_sound_files++;
	return i;
}

bitmap_index piggy_find_bitmap( char * name )	
{
	bitmap_index bmp;
	int i;

	bmp.index = 0;

	i = hashtable_search( &AllBitmapsNames, name );
	Assert( i != 0 );
	if ( i < 0 )
		return bmp;

	bmp.index = i;
	return bmp;
}

int piggy_find_sound( char * name )	
{
	int i;

	i = hashtable_search( &AllDigiSndNames, name );

	if ( i < 0 )
		return 255;

	return i;
}

CFILE * Piggy_fp = NULL;

void piggy_close_file()
{
	if ( Piggy_fp )	{
		cfclose( Piggy_fp );
		Piggy_fp	= NULL;
	}
}

ubyte bogus_data[64*64];
grs_bitmap bogus_bitmap;
ubyte bogus_bitmap_initialized=0;
digi_sound bogus_sound;

extern void bm_read_all(CFILE * fp);
#ifdef EDITOR
extern void bm_write_all(FILE * fp);
#endif

int piggy_init()
{
	int sbytes = 0;
	char temp_name_read[16];
	char temp_name[16];
	grs_bitmap temp_bitmap;
	digi_sound temp_sound;
	DiskBitmapHeader bmh;
	DiskSoundHeader sndh;
	int header_size, N_bitmaps, N_sounds;
	int i,size, length;
	char * filename;
	int read_sounds = 1;
	int Pigdata_start;

	hashtable_init( &AllBitmapsNames, MAX_BITMAP_FILES );
	hashtable_init( &AllDigiSndNames, MAX_SOUND_FILES );

	atexit(piggy_close);

	if ( FindArg( "-nosound" )
#ifdef __MSDOS__
        || (digi_driver_board<1)
#endif
        )		{
		read_sounds = 0;
		mprintf(( 0, "Not loading sound data!!!!!\n" ));
	}
	
	for (i=0; i<MAX_SOUND_FILES; i++ )	{
#ifdef ALLEGRO
		GameSounds[i].len = 0;
#else
		GameSounds[i].length = 0;
#endif
		GameSounds[i].data = NULL;
		SoundOffset[i] = 0;

//added on 11/13/99 by Victor Rachels to ready for changing freq
                GameSounds[i].bits = 0;
                GameSounds[i].freq = 0;
//end this section addition - VR
	}

	for (i=0; i<MAX_BITMAP_FILES; i++ )		{
		GameBitmapXlat[i] = i;
		GameBitmaps[i].bm_flags = BM_FLAG_PAGED_OUT;
	}

	if ( !bogus_bitmap_initialized )	{
		int i;
		ubyte c;
		bogus_bitmap_initialized = 1;
		c = gr_find_closest_color( 0, 0, 63 );
		for (i=0; i<4096; i++ ) bogus_data[i] = c;
		c = gr_find_closest_color( 63, 0, 0 );
		// Make a big red X !
		for (i=0; i<64; i++ )	{
			bogus_data[i*64+i] = c;
			bogus_data[i*64+(63-i)] = c;
		}
		gr_init_bitmap (&bogus_bitmap, 0, 0, 0, 64, 64, 64, bogus_data);
		piggy_register_bitmap( &bogus_bitmap, "bogus", 1 );
#ifdef ALLEGRO
		bogus_sound.len = 64*64;
#else
        bogus_sound.length = 64*64;
#endif
		bogus_sound.data = bogus_data;
//added on 11/13/99 by Victor Rachels to ready for changing freq
                bogus_sound.freq = 11025;
                bogus_sound.bits = 8;
//end this section addition - VR
		GameBitmapOffset[0] = 0;
	}

        filename = DESCENT_DATA_PATH "descent.pig";
	
	if ( FindArg( "-bigpig" ))
		BigPig = 1;

	if (GameArg.SysLowMem)
		digi_lomem = 1;

	if ( (i=FindArg( "-piggy" )) )	{
		filename	= Args[i+1];
		mprintf( (0, "Using alternate pigfile, '%s'\n", filename ));
	}
	Piggy_fp = cfopen( filename, "rb" );
	if (Piggy_fp==NULL) return 0;

#ifdef SHAREWARE
	Pigdata_start = 0;
#else
	cfread( &Pigdata_start, sizeof(int), 1, Piggy_fp );
#ifdef EDITOR
	if ( FindArg("-nobm") )
#endif
	{
		bm_read_all( Piggy_fp );	// Note connection to above if!!!
		cfread( GameBitmapXlat, sizeof(ushort)*MAX_BITMAP_FILES, 1, Piggy_fp );
	}
#endif

	cfseek( Piggy_fp, Pigdata_start, SEEK_SET );
	size = cfilelength(Piggy_fp) - Pigdata_start;
	length = size;
	mprintf( (0, "\nReading data (%d KB) ", size/1024 ));

	cfread( &N_bitmaps, sizeof(int), 1, Piggy_fp );
	size -= sizeof(int);
	cfread( &N_sounds, sizeof(int), 1, Piggy_fp );
	size -= sizeof(int);

	header_size = (N_bitmaps*sizeof(DiskBitmapHeader)) + (N_sounds*sizeof(DiskSoundHeader));

	gr_set_curfont( Gamefonts[GFONT_SMALL] );
	gr_set_fontcolor(gr_find_closest_color_current( 20, 20, 20 ),-1 );
	gr_printf( 0x8000, SHEIGHT - 22, "%s...", TXT_LOADING_DATA );

	for (i=0; i<N_bitmaps; i++ )	{
		cfread( &bmh, sizeof(DiskBitmapHeader), 1, Piggy_fp );

		GameBitmapFlags[i+1] = 0;
		if ( bmh.flags & BM_FLAG_TRANSPARENT ) GameBitmapFlags[i+1] |= BM_FLAG_TRANSPARENT;
		if ( bmh.flags & BM_FLAG_SUPER_TRANSPARENT ) GameBitmapFlags[i+1] |= BM_FLAG_SUPER_TRANSPARENT;
		if ( bmh.flags & BM_FLAG_NO_LIGHTING ) GameBitmapFlags[i+1] |= BM_FLAG_NO_LIGHTING;
		if ( bmh.flags & BM_FLAG_RLE ) GameBitmapFlags[i+1] |= BM_FLAG_RLE;

		GameBitmapOffset[i+1] = bmh.offset + header_size + (sizeof(int)*2) + Pigdata_start;
		Assert( (i+1) == Num_bitmap_files );

		//size -= sizeof(DiskBitmapHeader);
		memcpy( temp_name_read, bmh.name, 8 );
		temp_name_read[8] = 0;
		if ( bmh.dflags & DBM_FLAG_ABM )	
			sprintf( temp_name, "%s#%d", temp_name_read, bmh.dflags & 63 );
		else
			strcpy( temp_name, temp_name_read );

		memset( &temp_bitmap, 0, sizeof(grs_bitmap) );
		gr_init_bitmap( &temp_bitmap, 0, 0, 0, 
			(bmh.dflags & DBM_FLAG_LARGE) ? bmh.width + 256 : bmh.width, bmh.height,
			(bmh.dflags & DBM_FLAG_LARGE) ? bmh.width + 256 : bmh.width, Piggy_bitmap_cache_data);
		temp_bitmap.bm_flags |= BM_FLAG_PAGED_OUT;
		temp_bitmap.avg_color = bmh.avg_color;

		piggy_register_bitmap( &temp_bitmap, temp_name, 1 );
	}

        for (i=0; i<N_sounds; i++ )     {
                cfread( &sndh, sizeof(DiskSoundHeader), 1, Piggy_fp );

		//size -= sizeof(DiskSoundHeader);
#ifdef ALLEGRO
		temp_sound.len = sndh.length;
#else
		temp_sound.length = sndh.length;
#endif

//added on 11/13/99 by Victor Rachels to ready for changing freq
                temp_sound.bits = 8;
                temp_sound.freq = 11025;
//end this section addition - VR
		temp_sound.data = (ubyte *)(sndh.offset + header_size + (sizeof(int)*2)+Pigdata_start);
		SoundOffset[Num_sound_files] = sndh.offset + header_size + (sizeof(int)*2)+Pigdata_start;
#ifdef SHAREWARE
		SoundCompressed[Num_sound_files] = sndh.data_length;
#endif
		memcpy( temp_name_read, sndh.name, 8 );
		temp_name_read[8] = 0;
		piggy_register_sound( &temp_sound, temp_name_read, 1 );
                sbytes += sndh.length;
		//mprintf(( 0, "%d bytes of sound\n", sbytes ));

	}

	SoundBits = malloc( sbytes + 16 );
         if ( SoundBits == NULL )
          Error( "Not enough memory to load DESCENT.PIG sounds\n");

#ifdef EDITOR
	Piggy_bitmap_cache_size	= size - header_size - sbytes + 16;
	Assert( Piggy_bitmap_cache_size > 0 );
#else
	Piggy_bitmap_cache_size = PIGGY_BUFFER_SIZE;
#endif
	BitmapBits = malloc( Piggy_bitmap_cache_size );
	if ( BitmapBits == NULL )
		Error( "Not enough memory to load DESCENT.PIG bitmaps\n" );
	Piggy_bitmap_cache_data = BitmapBits;	
	Piggy_bitmap_cache_next = 0;
	
	mprintf(( 0, "\nBitmaps: %d KB   Sounds: %d KB\n", Piggy_bitmap_cache_size/1024, sbytes/1024 ));

	atexit(piggy_close_file);

//	mprintf( (0, "<<<<Paging in all piggy bitmaps...>>>>>" ));
//	for (i=0; i < Num_bitmap_files; i++ )	{
//		bitmap_index bi;
//		bi.index = i;
//		PIGGY_PAGE_IN( bi );
//	}
//	mprintf( (0, "\n (USed %d / %d KB)\n", Piggy_bitmap_cache_next/1024, (size - header_size - sbytes + 16)/1024 ));
//	key_getch();

	return 0;
}

int piggy_is_needed(int soundnum)
{
	int i;

	if ( !digi_lomem ) return 1;

	for (i=0; i<MAX_SOUNDS; i++ )	{
		if ( (AltSounds[i] < 255) && (Sounds[AltSounds[i]] == soundnum) )
			return 1;
	}
	return 0;
}

void piggy_read_sounds()
{
	ubyte * ptr;
	int i, sbytes;
#ifdef SHAREWARE
	int lastsize = 0;
	ubyte * lastbuf = NULL;
#endif

	ptr = SoundBits;
	sbytes = 0;

	for (i=0; i<Num_sound_files; i++ )	{
		digi_sound *snd = &GameSounds[i];

                if ( SoundOffset[i] > 0 )       {
                        if ( piggy_is_needed(i) )       {
				cfseek( Piggy_fp, SoundOffset[i], SEEK_SET );
	
				// Read in the sound data!!!
				snd->data = ptr;
#ifdef ALLEGRO
				ptr += snd->len;
				sbytes += snd->len;
#else
				ptr += snd->length;
				sbytes += snd->length;
#endif
//Arne's decompress for shareware on all soundcards - Tim@Rikers.org
#ifdef SHAREWARE
				if (lastsize < SoundCompressed[i]) {
					if (lastbuf) free(lastbuf);
					lastbuf = malloc(SoundCompressed[i]);
				}
				cfread( lastbuf, SoundCompressed[i], 1, Piggy_fp );
				sound_decompress( lastbuf, SoundCompressed[i], snd->data );
#else
#ifdef ALLEGRO
				cfread( snd->data, snd->len, 1, Piggy_fp );
#else
				cfread( snd->data, snd->length, 1, Piggy_fp );
#endif
#endif
			}
                }
	}
        mprintf(( 0, "\nActual Sound usage: %d KB\n", sbytes/1024 ));
#ifdef SHAREWARE
	if (lastbuf)
	  free(lastbuf);
#endif
}

extern int descent_critical_error;
extern unsigned descent_critical_deverror;
extern unsigned descent_critical_errcode;

char * crit_errors[13] = { "Write Protected", "Unknown Unit", "Drive Not Ready", "Unknown Command", "CRC Error",
"Bad struct length", "Seek Error", "Unknown media type", "Sector not found", "Printer out of paper", "Write Fault",
"Read fault", "General Failure" };

void piggy_critical_error()
{
	grs_canvas * save_canv;
	grs_font * save_font;
	int i;
	save_canv = grd_curcanv;
	save_font = grd_curcanv->cv_font;
	gr_palette_load( gr_palette );
	i = nm_messagebox( "Disk Error", 2, "Retry", "Exit", "%s\non drive %c:", crit_errors[descent_critical_errcode&0xf], (descent_critical_deverror&0xf)+'A'  );
	if ( i == 1 )
		exit(1);
	gr_set_current_canvas(save_canv);
	grd_curcanv->cv_font = save_font;
}

void piggy_bitmap_page_in( bitmap_index bitmap )
{
	grs_bitmap * bmp;
	int i,org_i,temp;

        org_i = 0;
			
	i = bitmap.index;
	Assert( i >= 0 );
	Assert( i < MAX_BITMAP_FILES );
	Assert( i < Num_bitmap_files );
	Assert( Piggy_bitmap_cache_size > 0 );

	if ( i < 1 ) return;
	if ( i >= MAX_BITMAP_FILES ) return;
	if ( i >= Num_bitmap_files ) return;

	if ( GameBitmapOffset[i] == 0 ) return;		// A read-from-disk bitmap!!!

	if ( GameArg.SysLowMem )	{
		org_i = i;
		i = GameBitmapXlat[i];		// Xlat for low-memory settings!
	}
	bmp = &GameBitmaps[i];
	
	if ( bmp->bm_flags & BM_FLAG_PAGED_OUT )	{
		stop_time();

	ReDoIt:
		descent_critical_error = 0;
		cfseek( Piggy_fp, GameBitmapOffset[i], SEEK_SET );
		if ( descent_critical_error )	{
			piggy_critical_error();
			goto ReDoIt;
		}
		gr_set_bitmap_flags (bmp, GameBitmapFlags[i]);
		gr_set_bitmap_data (bmp, &Piggy_bitmap_cache_data [Piggy_bitmap_cache_next]);

		if ( bmp->bm_flags & BM_FLAG_RLE )	{
			int zsize = 0;
			descent_critical_error = 0;
			temp = cfread( &zsize, 1, sizeof(int), Piggy_fp );
			if ( descent_critical_error )	{
				piggy_critical_error();
				goto ReDoIt;
			}
	
			// GET JOHN NOW IF YOU GET THIS ASSERT!!!
			Assert( Piggy_bitmap_cache_next+zsize < Piggy_bitmap_cache_size );	
			if ( Piggy_bitmap_cache_next+zsize >= Piggy_bitmap_cache_size )	{
				piggy_bitmap_page_out_all();
				goto ReDoIt;
			}
			memcpy( &Piggy_bitmap_cache_data[Piggy_bitmap_cache_next], &zsize, sizeof(int) );
			Piggy_bitmap_cache_next += sizeof(int);
			descent_critical_error = 0;
			temp = cfread( &Piggy_bitmap_cache_data[Piggy_bitmap_cache_next], 1, zsize-4, Piggy_fp );
			if ( descent_critical_error )	{
				piggy_critical_error();
				goto ReDoIt;
			}
			Piggy_bitmap_cache_next += zsize-4;
		} else {
			// GET JOHN NOW IF YOU GET THIS ASSERT!!!
			Assert( Piggy_bitmap_cache_next+(bmp->bm_h*bmp->bm_w) < Piggy_bitmap_cache_size );	
			if ( Piggy_bitmap_cache_next+(bmp->bm_h*bmp->bm_w) >= Piggy_bitmap_cache_size )	{
				piggy_bitmap_page_out_all();
				goto ReDoIt;
			}
			descent_critical_error = 0;
			temp = cfread( &Piggy_bitmap_cache_data[Piggy_bitmap_cache_next], 1, bmp->bm_h*bmp->bm_w, Piggy_fp );
			if ( descent_critical_error )	{
				piggy_critical_error();
				goto ReDoIt;
			}
			Piggy_bitmap_cache_next+=bmp->bm_h*bmp->bm_w;
		}
	
		#ifdef BITMAP_SELECTOR
		if ( bmp->bm_selector ) {
			if (!dpmi_modify_selector_base( bmp->bm_selector, bmp->bm_data ))
				Error( "Error modifying selector base in piggy.c\n" );
		}
		#endif
		start_time();
	}

	if ( GameArg.SysLowMem )	{
		if ( org_i != i )
			GameBitmaps[org_i] = GameBitmaps[i];
	}
	
}

void piggy_bitmap_page_out_all()
{
	int i;
	
	Piggy_bitmap_cache_next = 0;

	piggy_page_flushed++;

	texmerge_flush();
	rle_cache_flush();

	for (i=0; i<Num_bitmap_files; i++ )		{
		if ( GameBitmapOffset[i] > 0 )	{	// Don't page out bitmaps read from disk!!!
			GameBitmaps[i].bm_flags = BM_FLAG_PAGED_OUT;
				gr_set_bitmap_data (&GameBitmaps[i], Piggy_bitmap_cache_data);
		}
	}

	mprintf(( 0, "Flushing piggy bitmap cache\n" ));
}

void piggy_load_level_data()
{
	piggy_bitmap_page_out_all();
	paging_touch_all();
}

#ifdef EDITOR
void piggy_dump_all()
{
	int i, xlat_offset;
	FILE * fp;
#ifndef RELEASE
	FILE * fp1;
	FILE * fp2;
#endif
	char * filename;
	int data_offset;
	int org_offset;
	DiskBitmapHeader bmh;
	DiskSoundHeader sndh;
	int header_offset;
	char subst_name[32];

	#ifdef NO_DUMP_SOUNDS
	Num_sound_files = 0;
	Num_sound_files_new = 0;
	#endif

//	{
//	bitmap_index bi;
//	bi.index = 614;
//	PIGGY_PAGE_IN( bi );
//	count_colors( bi.index, &GameBitmaps[bi.index] );
//	key_getch();
//	}
//	{
//	bitmap_index bi;
//	bi.index = 478;
//	PIGGY_PAGE_IN( bi );
//	Int3();
//	count_colors( bi.index, &GameBitmaps[bi.index] );
//	key_getch();
//	}
//	{
//	bitmap_index bi;
//	bi.index = 1398;
//	PIGGY_PAGE_IN( bi );
//	count_colors( bi.index, &GameBitmaps[bi.index] );
//	key_getch();
//	}
//	{
//	bitmap_index bi;
//	bi.index = 642;
//	PIGGY_PAGE_IN( bi );
//	count_colors( bi.index, &GameBitmaps[bi.index] );
//	key_getch();
//	}
//	{
//	bitmap_index bi;
//	bi.index = 529;
//	PIGGY_PAGE_IN( bi );
//	count_colors( bi.index, &GameBitmaps[bi.index] );
//	key_getch();
//	}
//	exit(0);
//
	if ((Num_bitmap_files_new == 0) && (Num_sound_files_new == 0) )
		return;

	mprintf( (0, "Paging in all piggy bitmaps..." ));
	for (i=0; i < Num_bitmap_files; i++ )	{
		bitmap_index bi;
		bi.index = i;
		PIGGY_PAGE_IN( bi );
	}
	mprintf( (0, "\n" ));

	piggy_close_file();

	mprintf( (0, "Creating DESCENT.PIG..." ));
        filename = DESCENT_DATA_PATH "descent.pig";
	if ( (i=FindArg( "-piggy" )) )	{
		filename	= Args[i+1];
		mprintf( (0, "Dumping alternate pigfile, '%s'\n", filename ));
	} 
	mprintf( (0, "\nDumping bitmaps..." ));

	fp = fopen( filename, "wb" );
	Assert( fp!=NULL );

#ifndef RELEASE
	fp1 = fopen( "piggy.lst", "wt" );
	fp2 = fopen( "piggy.all", "wt" );
#endif

	i = 0;
	fwrite( &i, sizeof(int), 1, fp );	
	bm_write_all(fp);
	xlat_offset = ftell(fp);
	fwrite( GameBitmapXlat, sizeof(ushort)*MAX_BITMAP_FILES, 1, fp );
	i = ftell(fp);
	fseek( fp, 0, SEEK_SET );
	fwrite( &i, sizeof(int), 1, fp );
	fseek( fp, i, SEEK_SET );
		
	Num_bitmap_files--;
	fwrite( &Num_bitmap_files, sizeof(int), 1, fp );
	Num_bitmap_files++;
	fwrite( &Num_sound_files, sizeof(int), 1, fp );

	header_offset = ftell(fp);
	header_offset += ((Num_bitmap_files-1)*sizeof(DiskBitmapHeader)) + (Num_sound_files*sizeof(DiskSoundHeader));
	data_offset = header_offset;

	for (i=1; i < Num_bitmap_files; i++ )	{
		int *size;
		grs_bitmap *bmp;

		{		
			char * p, *p1;
			p = strchr(AllBitmaps[i].name,'#');
			if (p)	{
				int n;
				p1 = p; p1++; 
				n = atoi(p1);
				*p = 0;
#ifndef RELEASE
				if (n==0)	{		
					fprintf( fp2, "%s.abm\n", AllBitmaps[i].name );
				}	
#endif
				memcpy( bmh.name, AllBitmaps[i].name, 8 );
				Assert( n <= 63 );
				bmh.dflags = DBM_FLAG_ABM + n;
				*p = '#';
			}else {
#ifndef RELEASE
				fprintf( fp2, "%s.bbm\n", AllBitmaps[i].name );
#endif
				memcpy( bmh.name, AllBitmaps[i].name, 8 );
				bmh.dflags = 0;
			}
		}
		bmp = &GameBitmaps[i];

		Assert( !(bmp->bm_flags&BM_FLAG_PAGED_OUT) );

#ifndef RELEASE
		fprintf( fp1, "BMP: %s, size %d bytes", AllBitmaps[i].name, bmp->bm_rowsize * bmp->bm_h );
#endif
		org_offset = ftell(fp);
		bmh.offset = data_offset - header_offset;
		fseek( fp, data_offset, SEEK_SET );

		if ( bmp->bm_flags & BM_FLAG_RLE )	{
			size = (int *)bmp->bm_data;
			fwrite( bmp->bm_data, sizeof(ubyte), *size, fp );
			data_offset += *size;
			//bmh.data_length = *size;
#ifndef RELEASE
			fprintf( fp1, ", and is already compressed to %d bytes.\n", *size );
#endif
		} else {
			fwrite( bmp->bm_data, sizeof(ubyte), bmp->bm_rowsize * bmp->bm_h, fp );
			data_offset += bmp->bm_rowsize * bmp->bm_h;
			//bmh.data_length = bmp->bm_rowsize * bmp->bm_h;
#ifndef RELEASE
			fprintf( fp1, ".\n" );
#endif
		}
		fseek( fp, org_offset, SEEK_SET );
		if ( GameBitmaps[i].bm_w > 255 )	{
			Assert( GameBitmaps[i].bm_w < 512 );
			bmh.width = GameBitmaps[i].bm_w - 256;
			bmh.dflags |= DBM_FLAG_LARGE;
		} else {
			bmh.width = GameBitmaps[i].bm_w;
		}
		Assert( GameBitmaps[i].bm_h < 256 );
		bmh.height = GameBitmaps[i].bm_h;
		bmh.flags = GameBitmaps[i].bm_flags;
		if (piggy_is_substitutable_bitmap( AllBitmaps[i].name, subst_name ))	{
			bitmap_index other_bitmap;
			other_bitmap = piggy_find_bitmap( subst_name );
			GameBitmapXlat[i] = other_bitmap.index;
			bmh.flags |= BM_FLAG_PAGED_OUT;
			//mprintf(( 0, "Skipping bitmap %d\n", i ));
			//mprintf(( 0, "Marking '%s' as substitutible\n", AllBitmaps[i].name ));
		} else	{
#ifdef BUILD_PSX_DATA
			count_colors( i, &GameBitmaps[i] );
#endif
			bmh.flags &= ~BM_FLAG_PAGED_OUT;
		}
		bmh.avg_color=GameBitmaps[i].avg_color;
		fwrite( &bmh, sizeof(DiskBitmapHeader), 1, fp );			// Mark as a bitmap
	}

	mprintf( (0, "\nDumping sounds..." ));

	for (i=0; i < Num_sound_files; i++ )
         {
		digi_sound *snd;

		snd = &GameSounds[i];
		strcpy( sndh.name, AllSounds[i].name );
#ifdef ALLEGRO
		sndh.length = GameSounds[i].len;
#else
                sndh.length = GameSounds[i].length;
#endif
		sndh.offset = data_offset - header_offset;

		org_offset = ftell(fp);
		fseek( fp, data_offset, SEEK_SET );

		sndh.data_length = sndh.length;
		fwrite( snd->data, sizeof(ubyte), sndh.length, fp );
		data_offset += sndh.length;
		fseek( fp, org_offset, SEEK_SET );
		fwrite( &sndh, sizeof(DiskSoundHeader), 1, fp );			// Mark as a bitmap

#ifndef RELEASE
		fprintf( fp1, "SND: %s, size %d bytes\n", AllSounds[i].name, sndh.length );

		fprintf( fp2, "%s.raw\n", AllSounds[i].name );
#endif
         }

	fseek( fp, xlat_offset, SEEK_SET );
	fwrite( GameBitmapXlat, sizeof(ushort)*MAX_BITMAP_FILES, 1, fp );

	fclose(fp);

	mprintf( (0, "\n" ));

	mprintf( (0, " Dumped %d assorted bitmaps.\n", Num_bitmap_files ));
	mprintf( (0, " Dumped %d assorted sounds.\n", Num_sound_files ));

#ifndef RELEASE
	fprintf( fp1, " Dumped %d assorted bitmaps.\n", Num_bitmap_files );
	fprintf( fp1, " Dumped %d assorted sounds.\n", Num_sound_files );

	fclose(fp1);
	fclose(fp2);
#endif

#ifdef BUILD_PSX_DATA
	fp = fopen( "psx/descent.dat", "wb" );
	fwrite( &i, sizeof(int), 1, fp );	
	bm_write_all(fp);
	fwrite( GameBitmapXlat, sizeof(ushort)*MAX_BITMAP_FILES, 1, fp );
	fclose(fp);
#endif

	// Never allow the game to run after building pig.
	exit(0);
}

#endif

void piggy_close()
{
	custom_close();

//added ifndef on 10/04/98 by Matt Mueller to fix crash on exit bug -- killed 2000/02/06 since they don't seem to cause crash anymore.  heh.
//#ifndef __LINUX__
	if (BitmapBits)
		free(BitmapBits);

	if ( SoundBits )
		free( SoundBits );
//#endif
//end addition -MM
	
	hashtable_free( &AllBitmapsNames );
	hashtable_free( &AllDigiSndNames );

}

int piggy_does_bitmap_exist_slow( char * name )
{
	int i;

	for (i=0; i<Num_bitmap_files; i++ )	{
		if ( !strcmp( AllBitmaps[i].name, name) )
			return 1;
	}
	return 0;
}


#define NUM_GAUGE_BITMAPS 10
char * gauge_bitmap_names[NUM_GAUGE_BITMAPS] = { "gauge01", "gauge02", "gauge06", "targ01", "targ02", "targ03", "targ04", "targ05", "targ06", "gauge18" };

int piggy_is_gauge_bitmap( char * base_name )
{
	int i;
	for (i=0; i<NUM_GAUGE_BITMAPS; i++ )	{
		if ( !strcasecmp( base_name, gauge_bitmap_names[i] ))	
			return 1;
	}

	return 0;	
}

int piggy_is_substitutable_bitmap( char * name, char * subst_name )
{
	int frame;
	char * p;
	char base_name[ 16 ];
	
	strcpy( subst_name, name );
	p = strchr( subst_name, '#' );
	if ( p ) 	{
		frame = atoi( &p[1] );
		*p = 0;
		strcpy( base_name, subst_name );
		if ( !piggy_is_gauge_bitmap( base_name ))	{
			sprintf( subst_name, "%s#%d", base_name, frame+1 );
			if ( piggy_does_bitmap_exist_slow( subst_name )  ) 	{
				if ( frame & 1 ) {
					sprintf( subst_name, "%s#%d", base_name, frame-1 );
					return 1;
				}
			}
		}
	}
	strcpy( subst_name, name );
	return 0;
}

