/* $Id: piggy.c,v 1.48 2003-12-28 00:35:16 schaffner Exp $ */
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
 * Functions for managing the pig files.
 *
 * Old Log:
 * Revision 1.16  1995/11/09  17:27:47  allender
 * put in missing quote on new gauge name
 *
 * Revision 1.15  1995/11/08  17:28:03  allender
 * add PC gauges to gauge list of non-substitutatble bitmaps
 *
 * Revision 1.14  1995/11/08  15:14:49  allender
 * fixed horrible bug where the piggy cache size was incorrect
 * for mac shareware
 *
 * Revision 1.13  1995/11/03  12:53:37  allender
 * shareware changes
 *
 * Revision 1.12  1995/10/21  22:25:14  allender
 * added bald guy cheat
 *
 * Revision 1.11  1995/10/20  22:42:15  allender
 * changed load path of descent.pig to :data:descent.pig
 *
 * Revision 1.10  1995/10/20  00:08:01  allender
 * put in event loop calls when loading data (hides it nicely
 * from user) so TM can get it's strokes stuff
 *
 * Revision 1.9  1995/09/13  08:48:01  allender
 * added lower memory requirement to load alternate bitmaps
 *
 * Revision 1.8  1995/08/16  09:39:13  allender
 * moved "loading" text up a little
 *
 * Revision 1.7  1995/08/08  13:54:26  allender
 * added macsys header file
 *
 * Revision 1.6  1995/07/12  12:49:56  allender
 * total hack for bitmaps > 512 bytes wide -- check these by name
 *
 * Revision 1.5  1995/07/05  16:47:05  allender
 * kitchen stuff
 *
 * Revision 1.4  1995/06/23  08:55:28  allender
 * make "loading data" text y loc based off of curcanv
 *
 * Revision 1.3  1995/06/08  14:08:52  allender
 * PPC aligned data sets
 *
 * Revision 1.2  1995/05/26  06:54:27  allender
 * removed refences to sound data at end of pig file (since they will
 * now be Macintosh snd resources for effects
 *
 * Revision 1.1  1995/05/16  15:29:51  allender
 * Initial revision
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


#ifdef HAVE_CONFIG_H
#include <conf.h>
#endif

#ifdef RCS
static char rcsid[] = "$Id: piggy.c,v 1.48 2003-12-28 00:35:16 schaffner Exp $";
#endif


#include <stdio.h>
#include <string.h>

#include "pstypes.h"
#include "strutil.h"
#include "inferno.h"
#include "gr.h"
#include "u_mem.h"
#include "iff.h"
#include "mono.h"
#include "error.h"
#include "sounds.h"
#include "songs.h"
#include "bm.h"
#include "bmread.h"
#include "hash.h"
#include "args.h"
#include "palette.h"
#include "gamefont.h"
#include "rle.h"
#include "screens.h"
#include "piggy.h"
#include "gamemine.h"
#include "textures.h"
#include "texmerge.h"
#include "paging.h"
#include "game.h"
#include "text.h"
#include "cfile.h"
#include "newmenu.h"
#include "byteswap.h"
#include "findfile.h"
#include "makesig.h"

#ifndef MACINTOSH
//	#include "unarj.h"
#else
	#include <Strings.h>		// MacOS Toolbox header
	#include <Files.h>
	#include <unistd.h>
#endif

//#define NO_DUMP_SOUNDS        1   //if set, dump bitmaps but not sounds

#define DEFAULT_PIGFILE_REGISTERED      "groupa.pig"
#define DEFAULT_PIGFILE_SHAREWARE       "d2demo.pig"
#define DEFAULT_HAMFILE_REGISTERED      "descent2.ham"
#define DEFAULT_HAMFILE_SHAREWARE       "d2demo.ham"

#define D1_PALETTE "palette.256"

#define DEFAULT_PIGFILE (cfexist(DEFAULT_PIGFILE_REGISTERED)?DEFAULT_PIGFILE_REGISTERED:DEFAULT_PIGFILE_SHAREWARE)
#define DEFAULT_HAMFILE (cfexist(DEFAULT_HAMFILE_REGISTERED)?DEFAULT_HAMFILE_REGISTERED:DEFAULT_HAMFILE_SHAREWARE)
#define DEFAULT_SNDFILE ((Piggy_hamfile_version < 3)?DEFAULT_HAMFILE_SHAREWARE:(digi_sample_rate==SAMPLE_RATE_22K)?"descent2.s22":"descent2.s11")

#define MAC_ALIEN1_PIGSIZE      5013035
#define MAC_ALIEN2_PIGSIZE      4909916
#define MAC_FIRE_PIGSIZE        4969035
#define MAC_GROUPA_PIGSIZE      4929684 // also used for mac shareware
#define MAC_ICE_PIGSIZE         4923425
#define MAC_WATER_PIGSIZE       4832403

ubyte *BitmapBits = NULL;
ubyte *SoundBits = NULL;

typedef struct BitmapFile {
	char    name[15];
} BitmapFile;

typedef struct SoundFile {
	char    name[15];
} SoundFile;

hashtable AllBitmapsNames;
hashtable AllDigiSndNames;

int Num_bitmap_files = 0;
int Num_sound_files = 0;

digi_sound GameSounds[MAX_SOUND_FILES];
int SoundOffset[MAX_SOUND_FILES];
grs_bitmap GameBitmaps[MAX_BITMAP_FILES];

alias alias_list[MAX_ALIASES];
int Num_aliases=0;

int Must_write_hamfile = 0;
int Num_bitmap_files_new = 0;
int Num_sound_files_new = 0;
BitmapFile AllBitmaps[ MAX_BITMAP_FILES ];
static SoundFile AllSounds[ MAX_SOUND_FILES ];

int Piggy_hamfile_version = 0;

int piggy_low_memory = 0;

int Piggy_bitmap_cache_size = 0;
int Piggy_bitmap_cache_next = 0;
ubyte * Piggy_bitmap_cache_data = NULL;
static int GameBitmapOffset[MAX_BITMAP_FILES];
static ubyte GameBitmapFlags[MAX_BITMAP_FILES];
ushort GameBitmapXlat[MAX_BITMAP_FILES];

#define PIGGY_BUFFER_SIZE (2400*1024)

#ifdef MACINTOSH
#define PIGGY_SMALL_BUFFER_SIZE (1400*1024)		// size of buffer when piggy_low_memory is set

#ifdef SHAREWARE
#undef PIGGY_BUFFER_SIZE
#undef PIGGY_SMALL_BUFFER_SIZE

#define PIGGY_BUFFER_SIZE (2000*1024)
#define PIGGY_SMALL_BUFFER_SIZE (1100 * 1024)
#endif		// SHAREWARE

#endif

int piggy_page_flushed = 0;

#define DBM_FLAG_ABM    64 // animated bitmap
#define DBM_NUM_FRAMES  63

#define BM_FLAGS_TO_COPY (BM_FLAG_TRANSPARENT | BM_FLAG_SUPER_TRANSPARENT \
                         | BM_FLAG_NO_LIGHTING | BM_FLAG_RLE | BM_FLAG_RLE_BIG)

typedef struct DiskBitmapHeader {
	char name[8];
	ubyte dflags;           // bits 0-5 anim frame num, bit 6 abm flag
	ubyte width;            // low 8 bits here, 4 more bits in wh_extra
	ubyte height;           // low 8 bits here, 4 more bits in wh_extra
	ubyte wh_extra;         // bits 0-3 width, bits 4-7 height
	ubyte flags;
	ubyte avg_color;
	int offset;
} __pack__ DiskBitmapHeader;

#define DISKBITMAPHEADER_D1_SIZE 17 // no wh_extra

typedef struct DiskSoundHeader {
	char name[8];
	int length;
	int data_length;
	int offset;
} __pack__ DiskSoundHeader;

#ifdef FAST_FILE_IO
#define DiskBitmapHeader_read(dbh, fp) cfread(dbh, sizeof(DiskBitmapHeader), 1, fp)
#define DiskSoundHeader_read(dsh, fp) cfread(dsh, sizeof(DiskSoundHeader), 1, fp)
#else
/*
 * reads a DiskBitmapHeader structure from a CFILE
 */
void DiskBitmapHeader_read(DiskBitmapHeader *dbh, CFILE *fp)
{
	cfread(dbh->name, 8, 1, fp);
	dbh->dflags = cfile_read_byte(fp);
	dbh->width = cfile_read_byte(fp);
	dbh->height = cfile_read_byte(fp);
	dbh->wh_extra = cfile_read_byte(fp);
	dbh->flags = cfile_read_byte(fp);
	dbh->avg_color = cfile_read_byte(fp);
	dbh->offset = cfile_read_int(fp);
}

/*
 * reads a DiskSoundHeader structure from a CFILE
 */
void DiskSoundHeader_read(DiskSoundHeader *dsh, CFILE *fp)
{
	cfread(dsh->name, 8, 1, fp);
	dsh->length = cfile_read_int(fp);
	dsh->data_length = cfile_read_int(fp);
	dsh->offset = cfile_read_int(fp);
}
#endif // FAST_FILE_IO

/*
 * reads a descent 1 DiskBitmapHeader structure from a CFILE
 */
void DiskBitmapHeader_d1_read(DiskBitmapHeader *dbh, CFILE *fp)
{
	cfread(dbh->name, 8, 1, fp);
	dbh->dflags = cfile_read_byte(fp);
	dbh->width = cfile_read_byte(fp);
	dbh->height = cfile_read_byte(fp);
	dbh->wh_extra = 0;
	dbh->flags = cfile_read_byte(fp);
	dbh->avg_color = cfile_read_byte(fp);
	dbh->offset = cfile_read_int(fp);
}

ubyte BigPig = 0;

#ifdef MACINTOSH
	extern short 	cd_VRefNum;
	extern void		ConcatPStr(StringPtr dst, StringPtr src);
	extern int		ConvertPToCStr(StringPtr inPStr, char* outCStrBuf);
	extern int		ConvertCToPStr(char* inCStr, StringPtr outPStrBuf);
#endif

int piggy_is_substitutable_bitmap( char * name, char * subst_name );

#ifdef EDITOR
void piggy_write_pigfile(char *filename);
static void write_int(int i,FILE *file);
#endif

void swap_0_255(grs_bitmap *bmp)
{
	int i;

	for (i = 0; i < bmp->bm_h * bmp->bm_w; i++) {
		if(bmp->bm_data[i] == 0)
			bmp->bm_data[i] = 255;
		else if (bmp->bm_data[i] == 255)
			bmp->bm_data[i] = 0;
	}
}

bitmap_index piggy_register_bitmap( grs_bitmap * bmp, char * name, int in_file )
{
	bitmap_index temp;
	Assert( Num_bitmap_files < MAX_BITMAP_FILES );

	temp.index = Num_bitmap_files;

	if (!in_file)   {
#ifdef EDITOR
		if ( FindArg("-macdata") )
			swap_0_255( bmp );
#endif
		if ( !BigPig )  gr_bitmap_rle_compress( bmp );
		Num_bitmap_files_new++;
	}

	strncpy( AllBitmaps[Num_bitmap_files].name, name, 12 );
	hashtable_insert( &AllBitmapsNames, AllBitmaps[Num_bitmap_files].name, Num_bitmap_files );
	GameBitmaps[Num_bitmap_files] = *bmp;
	if ( !in_file ) {
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
	if ( !in_file ) {
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
	char *t;

	bmp.index = 0;

	if ((t=strchr(name,'#'))!=NULL)
		*t=0;

	for (i=0;i<Num_aliases;i++)
		if (stricmp(name,alias_list[i].alias_name)==0) {
			if (t) {                //extra stuff for ABMs
				static char temp[FILENAME_LEN];
				_splitpath(alias_list[i].file_name, NULL, NULL, temp, NULL );
				name = temp;
				strcat(name,"#");
				strcat(name,t+1);
			}
			else
				name=alias_list[i].file_name; 
			break;
		}

	if (t)
		*t = '#';

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

#define FILENAME_LEN 13

char Current_pigfile[FILENAME_LEN] = "";

void piggy_close_file()
{
	if ( Piggy_fp ) {
		cfclose( Piggy_fp );
		Piggy_fp        = NULL;
		Current_pigfile[0] = 0;
	}
}

int Pigfile_initialized=0;

#define PIGFILE_ID              MAKE_SIG('G','I','P','P') //PPIG
#define PIGFILE_VERSION         2

extern char CDROM_dir[];

int request_cd(void);


#ifdef MACINTOSH

//copies a pigfile from the CD to the current dir
//retuns file handle of new pig
CFILE *copy_pigfile_from_cd(char *filename)		// MACINTOSH VERSION
{
	// C Stuff
	char			sourcePathAndFileCStr[255] = "";
	char			destPathAndFileCStr[255]	= "";
	FILEFINDSTRUCT	find;
	FILE*			sourceFile	= NULL;
	FILE*			destFile	= NULL;
	const int		BUF_SIZE = 4096;
	ubyte 			buf[BUF_SIZE];

	// Mac Stuff
	Str255			sourcePathAndFilePStr = "\p";
	Str255			destPathAndFilePStr = "\p";
	Str255			pigfileNamePStr = "\p";
	HParamBlockRec	theSourcePigHFSParams;
	HParamBlockRec	theDestPigHFSParams;
	OSErr			theErr = noErr;
	char			oldDirCStr[255] = "";

	getcwd(oldDirCStr, 255);
	
	show_boxed_message("Copying bitmap data from CD...");
	gr_palette_load(gr_palette);    //I don't think this line is really needed

	chdir(":Data");
	//First, delete all PIG files currently in the directory
	if( !FileFindFirst( "*.pig", &find ) )
	{
		do
		{
			remove(find.name);
		} while( !FileFindNext( &find ) );
		
		FileFindClose();
	}
	chdir(oldDirCStr);

	//Now, copy over new pig
	songs_stop_redbook();           //so we can read off the cd

	// make the source path "<cd volume>:Data:filename.pig"
//MWA	ConvertCToPStr(filename, pigfileNamePStr);

//MWA	ConcatPStr(sourcePathAndFilePStr, "\pDescent II:Data:");	// volume ID is cd_VRefNum
//MWA	ConcatPStr(sourcePathAndFilePStr, pigfileNamePStr);

	strupr(filename);
	strcpy(sourcePathAndFileCStr, "Descent II:Data:");
	strcat(sourcePathAndFileCStr, filename);
	
	// make the destination path "<default directory>:Data:filename.pig"
//MWA	ConcatPStr(destPathAndFilePStr, "\p:Data:");
//MWA	ConcatPStr(destPathAndFilePStr, pigfileNamePStr);
//MWA	ConvertPToCStr(sourcePathAndFilePStr, sourcePathAndFileCStr);
//MWA	ConvertPToCStr(destPathAndFilePStr, destPathAndFileCStr);

	strcpy(destPathAndFileCStr, ":Data:");
	strcat(destPathAndFileCStr, filename);

	strcpy(sourcePathAndFilePStr, sourcePathAndFileCStr);
	strcpy(destPathAndFilePStr, destPathAndFileCStr);
	c2pstr(sourcePathAndFilePStr);
	c2pstr(destPathAndFilePStr);
	
	do {
		// Open the source file
		sourceFile = fopen(sourcePathAndFileCStr,"rb");

		if (!sourceFile) {

			if (request_cd() == -1)
				Error("Cannot load file <%s> from CD",filename);
		}

	} while (!sourceFile);


	// Get the time stamp from the source file
	theSourcePigHFSParams.fileParam.ioCompletion 	= nil;
	theSourcePigHFSParams.fileParam.ioNamePtr		= sourcePathAndFilePStr;
	theSourcePigHFSParams.fileParam.ioVRefNum		= cd_VRefNum;
	theSourcePigHFSParams.fileParam.ioFDirIndex	= 0;
	theSourcePigHFSParams.fileParam.ioDirID		= 0;
	
	theErr = PBHGetFInfo(&theSourcePigHFSParams, false);
	if (theErr != noErr)
	{
		// Error getting file time stamp!! Why? JTS
		Error("Can't get old file time stamp: <%s>\n", sourcePathAndFileCStr);
	}
	
	// Copy the file over
	// C Stuff......
	
	// Open the destination file
	destFile = fopen(destPathAndFileCStr,"wb");
	if (!destFile)
	{
		Error("Cannot create file: <%s>\n", destPathAndFileCStr);
	}
	
	// Copy bytes until the end of the source file
	while (!feof(sourceFile))
	{
		int bytes_read;
		int x;

		bytes_read = fread(buf,1,BUF_SIZE,sourceFile);
		if (ferror(sourceFile))
			Error("Cannot read from file <%s>: %s", sourcePathAndFileCStr, strerror(errno));

// Assert is bogus		Assert(bytes_read == BUF_SIZE || feof(sourceFile));

		fwrite(buf,1,bytes_read,destFile);
		if (ferror(destFile))
			Error("Cannot write to file <%s>: %s",destPathAndFileCStr, strerror(errno));
	}

	// close the source/dest files
	if (fclose(sourceFile))
		Error("Error closing file <%s>: %s", sourcePathAndFileCStr, strerror(errno));
	if (fclose(destFile))
		Error("Error closing file <%s>: %s", destPathAndFileCStr, strerror(errno));

	// Get the current hfs data for the new file
	theDestPigHFSParams.fileParam.ioCompletion 	= nil;
	theDestPigHFSParams.fileParam.ioNamePtr		= destPathAndFilePStr;
	theDestPigHFSParams.fileParam.ioVRefNum		= 0;
	theDestPigHFSParams.fileParam.ioFDirIndex	= 0;
	theDestPigHFSParams.fileParam.ioDirID		= 0;
	theErr = PBHGetFInfo(&theDestPigHFSParams, false);
	if ((theErr != noErr) || (theDestPigHFSParams.fileParam.ioResult != noErr))
	{
		// Error getting file time stamp!! Why? JTS
		Error("Can't get destination pig file information: <%s>\n", destPathAndFileCStr);
	}

	// Reset this data !!!!! or else the relative pathname won't work, could use just filename instead but, oh well.
	theDestPigHFSParams.fileParam.ioNamePtr		= destPathAndFilePStr;
	theDestPigHFSParams.fileParam.ioVRefNum		= 0;
	theDestPigHFSParams.fileParam.ioFDirIndex	= 0;
	theDestPigHFSParams.fileParam.ioDirID		= 0;

	// Copy the time stamp from the source file info
	theDestPigHFSParams.fileParam.ioFlCrDat	= theSourcePigHFSParams.fileParam.ioFlCrDat;
	theDestPigHFSParams.fileParam.ioFlMdDat	= theSourcePigHFSParams.fileParam.ioFlMdDat;
	theDestPigHFSParams.fileParam.ioFlFndrInfo.fdType = 'PGGY';
	theDestPigHFSParams.fileParam.ioFlFndrInfo.fdCreator = 'DCT2';
	
	// Set the dest file's time stamp to the source file's time stamp values
	theErr = PBHSetFInfo(&theDestPigHFSParams, false);

	if ((theErr != noErr) || (theDestPigHFSParams.fileParam.ioResult != noErr))
	{
		Error("Can't set destination pig file time stamp: <%s>\n", destPathAndFileCStr);
	}

	theErr = PBHGetFInfo(&theDestPigHFSParams, false);

	return cfopen(destPathAndFileCStr, "rb");
}

#else	//PC Version of copy_pigfile_from_cd is below

//copies a pigfile from the CD to the current dir
//retuns file handle of new pig
CFILE *copy_pigfile_from_cd(char *filename)
{
	char name[80];
	FILEFINDSTRUCT find;
	int ret;

	return cfopen(filename, "rb");
	show_boxed_message("Copying bitmap data from CD...");
	gr_palette_load(gr_palette);    //I don't think this line is really needed

	//First, delete all PIG files currently in the directory

	if( !FileFindFirst( "*.pig", &find ) ) {
		do      {
			cfile_delete(find.name);
		} while( !FileFindNext( &find ) );
		FileFindClose();
	}

	//Now, copy over new pig

	songs_stop_redbook();           //so we can read off the cd

	//new code to unarj file
	strcpy(name,CDROM_dir);
	strcat(name,"descent2.sow");

	do {
//		ret = unarj_specific_file(name,filename,filename);
// DPH:FIXME

		ret = !EXIT_SUCCESS;

		if (ret != EXIT_SUCCESS) {

			//delete file, so we don't leave partial file
			cfile_delete(filename);

			#ifndef MACINTOSH
			if (request_cd() == -1)
			#endif
				//NOTE LINK TO ABOVE IF
				Error("Cannot load file <%s> from CD",filename);
		}

	} while (ret != EXIT_SUCCESS);

	return cfopen(filename, "rb");
}

#endif // end of ifdef MAC around copy_pigfile_from_cd

//initialize a pigfile, reading headers
//returns the size of all the bitmap data
void piggy_init_pigfile(char *filename)
{
	int i;
	char temp_name[16];
	char temp_name_read[16];
	grs_bitmap temp_bitmap;
	DiskBitmapHeader bmh;
	int header_size, N_bitmaps, data_size, data_start;
	#ifdef MACINTOSH
	char name[255];		// filename + path for the mac
	#endif

	piggy_close_file();             //close old pig if still open

	//rename pigfile for shareware
	if (stricmp(DEFAULT_PIGFILE, DEFAULT_PIGFILE_SHAREWARE) == 0 && !cfexist(filename))
		filename = DEFAULT_PIGFILE_SHAREWARE;

	#ifndef MACINTOSH
		Piggy_fp = cfopen( filename, "rb" );
	#else
		sprintf(name, ":Data:%s", filename);
		Piggy_fp = cfopen( name, "rb" );

		#ifdef SHAREWARE	// if we are in the shareware version, we must have the pig by now.
			if (Piggy_fp == NULL)
			{
				Error("Cannot load required file <%s>",name);
			}
		#endif	// end of if def shareware

	#endif

	if (!Piggy_fp) {
		#ifdef EDITOR
			return;         //if editor, ok to not have pig, because we'll build one
		#else
			Piggy_fp = copy_pigfile_from_cd(filename);
		#endif
	}

	if (Piggy_fp) {                         //make sure pig is valid type file & is up-to-date
		int pig_id,pig_version;

		pig_id = cfile_read_int(Piggy_fp);
		pig_version = cfile_read_int(Piggy_fp);
		if (pig_id != PIGFILE_ID || pig_version != PIGFILE_VERSION) {
			cfclose(Piggy_fp);              //out of date pig
			Piggy_fp = NULL;                        //..so pretend it's not here
		}
	}

	if (!Piggy_fp) {

		#ifdef EDITOR
			return;         //if editor, ok to not have pig, because we'll build one
		#else
			Error("Cannot load required file <%s>",filename);
		#endif
	}

	strncpy(Current_pigfile,filename,sizeof(Current_pigfile));

	N_bitmaps = cfile_read_int(Piggy_fp);

	header_size = N_bitmaps * sizeof(DiskBitmapHeader);

	data_start = header_size + cftell(Piggy_fp);

	data_size = cfilelength(Piggy_fp) - data_start;

	Num_bitmap_files = 1;

	for (i=0; i<N_bitmaps; i++ )    {
		DiskBitmapHeader_read(&bmh, Piggy_fp);
		memcpy( temp_name_read, bmh.name, 8 );
		temp_name_read[8] = 0;
		if ( bmh.dflags & DBM_FLAG_ABM )        
			sprintf( temp_name, "%s#%d", temp_name_read, bmh.dflags & DBM_NUM_FRAMES );
		else
			strcpy( temp_name, temp_name_read );
		memset( &temp_bitmap, 0, sizeof(grs_bitmap) );
		temp_bitmap.bm_w = temp_bitmap.bm_rowsize = bmh.width + ((short) (bmh.wh_extra&0x0f)<<8);
		temp_bitmap.bm_h = bmh.height + ((short) (bmh.wh_extra&0xf0)<<4);
		temp_bitmap.bm_flags = BM_FLAG_PAGED_OUT;
		temp_bitmap.avg_color = bmh.avg_color;
		temp_bitmap.bm_data = Piggy_bitmap_cache_data;

		GameBitmapFlags[i+1] = bmh.flags & BM_FLAGS_TO_COPY;

		GameBitmapOffset[i+1] = bmh.offset + data_start;
		Assert( (i+1) == Num_bitmap_files );
		piggy_register_bitmap( &temp_bitmap, temp_name, 1 );
	}

#ifdef EDITOR
	Piggy_bitmap_cache_size = data_size + (data_size/10);   //extra mem for new bitmaps
	Assert( Piggy_bitmap_cache_size > 0 );
#else
	Piggy_bitmap_cache_size = PIGGY_BUFFER_SIZE;
	#ifdef MACINTOSH
	if (piggy_low_memory)
		Piggy_bitmap_cache_size = PIGGY_SMALL_BUFFER_SIZE;
	#endif
#endif
	BitmapBits = d_malloc( Piggy_bitmap_cache_size );
	if ( BitmapBits == NULL )
		Error( "Not enough memory to load bitmaps\n" );
	Piggy_bitmap_cache_data = BitmapBits;
	Piggy_bitmap_cache_next = 0;

	#if defined(MACINTOSH) && defined(SHAREWARE)
//	load_exit_models();
	#endif

	Pigfile_initialized=1;
}

#define FILENAME_LEN 13
#define MAX_BITMAPS_PER_BRUSH 30

extern int compute_average_pixel(grs_bitmap *new);

ubyte *Bitmap_replacement_data = NULL;

//reads in a new pigfile (for new palette)
//returns the size of all the bitmap data
void piggy_new_pigfile(char *pigname)
{
	int i;
	char temp_name[16];
	char temp_name_read[16];
	grs_bitmap temp_bitmap;
	DiskBitmapHeader bmh;
	int header_size, N_bitmaps, data_size, data_start;
	int must_rewrite_pig = 0;
	#ifdef MACINTOSH
	char name[255];
	#endif

	strlwr(pigname);

	//rename pigfile for shareware
	if (stricmp(DEFAULT_PIGFILE, DEFAULT_PIGFILE_SHAREWARE) == 0 && !cfexist(pigname))
		pigname = DEFAULT_PIGFILE_SHAREWARE;

	if (strnicmp(Current_pigfile, pigname, sizeof(Current_pigfile)) == 0 // correct pig already loaded
	    && !Bitmap_replacement_data) // no need to reload: no bitmaps were altered
		return;

	if (!Pigfile_initialized) {                     //have we ever opened a pigfile?
		piggy_init_pigfile(pigname);            //..no, so do initialization stuff
		return;
	}
	else
		piggy_close_file();             //close old pig if still open

	Piggy_bitmap_cache_next = 0;            //free up cache

	strncpy(Current_pigfile,pigname,sizeof(Current_pigfile));

	#ifndef MACINTOSH
		Piggy_fp = cfopen( pigname, "rb" );
	#else
		sprintf(name, ":Data:%s", pigname);
		Piggy_fp = cfopen( name, "rb" );

		#ifdef SHAREWARE	// if we are in the shareware version, we must have the pig by now.
			if (Piggy_fp == NULL)
			{
				Error("Cannot load required file <%s>",name);
			}
		#endif	// end of if def shareware
	#endif

	#ifndef EDITOR
	if (!Piggy_fp)
		Piggy_fp = copy_pigfile_from_cd(pigname);
	#endif

	if (Piggy_fp) {  //make sure pig is valid type file & is up-to-date
		int pig_id,pig_version;

		pig_id = cfile_read_int(Piggy_fp);
		pig_version = cfile_read_int(Piggy_fp);
		if (pig_id != PIGFILE_ID || pig_version != PIGFILE_VERSION) {
			cfclose(Piggy_fp);              //out of date pig
			Piggy_fp = NULL;                        //..so pretend it's not here
		}
	}

#ifndef EDITOR
	if (!Piggy_fp)
		Error("Cannot open correct version of <%s>", pigname);
#endif

	if (Piggy_fp) {

		N_bitmaps = cfile_read_int(Piggy_fp);

		header_size = N_bitmaps * sizeof(DiskBitmapHeader);

		data_start = header_size + cftell(Piggy_fp);

		data_size = cfilelength(Piggy_fp) - data_start;

		for (i=1; i<=N_bitmaps; i++ )   {
			DiskBitmapHeader_read(&bmh, Piggy_fp);
			memcpy( temp_name_read, bmh.name, 8 );
			temp_name_read[8] = 0;
	
			if ( bmh.dflags & DBM_FLAG_ABM )        
				sprintf( temp_name, "%s#%d", temp_name_read, bmh.dflags & DBM_NUM_FRAMES );
			else
				strcpy( temp_name, temp_name_read );
	
			//Make sure name matches
			if (strcmp(temp_name,AllBitmaps[i].name)) {
				//Int3();       //this pig is out of date.  Delete it
				must_rewrite_pig=1;
			}
	
			strcpy(AllBitmaps[i].name,temp_name);

			memset( &temp_bitmap, 0, sizeof(grs_bitmap) );
	
			temp_bitmap.bm_w = temp_bitmap.bm_rowsize = bmh.width + ((short) (bmh.wh_extra&0x0f)<<8);
			temp_bitmap.bm_h = bmh.height + ((short) (bmh.wh_extra&0xf0)<<4);
			temp_bitmap.bm_flags = BM_FLAG_PAGED_OUT;
			temp_bitmap.avg_color = bmh.avg_color;
			temp_bitmap.bm_data = Piggy_bitmap_cache_data;

			GameBitmapFlags[i] = bmh.flags & BM_FLAGS_TO_COPY;
	
			GameBitmapOffset[i] = bmh.offset + data_start;
	
			GameBitmaps[i] = temp_bitmap;
		}
	}
	else
		N_bitmaps = 0;          //no pigfile, so no bitmaps

	#ifndef EDITOR

	Assert(N_bitmaps == Num_bitmap_files-1);

	#else

	if (must_rewrite_pig || (N_bitmaps < Num_bitmap_files-1)) {
		int size;

		//re-read the bitmaps that aren't in this pig

		for (i=N_bitmaps+1;i<Num_bitmap_files;i++) {
			char *p;

			p = strchr(AllBitmaps[i].name,'#');

			if (p) {   // this is an ABM == animated bitmap
				char abmname[FILENAME_LEN];
				int fnum;
				grs_bitmap * bm[MAX_BITMAPS_PER_BRUSH];
				int iff_error;          //reference parm to avoid warning message
				ubyte newpal[768];
				char basename[FILENAME_LEN];
				int nframes;
			
				strcpy(basename,AllBitmaps[i].name);
				basename[p-AllBitmaps[i].name] = 0;  //cut off "#nn" part
				
				sprintf( abmname, "%s.abm", basename );

				iff_error = iff_read_animbrush(abmname,bm,MAX_BITMAPS_PER_BRUSH,&nframes,newpal);

				if (iff_error != IFF_NO_ERROR)  {
					mprintf((1,"File %s - IFF error: %s",abmname,iff_errormsg(iff_error)));
					Error("File %s - IFF error: %s",abmname,iff_errormsg(iff_error));
				}
			
				for (fnum=0;fnum<nframes; fnum++)       {
					char tempname[20];
					int SuperX;

					sprintf( tempname, "%s#%d", basename, fnum );

					//SuperX = (GameBitmaps[i+fnum].bm_flags&BM_FLAG_SUPER_TRANSPARENT)?254:-1;
					SuperX = (GameBitmapFlags[i+fnum]&BM_FLAG_SUPER_TRANSPARENT)?254:-1;
					//above makes assumption that supertransparent color is 254

					if ( iff_has_transparency )
						gr_remap_bitmap_good( bm[fnum], newpal, iff_transparent_color, SuperX );
					else
						gr_remap_bitmap_good( bm[fnum], newpal, -1, SuperX );

					bm[fnum]->avg_color = compute_average_pixel(bm[fnum]);

#ifdef EDITOR
					if ( FindArg("-macdata") )
						swap_0_255( bm[fnum] );
#endif
					if ( !BigPig ) gr_bitmap_rle_compress( bm[fnum] );

					if (bm[fnum]->bm_flags & BM_FLAG_RLE)
						size = *((int *) bm[fnum]->bm_data);
					else
						size = bm[fnum]->bm_w * bm[fnum]->bm_h;

					memcpy( &Piggy_bitmap_cache_data[Piggy_bitmap_cache_next],bm[fnum]->bm_data,size);
					d_free(bm[fnum]->bm_data);
					bm[fnum]->bm_data = &Piggy_bitmap_cache_data[Piggy_bitmap_cache_next];
					Piggy_bitmap_cache_next += size;

					GameBitmaps[i+fnum] = *bm[fnum];

					// -- mprintf( (0, "U" ));
					d_free( bm[fnum] );
				}

				i += nframes-1;         //filled in multiple bitmaps
			}
			else {          //this is a BBM

				grs_bitmap * new;
				ubyte newpal[256*3];
				int iff_error;
				char bbmname[FILENAME_LEN];
				int SuperX;

				MALLOC( new, grs_bitmap, 1 );

				sprintf( bbmname, "%s.bbm", AllBitmaps[i].name );
				iff_error = iff_read_bitmap(bbmname,new,BM_LINEAR,newpal);

				new->bm_handle=0;
				if (iff_error != IFF_NO_ERROR)          {
					mprintf((1, "File %s - IFF error: %s",bbmname,iff_errormsg(iff_error)));
					Error("File %s - IFF error: %s",bbmname,iff_errormsg(iff_error));
				}

				SuperX = (GameBitmapFlags[i]&BM_FLAG_SUPER_TRANSPARENT)?254:-1;
				//above makes assumption that supertransparent color is 254

				if ( iff_has_transparency )
					gr_remap_bitmap_good( new, newpal, iff_transparent_color, SuperX );
				else
					gr_remap_bitmap_good( new, newpal, -1, SuperX );

				new->avg_color = compute_average_pixel(new);

#ifdef EDITOR
				if ( FindArg("-macdata") )
					swap_0_255( new );
#endif
				if ( !BigPig )  gr_bitmap_rle_compress( new );

				if (new->bm_flags & BM_FLAG_RLE)
					size = *((int *) new->bm_data);
				else
					size = new->bm_w * new->bm_h;

				memcpy( &Piggy_bitmap_cache_data[Piggy_bitmap_cache_next],new->bm_data,size);
				d_free(new->bm_data);
				new->bm_data = &Piggy_bitmap_cache_data[Piggy_bitmap_cache_next];
				Piggy_bitmap_cache_next += size;

				GameBitmaps[i] = *new;
	
				d_free( new );

				// -- mprintf( (0, "U" ));
			}
		}

		//@@Dont' do these things which are done when writing
		//@@for (i=0; i < Num_bitmap_files; i++ )       {
		//@@    bitmap_index bi;
		//@@    bi.index = i;
		//@@    PIGGY_PAGE_IN( bi );
		//@@}
		//@@
		//@@piggy_close_file();

		piggy_write_pigfile(pigname);

		Current_pigfile[0] = 0;                 //say no pig, to force reload

		piggy_new_pigfile(pigname);             //read in just-generated pig


	}
	#endif  //ifdef EDITOR

}

ubyte bogus_data[64*64];
grs_bitmap bogus_bitmap;
ubyte bogus_bitmap_initialized=0;
digi_sound bogus_sound;

#define HAMFILE_ID              MAKE_SIG('!','M','A','H') //HAM!
#define HAMFILE_VERSION 3
//version 1 -> 2:  save marker_model_num
//version 2 -> 3:  removed sound files

#define SNDFILE_ID              MAKE_SIG('D','N','S','D') //DSND
#define SNDFILE_VERSION 1

int read_hamfile()
{
	CFILE * ham_fp = NULL;
	int ham_id;
	int sound_offset = 0;
	#ifdef MACINTOSH
	char name[255];
	#endif

	#ifndef MACINTOSH
	ham_fp = cfopen( DEFAULT_HAMFILE, "rb" );
	#else
	sprintf(name, ":Data:%s", DEFAULT_HAMFILE );
	ham_fp = cfopen( name, "rb" );
	#endif

	if (ham_fp == NULL) {
		Must_write_hamfile = 1;
		return 0;
	}

	//make sure ham is valid type file & is up-to-date
	ham_id = cfile_read_int(ham_fp);
	Piggy_hamfile_version = cfile_read_int(ham_fp);
	if (ham_id != HAMFILE_ID)
		Error("Cannot open ham file %s\n", DEFAULT_HAMFILE);
#if 0
	if (ham_id != HAMFILE_ID || Piggy_hamfile_version != HAMFILE_VERSION) {
		Must_write_hamfile = 1;
		cfclose(ham_fp);						//out of date ham
		return 0;
	}
#endif

	if (Piggy_hamfile_version < 3) // hamfile contains sound info
		sound_offset = cfile_read_int(ham_fp);

	#ifndef EDITOR
	{
		//int i;

		bm_read_all(ham_fp);
		cfread( GameBitmapXlat, sizeof(ushort)*MAX_BITMAP_FILES, 1, ham_fp );
		// no swap here?
		//for (i = 0; i < MAX_BITMAP_FILES; i++) {
			//GameBitmapXlat[i] = INTEL_SHORT(GameBitmapXlat[i]);
			//printf("GameBitmapXlat[%d] = %d\n", i, GameBitmapXlat[i]);
		//}
	}
	#endif

	if (Piggy_hamfile_version < 3) {
		int N_sounds;
		int sound_start;
		int header_size;
		int i;
		DiskSoundHeader sndh;
		digi_sound temp_sound;
		char temp_name_read[16];
		int sbytes = 0;

		cfseek(ham_fp, sound_offset, SEEK_SET);
		N_sounds = cfile_read_int(ham_fp);

		sound_start = cftell(ham_fp);

		header_size = N_sounds * sizeof(DiskSoundHeader);

		//Read sounds

		for (i=0; i<N_sounds; i++ ) {
			DiskSoundHeader_read(&sndh, ham_fp);
			temp_sound.length = sndh.length;
			temp_sound.data = (ubyte *)(sndh.offset + header_size + sound_start);
			SoundOffset[Num_sound_files] = sndh.offset + header_size + sound_start;
			memcpy( temp_name_read, sndh.name, 8 );
			temp_name_read[8] = 0;
			piggy_register_sound( &temp_sound, temp_name_read, 1 );
#ifdef MACINTOSH
			if (piggy_is_needed(i))
#endif		// note link to if.
				sbytes += sndh.length;
			//mprintf(( 0, "%d bytes of sound\n", sbytes ));
		}

		SoundBits = d_malloc( sbytes + 16 );
		if ( SoundBits == NULL )
			Error( "Not enough memory to load sounds\n" );

		mprintf(( 0, "\nBitmaps: %d KB   Sounds: %d KB\n", Piggy_bitmap_cache_size/1024, sbytes/1024 ));

		//	piggy_read_sounds(ham_fp);

	}

	cfclose(ham_fp);

	return 1;

}

int read_sndfile()
{
	CFILE * snd_fp = NULL;
	int snd_id,snd_version;
	int N_sounds;
	int sound_start;
	int header_size;
	int i,size, length;
	DiskSoundHeader sndh;
	digi_sound temp_sound;
	char temp_name_read[16];
	int sbytes = 0;
	#ifdef MACINTOSH
	char name[255];
	#endif

	#ifndef MACINTOSH
	snd_fp = cfopen( DEFAULT_SNDFILE, "rb" );
	#else
	sprintf( name, ":Data:%s", DEFAULT_SNDFILE );
	snd_fp = cfopen( name, "rb");
	#endif
	
	if (snd_fp == NULL)
		return 0;

	//make sure soundfile is valid type file & is up-to-date
	snd_id = cfile_read_int(snd_fp);
	snd_version = cfile_read_int(snd_fp);
	if (snd_id != SNDFILE_ID || snd_version != SNDFILE_VERSION) {
		cfclose(snd_fp);						//out of date sound file
		return 0;
	}

	N_sounds = cfile_read_int(snd_fp);

	sound_start = cftell(snd_fp);
	size = cfilelength(snd_fp) - sound_start;
	length = size;
	mprintf( (0, "\nReading data (%d KB) ", size/1024 ));

	header_size = N_sounds*sizeof(DiskSoundHeader);

	//Read sounds

	for (i=0; i<N_sounds; i++ ) {
		DiskSoundHeader_read(&sndh, snd_fp);
		//size -= sizeof(DiskSoundHeader);
		temp_sound.length = sndh.length;
		temp_sound.data = (ubyte *)(sndh.offset + header_size + sound_start);
		SoundOffset[Num_sound_files] = sndh.offset + header_size + sound_start;
		memcpy( temp_name_read, sndh.name, 8 );
		temp_name_read[8] = 0;
		piggy_register_sound( &temp_sound, temp_name_read, 1 );
		#ifdef MACINTOSH
		if (piggy_is_needed(i))
		#endif		// note link to if.
		sbytes += sndh.length;
		//mprintf(( 0, "%d bytes of sound\n", sbytes ));
	}

	SoundBits = d_malloc( sbytes + 16 );
	if ( SoundBits == NULL )
		Error( "Not enough memory to load sounds\n" );

	mprintf(( 0, "\nBitmaps: %d KB   Sounds: %d KB\n", Piggy_bitmap_cache_size/1024, sbytes/1024 ));

//	piggy_read_sounds(snd_fp);

	cfclose(snd_fp);

	return 1;
}

int piggy_init(void)
{
	int ham_ok=0,snd_ok=0;
	int i;

	hashtable_init( &AllBitmapsNames, MAX_BITMAP_FILES );
	hashtable_init( &AllDigiSndNames, MAX_SOUND_FILES );

	for (i=0; i<MAX_SOUND_FILES; i++ )	{
		GameSounds[i].length = 0;
		GameSounds[i].data = NULL;
		SoundOffset[i] = 0;
	}

	for (i=0; i<MAX_BITMAP_FILES; i++ )     
		GameBitmapXlat[i] = i;

	if ( !bogus_bitmap_initialized )        {
		int i;
		ubyte c;
		bogus_bitmap_initialized = 1;
		memset( &bogus_bitmap, 0, sizeof(grs_bitmap) );
		bogus_bitmap.bm_w = bogus_bitmap.bm_h = bogus_bitmap.bm_rowsize = 64;
		bogus_bitmap.bm_data = bogus_data;
		c = gr_find_closest_color( 0, 0, 63 );
		for (i=0; i<4096; i++ ) bogus_data[i] = c;
		c = gr_find_closest_color( 63, 0, 0 );
		// Make a big red X !
		for (i=0; i<64; i++ )   {
			bogus_data[i*64+i] = c;
			bogus_data[i*64+(63-i)] = c;
		}
		piggy_register_bitmap( &bogus_bitmap, "bogus", 1 );
		bogus_sound.length = 64*64;
		bogus_sound.data = bogus_data;
		GameBitmapOffset[0] = 0;
	}

	if ( FindArg( "-bigpig" ))
		BigPig = 1;

	if ( FindArg( "-lowmem" ))
		piggy_low_memory = 1;

	if ( FindArg( "-nolowmem" ))
		piggy_low_memory = 0;

	if (piggy_low_memory)
		digi_lomem = 1;

	WIN(DDGRLOCK(dd_grd_curcanv));
		gr_set_curfont( SMALL_FONT );
		gr_set_fontcolor(gr_find_closest_color_current( 20, 20, 20 ),-1 );
		gr_printf( 0x8000, grd_curcanv->cv_h-20, "%s...", TXT_LOADING_DATA );
	WIN(DDGRUNLOCK(dd_grd_curcanv));

#if 1 //def EDITOR //need for d1 mission briefings
	piggy_init_pigfile(DEFAULT_PIGFILE);
#endif

	snd_ok = ham_ok = read_hamfile();

	if (Piggy_hamfile_version >= 3)
		snd_ok = read_sndfile();

	atexit(piggy_close);

	mprintf ((0,"HamOk=%d SndOk=%d\n",ham_ok,snd_ok));
	return (ham_ok && snd_ok);               //read ok
}

int piggy_is_needed(int soundnum)
{
	int i;

	if ( !digi_lomem ) return 1;

	for (i=0; i<MAX_SOUNDS; i++ )   {
		if ( (AltSounds[i] < 255) && (Sounds[AltSounds[i]] == soundnum) )
			return 1;
	}
	return 0;
}


void piggy_read_sounds(void)
{
	CFILE * fp = NULL;
	ubyte * ptr;
	int i, sbytes;
	#ifdef MACINTOSH
	char name[255];
	#endif

	ptr = SoundBits;
	sbytes = 0;

	#ifndef MACINTOSH
	fp = cfopen( DEFAULT_SNDFILE, "rb" );
	#else
	sprintf( name, ":Data:%s", DEFAULT_SNDFILE );
	fp = cfopen( name, "rb");
	#endif

	if (fp == NULL)
		return;

	for (i=0; i<Num_sound_files; i++ )      {
		digi_sound *snd = &GameSounds[i];

		if ( SoundOffset[i] > 0 )       {
			if ( piggy_is_needed(i) )       {
				cfseek( fp, SoundOffset[i], SEEK_SET );

				// Read in the sound data!!!
				snd->data = ptr;
				ptr += snd->length;
				sbytes += snd->length;
				cfread( snd->data, snd->length, 1, fp );
			}
			else
				snd->data = (ubyte *) -1;
		}
	}

	cfclose(fp);

	mprintf(( 0, "\nActual Sound usage: %d KB\n", sbytes/1024 ));

}


extern int descent_critical_error;
extern unsigned descent_critical_deverror;
extern unsigned descent_critical_errcode;

char * crit_errors[13] = { "Write Protected", "Unknown Unit", "Drive Not Ready", "Unknown Command", "CRC Error", \
"Bad struct length", "Seek Error", "Unknown media type", "Sector not found", "Printer out of paper", "Write Fault", \
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

	if ( piggy_low_memory ) {
		org_i = i;
		i = GameBitmapXlat[i];          // Xlat for low-memory settings!
	}

	bmp = &GameBitmaps[i];

	if ( bmp->bm_flags & BM_FLAG_PAGED_OUT )        {
		stop_time();

	ReDoIt:
		descent_critical_error = 0;
		cfseek( Piggy_fp, GameBitmapOffset[i], SEEK_SET );
		if ( descent_critical_error )   {
			piggy_critical_error();
			goto ReDoIt;
		}

		bmp->bm_data = &Piggy_bitmap_cache_data[Piggy_bitmap_cache_next];
		bmp->bm_flags = GameBitmapFlags[i];

		if ( bmp->bm_flags & BM_FLAG_RLE )      {
			int zsize = 0;
			descent_critical_error = 0;
			zsize = cfile_read_int(Piggy_fp);
			if ( descent_critical_error )   {
				piggy_critical_error();
				goto ReDoIt;
			}

			// GET JOHN NOW IF YOU GET THIS ASSERT!!!
			//Assert( Piggy_bitmap_cache_next+zsize < Piggy_bitmap_cache_size );
			if ( Piggy_bitmap_cache_next+zsize >= Piggy_bitmap_cache_size ) {
				Int3();
				piggy_bitmap_page_out_all();
				goto ReDoIt;
			}
			descent_critical_error = 0;
			temp = cfread( &Piggy_bitmap_cache_data[Piggy_bitmap_cache_next+4], 1, zsize-4, Piggy_fp );
			if ( descent_critical_error )   {
				piggy_critical_error();
				goto ReDoIt;
			}

#ifndef MACDATA
			switch (cfilelength(Piggy_fp)) {
			default:
				if (!FindArg("-macdata"))
					break;
				// otherwise, fall through...
			case MAC_ALIEN1_PIGSIZE:
			case MAC_ALIEN2_PIGSIZE:
			case MAC_FIRE_PIGSIZE:
			case MAC_GROUPA_PIGSIZE:
			case MAC_ICE_PIGSIZE:
			case MAC_WATER_PIGSIZE:
				rle_swap_0_255( bmp );
				memcpy(&zsize, bmp->bm_data, 4);
				break;
			}
#endif

			memcpy( &Piggy_bitmap_cache_data[Piggy_bitmap_cache_next], &zsize, sizeof(int) );
			Piggy_bitmap_cache_next += zsize;
			if ( Piggy_bitmap_cache_next+zsize >= Piggy_bitmap_cache_size ) {
				Int3();
				piggy_bitmap_page_out_all();
				goto ReDoIt;
			}

		} else {
			// GET JOHN NOW IF YOU GET THIS ASSERT!!!
			Assert( Piggy_bitmap_cache_next+(bmp->bm_h*bmp->bm_w) < Piggy_bitmap_cache_size );
			if ( Piggy_bitmap_cache_next+(bmp->bm_h*bmp->bm_w) >= Piggy_bitmap_cache_size ) {
				piggy_bitmap_page_out_all();
				goto ReDoIt;
			}
			descent_critical_error = 0;
			temp = cfread( &Piggy_bitmap_cache_data[Piggy_bitmap_cache_next], 1, bmp->bm_h*bmp->bm_w, Piggy_fp );
			if ( descent_critical_error )   {
				piggy_critical_error();
				goto ReDoIt;
			}
			Piggy_bitmap_cache_next+=bmp->bm_h*bmp->bm_w;

#ifndef MACDATA
			switch (cfilelength(Piggy_fp)) {
			default:
				if (!FindArg("-macdata"))
					break;
				// otherwise, fall through...
			case MAC_ALIEN1_PIGSIZE:
			case MAC_ALIEN2_PIGSIZE:
			case MAC_FIRE_PIGSIZE:
			case MAC_GROUPA_PIGSIZE:
			case MAC_ICE_PIGSIZE:
			case MAC_WATER_PIGSIZE:
				swap_0_255( bmp );
				break;
			}
#endif
		}

		//@@if ( bmp->bm_selector ) {
		//@@#if !defined(WINDOWS) && !defined(MACINTOSH)
		//@@	if (!dpmi_modify_selector_base( bmp->bm_selector, bmp->bm_data ))
		//@@		Error( "Error modifying selector base in piggy.c\n" );
		//@@#endif
		//@@}

		start_time();
	}

	if ( piggy_low_memory ) {
		if ( org_i != i )
			GameBitmaps[org_i] = GameBitmaps[i];
	}

//@@Removed from John's code:
//@@#ifndef WINDOWS
//@@    if ( bmp->bm_selector ) {
//@@            if (!dpmi_modify_selector_base( bmp->bm_selector, bmp->bm_data ))
//@@                    Error( "Error modifying selector base in piggy.c\n" );
//@@    }
//@@#endif

}

void piggy_bitmap_page_out_all()
{
	int i;
	
	Piggy_bitmap_cache_next = 0;

	piggy_page_flushed++;

	texmerge_flush();
	rle_cache_flush();

	for (i=0; i<Num_bitmap_files; i++ )             {
		if ( GameBitmapOffset[i] > 0 )  {       // Don't page out bitmaps read from disk!!!
			GameBitmaps[i].bm_flags = BM_FLAG_PAGED_OUT;
			GameBitmaps[i].bm_data = Piggy_bitmap_cache_data;
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

void change_filename_ext( char *dest, char *src, char *ext );

void piggy_write_pigfile(char *filename)
{
	FILE *pig_fp;
	int bitmap_data_start, data_offset;
	DiskBitmapHeader bmh;
	int org_offset;
	char subst_name[32];
	int i;
	FILE *fp1,*fp2;
	char tname[FILENAME_LEN];

	// -- mprintf( (0, "Paging in all piggy bitmaps..." ));
	for (i=0; i < Num_bitmap_files; i++ )   {
		bitmap_index bi;
		bi.index = i;
		PIGGY_PAGE_IN( bi );
	}
	// -- mprintf( (0, "\n" ));

	piggy_close_file();

	// -- mprintf( (0, "Creating %s...",filename ));

	pig_fp = fopen( filename, "wb" );       //open PIG file
	Assert( pig_fp!=NULL );

	write_int(PIGFILE_ID,pig_fp);
	write_int(PIGFILE_VERSION,pig_fp);

	Num_bitmap_files--;
	fwrite( &Num_bitmap_files, sizeof(int), 1, pig_fp );
	Num_bitmap_files++;

	bitmap_data_start = ftell(pig_fp);
	bitmap_data_start += (Num_bitmap_files - 1) * sizeof(DiskBitmapHeader);
	data_offset = bitmap_data_start;

	change_filename_ext(tname,filename,"lst");
	fp1 = fopen( tname, "wt" );
	change_filename_ext(tname,filename,"all");
	fp2 = fopen( tname, "wt" );

	for (i=1; i < Num_bitmap_files; i++ )   {
		int *size;
		grs_bitmap *bmp;

		{               
			char * p, *p1;
			p = strchr(AllBitmaps[i].name, '#');
			if (p) {   // this is an ABM == animated bitmap
				int n;
				p1 = p; p1++; 
				n = atoi(p1);
				*p = 0;
				if (fp2 && n==0)
					fprintf( fp2, "%s.abm\n", AllBitmaps[i].name );
				memcpy( bmh.name, AllBitmaps[i].name, 8 );
				Assert( n <= DBM_NUM_FRAMES );
				bmh.dflags = DBM_FLAG_ABM + n;
				*p = '#';
			} else {
				if (fp2)
					fprintf( fp2, "%s.bbm\n", AllBitmaps[i].name );
				memcpy( bmh.name, AllBitmaps[i].name, 8 );
				bmh.dflags = 0;
			}
		}
		bmp = &GameBitmaps[i];

		Assert( !(bmp->bm_flags&BM_FLAG_PAGED_OUT) );

		if (fp1)
			fprintf( fp1, "BMP: %s, size %d bytes", AllBitmaps[i].name, bmp->bm_rowsize * bmp->bm_h );
		org_offset = ftell(pig_fp);
		bmh.offset = data_offset - bitmap_data_start;
		fseek( pig_fp, data_offset, SEEK_SET );

		if ( bmp->bm_flags & BM_FLAG_RLE )      {
			size = (int *)bmp->bm_data;
			fwrite( bmp->bm_data, sizeof(ubyte), *size, pig_fp );
			data_offset += *size;
			if (fp1)
				fprintf( fp1, ", and is already compressed to %d bytes.\n", *size );
		} else {
			fwrite( bmp->bm_data, sizeof(ubyte), bmp->bm_rowsize * bmp->bm_h, pig_fp );
			data_offset += bmp->bm_rowsize * bmp->bm_h;
			if (fp1)
				fprintf( fp1, ".\n" );
		}
		fseek( pig_fp, org_offset, SEEK_SET );
		Assert( GameBitmaps[i].bm_w < 4096 );
		bmh.width = (GameBitmaps[i].bm_w & 0xff);
		bmh.wh_extra = ((GameBitmaps[i].bm_w >> 8) & 0x0f);
		Assert( GameBitmaps[i].bm_h < 4096 );
		bmh.height = GameBitmaps[i].bm_h;
		bmh.wh_extra |= ((GameBitmaps[i].bm_h >> 4) & 0xf0);
		bmh.flags = GameBitmaps[i].bm_flags;
		if (piggy_is_substitutable_bitmap( AllBitmaps[i].name, subst_name ))    {
			bitmap_index other_bitmap;
			other_bitmap = piggy_find_bitmap( subst_name );
			GameBitmapXlat[i] = other_bitmap.index;
			bmh.flags |= BM_FLAG_PAGED_OUT;
			//mprintf(( 0, "Skipping bitmap %d\n", i ));
			//mprintf(( 0, "Marking '%s' as substitutible\n", AllBitmaps[i].name ));
		} else  {
			bmh.flags &= ~BM_FLAG_PAGED_OUT;
		}
		bmh.avg_color=GameBitmaps[i].avg_color;
		fwrite(&bmh, sizeof(DiskBitmapHeader), 1, pig_fp);  // Mark as a bitmap
	}

	fclose(pig_fp);

	mprintf( (0, " Dumped %d assorted bitmaps.\n", Num_bitmap_files ));
	fprintf( fp1, " Dumped %d assorted bitmaps.\n", Num_bitmap_files );

	fclose(fp1);
	fclose(fp2);

}

static void write_int(int i,FILE *file)
{
	if (fwrite( &i, sizeof(i), 1, file) != 1)
		Error( "Error reading int in gamesave.c" );

}

void piggy_dump_all()
{
	int i, xlat_offset;
	FILE * ham_fp;
	int org_offset,data_offset=0;
	DiskSoundHeader sndh;
	int sound_data_start=0;
	FILE *fp1,*fp2;

	#ifdef NO_DUMP_SOUNDS
	Num_sound_files = 0;
	Num_sound_files_new = 0;
	#endif

	if (!Must_write_hamfile && (Num_bitmap_files_new == 0) && (Num_sound_files_new == 0) )
		return;

	fp1 = fopen( "ham.lst", "wt" );
	fp2 = fopen( "ham.all", "wt" );

	if (Must_write_hamfile || Num_bitmap_files_new) {

		mprintf( (0, "Creating %s...",DEFAULT_HAMFILE));
	
		ham_fp = fopen( DEFAULT_HAMFILE, "wb" );                       //open HAM file
		Assert( ham_fp!=NULL );
	
		write_int(HAMFILE_ID,ham_fp);
		write_int(HAMFILE_VERSION,ham_fp);
	
		bm_write_all(ham_fp);
		xlat_offset = ftell(ham_fp);
		fwrite( GameBitmapXlat, sizeof(ushort)*MAX_BITMAP_FILES, 1, ham_fp );
		//Dump bitmaps
	
		if (Num_bitmap_files_new)
			piggy_write_pigfile(DEFAULT_PIGFILE);
	
		//free up memeory used by new bitmaps
		for (i=Num_bitmap_files-Num_bitmap_files_new;i<Num_bitmap_files;i++)
			d_free(GameBitmaps[i].bm_data);
	
		//next thing must be done after pig written
		fseek( ham_fp, xlat_offset, SEEK_SET );
		fwrite( GameBitmapXlat, sizeof(ushort)*MAX_BITMAP_FILES, 1, ham_fp );
	
		fclose(ham_fp);
		mprintf( (0, "\n" ));
	}
	
	if (Num_sound_files_new) {

		mprintf( (0, "Creating %s...",DEFAULT_HAMFILE));
		// Now dump sound file
		ham_fp = fopen( DEFAULT_SNDFILE, "wb" );
		Assert( ham_fp!=NULL );
	
		write_int(SNDFILE_ID,ham_fp);
		write_int(SNDFILE_VERSION,ham_fp);

		fwrite( &Num_sound_files, sizeof(int), 1, ham_fp );
	
		mprintf( (0, "\nDumping sounds..." ));
	
		sound_data_start = ftell(ham_fp);
		sound_data_start += Num_sound_files*sizeof(DiskSoundHeader);
		data_offset = sound_data_start;
	
		for (i=0; i < Num_sound_files; i++ )    {
			digi_sound *snd;
	
			snd = &GameSounds[i];
			strcpy( sndh.name, AllSounds[i].name );
			sndh.length = GameSounds[i].length;
			sndh.offset = data_offset - sound_data_start;
	
			org_offset = ftell(ham_fp);
			fseek( ham_fp, data_offset, SEEK_SET );
	
			sndh.data_length = GameSounds[i].length;
			fwrite( snd->data, sizeof(ubyte), snd->length, ham_fp );
			data_offset += snd->length;
			fseek( ham_fp, org_offset, SEEK_SET );
			fwrite( &sndh, sizeof(DiskSoundHeader), 1, ham_fp );                    // Mark as a bitmap
	
			fprintf( fp1, "SND: %s, size %d bytes\n", AllSounds[i].name, snd->length );
			fprintf( fp2, "%s.raw\n", AllSounds[i].name );
		}

		fclose(ham_fp);
		mprintf( (0, "\n" ));
	}

	fprintf( fp1, "Total sound size: %d bytes\n", data_offset-sound_data_start);
	mprintf( (0, " Dumped %d assorted sounds.\n", Num_sound_files ));
	fprintf( fp1, " Dumped %d assorted sounds.\n", Num_sound_files );

	fclose(fp1);
	fclose(fp2);

	// Never allow the game to run after building ham.
	exit(0);
}

#endif

void piggy_close()
{
	piggy_close_file();

	if (BitmapBits)
		d_free(BitmapBits);

	if ( SoundBits )
		d_free( SoundBits );

	hashtable_free( &AllBitmapsNames );
	hashtable_free( &AllDigiSndNames );

}

int piggy_does_bitmap_exist_slow( char * name )
{
	int i;

	for (i=0; i<Num_bitmap_files; i++ )     {
		if ( !strcmp( AllBitmaps[i].name, name) )
			return 1;
	}
	return 0;
}


#define NUM_GAUGE_BITMAPS 23
char * gauge_bitmap_names[NUM_GAUGE_BITMAPS] = {
	"gauge01", "gauge01b",
	"gauge02", "gauge02b",
	"gauge06", "gauge06b",
	"targ01", "targ01b",
	"targ02", "targ02b", 
	"targ03", "targ03b",
	"targ04", "targ04b",
	"targ05", "targ05b",
	"targ06", "targ06b",
	"gauge18", "gauge18b",
	"gauss1", "helix1",
	"phoenix1"
};


int piggy_is_gauge_bitmap( char * base_name )
{
	int i;
	for (i=0; i<NUM_GAUGE_BITMAPS; i++ )    {
		if ( !stricmp( base_name, gauge_bitmap_names[i] ))      
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
	if ( p )        {
		frame = atoi( &p[1] );
		*p = 0;
		strcpy( base_name, subst_name );
		if ( !piggy_is_gauge_bitmap( base_name ))       {
			sprintf( subst_name, "%s#%d", base_name, frame+1 );
			if ( piggy_does_bitmap_exist_slow( subst_name )  )      {
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



#ifdef WINDOWS
//	New Windows stuff

//	windows bitmap page in
//		Page in a bitmap, if ddraw, then page it into a ddsurface in 
//		'video' memory.  if that fails, page it in normally.

void piggy_bitmap_page_in_w( bitmap_index bitmap, int ddraw )
{
}


//	Essential when switching video modes!

void piggy_bitmap_page_out_all_w()
{
}

#endif // WINDOWS


/*
 * Functions for loading replacement textures
 *  1) From .pog files
 *  2) From descent.pig (for loading d1 levels)
 */

extern void change_filename_extension( char *dest, char *src, char *new_ext );
extern char last_palette_loaded_pig[];

void free_bitmap_replacements()
{
	if (Bitmap_replacement_data) {
		d_free(Bitmap_replacement_data);
		Bitmap_replacement_data = NULL;
	}
}

void load_bitmap_replacements(char *level_name)
{
	char ifile_name[FILENAME_LEN];
	CFILE *ifile;
	int i;

	//first, free up data allocated for old bitmaps
	free_bitmap_replacements();

	change_filename_extension(ifile_name, level_name, ".POG" );

	ifile = cfopen(ifile_name,"rb");

	if (ifile) {
		int id,version,n_bitmaps;
		int bitmap_data_size;
		ushort *indices;

		id = cfile_read_int(ifile);
		version = cfile_read_int(ifile);

		if (id != MAKE_SIG('G','O','P','D') || version != 1) {
			cfclose(ifile);
			return;
		}

		n_bitmaps = cfile_read_int(ifile);

		MALLOC( indices, ushort, n_bitmaps );

		for (i = 0; i < n_bitmaps; i++)
			indices[i] = cfile_read_short(ifile);

		bitmap_data_size = cfilelength(ifile) - cftell(ifile) - sizeof(DiskBitmapHeader) * n_bitmaps;
		MALLOC( Bitmap_replacement_data, ubyte, bitmap_data_size );

		for (i=0;i<n_bitmaps;i++) {
			DiskBitmapHeader bmh;
			grs_bitmap temp_bitmap;

			DiskBitmapHeader_read(&bmh, ifile);

			memset( &temp_bitmap, 0, sizeof(grs_bitmap) );

			temp_bitmap.bm_w = temp_bitmap.bm_rowsize = bmh.width + ((short) (bmh.wh_extra&0x0f)<<8);
			temp_bitmap.bm_h = bmh.height + ((short) (bmh.wh_extra&0xf0)<<4);
			temp_bitmap.avg_color = bmh.avg_color;
			temp_bitmap.bm_data = Bitmap_replacement_data + bmh.offset;

			temp_bitmap.bm_flags |= bmh.flags & BM_FLAGS_TO_COPY;

			GameBitmaps[indices[i]] = temp_bitmap;
			// don't we need the following? GameBitmapOffset[indices[i]] = 0; // don't try to read bitmap from current pigfile
		}

		cfread(Bitmap_replacement_data,1,bitmap_data_size,ifile);

		d_free(indices);

		cfclose(ifile);

		last_palette_loaded_pig[0]= 0;  //force pig re-load

		texmerge_flush();       //for re-merging with new textures
	}

	atexit(free_bitmap_replacements);
}

/* calculate table to translate d1 bitmaps to current palette,
 * return -1 on error
 */
int get_d1_colormap( ubyte *d1_palette, ubyte *colormap )
{
	int freq[256];
	CFILE * palette_file = cfopen(D1_PALETTE, "rb");
	if (!palette_file || cfilelength(palette_file) != 9472)
		return -1;
	cfread( d1_palette, 256, 3, palette_file);
	cfclose( palette_file );
	build_colormap_good( d1_palette, colormap, freq );
	// don't change transparencies:
	colormap[254] = 254;
	colormap[255] = 255;
	return 0;
}

#define JUST_IN_CASE 132 /* is enough for d1 pc registered */
void bitmap_read_d1( grs_bitmap *bitmap, /* read into this bitmap */
                     CFILE *d1_Piggy_fp, /* read from this file */
                     int bitmap_data_start, /* specific to file */
                     DiskBitmapHeader *bmh, /* header info for bitmap */
                     ubyte **next_bitmap, /* where to write it (if 0, use malloc) */
		     ubyte *d1_palette, /* what palette the bitmap has */
                     ubyte *colormap) /* how to translate bitmap's colors */
{
	int zsize;
	memset( bitmap, 0, sizeof(grs_bitmap) );

	bitmap->bm_w = bitmap->bm_rowsize = bmh->width + ((short) (bmh->wh_extra&0x0f)<<8);
	bitmap->bm_h = bmh->height + ((short) (bmh->wh_extra&0xf0)<<4);
	bitmap->avg_color = bmh->avg_color;
	bitmap->bm_flags |= bmh->flags & BM_FLAGS_TO_COPY;

	cfseek(d1_Piggy_fp, bitmap_data_start + bmh->offset, SEEK_SET);
	if (bmh->flags & BM_FLAG_RLE) {
		zsize = cfile_read_int(d1_Piggy_fp);
		cfseek(d1_Piggy_fp, -4, SEEK_CUR);
	} else
		zsize = bitmap->bm_h * bitmap->bm_w;

	if (next_bitmap)
		bitmap->bm_data = d_malloc(zsize + JUST_IN_CASE);
	else {
		bitmap->bm_data = *next_bitmap;
		*next_bitmap += zsize;
	}
	cfread(bitmap->bm_data, 1, zsize, d1_Piggy_fp);
	switch(cfilelength(d1_Piggy_fp)) {
	case D1_MAC_PIGSIZE:
	case D1_MAC_SHARE_PIGSIZE:
		if (bmh->flags & BM_FLAG_RLE)
			rle_swap_0_255(bitmap);
		else
			swap_0_255(bitmap);
	}
	if (bmh->flags & BM_FLAG_RLE)
		rle_remap(bitmap, colormap);
	else
		gr_remap_bitmap_good(bitmap, d1_palette, TRANSPARENCY_COLOR, -1);
	if (bmh->flags & BM_FLAG_RLE) { // size of bitmap could have changed!
		int new_size = *(int*)bitmap->bm_data;
		if (next_bitmap) {
			Assert( zsize + JUST_IN_CASE >= new_size );
			bitmap->bm_data = d_realloc(bitmap->bm_data, new_size);
			Assert(bitmap->bm_data);
		} else
			*next_bitmap += new_size - zsize;
	}
}

#define D1_MAX_TEXTURES 800
#define D1_MAX_TMAP_NUM 1600 // 1555 in descent.pig PC registered

/* the inverse of the Textures array, for descent 1.
 * "Textures" looks up a d2 bitmap index given a d2 tmap_num
 * "d1_tmap_nums" looks up a d1 tmap_num given a d1 bitmap. "-1" means "None"
 */
short *d1_tmap_nums = NULL;

void free_d1_tmap_nums() {
	if (d1_tmap_nums) {
		d_free(d1_tmap_nums);
		d1_tmap_nums = NULL;
	}
}

void bm_read_d1_tmap_nums(CFILE *d1pig)
{
	int i, d1_index;

	free_d1_tmap_nums();
	cfseek(d1pig, 8, SEEK_SET);
	MALLOC(d1_tmap_nums, short, D1_MAX_TMAP_NUM);
	for (i = 0; i < D1_MAX_TMAP_NUM; i++)
		d1_tmap_nums[i] = -1;
	for (i = 0; i < D1_MAX_TEXTURES; i++) {
		d1_index = cfile_read_short(d1pig);
		Assert(d1_index >= 0 && d1_index < D1_MAX_TMAP_NUM);
		d1_tmap_nums[d1_index] = i;
	}
	atexit(free_d1_tmap_nums);
}

/* If the given d1_index is the index of a bitmap we have to load
 * (because it is unique to descent 1), then returns the d2_index that
 * the given d1_index replaces.
 * Returns -1 if the given d1_index is not unique to descent 1.
 */
short d2_index_for_d1_index(short d1_index)
{
	Assert(d1_index >= 0 && d1_index < D1_MAX_TMAP_NUM);
	if (! d1_tmap_nums || d1_tmap_nums[d1_index] == -1
	    || ! d1_tmap_num_unique(d1_tmap_nums[d1_index]))
  		return -1;
	else
		return Textures[convert_d1_tmap_num(d1_tmap_nums[d1_index])].index;
}

#define D1_BITMAPS_SIZE 300000
void load_d1_bitmap_replacements()
{
	CFILE * d1_Piggy_fp;
	DiskBitmapHeader bmh;
	int pig_data_start, bitmap_header_start, bitmap_data_start;
	int N_bitmaps;
	short d1_index, d2_index;
	ubyte* next_bitmap;
	ubyte colormap[256];
	ubyte d1_palette[256*3];
	char *p;

	d1_Piggy_fp = cfopen( D1_PIGFILE, "rb" );

#define D1_PIG_LOAD_FAILED "Failed loading " D1_PIGFILE
	if (!d1_Piggy_fp) {
		Warning(D1_PIG_LOAD_FAILED);
		return;
	}

	//first, free up data allocated for old bitmaps
	free_bitmap_replacements();

	Assert( get_d1_colormap( d1_palette, colormap ) == 0 );

	switch (cfilelength(d1_Piggy_fp)) {
	case D1_SHARE_BIG_PIGSIZE:
	case D1_SHARE_10_PIGSIZE:
	case D1_SHARE_PIGSIZE:
	case D1_10_BIG_PIGSIZE:
	case D1_10_PIGSIZE:
		pig_data_start = 0;
		Warning(D1_PIG_LOAD_FAILED ". descent.pig of v1.0 and all PC shareware versions not supported.");
		return;
		break;
	default:
		Warning("Unknown size for " D1_PIGFILE);
		Int3();
		// fall through
	case D1_PIGSIZE:
	case D1_OEM_PIGSIZE:
	case D1_MAC_PIGSIZE:
	case D1_MAC_SHARE_PIGSIZE:
		pig_data_start = cfile_read_int(d1_Piggy_fp );
		bm_read_d1_tmap_nums(d1_Piggy_fp); //was: bm_read_all_d1(fp);
		//for (i = 0; i < 1800; i++) GameBitmapXlat[i] = cfile_read_short(d1_Piggy_fp);
		break;
	}

	cfseek( d1_Piggy_fp, pig_data_start, SEEK_SET );
	N_bitmaps = cfile_read_int(d1_Piggy_fp);
	{
		int N_sounds = cfile_read_int(d1_Piggy_fp);
		int header_size = N_bitmaps * DISKBITMAPHEADER_D1_SIZE
			+ N_sounds * sizeof(DiskSoundHeader);
		bitmap_header_start = pig_data_start + 2 * sizeof(int);
		bitmap_data_start = bitmap_header_start + header_size;
	}

	MALLOC( Bitmap_replacement_data, ubyte, D1_BITMAPS_SIZE);
	if (!Bitmap_replacement_data) {
		Warning(D1_PIG_LOAD_FAILED);
		return;
	}
	atexit(free_bitmap_replacements);

	next_bitmap = Bitmap_replacement_data;

	for (d1_index = 1; d1_index <= N_bitmaps; d1_index++ ) {
		d2_index = d2_index_for_d1_index(d1_index);
		// only change bitmaps which are unique to d1
		if (d2_index != -1) {
			cfseek(d1_Piggy_fp, bitmap_header_start + (d1_index-1) * DISKBITMAPHEADER_D1_SIZE, SEEK_SET);
			DiskBitmapHeader_d1_read(&bmh, d1_Piggy_fp);

			bitmap_read_d1( &GameBitmaps[d2_index], d1_Piggy_fp, bitmap_data_start, &bmh, &next_bitmap, d1_palette, colormap );
			Assert(next_bitmap - Bitmap_replacement_data < D1_BITMAPS_SIZE);
			GameBitmapOffset[d2_index] = 0; // don't try to read bitmap from current d2 pigfile
			GameBitmapFlags[d2_index] = bmh.flags;

			if ( (p = strchr(AllBitmaps[d2_index].name, '#')) /* d2 BM is animated */
			     && !(bmh.dflags & DBM_FLAG_ABM) ) { /* d1 bitmap is not animated */
				int i, len = p - AllBitmaps[d2_index].name;
				for (i = 0; i < Num_bitmap_files; i++)
					if (i != d2_index && ! memcmp(AllBitmaps[d2_index].name, AllBitmaps[i].name, len)) {
						GameBitmaps[i] = GameBitmaps[d2_index];
						GameBitmapOffset[i] = 0;
						GameBitmapFlags[i] = bmh.flags;
					}
			}
		}
	}

	cfclose(d1_Piggy_fp);

	last_palette_loaded_pig[0]= 0;  //force pig re-load

	texmerge_flush();       //for re-merging with new textures
}


extern int extra_bitmap_num;

/*
 * Find and load the named bitmap from descent.pig
 * similar to read_extra_bitmap_iff
 */
bitmap_index read_extra_bitmap_d1_pig(char *name)
{
	bitmap_index bitmap_num;
	grs_bitmap * new = &GameBitmaps[extra_bitmap_num];

	bitmap_num.index = 0;

	{
		CFILE *d1_Piggy_fp;
		DiskBitmapHeader bmh;
		int pig_data_start, bitmap_header_start, bitmap_data_start;
		int i, N_bitmaps;
		ubyte colormap[256];
		ubyte d1_palette[256*3];

		d1_Piggy_fp = cfopen(D1_PIGFILE, "rb");

		if (!d1_Piggy_fp)
		{
			Warning(D1_PIG_LOAD_FAILED);
			return bitmap_num;
		}

		Assert( get_d1_colormap( d1_palette, colormap ) == 0 );

		switch (cfilelength(d1_Piggy_fp)) {
		case D1_SHARE_BIG_PIGSIZE:
		case D1_SHARE_10_PIGSIZE:
		case D1_SHARE_PIGSIZE:
		case D1_10_BIG_PIGSIZE:
		case D1_10_PIGSIZE:
			pig_data_start = 0;
			break;
		default:
			Warning("Unknown size for " D1_PIGFILE);
			Int3();
			// fall through
		case D1_PIGSIZE:
		case D1_OEM_PIGSIZE:
		case D1_MAC_PIGSIZE:
		case D1_MAC_SHARE_PIGSIZE:
			pig_data_start = cfile_read_int(d1_Piggy_fp );

			break;
		}

		cfseek( d1_Piggy_fp, pig_data_start, SEEK_SET );
		N_bitmaps = cfile_read_int(d1_Piggy_fp);
		{
			int N_sounds = cfile_read_int(d1_Piggy_fp);
			int header_size = N_bitmaps * DISKBITMAPHEADER_D1_SIZE
				+ N_sounds * sizeof(DiskSoundHeader);
			bitmap_header_start = pig_data_start + 2 * sizeof(int);
			bitmap_data_start = bitmap_header_start + header_size;
		}

		for (i = 1; i <= N_bitmaps; i++)
		{
			DiskBitmapHeader_d1_read(&bmh, d1_Piggy_fp);
			if (!strnicmp(bmh.name, name, 8))
				break;
		}

		if (strnicmp(bmh.name, name, 8))
		{
			con_printf(CON_DEBUG, "could not find bitmap %s\n", name);
			return bitmap_num;
		}

		bitmap_read_d1( new, d1_Piggy_fp, bitmap_data_start, &bmh, 0, d1_palette, colormap );

		cfclose(d1_Piggy_fp);
	}

	new->avg_color = 0;	//compute_average_pixel(new);

	bitmap_num.index = extra_bitmap_num;

	GameBitmaps[extra_bitmap_num++] = *new;

	return bitmap_num;
}


#ifndef FAST_FILE_IO
/*
 * reads a bitmap_index structure from a CFILE
 */
void bitmap_index_read(bitmap_index *bi, CFILE *fp)
{
	bi->index = cfile_read_short(fp);
}

/*
 * reads n bitmap_index structs from a CFILE
 */
int bitmap_index_read_n(bitmap_index *bi, int n, CFILE *fp)
{
	int i;

	for (i = 0; i < n; i++)
		bi[i].index = cfile_read_short(fp);
	return i;
}
#endif // FAST_FILE_IO
