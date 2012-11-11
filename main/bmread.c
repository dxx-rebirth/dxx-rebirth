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
 * Routines to parse bitmaps.tbl
 * Only used for editor, since the registered version of descent 1.
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <ctype.h>

#include "pstypes.h"
#include "inferno.h"
#include "gr.h"
#include "bm.h"
#include "gamepal.h"
#include "u_mem.h"
#include "dxxerror.h"
#include "object.h"
#include "vclip.h"
#include "effects.h"
#include "polyobj.h"
#include "wall.h"
#include "textures.h"
#include "game.h"
#ifdef NETWORK
#include "multi.h"
#endif
#include "iff.h"
#include "hostage.h"
#include "powerup.h"
#include "laser.h"
#include "sounds.h"
#include "piggy.h"
#include "aistruct.h"
#include "robot.h"
#include "weapon.h"
#include "gauges.h"
#include "player.h"
#include "endlevel.h"
#include "cntrlcen.h"
#include "args.h"
#include "text.h"
#include "interp.h"
#include "strutil.h"
#include "editor/texpage.h"

#define BM_NONE			-1
#define BM_COCKPIT		 0
#define BM_TEXTURES		 2
#define BM_UNUSED		 	 3
#define BM_VCLIP		 	 4
#define BM_EFFECTS	    5
#define BM_ECLIP 	 		 6
#define BM_WEAPON			 7
#define BM_DEMO	 		 8
#define BM_ROBOTEX	    9
#define BM_WALL_ANIMS	12
#define BM_WCLIP 			13
#define BM_ROBOT			14
#define BM_GAUGES			20
#define BM_GAUGES_HIRES	21

#define MAX_BITMAPS_PER_BRUSH 30

short		N_ObjBitmapPtrs=0;
static int			Num_robot_ais = 0;
int	TmapList[MAX_TEXTURES];
char	Powerup_names[MAX_POWERUP_TYPES][POWERUP_NAME_LENGTH];
char	Robot_names[MAX_ROBOT_TYPES][ROBOT_NAME_LENGTH];

//---------------- Internal variables ---------------------------
static int			SuperX = -1;
static int			Installed=0;
static char 		*arg;
static short 		tmap_count = 0;
static short 		texture_count = 0;
static short 		clip_count = 0;
static short 		clip_num;
static short 		sound_num;
static short 		frames;
static float 		play_time;
static int			hit_sound = -1;
static sbyte 		bm_flag = BM_NONE;
static int 			abm_flag = 0;
static int 			rod_flag = 0;
static short		wall_open_sound, wall_close_sound,wall_explodes,wall_blastable, wall_hidden;
float		vlighting=0;
static int			obj_eclip;
static char 		*dest_bm;		//clip number to play when destroyed
static int			dest_vclip;		//what vclip to play when exploding
static int			dest_eclip;		//what eclip to play when exploding
static fix			dest_size;		//3d size of explosion
static int			crit_clip;		//clip number to play when destroyed
static int			crit_flag;		//flag if this is a destroyed eclip
static int			tmap1_flag;		//flag if this is used as tmap_num (not tmap_num2)
static int			num_sounds=0;

int	linenum;		//line int table currently being parsed

//------------------- Useful macros and variables ---------------

#define IFTOK(str) if (!strcmp(arg, str))

//	For the sake of LINT, defining prototypes to module's functions
void bm_read_alias(void);
void bm_read_marker(void);
void bm_read_robot_ai(int skip);
void bm_read_powerup(int unused_flag);
void bm_read_hostage(void);
void bm_read_robot(int skip);
void bm_read_weapon(int skip, int unused_flag);
void bm_read_reactor(void);
void bm_read_exitmodel(void);
void bm_read_player_ship(void);
void bm_read_some_file(int skip);
void bm_read_sound(int skip, int pc_shareware);
void bm_write_extra_robots(void);
void clear_to_end_of_line(void);
void verify_textures(void);


//---------------------------------------------------------------
int compute_average_pixel(grs_bitmap *n)
{
	int	row, column, color;
	char	*pptr;
	int	total_red, total_green, total_blue;

	pptr = (char *)n->bm_data;

	total_red = 0;
	total_green = 0;
	total_blue = 0;

	for (row=0; row<n->bm_h; row++)
		for (column=0; column<n->bm_w; column++) {
			color = *pptr++;
			total_red += gr_palette[color*3];
			total_green += gr_palette[color*3+1];
			total_blue += gr_palette[color*3+2];
		}

	total_red /= (n->bm_h * n->bm_w);
	total_green /= (n->bm_h * n->bm_w);
	total_blue /= (n->bm_h * n->bm_w);

	return BM_XRGB(total_red/2, total_green/2, total_blue/2);
}

//---------------------------------------------------------------
// Loads a bitmap from either the piggy file, a r64 file, or a
// whatever extension is passed.

bitmap_index bm_load_sub(int skip, char * filename )
{
	bitmap_index bitmap_num;
	grs_bitmap * n;
	ubyte newpal[256*3];
	int iff_error;		//reference parm to avoid warning message
	char fname[20];

	bitmap_num.index = 0;

	if (skip) {
		return bitmap_num;
	}

	d_splitpath(  filename, NULL, NULL, fname, NULL );

	bitmap_num=piggy_find_bitmap( fname );
	if (bitmap_num.index)	{
		return bitmap_num;
	}

	MALLOC( n, grs_bitmap, 1 );
	iff_error = iff_read_bitmap(filename,n,BM_LINEAR,newpal);
	n->bm_handle=0;
	if (iff_error != IFF_NO_ERROR)		{
		Error("File <%s> - IFF error: %s, line %d",filename,iff_errormsg(iff_error),linenum);
	}

	if ( iff_has_transparency )
		gr_remap_bitmap_good( n, newpal, iff_transparent_color, SuperX );
	else
		gr_remap_bitmap_good( n, newpal, -1, SuperX );

	n->avg_color = compute_average_pixel(n);

	bitmap_num = piggy_register_bitmap( n, fname, 0 );
	d_free( n );
	return bitmap_num;
}

void ab_load(int skip, char * filename, bitmap_index bmp[], int *nframes )
{
	grs_bitmap * bm[MAX_BITMAPS_PER_BRUSH];
	bitmap_index bi;
	int i;
	int iff_error;		//reference parm to avoid warning message
	ubyte newpal[768];
	char fname[20];
	char tempname[20];

	if (skip) {
		Assert( bogus_bitmap_initialized != 0 );
		bmp[0].index = 0;		//index of bogus bitmap==0 (I think)		//&bogus_bitmap;
		*nframes = 1;
		return;
	}


	d_splitpath( filename, NULL, NULL, fname, NULL );
	
	for (i=0; i<MAX_BITMAPS_PER_BRUSH; i++ )	{
		sprintf( tempname, "%s#%d", fname, i );
		bi = piggy_find_bitmap( tempname );
		if ( !bi.index )	
			break;
		bmp[i] = bi;
	}

	if (i) {
		*nframes = i;
		return;
	}

//	Note that last argument passes an address to the array newpal (which is a pointer).
//	type mismatch found using lint, will substitute this line with an adjusted
//	one.  If fatal error, then it can be easily changed.
//	iff_error = iff_read_animbrush(filename,bm,MAX_BITMAPS_PER_BRUSH,nframes,&newpal);
	iff_error = iff_read_animbrush(filename,bm,MAX_BITMAPS_PER_BRUSH,nframes,newpal);
	if (iff_error != IFF_NO_ERROR)	{
		Error("File <%s> - IFF error: %s, line %d",filename,iff_errormsg(iff_error),linenum);
	}

	for (i=0;i< *nframes; i++)	{
		bitmap_index new_bmp;
		sprintf( tempname, "%s#%d", fname, i );
		if ( iff_has_transparency )
			gr_remap_bitmap_good( bm[i], newpal, iff_transparent_color, SuperX );
		else
			gr_remap_bitmap_good( bm[i], newpal, -1, SuperX );

		bm[i]->avg_color = compute_average_pixel(bm[i]);

		new_bmp = piggy_register_bitmap( bm[i], tempname, 0 );
		d_free( bm[i] );
		bmp[i] = new_bmp;
	}
}

int ds_load(int skip, char * filename )	{
	int i;
	PHYSFS_file * cfp;
	digi_sound n;
	char fname[20];
	char rawname[100];

	if (skip) {
		// We tell piggy_register_sound it's in the pig file, when in actual fact it's in no file
		// This just tells piggy_close not to attempt to free it
		return piggy_register_sound( &bogus_sound, "bogus", 1 );
	}

	removeext(filename, fname);
	sprintf(rawname, "Sounds/%s.%s", fname, (GameArg.SndDigiSampleRate==SAMPLE_RATE_22K) ? "r22" : "raw");

	i=piggy_find_sound( fname );
	if (i!=255)	{
		return i;
	}

	cfp = PHYSFSX_openReadBuffered(rawname);

	if (cfp!=NULL) {
		n.length	= PHYSFS_fileLength( cfp );
		MALLOC( n.data, ubyte, n.length );
		PHYSFS_read( cfp, n.data, 1, n.length );
		PHYSFS_close(cfp);
		n.bits = 8;
		n.freq = 11025;
	} else {
		return 255;
	}
	i = piggy_register_sound( &n, fname, 0 );
	return i;
}

//parse a float
float get_float()
{
	char *xarg;

	xarg = strtok( NULL, space );
	return atof( xarg );
}

//parse an int
int get_int()
{
	char *xarg;

	xarg = strtok( NULL, space );
	return atoi( xarg );
}

// rotates a byte left one bit, preserving the bit falling off the right
//void
//rotate_left(char *c)
//{
//	int found;
//
//	found = 0;
//	if (*c & 0x80)
//		found = 1;
//	*c = *c << 1;
//	if (found)
//		*c |= 0x01;
//}

//loads a texture and returns the texture num
int get_texture(char *name)
{
	char short_name[FILENAME_LEN];
	int i;

	strcpy(short_name,name);
	REMOVE_DOTS(short_name);
	for (i=0;i<texture_count;i++)
		if (!d_stricmp(TmapInfo[i].filename,short_name))
			break;
	if (i==texture_count) {
		Textures[texture_count] = bm_load_sub(0, name);
		strcpy( TmapInfo[texture_count].filename, short_name);
		texture_count++;
		Assert(texture_count < MAX_TEXTURES);
		NumTextures = texture_count;
	}

	return i;
}

#define LINEBUF_SIZE 600

#define DEFAULT_PIG_PALETTE	"groupa.256"

//-----------------------------------------------------------------
// Initializes all properties and bitmaps from BITMAPS.TBL file.
// This is called when the editor is IN.
// If no editor, properties_read_cmp() is called.
int gamedata_read_tbl(int pc_shareware)
{
	PHYSFS_file	* InfoFile;
	char	inputline[LINEBUF_SIZE];
	int	i, have_bin_tbl;

	// Open BITMAPS.TBL for reading.
	have_bin_tbl = 0;
	InfoFile = PHYSFSX_openReadBuffered("BITMAPS.TBL");
	if (InfoFile == NULL) {
		InfoFile = PHYSFSX_openReadBuffered("BITMAPS.BIN");
		if (InfoFile == NULL)
			return 0;	//missing BITMAPS.TBL and BITMAPS.BIN file
		have_bin_tbl = 1;
	}

	gr_use_palette_table(DEFAULT_PIG_PALETTE);

	load_palette(DEFAULT_PIG_PALETTE,-2,0);		//special: tell palette code which pig is loaded

	for (i=0; i<MAX_SOUNDS; i++ )	{
		Sounds[i] = 255;
		AltSounds[i] = 255;
	}

	for (i=0; i<MAX_TEXTURES; i++ ) {
		TmapInfo[i].eclip_num = -1;
		TmapInfo[i].flags = 0;
		TmapInfo[i].slide_u = TmapInfo[i].slide_v = 0;
		TmapInfo[i].destroyed = -1;
	}

	for (i=0;i<MAX_REACTORS;i++)
		Reactors[i].model_num = -1;

	Num_effects = 0;
	for (i=0; i<MAX_EFFECTS; i++ ) {
		//Effects[i].bm_ptr = (grs_bitmap **) -1;
		Effects[i].changing_wall_texture = -1;
		Effects[i].changing_object_texture = -1;
		Effects[i].segnum = -1;
		Effects[i].vc.num_frames = -1;		//another mark of being unused
	}

	for (i=0;i<MAX_POLYGON_MODELS;i++)
		Dying_modelnums[i] = Dead_modelnums[i] = -1;

	Num_vclips = 0;
	for (i=0; i<VCLIP_MAXNUM; i++ )	{
		Vclip[i].num_frames = -1;
		Vclip[i].flags = 0;
	}

	for (i=0; i<MAX_WALL_ANIMS; i++ )
		WallAnims[i].num_frames = -1;
	Num_wall_anims = 0;

	if (Installed)
		return 1;

	Installed = 1;

	linenum = 0;
	
	PHYSFSX_fseek( InfoFile, 0L, SEEK_SET);

	while (PHYSFSX_fgets(inputline, LINEBUF_SIZE, InfoFile)) {
		int l;
		char *temp_ptr;
		int skip;

		linenum++;

		if (inputline[0]==' ' || inputline[0]=='\t') {
			char *t;
			for (t=inputline;*t && *t!='\n';t++)
				if (! (*t==' ' || *t=='\t')) {
					break;
				}
		}

		if (have_bin_tbl) {				// is this a binary tbl file
			decode_text_line (inputline);
		} else {
			while (inputline[(l=strlen(inputline))-2]=='\\') {
				if (!isspace(inputline[l-3])) {		//if not space before backslash...
					inputline[l-2] = ' ';				//add one
					l++;
				}
				PHYSFSX_fgets(inputline+l-2,LINEBUF_SIZE-(l-2), InfoFile);
				linenum++;
			}
		}

		REMOVE_EOL(inputline);
		if (strchr(inputline, ';')!=NULL) REMOVE_COMMENTS(inputline);

		if (strlen(inputline) == LINEBUF_SIZE-1)
			Error("Possible line truncation in BITMAPS.TBL on line %d\n",linenum);

		SuperX = -1;

		if ( (temp_ptr=strstr( inputline, "superx=" )) )	{
			SuperX = atoi( &temp_ptr[7] );
			Assert(SuperX == 254);
				//the superx color isn't kept around, so the new piggy regeneration
				//code doesn't know what it is, so it assumes that it's 254, so
				//this code requires that it be 254
										
		}

		arg = strtok( inputline, space );
		if (arg[0] == '@') {
			arg++;
			skip = pc_shareware;
		} else
			skip = 0;

		while (arg != NULL )
			{
			// Check all possible flags and defines.
			if (*arg == '$') bm_flag = BM_NONE; // reset to no flags as default.

			IFTOK("$COCKPIT") 			bm_flag = BM_COCKPIT;
			else IFTOK("$GAUGES")		{bm_flag = BM_GAUGES;   clip_count = 0;}
			else IFTOK("$GAUGES_HIRES"){bm_flag = BM_GAUGES_HIRES; clip_count = 0;}
			else IFTOK("$SOUND") 		bm_read_sound(skip, pc_shareware);
			else IFTOK("$DOOR_ANIMS")	bm_flag = BM_WALL_ANIMS;
			else IFTOK("$WALL_ANIMS")	bm_flag = BM_WALL_ANIMS;
			else IFTOK("$TEXTURES") 	bm_flag = BM_TEXTURES;
			else IFTOK("$VCLIP")			{bm_flag = BM_VCLIP;		vlighting = 0;	clip_count = 0;}
			else IFTOK("$ECLIP")			{bm_flag = BM_ECLIP;		vlighting = 0;	clip_count = 0; obj_eclip=0; dest_bm=NULL; dest_vclip=-1; dest_eclip=-1; dest_size=-1; crit_clip=-1; crit_flag=0; sound_num=-1;}
			else IFTOK("$WCLIP")			{bm_flag = BM_WCLIP;		vlighting = 0;	clip_count = 0; wall_explodes = wall_blastable = 0; wall_open_sound=wall_close_sound=-1; tmap1_flag=0; wall_hidden=0;}

			else IFTOK("$EFFECTS")		{bm_flag = BM_EFFECTS;	clip_num = 0;}
			else IFTOK("$ALIAS")			bm_read_alias();

			#ifdef EDITOR
			else IFTOK("!METALS_FLAG")		TextureMetals = texture_count;
			else IFTOK("!LIGHTS_FLAG")		TextureLights = texture_count;
			else IFTOK("!EFFECTS_FLAG")	TextureEffects = texture_count;
			#endif

			else IFTOK("lighting") 			TmapInfo[texture_count-1].lighting = fl2f(get_float());
			else IFTOK("damage") 			TmapInfo[texture_count-1].damage = fl2f(get_float());
			else IFTOK("volatile") 			TmapInfo[texture_count-1].flags |= TMI_VOLATILE;
			else IFTOK("goal_blue")			TmapInfo[texture_count-1].flags |= TMI_GOAL_BLUE;
			else IFTOK("goal_red")			TmapInfo[texture_count-1].flags |= TMI_GOAL_RED;
			else IFTOK("water")	 			TmapInfo[texture_count-1].flags |= TMI_WATER;
			else IFTOK("force_field")		TmapInfo[texture_count-1].flags |= TMI_FORCE_FIELD;
			else IFTOK("slide")	 			{TmapInfo[texture_count-1].slide_u = fl2f(get_float())>>8; TmapInfo[texture_count-1].slide_v = fl2f(get_float())>>8;}
			else IFTOK("destroyed")	 		{int t=texture_count-1; TmapInfo[t].destroyed = get_texture(strtok( NULL, space ));}
			//else IFTOK("Num_effects")		Num_effects = get_int();
			else IFTOK("Num_wall_anims")	Num_wall_anims = get_int();
			else IFTOK("clip_num")			clip_num = get_int();
			else IFTOK("dest_bm")			dest_bm = strtok( NULL, space );
			else IFTOK("dest_vclip")		dest_vclip = get_int();
			else IFTOK("dest_eclip")		dest_eclip = get_int();
			else IFTOK("dest_size")			dest_size = fl2f(get_float());
			else IFTOK("crit_clip")			crit_clip = get_int();
			else IFTOK("crit_flag")			crit_flag = get_int();
			else IFTOK("sound_num") 		sound_num = get_int();
			else IFTOK("frames") 			frames = get_int();
			else IFTOK("time") 				play_time = get_float();
			else IFTOK("obj_eclip")			obj_eclip = get_int();
			else IFTOK("hit_sound") 		hit_sound = get_int();
			else IFTOK("abm_flag")			abm_flag = get_int();
			else IFTOK("tmap1_flag")		tmap1_flag = get_int();
			else IFTOK("vlighting")			vlighting = get_float();
			else IFTOK("rod_flag")			rod_flag = get_int();
			else IFTOK("superx") 			get_int();
			else IFTOK("open_sound") 		wall_open_sound = get_int();
			else IFTOK("close_sound") 		wall_close_sound = get_int();
			else IFTOK("explodes")	 		wall_explodes = get_int();
			else IFTOK("blastable")	 		wall_blastable = get_int();
			else IFTOK("hidden")	 			wall_hidden = get_int();
			else IFTOK("$ROBOT_AI") 		bm_read_robot_ai(skip);

			else IFTOK("$POWERUP")			{bm_read_powerup(0);		continue;}
			else IFTOK("$POWERUP_UNUSED")	{bm_read_powerup(1);		continue;}
			else IFTOK("$HOSTAGE")			{bm_read_hostage();		continue;}
			else IFTOK("$ROBOT")				{bm_read_robot(skip);			continue;}
			else IFTOK("$WEAPON")			{bm_read_weapon(skip, 0);		continue;}
			else IFTOK("$WEAPON_UNUSED")	{bm_read_weapon(skip, 1);		continue;}
			else IFTOK("$REACTOR")			{bm_read_reactor();		continue;}
			else IFTOK("$MARKER")			{bm_read_marker();		continue;}
			else IFTOK("$PLAYER_SHIP")		{bm_read_player_ship();	continue;}
			else IFTOK("$EXIT") {
				if (pc_shareware)
					bm_read_exitmodel();	
				else
					clear_to_end_of_line();
				continue;
			}
			else	{		//not a special token, must be a bitmap!

				// Remove any illegal/unwanted spaces and tabs at this point.
				while ((*arg=='\t') || (*arg==' ')) arg++;
				if (*arg == '\0') { break; }	

				//check for '=' in token, indicating error
				if (strchr(arg,'='))
					Error("Unknown token <'%s'> on line %d of BITMAPS.TBL",arg,linenum);

				// Otherwise, 'arg' is apparently a bitmap filename.
				// Load bitmap and process it below:
				bm_read_some_file(skip);

			}

			arg = strtok( NULL, equal_space );
			continue;
      }
	}

	NumTextures = texture_count;
	Num_tmaps = tmap_count;

	Textures[NumTextures++].index = 0;		//entry for bogus tmap

	PHYSFS_close( InfoFile );

	Assert(N_robot_types == Num_robot_ais);		//should be one ai info per robot

	verify_textures();

	//check for refereced but unused clip count
	for (i=0; i<MAX_EFFECTS; i++ )
		if (	(
				  (Effects[i].changing_wall_texture!=-1) ||
				  (Effects[i].changing_object_texture!=-1)
             )
			 && (Effects[i].vc.num_frames==-1) )
			Error("EClip %d referenced (by polygon object?), but not defined",i);

	#ifndef NDEBUG
	{
		int used;
		for (i=used=0; i<num_sounds; i++ )
			if (Sounds[i] != 255)
				used++;

		//make sure all alt sounds refer to valid main sounds
		for (i=used=0; i<num_sounds; i++ ) {
			int alt = AltSounds[i];
			Assert(alt==0 || alt==-1 || Sounds[alt]!=255);
		}
	}
	#endif

	#ifdef EDITOR
//	piggy_dump_all();	// causes problems - we're not going to bother
	#endif

	gr_use_palette_table(D2_DEFAULT_PALETTE);

	return 0;
}

void verify_textures()
{
	grs_bitmap * bmp;
	int i,j;
	j=0;
	for (i=0; i<Num_tmaps; i++ )	{
		bmp = &GameBitmaps[Textures[i].index];
		if ( (bmp->bm_w!=64)||(bmp->bm_h!=64)||(bmp->bm_rowsize!=64) )	{
			j++;
		}
	}
	if (j)
		Error("%d textures were not 64x64.",j);

	for (i=0;i<Num_effects;i++)
		if (Effects[i].changing_object_texture != -1)
			if (GameBitmaps[ObjBitmaps[Effects[i].changing_object_texture].index].bm_w!=64 || GameBitmaps[ObjBitmaps[Effects[i].changing_object_texture].index].bm_h!=64)
				Error("Effect %d is used on object, but is not 64x64",i);

}

void bm_read_alias()
{
	char *t;

	Assert(Num_aliases < MAX_ALIASES);

	t = strtok( NULL, space );  strncpy(alias_list[Num_aliases].alias_name,t,sizeof(alias_list[Num_aliases].alias_name));
	t = strtok( NULL, space );  strncpy(alias_list[Num_aliases].file_name,t,sizeof(alias_list[Num_aliases].file_name));

	Num_aliases++;
}

void set_lighting_flag(sbyte *bp)
{
	if (vlighting < 0)
		*bp |= BM_FLAG_NO_LIGHTING;
	else
		*bp &= (0xff ^ BM_FLAG_NO_LIGHTING);
}

void set_texture_name(char *name)
{
	strcpy ( TmapInfo[texture_count].filename, name );
	REMOVE_DOTS(TmapInfo[texture_count].filename);
}

void bm_read_eclip(int skip)
{
	bitmap_index bitmap;
	int dest_bm_num = 0;

	Assert(clip_num < MAX_EFFECTS);

	if (clip_num+1 > Num_effects)
		Num_effects = clip_num+1;

	Effects[clip_num].flags = 0;

	//load the dest bitmap first, so that after this routine, the last-loaded
	//texture will be the monitor, so that lighting parameter will be applied
	//to the correct texture
	if (dest_bm) {			//deal with bitmap for blown up clip
		char short_name[FILENAME_LEN];
		int i;
		strcpy(short_name,dest_bm);
		REMOVE_DOTS(short_name);
		for (i=0;i<texture_count;i++)
			if (!d_stricmp(TmapInfo[i].filename,short_name))
				break;
		if (i==texture_count) {
			Textures[texture_count] = bm_load_sub(skip, dest_bm);
			strcpy( TmapInfo[texture_count].filename, short_name);
			texture_count++;
			Assert(texture_count < MAX_TEXTURES);
			NumTextures = texture_count;
		}
		else if (Textures[i].index == 0)		//was found, but registered out
			Textures[i] = bm_load_sub(skip, dest_bm);
		dest_bm_num = i;
	}

	if (!abm_flag)	{
		bitmap = bm_load_sub(skip, arg);

		Effects[clip_num].vc.play_time = fl2f(play_time);
		Effects[clip_num].vc.num_frames = frames;
		Effects[clip_num].vc.frame_time = fl2f(play_time)/frames;

		Assert(clip_count < frames);
		Effects[clip_num].vc.frames[clip_count] = bitmap;
		set_lighting_flag(&GameBitmaps[bitmap.index].bm_flags);

		Assert(!obj_eclip);		//obj eclips for non-abm files not supported!
		Assert(crit_flag==0);

		if (clip_count == 0) {
			Effects[clip_num].changing_wall_texture = texture_count;
			Assert(tmap_count < MAX_TEXTURES);
	  		TmapList[tmap_count++] = texture_count;
			Textures[texture_count] = bitmap;
			set_texture_name(arg);
			Assert(texture_count < MAX_TEXTURES);
			texture_count++;
			TmapInfo[texture_count].eclip_num = clip_num;
			NumTextures = texture_count;
		}

		clip_count++;

	} else {
		bitmap_index bm[MAX_BITMAPS_PER_BRUSH];
		abm_flag = 0;

		ab_load(skip, arg, bm, &Effects[clip_num].vc.num_frames );

		Effects[clip_num].vc.play_time = fl2f(play_time);
		Effects[clip_num].vc.frame_time = Effects[clip_num].vc.play_time/Effects[clip_num].vc.num_frames;

		clip_count = 0;	
		set_lighting_flag( &GameBitmaps[bm[clip_count].index].bm_flags);
		Effects[clip_num].vc.frames[clip_count] = bm[clip_count];

		if (!obj_eclip && !crit_flag) {
			Effects[clip_num].changing_wall_texture = texture_count;
			Assert(tmap_count < MAX_TEXTURES);
  			TmapList[tmap_count++] = texture_count;
			Textures[texture_count] = bm[clip_count];
			set_texture_name( arg );
			Assert(texture_count < MAX_TEXTURES);
			TmapInfo[texture_count].eclip_num = clip_num;
			texture_count++;
			NumTextures = texture_count;
		}

		if (obj_eclip) {

			if (Effects[clip_num].changing_object_texture == -1) {		//first time referenced
				Effects[clip_num].changing_object_texture = N_ObjBitmaps;		// XChange ObjectBitmaps
				N_ObjBitmaps++;
			}

			ObjBitmaps[Effects[clip_num].changing_object_texture] = Effects[clip_num].vc.frames[0];
		}

		//if for an object, Effects_bm_ptrs set in object load

		for(clip_count=1;clip_count < Effects[clip_num].vc.num_frames; clip_count++) {
			set_lighting_flag( &GameBitmaps[bm[clip_count].index].bm_flags);
			Effects[clip_num].vc.frames[clip_count] = bm[clip_count];
		}

	}

	Effects[clip_num].crit_clip = crit_clip;
	Effects[clip_num].sound_num = sound_num;

	if (dest_bm) {			//deal with bitmap for blown up clip

		Effects[clip_num].dest_bm_num = dest_bm_num;

		if (dest_vclip==-1)
			Error("Desctuction vclip missing on line %d",linenum);
		if (dest_size==-1)
			Error("Desctuction vclip missing on line %d",linenum);

		Effects[clip_num].dest_vclip = dest_vclip;
		Effects[clip_num].dest_size = dest_size;

		Effects[clip_num].dest_eclip = dest_eclip;
	}
	else {
		Effects[clip_num].dest_bm_num = -1;
		Effects[clip_num].dest_eclip = -1;
	}

	if (crit_flag)
		Effects[clip_num].flags |= EF_CRITICAL;
}


void bm_read_gauges(int skip)
{
	bitmap_index bitmap;
	int i, num_abm_frames;

	if (!abm_flag)	{
		bitmap = bm_load_sub(skip, arg);
		Assert(clip_count < MAX_GAUGE_BMS);
		Gauges[clip_count] = bitmap;
		clip_count++;
	} else {
		bitmap_index bm[MAX_BITMAPS_PER_BRUSH];
		abm_flag = 0;
		ab_load(skip, arg, bm, &num_abm_frames );
		for (i=clip_count; i<clip_count+num_abm_frames; i++) {
			Assert(i < MAX_GAUGE_BMS);
			Gauges[i] = bm[i-clip_count];
		}
		clip_count += num_abm_frames;
	}
}

void bm_read_gauges_hires()
{
	bitmap_index bitmap;
	int i, num_abm_frames;

	if (!abm_flag)	{
		bitmap = bm_load_sub(0, arg);
		Assert(clip_count < MAX_GAUGE_BMS);
		Gauges_hires[clip_count] = bitmap;
		clip_count++;
	} else {
		bitmap_index bm[MAX_BITMAPS_PER_BRUSH];
		abm_flag = 0;
		ab_load(0, arg, bm, &num_abm_frames );
		for (i=clip_count; i<clip_count+num_abm_frames; i++) {
			Assert(i < MAX_GAUGE_BMS);
			Gauges_hires[i] = bm[i-clip_count];
		}
		clip_count += num_abm_frames;
	}
}

void bm_read_wclip(int skip)
{
	bitmap_index bitmap;
	Assert(clip_num < MAX_WALL_ANIMS);

	WallAnims[clip_num].flags = 0;

	if (wall_explodes)	WallAnims[clip_num].flags |= WCF_EXPLODES;
	if (wall_blastable)	WallAnims[clip_num].flags |= WCF_BLASTABLE;
	if (wall_hidden)		WallAnims[clip_num].flags |= WCF_HIDDEN;
	if (tmap1_flag)		WallAnims[clip_num].flags |= WCF_TMAP1;

	if (!abm_flag)	{
		bitmap = bm_load_sub(skip, arg);
		if ( (WallAnims[clip_num].num_frames>-1) && (clip_count==0) )
			Error( "Wall Clip %d is already used!", clip_num );
		WallAnims[clip_num].play_time = fl2f(play_time);
		WallAnims[clip_num].num_frames = frames;
		//WallAnims[clip_num].frame_time = fl2f(play_time)/frames;
		Assert(clip_count < frames);
		WallAnims[clip_num].frames[clip_count++] = texture_count;
		WallAnims[clip_num].open_sound = wall_open_sound;
		WallAnims[clip_num].close_sound = wall_close_sound;
		Textures[texture_count] = bitmap;
		set_lighting_flag(&GameBitmaps[bitmap.index].bm_flags);
		set_texture_name( arg );
		Assert(texture_count < MAX_TEXTURES);
		texture_count++;
		NumTextures = texture_count;
		if (clip_num >= Num_wall_anims) Num_wall_anims = clip_num+1;
	} else {
		bitmap_index bm[MAX_BITMAPS_PER_BRUSH];
		int nframes;
		if ( (WallAnims[clip_num].num_frames>-1)  )
			Error( "AB_Wall clip %d is already used!", clip_num );
		abm_flag = 0;
		ab_load(0, arg, bm, &nframes );
		WallAnims[clip_num].num_frames = nframes;
		WallAnims[clip_num].play_time = fl2f(play_time);
		//WallAnims[clip_num].frame_time = fl2f(play_time)/nframes;
		WallAnims[clip_num].open_sound = wall_open_sound;
		WallAnims[clip_num].close_sound = wall_close_sound;

		WallAnims[clip_num].close_sound = wall_close_sound;
		strcpy(WallAnims[clip_num].filename, arg);
		REMOVE_DOTS(WallAnims[clip_num].filename);	

		if (clip_num >= Num_wall_anims) Num_wall_anims = clip_num+1;

		set_lighting_flag(&GameBitmaps[bm[clip_count].index].bm_flags);

		for (clip_count=0;clip_count < WallAnims[clip_num].num_frames; clip_count++)	{
			Textures[texture_count] = bm[clip_count];
			set_lighting_flag(&GameBitmaps[bm[clip_count].index].bm_flags);
			WallAnims[clip_num].frames[clip_count] = texture_count;
			REMOVE_DOTS(arg);
			sprintf( TmapInfo[texture_count].filename, "%s#%d", arg, clip_count);
			Assert(texture_count < MAX_TEXTURES);
			texture_count++;
			NumTextures = texture_count;
		}
	}
}

void bm_read_vclip(int skip)
{
	bitmap_index bi;
	Assert(clip_num < VCLIP_MAXNUM);

	if (clip_num >= Num_vclips)
		Num_vclips = clip_num+1;

	if (!abm_flag)	{
		if ( (Vclip[clip_num].num_frames>-1) && (clip_count==0)  )
			Error( "Vclip %d is already used!", clip_num );
		bi = bm_load_sub(skip, arg);
		Vclip[clip_num].play_time = fl2f(play_time);
		Vclip[clip_num].num_frames = frames;
		Vclip[clip_num].frame_time = fl2f(play_time)/frames;
		Vclip[clip_num].light_value = fl2f(vlighting);
		Vclip[clip_num].sound_num = sound_num;
		set_lighting_flag(&GameBitmaps[bi.index].bm_flags);
		Assert(clip_count < frames);
		Vclip[clip_num].frames[clip_count++] = bi;
		if (rod_flag) {
			rod_flag=0;
			Vclip[clip_num].flags |= VF_ROD;
		}			

	} else	{
		bitmap_index bm[MAX_BITMAPS_PER_BRUSH];
		abm_flag = 0;
		if ( (Vclip[clip_num].num_frames>-1)  )
			Error( "AB_Vclip %d is already used!", clip_num );
		ab_load(skip, arg, bm, &Vclip[clip_num].num_frames );

		if (rod_flag) {
			//int i;
			rod_flag=0;
			Vclip[clip_num].flags |= VF_ROD;
		}			
		Vclip[clip_num].play_time = fl2f(play_time);
		Vclip[clip_num].frame_time = fl2f(play_time)/Vclip[clip_num].num_frames;
		Vclip[clip_num].light_value = fl2f(vlighting);
		Vclip[clip_num].sound_num = sound_num;
		set_lighting_flag(&GameBitmaps[bm[clip_count].index].bm_flags);

		for (clip_count=0;clip_count < Vclip[clip_num].num_frames; clip_count++) {
			set_lighting_flag(&GameBitmaps[bm[clip_count].index].bm_flags);
			Vclip[clip_num].frames[clip_count] = bm[clip_count];
		}
	}
}

// ------------------------------------------------------------------------------
void get4fix(fix *fixp)
{
	char	*curtext;
	int	i;

	for (i=0; i<NDL; i++) {
		curtext = strtok(NULL, space);
		fixp[i] = fl2f(atof(curtext));
	}
}

// ------------------------------------------------------------------------------
void get4byte(sbyte *bytep)
{
	char	*curtext;
	int	i;

	for (i=0; i<NDL; i++) {
		curtext = strtok(NULL, space);
		bytep[i] = atoi(curtext);
	}
}

// ------------------------------------------------------------------------------
//	Convert field of view from an angle in 0..360 to cosine.
void adjust_field_of_view(fix *fovp)
{
	int		i;
	fixang	tt;
	float		ff;
	fix		temp;

	for (i=0; i<NDL; i++) {
		ff = - f2fl(fovp[i]);
		if (ff > 179) {
			ff = 179;
		}
		ff = ff/360;
		tt = fl2f(ff);
		fix_sincos(tt, &temp, &fovp[i]);
	}
}

void clear_to_end_of_line(void)
{
	arg = strtok( NULL, space );
	while (arg != NULL)
		arg = strtok( NULL, space );
}

void bm_read_sound(int skip, int pc_shareware)
{
	int sound_num;
	int alt_sound_num;

	sound_num = get_int();
	alt_sound_num = get_int();

	if ( sound_num>=MAX_SOUNDS )
		Error( "Too many sound files.\n" );

	if (sound_num >= num_sounds)
		num_sounds = sound_num+1;

	if (Sounds[sound_num] != 255)
		Error("Sound num %d already used, bitmaps.tbl, line %d\n",sound_num,linenum);

	arg = strtok(NULL, space);

	Sounds[sound_num] = ds_load(skip, arg);

	if ( alt_sound_num == 0 )
		AltSounds[sound_num] = sound_num;
	else if (alt_sound_num < 0 )
		AltSounds[sound_num] = 255;
	else
		AltSounds[sound_num] = alt_sound_num;

	if (Sounds[sound_num] == 255)
		Error("Can't load soundfile <%s>",arg);
}

// ------------------------------------------------------------------------------
void bm_read_robot_ai(int skip)	
{
	char			*robotnum_text;
	int			robotnum;
	robot_info	*robptr;

	robotnum_text = strtok(NULL, space);
	robotnum = atoi(robotnum_text);
	Assert(robotnum < MAX_ROBOT_TYPES);
	robptr = &Robot_info[robotnum];

	Assert(robotnum == Num_robot_ais);		//make sure valid number

	if (skip) {
		Num_robot_ais++;
		clear_to_end_of_line();
		return;
	}

	Num_robot_ais++;

	get4fix(robptr->field_of_view);
	get4fix(robptr->firing_wait);
	get4fix(robptr->firing_wait2);
	get4byte(robptr->rapidfire_count);
	get4fix(robptr->turn_time);
//	get4fix(robptr->fire_power);
//	get4fix(robptr->shield);
	get4fix(robptr->max_speed);
	get4fix(robptr->circle_distance);
	get4byte(robptr->evade_speed);

	robptr->always_0xabcd	= 0xabcd;

	adjust_field_of_view(robptr->field_of_view);

}

//	----------------------------------------------------------------------------------------------
//this will load a bitmap for a polygon models.  it puts the bitmap into
//the array ObjBitmaps[], and also deals with animating bitmaps
//returns a pointer to the bitmap
grs_bitmap *load_polymodel_bitmap(int skip, char *name)
{
	Assert(N_ObjBitmaps < MAX_OBJ_BITMAPS);

//	Assert( N_ObjBitmaps == N_ObjBitmapPtrs );

	if (name[0] == '%') {		//an animating bitmap!
		int eclip_num;

		eclip_num = atoi(name+1);

		if (Effects[eclip_num].changing_object_texture == -1) {		//first time referenced
			Effects[eclip_num].changing_object_texture = N_ObjBitmaps;
			ObjBitmapPtrs[N_ObjBitmapPtrs++] = N_ObjBitmaps;
			N_ObjBitmaps++;
		} else {
			ObjBitmapPtrs[N_ObjBitmapPtrs++] = Effects[eclip_num].changing_object_texture;
		}
		Assert(N_ObjBitmaps < MAX_OBJ_BITMAPS);
		Assert(N_ObjBitmapPtrs < MAX_OBJ_BITMAPS);
		return NULL;
	}
	else 	{
		ObjBitmaps[N_ObjBitmaps] = bm_load_sub(skip, name);
		if (GameBitmaps[ObjBitmaps[N_ObjBitmaps].index].bm_w!=64 || GameBitmaps[ObjBitmaps[N_ObjBitmaps].index].bm_h!=64)
			Error("Bitmap <%s> is not 64x64",name);
		ObjBitmapPtrs[N_ObjBitmapPtrs++] = N_ObjBitmaps;
		N_ObjBitmaps++;
		Assert(N_ObjBitmaps < MAX_OBJ_BITMAPS);
		Assert(N_ObjBitmapPtrs < MAX_OBJ_BITMAPS);
		return &GameBitmaps[ObjBitmaps[N_ObjBitmaps-1].index];
	}
}

#define MAX_MODEL_VARIANTS	4

// ------------------------------------------------------------------------------
void bm_read_robot(int skip)	
{
	char			*model_name[MAX_MODEL_VARIANTS];
	int			n_models,i;
	int			first_bitmap_num[MAX_MODEL_VARIANTS];
	char			*equal_ptr;
	int 			exp1_vclip_num=-1;
	int			exp1_sound_num=-1;
	int 			exp2_vclip_num=-1;
	int			exp2_sound_num=-1;
	fix			lighting = F1_0/2;		// Default
	fix			strength = F1_0*10;		// Default strength
	fix			mass = f1_0*4;
	fix			drag = f1_0/2;
	short 		weapon_type = 0, weapon_type2 = -1;
	int			g,s;
	char			name[ROBOT_NAME_LENGTH];
	int			contains_count=0, contains_id=0, contains_prob=0, contains_type=0, behavior=AIB_NORMAL;
	int			companion = 0, smart_blobs=0, energy_blobs=0, badass=0, energy_drain=0, kamikaze=0, thief=0, pursuit=0, lightcast=0, death_roll=0;
	fix			glow=0, aim=F1_0;
	int			deathroll_sound = SOUND_BOSS_SHARE_DIE;	//default
	int			score_value=1000;
	int			cloak_type=0;		//	Default = this robot does not cloak
	int			attack_type=0;		//	Default = this robot attacks by firing (1=lunge)
	int			boss_flag=0;				//	Default = robot is not a boss.
	int			see_sound = ROBOT_SEE_SOUND_DEFAULT;
	int			attack_sound = ROBOT_ATTACK_SOUND_DEFAULT;
	int			claw_sound = ROBOT_CLAW_SOUND_DEFAULT;
	int			taunt_sound = ROBOT_SEE_SOUND_DEFAULT;
	ubyte flags=0;

	Assert(N_robot_types < MAX_ROBOT_TYPES);

	if (skip) {
		Robot_info[N_robot_types].model_num = -1;
		N_robot_types++;
		Assert(N_robot_types < MAX_ROBOT_TYPES);
		clear_to_end_of_line();
		return;
	}

	model_name[0] = strtok( NULL, space );
	first_bitmap_num[0] = N_ObjBitmapPtrs;
	n_models = 1;

	// Process bitmaps
	bm_flag=BM_ROBOT;
	arg = strtok( NULL, space );
	while (arg!=NULL)	{
		equal_ptr = strchr( arg, '=' );
		if ( equal_ptr )	{
			*equal_ptr='\0';
			equal_ptr++;
			// if we have john=cool, arg is 'john' and equal_ptr is 'cool'
			if (!d_stricmp( arg, "exp1_vclip" ))	{
				exp1_vclip_num = atoi(equal_ptr);
			} else if (!d_stricmp( arg, "exp2_vclip" ))	{
				exp2_vclip_num = atoi(equal_ptr);
			} else if (!d_stricmp( arg, "exp1_sound" ))	{
				exp1_sound_num = atoi(equal_ptr);
			} else if (!d_stricmp( arg, "exp2_sound" ))	{
				exp2_sound_num = atoi(equal_ptr);
			} else if (!d_stricmp( arg, "lighting" ))	{
				lighting = fl2f(atof(equal_ptr));
				if ( (lighting < 0) || (lighting > F1_0 )) {
					Error( "In bitmaps.tbl, lighting value of %.2f is out of range 0..1.\n", f2fl(lighting));
				}
			} else if (!d_stricmp( arg, "weapon_type" )) {
				weapon_type = atoi(equal_ptr);
			} else if (!d_stricmp( arg, "weapon_type2" )) {
				weapon_type2 = atoi(equal_ptr);
			} else if (!d_stricmp( arg, "strength" )) {
				strength = i2f(atoi(equal_ptr));
			} else if (!d_stricmp( arg, "mass" )) {
				mass = fl2f(atof(equal_ptr));
			} else if (!d_stricmp( arg, "drag" )) {
				drag = fl2f(atof(equal_ptr));
			} else if (!d_stricmp( arg, "contains_id" )) {
				contains_id = atoi(equal_ptr);
			} else if (!d_stricmp( arg, "contains_type" )) {
				contains_type = atoi(equal_ptr);
			} else if (!d_stricmp( arg, "contains_count" )) {
				contains_count = atoi(equal_ptr);
			} else if (!d_stricmp( arg, "companion" )) {
				companion = atoi(equal_ptr);
			} else if (!d_stricmp( arg, "badass" )) {
				badass = atoi(equal_ptr);
			} else if (!d_stricmp( arg, "lightcast" )) {
				lightcast = atoi(equal_ptr);
			} else if (!d_stricmp( arg, "glow" )) {
				glow = fl2f(atof(equal_ptr));
			} else if (!d_stricmp( arg, "death_roll" )) {
				death_roll = atoi(equal_ptr);
			} else if (!d_stricmp( arg, "deathroll_sound" )) {
				deathroll_sound = atoi(equal_ptr);
			} else if (!d_stricmp( arg, "thief" )) {
				thief = atoi(equal_ptr);
			} else if (!d_stricmp( arg, "kamikaze" )) {
				kamikaze = atoi(equal_ptr);
			} else if (!d_stricmp( arg, "pursuit" )) {
				pursuit = atoi(equal_ptr);
			} else if (!d_stricmp( arg, "smart_blobs" )) {
				smart_blobs = atoi(equal_ptr);
			} else if (!d_stricmp( arg, "energy_blobs" )) {
				energy_blobs = atoi(equal_ptr);
			} else if (!d_stricmp( arg, "energy_drain" )) {
				energy_drain = atoi(equal_ptr);
			} else if (!d_stricmp( arg, "contains_prob" )) {
				contains_prob = atoi(equal_ptr);
			} else if (!d_stricmp( arg, "cloak_type" )) {
				cloak_type = atoi(equal_ptr);
			} else if (!d_stricmp( arg, "attack_type" )) {
				attack_type = atoi(equal_ptr);
			} else if (!d_stricmp( arg, "boss" )) {
				boss_flag = atoi(equal_ptr);
			} else if (!d_stricmp( arg, "score_value" )) {
				score_value = atoi(equal_ptr);
			} else if (!d_stricmp( arg, "see_sound" )) {
				see_sound = atoi(equal_ptr);
			} else if (!d_stricmp( arg, "attack_sound" )) {
				attack_sound = atoi(equal_ptr);
			} else if (!d_stricmp( arg, "claw_sound" )) {
				claw_sound = atoi(equal_ptr);
			} else if (!d_stricmp( arg, "taunt_sound" )) {
				taunt_sound = atoi(equal_ptr);
			} else if (!d_stricmp( arg, "aim" )) {
				aim = fl2f(atof(equal_ptr));
			} else if (!d_stricmp( arg, "big_radius" )) {
				if (atoi(equal_ptr))
					flags |= RIF_BIG_RADIUS;
			} else if (!d_stricmp( arg, "behavior" )) {
				if (!d_stricmp(equal_ptr, "STILL"))
					behavior = AIB_STILL;
				else if (!d_stricmp(equal_ptr, "NORMAL"))
					behavior = AIB_NORMAL;
				else if (!d_stricmp(equal_ptr, "BEHIND"))
					behavior = AIB_BEHIND;
				else if (!d_stricmp(equal_ptr, "RUN_FROM"))
					behavior = AIB_RUN_FROM;
				else if (!d_stricmp(equal_ptr, "SNIPE"))
					behavior = AIB_SNIPE;
				else if (!d_stricmp(equal_ptr, "STATION"))
					behavior = AIB_STATION;
				else if (!d_stricmp(equal_ptr, "FOLLOW"))
					behavior = AIB_FOLLOW;
				else
					Int3();	//	Error.  Illegal behavior type for current robot.
			} else if (!d_stricmp( arg, "name" )) {
				Assert(strlen(equal_ptr) < ROBOT_NAME_LENGTH);	//	Oops, name too long.
				strcpy(name, &equal_ptr[1]);
				name[strlen(name)-1] = 0;
			} else if (!d_stricmp( arg, "simple_model" )) {
				model_name[n_models] = equal_ptr;
				first_bitmap_num[n_models] = N_ObjBitmapPtrs;
				n_models++;
				Assert(n_models < MAX_MODEL_VARIANTS);
			} else {
				Int3();
			}		
		} else {			// Must be a texture specification...
			load_polymodel_bitmap(skip, arg);
		}
		arg = strtok( NULL, space );
	}

	//clear out anim info
	for (g=0;g<MAX_GUNS+1;g++)
		for (s=0;s<N_ANIM_STATES;s++)
			Robot_info[N_robot_types].anim_states[g][s].n_joints = 0;	//inialize to zero

	first_bitmap_num[n_models] = N_ObjBitmapPtrs;

	for (i=0;i<n_models;i++) {
		int n_textures;
		int model_num,last_model_num=0;

		n_textures = first_bitmap_num[i+1] - first_bitmap_num[i];

		model_num = load_polygon_model(model_name[i],n_textures,first_bitmap_num[i],(i==0)?&Robot_info[N_robot_types]:NULL);

		if (i==0)
			Robot_info[N_robot_types].model_num = model_num;
		else
			Polygon_models[last_model_num].simpler_model = model_num+1;

		last_model_num = model_num;
	}

	if ((glow > i2f(15)) || (glow < 0) || (glow != 0 && glow < 0x1000)) {
		Int3();
	}

	Robot_info[N_robot_types].exp1_vclip_num = exp1_vclip_num;
	Robot_info[N_robot_types].exp2_vclip_num = exp2_vclip_num;
	Robot_info[N_robot_types].exp1_sound_num = exp1_sound_num;
	Robot_info[N_robot_types].exp2_sound_num = exp2_sound_num;
	Robot_info[N_robot_types].lighting = lighting;
	Robot_info[N_robot_types].weapon_type = weapon_type;
	Robot_info[N_robot_types].weapon_type2 = weapon_type2;
	Robot_info[N_robot_types].strength = strength;
	Robot_info[N_robot_types].mass = mass;
	Robot_info[N_robot_types].drag = drag;
	Robot_info[N_robot_types].cloak_type = cloak_type;
	Robot_info[N_robot_types].attack_type = attack_type;
	Robot_info[N_robot_types].boss_flag = boss_flag;

	Robot_info[N_robot_types].contains_id = contains_id;
	Robot_info[N_robot_types].contains_count = contains_count;
	Robot_info[N_robot_types].contains_prob = contains_prob;
	Robot_info[N_robot_types].companion = companion;
	Robot_info[N_robot_types].badass = badass;
	Robot_info[N_robot_types].lightcast = lightcast;
	Robot_info[N_robot_types].glow = (glow>>12);		//convert to 4:4
	Robot_info[N_robot_types].death_roll = death_roll;
	Robot_info[N_robot_types].deathroll_sound = deathroll_sound;
	Robot_info[N_robot_types].thief = thief;
	Robot_info[N_robot_types].flags = flags;
	Robot_info[N_robot_types].kamikaze = kamikaze;
	Robot_info[N_robot_types].pursuit = pursuit;
	Robot_info[N_robot_types].smart_blobs = smart_blobs;
	Robot_info[N_robot_types].energy_blobs = energy_blobs;
	Robot_info[N_robot_types].energy_drain = energy_drain;
	Robot_info[N_robot_types].score_value = score_value;
	Robot_info[N_robot_types].see_sound = see_sound;
	Robot_info[N_robot_types].attack_sound = attack_sound;
	Robot_info[N_robot_types].claw_sound = claw_sound;
	Robot_info[N_robot_types].taunt_sound = taunt_sound;
	Robot_info[N_robot_types].behavior = behavior;		//	Default behavior for this robot, if coming out of matcen.
	Robot_info[N_robot_types].aim = min(f2i(aim*255), 255);		//	how well this robot type can aim.  255=perfect

	if (contains_type)
		Robot_info[N_robot_types].contains_type = OBJ_ROBOT;
	else
		Robot_info[N_robot_types].contains_type = OBJ_POWERUP;

	strcpy(Robot_names[N_robot_types], name);

	N_robot_types++;

	Assert(N_robot_types < MAX_ROBOT_TYPES);

	bm_flag = BM_NONE;
}

//read a reactor model
void bm_read_reactor(void)
{
	char *model_name, *model_name_dead=NULL;
	int first_bitmap_num, first_bitmap_num_dead=0, n_normal_bitmaps;
	char *equal_ptr;
	short model_num;
	fix	lighting = F1_0/2;		// Default
	int type=-1;

	Assert(Num_reactors < MAX_REACTORS);

	if (0 /*skip*/) {
		Num_reactors++;
		clear_to_end_of_line();
		return;
	}

	model_name = strtok( NULL, space );

	// Process bitmaps
	bm_flag = BM_NONE;
	arg = strtok( NULL, space );
	first_bitmap_num = N_ObjBitmapPtrs;

	while (arg!=NULL)	{

		equal_ptr = strchr( arg, '=' );

		if ( equal_ptr )	{
			*equal_ptr='\0';
			equal_ptr++;

			// if we have john=cool, arg is 'john' and equal_ptr is 'cool'

			if (!d_stricmp( arg, "dead_pof" ))	{
				model_name_dead = equal_ptr;
				first_bitmap_num_dead=N_ObjBitmapPtrs;
			} else if (!d_stricmp( arg, "lighting" ))	{
				lighting = fl2f(atof(equal_ptr));
				if ( (lighting < 0) || (lighting > F1_0 )) {
					Error( "In bitmaps.tbl, lighting value of %.2f is out of range 0..1.\n", f2fl(lighting));
				}
			} else {
				Int3();
			}		
		} else {			// Must be a texture specification...
			load_polymodel_bitmap(0, arg);
		}
		arg = strtok( NULL, space );
	}

	if ( model_name_dead )
		n_normal_bitmaps = first_bitmap_num_dead-first_bitmap_num;
	else
		n_normal_bitmaps = N_ObjBitmapPtrs-first_bitmap_num;

	model_num = load_polygon_model(model_name,n_normal_bitmaps,first_bitmap_num,NULL);

	if ( model_name_dead )
		Dead_modelnums[model_num]  = load_polygon_model(model_name_dead,N_ObjBitmapPtrs-first_bitmap_num_dead,first_bitmap_num_dead,NULL);
	else
		Dead_modelnums[model_num] = -1;

	if (type == -1)
		Error("No object type specfied for object in BITMAPS.TBL on line %d\n",linenum);

	Reactors[Num_reactors].model_num = model_num;
	Reactors[Num_reactors].n_guns = read_model_guns(model_name,Reactors[Num_reactors].gun_points,Reactors[Num_reactors].gun_dirs,NULL);

	Num_reactors++;
}

//read the marker object
void bm_read_marker()
{
	char *model_name;
	int first_bitmap_num, n_normal_bitmaps;
	char *equal_ptr;

	model_name = strtok( NULL, space );

	// Process bitmaps
	bm_flag = BM_NONE;
	arg = strtok( NULL, space );
	first_bitmap_num = N_ObjBitmapPtrs;

	while (arg!=NULL)	{

		equal_ptr = strchr( arg, '=' );

		if ( equal_ptr )	{
			*equal_ptr='\0';
			equal_ptr++;

			// if we have john=cool, arg is 'john' and equal_ptr is 'cool'
			Int3();

		} else {			// Must be a texture specification...
			load_polymodel_bitmap(0, arg);
		}
		arg = strtok( NULL, space );
	}

	n_normal_bitmaps = N_ObjBitmapPtrs-first_bitmap_num;

	Marker_model_num = load_polygon_model(model_name,n_normal_bitmaps,first_bitmap_num,NULL);
}

//read the exit model
void bm_read_exitmodel()
{
	char *model_name, *model_name_dead=NULL;
	int first_bitmap_num=0, first_bitmap_num_dead=0, n_normal_bitmaps;
	char *equal_ptr;
	short model_num;

	model_name = strtok( NULL, space );

	// Process bitmaps
	bm_flag = BM_NONE;
	arg = strtok( NULL, space );
	first_bitmap_num = N_ObjBitmapPtrs;

	while (arg!=NULL)	{

		equal_ptr = strchr( arg, '=' );

		if ( equal_ptr )	{
			*equal_ptr='\0';
			equal_ptr++;

			// if we have john=cool, arg is 'john' and equal_ptr is 'cool'

			if (!d_stricmp( arg, "dead_pof" ))	{
				model_name_dead = equal_ptr;
				first_bitmap_num_dead=N_ObjBitmapPtrs;
			} else {
				Int3();
			}		
		} else {			// Must be a texture specification...
			load_polymodel_bitmap(0, arg);
		}
		arg = strtok( NULL, space );
	}

	if ( model_name_dead )
		n_normal_bitmaps = first_bitmap_num_dead-first_bitmap_num;
	else
		n_normal_bitmaps = N_ObjBitmapPtrs-first_bitmap_num;

	model_num = load_polygon_model(model_name,n_normal_bitmaps,first_bitmap_num,NULL);

	if ( model_name_dead )
		Dead_modelnums[model_num]  = load_polygon_model(model_name_dead,N_ObjBitmapPtrs-first_bitmap_num_dead,first_bitmap_num_dead,NULL);
	else
		Dead_modelnums[model_num] = -1;

	exit_modelnum = model_num;
	destroyed_exit_modelnum = Dead_modelnums[model_num];

}

void bm_read_player_ship(void)
{
	char	*model_name_dying=NULL;
	char	*model_name[MAX_MODEL_VARIANTS];
	int	n_models=0,i;
	int	first_bitmap_num[MAX_MODEL_VARIANTS];
	char *equal_ptr;
	robot_info ri;
	int last_multi_bitmap_num=-1;

	// Process bitmaps
	bm_flag = BM_NONE;

	arg = strtok( NULL, space );

	Player_ship->mass = Player_ship->drag = 0;	//stupid defaults
	Player_ship->expl_vclip_num = -1;

	while (arg!=NULL)	{

		equal_ptr = strchr( arg, '=' );

		if ( equal_ptr )	{

			*equal_ptr='\0';
			equal_ptr++;

			// if we have john=cool, arg is 'john' and equal_ptr is 'cool'

			if (!d_stricmp( arg, "model" )) {
				Assert(n_models==0);
				model_name[0] = equal_ptr;
				first_bitmap_num[0] = N_ObjBitmapPtrs;
				n_models = 1;
			} else if (!d_stricmp( arg, "simple_model" )) {
				model_name[n_models] = equal_ptr;
				first_bitmap_num[n_models] = N_ObjBitmapPtrs;
				n_models++;
				Assert(n_models < MAX_MODEL_VARIANTS);

				if (First_multi_bitmap_num!=-1 && last_multi_bitmap_num==-1)
					last_multi_bitmap_num=N_ObjBitmapPtrs;
			}
			else if (!d_stricmp( arg, "mass" ))
				Player_ship->mass = fl2f(atof(equal_ptr));
			else if (!d_stricmp( arg, "drag" ))
				Player_ship->drag = fl2f(atof(equal_ptr));
//			else if (!d_stricmp( arg, "low_thrust" ))
//				Player_ship->low_thrust = fl2f(atof(equal_ptr));
			else if (!d_stricmp( arg, "max_thrust" ))
				Player_ship->max_thrust = fl2f(atof(equal_ptr));
			else if (!d_stricmp( arg, "reverse_thrust" ))
				Player_ship->reverse_thrust = fl2f(atof(equal_ptr));
			else if (!d_stricmp( arg, "brakes" ))
				Player_ship->brakes = fl2f(atof(equal_ptr));
			else if (!d_stricmp( arg, "wiggle" ))
				Player_ship->wiggle = fl2f(atof(equal_ptr));
			else if (!d_stricmp( arg, "max_rotthrust" ))
				Player_ship->max_rotthrust = fl2f(atof(equal_ptr));
			else if (!d_stricmp( arg, "dying_pof" ))
				model_name_dying = equal_ptr;
			else if (!d_stricmp( arg, "expl_vclip_num" ))
				Player_ship->expl_vclip_num=atoi(equal_ptr);
			else {
				Int3();
			}		
		}
		else if (!d_stricmp( arg, "multi_textures" )) {

			First_multi_bitmap_num = N_ObjBitmapPtrs;
			first_bitmap_num[n_models] = N_ObjBitmapPtrs;

		}
		else			// Must be a texture specification...

			load_polymodel_bitmap(0, arg);

		arg = strtok( NULL, space );
	}

	Assert(model_name != NULL);

	if (First_multi_bitmap_num!=-1 && last_multi_bitmap_num==-1)
		last_multi_bitmap_num=N_ObjBitmapPtrs;

	if (First_multi_bitmap_num==-1)
		first_bitmap_num[n_models] = N_ObjBitmapPtrs;

#ifdef NETWORK
	Assert(last_multi_bitmap_num-First_multi_bitmap_num == (MAX_PLAYERS-1)*2);
#endif

	for (i=0;i<n_models;i++) {
		int n_textures;
		int model_num,last_model_num=0;

		n_textures = first_bitmap_num[i+1] - first_bitmap_num[i];

		model_num = load_polygon_model(model_name[i],n_textures,first_bitmap_num[i],(i==0)?&ri:NULL);

		if (i==0)
			Player_ship->model_num = model_num;
		else
			Polygon_models[last_model_num].simpler_model = model_num+1;

		last_model_num = model_num;
	}

	if ( model_name_dying ) {
		Assert(n_models);
		Dying_modelnums[Player_ship->model_num]  = load_polygon_model(model_name_dying,first_bitmap_num[1]-first_bitmap_num[0],first_bitmap_num[0],NULL);
	}

	Assert(ri.n_guns == N_PLAYER_GUNS);

	//calc player gun positions

	{
		polymodel *pm;
		robot_info *r;
		vms_vector pnt;
		int mn;				//submodel number
		int gun_num;
	
		r = &ri;
		pm = &Polygon_models[Player_ship->model_num];

		for (gun_num=0;gun_num<r->n_guns;gun_num++) {

			pnt = r->gun_points[gun_num];
			mn = r->gun_submodels[gun_num];
		
			//instance up the tree for this gun
			while (mn != 0) {
				vm_vec_add2(&pnt,&pm->submodel_offsets[mn]);
				mn = pm->submodel_parents[mn];
			}

			Player_ship->gun_points[gun_num] = pnt;
		
		}
	}


}

void bm_read_some_file(int skip)
{

	switch (bm_flag) {
	case BM_NONE:
		Error("Trying to read bitmap <%s> with bm_flag==BM_NONE on line %d of BITMAPS.TBL",arg,linenum);
		break;
	case BM_COCKPIT:	{
		bitmap_index bitmap;
		bitmap = bm_load_sub(skip, arg);
		Assert( Num_cockpits < N_COCKPIT_BITMAPS );
		cockpit_bitmap[Num_cockpits++] = bitmap;
		//bm_flag = BM_NONE;
		return;
		}
		break;
	case BM_GAUGES:
		bm_read_gauges(skip);
		return;
		break;
	case BM_GAUGES_HIRES:
		bm_read_gauges_hires();
		return;
		break;
	case BM_WEAPON:
		bm_read_weapon(skip, 0);
		return;
		break;
	case BM_VCLIP:
		bm_read_vclip(skip);
		return;
		break;					
	case BM_ECLIP:
		bm_read_eclip(skip);
		return;
		break;
	case BM_TEXTURES:			{
		bitmap_index bitmap;
		bitmap = bm_load_sub(skip, arg);
		Assert(tmap_count < MAX_TEXTURES);
  		TmapList[tmap_count++] = texture_count;
		Textures[texture_count] = bitmap;
		set_texture_name( arg );
		Assert(texture_count < MAX_TEXTURES);
		texture_count++;
		NumTextures = texture_count;
		return;
		}
		break;
	case BM_WCLIP:
		bm_read_wclip(skip);
		return;
		break;
	}

	Error("Trying to read bitmap <%s> with unknown bm_flag <%x> on line %d of BITMAPS.TBL",arg,bm_flag,linenum);
}

// ------------------------------------------------------------------------------
//	If unused_flag is set, then this is just a placeholder.  Don't actually reference vclips or load bbms.
void bm_read_weapon(int skip, int unused_flag)
{
	int	i,n;
	int	n_models=0;
	char 	*equal_ptr;
	char	*pof_file_inner=NULL;
	char	*model_name[MAX_MODEL_VARIANTS];
	int	first_bitmap_num[MAX_MODEL_VARIANTS];
	int	lighted;					//flag for whether is a texture is lighted

	Assert(N_weapon_types < MAX_WEAPON_TYPES);

	n = N_weapon_types;
	N_weapon_types++;
	Assert(N_weapon_types <= MAX_WEAPON_TYPES);

	if (unused_flag) {
		clear_to_end_of_line();
		return;
	}

	if (skip) {
		clear_to_end_of_line();
		return;
	}

	// Initialize weapon array
	Weapon_info[n].render_type = WEAPON_RENDER_NONE;		// 0=laser, 1=blob, 2=object
	Weapon_info[n].bitmap.index = 0;
	Weapon_info[n].model_num = -1;
	Weapon_info[n].model_num_inner = -1;
	Weapon_info[n].blob_size = 0x1000;									// size of blob
	Weapon_info[n].flash_vclip = -1;
	Weapon_info[n].flash_sound = SOUND_LASER_FIRED;
	Weapon_info[n].flash_size = 0;
	Weapon_info[n].robot_hit_vclip = -1;
	Weapon_info[n].robot_hit_sound = -1;
	Weapon_info[n].wall_hit_vclip = -1;
	Weapon_info[n].wall_hit_sound = -1;
	Weapon_info[n].impact_size = 0;
	for (i=0; i<NDL; i++) {
		Weapon_info[n].strength[i] = F1_0;
		Weapon_info[n].speed[i] = F1_0*10;
	}
	Weapon_info[n].mass = F1_0;
	Weapon_info[n].thrust = 0;
	Weapon_info[n].drag = 0;
	Weapon_info[n].persistent = 0;

	Weapon_info[n].energy_usage = 0;					//	How much fuel is consumed to fire this weapon.
	Weapon_info[n].ammo_usage = 0;					//	How many units of ammunition it uses.
	Weapon_info[n].fire_wait = F1_0/4;				//	Time until this weapon can be fired again.
	Weapon_info[n].fire_count = 1;					//	Number of bursts fired from EACH GUN per firing.  For weapons which fire from both sides, 3*fire_count shots will be fired.
	Weapon_info[n].damage_radius = 0;				//	Radius of damage for missiles, not lasers.  Does damage to objects within this radius of hit point.
//--01/19/95, mk--	Weapon_info[n].damage_force = 0;					//	Force (movement) due to explosion
	Weapon_info[n].destroyable = 1;					//	Weapons default to destroyable
	Weapon_info[n].matter = 0;							//	Weapons default to not being constructed of matter (they are energy!)
	Weapon_info[n].bounce = 0;							//	Weapons default to not bouncing off walls

	Weapon_info[n].flags = 0;

	Weapon_info[n].lifetime = WEAPON_DEFAULT_LIFETIME;					//	Number of bursts fired from EACH GUN per firing.  For weapons which fire from both sides, 3*fire_count shots will be fired.

	Weapon_info[n].po_len_to_width_ratio = F1_0*10;

	Weapon_info[n].picture.index = 0;
	Weapon_info[n].hires_picture.index = 0;
	Weapon_info[n].homing_flag = 0;

	Weapon_info[n].flash = 0;
	Weapon_info[n].multi_damage_scale = F1_0;
	Weapon_info[n].afterburner_size = 0;
	Weapon_info[n].children = -1;

	// Process arguments
	arg = strtok( NULL, space );

	lighted = 1;			//assume first texture is lighted

	Weapon_info[n].speedvar = 128;

	while (arg!=NULL)	{
		equal_ptr = strchr( arg, '=' );
		if ( equal_ptr )	{
			*equal_ptr='\0';
			equal_ptr++;
			// if we have john=cool, arg is 'john' and equal_ptr is 'cool'
			if (!d_stricmp( arg, "laser_bmp" ))	{
				// Load bitmap with name equal_ptr

				Weapon_info[n].bitmap = bm_load_sub(skip, equal_ptr);		//load_polymodel_bitmap(equal_ptr);
				Weapon_info[n].render_type = WEAPON_RENDER_LASER;

			} else if (!d_stricmp( arg, "blob_bmp" ))	{
				// Load bitmap with name equal_ptr

				Weapon_info[n].bitmap = bm_load_sub(skip, equal_ptr);		//load_polymodel_bitmap(equal_ptr);
				Weapon_info[n].render_type = WEAPON_RENDER_BLOB;

			} else if (!d_stricmp( arg, "weapon_vclip" ))	{
				// Set vclip to play for this weapon.
				Weapon_info[n].bitmap.index = 0;
				Weapon_info[n].render_type = WEAPON_RENDER_VCLIP;
				Weapon_info[n].weapon_vclip = atoi(equal_ptr);

			} else if (!d_stricmp( arg, "none_bmp" )) {
				Weapon_info[n].bitmap = bm_load_sub(skip, equal_ptr);
				Weapon_info[n].render_type = WEAPON_RENDER_NONE;

			} else if (!d_stricmp( arg, "weapon_pof" ))	{
				// Load pof file
				Assert(n_models==0);
				model_name[0] = equal_ptr;
				first_bitmap_num[0] = N_ObjBitmapPtrs;
				n_models=1;
			} else if (!d_stricmp( arg, "simple_model" )) {
				model_name[n_models] = equal_ptr;
				first_bitmap_num[n_models] = N_ObjBitmapPtrs;
				n_models++;
				Assert(n_models < MAX_MODEL_VARIANTS);
			} else if (!d_stricmp( arg, "weapon_pof_inner" ))	{
				// Load pof file
				pof_file_inner = equal_ptr;
			} else if (!d_stricmp( arg, "strength" )) {
				for (i=0; i<NDL-1; i++) {
					Weapon_info[n].strength[i] = fl2f(atof(equal_ptr));
					equal_ptr = strtok(NULL, space);
				}
				Weapon_info[n].strength[i] = i2f(atoi(equal_ptr));
			} else if (!d_stricmp( arg, "mass" )) {
				Weapon_info[n].mass = fl2f(atof(equal_ptr));
			} else if (!d_stricmp( arg, "drag" )) {
				Weapon_info[n].drag = fl2f(atof(equal_ptr));
			} else if (!d_stricmp( arg, "thrust" )) {
				Weapon_info[n].thrust = fl2f(atof(equal_ptr));
			} else if (!d_stricmp( arg, "matter" )) {
				Weapon_info[n].matter = atoi(equal_ptr);
			} else if (!d_stricmp( arg, "bounce" )) {
				Weapon_info[n].bounce = atoi(equal_ptr);
			} else if (!d_stricmp( arg, "speed" )) {
				for (i=0; i<NDL-1; i++) {
					Weapon_info[n].speed[i] = i2f(atoi(equal_ptr));
					equal_ptr = strtok(NULL, space);
				}
				Weapon_info[n].speed[i] = i2f(atoi(equal_ptr));
			} else if (!d_stricmp( arg, "speedvar" ))	{
				Weapon_info[n].speedvar = (atoi(equal_ptr) * 128) / 100;
			} else if (!d_stricmp( arg, "flash_vclip" ))	{
				Weapon_info[n].flash_vclip = atoi(equal_ptr);
			} else if (!d_stricmp( arg, "flash_sound" ))	{
				Weapon_info[n].flash_sound = atoi(equal_ptr);
			} else if (!d_stricmp( arg, "flash_size" ))	{
				Weapon_info[n].flash_size = fl2f(atof(equal_ptr));
			} else if (!d_stricmp( arg, "blob_size" ))	{
				Weapon_info[n].blob_size = fl2f(atof(equal_ptr));
			} else if (!d_stricmp( arg, "robot_hit_vclip" ))	{
				Weapon_info[n].robot_hit_vclip = atoi(equal_ptr);
			} else if (!d_stricmp( arg, "robot_hit_sound" ))	{
				Weapon_info[n].robot_hit_sound = atoi(equal_ptr);
			} else if (!d_stricmp( arg, "wall_hit_vclip" ))	{
				Weapon_info[n].wall_hit_vclip = atoi(equal_ptr);
			} else if (!d_stricmp( arg, "wall_hit_sound" ))	{
				Weapon_info[n].wall_hit_sound = atoi(equal_ptr);
			} else if (!d_stricmp( arg, "impact_size" ))	{
				Weapon_info[n].impact_size = fl2f(atof(equal_ptr));
			} else if (!d_stricmp( arg, "lighted" ))	{
				lighted = atoi(equal_ptr);
			} else if (!d_stricmp( arg, "lw_ratio" ))	{
				Weapon_info[n].po_len_to_width_ratio = fl2f(atof(equal_ptr));
			} else if (!d_stricmp( arg, "lightcast" ))	{
				Weapon_info[n].light = fl2f(atof(equal_ptr));
			} else if (!d_stricmp( arg, "persistent" ))	{
				Weapon_info[n].persistent = atoi(equal_ptr);
			} else if (!d_stricmp(arg, "energy_usage" )) {
				Weapon_info[n].energy_usage = fl2f(atof(equal_ptr));
			} else if (!d_stricmp(arg, "ammo_usage" )) {
				Weapon_info[n].ammo_usage = atoi(equal_ptr);
			} else if (!d_stricmp(arg, "fire_wait" )) {
				Weapon_info[n].fire_wait = fl2f(atof(equal_ptr));
			} else if (!d_stricmp(arg, "fire_count" )) {
				Weapon_info[n].fire_count = atoi(equal_ptr);
			} else if (!d_stricmp(arg, "damage_radius" )) {
				Weapon_info[n].damage_radius = fl2f(atof(equal_ptr));
//--01/19/95, mk--			} else if (!d_stricmp(arg, "damage_force" )) {
//--01/19/95, mk--				Weapon_info[n].damage_force = fl2f(atof(equal_ptr));
			} else if (!d_stricmp(arg, "lifetime" )) {
				Weapon_info[n].lifetime = fl2f(atof(equal_ptr));
			} else if (!d_stricmp(arg, "destroyable" )) {
				Weapon_info[n].destroyable = atoi(equal_ptr);
			} else if (!d_stricmp(arg, "picture" )) {
				Weapon_info[n].picture = bm_load_sub(skip, equal_ptr);
			} else if (!d_stricmp(arg, "hires_picture" )) {
				Weapon_info[n].hires_picture = bm_load_sub(skip, equal_ptr);
			} else if (!d_stricmp(arg, "homing" )) {
				Weapon_info[n].homing_flag = !!atoi(equal_ptr);
			} else if (!d_stricmp(arg, "flash" )) {
				Weapon_info[n].flash = atoi(equal_ptr);
			} else if (!d_stricmp(arg, "multi_damage_scale" )) {
				Weapon_info[n].multi_damage_scale = fl2f(atof(equal_ptr));
			} else if (!d_stricmp(arg, "afterburner_size" )) {
				Weapon_info[n].afterburner_size = f2i(16*fl2f(atof(equal_ptr)));
			} else if (!d_stricmp(arg, "children" )) {
				Weapon_info[n].children = atoi(equal_ptr);
			} else if (!d_stricmp(arg, "placable" )) {
				if (atoi(equal_ptr)) {
					Weapon_info[n].flags |= WIF_PLACABLE;
				}
			} else {
				Int3();
			}		
		} else {			// Must be a texture specification...
			grs_bitmap *bm;

			bm = load_polymodel_bitmap(skip, arg);
			if (! lighted)
				bm->bm_flags |= BM_FLAG_NO_LIGHTING;

			lighted = 1;			//default for next bitmap is lighted
		}
		arg = strtok( NULL, space );
	}

	first_bitmap_num[n_models] = N_ObjBitmapPtrs;

	for (i=0;i<n_models;i++) {
		int n_textures;
		int model_num,last_model_num=0;

		n_textures = first_bitmap_num[i+1] - first_bitmap_num[i];

		model_num = load_polygon_model(model_name[i],n_textures,first_bitmap_num[i],NULL);

		if (i==0) {
			Weapon_info[n].render_type = WEAPON_RENDER_POLYMODEL;
			Weapon_info[n].model_num = model_num;
		}
		else
			Polygon_models[last_model_num].simpler_model = model_num+1;

		last_model_num = model_num;
	}

	if ( pof_file_inner )	{
		Assert(n_models);
		Weapon_info[n].model_num_inner = load_polygon_model(pof_file_inner,first_bitmap_num[1]-first_bitmap_num[0],first_bitmap_num[0],NULL);
	}
}

// ------------------------------------------------------------------------------
#define DEFAULT_POWERUP_SIZE i2f(3)

void bm_read_powerup(int unused_flag)
{
	int n;
	char 	*equal_ptr;

	Assert(N_powerup_types < MAX_POWERUP_TYPES);

	n = N_powerup_types;
	N_powerup_types++;

	if (unused_flag) {
		clear_to_end_of_line();
		return;
	}

	// Initialize powerup array
	Powerup_info[n].light = F1_0/3;		//	Default lighting value.
	Powerup_info[n].vclip_num = -1;
	Powerup_info[n].hit_sound = -1;
	Powerup_info[n].size = DEFAULT_POWERUP_SIZE;
	Powerup_names[n][0] = 0;

	// Process arguments
	arg = strtok( NULL, space );

	while (arg!=NULL)	{
		equal_ptr = strchr( arg, '=' );
		if ( equal_ptr )	{
			*equal_ptr='\0';
			equal_ptr++;
			// if we have john=cool, arg is 'john' and equal_ptr is 'cool'
			if (!d_stricmp( arg, "vclip_num" ))	{
				Powerup_info[n].vclip_num = atoi(equal_ptr);
			} else if (!d_stricmp( arg, "light" ))	{
				Powerup_info[n].light = fl2f(atof(equal_ptr));
			} else if (!d_stricmp( arg, "hit_sound" ))	{
				Powerup_info[n].hit_sound = atoi(equal_ptr);
			} else if (!d_stricmp( arg, "name" )) {
				Assert(strlen(equal_ptr) < POWERUP_NAME_LENGTH);	//	Oops, name too long.
				strcpy(Powerup_names[n], &equal_ptr[1]);
				Powerup_names[n][strlen(Powerup_names[n])-1] = 0;
			} else if (!d_stricmp( arg, "size" ))	{
				Powerup_info[n].size = fl2f(atof(equal_ptr));
			} else {
				Int3();
			}		
		} else {			// Must be a texture specification...
			Int3();
		}
		arg = strtok( NULL, space );
	}
}

void bm_read_hostage()
{
	int n;
	char 	*equal_ptr;

	Assert(N_hostage_types < MAX_HOSTAGE_TYPES);

	n = N_hostage_types;
	N_hostage_types++;

	// Process arguments
	arg = strtok( NULL, space );

	while (arg!=NULL)	{
		equal_ptr = strchr( arg, '=' );
		if ( equal_ptr )	{
			*equal_ptr='\0';
			equal_ptr++;

			if (!d_stricmp( arg, "vclip_num" ))

				Hostage_vclip_num[n] = atoi(equal_ptr);

			else {
				Int3();
			}

		} else {
			Int3();
		}
		arg = strtok( NULL, space );
	}
}

//these values are the number of each item in the release of d2
//extra items added after the release get written in an additional hamfile
#define N_D2_ROBOT_TYPES		66
#define N_D2_ROBOT_JOINTS		1145
#define N_D2_POLYGON_MODELS	166
#define N_D2_OBJBITMAPS			422
#define N_D2_OBJBITMAPPTRS		502
#define N_D2_WEAPON_TYPES		62

void bm_write_all(PHYSFS_file *fp)
{
	int i,t;
	PHYSFS_file *tfile;
	int s=0;

	tfile = PHYSFSX_openWriteBuffered("hamfile.lst");

	t = NumTextures-1;	//don't save bogus texture
	PHYSFS_write( fp, &t, sizeof(int), 1 );
	PHYSFS_write( fp, Textures, sizeof(bitmap_index), t );
	for (i=0;i<t;i++)
		PHYSFS_write( fp, &TmapInfo[i], sizeof(*TmapInfo)-sizeof(TmapInfo->filename)-sizeof(TmapInfo->pad2), 1 );
	PHYSFSX_printf(tfile, "NumTextures = %d, Textures array = %d, TmapInfo array = %d\n", NumTextures, (int) sizeof(bitmap_index)*NumTextures, (int) sizeof(tmap_info)*NumTextures);

	t = MAX_SOUNDS;
	PHYSFS_write( fp, &t, sizeof(int), 1 );
	PHYSFS_write( fp, Sounds, sizeof(ubyte), t );
	PHYSFS_write( fp, AltSounds, sizeof(ubyte), t );
	PHYSFSX_printf(tfile,"Num Sounds = %d, Sounds array = %d, AltSounds array = %d\n",t,t,t);

	PHYSFS_write( fp, &Num_vclips, sizeof(int), 1 );
	PHYSFS_write( fp, Vclip, sizeof(vclip), Num_vclips );
	PHYSFSX_printf(tfile, "Num_vclips = %d, Vclip array = %d\n", Num_vclips, (int) sizeof(vclip)*Num_vclips);

	PHYSFS_write( fp, &Num_effects, sizeof(int), 1 );
	PHYSFS_write( fp, Effects, sizeof(eclip), Num_effects );
	PHYSFSX_printf(tfile, "Num_effects = %d, Effects array = %d\n", Num_effects, (int) sizeof(eclip)*Num_effects);

	PHYSFS_write( fp, &Num_wall_anims, sizeof(int), 1 );
	PHYSFS_write( fp, WallAnims, sizeof(wclip), Num_wall_anims );
	PHYSFSX_printf(tfile, "Num_wall_anims = %d, WallAnims array = %d\n", Num_wall_anims, (int) sizeof(wclip)*Num_wall_anims);

	t = N_D2_ROBOT_TYPES;
	PHYSFS_write( fp, &t, sizeof(int), 1 );
	PHYSFS_write( fp, Robot_info, sizeof(robot_info), t );
	PHYSFSX_printf(tfile, "N_robot_types = %d, Robot_info array = %d\n", t, (int) sizeof(robot_info)*N_robot_types);

	t = N_D2_ROBOT_JOINTS;
	PHYSFS_write( fp, &t, sizeof(int), 1 );
	PHYSFS_write( fp, Robot_joints, sizeof(jointpos), t );
	PHYSFSX_printf(tfile, "N_robot_joints = %d, Robot_joints array = %d\n", t, (int) sizeof(jointpos)*N_robot_joints);

	t = N_D2_WEAPON_TYPES;
	PHYSFS_write( fp, &t, sizeof(int), 1 );
	PHYSFS_write( fp, Weapon_info, sizeof(weapon_info), t );
	PHYSFSX_printf(tfile, "N_weapon_types = %d, Weapon_info array = %d\n", N_weapon_types, (int) sizeof(weapon_info)*N_weapon_types);

	PHYSFS_write( fp, &N_powerup_types, sizeof(int), 1 );
	PHYSFS_write( fp, Powerup_info, sizeof(powerup_type_info), N_powerup_types );
	PHYSFSX_printf(tfile, "N_powerup_types = %d, Powerup_info array = %d\n", N_powerup_types, (int) sizeof(powerup_info)*N_powerup_types);

	t = N_D2_POLYGON_MODELS;
	PHYSFS_write( fp, &t, sizeof(int), 1 );
	PHYSFS_write( fp, Polygon_models, sizeof(polymodel), t );
	PHYSFSX_printf(tfile, "N_polygon_models = %d, Polygon_models array = %d\n", t, (int) sizeof(polymodel)*t);

	for (i=0; i<t; i++ )	{
		g3_uninit_polygon_model(Polygon_models[i].model_data);	//get RGB colors
		PHYSFS_write( fp, Polygon_models[i].model_data, sizeof(ubyte), Polygon_models[i].model_data_size);
		PHYSFSX_printf(tfile, "  Model %d, data size = %d\n", i, Polygon_models[i].model_data_size); s += Polygon_models[i].model_data_size;
		g3_init_polygon_model(Polygon_models[i].model_data);	//map colors again
	}
	PHYSFSX_printf(tfile,"Total model size = %d\n",s);

	PHYSFS_write( fp, Dying_modelnums, sizeof(int), t );
	PHYSFS_write( fp, Dead_modelnums, sizeof(int), t );
	PHYSFSX_printf(tfile, "Dying_modelnums array = %d, Dead_modelnums array = %d\n", (int) sizeof(int)*t, (int) sizeof(int)*t);

	t = MAX_GAUGE_BMS;
	PHYSFS_write( fp, &t, sizeof(int), 1 );
	PHYSFS_write( fp, Gauges, sizeof(bitmap_index), t );
	PHYSFS_write( fp, Gauges_hires, sizeof(bitmap_index), t );
	PHYSFSX_printf(tfile, "Num gauge bitmaps = %d, Gauges array = %d, Gauges_hires array = %d\n", t, (int) sizeof(bitmap_index)*t, (int) sizeof(bitmap_index)*t);

	t = MAX_OBJ_BITMAPS;
	PHYSFS_write( fp, &t, sizeof(int), 1 );
	PHYSFS_write( fp, ObjBitmaps, sizeof(bitmap_index), t );
	PHYSFS_write( fp, ObjBitmapPtrs, sizeof(ushort), t );
	PHYSFSX_printf(tfile, "Num obj bitmaps = %d, ObjBitmaps array = %d, ObjBitmapPtrs array = %d\n", t, (int) sizeof(bitmap_index)*t, (int) sizeof(ushort)*t);

	PHYSFS_write( fp, &only_player_ship, sizeof(player_ship), 1 );
	PHYSFSX_printf(tfile, "player_ship size = %d\n", (int) sizeof(player_ship));

	PHYSFS_write( fp, &Num_cockpits, sizeof(int), 1 );
	PHYSFS_write( fp, cockpit_bitmap, sizeof(bitmap_index), Num_cockpits );
	PHYSFSX_printf(tfile, "Num_cockpits = %d, cockpit_bitmaps array = %d\n", Num_cockpits, (int) sizeof(bitmap_index)*Num_cockpits);

	PHYSFS_write( fp, &First_multi_bitmap_num, sizeof(int), 1 );

	PHYSFS_write( fp, &Num_reactors, sizeof(Num_reactors), 1 );
	PHYSFS_write( fp, Reactors, sizeof(*Reactors), Num_reactors);
	PHYSFSX_printf(tfile, "Num_reactors = %d, Reactors array = %d\n", Num_reactors, (int) sizeof(*Reactors)*Num_reactors);

	PHYSFS_write( fp, &Marker_model_num, sizeof(Marker_model_num), 1);

	#ifdef SHAREWARE
	PHYSFS_write( fp, &exit_modelnum, sizeof(int), 1 );
	PHYSFS_write( fp, &destroyed_exit_modelnum, sizeof(int), 1 );
	#endif

	PHYSFS_close(tfile);

	bm_write_extra_robots();
}

void bm_write_extra_robots()
{
	PHYSFS_file *fp;
	u_int32_t t;
	int i;

	fp = PHYSFSX_openWriteBuffered("robots.ham");

	t = 0x5848414d; /* 'XHAM' */
	PHYSFS_write( fp, &t, sizeof(int), 1);
	t = 1;	//version
	PHYSFS_write( fp, &t, sizeof(int), 1);

	//write weapon info
	t = N_weapon_types - N_D2_WEAPON_TYPES;
	PHYSFS_write( fp, &t, sizeof(int), 1);
	PHYSFS_write( fp, &Weapon_info[N_D2_WEAPON_TYPES], sizeof(weapon_info), t);

	//now write robot info

	t = N_robot_types - N_D2_ROBOT_TYPES;
	PHYSFS_write( fp, &t, sizeof(int), 1);
	PHYSFS_write( fp, &Robot_info[N_D2_ROBOT_TYPES], sizeof(robot_info), t);

	t = N_robot_joints - N_D2_ROBOT_JOINTS;
	PHYSFS_write( fp, &t, sizeof(int), 1);
	PHYSFS_write( fp, &Robot_joints[N_D2_ROBOT_JOINTS], sizeof(jointpos), t);

	t = N_polygon_models - N_D2_POLYGON_MODELS;
	PHYSFS_write( fp, &t, sizeof(int), 1);
	PHYSFS_write( fp, &Polygon_models[N_D2_POLYGON_MODELS], sizeof(polymodel), t);

	for (i=N_D2_POLYGON_MODELS; i<N_polygon_models; i++ )	{
		g3_uninit_polygon_model(Polygon_models[i].model_data);	//get RGB colors
		PHYSFS_write( fp, Polygon_models[i].model_data, sizeof(ubyte), Polygon_models[i].model_data_size);
		g3_init_polygon_model(Polygon_models[i].model_data);	//map colors again
	}

	PHYSFS_write( fp, &Dying_modelnums[N_D2_POLYGON_MODELS], sizeof(int), t);
	PHYSFS_write( fp, &Dead_modelnums[N_D2_POLYGON_MODELS], sizeof(int), t);

	t = N_ObjBitmaps - N_D2_OBJBITMAPS;
	PHYSFS_write( fp, &t, sizeof(int), 1);
	PHYSFS_write( fp, &ObjBitmaps[N_D2_OBJBITMAPS], sizeof(bitmap_index), t);

	t = N_ObjBitmapPtrs - N_D2_OBJBITMAPPTRS;
	PHYSFS_write( fp, &t, sizeof(int), 1);
	PHYSFS_write( fp, &ObjBitmapPtrs[N_D2_OBJBITMAPPTRS], sizeof(ushort), t);

	PHYSFS_write( fp, ObjBitmapPtrs, sizeof(ushort), t);

	PHYSFS_close(fp);
}
