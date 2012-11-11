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
 * Save game information
 *
 */

#include <stdio.h>
#include <string.h>
#include "pstypes.h"
#include "strutil.h"
#include "console.h"
#include "key.h"
#include "gr.h"
#include "palette.h"
#include "newmenu.h"
#include "inferno.h"
#ifdef EDITOR
#include "editor/editor.h"
#include "editor/esegment.h"
#endif
#include "dxxerror.h"
#include "object.h"
#include "game.h"
#include "screens.h"
#include "wall.h"
#include "gamemine.h"
#include "robot.h"
#include "bm.h"
#include "menu.h"
#include "switch.h"
#include "fuelcen.h"
#include "cntrlcen.h"
#include "powerup.h"
#include "weapon.h"
#include "newdemo.h"
#include "gameseq.h"
#include "automap.h"
#include "polyobj.h"
#include "text.h"
#include "gamefont.h"
#include "gamesave.h"
#include "gamepal.h"
#include "laser.h"
#include "byteswap.h"
#include "multi.h"
#include "makesig.h"

char Gamesave_current_filename[PATH_MAX];

int Gamesave_current_version;

#define GAME_VERSION            32
#define GAME_COMPATIBLE_VERSION 22

//version 28->29  add delta light support
//version 27->28  controlcen id now is reactor number, not model number
//version 28->29  ??
//version 29->30  changed trigger structure
//version 30->31  changed trigger structure some more
//version 31->32  change segment structure, make it 512 bytes w/o editor, add Segment2s array.

#define MENU_CURSOR_X_MIN       MENU_X
#define MENU_CURSOR_X_MAX       MENU_X+6

#ifdef EDITOR
struct {
	ushort  fileinfo_signature;
	ushort  fileinfo_version;
	int     fileinfo_sizeof;
} game_top_fileinfo;    // Should be same as first two fields below...

struct {
	ushort  fileinfo_signature;
	ushort  fileinfo_version;
	int     fileinfo_sizeof;
	char    mine_filename[15];
	int     level;
	int     player_offset;              // Player info
	int     player_sizeof;
	int     object_offset;              // Object info
	int     object_howmany;
	int     object_sizeof;
	int     walls_offset;
	int     walls_howmany;
	int     walls_sizeof;
	int     doors_offset;
	int     doors_howmany;
	int     doors_sizeof;
	int     triggers_offset;
	int     triggers_howmany;
	int     triggers_sizeof;
	int     links_offset;
	int     links_howmany;
	int     links_sizeof;
	int     control_offset;
	int     control_howmany;
	int     control_sizeof;
	int     matcen_offset;
	int     matcen_howmany;
	int     matcen_sizeof;
	int     dl_indices_offset;
	int     dl_indices_howmany;
	int     dl_indices_sizeof;
	int     delta_light_offset;
	int     delta_light_howmany;
	int     delta_light_sizeof;
} game_fileinfo;
#endif // EDITOR

//  LINT: adding function prototypes
void read_object(object *obj, PHYSFS_file *f, int version);
#ifdef EDITOR
void write_object(object *obj, short version, PHYSFS_file *f);
void do_load_save_levels(int save);
#endif
#ifndef NDEBUG
void dump_mine_info(void);
#endif

#ifdef EDITOR
extern char mine_filename[];
extern int save_mine_data_compiled(PHYSFS_file *SaveFile);
//--unused-- #else
//--unused-- char mine_filename[128];
#endif

int Gamesave_num_org_robots = 0;
//--unused-- grs_bitmap * Gamesave_saved_bitmap = NULL;

#ifdef EDITOR
// Return true if this level has a name of the form "level??"
// Note that a pathspec can appear at the beginning of the filename.
int is_real_level(char *filename)
{
	int len = strlen(filename);

	if (len < 6)
		return 0;

	return !d_strnicmp(&filename[len-11], "level", 5);

}
#endif

//--unused-- vms_angvec zero_angles={0,0,0};

#define vm_angvec_zero(v) do {(v)->p=(v)->b=(v)->h=0;} while (0)

int Gamesave_num_players=0;

int N_save_pof_names;
char Save_pof_names[MAX_POLYGON_MODELS][FILENAME_LEN];

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
			Assert(Robot_info[obj->id].model_num != -1);
				//if you fail this assert, it means that a robot in this level
				//hasn't been loaded, possibly because he's marked as
				//non-shareware.  To see what robot number, print obj->id.

			Assert(Robot_info[obj->id].always_0xabcd == 0xabcd);
				//if you fail this assert, it means that the robot_ai for
				//a robot in this level hasn't been loaded, possibly because
				//it's marked as non-shareware.  To see what robot number,
				//print obj->id.

			obj->rtype.pobj_info.model_num = Robot_info[obj->id].model_num;
			obj->size = Polygon_models[obj->rtype.pobj_info.model_num].rad;

			//@@Took out this ugly hack 1/12/96, because Mike has added code
			//@@that should fix it in a better way.
			//@@//this is a super-ugly hack.  Since the baby stripe robots have
			//@@//their firing point on their bounding sphere, the firing points
			//@@//can poke through a wall if the robots are very close to it. So
			//@@//we make their radii bigger so the guns can't get too close to 
			//@@//the walls
			//@@if (Robot_info[obj->id].flags & RIF_BIG_RADIUS)
			//@@	obj->size = (obj->size*3)/2;

			//@@if (obj->control_type==CT_AI && Robot_info[obj->id].attack_type)
			//@@	obj->size = obj->size*3/4;
		}

		if (obj->id == 65)						//special "reactor" robots
			obj->movement_type = MT_NONE;

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
				if (!d_stricmp(Pof_names[i],name)) {		//found it!	
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
		obj->ctype.powerup_info.creation_time = 0;

#ifdef NETWORK
#ifdef OLDPOWCAP
		if (Game_mode & GM_NETWORK)
			{
			  if (multi_powerup_is_4pack(obj->id))
				{
				 PowerupsInMine[obj->id-1]+=4;
			 	 MaxPowerupsAllowed[obj->id-1]+=4;
				}
			  PowerupsInMine[obj->id]++;
		     MaxPowerupsAllowed[obj->id]++;
		 	}
#else
		if (Game_mode & GM_NETWORK)
		{
			if (multi_powerup_is_4pack(obj->id))
			{
				PowerupsInMine[obj->id-1]+=4;
				MaxPowerupsAllowed[obj->id-1]+=4;
			}
			else
			{
				PowerupsInMine[obj->id]++;
				MaxPowerupsAllowed[obj->id]++;
			}
		}
#endif
#endif

	}

	if ( obj->type == OBJ_WEAPON )	{
		if ( obj->id >= N_weapon_types )	{
			obj->id = 0;
			Assert( obj->render_type != RT_POLYOBJ );
		}

		if (obj->id == PMINE_ID) {		//make sure pmines have correct values

			obj->mtype.phys_info.mass = Weapon_info[obj->id].mass;
			obj->mtype.phys_info.drag = Weapon_info[obj->id].drag;
			obj->mtype.phys_info.flags |= PF_FREE_SPINNING;

			// Make sure model number & size are correct...		
			Assert( obj->render_type == RT_POLYOBJ );

			obj->rtype.pobj_info.model_num = Weapon_info[obj->id].model_num;
			obj->size = Polygon_models[obj->rtype.pobj_info.model_num].rad;
		}
	}

	if ( obj->type == OBJ_CNTRLCEN ) {

		obj->render_type = RT_POLYOBJ;
		obj->control_type = CT_CNTRLCEN;

		if (Gamesave_current_version <= 1) { // descent 1 reactor
			obj->id = 0;                         // used to be only one kind of reactor
			obj->rtype.pobj_info.model_num = Reactors[0].model_num;// descent 1 reactor
		}

		// Make sure model number is correct...
		//obj->rtype.pobj_info.model_num = Reactors[obj->id].model_num;
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

		//@@if (obj->id > N_hostage_types)
		//@@	obj->id = 0;

		obj->render_type = RT_HOSTAGE;
		obj->control_type = CT_POWERUP;
	}

}

//static gs_skip(int len,PHYSFS_file *file)
//{
//
//	PHYSFSX_fseek(file,len,SEEK_CUR);
//}


extern int multi_powerup_is_4pack(int);
//reads one object of the given version from the given file
void read_object(object *obj,PHYSFS_file *f,int version)
{

	obj->type           = PHYSFSX_readByte(f);
	obj->id             = PHYSFSX_readByte(f);

	obj->control_type   = PHYSFSX_readByte(f);
	obj->movement_type  = PHYSFSX_readByte(f);
	obj->render_type    = PHYSFSX_readByte(f);
	obj->flags          = PHYSFSX_readByte(f);

	obj->segnum         = PHYSFSX_readShort(f);
	obj->attached_obj   = -1;

	PHYSFSX_readVector(&obj->pos,f);
	PHYSFSX_readMatrix(&obj->orient,f);

	obj->size           = PHYSFSX_readFix(f);
	obj->shields        = PHYSFSX_readFix(f);

	PHYSFSX_readVector(&obj->last_pos,f);

	obj->contains_type  = PHYSFSX_readByte(f);
	obj->contains_id    = PHYSFSX_readByte(f);
	obj->contains_count = PHYSFSX_readByte(f);

	switch (obj->movement_type) {

		case MT_PHYSICS:

			PHYSFSX_readVector(&obj->mtype.phys_info.velocity,f);
			PHYSFSX_readVector(&obj->mtype.phys_info.thrust,f);

			obj->mtype.phys_info.mass		= PHYSFSX_readFix(f);
			obj->mtype.phys_info.drag		= PHYSFSX_readFix(f);
			obj->mtype.phys_info.brakes	= PHYSFSX_readFix(f);

			PHYSFSX_readVector(&obj->mtype.phys_info.rotvel,f);
			PHYSFSX_readVector(&obj->mtype.phys_info.rotthrust,f);

			obj->mtype.phys_info.turnroll	= PHYSFSX_readFixAng(f);
			obj->mtype.phys_info.flags		= PHYSFSX_readShort(f);

			break;

		case MT_SPINNING:

			PHYSFSX_readVector(&obj->mtype.spin_rate,f);
			break;

		case MT_NONE:
			break;

		default:
			Int3();
	}

	switch (obj->control_type) {

		case CT_AI: {
			int i;

			obj->ctype.ai_info.behavior				= PHYSFSX_readByte(f);

			for (i=0;i<MAX_AI_FLAGS;i++)
				obj->ctype.ai_info.flags[i]			= PHYSFSX_readByte(f);

			obj->ctype.ai_info.hide_segment			= PHYSFSX_readShort(f);
			obj->ctype.ai_info.hide_index			= PHYSFSX_readShort(f);
			obj->ctype.ai_info.path_length			= PHYSFSX_readShort(f);
			obj->ctype.ai_info.cur_path_index		= PHYSFSX_readShort(f);

			if (version <= 25) {
				PHYSFSX_readShort(f);	//				obj->ctype.ai_info.follow_path_start_seg	= 
				PHYSFSX_readShort(f);	//				obj->ctype.ai_info.follow_path_end_seg		= 
			}

			break;
		}

		case CT_EXPLOSION:

			obj->ctype.expl_info.spawn_time		= PHYSFSX_readFix(f);
			obj->ctype.expl_info.delete_time		= PHYSFSX_readFix(f);
			obj->ctype.expl_info.delete_objnum	= PHYSFSX_readShort(f);
			obj->ctype.expl_info.next_attach = obj->ctype.expl_info.prev_attach = obj->ctype.expl_info.attach_parent = -1;

			break;

		case CT_WEAPON:

			//do I really need to read these?  Are they even saved to disk?

			obj->ctype.laser_info.parent_type		= PHYSFSX_readShort(f);
			obj->ctype.laser_info.parent_num		= PHYSFSX_readShort(f);
			obj->ctype.laser_info.parent_signature	= PHYSFSX_readInt(f);

			break;

		case CT_LIGHT:

			obj->ctype.light_info.intensity = PHYSFSX_readFix(f);
			break;

		case CT_POWERUP:

			if (version >= 25)
				obj->ctype.powerup_info.count = PHYSFSX_readInt(f);
			else
				obj->ctype.powerup_info.count = 1;

			if (obj->id == POW_VULCAN_WEAPON)
					obj->ctype.powerup_info.count = VULCAN_WEAPON_AMMO_AMOUNT;

			if (obj->id == POW_GAUSS_WEAPON)
					obj->ctype.powerup_info.count = VULCAN_WEAPON_AMMO_AMOUNT;

			if (obj->id == POW_OMEGA_WEAPON)
					obj->ctype.powerup_info.count = MAX_OMEGA_CHARGE;

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

			obj->rtype.pobj_info.model_num		= PHYSFSX_readInt(f);

			for (i=0;i<MAX_SUBMODELS;i++)
				PHYSFSX_readAngleVec(&obj->rtype.pobj_info.anim_angles[i],f);

			obj->rtype.pobj_info.subobj_flags	= PHYSFSX_readInt(f);

			tmo = PHYSFSX_readInt(f);

			#ifndef EDITOR
			obj->rtype.pobj_info.tmap_override	= tmo;
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

			obj->rtype.vclip_info.vclip_num	= PHYSFSX_readInt(f);
			obj->rtype.vclip_info.frametime	= PHYSFSX_readFix(f);
			obj->rtype.vclip_info.framenum	= PHYSFSX_readByte(f);

			break;

		case RT_LASER:
			break;

		default:
			Int3();

	}

}

#ifdef EDITOR

//writes one object to the given file
void write_object(object *obj, short version, PHYSFS_file *f)
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

			if (version <= 25)
			{
				PHYSFS_writeSLE16(f, -1);	//obj->ctype.ai_info.follow_path_start_seg
				PHYSFS_writeSLE16(f, -1);	//obj->ctype.ai_info.follow_path_end_seg
			}

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

			if (version >= 25)
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

extern int remove_trigger_num(int trigger_num);

// --------------------------------------------------------------------
// Load game 
// Loads all the relevant data for a level.
// If level != -1, it loads the filename with extension changed to .min
// Otherwise it loads the appropriate level mine.
// returns 0=everything ok, 1=old version, -1=error
int load_game_data(PHYSFS_file *LoadFile)
{
	int i,j;

	short game_top_fileinfo_version;
	int object_offset;
	int gs_num_objects;
	int num_delta_lights;
	int trig_size;

	//===================== READ FILE INFO ========================

#if 0
	PHYSFS_read(LoadFile, &game_top_fileinfo, sizeof(game_top_fileinfo), 1);
#endif

	// Check signature
	if (PHYSFSX_readShort(LoadFile) != 0x6705)
		return -1;

	// Read and check version number
	game_top_fileinfo_version = PHYSFSX_readShort(LoadFile);
	if (game_top_fileinfo_version < GAME_COMPATIBLE_VERSION )
		return -1;

	// We skip some parts of the former game_top_fileinfo
	PHYSFSX_fseek(LoadFile, 31, SEEK_CUR);

	object_offset = PHYSFSX_readInt(LoadFile);
	gs_num_objects = PHYSFSX_readInt(LoadFile);
	PHYSFSX_fseek(LoadFile, 8, SEEK_CUR);

	Num_walls = PHYSFSX_readInt(LoadFile);
	PHYSFSX_fseek(LoadFile, 20, SEEK_CUR);

	Num_triggers = PHYSFSX_readInt(LoadFile);
	PHYSFSX_fseek(LoadFile, 24, SEEK_CUR);

	trig_size = PHYSFSX_readInt(LoadFile);
	Assert(trig_size == sizeof(ControlCenterTriggers));
	(void)trig_size;
	PHYSFSX_fseek(LoadFile, 4, SEEK_CUR);

	Num_robot_centers = PHYSFSX_readInt(LoadFile);
	PHYSFSX_fseek(LoadFile, 4, SEEK_CUR);

	if (game_top_fileinfo_version >= 29) {
		PHYSFSX_fseek(LoadFile, 4, SEEK_CUR);
		Num_static_lights = PHYSFSX_readInt(LoadFile);
		PHYSFSX_fseek(LoadFile, 8, SEEK_CUR);
		num_delta_lights = PHYSFSX_readInt(LoadFile);
		PHYSFSX_fseek(LoadFile, 4, SEEK_CUR);
	} else {
		Num_static_lights = 0;
		num_delta_lights = 0;
	}

	if (game_top_fileinfo_version >= 31) //load mine filename
		// read newline-terminated string, not sure what version this changed.
		PHYSFSX_fgets(Current_level_name,sizeof(Current_level_name),LoadFile);
	else if (game_top_fileinfo_version >= 14) { //load mine filename
		// read null-terminated string
		char *p=Current_level_name;
		//must do read one char at a time, since no PHYSFSX_fgets()
		do *p = PHYSFSX_fgetc(LoadFile); while (*p++!=0);
	}
	else
		Current_level_name[0]=0;

	if (game_top_fileinfo_version >= 19) {	//load pof names
		N_save_pof_names = PHYSFSX_readShort(LoadFile);
		if (N_save_pof_names != 0x614d && N_save_pof_names != 0x5547) { // "Ma"de w/DMB beta/"GU"ILE
			Assert(N_save_pof_names < MAX_POLYGON_MODELS);
			PHYSFS_read(LoadFile,Save_pof_names,N_save_pof_names,FILENAME_LEN);
		}
	}

	//===================== READ PLAYER INFO ==========================


	//===================== READ OBJECT INFO ==========================

	Gamesave_num_org_robots = 0;
	Gamesave_num_players = 0;

	if (object_offset > -1) {
		if (PHYSFSX_fseek( LoadFile, object_offset, SEEK_SET ))
			Error( "Error seeking to object_offset in gamesave.c" );

		for (i = 0; i < gs_num_objects; i++) {

			read_object(&Objects[i], LoadFile, game_top_fileinfo_version);

			Objects[i].signature = obj_get_signature();
			verify_object( &Objects[i] );
		}

	}

	//===================== READ WALL INFO ============================

	for (i = 0; i < Num_walls; i++) {
		if (game_top_fileinfo_version >= 20)
			wall_read(&Walls[i], LoadFile); // v20 walls and up.
		else if (game_top_fileinfo_version >= 17) {
			v19_wall w;
			v19_wall_read(&w, LoadFile);
			Walls[i].segnum	        = w.segnum;
			Walls[i].sidenum	= w.sidenum;
			Walls[i].linked_wall	= w.linked_wall;
			Walls[i].type		= w.type;
			Walls[i].flags		= w.flags;
			Walls[i].hps		= w.hps;
			Walls[i].trigger	= w.trigger;
			Walls[i].clip_num	= w.clip_num;
			Walls[i].keys		= w.keys;
			Walls[i].state		= WALL_DOOR_CLOSED;
		} else {
			v16_wall w;
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

#if 0
	//===================== READ DOOR INFO ============================

	if (game_fileinfo.doors_offset > -1)
	{
		if (!PHYSFSX_fseek( LoadFile, game_fileinfo.doors_offset,SEEK_SET ))	{

			for (i=0;i<game_fileinfo.doors_howmany;i++) {

				if (game_top_fileinfo_version >= 20)
					active_door_read(&ActiveDoors[i], LoadFile); // version 20 and up
				else {
					v19_door d;
					int p;

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
#endif // 0

	//==================== READ TRIGGER INFO ==========================

	for (i = 0; i < Num_triggers; i++)
	{
		if (game_top_fileinfo_version < 31)
		{
			v30_trigger trig;
			int t,type;
			int flags = 0;
			type=0;

			if (game_top_fileinfo_version < 30) {
				v29_trigger trig29;
				int t;
				v29_trigger_read(&trig29, LoadFile);
				trig.flags	= trig29.flags;
				trig.num_links	= trig29.num_links;
				trig.num_links	= trig29.num_links;
				trig.value	= trig29.value;
				trig.time	= trig29.time;

				for (t=0;t<trig.num_links;t++) {
					trig.seg[t]  = trig29.seg[t];
					trig.side[t] = trig29.side[t];
				}
			}
			else
				v30_trigger_read(&trig, LoadFile);

			//Assert(trig.flags & TRIGGER_ON);
			trig.flags &= ~TRIGGER_ON;

			if (trig.flags & TRIGGER_CONTROL_DOORS)
				type = TT_OPEN_DOOR;
			else if (trig.flags & TRIGGER_SHIELD_DAMAGE)
				Int3();
			else if (trig.flags & TRIGGER_ENERGY_DRAIN)
				Int3();
			else if (trig.flags & TRIGGER_EXIT)
				type = TT_EXIT;
			//else if (trig.flags & TRIGGER_ONE_SHOT)
			//	Int3();
			else if (trig.flags & TRIGGER_MATCEN)
				type = TT_MATCEN;
			else if (trig.flags & TRIGGER_ILLUSION_OFF)
				type = TT_ILLUSION_OFF;
			else if (trig.flags & TRIGGER_SECRET_EXIT)
				type = TT_SECRET_EXIT;
			else if (trig.flags & TRIGGER_ILLUSION_ON)
				type = TT_ILLUSION_ON;
			else if (trig.flags & TRIGGER_UNLOCK_DOORS)
				type = TT_UNLOCK_DOOR;
			else if (trig.flags & TRIGGER_OPEN_WALL)
				type = TT_OPEN_WALL;
			else if (trig.flags & TRIGGER_CLOSE_WALL)
				type = TT_CLOSE_WALL;
			else if (trig.flags & TRIGGER_ILLUSORY_WALL)
				type = TT_ILLUSORY_WALL;
			else
				Int3();
			if (trig.flags & TRIGGER_ONE_SHOT)
				flags = TF_ONE_SHOT;
			Triggers[i].type        = type;
			Triggers[i].flags       = flags;
			Triggers[i].num_links   = trig.num_links;
			Triggers[i].num_links   = trig.num_links;
			Triggers[i].value       = trig.value;
			Triggers[i].time        = trig.time;
			for (t=0;t<trig.num_links;t++) {
				Triggers[i].seg[t] = trig.seg[t];
				Triggers[i].side[t] = trig.side[t];
			}
		}
		else
			trigger_read(&Triggers[i], LoadFile);
	}

	//================ READ CONTROL CENTER TRIGGER INFO ===============

	control_center_triggers_read_n(&ControlCenterTriggers, 1, LoadFile);

	//================ READ MATERIALOGRIFIZATIONATORS INFO ===============

	for (i = 0; i < Num_robot_centers; i++) {
		if (game_top_fileinfo_version < 27) {
			d1_matcen_info m;
			d1_matcen_info_read(&m, LoadFile);
			RobotCenters[i].robot_flags[0] = m.robot_flags[0];
			RobotCenters[i].robot_flags[1] = 0;
			RobotCenters[i].hit_points = m.hit_points;
			RobotCenters[i].interval = m.interval;
			RobotCenters[i].segnum = m.segnum;
			RobotCenters[i].fuelcen_num = m.fuelcen_num;
		}
		else
			matcen_info_read(&RobotCenters[i], LoadFile);
			//	Set links in RobotCenters to Station array
			for (j = 0; j <= Highest_segment_index; j++)
			if (Segment2s[j].special == SEGMENT_IS_ROBOTMAKER)
				if (Segment2s[j].matcen_num == i)
					RobotCenters[i].fuelcen_num = Segment2s[j].value;
	}

	//================ READ DL_INDICES INFO ===============

	for (i = 0; i < Num_static_lights; i++) {
		if (game_top_fileinfo_version < 29) {
			Int3();	//shouldn't be here!!!
		} else
			dl_index_read(&Dl_indices[i], LoadFile);
	}

	//	Indicate that no light has been subtracted from any vertices.
	clear_light_subtracted();

	//================ READ DELTA LIGHT INFO ===============

	for (i = 0; i < num_delta_lights; i++) {
		if (game_top_fileinfo_version < 29) {
			;
		} else
			delta_light_read(&Delta_lights[i], LoadFile);
	}

	//========================= UPDATE VARIABLES ======================

	reset_objects(gs_num_objects);

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


	reset_walls();

#if 0
	Num_open_doors = game_fileinfo.doors_howmany;
#endif // 0
	Num_open_doors = 0;

	//go through all walls, killing references to invalid triggers
	for (i=0;i<Num_walls;i++)
		if (Walls[i].trigger >= Num_triggers) {
			Walls[i].trigger = -1;	//kill trigger
		}

	//go through all triggers, killing unused ones
	for (i=0;i<Num_triggers;) {
		int w;

		//	Find which wall this trigger is connected to.
		for (w=0; w<Num_walls; w++)
			if (Walls[w].trigger == i)
				break;

	#ifdef EDITOR
		if (w == Num_walls) {
			remove_trigger_num(i);
		}
		else
	#endif
			i++;
	}

	//	MK, 10/17/95: Make walls point back at the triggers that control them.
	//	Go through all triggers, stuffing controlling_trigger field in Walls.
	{
		int t;

		for (i=0; i<Num_walls; i++)
			Walls[i].controlling_trigger = -1;

		for (t=0; t<Num_triggers; t++) {
			int	l;
			for (l=0; l<Triggers[t].num_links; l++) {
				int	seg_num, side_num, wall_num;

				seg_num = Triggers[t].seg[l];
				side_num = Triggers[t].side[l];
				wall_num = Segments[seg_num].sides[side_num].wall_num;

				//check to see that if a trigger requires a wall that it has one,
				//and if it requires a matcen that it has one

				if (Triggers[t].type == TT_MATCEN) {
					if (Segment2s[seg_num].special != SEGMENT_IS_ROBOTMAKER)
						Int3();		//matcen trigger doesn't point to matcen
				}
				else if (Triggers[t].type != TT_LIGHT_OFF && Triggers[t].type != TT_LIGHT_ON) {	//light triggers don't require walls
					if (wall_num == -1)
						Int3();	//	This is illegal.  This trigger requires a wall
					else
						Walls[wall_num].controlling_trigger = t;
				}
			}
		}
	}

	//fix old wall structs
	if (game_top_fileinfo_version < 17) {
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

	if (game_top_fileinfo_version < GAME_VERSION
	    && !(game_top_fileinfo_version == 25 && GAME_VERSION == 26))
		return 1;		//means old version
	else
		return 0;
}


int check_segment_connections(void);

extern void	set_ambient_sound_flags(void);

// ----------------------------------------------------------------------------

#define LEVEL_FILE_VERSION      8
//1 -> 2  add palette name
//2 -> 3  add control center explosion time
//3 -> 4  add reactor strength
//4 -> 5  killed hostage text stuff
//5 -> 6  added Secret_return_segment and Secret_return_orient
//6 -> 7  added flickering lights
//7 -> 8  made version 8 to be not compatible with D2 1.0 & 1.1

#ifndef RELEASE
const char *Level_being_loaded=NULL;
#endif

#ifdef COMPACT_SEGS
extern void ncache_flush();
#endif

extern int Slide_segs_computed;
extern int d1_pig_present;

int no_old_level_file_error=0;

//loads a level (.LVL) file from disk
//returns 0 if success, else error code
int load_level(const char * filename_passed)
{
#ifdef EDITOR
	int use_compiled_level=1;
#endif
	PHYSFS_file * LoadFile;
	char filename[PATH_MAX];
	int sig, minedata_offset, gamedata_offset;
	int mine_err, game_err;
#ifdef NETWORK
	int i;
#endif

	Slide_segs_computed = 0;

#ifdef NETWORK
   if (Game_mode & GM_NETWORK)
	 {
	  for (i=0;i<MAX_POWERUP_TYPES;i++)
		{
			MaxPowerupsAllowed[i]=0;
			PowerupsInMine[i]=0;
		}
	 }
#endif

	#ifdef COMPACT_SEGS
	ncache_flush();
	#endif

	#ifndef RELEASE
	Level_being_loaded = filename_passed;
	#endif

	strcpy(filename,filename_passed);

#ifdef EDITOR
	//if we have the editor, try the LVL first, no matter what was passed.
	//if we don't have an LVL, try what was passed or RL2  
	//if we don't have the editor, we just use what was passed

	change_filename_extension(filename,filename_passed,".lvl");
	use_compiled_level = 0;

	if (!PHYSFSX_exists(filename,1))
	{
		char *p = strrchr(filename_passed, '.');

		if (d_stricmp(p, ".lvl"))
			strcpy(filename, filename_passed);	// set to what was passed
		else
			change_filename_extension(filename, filename, ".rl2");
		use_compiled_level = 1;
	}		
#endif

	if (!PHYSFSX_exists(filename,1))
		sprintf(filename,"%s%s",MISSION_DIR,filename_passed);

	LoadFile = PHYSFSX_openReadBuffered( filename );

	if (!LoadFile)	{
		#ifdef EDITOR
			return 1;
		#else
			Error("Can't open file <%s>\n",filename);
		#endif
	}

	strcpy( Gamesave_current_filename, filename );

	sig                      = PHYSFSX_readInt(LoadFile);
	Gamesave_current_version = PHYSFSX_readInt(LoadFile);
	minedata_offset          = PHYSFSX_readInt(LoadFile);
	gamedata_offset          = PHYSFSX_readInt(LoadFile);

	Assert(sig == MAKE_SIG('P','L','V','L'));
	(void)sig;

	if (Gamesave_current_version >= 8) {    //read dummy data
		PHYSFSX_readInt(LoadFile);
		PHYSFSX_readShort(LoadFile);
		PHYSFSX_readByte(LoadFile);
	}

	if (Gamesave_current_version < 5)
		PHYSFSX_readInt(LoadFile);       //was hostagetext_offset

	if (Gamesave_current_version > 1)
		PHYSFSX_fgets(Current_level_palette,sizeof(Current_level_palette),LoadFile);
	if (Gamesave_current_version <= 1 || Current_level_palette[0]==0) // descent 1 level
		strcpy(Current_level_palette, DEFAULT_LEVEL_PALETTE);

	if (Gamesave_current_version >= 3)
		Base_control_center_explosion_time = PHYSFSX_readInt(LoadFile);
	else
		Base_control_center_explosion_time = DEFAULT_CONTROL_CENTER_EXPLOSION_TIME;

	if (Gamesave_current_version >= 4)
		Reactor_strength = PHYSFSX_readInt(LoadFile);
	else
		Reactor_strength = -1;  //use old defaults

	if (Gamesave_current_version >= 7) {
		int i;

		Num_flickering_lights = PHYSFSX_readInt(LoadFile);
		Assert((Num_flickering_lights >= 0) && (Num_flickering_lights < MAX_FLICKERING_LIGHTS));
		for (i = 0; i < Num_flickering_lights; i++)
			flickering_light_read(&Flickering_lights[i], LoadFile);
	}
	else
		Num_flickering_lights = 0;

	if (Gamesave_current_version < 6) {
		Secret_return_segment = 0;
		Secret_return_orient.rvec.x = F1_0;
		Secret_return_orient.rvec.y = 0;
		Secret_return_orient.rvec.z = 0;
		Secret_return_orient.fvec.x = 0;
		Secret_return_orient.fvec.y = F1_0;
		Secret_return_orient.fvec.z = 0;
		Secret_return_orient.uvec.x = 0;
		Secret_return_orient.uvec.y = 0;
		Secret_return_orient.uvec.z = F1_0;
	} else {
		Secret_return_segment = PHYSFSX_readInt(LoadFile);
		Secret_return_orient.rvec.x = PHYSFSX_readInt(LoadFile);
		Secret_return_orient.rvec.y = PHYSFSX_readInt(LoadFile);
		Secret_return_orient.rvec.z = PHYSFSX_readInt(LoadFile);
		Secret_return_orient.fvec.x = PHYSFSX_readInt(LoadFile);
		Secret_return_orient.fvec.y = PHYSFSX_readInt(LoadFile);
		Secret_return_orient.fvec.z = PHYSFSX_readInt(LoadFile);
		Secret_return_orient.uvec.x = PHYSFSX_readInt(LoadFile);
		Secret_return_orient.uvec.y = PHYSFSX_readInt(LoadFile);
		Secret_return_orient.uvec.z = PHYSFSX_readInt(LoadFile);
	}

	PHYSFSX_fseek(LoadFile,minedata_offset,SEEK_SET);
	#ifdef EDITOR
	if (!use_compiled_level) {
		mine_err = load_mine_data(LoadFile);
#if 0 // get from d1src if needed
		// Compress all uv coordinates in mine, improves texmap precision. --MK, 02/19/96
		compress_uv_coordinates_all();
#endif
	} else
	#endif
		//NOTE LINK TO ABOVE!!
		mine_err = load_mine_data_compiled(LoadFile);

	/* !!!HACK!!!
	 * Descent 1 - Level 19: OBERON MINE has some ugly overlapping rooms (segment 484).
	 * HACK to make this issue less visible by moving one vertex a little.
	 */
	if (Current_mission && !d_stricmp("Descent: First Strike",Current_mission_longname) && !d_stricmp("level19.rdl",filename) && PHYSFS_fileLength(LoadFile) == 136706)
		Vertices[1905].z =-385*F1_0;
	/* !!!HACK!!!
	 * Descent 2 - Level 12: MAGNACORE STATION has a segment (104) with illegal dimensions.
	 * HACK to fix this by moving the Vertex and fixing the associated Normals.
	 * NOTE: This only fixes the normals of segment 104, not the other ones connected to this Vertex but this is unsignificant.
	 */
	if (Current_mission && !d_stricmp("Descent 2: Counterstrike!",Current_mission_longname) && !d_stricmp("d2levc-4.rl2",filename)
		&& ( Vertices[Segments[104].verts[0]].x == -53990800 && Vertices[Segments[104].verts[0]].y == -59927741 && Vertices[Segments[104].verts[0]].z == 23034584 )
		&& ( Segments[104].sides[1].normals[0].x == 56775 && Segments[104].sides[1].normals[0].y == -27796 && Segments[104].sides[1].normals[0].z == -17288 && Segments[104].sides[1].normals[1].x == 50157 && Segments[104].sides[1].normals[1].y == -34561 && Segments[104].sides[1].normals[1].z == -24180 )
		&& ( Segments[104].sides[2].normals[0].x == 60867 && Segments[104].sides[2].normals[0].y == -19485 && Segments[104].sides[2].normals[0].z == -14507 && Segments[104].sides[2].normals[1].x == 55485 && Segments[104].sides[2].normals[1].y == -29668 && Segments[104].sides[2].normals[1].z == -18332 )
		)
	{
			Vertices[Segments[104].verts[0]].x = -53859726;
			Vertices[Segments[104].verts[0]].y = -59927743;
			Vertices[Segments[104].verts[0]].z = 23034586;
			Segments[104].sides[1].normals[0].x = 56123;
			Segments[104].sides[1].normals[0].y = -27725;
			Segments[104].sides[1].normals[0].z = -19401;
			Segments[104].sides[1].normals[1].x = 49910;
			Segments[104].sides[1].normals[1].y = -33946;
			Segments[104].sides[1].normals[1].z = -25525;
			Segments[104].sides[2].normals[0].x = 60903;
			Segments[104].sides[2].normals[0].y = -18371;
			Segments[104].sides[2].normals[0].z = -15753;
			Segments[104].sides[2].normals[1].x = 57004;
			Segments[104].sides[2].normals[1].y = -26385;
			Segments[104].sides[2].normals[1].z = -18688;
			// I feel so dirty now ...
	}

	if (mine_err == -1) {   //error!!
		PHYSFS_close(LoadFile);
		return 2;
	}

	PHYSFSX_fseek(LoadFile,gamedata_offset,SEEK_SET);
	game_err = load_game_data(LoadFile);

	if (game_err == -1) {   //error!!
		PHYSFS_close(LoadFile);
		return 3;
	}

	//======================== CLOSE FILE =============================

	PHYSFS_close( LoadFile );

	set_ambient_sound_flags();

	#ifdef EDITOR
	//If a Descent 1 level and the Descent 1 pig isn't present, pretend it's a Descent 2 level.
	if (EditorWindow && (Gamesave_current_version <= 3) && !d1_pig_present)
	{
		if (!no_old_level_file_error)
			Warning("A Descent 1 level was loaded,\n"
					"and there is no Descent 1 texture\n"
					"set available. Saving it will\n"
					"convert it to a Descent 2 level.");

		Gamesave_current_version = LEVEL_FILE_VERSION;
	}
	#endif

	#ifdef EDITOR
	if (EditorWindow)
		editor_status_fmt("Loaded NEW mine %s, \"%s\"",filename,Current_level_name);
	#endif

	#if !defined(NDEBUG) && !defined(COMPACT_SEGS)
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

	return newmenu_do( NULL, "Enter mine name", 2, m, NULL, NULL ) >= 0;

}
#endif


#ifdef EDITOR

// --------------------------------------------------------------------------------------
//	Create a new mine, set global variables.
int create_new_mine(void)
{
	int	s;
	vms_vector	sizevec;
	vms_matrix	m1 = IDENTITY_MATRIX;
	
	// initialize_mine_arrays();
	
	//	gamestate_not_restored = 1;
	
	// Clear refueling center code
	fuelcen_reset();
	//	hostage_init_all();
	
	init_all_vertices();
	
	Current_level_num = 0;		//0 means not a real level
	Current_level_name[0] = 0;
	Gamesave_current_version = GAME_VERSION;
	
	strcpy(Current_level_palette, DEFAULT_LEVEL_PALETTE);
	
	Cur_object_index = -1;
	reset_objects(1);		//just one object, the player
	
	num_groups = 0;
	current_group = -1;
	
	
	Num_vertices = 0;		// Number of vertices in global array.
	Highest_vertex_index = 0;
	Num_segments = 0;		// Number of segments in global array, will get increased in med_create_segment
	Highest_segment_index = 0;
	Cursegp = Segments;	// Say current segment is the only segment.
	Curside = WBACK;		// The active side is the back side
	Markedsegp = 0;		// Say there is no marked segment.
	Markedside = WBACK;	//	Shouldn't matter since Markedsegp == 0, but just in case...
	for (s=0;s<MAX_GROUPS+1;s++) {
		GroupList[s].num_segments = 0;		
		GroupList[s].num_vertices = 0;		
		Groupsegp[s] = NULL;
		Groupside[s] = 0;
	}
	
	Num_robot_centers = 0;
	Num_open_doors = 0;
	wall_init();
	trigger_init();
	
	// Create New_segment, which is the segment we will be adding at each instance.
	med_create_new_segment(vm_vec_make(&sizevec,DEFAULT_X_SIZE,DEFAULT_Y_SIZE,DEFAULT_Z_SIZE));		// New_segment = Segments[0];
	//	med_create_segment(Segments,0,0,0,DEFAULT_X_SIZE,DEFAULT_Y_SIZE,DEFAULT_Z_SIZE,vm_mat_make(&m1,F1_0,0,0,0,F1_0,0,0,0,F1_0));
	med_create_segment(Segments,0,0,0,DEFAULT_X_SIZE,DEFAULT_Y_SIZE,DEFAULT_Z_SIZE,&m1);
	
	N_found_segs = 0;
	N_selected_segs = 0;
	N_warning_segs = 0;
	
	//--repair-- create_local_segment_data();
	
	ControlCenterTriggers.num_links = 0;
	
	create_new_mission();
	
    //editor_status("New mine created.");
	return	0;			// say no error
}

int	Errors_in_mine;

// -----------------------------------------------------------------------------
int compute_num_delta_light_records(void)
{
	int	i;
	int	total = 0;

	for (i=0; i<Num_static_lights; i++) {
		total += Dl_indices[i].count;
	}

	return total;

}

// -----------------------------------------------------------------------------
// Save game
int save_game_data(PHYSFS_file *SaveFile)
{
	short game_top_fileinfo_version = Gamesave_current_version >= 5 ? 31 : 25;
	int  player_offset=0, object_offset=0, walls_offset=0, doors_offset=0, triggers_offset=0, control_offset=0, matcen_offset=0; //, links_offset;
	int	dl_indices_offset=0, delta_light_offset=0;
	int offset_offset=0, end_offset=0;
	int num_delta_lights=0;
	int i;

	//===================== SAVE FILE INFO ========================

	PHYSFS_writeSLE16(SaveFile, 0x6705);	// signature
	PHYSFS_writeSLE16(SaveFile, game_top_fileinfo_version);
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

	if (game_top_fileinfo_version >= 29)
	{
		WRITE_HEADER_ENTRY(dl_index, Num_static_lights);
		WRITE_HEADER_ENTRY(delta_light, num_delta_lights = compute_num_delta_light_records());
	}

	// Write the mine name
	if (game_top_fileinfo_version >= 31)
		PHYSFSX_printf(SaveFile, "%s\n", Current_level_name);
	else if (game_top_fileinfo_version >= 14)
		PHYSFSX_writeString(SaveFile, Current_level_name);

	if (game_top_fileinfo_version >= 19)
	{
		PHYSFS_writeSLE16(SaveFile, N_polygon_models);
		PHYSFS_write(SaveFile, Pof_names, sizeof(*Pof_names), N_polygon_models);
	}

	//==================== SAVE PLAYER INFO ===========================

	player_offset = PHYSFS_tell(SaveFile);
	PHYSFS_write(SaveFile, &Players[Player_num], sizeof(player), 1);	// not endian friendly, but not used either

	//==================== SAVE OBJECT INFO ===========================

	object_offset = PHYSFS_tell(SaveFile);
	//fwrite( &Objects, sizeof(object), game_fileinfo.object_howmany, SaveFile );
	{
		for (i = 0; i <= Highest_object_index; i++)
			write_object(&Objects[i], game_top_fileinfo_version, SaveFile);
	}

	//==================== SAVE WALL INFO =============================

	walls_offset = PHYSFS_tell(SaveFile);
	for (i = 0; i < Num_walls; i++)
		wall_write(&Walls[i], game_top_fileinfo_version, SaveFile);

	//==================== SAVE DOOR INFO =============================

#if 0
	doors_offset = PHYSFS_tell(SaveFile);
	for (i = 0; i < Num_open_doors; i++)
		door_write(&ActiveDoors[i], game_top_fileinfo_version, SaveFile);
#endif

	//==================== SAVE TRIGGER INFO =============================

	triggers_offset = PHYSFS_tell(SaveFile);
	for (i = 0; i < Num_triggers; i++)
		trigger_write(&Triggers[i], game_top_fileinfo_version, SaveFile);

	//================ SAVE CONTROL CENTER TRIGGER INFO ===============

	control_offset = PHYSFS_tell(SaveFile);
	control_center_triggers_write(&ControlCenterTriggers, SaveFile);


	//================ SAVE MATERIALIZATION CENTER TRIGGER INFO ===============

	matcen_offset = PHYSFS_tell(SaveFile);
	for (i = 0; i < Num_robot_centers; i++)
		matcen_info_write(&RobotCenters[i], game_top_fileinfo_version, SaveFile);

	//================ SAVE DELTA LIGHT INFO ===============
	if (game_top_fileinfo_version >= 29)
	{
		dl_indices_offset = PHYSFS_tell(SaveFile);
		for (i = 0; i < Num_static_lights; i++)
			dl_index_write(&Dl_indices[i], SaveFile);

		delta_light_offset = PHYSFS_tell(SaveFile);
		for (i = 0; i < num_delta_lights; i++)
			delta_light_write(&Delta_lights[i], SaveFile);
	}

	//============= SAVE OFFSETS ===============

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
	if (game_top_fileinfo_version >= 29)
	{
		WRITE_OFFSET(dl_indices, 3);
		WRITE_OFFSET(delta_light, 0);
	}

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
	char temp_filename[PATH_MAX];
	int minedata_offset=0,gamedata_offset=0;

//	if ( !compiled_version )
	{
		write_game_text_file(filename);

		if (Errors_in_mine) {
			if (is_real_level(filename)) {
				char  ErrorMessage[200];
	
				sprintf( ErrorMessage, "Warning: %i errors in this mine!\n", Errors_in_mine );
				gr_palette_load(gr_palette);
	 
				if (nm_messagebox( NULL, 2, "Cancel Save", "Save", ErrorMessage )!=1)	{
					return 1;
				}
			}
		}
//		change_filename_extension(temp_filename,filename,".LVL");
	}
//	else
	{
		if (Gamesave_current_version <= 3)
			change_filename_extension(temp_filename, filename, ".RDL");
		else
			change_filename_extension(temp_filename, filename, ".RL2");
	}

	SaveFile = PHYSFSX_openWriteBuffered(temp_filename);
	if (!SaveFile)
	{
		char ErrorMessage[256];

		snprintf( ErrorMessage, sizeof(ErrorMessage), "ERROR: Cannot write to '%s'.", temp_filename);
		gr_palette_load(gr_palette);
		nm_messagebox( NULL, 1, "Ok", ErrorMessage );
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

	PHYSFS_writeSLE32(SaveFile, MAKE_SIG('P','L','V','L'));
	PHYSFS_writeSLE32(SaveFile, Gamesave_current_version);

	//save placeholders
	PHYSFS_writeSLE32(SaveFile, minedata_offset);
	PHYSFS_writeSLE32(SaveFile, gamedata_offset);

	//Now write the damn data

	if (Gamesave_current_version >= 8)
	{
		//write the version 8 data (to make file unreadable by 1.0 & 1.1)
		PHYSFS_writeSLE32(SaveFile, GameTime64);
		PHYSFS_writeSLE16(SaveFile, d_tick_count);
		PHYSFSX_writeU8(SaveFile, FrameTime);
	}

	if (Gamesave_current_version < 5)
		PHYSFS_writeSLE32(SaveFile, -1);       //was hostagetext_offset

	// Write the palette file name
	if (Gamesave_current_version > 1)
		PHYSFSX_printf(SaveFile, "%s\n", Current_level_palette);

	if (Gamesave_current_version >= 3)
		PHYSFS_writeSLE32(SaveFile, Base_control_center_explosion_time);
	if (Gamesave_current_version >= 4)
		PHYSFS_writeSLE32(SaveFile, Reactor_strength);

	if (Gamesave_current_version >= 7)
	{
		int i;

		PHYSFS_writeSLE32(SaveFile, Num_flickering_lights);
		for (i = 0; i < Num_flickering_lights; i++)
			flickering_light_write(&Flickering_lights[i], SaveFile);
	}

	if (Gamesave_current_version >= 6)
	{
		PHYSFS_writeSLE32(SaveFile, Secret_return_segment);
		PHYSFSX_writeVector(SaveFile, &Secret_return_orient.rvec);
		PHYSFSX_writeVector(SaveFile, &Secret_return_orient.fvec);
		PHYSFSX_writeVector(SaveFile, &Secret_return_orient.uvec);
	}

	minedata_offset = PHYSFS_tell(SaveFile);
#if 0	// only save compiled mine data
	if ( !compiled_version )	
		save_mine_data(SaveFile);
	else
#endif
		save_mine_data_compiled(SaveFile);
	gamedata_offset = PHYSFS_tell(SaveFile);
	save_game_data(SaveFile);

	PHYSFS_seek(SaveFile, sizeof(int) + sizeof(Gamesave_current_version));
	PHYSFS_writeSLE32(SaveFile, minedata_offset);
	PHYSFS_writeSLE32(SaveFile, gamedata_offset);

	if (Gamesave_current_version < 5)
		PHYSFS_writeSLE32(SaveFile, PHYSFS_fileLength(SaveFile));

	//==================== CLOSE THE FILE =============================
	PHYSFS_close(SaveFile);

//	if ( !compiled_version )
	{
		if (EditorWindow)
			editor_status_fmt("Saved mine %s, \"%s\"",filename,Current_level_name);
	}

	return 0;

}

#if 0 //dunno - 3rd party stuff?
extern void compress_uv_coordinates_all(void);
#endif

int save_level(char * filename)
{
	int r1;

	// Save normal version...
	//save_level_sub(filename, 0);	// just save compiled one

	// Save compiled version...
	r1 = save_level_sub(filename, 1);

	return r1;
}

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

			if (Segment2s[segnum].static_light > max_sl)
				max_sl = Segment2s[segnum].static_light;

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

#ifdef EDITOR

//read in every level in mission and save out compiled version 
void save_all_compiled_levels(void)
{
	do_load_save_levels(1);
}

//read in every level in mission
void load_all_levels(void)
{
	do_load_save_levels(0);
}


void do_load_save_levels(int save)
{
	int level_num;

	if (! SafetyCheck())
		return;

	no_old_level_file_error=1;

	for (level_num=1;level_num<=Last_level;level_num++) {
		load_level(Level_names[level_num-1]);
		load_palette(Current_level_palette,1,1);		//don't change screen
		if (save)
			save_level_sub(Level_names[level_num-1],1);
	}

	for (level_num=-1;level_num>=Last_secret_level;level_num--) {
		load_level(Secret_level_names[-level_num-1]);
		load_palette(Current_level_palette,1,1);		//don't change screen
		if (save)
			save_level_sub(Secret_level_names[-level_num-1],1);
	}

	no_old_level_file_error=0;

}

#endif
