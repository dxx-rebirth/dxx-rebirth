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
COPYRIGHT 1993-1999 PARALLAX SOFTWARE CORPORATION.  ALL RIGHTS RESERVED.
*/

/*
 *
 * Routines to parse bitmaps.tbl
 * Only used for editor, since the registered version of descent 1.
 *
 */

#include <algorithm>
#include <string>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <ctype.h>

#include "pstypes.h"
#include "gr.h"
#include "bm.h"
#include "u_mem.h"
#include "dxxerror.h"
#include "object.h"
#include "physfsx.h"
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
#include "endlevel.h"
#include "cntrlcen.h"
#include "physfs-serial.h"
#include "args.h"
#include "text.h"
#if defined(DXX_BUILD_DESCENT_I)
#include "fuelcen.h"
#elif defined(DXX_BUILD_DESCENT_II)
#include "gamepal.h"
#include "interp.h"
#endif
#include "strutil.h"
#if DXX_USE_EDITOR
#include "editor/texpage.h"
#endif

#include "compiler-range_for.h"
#include "partial_range.h"
#include "d_enumerate.h"

using std::min;

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

#if defined(DXX_BUILD_DESCENT_I)
static short		N_ObjBitmaps=0;
#elif defined(DXX_BUILD_DESCENT_II)
#define BM_GAUGES_HIRES	21
#endif

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
#if defined(DXX_BUILD_DESCENT_I)
static unsigned		clip_count;
static unsigned		clip_num;
static uint16_t		frames;
#elif defined(DXX_BUILD_DESCENT_II)
static char 		*arg;
static short 		clip_count = 0;
static short 		clip_num;
static short 		frames;
static char 		*dest_bm;		//clip number to play when destroyed
#endif
static short 		sound_num;
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

static int linenum;		//line int table currently being parsed

//------------------- Useful macros and variables ---------------

#define IFTOK(str) if (!strcmp(arg, str))

//	For the sake of LINT, defining prototypes to module's functions
#if defined(DXX_BUILD_DESCENT_I)
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
#elif defined(DXX_BUILD_DESCENT_II)
static void bm_read_alias(void);
static void bm_read_marker(void);
static void bm_read_robot_ai(int skip);
static void bm_read_powerup(int unused_flag);
static void bm_read_hostage(void);
static void bm_read_robot(int skip);
static void bm_read_weapon(int skip, int unused_flag);
static void bm_read_reactor(void);
static void bm_read_exitmodel(void);
static void bm_read_player_ship(void);
static void bm_read_some_file(int skip);
static void bm_read_sound(int skip);
static void clear_to_end_of_line(void);
static void verify_textures(void);
#endif

//---------------------------------------------------------------
int compute_average_pixel(grs_bitmap *n)
{
	int	total_red, total_green, total_blue;

#if defined(DXX_BUILD_DESCENT_II)
	auto pptr = n->bm_data;
#endif
	const unsigned bm_h = n->bm_h, bm_w = n->bm_w;

	total_red = 0;
	total_green = 0;
	total_blue = 0;

	const auto product = (bm_h * bm_w);
#if defined(DXX_BUILD_DESCENT_I)
	for (unsigned row = 0; row < bm_h; row++)
		for (unsigned column = 0; column < bm_w; column++)
#elif defined(DXX_BUILD_DESCENT_II)
	for (auto counter = product; counter--;)
#endif
	{
#if defined(DXX_BUILD_DESCENT_I)
		const auto color = gr_gpixel (*n, column, row);
		const auto &p = gr_palette[color];
#elif defined(DXX_BUILD_DESCENT_II)
		const auto &p = gr_palette[*pptr++];
#endif
		total_red += p.r;
		total_green += p.g;
		total_blue += p.b;
		}

	total_red /= product;
	total_green /= product;
	total_blue /= product;

	return BM_XRGB(total_red/2, total_green/2, total_blue/2);
}

//---------------------------------------------------------------
// Loads a bitmap from either the piggy file, a r64 file, or a
// whatever extension is passed.

static bitmap_index bm_load_sub(const int skip, const char *const filename)
{
	bitmap_index bitmap_num;
	palette_array_t newpal;
	int iff_error;		//reference parm to avoid warning message
	char fname[20];

	bitmap_num.index = 0;

	if (skip) {
		return bitmap_num;
	}

#if defined(DXX_BUILD_DESCENT_I)
	removeext(filename, fname);
#elif defined(DXX_BUILD_DESCENT_II)
	struct splitpath_t path;
	d_splitpath(  filename, &path);
	if (path.base_end - path.base_start >= sizeof(fname))
		Error("File <%s> - bitmap error, filename too long", filename);
	memcpy(fname,path.base_start,path.base_end - path.base_start);
#endif

	bitmap_num=piggy_find_bitmap( fname );
	if (bitmap_num.index)	{
		return bitmap_num;
	}

	grs_bitmap n;
	iff_error = iff_read_bitmap(filename, n, &newpal);
	if (iff_error != IFF_NO_ERROR)		{
		Error("File <%s> - IFF error: %s, line %d",filename,iff_errormsg(iff_error),linenum);
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
	char tempname[20];

	if (skip) {
		Assert( bogus_bitmap_initialized != 0 );
#if defined(DXX_BUILD_DESCENT_I)
		bmp[0] = piggy_register_bitmap(bogus_bitmap, "bogus", 0);
#elif defined(DXX_BUILD_DESCENT_II)
		bmp[0].index = 0;		//index of bogus bitmap==0 (I think)		//&bogus_bitmap;
#endif
		*nframes = 1;
		return;
	}


#if defined(DXX_BUILD_DESCENT_I)
	char fname[20];
	removeext(filename, fname);
#elif defined(DXX_BUILD_DESCENT_II)
	struct splitpath_t path;
	d_splitpath( filename, &path);
#endif

	{
	unsigned i;
	for (i=0; i<MAX_BITMAPS_PER_BRUSH; i++ )	{
#if defined(DXX_BUILD_DESCENT_I)
		snprintf(tempname, sizeof(tempname), "%s#%d", fname, i);
#elif defined(DXX_BUILD_DESCENT_II)
		snprintf( tempname, sizeof(tempname), "%.*s#%d", DXX_ptrdiff_cast_int(path.base_end - path.base_start), path.base_start, i );
#endif
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

//	Note that last argument passes an address to the array newpal (which is a pointer).
//	type mismatch found using lint, will substitute this line with an adjusted
//	one.  If fatal error, then it can be easily changed.
	array<std::unique_ptr<grs_main_bitmap>, MAX_BITMAPS_PER_BRUSH> bm;
	iff_error = iff_read_animbrush(filename,bm,nframes,newpal);
	if (iff_error != IFF_NO_ERROR)	{
		Error("File <%s> - IFF error: %s, line %d",filename,iff_errormsg(iff_error),linenum);
	}

	const auto nf = *nframes;
#if defined(DXX_BUILD_DESCENT_I)
	if (nf >= bm.size())
		return;
#endif
	for (uint_fast32_t i = 0; i != nf; ++i)
	{
#if defined(DXX_BUILD_DESCENT_I)
		snprintf(tempname, sizeof(tempname), "%s#%" PRIuFAST32, fname, i);
#elif defined(DXX_BUILD_DESCENT_II)
		snprintf( tempname, sizeof(tempname), "%.*s#%" PRIuFAST32, DXX_ptrdiff_cast_int(path.base_end - path.base_start), path.base_start, i );
#endif
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
#if defined(DXX_BUILD_DESCENT_I)
	snprintf(rawname, sizeof(rawname), "Sounds/%s.raw", fname);
#elif defined(DXX_BUILD_DESCENT_II)
	snprintf(rawname, sizeof(rawname), "Sounds/%s.%s", fname, (GameArg.SndDigiSampleRate==SAMPLE_RATE_22K) ? "r22" : "raw");
#endif

	i=piggy_find_sound( fname );
	if (i!=255)	{
		return i;
	}
	if (auto cfp = PHYSFSX_openReadBuffered(rawname))
	{
		n.length	= PHYSFS_fileLength( cfp );
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

#if defined(DXX_BUILD_DESCENT_II)
//loads a texture and returns the texture num
static int get_texture(char *name)
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
		TmapInfo[texture_count].filename.copy_if(short_name);
		texture_count++;
		Assert(texture_count < MAX_TEXTURES);
		NumTextures = texture_count;
	}

	return i;
}

#define DEFAULT_PIG_PALETTE	"groupa.256"
#endif

#define LINEBUF_SIZE 600

namespace dsx {

//-----------------------------------------------------------------
// Initializes all properties and bitmaps from BITMAPS.TBL file.
// This is called when the editor is IN.
// If no editor, properties_read_cmp() is called.
int gamedata_read_tbl(int pc_shareware)
{
	int	have_bin_tbl;

#if defined(DXX_BUILD_DESCENT_I)
	std::string dest_bm;
	ObjType[0] = OL_PLAYER;
	ObjId[0] = 0;
	Num_total_object_types = 1;
#elif defined(DXX_BUILD_DESCENT_II)
	// Open BITMAPS.TBL for reading.
	have_bin_tbl = 0;
	auto InfoFile = PHYSFSX_openReadBuffered("BITMAPS.TBL");
	if (!InfoFile)
	{
		InfoFile = PHYSFSX_openReadBuffered("BITMAPS.BIN");
		if (!InfoFile)
			return 0;	//missing BITMAPS.TBL and BITMAPS.BIN file
		have_bin_tbl = 1;
	}

	gr_use_palette_table(DEFAULT_PIG_PALETTE);

	load_palette(DEFAULT_PIG_PALETTE,-2,0);		//special: tell palette code which pig is loaded
#endif

	for (unsigned i = 0; i < MAX_SOUNDS; ++i)
	{
		Sounds[i] = 255;
		AltSounds[i] = 255;
	}

	range_for (auto &ti, TmapInfo)
	{
		ti.eclip_num = eclip_none;
		ti.flags = 0;
#if defined(DXX_BUILD_DESCENT_II)
		ti.slide_u = ti.slide_v = 0;
		ti.destroyed = -1;
#endif
	}

#if defined(DXX_BUILD_DESCENT_II)
	range_for (auto &i, Reactors)
		i.model_num = -1;
#endif

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
	range_for (auto &vc, Vclip)
	{
		vc.num_frames = -1;
		vc.flags = 0;
	}

	range_for (auto &wa, WallAnims)
		wa.num_frames = wclip_frames_none;
	Num_wall_anims = 0;

	if (Installed)
		return 1;

	Installed = 1;

#if defined(DXX_BUILD_DESCENT_I)
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
#endif
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
			const auto s = strtol(&temp_ptr[7], &p, 10);
			if (!*p)
				SuperX = s;
#if defined(DXX_BUILD_DESCENT_II)
			Assert(SuperX == 254);
				//the superx color isn't kept around, so the new piggy regeneration
				//code doesn't know what it is, so it assumes that it's 254, so
				//this code requires that it be 254
#endif
		}

#if defined(DXX_BUILD_DESCENT_I)
		char *arg = strtok( inputline, space_tab );
		if (arg && arg[0] == '@')
#elif defined(DXX_BUILD_DESCENT_II)
		arg = strtok( inputline, space_tab );
		if (arg[0] == '@')
#endif
		{
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
#if defined(DXX_BUILD_DESCENT_I)
			else IFTOK("$SOUND") 		bm_read_sound(arg, skip, pc_shareware);
#elif defined(DXX_BUILD_DESCENT_II)
			else IFTOK("$GAUGES_HIRES"){bm_flag = BM_GAUGES_HIRES; clip_count = 0;}
			else IFTOK("$ALIAS")			bm_read_alias();
			else IFTOK("$SOUND") 		bm_read_sound(skip);
#endif
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
#if defined(DXX_BUILD_DESCENT_I)
				dest_bm.clear();
#elif defined(DXX_BUILD_DESCENT_II)
				dest_bm=NULL;
#endif
				dest_vclip=vclip_none;
				dest_eclip=eclip_none;
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
#if defined(DXX_BUILD_DESCENT_I)
			else IFTOK("!METALS_FLAG") ;
			else IFTOK("!LIGHTS_FLAG") ;
			else IFTOK("!EFFECTS_FLAG") ;
#endif
			#endif

			else IFTOK("lighting") 			TmapInfo[texture_count-1].lighting = fl2f(get_float());
			else IFTOK("damage") 			TmapInfo[texture_count-1].damage = fl2f(get_float());
			else IFTOK("volatile") 			TmapInfo[texture_count-1].flags |= TMI_VOLATILE;
#if defined(DXX_BUILD_DESCENT_II)
			else IFTOK("goal_blue")			TmapInfo[texture_count-1].flags |= TMI_GOAL_BLUE;
			else IFTOK("goal_red")			TmapInfo[texture_count-1].flags |= TMI_GOAL_RED;
			else IFTOK("water")	 			TmapInfo[texture_count-1].flags |= TMI_WATER;
			else IFTOK("force_field")		TmapInfo[texture_count-1].flags |= TMI_FORCE_FIELD;
			else IFTOK("slide")	 			{TmapInfo[texture_count-1].slide_u = fl2f(get_float())>>8; TmapInfo[texture_count-1].slide_v = fl2f(get_float())>>8;}
			else IFTOK("destroyed")	 		{int t=texture_count-1; TmapInfo[t].destroyed = get_texture(strtok( NULL, space_tab ));}
#endif
			//else IFTOK("Num_effects")		Num_effects = get_int();
			else IFTOK("Num_wall_anims")	Num_wall_anims = get_int();
			else IFTOK("clip_num")			clip_num = get_int();
#if defined(DXX_BUILD_DESCENT_I)
			else IFTOK("dest_bm")
			{
				char *p = strtok( NULL, space_tab );
				if (p)
					dest_bm = p;
				else
					dest_bm.clear();
			}
#elif defined(DXX_BUILD_DESCENT_II)
			else IFTOK("dest_bm")			dest_bm = strtok( NULL, space_tab );
#endif
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
#if defined(DXX_BUILD_DESCENT_I)
			else IFTOK("$ROBOT_AI") 		bm_read_robot_ai(arg, skip);

			else IFTOK("$POWERUP")			{bm_read_powerup(arg, 0);		continue;}
			else IFTOK("$POWERUP_UNUSED")	{bm_read_powerup(arg, 1);		continue;}
			else IFTOK("$HOSTAGE")			{bm_read_hostage(arg);		continue;}
			else IFTOK("$ROBOT")				{bm_read_robot(arg, skip);			continue;}
			else IFTOK("$WEAPON")			{bm_read_weapon(arg, skip, 0);		continue;}
			else IFTOK("$WEAPON_UNUSED")	{bm_read_weapon(arg, skip, 1);		continue;}
			else IFTOK("$OBJECT")			{bm_read_object(arg, skip);		continue;}
			else IFTOK("$PLAYER_SHIP")		{bm_read_player_ship(arg, skip);	continue;}
#elif defined(DXX_BUILD_DESCENT_II)
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
#endif
			else	{		//not a special token, must be a bitmap!

				// Remove any illegal/unwanted spaces and tabs at this point.
				while ((*arg=='\t') || (*arg==' ')) arg++;
				if (*arg == '\0') { break; }

#if defined(DXX_BUILD_DESCENT_II)
				//check for '=' in token, indicating error
				if (strchr(arg,'='))
					Error("Unknown token <'%s'> on line %d of BITMAPS.TBL",arg,linenum);
#endif

				// Otherwise, 'arg' is apparently a bitmap filename.
				// Load bitmap and process it below:
#if defined(DXX_BUILD_DESCENT_I)
				bm_read_some_file(dest_bm, arg, skip);
#elif defined(DXX_BUILD_DESCENT_II)
				bm_read_some_file(skip);
#endif

			}

			arg = strtok( NULL, equal_space );
			continue;
      }
	}

	NumTextures = texture_count;
	Num_tmaps = tmap_count;

#if defined(DXX_BUILD_DESCENT_II)
	Textures[NumTextures++].index = 0;		//entry for bogus tmap
	InfoFile.reset();
#endif
	Assert(N_robot_types == Num_robot_ais);		//should be one ai info per robot

	verify_textures();

	//check for refereced but unused clip count
	range_for (auto &&en, enumerate(Effects))
	{
		auto &e = en.value;
		if ((e.changing_wall_texture != -1 || e.changing_object_texture != -1) && e.vc.num_frames == ~0u)
			Error("EClip %" PRIuFAST32 " referenced (by polygon object?), but not defined", en.idx);
	}

#if defined(DXX_BUILD_DESCENT_II)
	#ifndef NDEBUG
	{
		//make sure all alt sounds refer to valid main sounds
		for (unsigned i = 0; i < num_sounds; ++i)
		{
			int alt = AltSounds[i];
			Assert(alt==0 || alt==-1 || Sounds[alt]!=255);
		}
	}
	#endif

	gr_use_palette_table(D2_DEFAULT_PALETTE);
#endif

	return 0;
}

}

void verify_textures()
{
	grs_bitmap * bmp;
	int j;
	j=0;
	for (uint_fast32_t i = 0; i < Num_tmaps; ++i)
	{
		bmp = &GameBitmaps[Textures[i].index];
		if ( (bmp->bm_w!=64)||(bmp->bm_h!=64)||(bmp->bm_rowsize!=64) )	{
			j++;
		}
	}
	if (j)
		Error("%d textures were not 64x64.",j);

#if defined(DXX_BUILD_DESCENT_II)
	for (uint_fast32_t i = 0; i < Num_effects; ++i)
		if (Effects[i].changing_object_texture != -1)
			if (GameBitmaps[ObjBitmaps[Effects[i].changing_object_texture].index].bm_w!=64 || GameBitmaps[ObjBitmaps[Effects[i].changing_object_texture].index].bm_h!=64)
				Error("Effect %" PRIuFAST32 " is used on object, but is not 64x64",i);
#endif
}

#if defined(DXX_BUILD_DESCENT_II)
void bm_read_alias()
{
	char *t;

	Assert(Num_aliases < MAX_ALIASES);

	t = strtok( NULL, space_tab );  strncpy(alias_list[Num_aliases].alias_name,t,sizeof(alias_list[Num_aliases].alias_name));
	t = strtok( NULL, space_tab );  strncpy(alias_list[Num_aliases].file_name,t,sizeof(alias_list[Num_aliases].file_name));

	Num_aliases++;
}
#endif

static void set_lighting_flag(grs_bitmap &bmp)
{
	bmp.set_flag_mask(vlighting < 0, BM_FLAG_NO_LIGHTING);
}

static void set_texture_name(const char *name)
{
	TmapInfo[texture_count].filename.copy_if(name, FILENAME_LEN);
	REMOVE_DOTS(&TmapInfo[texture_count].filename[0u]);
}

#if defined(DXX_BUILD_DESCENT_I)
static void bm_read_eclip(const std::string &dest_bm, const char *const arg, int skip)
#elif defined(DXX_BUILD_DESCENT_II)
static void bm_read_eclip(int skip)
#endif
{
	bitmap_index bitmap;

	assert(clip_num < Effects.size());

	if (clip_num+1 > Num_effects)
		Num_effects = clip_num+1;

	Effects[clip_num].flags = 0;

#if defined(DXX_BUILD_DESCENT_II)
	unsigned dest_bm_num = 0;
	//load the dest bitmap first, so that after this routine, the last-loaded
	//texture will be the monitor, so that lighting parameter will be applied
	//to the correct texture
	if (dest_bm) {			//deal with bitmap for blown up clip
		d_fname short_name;
		int i;
		short_name.copy_if(dest_bm, FILENAME_LEN);
		REMOVE_DOTS(&short_name[0u]);
		for (i=0;i<texture_count;i++)
			if (!d_stricmp(TmapInfo[i].filename,short_name))
				break;
		if (i==texture_count) {
			Textures[texture_count] = bm_load_sub(skip, dest_bm);
			TmapInfo[texture_count].filename = short_name;
			texture_count++;
			Assert(texture_count < MAX_TEXTURES);
			NumTextures = texture_count;
		}
		else if (Textures[i].index == 0)		//was found, but registered out
			Textures[i] = bm_load_sub(skip, dest_bm);
		dest_bm_num = i;
	}
#endif

	if (!abm_flag)
	{
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
		array<bitmap_index, MAX_BITMAPS_PER_BRUSH> bm;
		abm_flag = 0;

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

#if defined(DXX_BUILD_DESCENT_I)
	if (!dest_bm.empty())
#elif defined(DXX_BUILD_DESCENT_II)
	if (dest_bm)
#endif
	{			//deal with bitmap for blown up clip
#if defined(DXX_BUILD_DESCENT_I)
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
#elif defined(DXX_BUILD_DESCENT_II)
		Effects[clip_num].dest_bm_num = dest_bm_num;
#endif

		if (dest_vclip==vclip_none)
			Error("Desctuction vclip missing on line %d",linenum);
		if (dest_size==-1)
			Error("Desctuction vclip missing on line %d",linenum);

		Effects[clip_num].dest_vclip = dest_vclip;
		Effects[clip_num].dest_size = dest_size;

		Effects[clip_num].dest_eclip = dest_eclip;
	}
	else {
		Effects[clip_num].dest_bm_num = ~0u;
		Effects[clip_num].dest_eclip = eclip_none;
	}

	if (crit_flag)
		Effects[clip_num].flags |= EF_CRITICAL;
}

#if defined(DXX_BUILD_DESCENT_I)
static void bm_read_gauges(const char *const arg, int skip)
#elif defined(DXX_BUILD_DESCENT_II)
static void bm_read_gauges(int skip)
#endif
{
	bitmap_index bitmap;
	unsigned i, num_abm_frames;

	if (!abm_flag)	{
		bitmap = bm_load_sub(skip, arg);
		Assert(clip_count < MAX_GAUGE_BMS);
		Gauges[clip_count] = bitmap;
		clip_count++;
	} else {
		array<bitmap_index, MAX_BITMAPS_PER_BRUSH> bm;
		abm_flag = 0;
		ab_load(skip, arg, bm, &num_abm_frames );
		for (i=clip_count; i<clip_count+num_abm_frames; i++) {
			Assert(i < MAX_GAUGE_BMS);
			Gauges[i] = bm[i-clip_count];
		}
		clip_count += num_abm_frames;
	}
}

#if defined(DXX_BUILD_DESCENT_I)
static void bm_read_wclip(char *const arg, int skip)
#elif defined(DXX_BUILD_DESCENT_II)
static void bm_read_gauges_hires()
{
	bitmap_index bitmap;
	unsigned i, num_abm_frames;

	if (!abm_flag)	{
		bitmap = bm_load_sub(0, arg);
		Assert(clip_count < MAX_GAUGE_BMS);
		Gauges_hires[clip_count] = bitmap;
		clip_count++;
	} else {
		array<bitmap_index, MAX_BITMAPS_PER_BRUSH> bm;
		abm_flag = 0;
		ab_load(0, arg, bm, &num_abm_frames );
		for (i=clip_count; i<clip_count+num_abm_frames; i++) {
			Assert(i < MAX_GAUGE_BMS);
			Gauges_hires[i] = bm[i-clip_count];
		}
		clip_count += num_abm_frames;
	}
}

static void bm_read_wclip(int skip)
#endif
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
		if (WallAnims[clip_num].num_frames != wclip_frames_none && clip_count == 0)
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
		array<bitmap_index, MAX_BITMAPS_PER_BRUSH> bm;
		unsigned nframes;
		if (WallAnims[clip_num].num_frames != wclip_frames_none)
			Error( "AB_Wall clip %d is already used!", clip_num );
		abm_flag = 0;
#if defined(DXX_BUILD_DESCENT_I)
		ab_load(skip, arg, bm, &nframes );
#elif defined(DXX_BUILD_DESCENT_II)
		ab_load(0, arg, bm, &nframes );
#endif
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

#if defined(DXX_BUILD_DESCENT_I)
static void bm_read_vclip(const char *const arg, int skip)
#elif defined(DXX_BUILD_DESCENT_II)
static void bm_read_vclip(int skip)
#endif
{
	bitmap_index bi;
	Assert(clip_num < VCLIP_MAXNUM);

#if defined(DXX_BUILD_DESCENT_II)
	if (clip_num >= Num_vclips)
		Num_vclips = clip_num+1;
#endif

	if (!abm_flag)	{
		if (Vclip[clip_num].num_frames != ~0u && clip_count == 0)
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
		array<bitmap_index, MAX_BITMAPS_PER_BRUSH> bm;
		abm_flag = 0;
		if (Vclip[clip_num].num_frames != ~0u)
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

#if defined(DXX_BUILD_DESCENT_I)
static void clear_to_end_of_line(char *&arg)
{
	arg = NULL;
}
#elif defined(DXX_BUILD_DESCENT_II)
static void clear_to_end_of_line()
{
	arg = strtok( NULL, space_tab );
	while (arg != NULL)
		arg = strtok( NULL, space_tab );
}
#endif

#if defined(DXX_BUILD_DESCENT_I)
static void bm_read_sound(char *&arg, int skip, int pc_shareware)
#elif defined(DXX_BUILD_DESCENT_II)
void bm_read_sound(int skip)
#endif
{
	int alt_sound_num;

	const int read_sound_num = get_int();
#if defined(DXX_BUILD_DESCENT_I)
	alt_sound_num = pc_shareware ? read_sound_num : get_int();
#elif defined(DXX_BUILD_DESCENT_II)
	alt_sound_num = get_int();
#endif

	if ( read_sound_num>=MAX_SOUNDS )
		Error( "Too many sound files.\n" );

	if (read_sound_num >= num_sounds)
		num_sounds = read_sound_num+1;

#if defined(DXX_BUILD_DESCENT_II)
	if (Sounds[read_sound_num] != 255)
		Error("Sound num %d already used, bitmaps.tbl, line %d\n",read_sound_num,linenum);
#endif

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
#if defined(DXX_BUILD_DESCENT_I)
static void bm_read_robot_ai(char *&arg, const int skip)
#elif defined(DXX_BUILD_DESCENT_II)
void bm_read_robot_ai(const int skip)
#endif
{
	char			*robotnum_text;
	int			robotnum;

	robotnum_text = strtok(NULL, space_tab);
	robotnum = atoi(robotnum_text);
	Assert(robotnum < MAX_ROBOT_TYPES);
	auto &robptr = Robot_info[robotnum];

	Assert(robotnum == Num_robot_ais);		//make sure valid number

	if (skip) {
		Num_robot_ais++;
#if defined(DXX_BUILD_DESCENT_I)
		clear_to_end_of_line(arg);
#elif defined(DXX_BUILD_DESCENT_II)
		clear_to_end_of_line();
#endif
		return;
	}

	Num_robot_ais++;

	get4fix(robptr.field_of_view);
	get4fix(robptr.firing_wait);
#if defined(DXX_BUILD_DESCENT_II)
	get4fix(robptr.firing_wait2);
#endif
	get4byte(robptr.rapidfire_count);
	get4fix(robptr.turn_time);
#if defined(DXX_BUILD_DESCENT_I)
	array<fix, NDL>		fire_power,						//	damage done by a hit from this robot
		shield;							//	shield strength of this robot
	get4fix(fire_power);
	get4fix(shield);
#elif defined(DXX_BUILD_DESCENT_II)
//	get4fix(robptr->fire_power);
//	get4fix(robptr->shield);
#endif
	get4fix(robptr.max_speed);
	get4fix(robptr.circle_distance);
	get4byte(robptr.evade_speed);

	robptr.always_0xabcd	= 0xabcd;
	adjust_field_of_view(robptr.field_of_view);
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
		const unsigned eclip_num = atoi(name+1);

		if (Effects[eclip_num].changing_object_texture == -1) {		//first time referenced
			Effects[eclip_num].changing_object_texture = N_ObjBitmaps;
			ObjBitmapPtrs[N_ObjBitmapPtrs++] = N_ObjBitmaps;
			N_ObjBitmaps++;
		} else {
			ObjBitmapPtrs[N_ObjBitmapPtrs++] = Effects[eclip_num].changing_object_texture;
		}
#if defined(DXX_BUILD_DESCENT_II)
		assert(N_ObjBitmaps < ObjBitmaps.size());
		assert(N_ObjBitmapPtrs < ObjBitmapPtrs.size());
#endif
		return NULL;
	}
	else 	{
		ObjBitmaps[N_ObjBitmaps] = bm_load_sub(skip, name);
#if defined(DXX_BUILD_DESCENT_II)
		if (GameBitmaps[ObjBitmaps[N_ObjBitmaps].index].bm_w!=64 || GameBitmaps[ObjBitmaps[N_ObjBitmaps].index].bm_h!=64)
			Error("Bitmap <%s> is not 64x64",name);
#endif
		ObjBitmapPtrs[N_ObjBitmapPtrs++] = N_ObjBitmaps;
		N_ObjBitmaps++;
#if defined(DXX_BUILD_DESCENT_II)
		assert(N_ObjBitmaps < ObjBitmaps.size());
		assert(N_ObjBitmapPtrs < ObjBitmapPtrs.size());
#endif
		return &GameBitmaps[ObjBitmaps[N_ObjBitmaps-1].index];
	}
}

#define MAX_MODEL_VARIANTS	4

// ------------------------------------------------------------------------------
#if defined(DXX_BUILD_DESCENT_I)
static void bm_read_robot(char *&arg, int skip)
#elif defined(DXX_BUILD_DESCENT_II)
void bm_read_robot(int skip)
#endif
{
	char			*model_name[MAX_MODEL_VARIANTS];
	int			n_models,i;
	int			first_bitmap_num[MAX_MODEL_VARIANTS];
	char			*equal_ptr;
	int exp1_vclip_num = vclip_none;
	int exp1_sound_num = sound_none;
	int exp2_vclip_num = vclip_none;
	int exp2_sound_num = sound_none;
	fix			lighting = F1_0/2;		// Default
	fix			strength = F1_0*10;		// Default strength
	fix			mass = f1_0*4;
	fix			drag = f1_0/2;
	weapon_id_type weapon_type = weapon_id_type::LASER_ID_L1;
	int			contains_count=0, contains_id=0, contains_prob=0, contains_type=0;
#if defined(DXX_BUILD_DESCENT_II)
	weapon_id_type weapon_type2 = weapon_id_type::unspecified;
	auto behavior = ai_behavior::AIB_NORMAL;
	int			companion = 0, smart_blobs=0, energy_blobs=0, badass=0, energy_drain=0, kamikaze=0, thief=0, pursuit=0, lightcast=0, death_roll=0;
	fix			glow=0, aim=F1_0;
	int			deathroll_sound = SOUND_BOSS_SHARE_DIE;	//default
	int			taunt_sound = ROBOT_SEE_SOUND_DEFAULT;
	ubyte flags=0;
#endif
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
#if defined(DXX_BUILD_DESCENT_I)
		Num_total_object_types++;
		clear_to_end_of_line(arg);
#elif defined(DXX_BUILD_DESCENT_II)
		Assert(N_robot_types < MAX_ROBOT_TYPES);
		clear_to_end_of_line();
#endif
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
			}
#if defined(DXX_BUILD_DESCENT_II)
			else if (!d_stricmp( arg, "weapon_type2"))
			{
				weapon_type2 = static_cast<weapon_id_type>(atoi(equal_ptr));
			}
#endif
			else if (!d_stricmp( arg, "strength" )) {
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
			}
#if defined(DXX_BUILD_DESCENT_II)
			else if (!d_stricmp( arg, "companion" )) {
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
			}
#endif
			else if (!d_stricmp( arg, "contains_prob" )) {
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
			}
#if defined(DXX_BUILD_DESCENT_II)
			else if (!d_stricmp( arg, "taunt_sound" )) {
				taunt_sound = atoi(equal_ptr);
			} else if (!d_stricmp( arg, "aim" )) {
				aim = fl2f(atof(equal_ptr));
			} else if (!d_stricmp( arg, "big_radius" )) {
				if (atoi(equal_ptr))
					flags |= RIF_BIG_RADIUS;
			} else if (!d_stricmp( arg, "behavior" )) {
				if (!d_stricmp(equal_ptr, "STILL"))
					behavior = ai_behavior::AIB_STILL;
				else if (!d_stricmp(equal_ptr, "NORMAL"))
					behavior = ai_behavior::AIB_NORMAL;
				else if (!d_stricmp(equal_ptr, "BEHIND"))
					behavior = ai_behavior::AIB_BEHIND;
				else if (!d_stricmp(equal_ptr, "RUN_FROM"))
					behavior = ai_behavior::AIB_RUN_FROM;
				else if (!d_stricmp(equal_ptr, "SNIPE"))
					behavior = ai_behavior::AIB_SNIPE;
				else if (!d_stricmp(equal_ptr, "STATION"))
					behavior = ai_behavior::AIB_STATION;
				else if (!d_stricmp(equal_ptr, "FOLLOW"))
					behavior = ai_behavior::AIB_FOLLOW;
				else
					Int3();	//	Error.  Illegal behavior type for current robot.
			}
#endif
			else if (!d_stricmp( arg, "name" )) {
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
#if defined(DXX_BUILD_DESCENT_II)
				Assert(n_models < MAX_MODEL_VARIANTS);
#endif
			}
#if defined(DXX_BUILD_DESCENT_II)
			else
			{
				Int3();
			}
#endif
		} else {			// Must be a texture specification...
			load_polymodel_bitmap(skip, arg);
		}
		arg = strtok( NULL, space_tab );
	}

	//clear out anim info
	range_for (auto &g, Robot_info[N_robot_types].anim_states)
		range_for (auto &s, g)
			s.n_joints = 0;

	first_bitmap_num[n_models] = N_ObjBitmapPtrs;

	for (i=0;i<n_models;i++) {
		int n_textures;
		int model_num,last_model_num=0;

		n_textures = first_bitmap_num[i+1] - first_bitmap_num[i];

		model_num = load_polygon_model(model_name[i],n_textures,first_bitmap_num[i], (i == 0) ? &Robot_info[N_robot_types] : nullptr);

		if (i==0)
			Robot_info[N_robot_types].model_num = model_num;
		else
			Polygon_models[last_model_num].simpler_model = model_num+1;

		last_model_num = model_num;
	}

#if defined(DXX_BUILD_DESCENT_I)
	ObjType[Num_total_object_types] = OL_ROBOT;
	ObjId[Num_total_object_types] = N_robot_types;
#elif defined(DXX_BUILD_DESCENT_II)
	if ((glow > i2f(15)) || (glow < 0) || (glow != 0 && glow < 0x1000)) {
		Int3();
	}
#endif

	Robot_info[N_robot_types].exp1_vclip_num = exp1_vclip_num;
	Robot_info[N_robot_types].exp2_vclip_num = exp2_vclip_num;
	Robot_info[N_robot_types].exp1_sound_num = exp1_sound_num;
	Robot_info[N_robot_types].exp2_sound_num = exp2_sound_num;
	Robot_info[N_robot_types].lighting = lighting;
	Robot_info[N_robot_types].weapon_type = weapon_type;
#if defined(DXX_BUILD_DESCENT_II)
	Robot_info[N_robot_types].weapon_type2 = weapon_type2;
#endif
	Robot_info[N_robot_types].strength = strength;
	Robot_info[N_robot_types].mass = mass;
	Robot_info[N_robot_types].drag = drag;
	Robot_info[N_robot_types].cloak_type = cloak_type;
	Robot_info[N_robot_types].attack_type = attack_type;
	Robot_info[N_robot_types].boss_flag = boss_flag;

	Robot_info[N_robot_types].contains_id = contains_id;
	Robot_info[N_robot_types].contains_count = contains_count;
	Robot_info[N_robot_types].contains_prob = contains_prob;
#if defined(DXX_BUILD_DESCENT_II)
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
#endif
	Robot_info[N_robot_types].score_value = score_value;
	Robot_info[N_robot_types].see_sound = see_sound;
	Robot_info[N_robot_types].attack_sound = attack_sound;
	Robot_info[N_robot_types].claw_sound = claw_sound;
#if defined(DXX_BUILD_DESCENT_II)
	Robot_info[N_robot_types].taunt_sound = taunt_sound;
	Robot_info[N_robot_types].behavior = behavior;		//	Default behavior for this robot, if coming out of matcen.
	Robot_info[N_robot_types].aim = min(f2i(aim*255), 255);		//	how well this robot type can aim.  255=perfect
#endif

	if (contains_type)
		Robot_info[N_robot_types].contains_type = OBJ_ROBOT;
	else
		Robot_info[N_robot_types].contains_type = OBJ_POWERUP;

	N_robot_types++;
#if defined(DXX_BUILD_DESCENT_I)
	Num_total_object_types++;
#elif defined(DXX_BUILD_DESCENT_II)

	Assert(N_robot_types < MAX_ROBOT_TYPES);

	bm_flag = BM_NONE;
#endif
}

#if defined(DXX_BUILD_DESCENT_I)
//read a polygon object of some sort
void bm_read_object(char *&arg, int skip)
#elif defined(DXX_BUILD_DESCENT_II)
//read a reactor model
void bm_read_reactor(void)
#endif
{
	char *model_name, *model_name_dead=NULL;
	int first_bitmap_num, first_bitmap_num_dead=0, n_normal_bitmaps;
	char *equal_ptr;
	short model_num;
	fix	lighting = F1_0/2;		// Default
	int type=-1;
#if defined(DXX_BUILD_DESCENT_I)
	fix strength=0;
#elif defined(DXX_BUILD_DESCENT_II)
	assert(Num_reactors < Reactors.size());
#endif

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

#if defined(DXX_BUILD_DESCENT_I)
			if (!d_stricmp(arg,"type")) {
				if (!d_stricmp(equal_ptr,"controlcen"))
					type = OL_CONTROL_CENTER;
				else if (!d_stricmp(equal_ptr,"clutter"))
					type = OL_CLUTTER;
				else if (!d_stricmp(equal_ptr,"exit"))
					type = OL_EXIT;
			}
			else
#endif
			if (!d_stricmp( arg, "dead_pof" ))	{
				model_name_dead = equal_ptr;
				first_bitmap_num_dead=N_ObjBitmapPtrs;
			} else if (!d_stricmp( arg, "lighting" ))	{
				lighting = fl2f(atof(equal_ptr));
				if ( (lighting < 0) || (lighting > F1_0 )) {
					Error( "In bitmaps.tbl, lighting value of %.2f is out of range 0..1.\n", f2fl(lighting));
				}
			}
#if defined(DXX_BUILD_DESCENT_I)
			else if (!d_stricmp( arg, "strength" )) {
				strength = fl2f(atof(equal_ptr));
			}
#elif defined(DXX_BUILD_DESCENT_II)
			else {
				Int3();
			}
#endif
		} else {			// Must be a texture specification...
#if defined(DXX_BUILD_DESCENT_I)
			load_polymodel_bitmap(skip, arg);
#elif defined(DXX_BUILD_DESCENT_II)
			load_polymodel_bitmap(0, arg);
#endif
		}
		arg = strtok( NULL, space_tab );
	}

	if ( model_name_dead )
		n_normal_bitmaps = first_bitmap_num_dead-first_bitmap_num;
	else
		n_normal_bitmaps = N_ObjBitmapPtrs-first_bitmap_num;

	model_num = load_polygon_model(model_name,n_normal_bitmaps,first_bitmap_num,NULL);

#if defined(DXX_BUILD_DESCENT_I)
	if (type == OL_CONTROL_CENTER)
		read_model_guns(model_name, Reactors[0]);
#endif
	if ( model_name_dead )
		Dead_modelnums[model_num]  = load_polygon_model(model_name_dead,N_ObjBitmapPtrs-first_bitmap_num_dead,first_bitmap_num_dead,NULL);
	else
		Dead_modelnums[model_num] = -1;

	if (type == -1)
		Error("No object type specfied for object in BITMAPS.TBL on line %d\n",linenum);

#if defined(DXX_BUILD_DESCENT_I)
	ObjType[Num_total_object_types] = type;
	ObjId[Num_total_object_types] = model_num;
	ObjStrength[Num_total_object_types] = strength;

	Num_total_object_types++;

	if (type == OL_EXIT) {
		exit_modelnum = model_num;
		destroyed_exit_modelnum = Dead_modelnums[model_num];
	}
#elif defined(DXX_BUILD_DESCENT_II)
	Reactors[Num_reactors].model_num = model_num;
	read_model_guns(model_name, Reactors[Num_reactors]);

	Num_reactors++;
#endif
}

#if defined(DXX_BUILD_DESCENT_II)
//read the marker object
void bm_read_marker()
{
	char *model_name;
	int first_bitmap_num, n_normal_bitmaps;
	char *equal_ptr;

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
			Int3();

		} else {			// Must be a texture specification...
			load_polymodel_bitmap(0, arg);
		}
		arg = strtok( NULL, space_tab );
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

			if (!d_stricmp( arg, "dead_pof" ))	{
				model_name_dead = equal_ptr;
				first_bitmap_num_dead=N_ObjBitmapPtrs;
			} else {
				Int3();
			}
		} else {			// Must be a texture specification...
			load_polymodel_bitmap(0, arg);
		}
		arg = strtok( NULL, space_tab );
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
#endif

#if defined(DXX_BUILD_DESCENT_I)
void bm_read_player_ship(char *&arg, int skip)
#elif defined(DXX_BUILD_DESCENT_II)
void bm_read_player_ship(void)
#endif
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
	Player_ship->expl_vclip_num = vclip_none;

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
#if defined(DXX_BUILD_DESCENT_II)
				Assert(n_models < MAX_MODEL_VARIANTS);
#endif

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
#if defined(DXX_BUILD_DESCENT_II)
			else {
				Int3();
			}
#endif
		}
		else if (!d_stricmp( arg, "multi_textures" )) {

			First_multi_bitmap_num = N_ObjBitmapPtrs;
			first_bitmap_num[n_models] = N_ObjBitmapPtrs;

		}
		else			// Must be a texture specification...
		{
#if defined(DXX_BUILD_DESCENT_I)
			load_polymodel_bitmap(skip, arg);
#elif defined(DXX_BUILD_DESCENT_II)
			load_polymodel_bitmap(0, arg);
#endif
		}

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

#if defined(DXX_BUILD_DESCENT_I)
		robot_info *pri = NULL;
		if (i == 0)
			pri = &ri;
		model_num = load_polygon_model(model_name[i],n_textures,first_bitmap_num[i],pri);
#elif defined(DXX_BUILD_DESCENT_II)
		model_num = load_polygon_model(model_name[i],n_textures,first_bitmap_num[i],(i==0) ? &ri : nullptr);
#endif

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

#if defined(DXX_BUILD_DESCENT_I)
void bm_read_some_file(const std::string &dest_bm, char *&arg, int skip)
#elif defined(DXX_BUILD_DESCENT_II)
void bm_read_some_file(int skip)
#endif
{

	switch (bm_flag) {
#if defined(DXX_BUILD_DESCENT_II)
	case BM_NONE:
		Error("Trying to read bitmap <%s> with bm_flag==BM_NONE on line %d of BITMAPS.TBL",arg,linenum);
		break;
#endif
	case BM_COCKPIT:	{
		bitmap_index bitmap;
		bitmap = bm_load_sub(skip, arg);
		Assert( Num_cockpits < N_COCKPIT_BITMAPS );
		cockpit_bitmap[Num_cockpits++] = bitmap;
		//bm_flag = BM_NONE;
#if defined(DXX_BUILD_DESCENT_II)
		return;
#endif
		}
		break;
	case BM_GAUGES:
#if defined(DXX_BUILD_DESCENT_I)
		bm_read_gauges(arg, skip);
#elif defined(DXX_BUILD_DESCENT_II)
		bm_read_gauges(skip);
		return;
		break;
	case BM_GAUGES_HIRES:
		bm_read_gauges_hires();
		return;
#endif
		break;
	case BM_WEAPON:
#if defined(DXX_BUILD_DESCENT_I)
		bm_read_weapon(arg, skip, 0);
#elif defined(DXX_BUILD_DESCENT_II)
		bm_read_weapon(skip, 0);
		return;
#endif
		break;
	case BM_VCLIP:
#if defined(DXX_BUILD_DESCENT_I)
		bm_read_vclip(arg, skip);
#elif defined(DXX_BUILD_DESCENT_II)
		bm_read_vclip(skip);
		return;
#endif
		break;
	case BM_ECLIP:
#if defined(DXX_BUILD_DESCENT_I)
		bm_read_eclip(dest_bm, arg, skip);
#elif defined(DXX_BUILD_DESCENT_II)
		bm_read_eclip(skip);
		return;
#endif
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
#if defined(DXX_BUILD_DESCENT_II)
		return;
#endif
		}
		break;
	case BM_WCLIP:
#if defined(DXX_BUILD_DESCENT_I)
		bm_read_wclip(arg, skip);
		break;
	default:
#elif defined(DXX_BUILD_DESCENT_II)
		bm_read_wclip(skip);
		return;
#endif
		break;
	}
#if defined(DXX_BUILD_DESCENT_II)
	Error("Trying to read bitmap <%s> with unknown bm_flag <%x> on line %d of BITMAPS.TBL",arg,bm_flag,linenum);
#endif
}

// ------------------------------------------------------------------------------
//	If unused_flag is set, then this is just a placeholder.  Don't actually reference vclips or load bbms.
#if defined(DXX_BUILD_DESCENT_I)
void bm_read_weapon(char *&arg, int skip, int unused_flag)
#elif defined(DXX_BUILD_DESCENT_II)
void bm_read_weapon(int skip, int unused_flag)
#endif
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
#if defined(DXX_BUILD_DESCENT_II)
	Assert(N_weapon_types <= MAX_WEAPON_TYPES);
#endif

	if (unused_flag) {
#if defined(DXX_BUILD_DESCENT_I)
		clear_to_end_of_line(arg);
#elif defined(DXX_BUILD_DESCENT_II)
		clear_to_end_of_line();
#endif
		return;
	}

	if (skip) {
#if defined(DXX_BUILD_DESCENT_I)
		clear_to_end_of_line(arg);
#elif defined(DXX_BUILD_DESCENT_II)
		clear_to_end_of_line();
#endif
		return;
	}

	// Initialize weapon array
	Weapon_info[n].render_type = WEAPON_RENDER_NONE;		// 0=laser, 1=blob, 2=object
	Weapon_info[n].bitmap.index = 0;
	Weapon_info[n].model_num = -1;
	Weapon_info[n].model_num_inner = -1;
	Weapon_info[n].blob_size = 0x1000;									// size of blob
	Weapon_info[n].flash_vclip = vclip_none;
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

#if defined(DXX_BUILD_DESCENT_II)
	Weapon_info[n].flags = 0;
#endif

	Weapon_info[n].lifetime = WEAPON_DEFAULT_LIFETIME;					//	Number of bursts fired from EACH GUN per firing.  For weapons which fire from both sides, 3*fire_count shots will be fired.

	Weapon_info[n].po_len_to_width_ratio = F1_0*10;

	Weapon_info[n].picture.index = 0;
#if defined(DXX_BUILD_DESCENT_II)
	Weapon_info[n].hires_picture.index = 0;
#endif
	Weapon_info[n].homing_flag = 0;

#if defined(DXX_BUILD_DESCENT_II)
	Weapon_info[n].flash = 0;
	Weapon_info[n].multi_damage_scale = F1_0;
	Weapon_info[n].afterburner_size = 0;
	Weapon_info[n].children = weapon_id_type::unspecified;
#endif

	// Process arguments
	arg = strtok( NULL, space_tab );

	lighted = 1;			//assume first texture is lighted

#if defined(DXX_BUILD_DESCENT_II)
	Weapon_info[n].speedvar = 128;
#endif

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
#if defined(DXX_BUILD_DESCENT_II)
				Assert(n_models < MAX_MODEL_VARIANTS);
#endif
			} else if (!d_stricmp( arg, "weapon_pof_inner" ))	{
				// Load pof file
				pof_file_inner = equal_ptr;
			} else if (!d_stricmp( arg, "strength" )) {
				for (i=0; i<NDL-1; i++) {
#if defined(DXX_BUILD_DESCENT_I)
					Weapon_info[n].strength[i] = i2f(atoi(equal_ptr));
#elif defined(DXX_BUILD_DESCENT_II)
					Weapon_info[n].strength[i] = fl2f(atof(equal_ptr));
#endif
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
			}
#if defined(DXX_BUILD_DESCENT_II)
			else if (!d_stricmp( arg, "speedvar" ))	{
				Weapon_info[n].speedvar = (atoi(equal_ptr) * 128) / 100;
			}
#endif
			else if (!d_stricmp( arg, "flash_vclip" ))	{
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
			}
#if defined(DXX_BUILD_DESCENT_II)
			else if (!d_stricmp(arg, "hires_picture" )) {
				Weapon_info[n].hires_picture = bm_load_sub(skip, equal_ptr);
			}
#endif
			else if (!d_stricmp(arg, "homing" )) {
				Weapon_info[n].homing_flag = !!atoi(equal_ptr);
			}
#if defined(DXX_BUILD_DESCENT_II)
			else if (!d_stricmp(arg, "flash" )) {
				Weapon_info[n].flash = atoi(equal_ptr);
			} else if (!d_stricmp(arg, "multi_damage_scale" )) {
				Weapon_info[n].multi_damage_scale = fl2f(atof(equal_ptr));
			} else if (!d_stricmp(arg, "afterburner_size" )) {
				Weapon_info[n].afterburner_size = f2i(16*fl2f(atof(equal_ptr)));
			} else if (!d_stricmp(arg, "children" )) {
				Weapon_info[n].children = static_cast<weapon_id_type>(atoi(equal_ptr));
			} else if (!d_stricmp(arg, "placable" )) {
				if (atoi(equal_ptr)) {
					Weapon_info[n].flags |= WIF_PLACABLE;
				}
			} else {
				Int3();
			}
#endif
		} else {			// Must be a texture specification...
			grs_bitmap *bm;

			bm = load_polymodel_bitmap(skip, arg);
#if defined(DXX_BUILD_DESCENT_I)
			if (bm && ! lighted)
#elif defined(DXX_BUILD_DESCENT_II)
			if (! lighted)
#endif
				bm->add_flags(BM_FLAG_NO_LIGHTING);

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

#if defined(DXX_BUILD_DESCENT_I)
void bm_read_powerup(char *&arg, int unused_flag)
#elif defined(DXX_BUILD_DESCENT_II)
void bm_read_powerup(int unused_flag)
#endif
{
	int n;
	char 	*equal_ptr;

	Assert(N_powerup_types < MAX_POWERUP_TYPES);

	n = N_powerup_types;
	N_powerup_types++;

	if (unused_flag) {
#if defined(DXX_BUILD_DESCENT_I)
		clear_to_end_of_line(arg);
#elif defined(DXX_BUILD_DESCENT_II)
		clear_to_end_of_line();
#endif
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
#if defined(DXX_BUILD_DESCENT_II)
			else {
				Int3();
			}
#endif
		}
#if defined(DXX_BUILD_DESCENT_II)
		else {			// Must be a texture specification...
			Int3();
		}
#endif
		arg = strtok( NULL, space_tab );
	}
#if defined(DXX_BUILD_DESCENT_I)
	ObjType[Num_total_object_types] = OL_POWERUP;
	ObjId[Num_total_object_types] = n;
	Num_total_object_types++;
#endif
}

#if defined(DXX_BUILD_DESCENT_I)
void bm_read_hostage(char *&arg)
#elif defined(DXX_BUILD_DESCENT_II)
void bm_read_hostage()
#endif
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

#if defined(DXX_BUILD_DESCENT_II)
			else {
				Int3();
			}
#endif
		}
#if defined(DXX_BUILD_DESCENT_II)
		else {
			Int3();
		}
#endif
		arg = strtok( NULL, space_tab );
	}
#if defined(DXX_BUILD_DESCENT_I)
	ObjType[Num_total_object_types] = OL_HOSTAGE;
	ObjId[Num_total_object_types] = n;
	Num_total_object_types++;
#endif
}

#if defined(DXX_BUILD_DESCENT_I)
DEFINE_SERIAL_UDT_TO_MESSAGE(tmap_info, t, (static_cast<const array<char, 13> &>(t.filename), t.flags, t.lighting, t.damage, t.eclip_num));
ASSERT_SERIAL_UDT_MESSAGE_SIZE(tmap_info, 26);
#elif defined(DXX_BUILD_DESCENT_II)
DEFINE_SERIAL_UDT_TO_MESSAGE(tmap_info, t, (t.flags, serial::pad<3>(), t.lighting, t.damage, t.eclip_num, t.destroyed, t.slide_u, t.slide_v));
ASSERT_SERIAL_UDT_MESSAGE_SIZE(tmap_info, 20);
#endif
