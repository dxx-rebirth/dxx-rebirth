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
 * Functions for managing the pig files.
 *
 */


#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>

#include "pstypes.h"
#include "inferno.h"
#include "gr.h"
#include "u_mem.h"
#include "dxxerror.h"
#include "sounds.h"
#include "bm.h"
#include "hash.h"
#include "args.h"
#include "palette.h"
#include "gamefont.h"
#include "rle.h"
#include "screens.h"
#include "snddecom.h"
#include "console.h"
#include "piggy.h"
#include "texmerge.h"
#include "paging.h"
#include "game.h"
#include "text.h"
#include "newmenu.h"
#include "custom.h"

#define DEFAULT_PIGFILE_REGISTERED      "descent.pig"


#ifdef EDITOR
static int piggy_is_substitutable_bitmap( char * name, char * subst_name );
#endif

ubyte *BitmapBits = NULL;
ubyte *SoundBits = NULL;

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
#define PIGGY_SMALL_BUFFER_SIZE (1400*1024)		// size of buffer when GameArg.SysLowMem is set

int piggy_page_flushed = 0;

PHYSFS_file * Piggy_fp = NULL;

ubyte bogus_data[64*64];
ubyte bogus_bitmap_initialized=0;
digi_sound bogus_sound;
grs_bitmap bogus_bitmap;
int MacPig = 0;	// using the Macintosh pigfile?
int PCSharePig = 0; // using PC Shareware pigfile?
static int SoundCompressed[ MAX_SOUND_FILES ];

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

/*
 * reads a DiskBitmapHeader structure from a PHYSFS_file
 */
static void DiskBitmapHeader_read(DiskBitmapHeader *dbh, PHYSFS_file *fp)
{
	PHYSFS_read(fp, dbh->name, 8, 1);
	dbh->dflags = PHYSFSX_readByte(fp);
	dbh->width = PHYSFSX_readByte(fp);
	dbh->height = PHYSFSX_readByte(fp);
	dbh->flags = PHYSFSX_readByte(fp);
	dbh->avg_color = PHYSFSX_readByte(fp);
	dbh->offset = PHYSFSX_readInt(fp);
}

/*
 * reads a DiskSoundHeader structure from a PHYSFS_file
 */
static void DiskSoundHeader_read(DiskSoundHeader *dsh, PHYSFS_file *fp)
{
	PHYSFS_read(fp, dsh->name, 8, 1);
	dsh->length = PHYSFSX_readInt(fp);
	dsh->data_length = PHYSFSX_readInt(fp);
	dsh->offset = PHYSFSX_readInt(fp);
}

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

bitmap_index piggy_register_bitmap( grs_bitmap * bmp, const char * name, int in_file )
{
	bitmap_index temp;
	Assert( Num_bitmap_files < MAX_BITMAP_FILES );

	temp.index = Num_bitmap_files;


	if (!in_file) {
		if ( !GameArg.DbgBigPig )  gr_bitmap_rle_compress( bmp );
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

int piggy_register_sound( digi_sound * snd, const char * name, int in_file )
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
	else if (SoundOffset[Num_sound_files] == 0)
		SoundOffset[Num_sound_files] = -1;		// make sure this sound's data is not individually freed

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

static void piggy_close_file()
{
	if ( Piggy_fp ) {
		PHYSFS_close( Piggy_fp );
		Piggy_fp        = NULL;
	}
}

int properties_init()
{
	int sbytes = 0;
	char temp_name_read[16];
	char temp_name[16];
	grs_bitmap temp_bitmap;
	digi_sound temp_sound;
	DiskBitmapHeader bmh;
	DiskSoundHeader sndh;
	int header_size, N_bitmaps, N_sounds;
	int i,size;
	int Pigdata_start;
	int pigsize;
	int retval;

	hashtable_init( &AllBitmapsNames, MAX_BITMAP_FILES );
	hashtable_init( &AllDigiSndNames, MAX_SOUND_FILES );

	
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
	
	Piggy_fp = PHYSFSX_openReadBuffered(DEFAULT_PIGFILE_REGISTERED);
	if (Piggy_fp==NULL)
	{
		if (!PHYSFSX_exists("BITMAPS.TBL",1) && !PHYSFSX_exists("BITMAPS.BIN",1))
			Error("Cannot find " DEFAULT_PIGFILE_REGISTERED " or BITMAPS.TBL");
		return 1;	// need to run gamedata_read_tbl
	}

	pigsize = PHYSFS_fileLength(Piggy_fp);
	switch (pigsize) {
		case D1_SHARE_BIG_PIGSIZE:
		case D1_SHARE_10_PIGSIZE:
		case D1_SHARE_PIGSIZE:
			PCSharePig = 1;
			Pigdata_start = 0;
			break;
		case D1_10_BIG_PIGSIZE:
		case D1_10_PIGSIZE:
			Pigdata_start = 0;
			break;
		default:
			Warning("Unknown size for " DEFAULT_PIGFILE_REGISTERED);
			Int3();
			// fall through
		case D1_MAC_PIGSIZE:
		case D1_MAC_SHARE_PIGSIZE:
			MacPig = 1;
		case D1_PIGSIZE:
		case D1_OEM_PIGSIZE:
			Pigdata_start = PHYSFSX_readInt(Piggy_fp );
			break;
	}
	
	HiresGFXAvailable = MacPig;	// for now at least

	if (PCSharePig)
		retval = PIGGY_PC_SHAREWARE;	// run gamedata_read_tbl in shareware mode
	else if (GameArg.EdiNoBm || (!PHYSFSX_exists("BITMAPS.TBL",1) && !PHYSFSX_exists("BITMAPS.BIN",1)))
	{
		properties_read_cmp( Piggy_fp );	// Note connection to above if!!!
		for (i = 0; i < MAX_BITMAP_FILES; i++)
		{
			GameBitmapXlat[i] = PHYSFSX_readShort(Piggy_fp);
			if (PHYSFS_eof(Piggy_fp))
				break;
		}
		retval = 0;	// don't run gamedata_read_tbl
	}
	else
		retval = 1;	// run gamedata_read_tbl

	PHYSFSX_fseek( Piggy_fp, Pigdata_start, SEEK_SET );
	size = PHYSFS_fileLength(Piggy_fp) - Pigdata_start;

	N_bitmaps = PHYSFSX_readInt(Piggy_fp);
	size -= sizeof(int);
	N_sounds = PHYSFSX_readInt(Piggy_fp);
	size -= sizeof(int);

	header_size = (N_bitmaps*sizeof(DiskBitmapHeader)) + (N_sounds*sizeof(DiskSoundHeader));

	for (i=0; i<N_bitmaps; i++ ) {
		DiskBitmapHeader_read(&bmh, Piggy_fp);
		
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

		if (MacPig)
		{
			// HACK HACK HACK!!!!!
			if (!d_strnicmp(bmh.name, "cockpit", 7) || !d_strnicmp(bmh.name, "status", 6) || !d_strnicmp(bmh.name, "rearview", 8)) {
				temp_bitmap.bm_w = temp_bitmap.bm_rowsize = 640;
				if (GameBitmapFlags[i+1] & BM_FLAG_RLE)
					GameBitmapFlags[i+1] |= BM_FLAG_RLE_BIG;
			}
			if (!d_strnicmp(bmh.name, "cockpit", 7) || !d_strnicmp(bmh.name, "rearview", 8))
				temp_bitmap.bm_h = 480;
		}
		
		piggy_register_bitmap( &temp_bitmap, temp_name, 1 );
	}

	for (i=0; !MacPig && i<N_sounds; i++ ) {
		DiskSoundHeader_read(&sndh, Piggy_fp);
		
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
		if (PCSharePig)
			SoundCompressed[Num_sound_files] = sndh.data_length;
		memcpy( temp_name_read, sndh.name, 8 );
		temp_name_read[8] = 0;
		piggy_register_sound( &temp_sound, temp_name_read, 1 );
                sbytes += sndh.length;
	}

	if (!MacPig)
	{
		MALLOC(SoundBits, ubyte, sbytes + 16 );
		if ( SoundBits == NULL )
			Error( "Not enough memory to load DESCENT.PIG sounds\n");
	}

#if 1	//def EDITOR
	Piggy_bitmap_cache_size	= size - header_size - sbytes + 16;
	Assert( Piggy_bitmap_cache_size > 0 );
#else
	Piggy_bitmap_cache_size = PIGGY_BUFFER_SIZE;
	if (GameArg.SysLowMem)
		Piggy_bitmap_cache_size = PIGGY_SMALL_BUFFER_SIZE;
#endif
	MALLOC(BitmapBits, ubyte, Piggy_bitmap_cache_size );
	if ( BitmapBits == NULL )
		Error( "Not enough memory to load DESCENT.PIG bitmaps\n" );
	Piggy_bitmap_cache_data = BitmapBits;
	Piggy_bitmap_cache_next = 0;

	return retval;
}

static int piggy_is_needed(int soundnum)
{
	int i;

	if ( !GameArg.SysLowMem ) return 1;

	for (i=0; i<MAX_SOUNDS; i++ ) {
		if ( (AltSounds[i] < 255) && (Sounds[AltSounds[i]] == soundnum) )
			return 1;
	}
	return 0;
}

void piggy_read_sounds(int pc_shareware)
{
	ubyte * ptr;
	int i, sbytes;
	int lastsize = 0;
	ubyte * lastbuf = NULL;

	if (MacPig)
	{
		// Read Mac sounds converted to RAW format (too messy to read them directly from the resource fork code-wise)
		char soundfile[32] = "Sounds/sounds.array";
		PHYSFS_file *array = PHYSFSX_openReadBuffered(soundfile);	// hack for Mac Demo

		if (!array && (PHYSFSX_fsize(DEFAULT_PIGFILE_REGISTERED) == D1_MAC_SHARE_PIGSIZE))
		{
			con_printf(CON_URGENT,"Warning: Missing Sounds/sounds.array for Mac data files");
			return;
		}
		else if (array)
		{
			if (PHYSFS_read(array, Sounds, MAX_SOUNDS, 1) != 1)	// make the 'Sounds' index array match with the sounds we're about to read in
			{
				con_printf(CON_URGENT,"Warning: Can't read Sounds/sounds.array: %s", PHYSFS_getLastError());
				PHYSFS_close(array);
				return;
			}
			PHYSFS_close(array);
		}

		for (i = 0; i < MAX_SOUND_FILES; i++)
		{
			sprintf(soundfile, "SND%04d.raw", i);
			if (ds_load(0, soundfile) == 255)
				break;
		}

		return;
	}

	ptr = SoundBits;
	sbytes = 0;

	for (i=0; i<Num_sound_files; i++ )
	{
		digi_sound *snd = &GameSounds[i];

		if ( SoundOffset[i] > 0 )
		{
			if ( piggy_is_needed(i) )
			{
				PHYSFSX_fseek( Piggy_fp, SoundOffset[i], SEEK_SET );

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
				if (pc_shareware)
				{
					if (lastsize < SoundCompressed[i]) {
						if (lastbuf) d_free(lastbuf);
						MALLOC(lastbuf, ubyte, SoundCompressed[i]);
					}
					PHYSFS_read( Piggy_fp, lastbuf, SoundCompressed[i], 1 );
					sound_decompress( lastbuf, SoundCompressed[i], snd->data );
				}
				else
#ifdef ALLEGRO
					PHYSFS_read( Piggy_fp, snd->data, snd->len, 1 );
#else
					PHYSFS_read( Piggy_fp, snd->data, snd->length, 1 );
#endif
			}
		}
	}
	if (lastbuf)
	  d_free(lastbuf);
}

void piggy_bitmap_page_in( bitmap_index bitmap )
{
	grs_bitmap * bmp;
	int i,org_i;

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

	if ( GameArg.SysLowMem ) {
		org_i = i;
		i = GameBitmapXlat[i];          // Xlat for low-memory settings!
	}
	bmp = &GameBitmaps[i];

	if ( bmp->bm_flags & BM_FLAG_PAGED_OUT ) {
		stop_time();

	ReDoIt:
		PHYSFSX_fseek( Piggy_fp, GameBitmapOffset[i], SEEK_SET );
		gr_set_bitmap_flags (bmp, GameBitmapFlags[i]);
		gr_set_bitmap_data (bmp, &Piggy_bitmap_cache_data [Piggy_bitmap_cache_next]);

		if ( bmp->bm_flags & BM_FLAG_RLE ) {
			int zsize = PHYSFSX_readInt(Piggy_fp);

			// GET JOHN NOW IF YOU GET THIS ASSERT!!!
			Assert( Piggy_bitmap_cache_next+zsize < Piggy_bitmap_cache_size );
			if ( Piggy_bitmap_cache_next+zsize >= Piggy_bitmap_cache_size ) {
				piggy_bitmap_page_out_all();
				goto ReDoIt;
			}
			memcpy( &Piggy_bitmap_cache_data[Piggy_bitmap_cache_next], &zsize, sizeof(int) );
			Piggy_bitmap_cache_next += sizeof(int);
			PHYSFS_read( Piggy_fp, &Piggy_bitmap_cache_data[Piggy_bitmap_cache_next], 1, zsize-4 );
			if (MacPig)
			{
				rle_swap_0_255(bmp);
				memcpy(&zsize, bmp->bm_data, 4);
			}
			Piggy_bitmap_cache_next += zsize-4;
		} else {
			// GET JOHN NOW IF YOU GET THIS ASSERT!!!
			Assert( Piggy_bitmap_cache_next+(bmp->bm_h*bmp->bm_w) < Piggy_bitmap_cache_size );
			if ( Piggy_bitmap_cache_next+(bmp->bm_h*bmp->bm_w) >= Piggy_bitmap_cache_size ) {
				piggy_bitmap_page_out_all();
				goto ReDoIt;
			}
			PHYSFS_read( Piggy_fp, &Piggy_bitmap_cache_data[Piggy_bitmap_cache_next], 1, bmp->bm_h*bmp->bm_w );
			if (MacPig)
				swap_0_255(bmp);
			Piggy_bitmap_cache_next+=bmp->bm_h*bmp->bm_w;
		}

		compute_average_rgb(bmp, bmp->avg_color_rgb);

		start_time();
	}

	if ( GameArg.SysLowMem ) {
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

	for (i=0; i<Num_bitmap_files; i++ ) {
		if ( GameBitmapOffset[i] > 0 ) {	// Don't page out bitmaps read from disk!!!
			GameBitmaps[i].bm_flags = BM_FLAG_PAGED_OUT;
				gr_set_bitmap_data (&GameBitmaps[i], Piggy_bitmap_cache_data);
		}
	}
}

void piggy_load_level_data()
{
	piggy_bitmap_page_out_all();
	paging_touch_all();
}

void piggy_close()
{
	int i;

	custom_close();
	piggy_close_file();

//added ifndef on 10/04/98 by Matt Mueller to fix crash on exit bug -- killed 2000/02/06 since they don't seem to cause crash anymore.  heh.
//#ifndef __LINUX__
	if (BitmapBits)
		d_free(BitmapBits);

	if ( SoundBits )
		d_free( SoundBits );

	for (i = 0; i < Num_sound_files; i++)
		if (SoundOffset[i] == 0)
			d_free(GameSounds[i].data);
//#endif
//end addition -MM
	
	hashtable_free( &AllBitmapsNames );
	hashtable_free( &AllDigiSndNames );
}

#ifdef EDITOR
static int piggy_does_bitmap_exist_slow(const char * name )
{
	int i;

	for (i=0; i<Num_bitmap_files; i++ ) {
		if ( !strcmp( AllBitmaps[i].name, name) )
			return 1;
	}
	return 0;
}


static const char gauge_bitmap_names[][9] = {
	"gauge01",
	"gauge02",
	"gauge06",
	"targ01",
	"targ02",
	"targ03",
	"targ04",
	"targ05",
	"targ06",
	"gauge18",
	"targ01pc",
	"targ02pc",
	"targ03pc",
	"gaug18pc"
};

static int piggy_is_gauge_bitmap(const char * base_name )
{
	for (unsigned i=0; i<sizeof(gauge_bitmap_names)/sizeof(gauge_bitmap_names[0]); i++ ) {
		if ( !d_stricmp( base_name, gauge_bitmap_names[i] ))
			return 1;
	}

	return 0;
}

static int piggy_is_substitutable_bitmap( char * name, char * subst_name )
{
	int frame;
	char * p;
	char base_name[ 16 ];
	
	strcpy( subst_name, name );
	p = strchr( subst_name, '#' );
	if ( p ) {
		frame = atoi( &p[1] );
		*p = 0;
		strcpy( base_name, subst_name );
		if ( !piggy_is_gauge_bitmap( base_name )) {
			sprintf( subst_name, "%s#%d", base_name, frame+1 );
			if ( piggy_does_bitmap_exist_slow( subst_name )  ) {
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
#endif

/*
 * reads a bitmap_index structure from a PHYSFS_file
 */
void bitmap_index_read(bitmap_index *bi, PHYSFS_file *fp)
{
	bi->index = PHYSFSX_readShort(fp);
}

/*
 * reads n bitmap_index structs from a PHYSFS_file
 */
int bitmap_index_read_n(bitmap_index *bi, int n, PHYSFS_file *fp)
{
	int i;

	for (i = 0; i < n; i++)
		bi[i].index = PHYSFSX_readShort(fp);
	return i;
}
