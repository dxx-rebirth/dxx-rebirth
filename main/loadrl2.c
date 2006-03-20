#include <stdio.h>
#include <string.h>

#include "hostage.h"

#include "error.h"

#include "cfile.h"
#include "object.h"
#include "wall.h"
#include "fuelcen.h"
#include "player.h"
#include "loadrdl.h"
#include "powerup.h"
#include "polyobj.h"

#include "textures.h"

#include "mono.h"

char Current_level_name[16];
char Level_palette[8];
int N_save_pof_names=25;
#define MAX_POLYGON_MODELS_NEW 167
char Save_pof_names[MAX_POLYGON_MODELS_NEW][13];
int Gamesave_num_players=0;
int Gamesave_num_org_robots = 0;

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
static int convert_tmap(int tmap) {
    return (tmap >= NumTextures) ? tmap % NumTextures : tmap;
}
static int convert_polymod(int polymod) {
    return (polymod >= N_polygon_models) ? polymod % N_polygon_models : polymod;
}

#if 1
void check_and_fix_matrix(vms_matrix *m);

void verify_object( object * obj )      {

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
				if (!strcasecmp(Pof_names[i],name)) {		//found it!	
					// mprintf((0,"Mapping <%s> to %d (was %d)\n",name,i,obj->rtype.pobj_info.model_num));
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
#endif

/*static*/ int read_int(CFILE *file)
{
	int i;

	if (cfread( &i, sizeof(i), 1, file) != 1)
		Error( "Error reading int in gamesave.c" );

	return i;
}

/*static*/ fix read_fix(CFILE *file)
{
	fix f;

	if (cfread( &f, sizeof(f), 1, file) != 1)
		Error( "Error reading fix in gamesave.c" );

	return f;
}

/*static*/ short read_short(CFILE *file)
{
	short s;

	if (cfread( &s, sizeof(s), 1, file) != 1)
		Error( "Error reading short in gamesave.c" );

	return s;
}

static short read_fixang(CFILE *file)
{
	fixang f;

	if (cfread( &f, sizeof(f), 1, file) != 1)
		Error( "Error reading fixang in gamesave.c" );

	return f;
}

/*static*/ byte read_byte(CFILE *file)
{
	byte b;

	if (cfread( &b, sizeof(b), 1, file) != 1)
		Error( "Error reading byte in gamesave.c" );

	return b;
}

#if 0
static ubyte read_ubyte(CFILE *file)
{
	byte b;

	if (cfread( &b, sizeof(b), 1, file) != 1)
		Error( "Error reading byte in gamesave.c" );

	return b;
}
#endif

static void read_vector(vms_vector *v,CFILE *file)
{
	v->x = read_fix(file);
	v->y = read_fix(file);
	v->z = read_fix(file);
}

static void read_matrix(vms_matrix *m,CFILE *file)
{
	read_vector(&m->rvec,file);
	read_vector(&m->uvec,file);
	read_vector(&m->fvec,file);
}

static void read_angvec(vms_angvec *v,CFILE *file)
{
	v->p = read_fixang(file);
	v->b = read_fixang(file);
	v->h = read_fixang(file);
}

//reads one object of the given version from the given file
void read_object(object *obj,CFILE *f,int version)
{
	obj->type		= read_byte(f);
	obj->id 		= read_byte(f);
	
	if (obj->type == OBJ_ROBOT && obj->id > 23) {
		obj->id = obj->id % 24;
	}

	obj->control_type	= read_byte(f);
	obj->movement_type	= read_byte(f);
	obj->render_type	= read_byte(f);
	obj->flags		= read_byte(f);

	obj->segnum		= read_short(f);
	obj->attached_obj	= -1;

	read_vector(&obj->pos,f);
	read_matrix(&obj->orient,f);

	obj->size		= read_fix(f);
	obj->shields		= read_fix(f);

	read_vector(&obj->last_pos,f);

	obj->contains_type	= read_byte(f);
	obj->contains_id	= read_byte(f);
	obj->contains_count	= read_byte(f);

	switch (obj->movement_type) {

		case MT_PHYSICS:

			read_vector(&obj->mtype.phys_info.velocity,f);
			read_vector(&obj->mtype.phys_info.thrust,f);

			obj->mtype.phys_info.mass	= read_fix(f);
			obj->mtype.phys_info.drag	= read_fix(f);
			obj->mtype.phys_info.brakes	= read_fix(f);

			read_vector(&obj->mtype.phys_info.rotvel,f);
			read_vector(&obj->mtype.phys_info.rotthrust,f);

			obj->mtype.phys_info.turnroll	= read_fixang(f);
			obj->mtype.phys_info.flags	= read_short(f);

			break;

		case MT_SPINNING:

			read_vector(&obj->mtype.spin_rate,f);
			break;

		case MT_NONE:
			break;

		default:
			Int3();
	}

	switch (obj->control_type) {

		case CT_AI: {
			int i;

			obj->ctype.ai_info.behavior				= read_byte(f);

			for (i=0;i<MAX_AI_FLAGS;i++)
				obj->ctype.ai_info.flags[i]			= read_byte(f);

			obj->ctype.ai_info.hide_segment   = read_short(f);
			obj->ctype.ai_info.hide_index	  = read_short(f);
			obj->ctype.ai_info.path_length	  = read_short(f);
			obj->ctype.ai_info.cur_path_index = read_short(f);

			if (version <= 25) {
				obj->ctype.ai_info.follow_path_start_seg = read_short(f);
				obj->ctype.ai_info.follow_path_end_seg	 = read_short(f);
			}

			break;
		}

		case CT_EXPLOSION:

			obj->ctype.expl_info.spawn_time		= read_fix(f);
			obj->ctype.expl_info.delete_time	= read_fix(f);
			obj->ctype.expl_info.delete_objnum	= read_short(f);
			obj->ctype.expl_info.next_attach = obj->ctype.expl_info.prev_attach = obj->ctype.expl_info.attach_parent = -1;

			break;

		case CT_WEAPON:

			//do I really need to read these?  Are they even saved to disk?

			obj->ctype.laser_info.parent_type	= read_short(f);
			obj->ctype.laser_info.parent_num	= read_short(f);
			obj->ctype.laser_info.parent_signature	= read_int(f);

			break;

		case CT_LIGHT:

			obj->ctype.light_info.intensity = read_fix(f);
			break;

		case CT_POWERUP:

			if (version >= 25)
				obj->ctype.powerup_info.count = read_int(f);
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

			obj->rtype.pobj_info.model_num		= convert_polymod(read_int(f));

			for (i=0;i<MAX_SUBMODELS;i++)
				read_angvec(&obj->rtype.pobj_info.anim_angles[i],f);

			obj->rtype.pobj_info.subobj_flags	= read_int(f);

			tmo = read_int(f);

			#ifndef EDITOR
			obj->rtype.pobj_info.tmap_override	= convert_tmap(tmo);
			#else
			if (tmo==-1)
				obj->rtype.pobj_info.tmap_override	= -1;
			else {
				int xlated_tmo = tmap_xlate_table[tmo];
				if (xlated_tmo < 0)	{
					mprintf( (0, "Couldn't find texture for demo object, model_num = %d\n", obj->rtype.pobj_info.model_num));
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

			obj->rtype.vclip_info.vclip_num = convert_vclip(read_int(f));
			obj->rtype.vclip_info.frametime	= read_fix(f);
			obj->rtype.vclip_info.framenum	= read_byte(f);

			break;

		case RT_LASER:
			break;

		default:
			Int3();

	}

}

int load_mine_data_compiled_new(CFILE *LoadFile, int lvl_version)
{
	int		i,segnum,sidenum;
	ubyte		version;
	short		temp_short;
	ushort	temp_ushort;
	ubyte		bit_mask;

	//	For compiled levels, textures map to themselves, prevent tmap_override always being gray,
	//	bug which Matt and John refused to acknowledge, so here is Mike, fixing it.
#ifdef EDITOR
	for (i=0; i<MAX_TEXTURES; i++)
		tmap_xlate_table[i] = i;
#endif

//	memset( Segments, 0, sizeof(segment)*MAX_SEGMENTS );
	fuelcen_reset();

	//=============================== Reading part ==============================
	cfread( &version, sizeof(ubyte), 1, LoadFile );						// 1 byte = compiled version
//        Assert( version==COMPILED_MINE_VERSION );
#ifndef NDEBUG
	if (version!=COMPILED_MINE_VERSION){
		mprintf((0,"mine version=%i\n"));//many levels have "wrong" versions.  Theres no point in aborting because of it, I think.
	}
#endif

	cfread( &temp_ushort, sizeof(ushort), 1, LoadFile );					// 2 bytes = Num_vertices
	Num_vertices = temp_ushort;
	Assert( Num_vertices <= MAX_VERTICES );

	cfread( &temp_ushort, sizeof(ushort), 1, LoadFile );					// 2 bytes = Num_segments
	Num_segments = temp_ushort;
	Assert( Num_segments <= MAX_SEGMENTS );

	cfread( Vertices, sizeof(vms_vector), Num_vertices, LoadFile );

	for (segnum=0; segnum<Num_segments; segnum++ )	{
		int	bit;

		#ifdef EDITOR
		Segments[segnum].segnum = segnum;
		Segments[segnum].group = 0;
		#endif

 		cfread( &bit_mask, sizeof(ubyte), 1, LoadFile );

		for (bit=0; bit<MAX_SIDES_PER_SEGMENT; bit++) {
			if (bit_mask & (1 << bit))
		 		cfread( &Segments[segnum].children[bit], sizeof(short), 1, LoadFile );
			else
				Segments[segnum].children[bit] = -1;
		}

		// Read short Segments[segnum].verts[MAX_VERTICES_PER_SEGMENT]
		cfread( Segments[segnum].verts, sizeof(short), MAX_VERTICES_PER_SEGMENT, LoadFile );
		Segments[segnum].objects = -1;

		if (lvl_version <= 5 && bit_mask & (1 << MAX_SIDES_PER_SEGMENT)) {
			// Read ubyte	Segments[segnum].special
			cfread( &Segments[segnum].special, sizeof(ubyte), 1, LoadFile );
			// Read byte	Segments[segnum].matcen_num
			cfread( &Segments[segnum].matcen_num, sizeof(ubyte), 1, LoadFile );
			// Read short	Segments[segnum].value
			cfread( &Segments[segnum].value, sizeof(short), 1, LoadFile );
		} else {
			Segments[segnum].special = 0;
			Segments[segnum].matcen_num = -1;
			Segments[segnum].value = 0;
		}

		if (lvl_version == 1) {
			// Read fix	Segments[segnum].static_light (shift down 5 bits, write as short)
			cfread( &temp_ushort, sizeof(temp_ushort), 1, LoadFile );
			Segments[segnum].static_light	= ((fix)temp_ushort) << 4;
			//cfread( &Segments[segnum].static_light, sizeof(fix), 1, LoadFile );
		}
	
		// Read the walls as a 6 byte array
		for (sidenum=0; sidenum<MAX_SIDES_PER_SEGMENT; sidenum++ )	{
			Segments[segnum].sides[sidenum].pad = 0;
		}

 		cfread( &bit_mask, sizeof(ubyte), 1, LoadFile );
		for (sidenum=0; sidenum<MAX_SIDES_PER_SEGMENT; sidenum++) {
			ubyte byte_wallnum;

			if (bit_mask & (1 << sidenum)) {
				cfread( &byte_wallnum, sizeof(ubyte), 1, LoadFile );
				if ( byte_wallnum == 255 )			
					Segments[segnum].sides[sidenum].wall_num = -1;
				else		
					Segments[segnum].sides[sidenum].wall_num = byte_wallnum;
			} else
					Segments[segnum].sides[sidenum].wall_num = -1;
		}

		for (sidenum=0; sidenum<MAX_SIDES_PER_SEGMENT; sidenum++ )	{

			if ( (Segments[segnum].children[sidenum]==-1) || (Segments[segnum].sides[sidenum].wall_num!=-1) )	{
				// Read short Segments[segnum].sides[sidenum].tmap_num;
				cfread( &temp_ushort, sizeof(ushort), 1, LoadFile );

				Segments[segnum].sides[sidenum].tmap_num = convert_tmap(temp_ushort & 0x7fff);

				if (!(temp_ushort & 0x8000))
					Segments[segnum].sides[sidenum].tmap_num2 = 0;
				else {
					// Read short Segments[segnum].sides[sidenum].tmap_num2;
					cfread( &Segments[segnum].sides[sidenum].tmap_num2, sizeof(short), 1, LoadFile );
					Segments[segnum].sides[sidenum].tmap_num2 =
					 (convert_tmap(Segments[segnum].sides[sidenum].tmap_num2 & 0x3fff)) |
					 (Segments[segnum].sides[sidenum].tmap_num2 & 0xc000);
				}

				// Read uvl Segments[segnum].sides[sidenum].uvls[4] (u,v>>5, write as short, l>>1 write as short)
				for (i=0; i<4; i++ )	{
					cfread( &temp_short, sizeof(short), 1, LoadFile );
					Segments[segnum].sides[sidenum].uvls[i].u = ((fix)temp_short) << 5;
					cfread( &temp_short, sizeof(short), 1, LoadFile );
					Segments[segnum].sides[sidenum].uvls[i].v = ((fix)temp_short) << 5;
					cfread( &temp_ushort, sizeof(temp_ushort), 1, LoadFile );
					Segments[segnum].sides[sidenum].uvls[i].l = ((fix)temp_ushort) << 1;
					//cfread( &Segments[segnum].sides[sidenum].uvls[i].l, sizeof(fix), 1, LoadFile );
				}	
			} else {
				Segments[segnum].sides[sidenum].tmap_num = 0;
				Segments[segnum].sides[sidenum].tmap_num2 = 0;
				for (i=0; i<4; i++ )	{
					Segments[segnum].sides[sidenum].uvls[i].u = 0;
					Segments[segnum].sides[sidenum].uvls[i].v = 0;
					Segments[segnum].sides[sidenum].uvls[i].l = 0;
				}	
			}
		}
	}

	if (lvl_version > 5) {
		for (segnum=0; segnum<Num_segments; segnum++ )	{
			// Read ubyte	Segments[segnum].special
			cfread( &Segments[segnum].special, sizeof(ubyte), 1, LoadFile );
			if (Segments[segnum].special >= MAX_CENTER_TYPES)
			    Segments[segnum].special = SEGMENT_IS_NOTHING; // remove goals etc.
			// Read byte	Segments[segnum].matcen_num
			cfread( &Segments[segnum].matcen_num, sizeof(ubyte), 1, LoadFile );
			// Read short	Segments[segnum].value
			cfread( &Segments[segnum].value, sizeof(short), 1, LoadFile );
			// Read fix	Segments[segnum].static_light (shift down 5 bits, write as short)
			cfread( &temp_ushort, sizeof(temp_ushort), 1, LoadFile );
			Segments[segnum].static_light	= ((fix)temp_ushort) << 3;
			// Read short ???
			cfread( &temp_ushort, sizeof(temp_ushort), 1, LoadFile );
			/*
			if (temp_ushort != 7)
				printf("%03d ",temp_ushort);
			*/
		}
	}

	Highest_vertex_index = Num_vertices-1;
	Highest_segment_index = Num_segments-1;

	validate_segment_all();			// Fill in side type and normals.

	// Activate fuelcentes
	for (i=0; i< Num_segments; i++ ) {
		fuelcen_activate( &Segments[i], Segments[i].special );
	}

	reset_objects(1);		//one object, the player

	return 0;
}

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

	// Read in game_top_fileinfo to get size of saved fileinfo.

	if (cfseek( LoadFile, start_offset, SEEK_SET )) 
		Error( "Error seeking in gamesave.c" ); 

	if (cfread( &game_top_fileinfo, sizeof(game_top_fileinfo), 1, LoadFile) != 1)
		Error( "Error reading game_top_fileinfo in gamesave.c" );

	// Check signature
	if (game_top_fileinfo.fileinfo_signature != 0x6705)
		return -1;

	// Check version number
	if (game_top_fileinfo.fileinfo_version < GAME_COMPATIBLE_VERSION )
		return -1;

	// Now, Read in the fileinfo
	if (cfseek( LoadFile, start_offset, SEEK_SET )) 
		Error( "Error seeking to game_fileinfo in gamesave.c" );

	game_fileinfo.fileinfo_signature = read_short(LoadFile);

	game_fileinfo.fileinfo_version = read_short(LoadFile);
	game_fileinfo.fileinfo_sizeof = read_int(LoadFile);
	for(i=0; i<15; i++)
		game_fileinfo.mine_filename[i] = read_byte(LoadFile);
	game_fileinfo.level = read_int(LoadFile);
	game_fileinfo.player_offset = read_int(LoadFile);				// Player info
	game_fileinfo.player_sizeof = read_int(LoadFile);
	game_fileinfo.object_offset = read_int(LoadFile);				// Object info
	game_fileinfo.object_howmany = read_int(LoadFile);    	
	game_fileinfo.object_sizeof = read_int(LoadFile);  
	game_fileinfo.walls_offset = read_int(LoadFile);
	game_fileinfo.walls_howmany = read_int(LoadFile);
	game_fileinfo.walls_sizeof = read_int(LoadFile);
	game_fileinfo.doors_offset = read_int(LoadFile);
	game_fileinfo.doors_howmany = read_int(LoadFile);
	game_fileinfo.doors_sizeof = read_int(LoadFile);
	game_fileinfo.triggers_offset = read_int(LoadFile);
	game_fileinfo.triggers_howmany = read_int(LoadFile);
	game_fileinfo.triggers_sizeof = read_int(LoadFile);
	game_fileinfo.links_offset = read_int(LoadFile);
	game_fileinfo.links_howmany = read_int(LoadFile);
	game_fileinfo.links_sizeof = read_int(LoadFile);
	game_fileinfo.control_offset = read_int(LoadFile);
	game_fileinfo.control_howmany = read_int(LoadFile);
	game_fileinfo.control_sizeof = read_int(LoadFile);
	game_fileinfo.matcen_offset = read_int(LoadFile);
	game_fileinfo.matcen_howmany = read_int(LoadFile);
	game_fileinfo.matcen_sizeof = read_int(LoadFile);
	if (game_top_fileinfo.fileinfo_version > 25)
	    cfseek(LoadFile,2*3*4,SEEK_CUR); // skip blastable light info
	if (game_top_fileinfo.fileinfo_version >= 14) { //load mine filename
		char *p=Current_level_name;
		//must do read one char at a time, since no cfgets()
		if (game_fileinfo.fileinfo_version > 25) {
			do *p = cfgetc(LoadFile); while ((*p++!=0x0a)&&(p-Current_level_name<sizeof(Current_level_name)));
		} else
			do *p = cfgetc(LoadFile); while (*p++!=0);
		p[-1] = 0;
	}
	else
		Current_level_name[0]=0;

	if (game_top_fileinfo.fileinfo_version >= 19) {	//load pof names
		cfread(&N_save_pof_names,2,1,LoadFile);
		if (N_save_pof_names != 0x614d && N_save_pof_names != 0x5547) { // "Ma"de w/DMB beta/"GU"ILE
			Assert(N_save_pof_names < MAX_POLYGON_MODELS_NEW);
			cfread(Save_pof_names,N_save_pof_names,13,LoadFile);
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

					if (cfread(&Walls[i], game_fileinfo.walls_sizeof, 1,LoadFile)!=1)
						Error( "Error reading Walls[%d] in gamesave.c", i);
					Walls[i].clip_num = convert_wclip(Walls[i].clip_num);
				}
				else if (game_top_fileinfo.fileinfo_version >= 17) {
					v19_wall w;

					Assert(sizeof(w) == game_fileinfo.walls_sizeof);

					if (cfread(&w, game_fileinfo.walls_sizeof, 1,LoadFile)!=1)
						Error( "Error reading Walls[%d] in gamesave.c", i);

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

					if (cfread(&w, game_fileinfo.walls_sizeof, 1,LoadFile)!=1)
						Error( "Error reading Walls[%d] in gamesave.c", i);

					Walls[i].segnum = Walls[i].sidenum = Walls[i].linked_wall = -1;

					Walls[i].type		= w.type;
					Walls[i].flags		= w.flags;
					Walls[i].hps		= w.hps;
					Walls[i].trigger	= w.trigger;
					Walls[i].clip_num	= w.clip_num % MAX_WALL_ANIMS;
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

					if (cfread(&ActiveDoors[i], game_fileinfo.doors_sizeof,1,LoadFile)!=1)
						Error( "Error reading ActiveDoors[%d] in gamesave.c", i);
				}
				else {
					v19_door d;
					int p;

					Assert(sizeof(d) == game_fileinfo.doors_sizeof);

					if (cfread(&d, game_fileinfo.doors_sizeof, 1,LoadFile)!=1)
						Error( "Error reading Doors[%d] in gamesave.c", i);

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
				//Assert( sizeof(Triggers[i]) == game_fileinfo.triggers_sizeof );
				//if (cfread(&Triggers[i], game_fileinfo.triggers_sizeof,1,LoadFile)!=1)
				//	Error( "Error reading Triggers[%d] in gamesave.c", i);
				if (game_top_fileinfo.fileinfo_version <= 25) {
					Triggers[i].type = read_byte(LoadFile);
					Triggers[i].flags = read_short(LoadFile);
					Triggers[i].value = read_int(LoadFile);
					Triggers[i].time = read_int(LoadFile);
					Triggers[i].link_num = read_byte(LoadFile);
					Triggers[i].num_links = read_short(LoadFile);
				} else {
					int type;
					switch (type = read_short(LoadFile)) {
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
							printf("Warning: unknown trigger type %d (%d)\n", type, i);
					}
					Triggers[i].num_links = read_short(LoadFile);
					Triggers[i].value = read_int(LoadFile);
					Triggers[i].time = read_int(LoadFile);
				}
				for (j=0; j<MAX_WALLS_PER_LINK; j++ )	
					Triggers[i].seg[j] = read_short(LoadFile);
				for (j=0; j<MAX_WALLS_PER_LINK; j++ )
					Triggers[i].side[j] = read_short(LoadFile);
			}
		}
	}

	//================ READ CONTROL CENTER TRIGGER INFO ===============

	if (game_fileinfo.control_offset > -1)
	{
		if (!cfseek( LoadFile, game_fileinfo.control_offset,SEEK_SET ))	{
			for (i=0;i<game_fileinfo.control_howmany;i++)
				if ( sizeof(ControlCenterTriggers) == game_fileinfo.control_sizeof )	{
                                        if ( cfread(&ControlCenterTriggers, game_fileinfo.control_sizeof,1,LoadFile)!=1)
                                                Error("Error reading ControlCenterTrigger %i in gamesave.c", i);
				} else {
					ControlCenterTriggers.num_links = read_short( LoadFile );
					for (j=0; j<MAX_WALLS_PER_LINK; j++ );
						ControlCenterTriggers.seg[j] = read_short( LoadFile );
					for (j=0; j<MAX_WALLS_PER_LINK; j++ );
						ControlCenterTriggers.side[j] = read_short( LoadFile );
				}
		}
	}


	//================ READ MATERIALOGRIFIZATIONATORS INFO ===============

	if (game_fileinfo.matcen_offset > -1)
	{	int	j;

		if (!cfseek( LoadFile, game_fileinfo.matcen_offset,SEEK_SET ))	{
			// mprintf((0, "Reading %i materialization centers.\n", game_fileinfo.matcen_howmany));
			for (i=0;i<game_fileinfo.matcen_howmany;i++) {
#if 0
				Assert( sizeof(RobotCenters[i]) == game_fileinfo.matcen_sizeof );
				if (cfread(&RobotCenters[i], game_fileinfo.matcen_sizeof,1,LoadFile)!=1)
					Error( "Error reading RobotCenters in gamesave.c", i);
#endif					
				RobotCenters[i].robot_flags = read_int(LoadFile);
				if (game_top_fileinfo.fileinfo_version > 25)
					/*RobotCenters[i].robot_flags2 =*/ read_int(LoadFile);
				RobotCenters[i].hit_points = read_fix(LoadFile);
				RobotCenters[i].interval = read_fix(LoadFile);
				RobotCenters[i].segnum = read_short(LoadFile);
				RobotCenters[i].fuelcen_num = read_short(LoadFile);
				//	Set links in RobotCenters to Station array
				for (j=0; j<=Highest_segment_index; j++)
					if (Segments[j].special == SEGMENT_IS_ROBOTMAKER)
						if (Segments[j].matcen_num == i)
							RobotCenters[i].fuelcen_num = Segments[j].value;

				// mprintf((0, "   %i: flags = %08x\n", i, RobotCenters[i].robot_flags));
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
				//mprintf((0, "Checking Wall %d\n", Segments[i].sides[j].wall_num));
				if (WallAnims[Walls[sidep->wall_num].clip_num].flags & WCF_TMAP1) {
					//mprintf((0, "Fixing non-transparent door.\n"));
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

        fix_object_segs();

	if (game_top_fileinfo.fileinfo_version < GAME_VERSION)
		return 1;		//means old version
	else
		return 0;
}

//loads a level (.RDL) file from disk
int load_level(char * filename_passed)
{
	CFILE * LoadFile;
	char filename[128];
	int sig,version,minedata_offset,gamedata_offset,hostagetext_offset;
	int mine_err,game_err;

	strcpy(filename,filename_passed);

	LoadFile = cfopen( filename, "rb" );
	if (!LoadFile)
		Error("Can't open file <%s>\n",filename);

	sig			= read_int(LoadFile);
	Assert(sig == 0x504c564c); /* 'PLVL' */

	version 		= read_int(LoadFile);
	minedata_offset		= read_int(LoadFile);
	gamedata_offset		= read_int(LoadFile);
	if (version == 1)
		hostagetext_offset	= read_int(LoadFile);
	else {
		char *p = Level_palette;
		do *p = cfgetc(LoadFile); while (*p++!=0x0a);
		if (read_int(LoadFile) != 0x1e)
			Error("rl2 int 1 != 0x1e");
		if (read_int(LoadFile) != -1)
			Error("rl2 int 2 != -1");
		
	}

	cfseek(LoadFile,minedata_offset,SEEK_SET);
	mine_err = load_mine_data_compiled_new(LoadFile, version);

	if (mine_err == -1)	//error!!
		return 1;

	cfseek(LoadFile,gamedata_offset,SEEK_SET);
	game_err = load_game_data(LoadFile);

	if (game_err == -1)	//error!!
		return 1;

	//======================== CLOSE FILE =============================

	cfclose( LoadFile );

        return 0;
}
