/* $Id: bm.c,v 1.31 2003-03-29 22:34:59 btb Exp $ */
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
 * Old Log:
 * Revision 1.1  1995/05/16  15:23:08  allender
 * Initial revision
 *
 * Revision 2.3  1995/03/14  16:22:04  john
 * Added cdrom alternate directory stuff.
 *
 * Revision 2.2  1995/03/07  16:51:48  john
 * Fixed robots not moving without edtiro bug.
 *
 * Revision 2.1  1995/03/06  15:23:06  john
 * New screen techniques.
 *
 * Revision 2.0  1995/02/27  11:27:05  john
 * New version 2.0, which has no anonymous unions, builds with
 * Watcom 10.0, and doesn't require parsing BITMAPS.TBL.
 *
 */

#ifdef HAVE_CONFIG_H
#include <conf.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

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
#include "makesig.h"
#include "interp.h"

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
int             Num_cockpits = 0;
bitmap_index    cockpit_bitmap[N_COCKPIT_BITMAPS];

//---------------- Variables for wall textures ------------------
int             Num_tmaps;
tmap_info       TmapInfo[MAX_TEXTURES];

//---------------- Variables for object textures ----------------

int             First_multi_bitmap_num=-1;

int             N_ObjBitmaps;
bitmap_index    ObjBitmaps[MAX_OBJ_BITMAPS];
ushort          ObjBitmapPtrs[MAX_OBJ_BITMAPS];     // These point back into ObjBitmaps, since some are used twice.

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

int tmap_info_read_n_d1(tmap_info *ti, int n, CFILE *fp)
{
	int i;

	for (i = 0; i < n; i++) {
		cfseek(fp, 13, SEEK_CUR);// skip filename
		ti[i].flags = cfile_read_byte(fp);
		ti[i].lighting = cfile_read_fix(fp);
		ti[i].damage = cfile_read_fix(fp);
		ti[i].eclip_num = cfile_read_int(fp);
	}
	return i;
}


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

	for (i=0; i<N_polygon_models; i++ )
		polygon_model_data_read(&Polygon_models[i], fp);

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
	else
		exit_modelnum = destroyed_exit_modelnum = N_polygon_models;
}

#define D1_MAX_TEXTURES 800
#define D1_MAX_SOUNDS 250
#define D1_MAX_VCLIPS 70
#define D1_MAX_EFFECTS 60
#define D1_MAX_WALL_ANIMS 30
#define D1_MAX_ROBOT_TYPES 30
#define D1_MAX_ROBOT_JOINTS 600
#define D1_MAX_WEAPON_TYPES 30
#define D1_MAX_POWERUP_TYPES 29
#define D1_MAX_GAUGE_BMS 80
#define D1_MAX_OBJ_BITMAPS 210
#define D1_MAX_COCKPIT_BITMAPS 4
#define D1_MAX_OBJTYPE 100
#define D1_MAX_POLYGON_MODELS 85

#define D1_TMAP_INFO_SIZE 26
#define D1_VCLIP_SIZE 66
#define D1_ROBOT_INFO_SIZE 486
#define D1_WEAPON_INFO_SIZE 115

#define D1_LAST_STATIC_TMAP_NUM 324

// store the Textures[] array as read from the descent 2 pig.
short *d2_Textures_backup = NULL;

void undo_bm_read_all_d1() {
	if (d2_Textures_backup) {
		int i;
		for (i = 0; i < D1_LAST_STATIC_TMAP_NUM; i++)
			Textures[i].index = d2_Textures_backup[i];
		d_free(d2_Textures_backup);
		d2_Textures_backup = NULL;
	}
}

/*
 * used by piggy_d1_init to read in descent 1 pigfile
 */
void bm_read_all_d1(CFILE * fp)
{
	int i;

	atexit(undo_bm_read_all_d1);

	/*NumTextures = */ cfile_read_int(fp);
	//bitmap_index_read_n(Textures, D1_MAX_TEXTURES, fp );
	//for (i = 0; i < D1_MAX_TEXTURES; i++)
	//	Textures[i].index = cfile_read_short(fp) + 600;  
	//cfseek(fp, D1_MAX_TEXTURES * sizeof(short), SEEK_CUR);
	MALLOC(d2_Textures_backup, short, D1_LAST_STATIC_TMAP_NUM);
	for (i = 0; i < D1_LAST_STATIC_TMAP_NUM; i++) {
		d2_Textures_backup[i] = Textures[i].index;
		Textures[i].index = cfile_read_short(fp) + 521;
	}
	cfseek(fp, (D1_MAX_TEXTURES - D1_LAST_STATIC_TMAP_NUM) * sizeof(short), SEEK_CUR);

	//tmap_info_read_n_d1(TmapInfo, D1_MAX_TEXTURES, fp);
	cfseek(fp, D1_MAX_TEXTURES * D1_TMAP_INFO_SIZE, SEEK_CUR);

	/*
	cfread( Sounds, sizeof(ubyte), D1_MAX_SOUNDS, fp );
	cfread( AltSounds, sizeof(ubyte), D1_MAX_SOUNDS, fp );
	*/cfseek(fp, D1_MAX_SOUNDS * 2, SEEK_CUR);

	/*Num_vclips = */ cfile_read_int(fp);
	//vclip_read_n(Vclip, D1_MAX_VCLIPS, fp);
	cfseek(fp, D1_MAX_VCLIPS * D1_VCLIP_SIZE, SEEK_CUR);

	/*
	Num_effects = cfile_read_int(fp);
	eclip_read_n(Effects, D1_MAX_EFFECTS, fp);

	Num_wall_anims = cfile_read_int(fp);
	wclip_read_n_d1(WallAnims, D1_MAX_WALL_ANIMS, fp);
	*/

	/*
	N_robot_types = cfile_read_int(fp);
	//robot_info_read_n(Robot_info, D1_MAX_ROBOT_TYPES, fp);
	cfseek(fp, D1_MAX_ROBOT_TYPES * D1_ROBOT_INFO_SIZE, SEEK_CUR);

	N_robot_joints = cfile_read_int(fp);
	jointpos_read_n(Robot_joints, D1_MAX_ROBOT_JOINTS, fp);

	N_weapon_types = cfile_read_int(fp);
	//weapon_info_read_n(Weapon_info, D1_MAX_WEAPON_TYPES, fp, Piggy_hamfile_version);
	cfseek(fp, D1_MAX_WEAPON_TYPES * D1_WEAPON_INFO_SIZE, SEEK_CUR);

	N_powerup_types = cfile_read_int(fp);
	powerup_type_info_read_n(Powerup_info, D1_MAX_POWERUP_TYPES, fp);
	*/

	/* in the following code are bugs, solved by hack
	N_polygon_models = cfile_read_int(fp);
	polymodel_read_n(Polygon_models, N_polygon_models, fp);
	for (i=0; i<N_polygon_models; i++ )
		polygon_model_data_read(&Polygon_models[i], fp);
	*/cfseek(fp, 521490-160, SEEK_SET); // OK, I admit, this is a dirty hack
	//bitmap_index_read_n(Gauges, D1_MAX_GAUGE_BMS, fp);
	cfseek(fp, D1_MAX_GAUGE_BMS * sizeof(bitmap_index), SEEK_CUR);

	/*
	for (i = 0; i < D1_MAX_POLYGON_MODELS; i++)
		Dying_modelnums[i] = cfile_read_int(fp);
	for (i = 0; i < D1_MAX_POLYGON_MODELS; i++)
		Dead_modelnums[i] = cfile_read_int(fp);
	*/ cfseek(fp, D1_MAX_POLYGON_MODELS * 8, SEEK_CUR);

	//bitmap_index_read_n(ObjBitmaps, D1_MAX_OBJ_BITMAPS, fp);
	cfseek(fp, D1_MAX_OBJ_BITMAPS * sizeof(bitmap_index), SEEK_CUR);
	for (i = 0; i < D1_MAX_OBJ_BITMAPS; i++)
		cfseek(fp, 2, SEEK_CUR);//ObjBitmapPtrs[i] = cfile_read_short(fp);

	//player_ship_read(&only_player_ship, fp);
	cfseek(fp, sizeof(player_ship), SEEK_CUR);

	/*Num_cockpits = */ cfile_read_int(fp);
	//bitmap_index_read_n(cockpit_bitmap, D1_MAX_COCKPIT_BITMAPS, fp);
	cfseek(fp, D1_MAX_COCKPIT_BITMAPS * sizeof(bitmap_index), SEEK_CUR);

	/*
	cfread( Sounds, sizeof(ubyte), D1_MAX_SOUNDS, fp );
	cfread( AltSounds, sizeof(ubyte), D1_MAX_SOUNDS, fp );
	*/cfseek(fp, D1_MAX_SOUNDS * 2, SEEK_CUR);

	/*Num_total_object_types = */ cfile_read_int( fp );
	/*
	cfread( ObjType, sizeof(byte), D1_MAX_OBJTYPE, fp );
	cfread( ObjId, sizeof(byte), D1_MAX_OBJTYPE, fp );
	for (i=0; i<D1_MAX_OBJTYPE; i++ )
		ObjStrength[i] = cfile_read_int( fp );
	*/ cfseek(fp, D1_MAX_OBJTYPE * 6, SEEK_CUR);

	/*First_multi_bitmap_num =*/ cfile_read_int(fp);
	/*Reactors[0].n_guns = */ cfile_read_int( fp );
	/*for (i=0; i<4; i++)
		cfile_read_vector(&(Reactors[0].gun_points[i]), fp);
	for (i=0; i<4; i++)
		cfile_read_vector(&(Reactors[0].gun_dirs[i]), fp);
	*/cfseek(fp, 8 * 12, SEEK_CUR);

	/*exit_modelnum = */ cfile_read_int(fp);
	/*destroyed_exit_modelnum = */ cfile_read_int(fp);
}

//these values are the number of each item in the release of d2
//extra items added after the release get written in an additional hamfile
#define N_D2_ROBOT_TYPES		66
#define N_D2_ROBOT_JOINTS		1145
#define N_D2_POLYGON_MODELS     166
#define N_D2_OBJBITMAPS			422
#define N_D2_OBJBITMAPPTRS		502
#define N_D2_WEAPON_TYPES		62

extern int Num_bitmap_files;
int extra_bitmap_num = 0;

void bm_free_extra_objbitmaps()
{
	int i;

	if (!extra_bitmap_num)
		extra_bitmap_num = Num_bitmap_files;

	for (i = Num_bitmap_files; i < extra_bitmap_num; i++)
	{
		N_ObjBitmaps--;
		d_free(GameBitmaps[i].bm_data);
	}
	extra_bitmap_num = Num_bitmap_files;
}

void bm_free_extra_models()
{
	while (N_polygon_models > N_D2_POLYGON_MODELS)
		free_model(&Polygon_models[--N_polygon_models]);
	while (N_polygon_models > exit_modelnum)
		free_model(&Polygon_models[--N_polygon_models]);
}

//type==1 means 1.1, type==2 means 1.2 (with weapons)
void bm_read_extra_robots(char *fname,int type)
{
	CFILE *fp;
	int t,i;
	int version;

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

	bm_free_extra_models();
	bm_free_extra_objbitmaps();

	//read extra weapons

	t = cfile_read_int(fp);
	N_weapon_types = N_D2_WEAPON_TYPES+t;
	if (N_weapon_types >= MAX_WEAPON_TYPES)
		Error("Too many weapons (%d) in <%s>.  Max is %d.",t,fname,MAX_WEAPON_TYPES-N_D2_WEAPON_TYPES);
	weapon_info_read_n(&Weapon_info[N_D2_WEAPON_TYPES], t, fp, 3);

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
		polygon_model_data_read(&Polygon_models[i], fp);

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

		free_model(&Polygon_models[i]);
		polygon_model_data_read(&Polygon_models[i], fp);

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


/*
 * Routines for loading exit models
 *
 * Used by d1 levels (including some add-ons), and by d2 shareware.
 * Could potentially be used by d2 add-on levels, but only if they
 * don't use "extra" robots...
 */

// formerly exitmodel_bm_load_sub
bitmap_index read_extra_bitmap_iff( char * filename )
{
	bitmap_index bitmap_num;
	grs_bitmap * new = &GameBitmaps[extra_bitmap_num];
	ubyte newpal[256*3];
	int iff_error;		//reference parm to avoid warning message

	bitmap_num.index = 0;

	//MALLOC( new, grs_bitmap, 1 );
	iff_error = iff_read_bitmap(filename,new,BM_LINEAR,newpal);
	new->bm_handle=0;
	if (iff_error != IFF_NO_ERROR)		{
		con_printf(CON_DEBUG, "Error loading exit model bitmap <%s> - IFF error: %s\n", filename, iff_errormsg(iff_error));
		return bitmap_num;
	}

	if ( iff_has_transparency )
		gr_remap_bitmap_good( new, newpal, iff_transparent_color, 254 );
	else
		gr_remap_bitmap_good( new, newpal, -1, 254 );

	new->avg_color = 0;	//compute_average_pixel(new);

	bitmap_num.index = extra_bitmap_num;

	GameBitmaps[extra_bitmap_num++] = *new;

	//d_free( new );
	return bitmap_num;
}

// formerly load_exit_model_bitmap
grs_bitmap *bm_load_extra_objbitmap(char *name)
{
	Assert(N_ObjBitmaps < MAX_OBJ_BITMAPS);

	{
		ObjBitmaps[N_ObjBitmaps] = read_extra_bitmap_iff(name);

		if (ObjBitmaps[N_ObjBitmaps].index == 0)
		{
			char *name2 = d_strdup(name);
			*strrchr(name2, '.') = '\0';
			ObjBitmaps[N_ObjBitmaps] = read_extra_bitmap_d1_pig(name2);
			d_free(name2);
		}
		if (ObjBitmaps[N_ObjBitmaps].index == 0)
			return NULL;

		if (GameBitmaps[ObjBitmaps[N_ObjBitmaps].index].bm_w!=64 || GameBitmaps[ObjBitmaps[N_ObjBitmaps].index].bm_h!=64)
			Error("Bitmap <%s> is not 64x64",name);
		ObjBitmapPtrs[N_ObjBitmaps] = N_ObjBitmaps;
		N_ObjBitmaps++;
		Assert(N_ObjBitmaps < MAX_OBJ_BITMAPS);
		return &GameBitmaps[ObjBitmaps[N_ObjBitmaps-1].index];
	}
}

#ifdef OGL
void ogl_cache_polymodel_textures(int model_num);
#endif

int load_exit_models()
{
	CFILE *exit_hamfile;
	int start_num;

	bm_free_extra_models();
	bm_free_extra_objbitmaps();

	start_num = N_ObjBitmaps;
	if (!bm_load_extra_objbitmap("steel1.bbm") ||
		!bm_load_extra_objbitmap("rbot061.bbm") ||
		!bm_load_extra_objbitmap("rbot062.bbm") ||
		!bm_load_extra_objbitmap("steel1.bbm") ||
		!bm_load_extra_objbitmap("rbot061.bbm") ||
		!bm_load_extra_objbitmap("rbot063.bbm"))
	{
		Warning("Can't load exit models!\n");
		return 0;
	}

#ifndef MACINTOSH
	exit_hamfile = cfopen("exit.ham","rb");
#else
	exit_hamfile = cfopen(":Data:exit.ham","rb");
#endif
	if (exit_hamfile) {
		exit_modelnum = N_polygon_models++;
		destroyed_exit_modelnum = N_polygon_models++;
		polymodel_read(&Polygon_models[exit_modelnum], exit_hamfile);
		polymodel_read(&Polygon_models[destroyed_exit_modelnum], exit_hamfile);
		Polygon_models[exit_modelnum].first_texture = start_num;
		Polygon_models[destroyed_exit_modelnum].first_texture = start_num+3;

		polygon_model_data_read(&Polygon_models[exit_modelnum], exit_hamfile);

		polygon_model_data_read(&Polygon_models[destroyed_exit_modelnum], exit_hamfile);

		cfclose(exit_hamfile);

	} else if (cfexist("exit01.pof") && cfexist("exit01d.pof")) {

		exit_modelnum = load_polygon_model("exit01.pof", 3, start_num, NULL);
		destroyed_exit_modelnum = load_polygon_model("exit01d.pof", 3, start_num + 3, NULL);

#ifdef OGL
		ogl_cache_polymodel_textures(exit_modelnum);
		ogl_cache_polymodel_textures(destroyed_exit_modelnum);
#endif
	}
	else if (cfexist(D1_PIGFILE))
	{
		int offset, offset2;

		exit_hamfile = cfopen(D1_PIGFILE, "rb");
		switch (cfilelength(exit_hamfile)) { //total hack for loading models
		case D1_PIGSIZE:
			offset = 91848;     /* and 92582  */
			offset2 = 383390;   /* and 394022 */
			break;
		default:
		case D1_SHAREWARE_10_PIGSIZE:
		case D1_SHAREWARE_PIGSIZE:
			Int3();             /* exit models should be in .pofs */
		case D1_OEM_PIGSIZE:
		case D1_MAC_PIGSIZE:
		case D1_MAC_SHARE_PIGSIZE:
			Warning("Can't load exit models!\n");
			return 0;
			break;
		}
		cfseek(exit_hamfile, offset, SEEK_SET);
		exit_modelnum = N_polygon_models++;
		destroyed_exit_modelnum = N_polygon_models++;
		polymodel_read(&Polygon_models[exit_modelnum], exit_hamfile);
		polymodel_read(&Polygon_models[destroyed_exit_modelnum], exit_hamfile);
		Polygon_models[exit_modelnum].first_texture = start_num;
		Polygon_models[destroyed_exit_modelnum].first_texture = start_num+3;

		cfseek(exit_hamfile, offset2, SEEK_SET);
		polygon_model_data_read(&Polygon_models[exit_modelnum], exit_hamfile);
		polygon_model_data_read(&Polygon_models[destroyed_exit_modelnum], exit_hamfile);

		cfclose(exit_hamfile);
	} else {
		Warning("Can't load exit models!\n");
		return 0;
	}

	atexit(bm_free_extra_objbitmaps);

	return 1;
}
