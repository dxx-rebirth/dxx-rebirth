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
 */


#include <stdio.h>
#include <string.h>

#include "pstypes.h"
#include "strutil.h"
#include "inferno.h"
#include "gr.h"
#include "grdef.h"
#include "u_mem.h"
#include "iff.h"
#include "dxxerror.h"
#include "sounds.h"
#include "songs.h"
#include "bm.h"
#include "bmread.h"
#include "hash.h"
#include "args.h"
#include "palette.h"
#include "gamefont.h"
#include "gamepal.h"
#include "rle.h"
#include "screens.h"
#include "piggy.h"
#include "gamemine.h"
#include "textures.h"
#include "texmerge.h"
#include "paging.h"
#include "game.h"
#include "text.h"
#include "newmenu.h"
#include "byteswap.h"
#include "makesig.h"
#include "console.h"

//#define NO_DUMP_SOUNDS        1   //if set, dump bitmaps but not sounds

#define DEFAULT_PIGFILE_REGISTERED      "groupa.pig"
#define DEFAULT_PIGFILE_SHAREWARE       "d2demo.pig"
#define DEFAULT_HAMFILE_REGISTERED      "descent2.ham"
#define DEFAULT_HAMFILE_SHAREWARE       "d2demo.ham"

#define D1_PALETTE "palette.256"

#define DEFAULT_SNDFILE ((Piggy_hamfile_version < 3)?DEFAULT_HAMFILE_SHAREWARE:(GameArg.SndDigiSampleRate==SAMPLE_RATE_22K)?"descent2.s22":"descent2.s11")

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

int Piggy_bitmap_cache_size = 0;
int Piggy_bitmap_cache_next = 0;
ubyte * Piggy_bitmap_cache_data = NULL;
static int GameBitmapOffset[MAX_BITMAP_FILES];
static ubyte GameBitmapFlags[MAX_BITMAP_FILES];
ushort GameBitmapXlat[MAX_BITMAP_FILES];

#define PIGGY_BUFFER_SIZE (2400*1024)
#define PIGGY_SMALL_BUFFER_SIZE (1400*1024)		// size of buffer when GameArg.SysLowMem is set

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
typedef char padding_check[sizeof(DiskBitmapHeader) == 0x12 ? 1 : -1];

#define DISKBITMAPHEADER_D1_SIZE 17 // no wh_extra

typedef struct DiskSoundHeader {
	char name[8];
	int length;
	int data_length;
	int offset;
} __pack__ DiskSoundHeader;

void free_bitmap_replacements();
void free_d1_tmap_nums();

/*
 * reads a DiskBitmapHeader structure from a PHYSFS_file
 */
void DiskBitmapHeader_read(DiskBitmapHeader *dbh, PHYSFS_file *fp)
{
	PHYSFS_read(fp, dbh->name, 8, 1);
	dbh->dflags = PHYSFSX_readByte(fp);
	dbh->width = PHYSFSX_readByte(fp);
	dbh->height = PHYSFSX_readByte(fp);
	dbh->wh_extra = PHYSFSX_readByte(fp);
	dbh->flags = PHYSFSX_readByte(fp);
	dbh->avg_color = PHYSFSX_readByte(fp);
	dbh->offset = PHYSFSX_readInt(fp);
}

/*
 * reads a DiskSoundHeader structure from a PHYSFS_file
 */
void DiskSoundHeader_read(DiskSoundHeader *dsh, PHYSFS_file *fp)
{
	PHYSFS_read(fp, dsh->name, 8, 1);
	dsh->length = PHYSFSX_readInt(fp);
	dsh->data_length = PHYSFSX_readInt(fp);
	dsh->offset = PHYSFSX_readInt(fp);
}

/*
 * reads a descent 1 DiskBitmapHeader structure from a PHYSFS_file
 */
void DiskBitmapHeader_d1_read(DiskBitmapHeader *dbh, PHYSFS_file *fp)
{
	PHYSFS_read(fp, dbh->name, 8, 1);
	dbh->dflags = PHYSFSX_readByte(fp);
	dbh->width = PHYSFSX_readByte(fp);
	dbh->height = PHYSFSX_readByte(fp);
	dbh->wh_extra = 0;
	dbh->flags = PHYSFSX_readByte(fp);
	dbh->avg_color = PHYSFSX_readByte(fp);
	dbh->offset = PHYSFSX_readInt(fp);
}

int piggy_is_substitutable_bitmap( char * name, char * subst_name );

#ifdef EDITOR
void piggy_write_pigfile(char *filename);
static void write_int(int i,PHYSFS_file *file);
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

char* piggy_game_bitmap_name(grs_bitmap *bmp)
{
	if (bmp >= GameBitmaps && bmp < &GameBitmaps[MAX_BITMAP_FILES])
	{
		int i = bmp-GameBitmaps; // i = (bmp - GameBitmaps) / sizeof(grs_bitmap);
		Assert (bmp == &GameBitmaps[i] && i >= 0 && i < MAX_BITMAP_FILES);
		return AllBitmaps[i].name;
	}
	return NULL;
}

bitmap_index piggy_register_bitmap( grs_bitmap * bmp, const char * name, int in_file )
{
	bitmap_index temp;
	Assert( Num_bitmap_files < MAX_BITMAP_FILES );

	temp.index = Num_bitmap_files;

	if (!in_file) {
#ifdef EDITOR
		if ( GameArg.EdiMacData )
			swap_0_255( bmp );
#endif
		if ( !GameArg.DbgBigPig )  gr_bitmap_rle_compress( bmp );
		Num_bitmap_files_new++;
	}
	else if (SoundOffset[Num_sound_files] == 0)
		SoundOffset[Num_sound_files] = -1;		// make sure this sound's data is not individually freed

	strncpy( AllBitmaps[Num_bitmap_files].name, name, 12 );
	hashtable_insert( &AllBitmapsNames, AllBitmaps[Num_bitmap_files].name, Num_bitmap_files );
	//GameBitmaps[Num_bitmap_files] = *bmp;
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

	size_t namelen;
	if ((t=strchr(name,'#'))!=NULL)
		namelen = t - name;
	else
		namelen = strlen(name);

	for (i=0;i<Num_aliases;i++)
		if (alias_list[i].alias_name[namelen] == 0 && d_strnicmp(name,alias_list[i].alias_name,namelen)==0) {
			if (t) {                //extra stuff for ABMs
				static char temp[FILENAME_LEN];
				struct splitpath_t path;
				d_splitpath(alias_list[i].file_name, &path);
				snprintf(temp, sizeof(temp), "%.*s%s\n", (int)(path.base_end - path.base_start), path.base_start, t);
				name = temp;
			}
			else
				name=alias_list[i].file_name; 
			break;
		}

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

PHYSFS_file * Piggy_fp = NULL;

char Current_pigfile[FILENAME_LEN] = "";

void piggy_close_file()
{
	if ( Piggy_fp ) {
		PHYSFS_close( Piggy_fp );
		Piggy_fp        = NULL;
		Current_pigfile[0] = 0;
	}
}

int Pigfile_initialized=0;

#define PIGFILE_ID              MAKE_SIG('G','I','P','P') //PPIG
#define PIGFILE_VERSION         2

extern char CDROM_dir[];

int request_cd(void);


//initialize a pigfile, reading headers
//returns the size of all the bitmap data
void piggy_init_pigfile(char *filename)
{
	int i;
	char temp_name[16];
	char temp_name_read[16];
	DiskBitmapHeader bmh;
	int header_size, N_bitmaps, data_start;
#ifdef EDITOR
	int data_size;
#endif

	piggy_close_file();             //close old pig if still open

	Piggy_fp = PHYSFSX_openReadBuffered(filename);
	
	//try pigfile for shareware
	if (!Piggy_fp)
		Piggy_fp = PHYSFSX_openReadBuffered(DEFAULT_PIGFILE_SHAREWARE);

	if (Piggy_fp) {                         //make sure pig is valid type file & is up-to-date
		int pig_id,pig_version;

		pig_id = PHYSFSX_readInt(Piggy_fp);
		pig_version = PHYSFSX_readInt(Piggy_fp);
		if (pig_id != PIGFILE_ID || pig_version != PIGFILE_VERSION) {
			PHYSFS_close(Piggy_fp);              //out of date pig
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

	N_bitmaps = PHYSFSX_readInt(Piggy_fp);

	header_size = N_bitmaps * sizeof(DiskBitmapHeader);

	data_start = header_size + PHYSFS_tell(Piggy_fp);
#ifdef EDITOR
	data_size = PHYSFS_fileLength(Piggy_fp) - data_start;
#endif
	Num_bitmap_files = 1;

	for (i=0; i<N_bitmaps; i++ )
	{
		int width;
		grs_bitmap *bm = &GameBitmaps[i + 1];
		
		DiskBitmapHeader_read(&bmh, Piggy_fp);
		memcpy( temp_name_read, bmh.name, 8 );
		temp_name_read[8] = 0;
		if ( bmh.dflags & DBM_FLAG_ABM )        
			sprintf( temp_name, "%s#%d", temp_name_read, bmh.dflags & DBM_NUM_FRAMES );
		else
			strcpy( temp_name, temp_name_read );
		width = bmh.width + ((short) (bmh.wh_extra & 0x0f) << 8);
		gr_init_bitmap(bm, 0, 0, 0, width, bmh.height + ((short) (bmh.wh_extra & 0xf0) << 4), width, NULL);
		bm->bm_flags = BM_FLAG_PAGED_OUT;
		bm->avg_color = bmh.avg_color;

		GameBitmapFlags[i+1] = bmh.flags & BM_FLAGS_TO_COPY;

		GameBitmapOffset[i+1] = bmh.offset + data_start;
		Assert( (i+1) == Num_bitmap_files );
		piggy_register_bitmap(bm, temp_name, 1);
	}

#ifdef EDITOR
	Piggy_bitmap_cache_size = data_size + (data_size/10);   //extra mem for new bitmaps
	Assert( Piggy_bitmap_cache_size > 0 );
#else
	Piggy_bitmap_cache_size = PIGGY_BUFFER_SIZE;
	if (GameArg.SysLowMem)
		Piggy_bitmap_cache_size = PIGGY_SMALL_BUFFER_SIZE;
#endif
	BitmapBits = d_malloc( Piggy_bitmap_cache_size );
	if ( BitmapBits == NULL )
		Error( "Not enough memory to load bitmaps\n" );
	Piggy_bitmap_cache_data = BitmapBits;
	Piggy_bitmap_cache_next = 0;

	Pigfile_initialized=1;
}

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
	DiskBitmapHeader bmh;
	int header_size, N_bitmaps, data_start;
#ifdef EDITOR
	int must_rewrite_pig = 0;
#endif

	d_strlwr(pigname);

	if (d_strnicmp(Current_pigfile, pigname, sizeof(Current_pigfile)) == 0 // correct pig already loaded
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

	Piggy_fp = PHYSFSX_openReadBuffered(pigname);

	//try pigfile for shareware
	if (!Piggy_fp)
		Piggy_fp = PHYSFSX_openReadBuffered(DEFAULT_PIGFILE_SHAREWARE);
	
	if (Piggy_fp) {  //make sure pig is valid type file & is up-to-date
		int pig_id,pig_version;

		pig_id = PHYSFSX_readInt(Piggy_fp);
		pig_version = PHYSFSX_readInt(Piggy_fp);
		if (pig_id != PIGFILE_ID || pig_version != PIGFILE_VERSION) {
			PHYSFS_close(Piggy_fp);              //out of date pig
			Piggy_fp = NULL;                        //..so pretend it's not here
		}
	}

#ifndef EDITOR
	if (!Piggy_fp)
		Error("Cannot open correct version of <%s>", pigname);
#endif

	if (Piggy_fp) {

		N_bitmaps = PHYSFSX_readInt(Piggy_fp);

		header_size = N_bitmaps * sizeof(DiskBitmapHeader);

		data_start = header_size + PHYSFS_tell(Piggy_fp);

		for (i=1; i<=N_bitmaps; i++ )
		{
			grs_bitmap *bm = &GameBitmaps[i];
			int width;
			
			DiskBitmapHeader_read(&bmh, Piggy_fp);
			memcpy( temp_name_read, bmh.name, 8 );
			temp_name_read[8] = 0;
	
			if ( bmh.dflags & DBM_FLAG_ABM )        
				sprintf( temp_name, "%s#%d", temp_name_read, bmh.dflags & DBM_NUM_FRAMES );
			else
				strcpy( temp_name, temp_name_read );

#ifdef EDITOR
			//Make sure name matches
			if (strcmp(temp_name,AllBitmaps[i].name)) {
				//Int3();       //this pig is out of date.  Delete it
				must_rewrite_pig=1;
			}
#endif
	
			strcpy(AllBitmaps[i].name,temp_name);

			width = bmh.width + ((short) (bmh.wh_extra & 0x0f) << 8);
			gr_set_bitmap_data(bm, NULL);	// free ogl texture
			gr_init_bitmap(bm, 0, 0, 0, width, bmh.height + ((short) (bmh.wh_extra & 0xf0) << 4), width, NULL);
			bm->bm_flags = BM_FLAG_PAGED_OUT;
			bm->avg_color = bmh.avg_color;

			GameBitmapFlags[i] = bmh.flags & BM_FLAGS_TO_COPY;
	
			GameBitmapOffset[i] = bmh.offset + data_start;
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

					if ( GameArg.EdiMacData )
						swap_0_255( bm[fnum] );

					if ( !GameArg.DbgBigPig ) gr_bitmap_rle_compress( bm[fnum] );

					if (bm[fnum]->bm_flags & BM_FLAG_RLE)
						size = *((int *) bm[fnum]->bm_data);
					else
						size = bm[fnum]->bm_w * bm[fnum]->bm_h;

					memcpy( &Piggy_bitmap_cache_data[Piggy_bitmap_cache_next],bm[fnum]->bm_data,size);
					d_free(bm[fnum]->bm_data);
					bm[fnum]->bm_data = &Piggy_bitmap_cache_data[Piggy_bitmap_cache_next];
					Piggy_bitmap_cache_next += size;

					GameBitmaps[i+fnum] = *bm[fnum];

					d_free( bm[fnum] );
				}

				i += nframes-1;         //filled in multiple bitmaps
			}
			else {          //this is a BBM

				grs_bitmap * n;
				ubyte newpal[256*3];
				int iff_error;
				char bbmname[FILENAME_LEN];
				int SuperX;

				MALLOC( n, grs_bitmap, 1 );

				sprintf( bbmname, "%s.bbm", AllBitmaps[i].name );
				iff_error = iff_read_bitmap(bbmname,n,BM_LINEAR,newpal);

				n->bm_handle=0;
				if (iff_error != IFF_NO_ERROR)          {
					Error("File %s - IFF error: %s",bbmname,iff_errormsg(iff_error));
				}

				SuperX = (GameBitmapFlags[i]&BM_FLAG_SUPER_TRANSPARENT)?254:-1;
				//above makes assumption that supertransparent color is 254

				if ( iff_has_transparency )
					gr_remap_bitmap_good( n, newpal, iff_transparent_color, SuperX );
				else
					gr_remap_bitmap_good( n, newpal, -1, SuperX );

				n->avg_color = compute_average_pixel(n);

				if ( GameArg.EdiMacData )
					swap_0_255( n );

				if ( !GameArg.DbgBigPig )  gr_bitmap_rle_compress( n );

				if (n->bm_flags & BM_FLAG_RLE)
					size = *((int *) n->bm_data);
				else
					size = n->bm_w * n->bm_h;

				memcpy( &Piggy_bitmap_cache_data[Piggy_bitmap_cache_next],n->bm_data,size);
				d_free(n->bm_data);
				n->bm_data = &Piggy_bitmap_cache_data[Piggy_bitmap_cache_next];
				Piggy_bitmap_cache_next += size;

				GameBitmaps[i] = *n;
	
				d_free( n );
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
ubyte bogus_bitmap_initialized=0;
digi_sound bogus_sound;

#define HAMFILE_ID              MAKE_SIG('!','M','A','H') //HAM!
#define HAMFILE_VERSION 3
//version 1 -> 2:  save marker_model_num
//version 2 -> 3:  removed sound files

#define SNDFILE_ID              MAKE_SIG('D','N','S','D') //DSND
#define SNDFILE_VERSION 1

int piggy_is_needed(int soundnum);

int read_hamfile()
{
	PHYSFS_file * ham_fp = NULL;
	int ham_id;
	int sound_offset = 0;
	int shareware = 0;

	ham_fp = PHYSFSX_openReadBuffered(DEFAULT_HAMFILE_REGISTERED);
	
	if (!ham_fp)
	{
		ham_fp = PHYSFSX_openReadBuffered(DEFAULT_HAMFILE_SHAREWARE);
		if (ham_fp)
		{
			shareware = 1;
			GameArg.SndDigiSampleRate = SAMPLE_RATE_11K;
#ifdef USE_SDLMIXER
			if (GameArg.SndDisableSdlMixer)
#endif
			{
				digi_close();
				digi_init();
			}
		}
	}

	if (ham_fp == NULL) {
		Must_write_hamfile = 1;
		return 0;
	}

	//make sure ham is valid type file & is up-to-date
	ham_id = PHYSFSX_readInt(ham_fp);
	Piggy_hamfile_version = PHYSFSX_readInt(ham_fp);
	if (ham_id != HAMFILE_ID)
		Error("Cannot open ham file %s or %s\n", DEFAULT_HAMFILE_REGISTERED, DEFAULT_HAMFILE_SHAREWARE);
#if 0
	if (ham_id != HAMFILE_ID || Piggy_hamfile_version != HAMFILE_VERSION) {
		Must_write_hamfile = 1;
		PHYSFS_close(ham_fp);						//out of date ham
		return 0;
	}
#endif

	if (Piggy_hamfile_version < 3) // hamfile contains sound info, probably PC demo
	{
		sound_offset = PHYSFSX_readInt(ham_fp);
		
		if (shareware) // deal with interactive PC demo
		{
			GameArg.GfxHiresGFXAvailable = 0;
			//GameArg.GfxHiresFNTAvailable = 0;		// fonts are in the hog
			//GameArg.SysLowMem = 1;
		}
	}

	#if 1 //ndef EDITOR
	{
		int i;

		bm_read_all(ham_fp);
		//PHYSFS_read( ham_fp, GameBitmapXlat, sizeof(ushort)*MAX_BITMAP_FILES, 1 );
		for (i = 0; i < MAX_BITMAP_FILES; i++)
		{
			GameBitmapXlat[i] = PHYSFSX_readShort(ham_fp);
			if (PHYSFS_eof(ham_fp))
				break;
		}
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
		static int justonce = 1;

		if (!justonce)
		{
			PHYSFS_close(ham_fp);
			return 1;
		}
		justonce = 0;

		PHYSFSX_fseek(ham_fp, sound_offset, SEEK_SET);
		N_sounds = PHYSFSX_readInt(ham_fp);

		sound_start = PHYSFS_tell(ham_fp);

		header_size = N_sounds * sizeof(DiskSoundHeader);

		//Read sounds

		for (i=0; i<N_sounds; i++ ) {
			DiskSoundHeader_read(&sndh, ham_fp);
			temp_sound.length = sndh.length;
			temp_sound.data = (ubyte *)(size_t)(sndh.offset + header_size + sound_start);
			SoundOffset[Num_sound_files] = sndh.offset + header_size + sound_start;
			memcpy( temp_name_read, sndh.name, 8 );
			temp_name_read[8] = 0;
			piggy_register_sound( &temp_sound, temp_name_read, 1 );
			if (piggy_is_needed(i))
				sbytes += sndh.length;
		}

		SoundBits = d_malloc( sbytes + 16 );
		if ( SoundBits == NULL )
			Error( "Not enough memory to load sounds\n" );
	}

	PHYSFS_close(ham_fp);

	return 1;

}

int read_sndfile()
{
	PHYSFS_file * snd_fp = NULL;
	int snd_id,snd_version;
	int N_sounds;
	int sound_start;
	int header_size;
	int i;
	DiskSoundHeader sndh;
	digi_sound temp_sound;
	char temp_name_read[16];
	int sbytes = 0;

	snd_fp = PHYSFSX_openReadBuffered(DEFAULT_SNDFILE);
	
	if (snd_fp == NULL)
		return 0;

	//make sure soundfile is valid type file & is up-to-date
	snd_id = PHYSFSX_readInt(snd_fp);
	snd_version = PHYSFSX_readInt(snd_fp);
	if (snd_id != SNDFILE_ID || snd_version != SNDFILE_VERSION) {
		PHYSFS_close(snd_fp);						//out of date sound file
		return 0;
	}

	N_sounds = PHYSFSX_readInt(snd_fp);

	sound_start = PHYSFS_tell(snd_fp);
	header_size = N_sounds*sizeof(DiskSoundHeader);

	//Read sounds

	for (i=0; i<N_sounds; i++ ) {
		DiskSoundHeader_read(&sndh, snd_fp);
		temp_sound.length = sndh.length;
		temp_sound.data = (ubyte *)(size_t)(sndh.offset + header_size + sound_start);
		SoundOffset[Num_sound_files] = sndh.offset + header_size + sound_start;
		memcpy( temp_name_read, sndh.name, 8 );
		temp_name_read[8] = 0;
		piggy_register_sound( &temp_sound, temp_name_read, 1 );
		if (piggy_is_needed(i))
			sbytes += sndh.length;
	}

	SoundBits = d_malloc( sbytes + 16 );
	if ( SoundBits == NULL )
		Error( "Not enough memory to load sounds\n" );

	PHYSFS_close(snd_fp);

	return 1;
}

int properties_init(void)
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
		c = gr_find_closest_color( 0, 0, 63 );
		for (i=0; i<4096; i++ ) bogus_data[i] = c;
		c = gr_find_closest_color( 63, 0, 0 );
		// Make a big red X !
		for (i=0; i<64; i++ )   {
			bogus_data[i*64+i] = c;
			bogus_data[i*64+(63-i)] = c;
		}
		gr_init_bitmap(&GameBitmaps[Num_bitmap_files], 0, 0, 0, 64, 64, 64, bogus_data);
		piggy_register_bitmap(&GameBitmaps[Num_bitmap_files], "bogus", 1);
		bogus_sound.length = 64*64;
		bogus_sound.data = bogus_data;
		GameBitmapOffset[0] = 0;
	}

	snd_ok = ham_ok = read_hamfile();

	if (Piggy_hamfile_version >= 3)
	{
		snd_ok = read_sndfile();
		if (!snd_ok)
			Error("Cannot open sound file: %s\n", DEFAULT_SNDFILE);
	}

	return (ham_ok && snd_ok);               //read ok
}

int piggy_is_needed(int soundnum)
{
	int i;

	if ( !GameArg.SysLowMem ) return 1;

	for (i=0; i<MAX_SOUNDS; i++ )   {
		if ( (AltSounds[i] < 255) && (Sounds[AltSounds[i]] == soundnum) )
			return 1;
	}
	return 0;
}


void piggy_read_sounds(void)
{
	PHYSFS_file * fp = NULL;
	ubyte * ptr;
	int i, sbytes;

	ptr = SoundBits;
	sbytes = 0;

	fp = PHYSFSX_openReadBuffered(DEFAULT_SNDFILE);

	if (fp == NULL)
		return;

	for (i=0; i<Num_sound_files; i++ )      {
		digi_sound *snd = &GameSounds[i];

		if ( SoundOffset[i] > 0 )       {
			if ( piggy_is_needed(i) )       {
				PHYSFSX_fseek( fp, SoundOffset[i], SEEK_SET );

				// Read in the sound data!!!
				snd->data = ptr;
				ptr += snd->length;
				sbytes += snd->length;
				PHYSFS_read( fp, snd->data, snd->length, 1 );
			}
			else
				snd->data = (ubyte *) -1;
		}
	}

	PHYSFS_close(fp);

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
		descent_critical_error = 0;
		PHYSFSX_fseek( Piggy_fp, GameBitmapOffset[i], SEEK_SET );
		if ( descent_critical_error ) {
			piggy_critical_error();
			goto ReDoIt;
		}

		gr_set_bitmap_flags (bmp, GameBitmapFlags[i]);

		if ( bmp->bm_flags & BM_FLAG_RLE ) {
			int zsize = 0, pigsize = PHYSFS_fileLength(Piggy_fp);
			descent_critical_error = 0;
			zsize = PHYSFSX_readInt(Piggy_fp);
			if ( descent_critical_error ) {
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
			PHYSFS_read( Piggy_fp, &Piggy_bitmap_cache_data[Piggy_bitmap_cache_next+4], 1, zsize-4 );
			if ( descent_critical_error ) {
				piggy_critical_error();
				goto ReDoIt;
			}
			*((int *) (Piggy_bitmap_cache_data + Piggy_bitmap_cache_next)) = INTEL_INT(zsize);
			gr_set_bitmap_data(bmp, &Piggy_bitmap_cache_data[Piggy_bitmap_cache_next]);

#ifndef MACDATA
			switch (pigsize) {
			default:
				if (!GameArg.EdiMacData)
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

			Piggy_bitmap_cache_next += zsize;
			if ( Piggy_bitmap_cache_next+zsize >= Piggy_bitmap_cache_size ) {
				Int3();
				piggy_bitmap_page_out_all();
				goto ReDoIt;
			}

		} else {
			int pigsize = PHYSFS_fileLength(Piggy_fp);
			// GET JOHN NOW IF YOU GET THIS ASSERT!!!
			Assert( Piggy_bitmap_cache_next+(bmp->bm_h*bmp->bm_w) < Piggy_bitmap_cache_size );
			if ( Piggy_bitmap_cache_next+(bmp->bm_h*bmp->bm_w) >= Piggy_bitmap_cache_size ) {
				piggy_bitmap_page_out_all();
				goto ReDoIt;
			}
			descent_critical_error = 0;
			PHYSFS_read( Piggy_fp, &Piggy_bitmap_cache_data[Piggy_bitmap_cache_next], 1, bmp->bm_h*bmp->bm_w );
			if ( descent_critical_error ) {
				piggy_critical_error();
				goto ReDoIt;
			}
			gr_set_bitmap_data(bmp, &Piggy_bitmap_cache_data[Piggy_bitmap_cache_next]);
			Piggy_bitmap_cache_next+=bmp->bm_h*bmp->bm_w;

#ifndef MACDATA
			switch (pigsize) {
			default:
				if (!GameArg.EdiMacData)
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

		compute_average_rgb(bmp, bmp->avg_color_rgb);

		start_time();
	}

	if ( GameArg.SysLowMem ) {
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

	for (i=0; i<Num_bitmap_files; i++ ) {
		if ( GameBitmapOffset[i] > 0 ) {	// Don't page out bitmaps read from disk!!!
			GameBitmaps[i].bm_flags = BM_FLAG_PAGED_OUT;
			gr_set_bitmap_data(&GameBitmaps[i], NULL);
		}
	}

}

void piggy_load_level_data()
{
	piggy_bitmap_page_out_all();
	paging_touch_all();
}

#ifdef EDITOR

void piggy_write_pigfile(char *filename)
{
	PHYSFS_file *pig_fp;
	int bitmap_data_start, data_offset;
	DiskBitmapHeader bmh;
	int org_offset;
	char subst_name[32];
	int i;
	PHYSFS_file *fp1,*fp2;
	char tname[FILENAME_LEN];

	for (i=0; i < Num_bitmap_files; i++ ) {
		bitmap_index bi;
		bi.index = i;
		PIGGY_PAGE_IN( bi );
	}

	piggy_close_file();

	pig_fp = PHYSFSX_openWriteBuffered( filename );       //open PIG file
	Assert( pig_fp!=NULL );

	write_int(PIGFILE_ID,pig_fp);
	write_int(PIGFILE_VERSION,pig_fp);

	Num_bitmap_files--;
	PHYSFS_write( pig_fp, &Num_bitmap_files, sizeof(int), 1 );
	Num_bitmap_files++;

	bitmap_data_start = PHYSFS_tell(pig_fp);
	bitmap_data_start += (Num_bitmap_files - 1) * sizeof(DiskBitmapHeader);
	data_offset = bitmap_data_start;

	change_filename_extension(tname,filename,"lst");
	fp1 = PHYSFSX_openWriteBuffered( tname );
	change_filename_extension(tname,filename,"all");
	fp2 = PHYSFSX_openWriteBuffered( tname );

	for (i=1; i < Num_bitmap_files; i++ ) {
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
					PHYSFSX_printf( fp2, "%s.abm\n", AllBitmaps[i].name );
				memcpy( bmh.name, AllBitmaps[i].name, 8 );
				Assert( n <= DBM_NUM_FRAMES );
				bmh.dflags = DBM_FLAG_ABM + n;
				*p = '#';
			} else {
				if (fp2)
					PHYSFSX_printf( fp2, "%s.bbm\n", AllBitmaps[i].name );
				memcpy( bmh.name, AllBitmaps[i].name, 8 );
				bmh.dflags = 0;
			}
		}
		bmp = &GameBitmaps[i];

		Assert( !(bmp->bm_flags&BM_FLAG_PAGED_OUT) );

		if (fp1)
			PHYSFSX_printf( fp1, "BMP: %s, size %d bytes", AllBitmaps[i].name, bmp->bm_rowsize * bmp->bm_h );
		org_offset = PHYSFS_tell(pig_fp);
		bmh.offset = data_offset - bitmap_data_start;
		PHYSFSX_fseek( pig_fp, data_offset, SEEK_SET );

		if ( bmp->bm_flags & BM_FLAG_RLE ) {
			size = (int *)bmp->bm_data;
			PHYSFS_write( pig_fp, bmp->bm_data, sizeof(ubyte), *size );
			data_offset += *size;
			if (fp1)
				PHYSFSX_printf( fp1, ", and is already compressed to %d bytes.\n", *size );
		} else {
			PHYSFS_write( pig_fp, bmp->bm_data, sizeof(ubyte), bmp->bm_rowsize * bmp->bm_h );
			data_offset += bmp->bm_rowsize * bmp->bm_h;
			if (fp1)
				PHYSFSX_printf( fp1, ".\n" );
		}
		PHYSFSX_fseek( pig_fp, org_offset, SEEK_SET );
		Assert( GameBitmaps[i].bm_w < 4096 );
		bmh.width = (GameBitmaps[i].bm_w & 0xff);
		bmh.wh_extra = ((GameBitmaps[i].bm_w >> 8) & 0x0f);
		Assert( GameBitmaps[i].bm_h < 4096 );
		bmh.height = GameBitmaps[i].bm_h;
		bmh.wh_extra |= ((GameBitmaps[i].bm_h >> 4) & 0xf0);
		bmh.flags = GameBitmaps[i].bm_flags;
		if (piggy_is_substitutable_bitmap( AllBitmaps[i].name, subst_name )) {
			bitmap_index other_bitmap;
			other_bitmap = piggy_find_bitmap( subst_name );
			GameBitmapXlat[i] = other_bitmap.index;
			bmh.flags |= BM_FLAG_PAGED_OUT;
		} else {
			bmh.flags &= ~BM_FLAG_PAGED_OUT;
		}
		bmh.avg_color=GameBitmaps[i].avg_color;
		PHYSFS_write(pig_fp, &bmh, sizeof(DiskBitmapHeader), 1);	// Mark as a bitmap
	}

	PHYSFS_close(pig_fp);

	PHYSFSX_printf( fp1, " Dumped %d assorted bitmaps.\n", Num_bitmap_files );

	PHYSFS_close(fp1);
	PHYSFS_close(fp2);

}

static void write_int(int i, PHYSFS_file *file)
{
	if (PHYSFS_write( file, &i, sizeof(i), 1) != 1)
		Error( "Error reading int in gamesave.c" );

}

void piggy_dump_all()
{
	int i, xlat_offset;
	PHYSFS_file * ham_fp;
	int org_offset,data_offset=0;
	DiskSoundHeader sndh;
	int sound_data_start=0;
	PHYSFS_file *fp1,*fp2;

	#ifdef NO_DUMP_SOUNDS
	Num_sound_files = 0;
	Num_sound_files_new = 0;
	#endif

	if (!Must_write_hamfile && (Num_bitmap_files_new == 0) && (Num_sound_files_new == 0) )
		return;

	fp1 = PHYSFSX_openWriteBuffered( "ham.lst" );
	fp2 = PHYSFSX_openWriteBuffered( "ham.all" );

	if (Must_write_hamfile || Num_bitmap_files_new) {

		ham_fp = PHYSFSX_openWriteBuffered( DEFAULT_HAMFILE_REGISTERED );                       //open HAM file
		Assert( ham_fp!=NULL );
	
		write_int(HAMFILE_ID,ham_fp);
		write_int(HAMFILE_VERSION,ham_fp);
	
		bm_write_all(ham_fp);
		xlat_offset = PHYSFS_tell(ham_fp);
		PHYSFS_write( ham_fp, GameBitmapXlat, sizeof(ushort)*MAX_BITMAP_FILES, 1 );
		//Dump bitmaps
	
		if (Num_bitmap_files_new)
			piggy_write_pigfile(DEFAULT_PIGFILE_REGISTERED);
	
		//free up memeory used by new bitmaps
		for (i=Num_bitmap_files-Num_bitmap_files_new;i<Num_bitmap_files;i++)
			d_free(GameBitmaps[i].bm_data);
	
		//next thing must be done after pig written
		PHYSFSX_fseek( ham_fp, xlat_offset, SEEK_SET );
		PHYSFS_write( ham_fp, GameBitmapXlat, sizeof(ushort)*MAX_BITMAP_FILES, 1 );
	
		PHYSFS_close(ham_fp);
	}
	
	if (Num_sound_files_new) {

		// Now dump sound file
		ham_fp = PHYSFSX_openWriteBuffered( DEFAULT_SNDFILE );
		Assert( ham_fp!=NULL );
	
		write_int(SNDFILE_ID,ham_fp);
		write_int(SNDFILE_VERSION,ham_fp);

		PHYSFS_write( ham_fp, &Num_sound_files, sizeof(int), 1 );
	
		sound_data_start = PHYSFS_tell(ham_fp);
		sound_data_start += Num_sound_files*sizeof(DiskSoundHeader);
		data_offset = sound_data_start;
	
		for (i=0; i < Num_sound_files; i++ )    {
			digi_sound *snd;
	
			snd = &GameSounds[i];
			strcpy( sndh.name, AllSounds[i].name );
			sndh.length = GameSounds[i].length;
			sndh.offset = data_offset - sound_data_start;
	
			org_offset = PHYSFS_tell(ham_fp);
			PHYSFSX_fseek( ham_fp, data_offset, SEEK_SET );
	
			sndh.data_length = GameSounds[i].length;
			PHYSFS_write( ham_fp, snd->data, sizeof(ubyte), snd->length );
			data_offset += snd->length;
			PHYSFSX_fseek( ham_fp, org_offset, SEEK_SET );
			PHYSFS_write( ham_fp, &sndh, sizeof(DiskSoundHeader), 1 );                    // Mark as a bitmap
	
			PHYSFSX_printf( fp1, "SND: %s, size %d bytes\n", AllSounds[i].name, snd->length );
			PHYSFSX_printf( fp2, "%s.raw\n", AllSounds[i].name );
		}

		PHYSFS_close(ham_fp);
	}

	PHYSFSX_printf( fp1, "Total sound size: %d bytes\n", data_offset-sound_data_start);
	PHYSFSX_printf( fp1, " Dumped %d assorted sounds.\n", Num_sound_files );

	PHYSFS_close(fp1);
	PHYSFS_close(fp2);

	// Never allow the game to run after building ham.
	exit(0);
}

#endif

void piggy_close()
{
	int i;

	piggy_close_file();

	if (BitmapBits)
		d_free(BitmapBits);

	if ( SoundBits )
		d_free( SoundBits );

	for (i = 0; i < Num_sound_files; i++)
		if (SoundOffset[i] == 0)
			d_free(GameSounds[i].data);
	
	hashtable_free( &AllBitmapsNames );
	hashtable_free( &AllDigiSndNames );

	free_bitmap_replacements();
	free_d1_tmap_nums();
}

int piggy_does_bitmap_exist_slow( char * name )
{
	int i;

	for (i=0; i<Num_bitmap_files; i++ ) {
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
	for (i=0; i<NUM_GAUGE_BITMAPS; i++ ) {
		if ( !d_stricmp( base_name, gauge_bitmap_names[i] ))
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



/*
 * Functions for loading replacement textures
 *  1) From .pog files
 *  2) From descent.pig (for loading d1 levels)
 */

void free_bitmap_replacements()
{
	if (Bitmap_replacement_data) {
		d_free(Bitmap_replacement_data);
		Bitmap_replacement_data = NULL;
	}
}

void load_bitmap_replacements(const char *level_name)
{
	char ifile_name[FILENAME_LEN];
	PHYSFS_file *ifile;
	int i;

	//first, free up data allocated for old bitmaps
	free_bitmap_replacements();

	change_filename_extension(ifile_name, level_name, ".POG" );

	ifile = PHYSFSX_openReadBuffered(ifile_name);

	if (ifile) {
		int id,version,n_bitmaps;
		int bitmap_data_size;
		ushort *indices;

		id = PHYSFSX_readInt(ifile);
		version = PHYSFSX_readInt(ifile);

		if (id != MAKE_SIG('G','O','P','D') || version != 1) {
			PHYSFS_close(ifile);
			return;
		}

		n_bitmaps = PHYSFSX_readInt(ifile);

		MALLOC( indices, ushort, n_bitmaps );

		for (i = 0; i < n_bitmaps; i++)
			indices[i] = PHYSFSX_readShort(ifile);

		bitmap_data_size = PHYSFS_fileLength(ifile) - PHYSFS_tell(ifile) - sizeof(DiskBitmapHeader) * n_bitmaps;
		MALLOC( Bitmap_replacement_data, ubyte, bitmap_data_size );

		for (i=0;i<n_bitmaps;i++) {
			DiskBitmapHeader bmh;
			grs_bitmap *bm = &GameBitmaps[indices[i]];
			int width;

			DiskBitmapHeader_read(&bmh, ifile);

			width = bmh.width + ((short) (bmh.wh_extra & 0x0f) << 8);
			gr_set_bitmap_data(bm, NULL);	// free ogl texture
			gr_init_bitmap(bm, 0, 0, 0, width, bmh.height + ((short) (bmh.wh_extra & 0xf0) << 4), width, NULL);
			bm->avg_color = bmh.avg_color;
			bm->bm_data = (ubyte *) (size_t)bmh.offset;

			gr_set_bitmap_flags(bm, bmh.flags & BM_FLAGS_TO_COPY);

			GameBitmapOffset[indices[i]] = 0; // don't try to read bitmap from current pigfile
		}

		PHYSFS_read(ifile,Bitmap_replacement_data,1,bitmap_data_size);

		for (i = 0; i < n_bitmaps; i++)
		{
			grs_bitmap *bm = &GameBitmaps[indices[i]];
			gr_set_bitmap_data(bm, Bitmap_replacement_data + (size_t) bm->bm_data);
		}

		d_free(indices);

		PHYSFS_close(ifile);

		last_palette_loaded_pig[0]= 0;  //force pig re-load

		texmerge_flush();       //for re-merging with new textures
	}
}

/* calculate table to translate d1 bitmaps to current palette,
 * return -1 on error
 */
int get_d1_colormap( ubyte *d1_palette, ubyte *colormap )
{
	int freq[256];
	PHYSFS_file * palette_file = PHYSFSX_openReadBuffered(D1_PALETTE);
	if (!palette_file || PHYSFS_fileLength(palette_file) != 9472)
		return -1;
	PHYSFS_read( palette_file, d1_palette, 256, 3 );
	PHYSFS_close( palette_file );
	build_colormap_good( d1_palette, colormap, freq );
	// don't change transparencies:
	colormap[254] = 254;
	colormap[255] = 255;
	return 0;
}

#define JUST_IN_CASE 132 /* is enough for d1 pc registered */
void bitmap_read_d1( grs_bitmap *bitmap, /* read into this bitmap */
                     PHYSFS_file *d1_Piggy_fp, /* read from this file */
                     int bitmap_data_start, /* specific to file */
                     DiskBitmapHeader *bmh, /* header info for bitmap */
                     ubyte **next_bitmap, /* where to write it (if 0, use malloc) */
		     ubyte *d1_palette, /* what palette the bitmap has */
                     ubyte *colormap) /* how to translate bitmap's colors */
{
	int zsize, pigsize = PHYSFS_fileLength(d1_Piggy_fp);
	ubyte *data;
	int width;

	width = bmh->width + ((short) (bmh->wh_extra & 0x0f) << 8);
	gr_set_bitmap_data(bitmap, NULL);	// free ogl texture
	gr_init_bitmap(bitmap, 0, 0, 0, width, bmh->height + ((short) (bmh->wh_extra & 0xf0) << 4), width, NULL);
	bitmap->avg_color = bmh->avg_color;
	gr_set_bitmap_flags(bitmap, bmh->flags & BM_FLAGS_TO_COPY);

	PHYSFSX_fseek(d1_Piggy_fp, bitmap_data_start + bmh->offset, SEEK_SET);
	if (bmh->flags & BM_FLAG_RLE) {
		zsize = PHYSFSX_readInt(d1_Piggy_fp);
		PHYSFSX_fseek(d1_Piggy_fp, -4, SEEK_CUR);
	} else
		zsize = bitmap->bm_h * bitmap->bm_w;

	if (next_bitmap) {
		data = *next_bitmap;
		*next_bitmap += zsize;
	} else {
		data = d_malloc(zsize + JUST_IN_CASE);
	}
	if (!data) return;

	PHYSFS_read(d1_Piggy_fp, data, 1, zsize);
	gr_set_bitmap_data(bitmap, data);
	switch(pigsize) {
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
		int new_size;
		memcpy(&new_size, bitmap->bm_data, 4);
		if (next_bitmap) {
			*next_bitmap += new_size - zsize;
		} else {
			Assert( zsize + JUST_IN_CASE >= new_size );
			bitmap->bm_data = d_realloc(bitmap->bm_data, new_size);
			Assert(bitmap->bm_data);
		}
	}
}

#define D1_MAX_TEXTURES 800
#define D1_MAX_TMAP_NUM 1630 // 1621 in descent.pig Mac registered

/* the inverse of the d2 Textures array, but for the descent 1 pigfile.
 * "Textures" looks up a d2 bitmap index given a d2 tmap_num.
 * "d1_tmap_nums" looks up a d1 tmap_num given a d1 bitmap. "-1" means "None".
 */
short *d1_tmap_nums = NULL;

void free_d1_tmap_nums() {
	if (d1_tmap_nums) {
		d_free(d1_tmap_nums);
		d1_tmap_nums = NULL;
	}
}

void bm_read_d1_tmap_nums(PHYSFS_file *d1pig)
{
	int i, d1_index;

	free_d1_tmap_nums();
	PHYSFSX_fseek(d1pig, 8, SEEK_SET);
	MALLOC(d1_tmap_nums, short, D1_MAX_TMAP_NUM);
	for (i = 0; i < D1_MAX_TMAP_NUM; i++)
		d1_tmap_nums[i] = -1;
	for (i = 0; i < D1_MAX_TEXTURES; i++) {
		d1_index = PHYSFSX_readShort(d1pig);
		Assert(d1_index >= 0 && d1_index < D1_MAX_TMAP_NUM);
		d1_tmap_nums[d1_index] = i;
		if (PHYSFS_eof(d1pig))
			break;
	}
}

void remove_char( char * s, char c )
{
	char *p;
	p = strchr(s,c);
	if (p) *p = '\0';
}

const char space[3] = " \t";
const char equal_space[4] = " \t=";

// this function is at the same position in the d1 shareware piggy loading 
// algorithm as bm_load_sub in main/bmread.c
int get_d1_bm_index(char *filename, PHYSFS_file *d1_pig) {
	int i, N_bitmaps;
	DiskBitmapHeader bmh;
	if (strchr (filename, '.'))
		*strchr (filename, '.') = '\0'; // remove extension
	PHYSFSX_fseek (d1_pig, 0, SEEK_SET);
	N_bitmaps = PHYSFSX_readInt (d1_pig);
	PHYSFSX_fseek (d1_pig, 8, SEEK_SET);
	for (i = 1; i <= N_bitmaps; i++) {
		DiskBitmapHeader_d1_read(&bmh, d1_pig);
		if (!d_strnicmp(bmh.name, filename, 8))
			return i;
	}
	return -1;
}

// imitate the algorithm of gamedata_read_tbl in main/bmread.c
void read_d1_tmap_nums_from_hog(PHYSFS_file *d1_pig)
{
#define LINEBUF_SIZE 600
	int reading_textures = 0;
	short texture_count = 0;
	char inputline[LINEBUF_SIZE];
	PHYSFS_file * bitmaps;
	int bitmaps_tbl_is_binary = 0;
	int i;

	bitmaps = PHYSFSX_openReadBuffered ("bitmaps.tbl");
	if (!bitmaps) {
		bitmaps = PHYSFSX_openReadBuffered ("bitmaps.bin");
		bitmaps_tbl_is_binary = 1;
	}

	if (!bitmaps) {
		Warning ("Could not find bitmaps.* for reading d1 textures");
		return;
	}

	free_d1_tmap_nums();
	MALLOC(d1_tmap_nums, short, D1_MAX_TMAP_NUM);
	for (i = 0; i < D1_MAX_TMAP_NUM; i++)
		d1_tmap_nums[i] = -1;

	while (PHYSFSX_fgets (inputline, LINEBUF_SIZE, bitmaps)) {
		char *arg;

		if (bitmaps_tbl_is_binary)
			decode_text_line((inputline));
		else
			while (inputline[(i=strlen(inputline))-2]=='\\')
				PHYSFSX_fgets(inputline+i-2,LINEBUF_SIZE-(i-2), bitmaps); // strip comments
		REMOVE_EOL(inputline);
                if (strchr(inputline, ';')!=NULL) REMOVE_COMMENTS(inputline);
		if (strlen(inputline) == LINEBUF_SIZE-1) {
			Warning("Possible line truncation in BITMAPS.TBL");
			return;
		}
		arg = strtok( inputline, space );
                if (arg && arg[0] == '@') {
			arg++;
			//Registered_only = 1;
		}

                while (arg != NULL) {
			if (*arg == '$')
				reading_textures = 0; // default
			if (!strcmp(arg, "$TEXTURES")) // BM_TEXTURES
				reading_textures = 1;
			else if (! d_stricmp(arg, "$ECLIP") // BM_ECLIP
				   || ! d_stricmp(arg, "$WCLIP")) // BM_WCLIP
					texture_count++;
			else // not a special token, must be a bitmap!
				if (reading_textures) {
					while (*arg == '\t' || *arg == ' ')
						arg++;//remove unwanted blanks
					if (*arg == '\0')
						break;
					if (d1_tmap_num_unique(texture_count)) {
						int d1_index = get_d1_bm_index(arg, d1_pig);
						if (d1_index >= 0 && d1_index < D1_MAX_TMAP_NUM) {
							d1_tmap_nums[d1_index] = texture_count;
							//int d2_index = d2_index_for_d1_index(d1_index);
						}
				}
				Assert (texture_count < D1_MAX_TEXTURES);
				texture_count++;
			}

			arg = strtok (NULL, equal_space);
		}
	}
	PHYSFS_close (bitmaps);
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

	return Textures[convert_d1_tmap_num(d1_tmap_nums[d1_index])].index;
}

#define D1_BITMAPS_SIZE 300000
void load_d1_bitmap_replacements()
{
	PHYSFS_file * d1_Piggy_fp;
	DiskBitmapHeader bmh;
	int pig_data_start, bitmap_header_start, bitmap_data_start;
	int N_bitmaps;
	short d1_index, d2_index;
	ubyte* next_bitmap;
	ubyte colormap[256];
	ubyte d1_palette[256*3];
	char *p;
	int pigsize;

	d1_Piggy_fp = PHYSFSX_openReadBuffered( D1_PIGFILE );

#define D1_PIG_LOAD_FAILED "Failed loading " D1_PIGFILE
	if (!d1_Piggy_fp) {
		Warning(D1_PIG_LOAD_FAILED);
		return;
	}

	//first, free up data allocated for old bitmaps
	free_bitmap_replacements();

	if (get_d1_colormap( d1_palette, colormap ) != 0)
		Warning("Could not load descent 1 color palette");

	pigsize = PHYSFS_fileLength(d1_Piggy_fp);
	switch (pigsize) {
	case D1_SHARE_BIG_PIGSIZE:
	case D1_SHARE_10_PIGSIZE:
	case D1_SHARE_PIGSIZE:
	case D1_10_BIG_PIGSIZE:
	case D1_10_PIGSIZE:
		pig_data_start = 0;
		// OK, now we need to read d1_tmap_nums by emulating d1's gamedata_read_tbl()
		read_d1_tmap_nums_from_hog(d1_Piggy_fp);
		break;
	default:
		Warning("Unknown size for " D1_PIGFILE);
		Int3();
		// fall through
	case D1_PIGSIZE:
	case D1_OEM_PIGSIZE:
	case D1_MAC_PIGSIZE:
	case D1_MAC_SHARE_PIGSIZE:
		pig_data_start = PHYSFSX_readInt(d1_Piggy_fp );
		bm_read_d1_tmap_nums(d1_Piggy_fp); //was: bm_read_all_d1(fp);
		//for (i = 0; i < 1800; i++) GameBitmapXlat[i] = PHYSFSX_readShort(d1_Piggy_fp);
		break;
	}

	PHYSFSX_fseek( d1_Piggy_fp, pig_data_start, SEEK_SET );
	N_bitmaps = PHYSFSX_readInt(d1_Piggy_fp);
	{
		int N_sounds = PHYSFSX_readInt(d1_Piggy_fp);
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

	next_bitmap = Bitmap_replacement_data;

	for (d1_index = 1; d1_index <= N_bitmaps; d1_index++ ) {
		d2_index = d2_index_for_d1_index(d1_index);
		// only change bitmaps which are unique to d1
		if (d2_index != -1) {
			PHYSFSX_fseek(d1_Piggy_fp, bitmap_header_start + (d1_index-1) * DISKBITMAPHEADER_D1_SIZE, SEEK_SET);
			DiskBitmapHeader_d1_read(&bmh, d1_Piggy_fp);

			bitmap_read_d1( &GameBitmaps[d2_index], d1_Piggy_fp, bitmap_data_start, &bmh, &next_bitmap, d1_palette, colormap );
			Assert(next_bitmap - Bitmap_replacement_data < D1_BITMAPS_SIZE);
			GameBitmapOffset[d2_index] = 0; // don't try to read bitmap from current d2 pigfile
			GameBitmapFlags[d2_index] = bmh.flags;

			if ( (p = strchr(AllBitmaps[d2_index].name, '#')) /* d2 BM is animated */
			     && !(bmh.dflags & DBM_FLAG_ABM) ) { /* d1 bitmap is not animated */
				int i, len = p - AllBitmaps[d2_index].name;
				for (i = 0; i < Num_bitmap_files; i++)
					if (i != d2_index && ! memcmp(AllBitmaps[d2_index].name, AllBitmaps[i].name, len))
					{
						gr_set_bitmap_data(&GameBitmaps[i], NULL);	// free ogl texture
						GameBitmaps[i] = GameBitmaps[d2_index];
						GameBitmapOffset[i] = 0;
						GameBitmapFlags[i] = bmh.flags;
					}
			}
		}
	}

	PHYSFS_close(d1_Piggy_fp);

	last_palette_loaded_pig[0]= 0;  //force pig re-load

	texmerge_flush();       //for re-merging with new textures
}


extern int extra_bitmap_num;

/*
 * Find and load the named bitmap from descent.pig
 * similar to read_extra_bitmap_iff
 */
bitmap_index read_extra_bitmap_d1_pig(const char *name)
{
	bitmap_index bitmap_num;
	grs_bitmap * n = &GameBitmaps[extra_bitmap_num];

	bitmap_num.index = 0;

	{
		PHYSFS_file *d1_Piggy_fp;
		DiskBitmapHeader bmh;
		int pig_data_start, bitmap_header_start, bitmap_data_start;
		int i, N_bitmaps;
		ubyte colormap[256];
		ubyte d1_palette[256*3];
		int pigsize;

		d1_Piggy_fp = PHYSFSX_openReadBuffered(D1_PIGFILE);

		if (!d1_Piggy_fp)
		{
			Warning(D1_PIG_LOAD_FAILED);
			return bitmap_num;
		}

		if (get_d1_colormap( d1_palette, colormap ) != 0)
			Warning("Could not load descent 1 color palette");

		pigsize = PHYSFS_fileLength(d1_Piggy_fp);
		switch (pigsize) {
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
			pig_data_start = PHYSFSX_readInt(d1_Piggy_fp );

			break;
		}

		PHYSFSX_fseek( d1_Piggy_fp, pig_data_start, SEEK_SET );
		N_bitmaps = PHYSFSX_readInt(d1_Piggy_fp);
		{
			int N_sounds = PHYSFSX_readInt(d1_Piggy_fp);
			int header_size = N_bitmaps * DISKBITMAPHEADER_D1_SIZE
				+ N_sounds * sizeof(DiskSoundHeader);
			bitmap_header_start = pig_data_start + 2 * sizeof(int);
			bitmap_data_start = bitmap_header_start + header_size;
		}

		for (i = 1; i <= N_bitmaps; i++)
		{
			DiskBitmapHeader_d1_read(&bmh, d1_Piggy_fp);
			if (!d_strnicmp(bmh.name, name, 8))
				break;
		}

		if (d_strnicmp(bmh.name, name, 8))
		{
			con_printf(CON_DEBUG, "could not find bitmap %s\n", name);
			return bitmap_num;
		}

		bitmap_read_d1( n, d1_Piggy_fp, bitmap_data_start, &bmh, 0, d1_palette, colormap );

		PHYSFS_close(d1_Piggy_fp);
	}

	n->avg_color = 0;	//compute_average_pixel(n);

	bitmap_num.index = extra_bitmap_num;

	GameBitmaps[extra_bitmap_num++] = *n;

	return bitmap_num;
}

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
