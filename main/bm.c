/* $Id: bm.c,v 1.14 2002-08-06 04:53:48 btb Exp $ */
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
 * Bitmap and palette loading functions.
 *
 *
 */

#ifdef HAVE_CONFIG_H
#include <conf.h>
#endif

#include <stdio.h>
#include <stdlib.h>

#include "pstypes.h"
#include "inferno.h"
#include "gr.h"
#include "bm.h"
#include "u_mem.h"
#include "mono.h"
#include "error.h"
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
#include "cfile.h"
#include "powerup.h"
#include "sounds.h"
#include "piggy.h"
#include "aistruct.h"
#include "robot.h"
#include "weapon.h"
#include "gauges.h"
#include "player.h"
#include "endlevel.h"
#include "cntrlcen.h"
#include "byteswap.h"
#include "makesig.h"

ubyte Sounds[MAX_SOUNDS];
ubyte AltSounds[MAX_SOUNDS];

#ifdef EDITOR
int Num_total_object_types;
byte	ObjType[MAX_OBJTYPE];
byte	ObjId[MAX_OBJTYPE];
fix	ObjStrength[MAX_OBJTYPE];
#endif

//for each model, a model number for dying & dead variants, or -1 if none
int Dying_modelnums[MAX_POLYGON_MODELS];
int Dead_modelnums[MAX_POLYGON_MODELS];

//the polygon model number to use for the marker
int	Marker_model_num = -1;

//right now there's only one player ship, but we can have another by 
//adding an array and setting the pointer to the active ship.
player_ship only_player_ship,*Player_ship=&only_player_ship;

//----------------- Miscellaneous bitmap pointers ---------------
int					Num_cockpits = 0;
bitmap_index		cockpit_bitmap[N_COCKPIT_BITMAPS];

//---------------- Variables for wall textures ------------------
int 					Num_tmaps;
tmap_info 			TmapInfo[MAX_TEXTURES];

//---------------- Variables for object textures ----------------

int					First_multi_bitmap_num=-1;

bitmap_index		ObjBitmaps[MAX_OBJ_BITMAPS];
ushort				ObjBitmapPtrs[MAX_OBJ_BITMAPS];		// These point back into ObjBitmaps, since some are used twice.

#ifdef FAST_FILE_IO
#define tmap_info_read_n(ti, n, fp) cfread(ti, sizeof(tmap_info), n, fp)
#else
/*
 * reads n tmap_info structs from a CFILE
 */
int tmap_info_read_n(tmap_info *ti, int n, CFILE *fp)
{
	int i;

	for (i = 0; i < n; i++) {
		ti[i].flags = cfile_read_byte(fp);
		ti[i].pad[0] = cfile_read_byte(fp);
		ti[i].pad[1] = cfile_read_byte(fp);
		ti[i].pad[2] = cfile_read_byte(fp);
		ti[i].lighting = cfile_read_fix(fp);
		ti[i].damage = cfile_read_fix(fp);
		ti[i].eclip_num = cfile_read_short(fp);
		ti[i].destroyed = cfile_read_short(fp);
		ti[i].slide_u = cfile_read_short(fp);
		ti[i].slide_v = cfile_read_short(fp);
	}
	return i;
}
#endif

//#ifdef MACINTOSH

extern int exit_modelnum,destroyed_exit_modelnum, Num_bitmap_files;
int N_ObjBitmaps, extra_bitmap_num;

bitmap_index exitmodel_bm_load_sub( char * filename )
{
	bitmap_index bitmap_num;
	grs_bitmap * new;
	ubyte newpal[256*3];
	int iff_error;		//reference parm to avoid warning message

	bitmap_num.index = 0;

	MALLOC( new, grs_bitmap, 1 );
	iff_error = iff_read_bitmap(filename,new,BM_LINEAR,newpal);
	new->bm_handle=0;
	if (iff_error != IFF_NO_ERROR)		{
		Error("Error loading exit model bitmap <%s> - IFF error: %s",filename,iff_errormsg(iff_error));
	}

	if ( iff_has_transparency )
		gr_remap_bitmap_good( new, newpal, iff_transparent_color, 254 );
	else
		gr_remap_bitmap_good( new, newpal, -1, 254 );

	new->avg_color = 0;	//compute_average_pixel(new);

	bitmap_num.index = extra_bitmap_num;

	GameBitmaps[extra_bitmap_num++] = *new;

	d_free( new );
	return bitmap_num;
}

grs_bitmap *load_exit_model_bitmap(char *name)
{
	Assert(N_ObjBitmaps < MAX_OBJ_BITMAPS);

	{
		ObjBitmaps[N_ObjBitmaps] = exitmodel_bm_load_sub(name);
		if (GameBitmaps[ObjBitmaps[N_ObjBitmaps].index].bm_w!=64 || GameBitmaps[ObjBitmaps[N_ObjBitmaps].index].bm_h!=64)
			Error("Bitmap <%s> is not 64x64",name);
		ObjBitmapPtrs[N_ObjBitmaps] = N_ObjBitmaps;
		N_ObjBitmaps++;
		Assert(N_ObjBitmaps < MAX_OBJ_BITMAPS);
		return &GameBitmaps[ObjBitmaps[N_ObjBitmaps-1].index];
	}
}

void load_exit_models()
{
	CFILE *exit_hamfile;
	int start_num;

	start_num = N_ObjBitmaps;
	extra_bitmap_num = Num_bitmap_files;
	load_exit_model_bitmap("steel1.bbm");
	load_exit_model_bitmap("rbot061.bbm");
	load_exit_model_bitmap("rbot062.bbm");

	load_exit_model_bitmap("steel1.bbm");
	load_exit_model_bitmap("rbot061.bbm");
	load_exit_model_bitmap("rbot063.bbm");

	exit_hamfile = cfopen(":Data:exit.ham","rb");

	exit_modelnum = N_polygon_models++;
	destroyed_exit_modelnum = N_polygon_models++;

	polymodel_read(&Polygon_models[exit_modelnum], exit_hamfile);
	polymodel_read(&Polygon_models[destroyed_exit_modelnum], exit_hamfile);
#ifdef MACINTOSH //not sure what these are for
	Polygon_models[exit_modelnum].first_texture = start_num;
	Polygon_models[destroyed_exit_modelnum].first_texture = start_num+3;
#endif

	Polygon_models[exit_modelnum].model_data = d_malloc(Polygon_models[exit_modelnum].model_data_size);
	Assert( Polygon_models[exit_modelnum].model_data != NULL );
	cfread( Polygon_models[exit_modelnum].model_data, sizeof(ubyte), Polygon_models[exit_modelnum].model_data_size, exit_hamfile );
#ifdef WORDS_BIGENDIAN
	swap_polygon_model_data(Polygon_models[exit_modelnum].model_data);
#endif
	g3_init_polygon_model(Polygon_models[exit_modelnum].model_data);

	Polygon_models[destroyed_exit_modelnum].model_data = d_malloc(Polygon_models[destroyed_exit_modelnum].model_data_size);
	Assert( Polygon_models[destroyed_exit_modelnum].model_data != NULL );
	cfread( Polygon_models[destroyed_exit_modelnum].model_data, sizeof(ubyte), Polygon_models[destroyed_exit_modelnum].model_data_size, exit_hamfile );
#ifdef WORDS_BIGENDIAN
	swap_polygon_model_data(Polygon_models[destroyed_exit_modelnum].model_data);
#endif
	g3_init_polygon_model(Polygon_models[destroyed_exit_modelnum].model_data);

	cfclose(exit_hamfile);

}

//#endif		// MACINTOSH

//-----------------------------------------------------------------
// Read data from piggy.
// This is called when the editor is OUT.
// If editor is in, bm_init_use_table() is called.
int bm_init()
{
	init_polygon_models();
	if (! piggy_init())				// This calls bm_read_all
		Error("Cannot open pig and/or ham file");

	piggy_read_sounds();

	init_endlevel();		//this is in bm_init_use_tbl(), so I gues it goes here

	return 0;
}

void bm_read_all(CFILE * fp)
{
	int i,t;

	NumTextures = cfile_read_int(fp);
	bitmap_index_read_n(Textures, NumTextures, fp );
	tmap_info_read_n(TmapInfo, NumTextures, fp);

	t = cfile_read_int(fp);
	cfread( Sounds, sizeof(ubyte), t, fp );
	cfread( AltSounds, sizeof(ubyte), t, fp );

	Num_vclips = cfile_read_int(fp);
	vclip_read_n(Vclip, Num_vclips, fp);

	Num_effects = cfile_read_int(fp);
	eclip_read_n(Effects, Num_effects, fp);

	Num_wall_anims = cfile_read_int(fp);
	wclip_read_n(WallAnims, Num_wall_anims, fp);

	N_robot_types = cfile_read_int(fp);
	robot_info_read_n(Robot_info, N_robot_types, fp);

	N_robot_joints = cfile_read_int(fp);
	jointpos_read_n(Robot_joints, N_robot_joints, fp);

	N_weapon_types = cfile_read_int(fp);
	weapon_info_read_n(Weapon_info, N_weapon_types, fp, Piggy_hamfile_version);

	N_powerup_types = cfile_read_int(fp);
	powerup_type_info_read_n(Powerup_info, N_powerup_types, fp);

	N_polygon_models = cfile_read_int(fp);
	polymodel_read_n(Polygon_models, N_polygon_models, fp);

	for (i=0; i<N_polygon_models; i++ )	{
		Polygon_models[i].model_data = d_malloc(Polygon_models[i].model_data_size);
		Assert( Polygon_models[i].model_data != NULL );
		cfread( Polygon_models[i].model_data, sizeof(ubyte), Polygon_models[i].model_data_size, fp );
#ifdef WORDS_BIGENDIAN
		swap_polygon_model_data(Polygon_models[i].model_data);
#endif
		g3_init_polygon_model(Polygon_models[i].model_data);
	}

	for (i = 0; i < N_polygon_models; i++)
		Dying_modelnums[i] = cfile_read_int(fp);
	for (i = 0; i < N_polygon_models; i++)
		Dead_modelnums[i] = cfile_read_int(fp);

	t = cfile_read_int(fp);
	bitmap_index_read_n(Gauges, t, fp);
	bitmap_index_read_n(Gauges_hires, t, fp);

	N_ObjBitmaps = cfile_read_int(fp);
	bitmap_index_read_n(ObjBitmaps, N_ObjBitmaps, fp);
	for (i = 0; i < N_ObjBitmaps; i++)
		ObjBitmapPtrs[i] = cfile_read_short(fp);

	player_ship_read(&only_player_ship, fp);

	Num_cockpits = cfile_read_int(fp);
	bitmap_index_read_n(cockpit_bitmap, Num_cockpits, fp);

//@@	cfread( &Num_total_object_types, sizeof(int), 1, fp );
//@@	cfread( ObjType, sizeof(byte), Num_total_object_types, fp );
//@@	cfread( ObjId, sizeof(byte), Num_total_object_types, fp );
//@@	cfread( ObjStrength, sizeof(fix), Num_total_object_types, fp );

	First_multi_bitmap_num = cfile_read_int(fp);

	Num_reactors = cfile_read_int(fp);
	reactor_read_n(Reactors, Num_reactors, fp);

	Marker_model_num = cfile_read_int(fp);

	//@@cfread( &N_controlcen_guns, sizeof(int), 1, fp );
	//@@cfread( controlcen_gun_points, sizeof(vms_vector), N_controlcen_guns, fp );
	//@@cfread( controlcen_gun_dirs, sizeof(vms_vector), N_controlcen_guns, fp );

	if (Piggy_hamfile_version < 3) {
		exit_modelnum = cfile_read_int(fp);
		destroyed_exit_modelnum = cfile_read_int(fp);
	}

}


//these values are the number of each item in the release of d2
//extra items added after the release get written in an additional hamfile
#define N_D2_ROBOT_TYPES		66
#define N_D2_ROBOT_JOINTS		1145
#define N_D2_POLYGON_MODELS		166
#define N_D2_OBJBITMAPS			422
#define N_D2_OBJBITMAPPTRS		502
#define N_D2_WEAPON_TYPES		62

//type==1 means 1.1, type==2 means 1.2 (with weaons)
void bm_read_extra_robots(char *fname,int type)
{
	CFILE *fp;
	int t,i;
	int version;

	#ifdef MACINTOSH
		ulong varSave = 0;
	#endif

	fp = cfopen(fname,"rb");

	if (type == 2) {
		int sig;

		sig = cfile_read_int(fp);
		if (sig != MAKE_SIG('X','H','A','M'))
			return;
		version = cfile_read_int(fp);
	}
	else
		version = 0;

	//read extra weapons

	t = cfile_read_int(fp);
	N_weapon_types = N_D2_WEAPON_TYPES+t;
	if (N_weapon_types >= MAX_WEAPON_TYPES)
		Error("Too many weapons (%d) in <%s>.  Max is %d.",t,fname,MAX_WEAPON_TYPES-N_D2_WEAPON_TYPES);
	weapon_info_read_n(&Weapon_info[N_D2_WEAPON_TYPES], t, fp, version);

	//now read robot info

	t = cfile_read_int(fp);
	N_robot_types = N_D2_ROBOT_TYPES+t;
	if (N_robot_types >= MAX_ROBOT_TYPES)
		Error("Too many robots (%d) in <%s>.  Max is %d.",t,fname,MAX_ROBOT_TYPES-N_D2_ROBOT_TYPES);
	robot_info_read_n(&Robot_info[N_D2_ROBOT_TYPES], t, fp);

	t = cfile_read_int(fp);
	N_robot_joints = N_D2_ROBOT_JOINTS+t;
	if (N_robot_joints >= MAX_ROBOT_JOINTS)
		Error("Too many robot joints (%d) in <%s>.  Max is %d.",t,fname,MAX_ROBOT_JOINTS-N_D2_ROBOT_JOINTS);
	jointpos_read_n(&Robot_joints[N_D2_ROBOT_JOINTS], t, fp);

	t = cfile_read_int(fp);
	N_polygon_models = N_D2_POLYGON_MODELS+t;
	if (N_polygon_models >= MAX_POLYGON_MODELS)
		Error("Too many polygon models (%d) in <%s>.  Max is %d.",t,fname,MAX_POLYGON_MODELS-N_D2_POLYGON_MODELS);
	polymodel_read_n(&Polygon_models[N_D2_POLYGON_MODELS], t, fp);

	for (i=N_D2_POLYGON_MODELS; i<N_polygon_models; i++ )
	{
		Polygon_models[i].model_data = d_malloc(Polygon_models[i].model_data_size);
		Assert( Polygon_models[i].model_data != NULL );
		cfread( Polygon_models[i].model_data, sizeof(ubyte), Polygon_models[i].model_data_size, fp );
#ifdef WORDS_BIGENDIAN
		swap_polygon_model_data(Polygon_models[i].model_data);
#endif
		g3_init_polygon_model(Polygon_models[i].model_data);
	}

	for (i = N_D2_POLYGON_MODELS; i < N_polygon_models; i++)
		Dying_modelnums[i] = cfile_read_int(fp);
	for (i = N_D2_POLYGON_MODELS; i < N_polygon_models; i++)
		Dead_modelnums[i] = cfile_read_int(fp);

	t = cfile_read_int(fp);
	if (N_D2_OBJBITMAPS+t >= MAX_OBJ_BITMAPS)
		Error("Too many object bitmaps (%d) in <%s>.  Max is %d.",t,fname,MAX_OBJ_BITMAPS-N_D2_OBJBITMAPS);
	bitmap_index_read_n(&ObjBitmaps[N_D2_OBJBITMAPS], t, fp);

	t = cfile_read_int(fp);
	if (N_D2_OBJBITMAPPTRS+t >= MAX_OBJ_BITMAPS)
		Error("Too many object bitmap pointers (%d) in <%s>.  Max is %d.",t,fname,MAX_OBJ_BITMAPS-N_D2_OBJBITMAPPTRS);
	for (i = N_D2_OBJBITMAPPTRS; i < (N_D2_OBJBITMAPPTRS + t); i++)
		ObjBitmapPtrs[i] = cfile_read_short(fp);

	cfclose(fp);
}

extern void change_filename_extension( char *dest, char *src, char *new_ext );

int Robot_replacements_loaded = 0;

void load_robot_replacements(char *level_name)
{
	CFILE *fp;
	int t,i,j;
	char ifile_name[FILENAME_LEN];

	change_filename_extension(ifile_name, level_name, ".HXM" );

	fp = cfopen(ifile_name,"rb");

	if (!fp)		//no robot replacement file
		return;

	t = cfile_read_int(fp);			//read id "HXM!"
	if (t!= 0x21584d48)
		Error("ID of HXM! file incorrect");

	t = cfile_read_int(fp);			//read version
	if (t<1)
		Error("HXM! version too old (%d)",t);

	t = cfile_read_int(fp);			//read number of robots
	for (j=0;j<t;j++) {
		i = cfile_read_int(fp);		//read robot number
		if (i<0 || i>=N_robot_types)
			Error("Robots number (%d) out of range in (%s).  Range = [0..%d].",i,level_name,N_robot_types-1);
		robot_info_read_n(&Robot_info[i], 1, fp);
	}

	t = cfile_read_int(fp);			//read number of joints
	for (j=0;j<t;j++) {
		i = cfile_read_int(fp);		//read joint number
		if (i<0 || i>=N_robot_joints)
			Error("Robots joint (%d) out of range in (%s).  Range = [0..%d].",i,level_name,N_robot_joints-1);
		jointpos_read_n(&Robot_joints[i], 1, fp);
	}

	t = cfile_read_int(fp);			//read number of polygon models
	for (j=0;j<t;j++)
	{
		i = cfile_read_int(fp);		//read model number
		if (i<0 || i>=N_polygon_models)
			Error("Polygon model (%d) out of range in (%s).  Range = [0..%d].",i,level_name,N_polygon_models-1);
		polymodel_read(&Polygon_models[i], fp);
	
		d_free(Polygon_models[i].model_data);
		Polygon_models[i].model_data = d_malloc(Polygon_models[i].model_data_size);
		Assert( Polygon_models[i].model_data != NULL );

		cfread( Polygon_models[i].model_data, sizeof(ubyte), Polygon_models[i].model_data_size, fp );
#ifdef WORDS_BIGENDIAN
		swap_polygon_model_data(Polygon_models[i].model_data);
#endif
		g3_init_polygon_model(Polygon_models[i].model_data);

		Dying_modelnums[i] = cfile_read_int(fp);
		Dead_modelnums[i] = cfile_read_int(fp);
	}

	t = cfile_read_int(fp);			//read number of objbitmaps
	for (j=0;j<t;j++) {
		i = cfile_read_int(fp);		//read objbitmap number
		if (i<0 || i>=MAX_OBJ_BITMAPS)
			Error("Object bitmap number (%d) out of range in (%s).  Range = [0..%d].",i,level_name,MAX_OBJ_BITMAPS-1);
		bitmap_index_read(&ObjBitmaps[i], fp);
	}

	t = cfile_read_int(fp);			//read number of objbitmapptrs
	for (j=0;j<t;j++) {
		i = cfile_read_int(fp);		//read objbitmapptr number
		if (i<0 || i>=MAX_OBJ_BITMAPS)
			Error("Object bitmap pointer (%d) out of range in (%s).  Range = [0..%d].",i,level_name,MAX_OBJ_BITMAPS-1);
		ObjBitmapPtrs[i] = cfile_read_short(fp);
	}

	cfclose(fp);
}
