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
 * Save game information
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>

#include "console.h"
#include "key.h"
#include "gr.h"
#include "palette.h"
#include "newmenu.h"

#include "inferno.h"
#ifdef EDITOR
#include "editor/editor.h"
#include "strutil.h"
#endif
#include "error.h"
#include "object.h"
#include "game.h"
#include "screens.h"
#include "wall.h"
#include "gamemine.h"
#include "robot.h"


#include "cfile.h"
#include "bm.h"
#include "menu.h"
#include "switch.h"
#include "fuelcen.h"
#include "cntrlcen.h"
#include "powerup.h"
#include "hostage.h"
#include "weapon.h"
#include "newdemo.h"
#include "gameseq.h"
#include "automap.h"
#include "polyobj.h"
#include "text.h"
#include "gamefont.h"
#include "gamesave.h"
#include "textures.h"

#ifndef NDEBUG
void dump_mine_info(void);
#endif

#ifdef EDITOR
#ifdef SHAREWARE
char *Shareware_level_names[NUM_SHAREWARE_LEVELS] = {
	"level01.sdl",
	"level02.sdl",
	"level03.sdl",
	"level04.sdl",
	"level05.sdl",
	"level06.sdl",
	"level07.sdl"
};
#else
char *Shareware_level_names[NUM_SHAREWARE_LEVELS] = {
	"level01.rdl",
	"level02.rdl",
	"level03.rdl",
	"level04.rdl",
	"level05.rdl",
	"level06.rdl",
	"level07.rdl"
};
#endif

char *Registered_level_names[NUM_REGISTERED_LEVELS] = {
	"level08.rdl",
	"level09.rdl",
	"level10.rdl",
	"level11.rdl",
	"level12.rdl",
	"level13.rdl",
	"level14.rdl",
	"level15.rdl",
	"level16.rdl",
	"level17.rdl",
	"level18.rdl",
	"level19.rdl",
	"level20.rdl",
	"level21.rdl",
	"level22.rdl",
	"level23.rdl",
	"level24.rdl",
	"level25.rdl",
	"level26.rdl",
	"level27.rdl",
	"levels1.rdl",
	"levels2.rdl",
	"levels3.rdl"
};
#endif

char Gamesave_current_filename[PATH_MAX];

int Gamesave_current_version;

#define GAME_VERSION					25
#define GAME_COMPATIBLE_VERSION	22

#define MENU_CURSOR_X_MIN			MENU_X
#define MENU_CURSOR_X_MAX			MENU_X+6

#define HOSTAGE_DATA_VERSION	0

struct {
	ushort 	fileinfo_signature;
	ushort	fileinfo_version;
	int		fileinfo_sizeof;
} __pack__ game_top_fileinfo;	 // Should be same as first two fields below...

struct {
	ushort 	fileinfo_signature;
	ushort	fileinfo_version;
	int		fileinfo_sizeof;
	char		mine_filename[15];
	int		level;
	int		player_offset;				// Player info
	int		player_sizeof;
	int		object_offset;				// Object info
	int		object_howmany;    	
	int		object_sizeof;  
	int		walls_offset;
	int		walls_howmany;
	int		walls_sizeof;
	int		doors_offset;
	int		doors_howmany;
	int		doors_sizeof;
	int		triggers_offset;
	int		triggers_howmany;
	int		triggers_sizeof;
	int		links_offset;
	int		links_howmany;
	int		links_sizeof;
	int		control_offset;
	int		control_howmany;
	int		control_sizeof;
	int		matcen_offset;
	int		matcen_howmany;
	int		matcen_sizeof;
} __pack__ game_fileinfo;

#ifdef EDITOR
extern char mine_filename[];
extern int save_mine_data_compiled(PHYSFS_file * SaveFile);
//--unused-- #else
//--unused-- char mine_filename[128];
#endif

int Gamesave_num_org_robots = 0;
//--unused-- grs_bitmap * Gamesave_saved_bitmap = NULL;

#ifdef EDITOR
//	Return true if this level has a name of the form "level??"
//	Note that a pathspec can appear at the beginning of the filename.
int is_real_level(char *filename)
{
	int	len = strlen(filename);

	if (len < 6)
		return 0;

	return !strnicmp(&filename[len-11], "level", 5);

}

void convert_name_to_CDL( char *dest, char *src )
{
	int i;

	strcpy (dest, src);

#ifdef SHAREWARE
	for (i=1; i<strlen(dest); i++ )
	{
		if (dest[i]=='.'||dest[i]==' '||dest[i]==0)
		{
			dest[i]='.';
			dest[i+1]='S';
			dest[i+2]= 'D';
			dest[i+3]= 'L';
			dest[i+4]=0;
			return;
		}
	}

	if (i < 123)
	{
		dest[i]='.';
		dest[i+1]='S';
		dest[i+2]= 'D';
		dest[i+3]= 'L';
		dest[i+4]=0;
		return;
	}
#else
	for (i=1; i<strlen(dest); i++ )
	{
		if (dest[i]=='.'||dest[i]==' '||dest[i]==0)
		{
			dest[i]='.';
			dest[i+1]='R';
			dest[i+2]= 'D';
			dest[i+3]= 'L';
			dest[i+4]=0;
			return;
		}
	}

	if (i < 123)
	{
		dest[i]='.';
		dest[i+1]='R';
		dest[i+2]= 'D';
		dest[i+3]= 'L';
		dest[i+4]=0;
		return;
	}
#endif




}
#endif

void convert_name_to_LVL( char *dest, char *src )
{
	int i;

	strcpy (dest, src);

	for (i=1; i<strlen(dest); i++ )
	{
		if (dest[i]=='.'||dest[i]==' '||dest[i]==0)
		{
			dest[i]='.';
			dest[i+1]='L';
			dest[i+2]= 'V';
			dest[i+3]= 'L';
			dest[i+4]=0;
			return;
		}
	}

	if (i < 123)
	{
		dest[i]='.';
		dest[i+1]='L';
		dest[i+2]= 'V';
		dest[i+3]= 'L';
		dest[i+4]=0;
		return;
	}
}

//--unused-- vms_angvec zero_angles={0,0,0};

#define vm_angvec_zero(v) do {(v)->p=(v)->b=(v)->h=0;} while (0)

int Gamesave_num_players=0;

int N_save_pof_names=25;
#define MAX_POLYGON_MODELS_NEW 167
char Save_pof_names[MAX_POLYGON_MODELS_NEW][13];

static int convert_vclip(int vc) {
	if (vc < 0)
		return vc;
	if ((vc < VCLIP_MAXNUM) && (Vclip[vc].num_frames >= 0))
		return vc;
	return 0;
}
static int convert_wclip(int wc) {
	return (wc < Num_wall_anims) ? wc : wc % Num_wall_anims;
}
int convert_tmap(int tmap)
{
    return (tmap >= NumTextures) ? tmap % NumTextures : tmap;
}
static int convert_polymod(int polymod) {
    return (polymod >= N_polygon_models) ? polymod % N_polygon_models : polymod;
}

void check_and_fix_matrix(vms_matrix *m);

void verify_object( object * obj )	{

	obj->lifeleft = IMMORTAL_TIME;		//all loaded object are immortal, for now

	if ( obj->type == OBJ_ROBOT )	{
		Gamesave_num_org_robots++;

		// Make sure valid id...
		if ( obj->id >= N_robot_types )
			obj->id = obj->id % N_robot_types;

		// Make sure model number & size are correct...		
		if ( obj->render_type == RT_POLYOBJ ) {
			obj->rtype.pobj_info.model_num = Robot_info[obj->id].model_num;
			obj->size = Polygon_models[obj->rtype.pobj_info.model_num].rad;

			//@@if (obj->control_type==CT_AI && Robot_info[obj->id].attack_type)
			//@@	obj->size = obj->size*3/4;
		}

		if (obj->movement_type == MT_PHYSICS) {
			obj->mtype.phys_info.mass = Robot_info[obj->id].mass;
			obj->mtype.phys_info.drag = Robot_info[obj->id].drag;
		}
	}
	else {		//Robots taken care of above

		if ( obj->render_type == RT_POLYOBJ ) {
			int i;
			char *name = Save_pof_names[obj->rtype.pobj_info.model_num];

			for (i=0;i<N_polygon_models;i++)
				if (!stricmp(Pof_names[i],name)) {		//found it!	
					obj->rtype.pobj_info.model_num = i;
					break;
				}
		}
	}

	if ( obj->type == OBJ_POWERUP ) {
		if ( obj->id >= N_powerup_types )	{
			obj->id = 0;
			Assert( obj->render_type != RT_POLYOBJ );
		}
		obj->control_type = CT_POWERUP;

		obj->size = Powerup_info[obj->id].size;
	}

	//if ( obj->type == OBJ_HOSTAGE )	{
	//	if ( obj->id >= N_hostage_types )	{
	//		obj->id = 0;
	//		Assert( obj->render_type == RT_POLYOBJ );
	//	}
	//}

	if ( obj->type == OBJ_WEAPON )	{
		if ( obj->id >= N_weapon_types )	{
			obj->id = 0;
			Assert( obj->render_type != RT_POLYOBJ );
		}
	}

	if ( obj->type == OBJ_CNTRLCEN )	{
		int i;

		obj->render_type = RT_POLYOBJ;
		obj->control_type = CT_CNTRLCEN;

		// Make model number is correct...	
		for (i=0; i<Num_total_object_types; i++ )	
			if ( ObjType[i] == OL_CONTROL_CENTER )		{
				obj->rtype.pobj_info.model_num = ObjId[i];
				obj->shields = ObjStrength[i];
				break;		
			}
	}

	if ( obj->type == OBJ_PLAYER )	{
		//int i;

		//Assert(obj == Player);

		if ( obj == ConsoleObject )		
			init_player_object();
		else
			if (obj->render_type == RT_POLYOBJ)	//recover from Matt's pof file matchup bug
				obj->rtype.pobj_info.model_num = Player_ship->model_num;

		//Make sure orient matrix is orthogonal
		check_and_fix_matrix(&obj->orient);

		obj->id = Gamesave_num_players++;
	}

	if (obj->type == OBJ_HOSTAGE) {

		if (obj->id > N_hostage_types)
			obj->id = 0;

		obj->render_type = RT_HOSTAGE;
		obj->control_type = CT_POWERUP;
		//obj->vclip_info.vclip_num = Hostage_vclip_num[Hostages[obj->id].type];
		//obj->vclip_info.frametime = Vclip[obj->vclip_info.vclip_num].frame_time;
		//obj->vclip_info.framenum = 0;
	}

}

//static gs_skip(int len,CFILE *file)
//{
//
//	cfseek(file,len,SEEK_CUR);
//}

//reads one object of the given version from the given file
void read_object(object *obj,CFILE *f,int version)
{
	obj->type				= cfile_read_byte(f);
	obj->id					= cfile_read_byte(f);

	if (obj->type == OBJ_ROBOT && obj->id > 23) {
		obj->id = obj->id % 24;
	}
	
	obj->control_type		= cfile_read_byte(f);
	obj->movement_type	= cfile_read_byte(f);
	obj->render_type		= cfile_read_byte(f);
	obj->flags				= cfile_read_byte(f);

	obj->segnum				= cfile_read_short(f);
	obj->attached_obj		= -1;

	cfile_read_vector(&obj->pos,f);
	cfile_read_matrix(&obj->orient,f);

	obj->size				= cfile_read_fix(f);
	obj->shields			= cfile_read_fix(f);

	cfile_read_vector(&obj->last_pos,f);

	obj->contains_type	= cfile_read_byte(f);
	obj->contains_id		= cfile_read_byte(f);
	obj->contains_count	= cfile_read_byte(f);

	switch (obj->movement_type) {

		case MT_PHYSICS:

			cfile_read_vector(&obj->mtype.phys_info.velocity,f);
			cfile_read_vector(&obj->mtype.phys_info.thrust,f);

			obj->mtype.phys_info.mass		= cfile_read_fix(f);
			obj->mtype.phys_info.drag		= cfile_read_fix(f);
			obj->mtype.phys_info.brakes		= cfile_read_fix(f);

			cfile_read_vector(&obj->mtype.phys_info.rotvel,f);
			cfile_read_vector(&obj->mtype.phys_info.rotthrust,f);

			obj->mtype.phys_info.turnroll	= cfile_read_fixang(f);
			obj->mtype.phys_info.flags		= cfile_read_short(f);

			break;

		case MT_SPINNING:

			cfile_read_vector(&obj->mtype.spin_rate,f);
			break;

		case MT_NONE:
			break;

		default:
			Int3();
	}

	switch (obj->control_type) {

		case CT_AI: {
			int i;

			obj->ctype.ai_info.behavior				= cfile_read_byte(f);

			for (i=0;i<MAX_AI_FLAGS;i++)
				obj->ctype.ai_info.flags[i]			= cfile_read_byte(f);

			obj->ctype.ai_info.hide_segment			= cfile_read_short(f);
			obj->ctype.ai_info.hide_index			= cfile_read_short(f);
			obj->ctype.ai_info.path_length			= cfile_read_short(f);
			obj->ctype.ai_info.cur_path_index		= cfile_read_short(f);

			if (version <= 25) {
				obj->ctype.ai_info.follow_path_start_seg = cfile_read_short(f);
				obj->ctype.ai_info.follow_path_end_seg	 = cfile_read_short(f);
			}
			
			break;
		}

		case CT_EXPLOSION:

			obj->ctype.expl_info.spawn_time		= cfile_read_fix(f);
			obj->ctype.expl_info.delete_time	= cfile_read_fix(f);
			obj->ctype.expl_info.delete_objnum	= cfile_read_short(f);
			obj->ctype.expl_info.next_attach = obj->ctype.expl_info.prev_attach = obj->ctype.expl_info.attach_parent = -1;

			break;

		case CT_WEAPON:

			//do I really need to read these?  Are they even saved to disk?

			obj->ctype.laser_info.parent_type		= cfile_read_short(f);
			obj->ctype.laser_info.parent_num		= cfile_read_short(f);
			obj->ctype.laser_info.parent_signature	= cfile_read_int(f);

			break;

		case CT_LIGHT:

			obj->ctype.light_info.intensity = cfile_read_fix(f);
			break;

		case CT_POWERUP:

			if (version >= 25)
				obj->ctype.powerup_info.count = cfile_read_int(f);
			else
				obj->ctype.powerup_info.count = 1;

			if (obj->id == POW_VULCAN_WEAPON)
					obj->ctype.powerup_info.count = VULCAN_WEAPON_AMMO_AMOUNT;

			break;


		case CT_NONE:
		case CT_FLYING:
		case CT_DEBRIS:
			break;

		case CT_SLEW:		//the player is generally saved as slew
			break;

		case CT_CNTRLCEN:
			break;

		case CT_MORPH:
		case CT_FLYTHROUGH:
		case CT_REPAIRCEN:
		default:
			Int3();
	
	}

	switch (obj->render_type) {

		case RT_NONE:
			break;

		case RT_MORPH:
		case RT_POLYOBJ: {
			int i,tmo;

			obj->rtype.pobj_info.model_num		= convert_polymod(cfile_read_int(f));

			for (i=0;i<MAX_SUBMODELS;i++)
				cfile_read_angvec(&obj->rtype.pobj_info.anim_angles[i],f);

			obj->rtype.pobj_info.subobj_flags	= cfile_read_int(f);

			tmo = cfile_read_int(f);

			#ifndef EDITOR
			obj->rtype.pobj_info.tmap_override	= convert_tmap(tmo);
			#else
			if (tmo==-1)
				obj->rtype.pobj_info.tmap_override	= -1;
			else {
				int xlated_tmo = tmap_xlate_table[tmo];
				if (xlated_tmo < 0)	{
					Int3();
					xlated_tmo = 0;
				}
				obj->rtype.pobj_info.tmap_override	= xlated_tmo;
			}
			#endif

			obj->rtype.pobj_info.alt_textures	= 0;

			break;
		}

		case RT_WEAPON_VCLIP:
		case RT_HOSTAGE:
		case RT_POWERUP:
		case RT_FIREBALL:

			obj->rtype.vclip_info.vclip_num	= convert_vclip(cfile_read_int(f));
			obj->rtype.vclip_info.frametime	= cfile_read_fix(f);
			obj->rtype.vclip_info.framenum	= cfile_read_byte(f);

			break;

		case RT_LASER:
			break;

		default:
			Int3();

	}

}

#ifdef EDITOR

//writes one object to the given file
void write_object(object *obj, PHYSFS_file *f)
{
	PHYSFSX_writeU8(f, obj->type);
	PHYSFSX_writeU8(f, obj->id);

	PHYSFSX_writeU8(f, obj->control_type);
	PHYSFSX_writeU8(f, obj->movement_type);
	PHYSFSX_writeU8(f, obj->render_type);
	PHYSFSX_writeU8(f, obj->flags);

	PHYSFS_writeSLE16(f, obj->segnum);

	PHYSFSX_writeVector(f, &obj->pos);
	PHYSFSX_writeMatrix(f, &obj->orient);

	PHYSFSX_writeFix(f, obj->size);
	PHYSFSX_writeFix(f, obj->shields);

	PHYSFSX_writeVector(f, &obj->last_pos);

	PHYSFSX_writeU8(f, obj->contains_type);
	PHYSFSX_writeU8(f, obj->contains_id);
	PHYSFSX_writeU8(f, obj->contains_count);

	switch (obj->movement_type) {

		case MT_PHYSICS:

	 		PHYSFSX_writeVector(f, &obj->mtype.phys_info.velocity);
			PHYSFSX_writeVector(f, &obj->mtype.phys_info.thrust);

			PHYSFSX_writeFix(f, obj->mtype.phys_info.mass);
			PHYSFSX_writeFix(f, obj->mtype.phys_info.drag);
			PHYSFSX_writeFix(f, obj->mtype.phys_info.brakes);

			PHYSFSX_writeVector(f, &obj->mtype.phys_info.rotvel);
			PHYSFSX_writeVector(f, &obj->mtype.phys_info.rotthrust);

			PHYSFSX_writeFixAng(f, obj->mtype.phys_info.turnroll);
			PHYSFS_writeSLE16(f, obj->mtype.phys_info.flags);

			break;

		case MT_SPINNING:

			PHYSFSX_writeVector(f, &obj->mtype.spin_rate);
			break;

		case MT_NONE:
			break;

		default:
			Int3();
	}

	switch (obj->control_type) {

		case CT_AI: {
			int i;

			PHYSFSX_writeU8(f, obj->ctype.ai_info.behavior);

			for (i = 0; i < MAX_AI_FLAGS; i++)
				PHYSFSX_writeU8(f, obj->ctype.ai_info.flags[i]);

			PHYSFS_writeSLE16(f, obj->ctype.ai_info.hide_segment);
			PHYSFS_writeSLE16(f, obj->ctype.ai_info.hide_index);
			PHYSFS_writeSLE16(f, obj->ctype.ai_info.path_length);
			PHYSFS_writeSLE16(f, obj->ctype.ai_info.cur_path_index);
			PHYSFS_writeSLE16(f, obj->ctype.ai_info.follow_path_start_seg);
			PHYSFS_writeSLE16(f, obj->ctype.ai_info.follow_path_end_seg);

			break;
		}

		case CT_EXPLOSION:

			PHYSFSX_writeFix(f, obj->ctype.expl_info.spawn_time);
			PHYSFSX_writeFix(f, obj->ctype.expl_info.delete_time);
			PHYSFS_writeSLE16(f, obj->ctype.expl_info.delete_objnum);

			break;

		case CT_WEAPON:

			//do I really need to write these objects?

			PHYSFS_writeSLE16(f, obj->ctype.laser_info.parent_type);
			PHYSFS_writeSLE16(f, obj->ctype.laser_info.parent_num);
			PHYSFS_writeSLE32(f, obj->ctype.laser_info.parent_signature);

			break;

		case CT_LIGHT:

			PHYSFSX_writeFix(f, obj->ctype.light_info.intensity);
			break;

		case CT_POWERUP:

			PHYSFS_writeSLE32(f, obj->ctype.powerup_info.count);
			break;

		case CT_NONE:
		case CT_FLYING:
		case CT_DEBRIS:
			break;

		case CT_SLEW:		//the player is generally saved as slew
			break;

		case CT_CNTRLCEN:
			break;			//control center object.

		case CT_MORPH:
		case CT_REPAIRCEN:
		case CT_FLYTHROUGH:
		default:
			Int3();
	
	}

	switch (obj->render_type) {

		case RT_NONE:
			break;

		case RT_MORPH:
		case RT_POLYOBJ: {
			int i;

			PHYSFS_writeSLE32(f, obj->rtype.pobj_info.model_num);

			for (i = 0; i < MAX_SUBMODELS; i++)
				PHYSFSX_writeAngleVec(f, &obj->rtype.pobj_info.anim_angles[i]);

			PHYSFS_writeSLE32(f, obj->rtype.pobj_info.subobj_flags);

			PHYSFS_writeSLE32(f, obj->rtype.pobj_info.tmap_override);

			break;
		}

		case RT_WEAPON_VCLIP:
		case RT_HOSTAGE:
		case RT_POWERUP:
		case RT_FIREBALL:

			PHYSFS_writeSLE32(f, obj->rtype.vclip_info.vclip_num);
			PHYSFSX_writeFix(f, obj->rtype.vclip_info.frametime);
			PHYSFSX_writeU8(f, obj->rtype.vclip_info.framenum);

			break;

		case RT_LASER:
			break;

		default:
			Int3();

	}

}
#endif

// -----------------------------------------------------------------------------
// Load game 
// Loads all the relevant data for a level.
// If level != -1, it loads the filename with extension changed to .min
// Otherwise it loads the appropriate level mine.
// returns 0=everything ok, 1=old version, -1=error
int load_game_data(CFILE *LoadFile)
{
	int i,j;
	int start_offset;

	start_offset = cftell(LoadFile);

	//===================== READ FILE INFO ========================

	// Set default values
	game_fileinfo.level					=	-1;
	game_fileinfo.player_offset		=	-1;
	game_fileinfo.player_sizeof		=	sizeof(player);
 	game_fileinfo.object_offset		=	-1;
	game_fileinfo.object_howmany		=	0;
	game_fileinfo.object_sizeof		=	sizeof(object);  
	game_fileinfo.walls_offset			=	-1;
	game_fileinfo.walls_howmany		=	0;
	game_fileinfo.walls_sizeof			=	sizeof(wall);  
	game_fileinfo.doors_offset			=	-1;
	game_fileinfo.doors_howmany		=	0;
	game_fileinfo.doors_sizeof			=	sizeof(active_door);  
	game_fileinfo.triggers_offset		=	-1;
	game_fileinfo.triggers_howmany	=	0;
	game_fileinfo.triggers_sizeof		=	sizeof(trigger);  
	game_fileinfo.control_offset		=	-1;
	game_fileinfo.control_howmany		=	0;
	game_fileinfo.control_sizeof		=	sizeof(control_center_triggers);
	game_fileinfo.matcen_offset		=	-1;
	game_fileinfo.matcen_howmany		=	0;
	game_fileinfo.matcen_sizeof		=	sizeof(matcen_info);

	if (cfseek(LoadFile, start_offset, SEEK_SET))
		Error("Error seeking in gamesave.c");
		
	// Read in game_top_fileinfo to get size of saved fileinfo.
	game_top_fileinfo.fileinfo_signature = cfile_read_short(LoadFile);
	game_top_fileinfo.fileinfo_version = cfile_read_short(LoadFile);
	game_top_fileinfo.fileinfo_sizeof = cfile_read_int(LoadFile);

	// Check signature
	if (game_top_fileinfo.fileinfo_signature != 0x6705)
		return -1;

	// Check version number
	if (game_top_fileinfo.fileinfo_version < GAME_COMPATIBLE_VERSION )
		return -1;

	// Now, Read in the fileinfo
	if (cfseek( LoadFile, start_offset, SEEK_SET )) 
		Error( "Error seeking to game_fileinfo in gamesave.c" );

	game_fileinfo.fileinfo_signature = cfile_read_short(LoadFile);

	game_fileinfo.fileinfo_version = cfile_read_short(LoadFile);
	game_fileinfo.fileinfo_sizeof = cfile_read_int(LoadFile);
	for(i=0; i<15; i++)
		game_fileinfo.mine_filename[i] = cfile_read_byte(LoadFile);
	game_fileinfo.level = cfile_read_int(LoadFile);
	game_fileinfo.player_offset = cfile_read_int(LoadFile);				// Player info
	game_fileinfo.player_sizeof = cfile_read_int(LoadFile);
	game_fileinfo.object_offset = cfile_read_int(LoadFile);				// Object info
	game_fileinfo.object_howmany = cfile_read_int(LoadFile);    	
	game_fileinfo.object_sizeof = cfile_read_int(LoadFile);  
	game_fileinfo.walls_offset = cfile_read_int(LoadFile);
	game_fileinfo.walls_howmany = cfile_read_int(LoadFile);
	game_fileinfo.walls_sizeof = cfile_read_int(LoadFile);
	game_fileinfo.doors_offset = cfile_read_int(LoadFile);
	game_fileinfo.doors_howmany = cfile_read_int(LoadFile);
	game_fileinfo.doors_sizeof = cfile_read_int(LoadFile);
	game_fileinfo.triggers_offset = cfile_read_int(LoadFile);
	game_fileinfo.triggers_howmany = cfile_read_int(LoadFile);
	game_fileinfo.triggers_sizeof = cfile_read_int(LoadFile);
	game_fileinfo.links_offset = cfile_read_int(LoadFile);
	game_fileinfo.links_howmany = cfile_read_int(LoadFile);
	game_fileinfo.links_sizeof = cfile_read_int(LoadFile);
	game_fileinfo.control_offset = cfile_read_int(LoadFile);
	game_fileinfo.control_howmany = cfile_read_int(LoadFile);
	game_fileinfo.control_sizeof = cfile_read_int(LoadFile);
	game_fileinfo.matcen_offset = cfile_read_int(LoadFile);
	game_fileinfo.matcen_howmany = cfile_read_int(LoadFile);
	game_fileinfo.matcen_sizeof = cfile_read_int(LoadFile);

	if (game_top_fileinfo.fileinfo_version >= 31) //load mine filename
		// read newline-terminated string, not sure what version this changed.
		cfgets(Current_level_name,sizeof(Current_level_name),LoadFile);
	else if (game_top_fileinfo.fileinfo_version >= 14) {	//load mine filename
		char *p=Current_level_name;
		//must do read one char at a time, since no cfgets()
		do *p = cfgetc(LoadFile); while (*p++!=0);
	}
	else
		Current_level_name[0]=0;

	if (game_top_fileinfo.fileinfo_version >= 19) {	//load pof names
		N_save_pof_names = cfile_read_short(LoadFile);
		if (N_save_pof_names != 0x614d && N_save_pof_names != 0x5547) { // "Ma"de w/DMB beta/"GU"ILE
			Assert(N_save_pof_names < MAX_POLYGON_MODELS);
			cfread(Save_pof_names,N_save_pof_names,FILENAME_LEN,LoadFile);
		}
	}

	//===================== READ PLAYER INFO ==========================
	Object_next_signature = 0;

	//===================== READ OBJECT INFO ==========================

	Gamesave_num_org_robots = 0;
	Gamesave_num_players = 0;

	if (game_fileinfo.object_offset > -1) {
		if (cfseek( LoadFile, game_fileinfo.object_offset, SEEK_SET )) 
			Error( "Error seeking to object_offset in gamesave.c" );
	
		for (i=0;i<game_fileinfo.object_howmany;i++)	{
			memset(&(Objects[i]), 0, sizeof(object));
			read_object(&Objects[i],LoadFile,game_top_fileinfo.fileinfo_version);

			Objects[i].signature = Object_next_signature++;
			verify_object( &Objects[i] );
		}

	}

	//===================== READ WALL INFO ============================

	if (game_fileinfo.walls_offset > -1)
	{

		if (!cfseek( LoadFile, game_fileinfo.walls_offset,SEEK_SET ))	{
			for (i=0;i<game_fileinfo.walls_howmany;i++) {

				if (game_top_fileinfo.fileinfo_version >= 20) {

					Assert(sizeof(Walls[i]) == game_fileinfo.walls_sizeof);

					wall_read(&Walls[i], LoadFile);
					Walls[i].clip_num = convert_wclip(Walls[i].clip_num);
				}
				else if (game_top_fileinfo.fileinfo_version >= 17) {
					v19_wall w;

					Assert(sizeof(w) == game_fileinfo.walls_sizeof);

					v19_wall_read(&w, LoadFile);

					Walls[i].segnum		= w.segnum;
					Walls[i].sidenum		= w.sidenum;
					Walls[i].linked_wall	= w.linked_wall;

					Walls[i].type			= w.type;
					Walls[i].flags			= w.flags;
					Walls[i].hps			= w.hps;
					Walls[i].trigger		= w.trigger;
					Walls[i].clip_num		= w.clip_num;
					Walls[i].keys			= w.keys;

					Walls[i].state			= WALL_DOOR_CLOSED;
				}
				else {
					v16_wall w;

					Assert(sizeof(w) == game_fileinfo.walls_sizeof);

					v16_wall_read(&w, LoadFile);

					Walls[i].segnum = Walls[i].sidenum = Walls[i].linked_wall = -1;

					Walls[i].type		= w.type;
					Walls[i].flags		= w.flags;
					Walls[i].hps		= w.hps;
					Walls[i].trigger	= w.trigger;
					Walls[i].clip_num	= w.clip_num;
					Walls[i].keys		= w.keys;
				}

			}
		}
	}

	//===================== READ DOOR INFO ============================

	if (game_fileinfo.doors_offset > -1)
	{
		if (!cfseek( LoadFile, game_fileinfo.doors_offset,SEEK_SET ))	{

			for (i=0;i<game_fileinfo.doors_howmany;i++) {

				if (game_top_fileinfo.fileinfo_version >= 20) {

					Assert(sizeof(ActiveDoors[i]) == game_fileinfo.doors_sizeof);

					active_door_read(&ActiveDoors[i], LoadFile);
				}
				else {
					v19_door d;
					int p;

					Assert(sizeof(d) == game_fileinfo.doors_sizeof);

					v19_door_read(&d, LoadFile);

					ActiveDoors[i].n_parts = d.n_parts;

					for (p=0;p<d.n_parts;p++) {
						int cseg,cside;

						cseg = Segments[d.seg[p]].children[d.side[p]];
						cside = find_connect_side(&Segments[d.seg[p]],&Segments[cseg]);

						ActiveDoors[i].front_wallnum[p] = Segments[d.seg[p]].sides[d.side[p]].wall_num;
						ActiveDoors[i].back_wallnum[p] = Segments[cseg].sides[cside].wall_num;
					}
				}

			}
		}
	}

	//==================== READ TRIGGER INFO ==========================

	if (game_fileinfo.triggers_offset > -1)		{
		if (!cfseek( LoadFile, game_fileinfo.triggers_offset,SEEK_SET ))	{
			for (i=0;i<game_fileinfo.triggers_howmany;i++)	{
				Assert( sizeof(Triggers[i]) == game_fileinfo.triggers_sizeof );
				if (game_top_fileinfo.fileinfo_version <= 25) {
					trigger_read(&Triggers[i], LoadFile);
				} else {
					int type;
					switch (type = cfile_read_short(LoadFile)) {
						case 0: // door
							Triggers[i].type = 0;
							Triggers[i].flags = TRIGGER_CONTROL_DOORS;
							break;
						case 2: // matcen
							Triggers[i].type = 0;
							Triggers[i].flags = TRIGGER_MATCEN;
							break;
						case 3: // exit
							Triggers[i].type = 0;
							Triggers[i].flags = TRIGGER_EXIT;
							break;
						case 9: // open wall
							Triggers[i].type = 0;
							Triggers[i].flags = TRIGGER_ILLUSION_OFF;
							break;
						default:
							con_printf(CON_URGENT,"Warning: unknown trigger type %d (%d)\n", type, i);
					}
					Triggers[i].num_links = cfile_read_short(LoadFile);
					Triggers[i].value = cfile_read_int(LoadFile);
					Triggers[i].time = cfile_read_int(LoadFile);
					for (j=0; j<MAX_WALLS_PER_LINK; j++ )	
						Triggers[i].seg[j] = cfile_read_short(LoadFile);
					for (j=0; j<MAX_WALLS_PER_LINK; j++ )
						Triggers[i].side[j] = cfile_read_short(LoadFile);
				}
			}
		}
	}

	//================ READ CONTROL CENTER TRIGGER INFO ===============
	control_center_triggers_read_n(&ControlCenterTriggers, 1, LoadFile);


	//================ READ MATERIALOGRIFIZATIONATORS INFO ===============

	if (game_fileinfo.matcen_offset > -1)
	{	int	j;

		if (!cfseek( LoadFile, game_fileinfo.matcen_offset,SEEK_SET ))	{
			for (i=0;i<game_fileinfo.matcen_howmany;i++) {
				//Assert( sizeof(RobotCenters[i]) == game_fileinfo.matcen_sizeof );
				matcen_info_read(&RobotCenters[i], LoadFile, game_top_fileinfo.fileinfo_version);
				
				//	Set links in RobotCenters to Station array
				for (j=0; j<=Highest_segment_index; j++)
					if (Segments[j].special == SEGMENT_IS_ROBOTMAKER)
						if (Segments[j].matcen_num == i)
							RobotCenters[i].fuelcen_num = Segments[j].value;
			}
		}
	}


	//========================= UPDATE VARIABLES ======================

	reset_objects(game_fileinfo.object_howmany);

	for (i=0; i<MAX_OBJECTS; i++) {
		Objects[i].next = Objects[i].prev = -1;
		if (Objects[i].type != OBJ_NONE) {
			int objsegnum = Objects[i].segnum;

			if (objsegnum > Highest_segment_index)		//bogus object
				Objects[i].type = OBJ_NONE;
			else {
				Objects[i].segnum = -1;			//avoid Assert()
				obj_link(i,objsegnum);
			}
		}
	}

	clear_transient_objects(1);		//1 means clear proximity bombs

	// Make sure non-transparent doors are set correctly.
	for (i=0; i< Num_segments; i++)
		for (j=0;j<MAX_SIDES_PER_SEGMENT;j++) {
			side	*sidep = &Segments[i].sides[j];
			if ((sidep->wall_num != -1) && (Walls[sidep->wall_num].clip_num != -1)) {
				if (WallAnims[Walls[sidep->wall_num].clip_num].flags & WCF_TMAP1) {
					sidep->tmap_num = WallAnims[Walls[sidep->wall_num].clip_num].frames[0];
					sidep->tmap_num2 = 0;
				}
			}
		}


	Num_walls = game_fileinfo.walls_howmany;
	reset_walls();

	Num_open_doors = game_fileinfo.doors_howmany;
	Num_triggers = game_fileinfo.triggers_howmany;

	Num_robot_centers = game_fileinfo.matcen_howmany;

	//fix old wall structs
	if (game_top_fileinfo.fileinfo_version < 17) {
		int segnum,sidenum,wallnum;

		for (segnum=0; segnum<=Highest_segment_index; segnum++)
			for (sidenum=0;sidenum<6;sidenum++)
				if ((wallnum=Segments[segnum].sides[sidenum].wall_num) != -1) {
					Walls[wallnum].segnum = segnum;
					Walls[wallnum].sidenum = sidenum;
				}
	}

	#ifndef NDEBUG
	{
		int	sidenum;
		for (sidenum=0; sidenum<6; sidenum++) {
			int	wallnum = Segments[Highest_segment_index].sides[sidenum].wall_num;
			if (wallnum != -1)
				if ((Walls[wallnum].segnum != Highest_segment_index) || (Walls[wallnum].sidenum != sidenum))
					Int3();	//	Error.  Bogus walls in this segment.
								// Consult Yuan or Mike.
		}
	}
	#endif

	//create_local_segment_data();

	fix_object_segs();

	#ifndef NDEBUG
	dump_mine_info();
	#endif

	if (game_top_fileinfo.fileinfo_version < GAME_VERSION)
		return 1;		//means old version
	else
		return 0;
}


int check_segment_connections(void);

// -----------------------------------------------------------------------------
//loads from an already-open file
// returns 0=everything ok, 1=old version, -1=error
int load_mine_data(CFILE *LoadFile);
int load_mine_data_compiled(CFILE *LoadFile);

#define LEVEL_FILE_VERSION		1

#ifndef RELEASE
char *Level_being_loaded=NULL;
#endif

#ifdef COMPACT_SEGS
extern void ncache_flush();
#endif

//loads a level (.LVL) file from disk
int load_level(char * filename_passed)
{
	#ifdef EDITOR
	int use_compiled_level=1;
	#endif
	CFILE * LoadFile;
	char filename[PATH_MAX];
	int sig, minedata_offset, gamedata_offset, hostagetext_offset;
	int mine_err, game_err;

	#ifdef COMPACT_SEGS
	ncache_flush();
	#endif

	#ifndef RELEASE
	Level_being_loaded = filename_passed;
	#endif

	strcpy(filename,filename_passed);

#ifdef EDITOR
	//if we have the editor, try the LVL first, no matter what was passed.
	//if we don't have an LVL, try what was passed or SDL/RDL  
	//if we don't have the editor, we just use what was passed
	
	change_filename_extension(filename,filename_passed,".lvl");
	use_compiled_level = 0;
	
	if (!cfexist(filename))
	{
		char *p = strrchr(filename_passed, '.');
		
		if (stricmp(p, ".lvl"))
			strcpy(filename, filename_passed);	// set to what was passed
		else
			convert_name_to_CDL(filename, filename);
		use_compiled_level = 1;
	}		
#endif

	if (!cfexist(filename))
		sprintf(filename,"%s%s",MISSION_DIR,filename_passed);
	
	LoadFile = cfopen( filename, "rb" );

	if (!LoadFile)	{
#ifdef EDITOR
		return 1;
#else
		Error("Can't open file <%s>\n",filename);
#endif
	}
	
	strcpy( Gamesave_current_filename, filename );

	sig					= cfile_read_int(LoadFile);
	Gamesave_current_version = cfile_read_int(LoadFile);
	minedata_offset		= cfile_read_int(LoadFile);
	gamedata_offset		= cfile_read_int(LoadFile);

	Assert(sig == 0x504c564c); /* 'PLVL' */

	if (Gamesave_current_version < 5)
		hostagetext_offset = cfile_read_int(LoadFile);
	
	cfseek(LoadFile,minedata_offset,SEEK_SET);
	#ifdef EDITOR
	if (!use_compiled_level)
		mine_err = load_mine_data(LoadFile);
	else
	#endif
		//NOTE LINK TO ABOVE!!
		mine_err = load_mine_data_compiled(LoadFile);

	if (mine_err == -1)	//error!!
		return 1;

	cfseek(LoadFile,gamedata_offset,SEEK_SET);
	game_err = load_game_data(LoadFile);

	if (game_err == -1)	//error!!
		return 1;

	#ifdef HOSTAGE_FACES
	cfseek(LoadFile,hostagetext_offset,SEEK_SET);
	load_hostage_data(LoadFile,(version>=1));
	#endif

	//======================== CLOSE FILE =============================

	cfclose( LoadFile );

	#ifdef EDITOR
	#ifndef RELEASE
	write_game_text_file(filename);
	if (Errors_in_mine) {
		if (is_real_level(filename)) {
			char  ErrorMessage[200];

			sprintf( ErrorMessage, "Warning: %i errors in %s!\n", Errors_in_mine, Level_being_loaded );
			stop_time();
			gr_palette_load(gr_palette);
			nm_messagebox( NULL, 1, "Continue", ErrorMessage );
			start_time();
		}
	}
	#endif
	#endif

	#ifdef EDITOR
	//If an old version, ask the use if he wants to save as new version
	if (((LEVEL_FILE_VERSION>1) && version<LEVEL_FILE_VERSION) || mine_err==1 || game_err==1) {
		char  ErrorMessage[200];

		sprintf( ErrorMessage, "You just loaded a old version level.  Would\n"
						"you like to save it as a current version level?");

		stop_time();
		gr_palette_load(gr_palette);
		if (nm_messagebox( NULL, 2, "Don't Save", "Save", ErrorMessage )==1)
			save_level(filename);
		start_time();
	}
	#endif

	#ifdef EDITOR
	if (Function_mode == FMODE_EDITOR)
		editor_status("Loaded NEW mine %s, \"%s\"",filename,Current_level_name);
	#endif

	#ifdef EDITOR
	if (check_segment_connections())
		nm_messagebox( "ERROR", 1, "Ok", 
				"Connectivity errors detected in\n"
				"mine.  See monochrome screen for\n"
				"details, and contact Matt or Mike." );
	#endif

	return 0;
}

#ifdef EDITOR
int get_level_name()
{
	newmenu_item m[2];

	m[0].type = NM_TYPE_TEXT; m[0].text = "Please enter a name for this mine:";
	m[1].type = NM_TYPE_INPUT; m[1].text = Current_level_name; m[1].text_len = LEVEL_NAME_LEN;

	newmenu_do( NULL, "Enter mine name", 2, m, NULL );
	return 0;
}
#endif


#ifdef EDITOR

int	Errors_in_mine;

// -----------------------------------------------------------------------------
// Save game
int save_game_data(PHYSFS_file * SaveFile)
{
	int  player_offset=0, object_offset=0, walls_offset=0, doors_offset=0, triggers_offset=0, control_offset=0, matcen_offset=0; //, links_offset;
	int start_offset=0,end_offset=0;

	//===================== SAVE FILE INFO ========================

	PHYSFS_writeSLE16(SaveFile, 0x6705);    // signature
	PHYSFS_writeSLE16(SaveFile, GAME_VERSION);
	PHYSFS_writeSLE32(SaveFile, sizeof(game_fileinfo));
	PHYSFS_write(SaveFile, Current_level_name, 15, 1);
	PHYSFS_writeSLE32(SaveFile, Current_level_num);
	offset_offset = PHYSFS_tell(SaveFile);	// write the offsets later
	PHYSFS_writeSLE32(SaveFile, -1);
	PHYSFS_writeSLE32(SaveFile, sizeof(player));

#define WRITE_HEADER_ENTRY(t, n) do { PHYSFS_writeSLE32(SaveFile, -1); PHYSFS_writeSLE32(SaveFile, n); PHYSFS_writeSLE32(SaveFile, sizeof(t)); } while(0)

	WRITE_HEADER_ENTRY(object, Highest_object_index + 1);
	WRITE_HEADER_ENTRY(wall, Num_walls);
	WRITE_HEADER_ENTRY(active_door, Num_open_doors);
	WRITE_HEADER_ENTRY(trigger, Num_triggers);
	WRITE_HEADER_ENTRY(0, 0);		// links (removed by Parallax)
	WRITE_HEADER_ENTRY(control_center_triggers, 1);
	WRITE_HEADER_ENTRY(matcen_info, Num_robot_centers);

	// Write the mine name
	PHYSFSX_printf(SaveFile, "%s\n", Current_level_name);

	PHYSFS_writeSLE16(SaveFile, N_polygon_models);
	PHYSFS_write(SaveFile, Pof_names, sizeof(*Pof_names), N_polygon_models);

	//==================== SAVE PLAYER INFO ===========================

	player_offset = PHYSFS_tell(SaveFile);
	PHYSFS_write(SaveFile, &Players[Player_num], sizeof(player), 1);        // not endian friendly, but not used either

	//==================== SAVE OBJECT INFO ===========================

	object_offset = ftell(SaveFile);
	//fwrite( &Objects, sizeof(object), game_fileinfo.object_howmany, SaveFile );
	{
		int i;
		for (i=0;i<Highest_object_index;i++)
			write_object(&Objects[i],SaveFile);
	}

	//==================== SAVE WALL INFO =============================

	walls_offset = PHYSFS_tell(SaveFile);
	for (i = 0; i < Num_walls; i++)
		wall_write(&Walls[i], SaveFile);

	//==================== SAVE DOOR INFO =============================

#if 0 // FIXME: okay to leave this out?
	doors_offset = PHYSFS_tell(SaveFile);
	for (i = 0; i < Num_open_doors; i++)
		door_write(&ActiveDoors[i], game_top_fileinfo_version, SaveFile);
#endif

	//==================== SAVE TRIGGER INFO =============================

	triggers_offset = PHYSFS_tell(SaveFile);
	for (i = 0; i < Num_triggers; i++)
		trigger_write(&Triggers[i], SaveFile);

	//================ SAVE CONTROL CENTER TRIGGER INFO ===============

	control_offset = PHYSFS_tell(SaveFile);
	control_center_triggers_write(&ControlCenterTriggers, SaveFile);


	//================ SAVE MATERIALIZATION CENTER TRIGGER INFO ===============

	matcen_offset = PHYSFS_tell(SaveFile);
	for (i = 0; i < Num_robot_centers; i++)
		matcen_info_write(&RobotCenters[i], game_top_fileinfo_version, SaveFile);

	//============= REWRITE FILE INFO, TO SAVE OFFSETS ===============

	end_offset = PHYSFS_tell(SaveFile);

	// Update the offset fields

#define WRITE_OFFSET(o, n) do { PHYSFS_seek(SaveFile, offset_offset); PHYSFS_writeSLE32(SaveFile, o ## _offset); offset_offset += sizeof(int)*n; } while (0)

	WRITE_OFFSET(player, 2);
	WRITE_OFFSET(object, 3);
	WRITE_OFFSET(walls, 3);
	WRITE_OFFSET(doors, 3);
	WRITE_OFFSET(triggers, 6);
	WRITE_OFFSET(control, 3);
	WRITE_OFFSET(matcen, 3);

	// Go back to end of data
	PHYSFS_seek(SaveFile, end_offset);

	return 0;
}

int save_mine_data(PHYSFS_file * SaveFile);

// -----------------------------------------------------------------------------
// Save game
int save_level_sub(char * filename, int compiled_version)
{
	PHYSFS_file * SaveFile;
	char temp_filename[128];
	int sig = 0x504c564c /*'PLVL'*/,version=LEVEL_FILE_VERSION;
        int minedata_offset=0,gamedata_offset=0,hostagetext_offset=0;

	if ( !compiled_version )	{
		write_game_text_file(filename);

		if (Errors_in_mine) {
			if (is_real_level(filename)) {
				char  ErrorMessage[200];
	
				sprintf( ErrorMessage, "Warning: %i errors in this mine!\n", Errors_in_mine );
				stop_time();
				gr_palette_load(gr_palette);
	 
				if (nm_messagebox( NULL, 2, "Cancel Save", "Save", ErrorMessage )!=1)	{
					start_time();
					return 1;
				}
				start_time();
			}
		}
		convert_name_to_LVL(temp_filename,filename);
	} else {
		convert_name_to_CDL(temp_filename,filename);
	}

	SaveFile = PHYSFSX_openWriteBuffered(temp_filename);
	if (!SaveFile)
	{
		char ErrorMessage[256];

		char fname[20];
		removeext( temp_filename, fname );

		sprintf( ErrorMessage, \
			"ERROR: Cannot write to '%s'.\nYou probably need to check out a locked\nversion of the file. You should save\nthis under a different filename, and then\ncheck out a locked copy by typing\n\'co -l %s.lvl'\nat the DOS prompt.\n" 
			, temp_filename, fname );
		stop_time();
		gr_palette_load(gr_palette);
		nm_messagebox( NULL, 1, "Ok", ErrorMessage );
		start_time();
		return 1;
	}

	if (Current_level_name[0] == 0)
		strcpy(Current_level_name,"Untitled");

	clear_transient_objects(1);		//1 means clear proximity bombs

	compress_objects();		//after this, Highest_object_index == num objects

	//make sure player is in a segment
	if (update_object_seg(&Objects[Players[0].objnum]) == 0) {
		if (ConsoleObject->segnum > Highest_segment_index)
			ConsoleObject->segnum = 0;
		compute_segment_center(&ConsoleObject->pos,&(Segments[ConsoleObject->segnum]));
	}
 
	fix_object_segs();

	//Write the header

	PHYSFS_writeSLE32(SaveFile, sig);
	PHYSFS_writeSLE32(SaveFile, version);

	//save placeholders
	PHYSFS_writeSLE32(SaveFile, minedata_offset);
	PHYSFS_writeSLE32(SaveFile, gamedata_offset);
	PHYSFS_writeSLE32(SaveFile, hostagetext_offset);

	//Now write the damn data

	minedata_offset = PHYSFS_tell(SaveFile);
	if ( !compiled_version )	
		save_mine_data(SaveFile);
	else
		save_mine_data_compiled(SaveFile);
	gamedata_offset = PHYSFS_tell(SaveFile);
	save_game_data(SaveFile);
	hostagetext_offset = PHYSFS_tell(SaveFile);

	#ifdef HOSTAGE_FACES
	save_hostage_data(SaveFile);
	#endif

	PHYSFS_seek(SaveFile, sizeof(sig) + sizeof(version));
	PHYSFS_writeSLE32(SaveFile, minedata_offset);
	PHYSFS_writeSLE32(SaveFile, gamedata_offset);
	PHYSFS_writeSLE32(SaveFile, hostagetext_offset);

	//==================== CLOSE THE FILE =============================
	PHYSFS_close(SaveFile);

	if ( !compiled_version )	{
		if (Function_mode == FMODE_EDITOR)
			editor_status("Saved mine %s, \"%s\"",filename,Current_level_name);
	}

	return 0;

}

int save_level(char * filename)
{
	int r1;

	// Save normal version...
	r1 = save_level_sub(filename, 0);

	// Save compiled version...
	save_level_sub(filename, 1);

	return r1;
}


#ifdef HOSTAGE_FACES
void save_hostage_data(PHYSFS_file * fp)
{
	int i,num_hostages=0;

	// Find number of hostages in mine...
	for (i=0; i<=Highest_object_index; i++ )	{
		int num;
		if ( Objects[i].type == OBJ_HOSTAGE )	{
			num = Objects[i].id;
			#ifndef SHAREWARE
			if (num<0 || num>=MAX_HOSTAGES || Hostage_face_clip[Hostages[num].vclip_num].num_frames<=0)
				num=0;
			#else
			num = 0;
			#endif
			if (num+1 > num_hostages)
				num_hostages = num+1;
		}
	}

	cfile_write_int(HOSTAGE_DATA_VERSION,fp);

	for (i=0; i<num_hostages; i++ )	{
		cfile_write_int(Hostages[i].vclip_num,fp);
		fputs(Hostages[i].text,fp);
		fputc('\n',fp);		//fgets wants a newline
	}
}
#endif	//HOSTAGE_FACES

#endif	//EDITOR

#ifndef NDEBUG
void dump_mine_info(void)
{
	int	segnum, sidenum;
	fix	min_u, max_u, min_v, max_v, min_l, max_l, max_sl;

	min_u = F1_0*1000;
	min_v = min_u;
	min_l = min_u;

	max_u = -min_u;
	max_v = max_u;
	max_l = max_u;

	max_sl = 0;

	for (segnum=0; segnum<=Highest_segment_index; segnum++) {
		for (sidenum=0; sidenum<MAX_SIDES_PER_SEGMENT; sidenum++) {
			int	vertnum;
			side	*sidep = &Segments[segnum].sides[sidenum];

			if (Segments[segnum].static_light > max_sl)
				max_sl = Segments[segnum].static_light;

			for (vertnum=0; vertnum<4; vertnum++) {
				if (sidep->uvls[vertnum].u < min_u)
					min_u = sidep->uvls[vertnum].u;
				else if (sidep->uvls[vertnum].u > max_u)
					max_u = sidep->uvls[vertnum].u;

				if (sidep->uvls[vertnum].v < min_v)
					min_v = sidep->uvls[vertnum].v;
				else if (sidep->uvls[vertnum].v > max_v)
					max_v = sidep->uvls[vertnum].v;

				if (sidep->uvls[vertnum].l < min_l)
					min_l = sidep->uvls[vertnum].l;
				else if (sidep->uvls[vertnum].l > max_l)
					max_l = sidep->uvls[vertnum].l;
			}

		}
	}
}

#endif

#ifdef HOSTAGE_FACES
void load_hostage_data(CFILE * fp,int do_read)
{
	int version,i,num,num_hostages;

	hostage_init_all();

	num_hostages = 0;

	// Find number of hostages in mine...
	for (i=0; i<=Highest_object_index; i++ )	{
		if ( Objects[i].type == OBJ_HOSTAGE )	{
			num = Objects[i].id;
			if (num+1 > num_hostages)
				num_hostages = num+1;

			if (Hostages[num].objnum != -1) {		//slot already used
				num = hostage_get_next_slot();		//..so get new slot
				if (num+1 > num_hostages)
					num_hostages = num+1;
				Objects[i].id = num;
			}

			if ( num > -1 && num < MAX_HOSTAGES )	{
				Assert(Hostages[num].objnum == -1);		//make sure not used
				// -- Matt -- commented out by MK on 11/19/94, hit often in level 3, level 4.  Assert(Hostages[num].objnum == -1);		//make sure not used
				Hostages[num].objnum = i;
				Hostages[num].objsig = Objects[i].signature;
			}
		}
	}

	if (do_read) {
		version = cfile_read_int(fp);

		for (i=0;i<num_hostages;i++) {

			Assert(Hostages[i].objnum != -1);		//make sure slot filled in

			Hostages[i].vclip_num = cfile_read_int(fp);

			#ifndef SHAREWARE
			if (Hostages[i].vclip_num<0 || Hostages[i].vclip_num>=MAX_HOSTAGES || Hostage_face_clip[Hostages[i].vclip_num].num_frames<=0)
				Hostages[i].vclip_num=0;

			Assert(Hostage_face_clip[Hostages[i].vclip_num].num_frames);
			#endif

			cfgets(Hostages[i].text, HOSTAGE_MESSAGE_LEN, fp);
		}
	}
	else
		for (i=0;i<num_hostages;i++) {
			Assert(Hostages[i].objnum != -1);		//make sure slot filled in
			Hostages[i].vclip_num = 0;
		}

}
#endif	//HOSTAGE_FACES

