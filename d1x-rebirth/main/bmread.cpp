/*
 * Portions of this file are copyright Rebirth contributors and licensed as
 * described in COPYING.txt.
 * Portions of this file are copyright Parallax Software and licensed
 * according to the Parallax license below.
 * See COPYING.txt for license details.

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
 * Routines to parse bitmaps.tbl
 *
 */


#include <string>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <ctype.h>
#include <inttypes.h>

#include "pstypes.h"
#include "gr.h"
#include "bm.h"
#include "u_mem.h"
#include "dxxerror.h"
#include "object.h"
#include "vclip.h"
#include "effects.h"
#include "polyobj.h"
#include "wall.h"
#include "textures.h"
#include "game.h"
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
#include "fuelcen.h"
#include "endlevel.h"
#include "cntrlcen.h"
#include "args.h"
#include "text.h"
#include "physfsx.h"
#include "physfs-serial.h"
#include "strutil.h"
#if DXX_USE_EDITOR
#include "editor/texpage.h"
#endif

#include "compiler-range_for.h"
#include "partial_range.h"

static void bm_read_sound(char *&arg, int skip, int pc_shareware);
static void bm_read_robot_ai(char *&arg, int skip);
static void bm_read_robot(char *&arg, int skip);
static void bm_read_object(char *&arg, int skip);
static void bm_read_player_ship(char *&arg, int skip);
static void bm_read_some_file(const std::string &dest_bm, char *&arg, int skip);
static void bm_read_weapon(char *&arg, int skip, int unused_flag);
static void bm_read_powerup(char *&arg, int unused_flag);
static void bm_read_hostage(char *&arg);
static void verify_textures();

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

static short		N_ObjBitmaps=0;
static short		N_ObjBitmapPtrs=0;
static int			Num_robot_ais = 0;
namespace dsx {
#if DXX_USE_EDITOR
powerup_names_array Powerup_names;
robot_names_array Robot_names;
#endif
}

//---------------- Internal variables ---------------------------
static int			SuperX = -1;
static int			Installed=0;
static short 		tmap_count = 0;
static short 		texture_count = 0;
static unsigned		clip_count;
static unsigned		clip_num;
static short 		sound_num;
static uint16_t		frames;
static float 		play_time;
static int			hit_sound = -1;
static sbyte 		bm_flag = BM_NONE;
static int 			abm_flag = 0;
static int 			rod_flag = 0;
static short		wall_open_sound, wall_close_sound,wall_explodes,wall_blastable, wall_hidden;
static float		vlighting=0;
static int			obj_eclip;
static int			dest_vclip;		//what vclip to play when exploding
static int			dest_eclip;		//what eclip to play when exploding
static fix			dest_size;		//3d size of explosion
static int			crit_clip;		//clip number to play when destroyed
static int			crit_flag;		//flag if this is a destroyed eclip
static int			tmap1_flag;		//flag if this is used as tmap_num (not tmap_num2)
static int			num_sounds=0;

int	linenum;

//------------------- Useful macros and variables ---------------

#define IFTOK(str) if (!strcmp(arg, str))

//---------------------------------------------------------------
int compute_average_pixel(grs_bitmap *n)
{
	int	row, column, color;
//        char    *pptr;
	int	total_red, total_green, total_blue;

	total_red = 0;
	total_green = 0;
	total_blue = 0;

	for (row=0; row<n->bm_h; row++)
		for (column=0; column<n->bm_w; column++) {
			color = gr_gpixel (*n, column, row);
			total_red += gr_palette[color].r;
			total_green += gr_palette[color].g;
			total_blue += gr_palette[color].b;
		}

	total_red /= (n->bm_h * n->bm_w);
	total_green /= (n->bm_h * n->bm_w);
	total_blue /= (n->bm_h * n->bm_w);

	return BM_XRGB(total_red/2, total_green/2, total_blue/2);
}

//---------------------------------------------------------------
// Loads a bitmap from either the piggy file, a r64 file, or a
// whatever extension is passed.

static bitmap_index bm_load_sub(int skip, const char * filename)
{
	bitmap_index bitmap_num;
	palette_array_t newpal;
	int iff_error;		//reference parm to avoid warning message
	char fname[20];

	bitmap_num.index = 0;

	if (skip) {
		return bitmap_num;
	}

	removeext( filename, fname );

	bitmap_num=piggy_find_bitmap( fname );
	if (bitmap_num.index)	{
		return bitmap_num;
	}

	grs_bitmap n;
	iff_error = iff_read_bitmap(filename, n, &newpal);
	if (iff_error != IFF_NO_ERROR)		{
		Error("File %s - IFF error: %s",filename,iff_errormsg(iff_error));
	}

	gr_remap_bitmap_good(n, newpal, iff_has_transparency ? iff_transparent_color : -1, SuperX);

	n.avg_color = compute_average_pixel(&n);

	bitmap_num = piggy_register_bitmap(n, fname, 0);
	return bitmap_num;
}

static void ab_load(int skip, const char * filename, array<bitmap_index, MAX_BITMAPS_PER_BRUSH> &bmp, unsigned *nframes )
{
	bitmap_index bi;
	int iff_error;		//reference parm to avoid warning message
	palette_array_t newpal;
	char fname[20];
	char tempname[20];

	if (skip) {
		Assert( bogus_bitmap_initialized != 0 );
		bmp[0] = piggy_register_bitmap(bogus_bitmap, "bogus", 0);
		*nframes = 1;
		return;
	}


	removeext( filename, fname );
	
	{
	unsigned i;
	for (i=0; i<MAX_BITMAPS_PER_BRUSH; i++ )	{
		snprintf(tempname, sizeof(tempname), "%s#%d", fname, i);
		bi = piggy_find_bitmap( tempname );
		if ( !bi.index )	
			break;
		bmp[i] = bi;
	}

	if (i) {
		*nframes = i;
		return;
	}
	}

	array<std::unique_ptr<grs_main_bitmap>, MAX_BITMAPS_PER_BRUSH> bm;
	iff_error = iff_read_animbrush(filename,bm,nframes,newpal);
	if (iff_error != IFF_NO_ERROR)	{
		Error("File %s - IFF error: %s",filename,iff_errormsg(iff_error));
	}

	for (uint_fast32_t i=0;i< *nframes; i++)	{
		snprintf(tempname, sizeof(tempname), "%s#%" PRIuFAST32, fname, i);
		gr_remap_bitmap_good(*bm[i].get(), newpal, iff_has_transparency ? iff_transparent_color : -1, SuperX);
		bm[i]->avg_color = compute_average_pixel(bm[i].get());
		bmp[i] = piggy_register_bitmap(*bm[i].get(), tempname, 0);
	}
}

int ds_load(int skip, const char * filename )	{
	int i;
	digi_sound n;
	char fname[20];
	char rawname[100];

	if (skip) {
		// We tell piggy_register_sound it's in the pig file, when in actual fact it's in no file
		// This just tells piggy_close not to attempt to free it
		return piggy_register_sound( &bogus_sound, "bogus", 1 );
	}

	removeext(filename, fname);
	snprintf(rawname, sizeof(rawname), "Sounds/%s.raw", fname);

	i=piggy_find_sound( fname );
	if (i!=255)	{
		return i;
	}

	if (auto cfp = PHYSFSX_openReadBuffered(rawname))
	{
		n.length      = PHYSFS_fileLength( cfp );
		MALLOC( n.data, ubyte, n.length );
		PHYSFS_read( cfp, n.data, 1, n.length );
		n.bits = 8;
		n.freq = 11025;
	} else {
		return 255;
	}
	i = piggy_register_sound( &n, fname, 0 );
	return i;
}

//parse a float
static float get_float()
{
	char *xarg;

	xarg = strtok( NULL, space_tab );
	return atof( xarg );
}

//parse an int
static int get_int()
{
	char *xarg;

	xarg = strtok( NULL, space_tab );
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

#define LINEBUF_SIZE 600

namespace dsx {

//-----------------------------------------------------------------
// Initializes all properties and bitmaps from BITMAPS.TBL file.
int gamedata_read_tbl(int pc_shareware)
{
	std::string dest_bm;
	int	have_bin_tbl;

	ObjType[0] = OL_PLAYER;
	ObjId[0] = 0;
	Num_total_object_types = 1;

	for (unsigned i = 0; i < MAX_SOUNDS; ++i)
	{
		Sounds[i] = 255;
		AltSounds[i] = 255;
	}

	range_for (auto &ti, TmapInfo)
	{
		ti.eclip_num = eclip_none;
		ti.flags = 0;
	}

	Num_effects = 0;
	range_for (auto &ec, Effects)
	{
		//Effects[i].bm_ptr = (grs_bitmap **) -1;
		ec.changing_wall_texture = -1;
		ec.changing_object_texture = -1;
		ec.segnum = segment_none;
		ec.vc.num_frames = -1;		//another mark of being unused
	}

	for (unsigned i = 0; i < MAX_POLYGON_MODELS; ++i)
		Dying_modelnums[i] = Dead_modelnums[i] = -1;

	Num_vclips = 0;
	range_for (auto &i, Vclip)
	{
		i.num_frames = -1;
		i.flags = 0;
	}

	range_for (auto &w, WallAnims)
		w.num_frames = -1;
	Num_wall_anims = 0;

	if (Installed)
		return 1;

	Installed = 1;

	// Open BITMAPS.TBL for reading.
	have_bin_tbl = 0;
	auto InfoFile = PHYSFSX_openReadBuffered("BITMAPS.TBL");
	if (!InfoFile)
	{
		InfoFile = PHYSFSX_openReadBuffered("BITMAPS.BIN");
		if (!InfoFile)
			Error("Missing BITMAPS.TBL and BITMAPS.BIN file\n");
		have_bin_tbl = 1;
	}
	linenum = 0;
	
	PHYSFSX_fseek( InfoFile, 0L, SEEK_SET);

	PHYSFSX_gets_line_t<LINEBUF_SIZE> inputline;
	while (PHYSFSX_fgets(inputline, InfoFile)) {
		int l;
		const char *temp_ptr;
		int skip;

		linenum++;

		if (have_bin_tbl) {				// is this a binary tbl file
			decode_text_line (inputline);
		} else {
			while (inputline[(l=strlen(inputline))-2]=='\\') {
				if (!isspace(inputline[l-3])) {		//if not space before backslash...
					inputline[l-2] = ' ';				//add one
					l++;
				}
				PHYSFSX_fgets(inputline,InfoFile,l-2);
				linenum++;
			}
		}

		REMOVE_EOL(inputline);
		if (strchr(inputline, ';')!=NULL) REMOVE_COMMENTS(inputline);

		if (strlen(inputline) == LINEBUF_SIZE-1)
			Error("Possible line truncation in BITMAPS.TBL on line %d\n",linenum);

		SuperX = -1;

		if ( (temp_ptr=strstr( inputline, "superx=" )) )	{
			char *p;
			auto s = strtol(&temp_ptr[7], &p, 10);
			if (!*p)
				SuperX = s;
		}

		char *arg = strtok( inputline, space_tab );
		if (arg && arg[0] == '@') {
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
			else IFTOK("$SOUND") 		bm_read_sound(arg, skip, pc_shareware);
			else IFTOK("$DOOR_ANIMS")	bm_flag = BM_WALL_ANIMS;
			else IFTOK("$WALL_ANIMS")	bm_flag = BM_WALL_ANIMS;
			else IFTOK("$TEXTURES") 	bm_flag = BM_TEXTURES;
			else IFTOK("$VCLIP")			{bm_flag = BM_VCLIP;		vlighting = 0;	clip_count = 0;}
			else IFTOK("$ECLIP")
			{
				bm_flag = BM_ECLIP;
				vlighting = 0;
				clip_count = 0;
				obj_eclip=0;
				dest_bm.clear();
				dest_vclip=-1;
				dest_eclip=-1;
				dest_size=-1;
				crit_clip=-1;
				crit_flag=0;
				sound_num=sound_none;
			}
			else IFTOK("$WCLIP")
			{
				bm_flag = BM_WCLIP;
				vlighting = 0;
				clip_count = 0;
				wall_explodes = wall_blastable = 0;
				wall_open_sound=wall_close_sound=sound_none;
				tmap1_flag=0;
				wall_hidden=0;
			}

			else IFTOK("$EFFECTS")		{bm_flag = BM_EFFECTS;	clip_num = 0;}

#if DXX_USE_EDITOR
			else IFTOK("!METALS_FLAG")		TextureMetals = texture_count;
			else IFTOK("!LIGHTS_FLAG")		TextureLights = texture_count;
			else IFTOK("!EFFECTS_FLAG")	TextureEffects = texture_count;
			#else
			else IFTOK("!METALS_FLAG") ;
			else IFTOK("!LIGHTS_FLAG") ;
			else IFTOK("!EFFECTS_FLAG") ;
			#endif

			else IFTOK("lighting") 			TmapInfo[texture_count-1].lighting = fl2f(get_float());
			else IFTOK("damage") 			TmapInfo[texture_count-1].damage = fl2f(get_float());
			else IFTOK("volatile") 			TmapInfo[texture_count-1].flags |= TMI_VOLATILE;
			//else IFTOK("Num_effects")		Num_effects = get_int();
			else IFTOK("Num_wall_anims")	Num_wall_anims = get_int();
			else IFTOK("clip_num")			clip_num = get_int();
			else IFTOK("dest_bm")
			{
				char *p = strtok( NULL, space_tab );
				if (p)
					dest_bm = p;
				else
					dest_bm.clear();
			}
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
			else IFTOK("$ROBOT_AI") 		bm_read_robot_ai(arg, skip);

			else IFTOK("$POWERUP")			{bm_read_powerup(arg, 0);		continue;}
			else IFTOK("$POWERUP_UNUSED")	{bm_read_powerup(arg, 1);		continue;}
			else IFTOK("$HOSTAGE")			{bm_read_hostage(arg);		continue;}
			else IFTOK("$ROBOT")				{bm_read_robot(arg, skip);			continue;}
			else IFTOK("$WEAPON")			{bm_read_weapon(arg, skip, 0);		continue;}
			else IFTOK("$WEAPON_UNUSED")	{bm_read_weapon(arg, skip, 1);		continue;}
			else IFTOK("$OBJECT")			{bm_read_object(arg, skip);		continue;}
			else IFTOK("$PLAYER_SHIP")		{bm_read_player_ship(arg, skip);	continue;}

			else	{		//not a special token, must be a bitmap!

				// Remove any illegal/unwanted spaces and tabs at this point.
				while ((*arg=='\t') || (*arg==' ')) arg++;
				if (*arg == '\0') { break; }	

				// Otherwise, 'arg' is apparently a bitmap filename.
				// Load bitmap and process it below:
				bm_read_some_file(dest_bm, arg, skip);

			}

			arg = strtok( NULL, equal_space );
			continue;
      }
	}

	NumTextures = texture_count;
	Num_tmaps = tmap_count;

	Assert(N_robot_types == Num_robot_ais);		//should be one ai info per robot

	verify_textures();

	//check for refereced but unused clip count
	for (unsigned i = 0; i < MAX_EFFECTS; ++i)
		if (	(
				  (Effects[i].changing_wall_texture!=-1) ||
				  (Effects[i].changing_object_texture!=-1)
             )
			 && (Effects[i].vc.num_frames==~0u) )
			Error("EClip %d referenced (by polygon object?), but not defined",i);

	return 0;
}

}

void verify_textures()
{
	grs_bitmap * bmp;
	int j;
	j=0;
	for (uint_fast32_t i=0; i<Num_tmaps; i++ )	{
		bmp = &GameBitmaps[Textures[i].index];
		if ( (bmp->bm_w!=64)||(bmp->bm_h!=64)||(bmp->bm_rowsize!=64) )	{
			j++;
		}
	}
	if (j) Error("There are game textures that are not 64x64");
}

static void set_lighting_flag(grs_bitmap &bmp)
{
	if (vlighting < 0)
		bmp.bm_flags |= BM_FLAG_NO_LIGHTING;
	else
		bmp.bm_flags &= ~BM_FLAG_NO_LIGHTING;
}

static void set_texture_name(const char *name)
{
	TmapInfo[texture_count].filename.copy_if(name, FILENAME_LEN);
	REMOVE_DOTS(&TmapInfo[texture_count].filename[0u]);
}

static void bm_read_eclip(const std::string &dest_bm, const char *const arg, int skip)
{
	bitmap_index bitmap;

	Assert(clip_num < MAX_EFFECTS);

	if (clip_num+1 > Num_effects)
		Num_effects = clip_num+1;

	Effects[clip_num].flags = 0;

	if (!abm_flag)	{ 
		bitmap = bm_load_sub(skip, arg);

		Effects[clip_num].vc.play_time = fl2f(play_time);
		Effects[clip_num].vc.num_frames = frames;
		Effects[clip_num].vc.frame_time = fl2f(play_time)/frames;

		Assert(clip_count < frames);
		Effects[clip_num].vc.frames[clip_count] = bitmap;
		set_lighting_flag(GameBitmaps[bitmap.index]);

		Assert(!obj_eclip);		//obj eclips for non-abm files not supported!
		Assert(crit_flag==0);

		if (clip_count == 0) {
			Effects[clip_num].changing_wall_texture = texture_count;
			Assert(tmap_count < MAX_TEXTURES);
	  		tmap_count++;
			Textures[texture_count] = bitmap;
			set_texture_name(arg);
			Assert(texture_count < MAX_TEXTURES);
			texture_count++;
			TmapInfo[texture_count].eclip_num = clip_num;
			NumTextures = texture_count;
		}

		clip_count++;

	} else {
		abm_flag = 0;

		array<bitmap_index, MAX_BITMAPS_PER_BRUSH> bm;
		ab_load(skip, arg, bm, &Effects[clip_num].vc.num_frames );

		Effects[clip_num].vc.play_time = fl2f(play_time);
		Effects[clip_num].vc.frame_time = Effects[clip_num].vc.play_time/Effects[clip_num].vc.num_frames;

		clip_count = 0;	
		set_lighting_flag(GameBitmaps[bm[clip_count].index]);
		Effects[clip_num].vc.frames[clip_count] = bm[clip_count];

		if (!obj_eclip && !crit_flag) {
			Effects[clip_num].changing_wall_texture = texture_count;
			Assert(tmap_count < MAX_TEXTURES);
  			tmap_count++;
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
			set_lighting_flag(GameBitmaps[bm[clip_count].index]);
			Effects[clip_num].vc.frames[clip_count] = bm[clip_count];
		}

	}

	Effects[clip_num].crit_clip = crit_clip;
	Effects[clip_num].sound_num = sound_num;

	if (!dest_bm.empty()) {			//deal with bitmap for blown up clip
		char short_name[13];
		int i;
		strcpy(short_name,dest_bm.c_str());
		REMOVE_DOTS(short_name);
		for (i=0;i<texture_count;i++)
			if (!d_stricmp(static_cast<const char *>(TmapInfo[i].filename),short_name))
				break;
		if (i==texture_count) {
			Textures[texture_count] = bm_load_sub(skip, dest_bm.c_str());
			TmapInfo[texture_count].filename.copy_if(short_name);
			texture_count++;
			Assert(texture_count < MAX_TEXTURES);
			NumTextures = texture_count;
		}
		Effects[clip_num].dest_bm_num = i;

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
		Effects[clip_num].dest_eclip = eclip_none;
	}

	if (crit_flag)
		Effects[clip_num].flags |= EF_CRITICAL;
}


static void bm_read_gauges(const char *const arg, int skip)
{
	bitmap_index bitmap;
	unsigned i, num_abm_frames;

	if (!abm_flag)	{
		bitmap = bm_load_sub(skip, arg);
		Assert(clip_count < MAX_GAUGE_BMS);
		Gauges[clip_count] = bitmap;
		clip_count++;
	} else {
		abm_flag = 0;
		array<bitmap_index, MAX_BITMAPS_PER_BRUSH> bm;
		ab_load(skip, arg, bm, &num_abm_frames );
		for (i=clip_count; i<clip_count+num_abm_frames; i++) {
			Assert(i < MAX_GAUGE_BMS);
			Gauges[i] = bm[i-clip_count];
		}
		clip_count += num_abm_frames;
	}
}

static void bm_read_wclip(char *const arg, int skip)
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
		set_lighting_flag(GameBitmaps[bitmap.index]);
		set_texture_name( arg );
		Assert(texture_count < MAX_TEXTURES);
		texture_count++;
		NumTextures = texture_count;
		if (clip_num >= Num_wall_anims) Num_wall_anims = clip_num+1;
	} else {
		unsigned nframes;
		if ( (WallAnims[clip_num].num_frames>-1)  )
			Error( "AB_Wall clip %d is already used!", clip_num );
		abm_flag = 0;
		array<bitmap_index, MAX_BITMAPS_PER_BRUSH> bm;
		ab_load(skip, arg, bm, &nframes );
		WallAnims[clip_num].num_frames = nframes;
		WallAnims[clip_num].play_time = fl2f(play_time);
		//WallAnims[clip_num].frame_time = fl2f(play_time)/nframes;
		WallAnims[clip_num].open_sound = wall_open_sound;
		WallAnims[clip_num].close_sound = wall_close_sound;

		WallAnims[clip_num].close_sound = wall_close_sound;
		strcpy(&WallAnims[clip_num].filename[0], arg);
		REMOVE_DOTS(&WallAnims[clip_num].filename[0]);	

		if (clip_num >= Num_wall_anims) Num_wall_anims = clip_num+1;

		set_lighting_flag(GameBitmaps[bm[clip_count].index]);

		for (clip_count=0;clip_count < WallAnims[clip_num].num_frames; clip_count++)	{
			Textures[texture_count] = bm[clip_count];
			set_lighting_flag(GameBitmaps[bm[clip_count].index]);
			WallAnims[clip_num].frames[clip_count] = texture_count;
			REMOVE_DOTS(arg);
			snprintf(&TmapInfo[texture_count].filename[0u], TmapInfo[texture_count].filename.size(), "%s#%d", arg, clip_count);
			Assert(texture_count < MAX_TEXTURES);
			texture_count++;
			NumTextures = texture_count;
		}
	}
}

static void bm_read_vclip(const char *const arg, int skip)
{
	bitmap_index bi;
	Assert(clip_num < VCLIP_MAXNUM);

	if (!abm_flag)	{
		if ( (Vclip[clip_num].num_frames != ~0u) && (clip_count==0)  )
			Error( "Vclip %d is already used!", clip_num );
		bi = bm_load_sub(skip, arg);
		Vclip[clip_num].play_time = fl2f(play_time);
		Vclip[clip_num].num_frames = frames;
		Vclip[clip_num].frame_time = fl2f(play_time)/frames;
		Vclip[clip_num].light_value = fl2f(vlighting);
		Vclip[clip_num].sound_num = sound_num;
		set_lighting_flag(GameBitmaps[bi.index]);
		Assert(clip_count < frames);
		Vclip[clip_num].frames[clip_count++] = bi;
		if (rod_flag) {
			rod_flag=0;
			Vclip[clip_num].flags |= VF_ROD;
		}			

	} else	{
		abm_flag = 0;
		if ( (Vclip[clip_num].num_frames != ~0u)  )
			Error( "AB_Vclip %d is already used!", clip_num );
		array<bitmap_index, MAX_BITMAPS_PER_BRUSH> bm;
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
		set_lighting_flag(GameBitmaps[bm[clip_count].index]);

		for (clip_count=0;clip_count < Vclip[clip_num].num_frames; clip_count++) {
			set_lighting_flag(GameBitmaps[bm[clip_count].index]);
			Vclip[clip_num].frames[clip_count] = bm[clip_count];
		}
	}
}

// ------------------------------------------------------------------------------
static void get4fix(array<fix, NDL> &fixp)
{
	char	*curtext;
	range_for (auto &i, fixp)
	{
		curtext = strtok(NULL, space_tab);
		i = fl2f(atof(curtext));
	}
}

// ------------------------------------------------------------------------------
static void get4byte(array<int8_t, NDL> &bytep)
{
	char	*curtext;
	range_for (auto &i, bytep)
	{
		curtext = strtok(NULL, space_tab);
		i = atoi(curtext);
	}
}

// ------------------------------------------------------------------------------
//	Convert field of view from an angle in 0..360 to cosine.
static void adjust_field_of_view(array<fix, NDL> &fovp)
{
	fixang	tt;
	float		ff;
	range_for (auto &i, fovp)
	{
		ff = - f2fl(i);
		if (ff > 179) {
			ff = 179;
		}
		ff = ff/360;
		tt = fl2f(ff);
		i = fix_cos(tt);
	}
}

static void clear_to_end_of_line(char *&arg)
{
	arg = NULL;
}

static void bm_read_sound(char *&arg, int skip, int pc_shareware)
{
	int alt_sound_num;

	const int read_sound_num = get_int();
	alt_sound_num = pc_shareware ? read_sound_num : get_int();

	if ( read_sound_num>=MAX_SOUNDS )
		Error( "Too many sound files.\n" );

	if (read_sound_num >= num_sounds)
		num_sounds = read_sound_num+1;

	arg = strtok(NULL, space_tab);

	Sounds[read_sound_num] = ds_load(skip, arg);

	if ( alt_sound_num == 0 )
		AltSounds[read_sound_num] = sound_num;
	else if (alt_sound_num < 0 )
		AltSounds[read_sound_num] = 255;
	else
		AltSounds[read_sound_num] = alt_sound_num;

	if (Sounds[read_sound_num] == 255)
		Error("Can't load soundfile <%s>",arg);
}

// ------------------------------------------------------------------------------
static void bm_read_robot_ai(char *&arg, int skip)	
{
	char			*robotnum_text;
	int			robotnum;
	robot_info	*robptr;

	robotnum_text = strtok(NULL, space_tab);
	robotnum = atoi(robotnum_text);
	Assert(robotnum < MAX_ROBOT_TYPES);
	robptr = &Robot_info[robotnum];

	Assert(robotnum == Num_robot_ais);		//make sure valid number

	if (skip) {
		Num_robot_ais++;
		clear_to_end_of_line(arg);
		return;
	}

	Num_robot_ais++;

	get4fix(robptr->field_of_view);
	get4fix(robptr->firing_wait);
	get4byte(robptr->rapidfire_count);
	get4fix(robptr->turn_time);
	array<fix, NDL>		fire_power,						//	damage done by a hit from this robot
		shield;							//	shield strength of this robot
	get4fix(fire_power);
	get4fix(shield);
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
static grs_bitmap *load_polymodel_bitmap(int skip, const char *name)
{
	assert(N_ObjBitmaps < ObjBitmaps.size());

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
		return NULL;
	}
	else 	{
		ObjBitmaps[N_ObjBitmaps] = bm_load_sub(skip, name);
		ObjBitmapPtrs[N_ObjBitmapPtrs++] = N_ObjBitmaps;
		N_ObjBitmaps++;
		return &GameBitmaps[ObjBitmaps[N_ObjBitmaps-1].index];
	}
}

#define MAX_MODEL_VARIANTS	4

// ------------------------------------------------------------------------------
static void bm_read_robot(char *&arg, int skip)	
{
	char			*model_name[MAX_MODEL_VARIANTS];
	int			n_models,i;
	int			first_bitmap_num[MAX_MODEL_VARIANTS];
	char			*equal_ptr;
	auto 			exp1_vclip_num=vclip_none;
	auto			exp1_sound_num=sound_none;
	auto 			exp2_vclip_num=vclip_none;
	auto			exp2_sound_num=sound_none;
	fix			lighting = F1_0/2;		// Default
	fix			strength = F1_0*10;		// Default strength
	fix			mass = f1_0*4;
	fix			drag = f1_0/2;
	weapon_id_type weapon_type = weapon_id_type::LASER_ID_L1;
	int			contains_count=0, contains_id=0, contains_prob=0, contains_type=0;
	int			score_value=1000;
	int			cloak_type=0;		//	Default = this robot does not cloak
	int			attack_type=0;		//	Default = this robot attacks by firing (1=lunge)
	int			boss_flag=0;				//	Default = robot is not a boss.
	int			see_sound = ROBOT_SEE_SOUND_DEFAULT;
	int			attack_sound = ROBOT_ATTACK_SOUND_DEFAULT;
	int			claw_sound = ROBOT_CLAW_SOUND_DEFAULT;

	Assert(N_robot_types < MAX_ROBOT_TYPES);

	if (skip) {
		Robot_info[N_robot_types].model_num = -1;
		N_robot_types++;
		Num_total_object_types++;
		clear_to_end_of_line(arg);
		return;
	}

	model_name[0] = strtok( NULL, space_tab );
	first_bitmap_num[0] = N_ObjBitmapPtrs;
	n_models = 1;

	// Process bitmaps
	bm_flag=BM_ROBOT;
	arg = strtok( NULL, space_tab );
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
				weapon_type = static_cast<weapon_id_type>(atoi(equal_ptr));
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
			} else if (!d_stricmp( arg, "name" )) {
#if DXX_USE_EDITOR
				auto &name = Robot_names[N_robot_types];
				const auto len = strlen(equal_ptr);
				assert(len < name.size());	//	Oops, name too long.
				memcpy(name.data(), &equal_ptr[1], len - 2);
				name[len - 2] = 0;
#endif
			} else if (!d_stricmp( arg, "simple_model" )) {
				model_name[n_models] = equal_ptr;
				first_bitmap_num[n_models] = N_ObjBitmapPtrs;
				n_models++;
			}
		} else {			// Must be a texture specification...
			load_polymodel_bitmap(skip, arg);
		}
		arg = strtok( NULL, space_tab );
	}

	//clear out anim info
	range_for (auto &g, Robot_info[N_robot_types].anim_states)
		range_for (auto &s, g)
			s.n_joints = 0;	//inialize to zero

	first_bitmap_num[n_models] = N_ObjBitmapPtrs;

	for (i=0;i<n_models;i++) {
		int n_textures;
		int model_num,last_model_num=0;

		n_textures = first_bitmap_num[i+1] - first_bitmap_num[i];

		robot_info *ri = NULL;
		if (i == 0)
			ri = &Robot_info[N_robot_types];
		model_num = load_polygon_model(model_name[i],n_textures,first_bitmap_num[i],ri);

		if (i==0)
			Robot_info[N_robot_types].model_num = model_num;
		else
			Polygon_models[last_model_num].simpler_model = model_num+1;

		last_model_num = model_num;
	}

	ObjType[Num_total_object_types] = OL_ROBOT;
	ObjId[Num_total_object_types] = N_robot_types;

	Robot_info[N_robot_types].exp1_vclip_num = exp1_vclip_num;
	Robot_info[N_robot_types].exp2_vclip_num = exp2_vclip_num;
	Robot_info[N_robot_types].exp1_sound_num = exp1_sound_num;
	Robot_info[N_robot_types].exp2_sound_num = exp2_sound_num;
	Robot_info[N_robot_types].lighting = lighting;
	Robot_info[N_robot_types].weapon_type = weapon_type;
	Robot_info[N_robot_types].strength = strength;
	Robot_info[N_robot_types].mass = mass;
	Robot_info[N_robot_types].drag = drag;
	Robot_info[N_robot_types].cloak_type = cloak_type;
	Robot_info[N_robot_types].attack_type = attack_type;
	Robot_info[N_robot_types].boss_flag = boss_flag;

	Robot_info[N_robot_types].contains_id = contains_id;
	Robot_info[N_robot_types].contains_count = contains_count;
	Robot_info[N_robot_types].contains_prob = contains_prob;
	Robot_info[N_robot_types].score_value = score_value;
	Robot_info[N_robot_types].see_sound = see_sound;
	Robot_info[N_robot_types].attack_sound = attack_sound;
	Robot_info[N_robot_types].claw_sound = claw_sound;

	if (contains_type)
		Robot_info[N_robot_types].contains_type = OBJ_ROBOT;
	else
		Robot_info[N_robot_types].contains_type = OBJ_POWERUP;

	N_robot_types++;
	Num_total_object_types++;
}

//read a polygon object of some sort
void bm_read_object(char *&arg, int skip)
{
	char *model_name, *model_name_dead=NULL;
        int first_bitmap_num, first_bitmap_num_dead=0, n_normal_bitmaps;
	char *equal_ptr;
	short model_num;
	fix	lighting = F1_0/2;		// Default
	int type=-1;
	fix strength=0;

	model_name = strtok( NULL, space_tab );

	// Process bitmaps
	bm_flag = BM_NONE;
	arg = strtok( NULL, space_tab );
	first_bitmap_num = N_ObjBitmapPtrs;

	while (arg!=NULL)	{

		equal_ptr = strchr( arg, '=' );

		if ( equal_ptr )	{
			*equal_ptr='\0';
			equal_ptr++;

			// if we have john=cool, arg is 'john' and equal_ptr is 'cool'

			if (!d_stricmp(arg,"type")) {
				if (!d_stricmp(equal_ptr,"controlcen"))
					type = OL_CONTROL_CENTER;
				else if (!d_stricmp(equal_ptr,"clutter"))
					type = OL_CLUTTER;
				else if (!d_stricmp(equal_ptr,"exit"))
					type = OL_EXIT;
			} else if (!d_stricmp( arg, "dead_pof" ))	{
				model_name_dead = equal_ptr;
				first_bitmap_num_dead=N_ObjBitmapPtrs;
			} else if (!d_stricmp( arg, "lighting" ))	{
				lighting = fl2f(atof(equal_ptr));
				if ( (lighting < 0) || (lighting > F1_0 )) {
					Error( "In bitmaps.tbl, lighting value of %.2f is out of range 0..1.\n", f2fl(lighting));
				}
			} else if (!d_stricmp( arg, "strength" )) {
				strength = fl2f(atof(equal_ptr));
			}
		} else {			// Must be a texture specification...
			load_polymodel_bitmap(skip, arg);
		}
		arg = strtok( NULL, space_tab );
	}

	if ( model_name_dead )
		n_normal_bitmaps = first_bitmap_num_dead-first_bitmap_num;
	else
		n_normal_bitmaps = N_ObjBitmapPtrs-first_bitmap_num;

	model_num = load_polygon_model(model_name,n_normal_bitmaps,first_bitmap_num,NULL);

	if (type == OL_CONTROL_CENTER)
		read_model_guns(model_name, Reactors[0]);

	if ( model_name_dead )
		Dead_modelnums[model_num]  = load_polygon_model(model_name_dead,N_ObjBitmapPtrs-first_bitmap_num_dead,first_bitmap_num_dead,NULL);
	else
		Dead_modelnums[model_num] = -1;

	if (type == -1)
		Error("No object type specfied for object in BITMAPS.TBL on line %d\n",linenum);

	ObjType[Num_total_object_types] = type;
	ObjId[Num_total_object_types] = model_num;
	ObjStrength[Num_total_object_types] = strength;

	Num_total_object_types++;

	if (type == OL_EXIT) {
		exit_modelnum = model_num;
		destroyed_exit_modelnum = Dead_modelnums[model_num];
	}

}

void bm_read_player_ship(char *&arg, int skip)
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

	arg = strtok( NULL, space_tab );

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
		}
		else if (!d_stricmp( arg, "multi_textures" )) {

			First_multi_bitmap_num = N_ObjBitmapPtrs;
			first_bitmap_num[n_models] = N_ObjBitmapPtrs;

		}
		else			// Must be a texture specification...

			load_polymodel_bitmap(skip, arg);

		arg = strtok( NULL, space_tab );
	}
	if (First_multi_bitmap_num!=-1 && last_multi_bitmap_num==-1)
		last_multi_bitmap_num=N_ObjBitmapPtrs;

	if (First_multi_bitmap_num==-1)
		first_bitmap_num[n_models] = N_ObjBitmapPtrs;

	Assert(last_multi_bitmap_num-First_multi_bitmap_num == (MAX_PLAYERS-1)*2);

	for (i=0;i<n_models;i++) {
		int n_textures;
		int model_num,last_model_num=0;

		n_textures = first_bitmap_num[i+1] - first_bitmap_num[i];

		robot_info *pri = NULL;
		if (i == 0)
			pri = &ri;
		model_num = load_polygon_model(model_name[i],n_textures,first_bitmap_num[i],pri);

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
				vm_vec_add2(pnt,pm->submodel_offsets[mn]);
				mn = pm->submodel_parents[mn];
			}

			Player_ship->gun_points[gun_num] = pnt;
		
		}
	}


}

void bm_read_some_file(const std::string &dest_bm, char *&arg, int skip)
{
	switch (bm_flag) {
	case BM_COCKPIT:	{
		bitmap_index bitmap;
		bitmap = bm_load_sub(skip, arg);
		Assert( Num_cockpits < N_COCKPIT_BITMAPS );
		cockpit_bitmap[Num_cockpits++] = bitmap;

		//bm_flag = BM_NONE;
		}
		break;
	case BM_GAUGES:
		bm_read_gauges(arg, skip);
		break;
	case BM_WEAPON:
		bm_read_weapon(arg, skip, 0);
		break;
	case BM_VCLIP:
		bm_read_vclip(arg, skip);
		break;					
	case BM_ECLIP:
		bm_read_eclip(dest_bm, arg, skip);
		break;
	case BM_TEXTURES:			{
		bitmap_index bitmap;
		bitmap = bm_load_sub(skip, arg);
		Assert(tmap_count < MAX_TEXTURES);
  		tmap_count++;
		Textures[texture_count] = bitmap;
		set_texture_name( arg );
		Assert(texture_count < MAX_TEXTURES);
		texture_count++;
		NumTextures = texture_count;
		}
		break;
	case BM_WCLIP:
		bm_read_wclip(arg, skip);
		break;
	default:
		break;
	}
}

// ------------------------------------------------------------------------------
//	If unused_flag is set, then this is just a placeholder.  Don't actually reference vclips or load bbms.
void bm_read_weapon(char *&arg, int skip, int unused_flag)
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

	if (unused_flag) {
		clear_to_end_of_line(arg);
		return;
	}

	if (skip) {
		clear_to_end_of_line(arg);
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
	Weapon_info[n].robot_hit_vclip = vclip_none;
	Weapon_info[n].robot_hit_sound = sound_none;
	Weapon_info[n].wall_hit_vclip = vclip_none;
	Weapon_info[n].wall_hit_sound = sound_none;
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

	Weapon_info[n].lifetime = WEAPON_DEFAULT_LIFETIME;					//	Number of bursts fired from EACH GUN per firing.  For weapons which fire from both sides, 3*fire_count shots will be fired.

	Weapon_info[n].po_len_to_width_ratio = F1_0*10;

	Weapon_info[n].picture.index = 0;
	Weapon_info[n].homing_flag = 0;

	// Process arguments
	arg = strtok( NULL, space_tab );

	lighted = 1;			//assume first texture is lighted

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
			} else if (!d_stricmp( arg, "weapon_pof_inner" ))	{
				// Load pof file
				pof_file_inner = equal_ptr;
			} else if (!d_stricmp( arg, "strength" )) {
				for (i=0; i<NDL-1; i++) {
					Weapon_info[n].strength[i] = i2f(atoi(equal_ptr));
					equal_ptr = strtok(NULL, space_tab);
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
					equal_ptr = strtok(NULL, space_tab);
				}
				Weapon_info[n].speed[i] = i2f(atoi(equal_ptr));
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
			} else if (!d_stricmp(arg, "homing" )) {
				Weapon_info[n].homing_flag = !!atoi(equal_ptr);
			}
		} else {			// Must be a texture specification...
			grs_bitmap *bm;

			bm = load_polymodel_bitmap(skip, arg);
			if (bm && ! lighted)
				bm->bm_flags |= BM_FLAG_NO_LIGHTING;

			lighted = 1;			//default for next bitmap is lighted
		}
		arg = strtok( NULL, space_tab );
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

void bm_read_powerup(char *&arg, int unused_flag)
{
	int n;
	char 	*equal_ptr;

	Assert(N_powerup_types < MAX_POWERUP_TYPES);

	n = N_powerup_types;
	N_powerup_types++;

	if (unused_flag) {
		clear_to_end_of_line(arg);
		return;
	}

	// Initialize powerup array
	Powerup_info[n].light = F1_0/3;		//	Default lighting value.
	Powerup_info[n].vclip_num = vclip_none;
	Powerup_info[n].hit_sound = sound_none;
	Powerup_info[n].size = DEFAULT_POWERUP_SIZE;
#if DXX_USE_EDITOR
	Powerup_names[n][0] = 0;
#endif

	// Process arguments
	arg = strtok( NULL, space_tab );

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
#if DXX_USE_EDITOR
				auto &name = Powerup_names[n];
				const auto len = strlen(equal_ptr);
				assert(len < name.size());	//	Oops, name too long.
				memcpy(name.data(), &equal_ptr[1], len - 2);
				name[len - 2] = 0;
#endif
			} else if (!d_stricmp( arg, "size" ))	{
				Powerup_info[n].size = fl2f(atof(equal_ptr));
			}
		}
		arg = strtok( NULL, space_tab );
	}

	ObjType[Num_total_object_types] = OL_POWERUP;
	ObjId[Num_total_object_types] = n;
	Num_total_object_types++;

}

void bm_read_hostage(char *&arg)
{
	int n;
	char 	*equal_ptr;

	Assert(N_hostage_types < MAX_HOSTAGE_TYPES);

	n = N_hostage_types;
	N_hostage_types++;

	// Process arguments
	arg = strtok( NULL, space_tab );

	while (arg!=NULL)	{
		equal_ptr = strchr( arg, '=' );
		if ( equal_ptr )	{
			*equal_ptr='\0';
			equal_ptr++;

			if (!d_stricmp( arg, "vclip_num" ))
				Hostage_vclip_num[n] = atoi(equal_ptr);
		}

		arg = strtok( NULL, space_tab );
	}

	ObjType[Num_total_object_types] = OL_HOSTAGE;
	ObjId[Num_total_object_types] = n;
	Num_total_object_types++;

}

DEFINE_SERIAL_UDT_TO_MESSAGE(tmap_info, t, (static_cast<const array<char, 13> &>(t.filename), t.flags, t.lighting, t.damage, t.eclip_num));
ASSERT_SERIAL_UDT_MESSAGE_SIZE(tmap_info, 26);

#if 0
static void tmap_info_write(PHYSFS_File *fp, const tmap_info &ti)
{
	PHYSFSX_serialize_write(fp, ti);
}

void bm_write_all(PHYSFS_File *fp)
{
	PHYSFS_write( fp, &NumTextures, sizeof(int), 1);
	range_for (const bitmap_index &bi, Textures)
		PHYSFS_write( fp, &bi, sizeof(bi), 1);
	range_for (const tmap_info &ti, TmapInfo)
		tmap_info_write(fp, ti);

	PHYSFS_write( fp, Sounds, sizeof(ubyte), MAX_SOUNDS);
	PHYSFS_write( fp, AltSounds, sizeof(ubyte), MAX_SOUNDS);

	PHYSFS_write( fp, &Num_vclips, sizeof(int), 1);
	range_for (const vclip &vc, Vclip)
		vclip_write(fp, vc);

	PHYSFS_write( fp, &Num_effects, sizeof(int), 1);
	range_for (const eclip &ec, Effects)
		eclip_write(fp, ec);

	PHYSFS_write( fp, &Num_wall_anims, sizeof(int), 1);
	range_for (const auto &w, WallAnims)
		wclip_write(fp, w);

	PHYSFS_write( fp, &N_robot_types, sizeof(int), 1);
	PHYSFS_write( fp, Robot_info, sizeof(robot_info), MAX_ROBOT_TYPES);

	PHYSFS_write( fp, &N_robot_joints, sizeof(int), 1);
	range_for (auto &r, Robot_joints)
		jointpos_write(fp, r);

	PHYSFS_write( fp, &N_weapon_types, sizeof(int), 1);
	range_for (const auto &w, Weapon_info)
		weapon_info_write(fp, w);

	PHYSFS_write( fp, &N_powerup_types, sizeof(int), 1);
	range_for (const auto &p, Powerup_info)
		powerup_type_info_write(fp, p);

	PHYSFS_write( fp, &N_polygon_models, sizeof(int), 1);
	range_for (const auto &p, partial_const_range(Polygon_models, N_polygon_models))
		polymodel_write(fp, p);

	range_for (const auto &p, partial_const_range(Polygon_models, N_polygon_models))
		PHYSFS_write( fp, p.model_data, sizeof(ubyte), p.model_data_size);

	PHYSFS_write( fp, Gauges, sizeof(bitmap_index), MAX_GAUGE_BMS);

	PHYSFS_write( fp, Dying_modelnums, sizeof(int), MAX_POLYGON_MODELS);
	PHYSFS_write( fp, Dead_modelnums, sizeof(int), MAX_POLYGON_MODELS);

	PHYSFS_write( fp, ObjBitmaps, sizeof(bitmap_index), MAX_OBJ_BITMAPS);
	PHYSFS_write( fp, ObjBitmapPtrs, sizeof(ushort), MAX_OBJ_BITMAPS);

	PHYSFS_write( fp, &only_player_ship, sizeof(player_ship), 1);

	PHYSFS_write( fp, &Num_cockpits, sizeof(int), 1);
	PHYSFS_write( fp, cockpit_bitmap, sizeof(bitmap_index), N_COCKPIT_BITMAPS);

	PHYSFS_write( fp, Sounds, sizeof(ubyte), MAX_SOUNDS);
	PHYSFS_write( fp, AltSounds, sizeof(ubyte), MAX_SOUNDS);

	PHYSFS_write( fp, &Num_total_object_types, sizeof(int), 1);
	PHYSFS_write( fp, ObjType, sizeof(sbyte), MAX_OBJTYPE);
	PHYSFS_write( fp, ObjId, sizeof(sbyte), MAX_OBJTYPE);
	PHYSFS_write( fp, ObjStrength, sizeof(fix), MAX_OBJTYPE);

	PHYSFS_write( fp, &First_multi_bitmap_num, sizeof(int), 1);

	PHYSFS_write( fp, &Reactors[0].n_guns, sizeof(int), 1);
	PHYSFS_write( fp, Reactors[0].gun_points, sizeof(vms_vector), MAX_CONTROLCEN_GUNS);
	PHYSFS_write( fp, Reactors[0].gun_dirs, sizeof(vms_vector), MAX_CONTROLCEN_GUNS);
	PHYSFS_write( fp, &exit_modelnum, sizeof(int), 1);
	PHYSFS_write( fp, &destroyed_exit_modelnum, sizeof(int), 1);
}
#endif

