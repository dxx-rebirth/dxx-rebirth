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

#ifdef HAVE_CONFIG_H
#include <conf.h>
#endif

#ifdef RCS
static char rcsid[] = "$Id: piggy.c,v 1.10 2002-07-29 02:32:32 btb Exp $";
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

//#define NO_DUMP_SOUNDS        1               //if set, dump bitmaps but not sounds

#define DEFAULT_PIGFILE_REGISTERED      "groupa.pig"
#define DEFAULT_PIGFILE_SHAREWARE       "d2demo.pig"


#ifdef SHAREWARE
	#define DEFAULT_HAMFILE         "d2demo.ham"
	#define DEFAULT_PIGFILE         DEFAULT_PIGFILE_SHAREWARE
	#define DEFAULT_SNDFILE			"descent2.s11"
#else
	#define DEFAULT_HAMFILE         "descent2.ham"
	#define DEFAULT_PIGFILE         DEFAULT_PIGFILE_REGISTERED
	#define DEFAULT_SNDFILE	 		((digi_sample_rate==SAMPLE_RATE_22K)?"descent2.s22":"descent2.s11")
#endif	// end of ifdef SHAREWARE

ubyte *BitmapBits = NULL;
ubyte *SoundBits = NULL;

typedef struct BitmapFile       {
	char                    name[15];
} BitmapFile;

typedef struct SoundFile        {
	char                    name[15];
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

#define DBM_FLAG_ABM            64

typedef struct DiskSoundHeader {
	char name[8];
	int length;
	int data_length;
	int offset;
} __pack__ DiskSoundHeader;

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
#endif

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
			remove(find.name);
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
			remove(filename);

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

	#ifdef SHAREWARE                //rename pigfile for shareware
	if (stricmp(filename,DEFAULT_PIGFILE_REGISTERED)==0)
		filename = DEFAULT_PIGFILE_SHAREWARE;
	#endif

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

	header_size = N_bitmaps*DISKBITMAPHEADER_SIZE;

	data_start = header_size + cftell(Piggy_fp);

	data_size = cfilelength(Piggy_fp) - data_start;

	Num_bitmap_files = 1;

	for (i=0; i<N_bitmaps; i++ )    {
		DiskBitmapHeader_read(&bmh, Piggy_fp);
		memcpy( temp_name_read, bmh.name, 8 );
		temp_name_read[8] = 0;
		if ( bmh.dflags & DBM_FLAG_ABM )        
			sprintf( temp_name, "%s#%d", temp_name_read, bmh.dflags & 63 );
		else
			strcpy( temp_name, temp_name_read );
		memset( &temp_bitmap, 0, sizeof(grs_bitmap) );
		temp_bitmap.bm_w = temp_bitmap.bm_rowsize = bmh.width + ((short) (bmh.wh_extra&0x0f)<<8);
		temp_bitmap.bm_h = bmh.height + ((short) (bmh.wh_extra&0xf0)<<4);
		temp_bitmap.bm_flags = BM_FLAG_PAGED_OUT;
		temp_bitmap.avg_color = bmh.avg_color;
		temp_bitmap.bm_data = Piggy_bitmap_cache_data;

		GameBitmapFlags[i+1] = 0;
		if ( bmh.flags & BM_FLAG_TRANSPARENT ) GameBitmapFlags[i+1] |= BM_FLAG_TRANSPARENT;
		if ( bmh.flags & BM_FLAG_SUPER_TRANSPARENT ) GameBitmapFlags[i+1] |= BM_FLAG_SUPER_TRANSPARENT;
		if ( bmh.flags & BM_FLAG_NO_LIGHTING ) GameBitmapFlags[i+1] |= BM_FLAG_NO_LIGHTING;
		if ( bmh.flags & BM_FLAG_RLE ) GameBitmapFlags[i+1] |= BM_FLAG_RLE;
		if ( bmh.flags & BM_FLAG_RLE_BIG ) GameBitmapFlags[i+1] |= BM_FLAG_RLE_BIG;

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

	#ifdef SHAREWARE                //rename pigfile for shareware
	if (stricmp(pigname,DEFAULT_PIGFILE_REGISTERED)==0)
		pigname = DEFAULT_PIGFILE_SHAREWARE;
	#endif

	if (strnicmp(Current_pigfile,pigname,sizeof(Current_pigfile))==0)
		return;         //already have correct pig

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

	if (Piggy_fp) {                         //make sure pig is valid type file & is up-to-date
		int pig_id,pig_version;

		pig_id = cfile_read_int(Piggy_fp);
		pig_version = cfile_read_int(Piggy_fp);
		if (pig_id != PIGFILE_ID || pig_version != PIGFILE_VERSION) {
			cfclose(Piggy_fp);              //out of date pig
			Piggy_fp = NULL;                        //..so pretend it's not here
		}
	}

	#ifndef EDITOR
	if (!Piggy_fp) Error ("Piggy_fp not defined in piggy_new_pigfile.");
	#endif

	if (Piggy_fp) {

		N_bitmaps = cfile_read_int(Piggy_fp);
	
		header_size = N_bitmaps*DISKBITMAPHEADER_SIZE;
	
		data_start = header_size + cftell(Piggy_fp);

		data_size = cfilelength(Piggy_fp) - data_start;
	
		for (i=1; i<=N_bitmaps; i++ )   {
			DiskBitmapHeader_read(&bmh, Piggy_fp);
			memcpy( temp_name_read, bmh.name, 8 );
			temp_name_read[8] = 0;
	
			if ( bmh.dflags & DBM_FLAG_ABM )        
				sprintf( temp_name, "%s#%d", temp_name_read, bmh.dflags & 63 );
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
	
			GameBitmapFlags[i] = 0;
	
			if ( bmh.flags & BM_FLAG_TRANSPARENT ) GameBitmapFlags[i] |= BM_FLAG_TRANSPARENT;
			if ( bmh.flags & BM_FLAG_SUPER_TRANSPARENT ) GameBitmapFlags[i] |= BM_FLAG_SUPER_TRANSPARENT;
			if ( bmh.flags & BM_FLAG_NO_LIGHTING ) GameBitmapFlags[i] |= BM_FLAG_NO_LIGHTING;
			if ( bmh.flags & BM_FLAG_RLE ) GameBitmapFlags[i] |= BM_FLAG_RLE;
			if ( bmh.flags & BM_FLAG_RLE_BIG ) GameBitmapFlags[i] |= BM_FLAG_RLE_BIG;
	
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

			if (p) {                //this is an ABM
				char abmname[FILENAME_LEN];
				int fnum;
				grs_bitmap * bm[MAX_BITMAPS_PER_BRUSH];
				int iff_error;          //reference parm to avoid warning message
				ubyte newpal[768];
				char basename[FILENAME_LEN];
				int nframes;
			
				strcpy(basename,AllBitmaps[i].name);
				basename[p-AllBitmaps[i].name] = 0;             //cut off "#nn" part
				
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

extern void bm_read_all(CFILE * fp);

#define HAMFILE_ID              MAKE_SIG('!','M','A','H') //HAM!
#ifdef SHAREWARE
#define HAMFILE_VERSION 2
#else
#define HAMFILE_VERSION 3
#endif
//version 1 -> 2:  save marker_model_num
//version 2 -> 3:  removed sound files

#define SNDFILE_ID              MAKE_SIG('D','N','S','D') //DSND
#define SNDFILE_VERSION 1

int read_hamfile()
{
	CFILE * ham_fp = NULL;
	int ham_id,ham_version;
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
	ham_version = cfile_read_int(ham_fp);
	if (ham_id != HAMFILE_ID || ham_version != HAMFILE_VERSION) {
		Must_write_hamfile = 1;
		cfclose(ham_fp);						//out of date ham
		return 0;
	}

	if (ham_version < 3) //mystery value
		cfseek(ham_fp, 4, SEEK_CUR);
	
	#ifndef EDITOR
	{
		int i;

		bm_read_all( ham_fp );  // Note connection to above if!!!
		printf("position: %d\n", cftell(ham_fp));
		cfread( GameBitmapXlat, sizeof(ushort)*MAX_BITMAP_FILES, 1, ham_fp );
		for (i = 0; i < MAX_BITMAP_FILES; i++) {
			//GameBitmapXlat[i] = INTEL_SHORT(GameBitmapXlat[i]);
			//printf("GameBitmapXlat[%d] = %d\n", i, GameBitmapXlat[i]);
		}
	}
	#endif

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

	for (i=0; i<N_sounds; i++ )     {
		cfread( sndh.name, 8, 1, snd_fp);
		sndh.length = cfile_read_int(snd_fp);
		sndh.data_length = cfile_read_int(snd_fp);
		sndh.offset = cfile_read_int(snd_fp);
//		cfread( &sndh, sizeof(DiskSoundHeader), 1, snd_fp );
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
		
	#ifdef EDITOR
	piggy_init_pigfile(DEFAULT_PIGFILE);
	#endif

	ham_ok = read_hamfile();

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
	
	if ( GameBitmapOffset[i] == 0 ) return;         // A read-from-disk bitmap!!!

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
			memcpy( &Piggy_bitmap_cache_data[Piggy_bitmap_cache_next], &zsize, sizeof(int) );
			Piggy_bitmap_cache_next += sizeof(int);
			descent_critical_error = 0;
			temp = cfread( &Piggy_bitmap_cache_data[Piggy_bitmap_cache_next], 1, zsize-4, Piggy_fp );
			if ( descent_critical_error )   {
				piggy_critical_error();
				goto ReDoIt;
			}
			Piggy_bitmap_cache_next += zsize-4;
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
	int bitmap_data_start,data_offset;
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
	bitmap_data_start += (Num_bitmap_files-1)*DISKBITMAPHEADER_SIZE; 
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
			p = strchr(AllBitmaps[i].name,'#');
			if (p)  {
				int n;
				p1 = p; p1++; 
				n = atoi(p1);
				*p = 0;
				if (fp2 && n==0)
					fprintf( fp2, "%s.abm\n", AllBitmaps[i].name );
				memcpy( bmh.name, AllBitmaps[i].name, 8 );
				Assert( n <= 63 );
				bmh.dflags = DBM_FLAG_ABM + n;
				*p = '#';
			}else {
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
		fwrite( &bmh, DISKBITMAPHEADER_SIZE, 1, pig_fp );                    // Mark as a bitmap
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

#endif

/*
 * reads a bitmap_index structure from a CFILE
 */
void bitmap_index_read(bitmap_index *bi, CFILE *fp)
{
	bi->index = cfile_read_short(fp);
}

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
