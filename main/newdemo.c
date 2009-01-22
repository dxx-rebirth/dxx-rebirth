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

#include <stdlib.h>
#include <stdio.h>
#include <string.h>	// for memset
#include <ctype.h>
#include <limits.h>
#include <errno.h>
#include "inferno.h"
#include "game.h"
#include "gr.h"
#include "stdlib.h"
#include "bm.h"
#include "3d.h"
#include "segment.h"
#include "texmap.h"
#include "laser.h"
#include "key.h"
#include "gameseg.h"
#include "object.h"
#include "physics.h"
#include "slew.h"
#include "render.h"
#include "wall.h"
#include "vclip.h"
#include "polyobj.h"
#include "fireball.h"
#include "laser.h"
#include "error.h"
#include "ai.h"
#include "hostage.h"
#include "morph.h"
#include "powerup.h"
#include "fuelcen.h"
#include "sounds.h"
#include "collide.h"
#include "lighting.h"
#include "newdemo.h"
#include "gameseq.h"
#include "gamesave.h"
#include "gamemine.h"
#include "switch.h"
#include "gauges.h"
#include "player.h"
#include "vecmat.h"
#include "newmenu.h"
#include "args.h"
#include "palette.h"
#include "multi.h"
#include "network.h"
#include "text.h"
#include "cntrlcen.h"
#include "aistruct.h"
#include "mission.h"
#include "piggy.h"
#include "byteswap.h"
#include "u_mem.h"
#include "physfsx.h"
#include "console.h"
#include "playsave.h"

#ifdef EDITOR
#include "editor/editor.h"
#endif

void DoJasonInterpolate (fix recorded_time);

#define ND_EVENT_EOF				0	// EOF
#define ND_EVENT_START_DEMO			1	// Followed by 16 character, NULL terminated filename of .SAV file to use
#define ND_EVENT_START_FRAME			2	// Followed by integer frame number, then a fix FrameTime
#define ND_EVENT_VIEWER_OBJECT			3	// Followed by an object structure
#define ND_EVENT_RENDER_OBJECT			4	// Followed by an object structure
#define ND_EVENT_SOUND				5	// Followed by int soundum
#define ND_EVENT_SOUND_ONCE			6	// Followed by int soundum
#define ND_EVENT_SOUND_3D			7	// Followed by int soundum, int angle, int volume
#define ND_EVENT_WALL_HIT_PROCESS		8	// Followed by int segnum, int side, fix damage 
#define ND_EVENT_TRIGGER			9	// Followed by int segnum, int side, int objnum
#define ND_EVENT_HOSTAGE_RESCUED 		10	// Followed by int hostage_type
#define ND_EVENT_SOUND_3D_ONCE			11	// Followed by int soundum, int angle, int volume
#define ND_EVENT_MORPH_FRAME			12	// Followed by ? data
#define ND_EVENT_WALL_TOGGLE			13	// Followed by int seg, int side
#define ND_EVENT_HUD_MESSAGE			14	// Followed by char size, char * string (+null)
#define ND_EVENT_CONTROL_CENTER_DESTROYED 	15	// Just a simple flag
#define ND_EVENT_PALETTE_EFFECT			16	// Followed by short r,g,b
#define ND_EVENT_PLAYER_ENERGY			17	// followed by byte energy
#define ND_EVENT_PLAYER_SHIELD			18	// followed by byte shields
#define ND_EVENT_PLAYER_FLAGS			19	// followed by player flags
#define ND_EVENT_PLAYER_WEAPON			20	// followed by weapon type and weapon number
#define ND_EVENT_EFFECT_BLOWUP			21	// followed by segment, side, and pnt
#define ND_EVENT_HOMING_DISTANCE		22	// followed by homing distance
#define ND_EVENT_LETTERBOX			23	// letterbox mode for death seq.
#define ND_EVENT_RESTORE_COCKPIT		24	// restore cockpit after death
#define ND_EVENT_REARVIEW			25	// going to rear view mode
#define ND_EVENT_WALL_SET_TMAP_NUM1		26	// Wall changed
#define ND_EVENT_WALL_SET_TMAP_NUM2		27	// Wall changed
#define ND_EVENT_NEW_LEVEL			28	// followed by level number
#define ND_EVENT_MULTI_CLOAK			29	// followed by player num
#define ND_EVENT_MULTI_DECLOAK			30	// followed by player num
#define ND_EVENT_RESTORE_REARVIEW		31	// restore cockpit after rearview mode

#ifndef SHAREWARE
#define ND_EVENT_MULTI_DEATH			32	// with player number
#define ND_EVENT_MULTI_KILL			33	// with player number
#define ND_EVENT_MULTI_CONNECT			34	// with player number
#define ND_EVENT_MULTI_RECONNECT		35	// with player number
#define ND_EVENT_MULTI_DISCONNECT		36	// with player number
#define ND_EVENT_MULTI_SCORE			37	// playernum / score
#define ND_EVENT_PLAYER_SCORE			38	// followed by score
#define ND_EVENT_PRIMARY_AMMO			39	// with old/new ammo count
#define ND_EVENT_SECONDARY_AMMO			40	// with old/new ammo count
#define ND_EVENT_DOOR_OPENING			41	// with segment/side
#endif
#define ND_EVENT_LASER_LEVEL			42	// with old/new level
#define ND_EVENT_LINK_SOUND_TO_OBJ  43  // record digi_link_sound_to_object3
#define ND_EVENT_KILL_SOUND_TO_OBJ  44  // record digi_kill_sound_linked_to_object

#define NORMAL_PLAYBACK 			0
#define SKIP_PLAYBACK				1
#define INTERPOLATE_PLAYBACK			2
#define INTERPOL_FACTOR				(F1_0 + (F1_0/5))

#ifdef SHAREWARE
#define DEMO_VERSION				5
#else
#define DEMO_VERSION				13
#endif

#define DEMO_FILENAME				DEMO_DIR "tmpdemo.dem"
#define DEMO_MAX_LEVELS				29

#ifdef SHAREWARE
#define DEMO_GAME_TYPE				1
#else
#define DEMO_GAME_TYPE				2
#endif

char nd_save_callsign[CALLSIGN_LEN+1];
int Newdemo_state = 0;
int Newdemo_vcr_state = 0;
int Newdemo_start_frame = -1;
int Newdemo_frame_number = -1;
unsigned int Newdemo_size;
int Newdemo_num_written;
int Newdemo_old_cockpit;
sbyte Newdemo_no_space;
sbyte Newdemo_at_eof;
sbyte Newdemo_do_interpolate = 1;
sbyte Newdemo_players_cloaked;
sbyte Newdemo_warning_given = 0;
sbyte Newdemo_cntrlcen_destroyed = 0;
static sbyte nd_bad_read;
int NewdemoFrameCount;
short frame_bytes_written = 0;
fix nd_playback_total;
fix nd_recorded_total;
fix nd_recorded_time;
sbyte playback_style;
sbyte First_time_playback=1;
fix JasonPlaybackTotal=0;
int Newdemo_show_percentage=1;

extern int digi_link_sound_to_object3( int org_soundnum, short objnum, int forever, fix max_volume, fix  max_distance, int loop_start, int loop_end );

PHYSFS_file *infile;
PHYSFS_file *outfile = NULL;

int newdemo_get_percent_done()	{
	if ( Newdemo_state == ND_STATE_PLAYBACK ) {
		return (PHYSFS_tell(infile) * 100) / Newdemo_size;
	}
	if ( Newdemo_state == ND_STATE_RECORDING ) {
		return PHYSFS_tell(outfile);
	}
	return 0;
}

#define VEL_PRECISION 12

void my_extract_shortpos(object *objp, shortpos *spp)
{
	int segnum;
	sbyte *sp;

	sp = spp->bytemat;
	objp->orient.rvec.x = *sp++ << MATRIX_PRECISION;
	objp->orient.uvec.x = *sp++ << MATRIX_PRECISION;
	objp->orient.fvec.x = *sp++ << MATRIX_PRECISION;

	objp->orient.rvec.y = *sp++ << MATRIX_PRECISION;
	objp->orient.uvec.y = *sp++ << MATRIX_PRECISION;
	objp->orient.fvec.y = *sp++ << MATRIX_PRECISION;

	objp->orient.rvec.z = *sp++ << MATRIX_PRECISION;
	objp->orient.uvec.z = *sp++ << MATRIX_PRECISION;
	objp->orient.fvec.z = *sp++ << MATRIX_PRECISION;

	segnum = spp->segment;
	objp->segnum = segnum;

	objp->pos.x = (spp->xo << RELPOS_PRECISION) + Vertices[Segments[segnum].verts[0]].x;
	objp->pos.y = (spp->yo << RELPOS_PRECISION) + Vertices[Segments[segnum].verts[0]].y;
	objp->pos.z = (spp->zo << RELPOS_PRECISION) + Vertices[Segments[segnum].verts[0]].z;

	objp->mtype.phys_info.velocity.x = (spp->velx << VEL_PRECISION);
	objp->mtype.phys_info.velocity.y = (spp->vely << VEL_PRECISION);
	objp->mtype.phys_info.velocity.z = (spp->velz << VEL_PRECISION);
}

int newdemo_read( void *buffer, int elsize, int nelem )
{
	int num_read;
	num_read = PHYSFS_read(infile, buffer, elsize, nelem);
	if (num_read < nelem || PHYSFS_eof(infile))
		nd_bad_read = -1;

	return num_read;
}

int newdemo_find_object( int signature )
{
	int i;
	object * objp;
	objp = Objects;
	for (i=0; i<=Highest_object_index; i++, objp++ ) {
		if ( (objp->type != OBJ_NONE) && (objp->signature == signature))
			return i;
	}
	return -1;
}

int newdemo_write( void *buffer, int elsize, int nelem )
{
	int num_written, total_size;

	total_size = elsize * nelem;
	frame_bytes_written += total_size;
	Newdemo_num_written += total_size;
	Assert(outfile != NULL);
	num_written = PHYSFS_write(outfile, buffer, elsize, nelem);
	//if ((Newdemo_num_written > Newdemo_size) && !Newdemo_no_space) {
	//	Newdemo_no_space=1;
	//	newdemo_stop_recording();
	//	return -1;
	//}
	if ((Newdemo_num_written > Newdemo_size) && !Newdemo_no_space)
		Newdemo_no_space=1;
	if (num_written == nelem && !Newdemo_no_space)
		return num_written;

	Newdemo_no_space=2;
	newdemo_stop_recording();
	return -1;
}

/*
 *  The next bunch of files taken from Matt's gamesave.c.  We have to modify
 *  these since the demo must save more information about objects that
 *  just a gamesave
*/

static void nd_write_byte(sbyte b)
{
	newdemo_write(&b, 1, 1);
}

static void nd_write_short(short s)
{
	newdemo_write(&s, 2, 1);
}

static void nd_write_int(int i)
{
	newdemo_write(&i, 4, 1);
}

static void nd_write_string(char *str)
{
	nd_write_byte(strlen(str) + 1);
	newdemo_write(str, strlen(str) + 1, 1);
}

static void nd_write_fix(fix f)
{
	newdemo_write(&f, sizeof(fix), 1);
}

static void nd_write_fixang(fixang f)
{
	newdemo_write(&f, sizeof(fixang), 1);
}

static void nd_write_vector(vms_vector *v)
{
	nd_write_fix(v->x);
	nd_write_fix(v->y);
	nd_write_fix(v->z);
}

static void nd_write_angvec(vms_angvec *v)
{
	nd_write_fixang(v->p);
	nd_write_fixang(v->b);
	nd_write_fixang(v->h);
}

void nd_write_shortpos(object *obj)
{
	int i;
	shortpos sp;
	ubyte render_type;

	create_shortpos(&sp, obj, 0);

	render_type = obj->render_type;
	if (((render_type == RT_POLYOBJ) || (render_type == RT_HOSTAGE) || (render_type == RT_MORPH)) || (obj->type == OBJ_CAMERA)) {
		for (i = 0; i < 9; i++)
			nd_write_byte(sp.bytemat[i]);
		for (i = 0; i < 9; i++) {
			if (sp.bytemat[i] != 0)
				break;
		}
		if (i == 9) {
			Int3();         // contact Allender about this.
		}
	}

	nd_write_short(sp.xo);
	nd_write_short(sp.yo);
	nd_write_short(sp.zo);
	nd_write_short(sp.segment);
	nd_write_short(sp.velx);
	nd_write_short(sp.vely);
	nd_write_short(sp.velz);
}

static void nd_read_byte(sbyte *b)
{
	newdemo_read(b, 1, 1);
}

static void nd_read_short(short *s)
{
	newdemo_read(s, 2, 1);
}

static void nd_read_int(int *i)
{
	newdemo_read(i, 4, 1);
}

static void nd_read_string(char *str)
{
	sbyte len;

	nd_read_byte(&len);
	newdemo_read(str, len, 1);
}

static void nd_read_fix(fix *f)
{
	newdemo_read(f, sizeof(fix), 1);
}

static void nd_read_fixang(fixang *f)
{
	newdemo_read(f, sizeof(fixang), 1);
}

static void nd_read_vector(vms_vector *v)
{
	nd_read_fix(&(v->x));
	nd_read_fix(&(v->y));
	nd_read_fix(&(v->z));
}

static void nd_read_angvec(vms_angvec *v)
{
	nd_read_fixang(&(v->p));
	nd_read_fixang(&(v->b));
	nd_read_fixang(&(v->h));
}

static void nd_read_shortpos(object *obj)
{
	shortpos sp;
	int i;
	ubyte render_type;

	memset(&sp,0,sizeof(shortpos));

	render_type = obj->render_type;
	if (((render_type == RT_POLYOBJ) || (render_type == RT_HOSTAGE) || (render_type == RT_MORPH)) || (obj->type == OBJ_CAMERA)) {
		for (i = 0; i < 9; i++)
			nd_read_byte(&(sp.bytemat[i]));
	}

	nd_read_short(&(sp.xo));
	nd_read_short(&(sp.yo));
	nd_read_short(&(sp.zo));
	nd_read_short(&(sp.segment));
	nd_read_short(&(sp.velx));
	nd_read_short(&(sp.vely));
	nd_read_short(&(sp.velz));

	my_extract_shortpos(obj, &sp);
	if ((obj->id == VCLIP_MORPHING_ROBOT) && (render_type == RT_FIREBALL) && (obj->control_type == CT_EXPLOSION))
		extract_orient_from_segment(&obj->orient,&Segments[obj->segnum]);

}

object *prev_obj=NULL;      //ptr to last object read in

void nd_read_object(object *obj)
{
	memset(obj, 0, sizeof(object));

	/*
	 * Do render type first, since with render_type == RT_NONE, we
	 * blow by all other object information
	 */
	nd_read_byte((sbyte *) &(obj->render_type));
	nd_read_byte((sbyte *) &(obj->type));
	if ((obj->render_type == RT_NONE) && (obj->type != OBJ_CAMERA))
		return;

	nd_read_byte((sbyte *) &(obj->id));
	nd_read_byte((sbyte *) &(obj->flags));
	nd_read_short((short *)&(obj->signature)); 

	    /* This doesn't make sense, since the object signature is supposed to 
	       be an int, but a short is probably half the size on many platforms.
	       The problem then is that probably only half the signature will be
	       initialized. This is definitely not portable.

	       See the typedef struct object's signature field in object.h to see
	       what I mean. The compiler warning with my gcc 4.2.3 seems to be 
	       warranted. 
	       
	       Kip (kip@thevertigo.com)
	    */

	nd_read_shortpos(obj);

	obj->attached_obj = -1;

	switch(obj->type) {

	case OBJ_HOSTAGE:
		obj->control_type = CT_POWERUP;
		obj->movement_type = MT_NONE;
		obj->size = HOSTAGE_SIZE;
		break;

	case OBJ_ROBOT:
		obj->control_type = CT_AI;
		obj->movement_type = MT_PHYSICS;
		obj->size = Polygon_models[Robot_info[obj->id].model_num].rad;
		obj->rtype.pobj_info.model_num = Robot_info[obj->id].model_num;
		obj->rtype.pobj_info.subobj_flags = 0;
		obj->ctype.ai_info.CLOAKED = (Robot_info[obj->id].cloak_type?1:0);
		break;

	case OBJ_POWERUP:
		obj->control_type = CT_POWERUP;
		nd_read_byte((sbyte *) &(obj->movement_type));        // might have physics movement
		obj->size = Powerup_info[obj->id].size;
		break;

	case OBJ_PLAYER:
		obj->control_type = CT_NONE;
		obj->movement_type = MT_PHYSICS;
		obj->size = Polygon_models[Player_ship->model_num].rad;
		obj->rtype.pobj_info.model_num = Player_ship->model_num;
		obj->rtype.pobj_info.subobj_flags = 0;
		break;

	case OBJ_CLUTTER:
		obj->control_type = CT_NONE;
		obj->movement_type = MT_NONE;
		obj->size = Polygon_models[obj->id].rad;
		obj->rtype.pobj_info.model_num = obj->id;
		obj->rtype.pobj_info.subobj_flags = 0;
		break;

	default:
		nd_read_byte((sbyte *) &(obj->control_type));
		nd_read_byte((sbyte *) &(obj->movement_type));
		nd_read_fix(&(obj->size));
		break;
	}


	nd_read_vector(&(obj->last_pos));
	if ((obj->type == OBJ_WEAPON) && (obj->render_type == RT_WEAPON_VCLIP))
		nd_read_fix(&(obj->lifeleft));
	else {
		sbyte b;

		nd_read_byte(&b);
		obj->lifeleft = (fix)b;
		// MWA old way -- won't work with big endian machines       nd_read_byte((ubyte *)&(obj->lifeleft));
		obj->lifeleft = (fix)((int)obj->lifeleft << 12);
	}

	if (obj->type == OBJ_ROBOT) {
		if (Robot_info[obj->id].boss_flag) {
			sbyte cloaked;

			nd_read_byte(&cloaked);
			obj->ctype.ai_info.CLOAKED = cloaked;
		}
	}

	switch (obj->movement_type) {

	case MT_PHYSICS:
		nd_read_vector(&(obj->mtype.phys_info.velocity));
		nd_read_vector(&(obj->mtype.phys_info.thrust));
		break;

	case MT_SPINNING:
		nd_read_vector(&(obj->mtype.spin_rate));
		break;

	case MT_NONE:
		break;

	default:
		Int3();
	}

	switch (obj->control_type) {

	case CT_EXPLOSION:

		nd_read_fix(&(obj->ctype.expl_info.spawn_time));
		nd_read_fix(&(obj->ctype.expl_info.delete_time));
		nd_read_short(&(obj->ctype.expl_info.delete_objnum));

		obj->ctype.expl_info.next_attach = obj->ctype.expl_info.prev_attach = obj->ctype.expl_info.attach_parent = -1;

		if (obj->flags & OF_ATTACHED) {     //attach to previous object
			Assert(prev_obj!=NULL);
			if (prev_obj->control_type == CT_EXPLOSION) {
				if (prev_obj->flags & OF_ATTACHED && prev_obj->ctype.expl_info.attach_parent!=-1)
					obj_attach(&Objects[prev_obj->ctype.expl_info.attach_parent],obj);
				else
					obj->flags &= ~OF_ATTACHED;
			}
			else
				obj_attach(prev_obj,obj);
		}

		break;

	case CT_LIGHT:
		nd_read_fix(&(obj->ctype.light_info.intensity));
		break;

	case CT_AI:
	case CT_WEAPON:
	case CT_NONE:
	case CT_FLYING:
	case CT_DEBRIS:
	case CT_POWERUP:
	case CT_SLEW:
	case CT_CNTRLCEN:
	case CT_REMOTE:
	case CT_MORPH:
		break;

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
		int i, tmo;

		if ((obj->type != OBJ_ROBOT) && (obj->type != OBJ_PLAYER) && (obj->type != OBJ_CLUTTER)) {
			nd_read_int(&(obj->rtype.pobj_info.model_num));
			nd_read_int(&(obj->rtype.pobj_info.subobj_flags));
		}

		if ((obj->type != OBJ_PLAYER) && (obj->type != OBJ_DEBRIS))
#if 0
			for (i=0;i<MAX_SUBMODELS;i++)
				nd_read_angvec(&(obj->pobj_info.anim_angles[i]));
#endif
		for (i = 0; i < Polygon_models[obj->rtype.pobj_info.model_num].n_models; i++)
			nd_read_angvec(&obj->rtype.pobj_info.anim_angles[i]);

		nd_read_int(&tmo);

#ifndef EDITOR
		obj->rtype.pobj_info.tmap_override = tmo;
#else
		if (tmo==-1)
			obj->rtype.pobj_info.tmap_override = -1;
		else {
			int xlated_tmo = tmap_xlate_table[tmo];
			if (xlated_tmo < 0) {
				Int3();
				xlated_tmo = 0;
			}
			obj->rtype.pobj_info.tmap_override = xlated_tmo;
		}
#endif

		break;
	}

	case RT_POWERUP:
	case RT_WEAPON_VCLIP:
	case RT_FIREBALL:
	case RT_HOSTAGE:
		nd_read_int(&(obj->rtype.vclip_info.vclip_num));
		nd_read_fix(&(obj->rtype.vclip_info.frametime));
		nd_read_byte(&(obj->rtype.vclip_info.framenum));
		break;

	case RT_LASER:
		break;

	default:
		Int3();

	}

	prev_obj = obj;
}

void nd_write_object(object *obj)
{
	int life;

	/*
	 * Do render_type first so on read, we can make determination of
	 * what else to read in
	 */
	nd_write_byte(obj->render_type);
	nd_write_byte(obj->type);
	if ((obj->render_type == RT_NONE) && (obj->type != OBJ_CAMERA))
		return;

	nd_write_byte(obj->id);
	nd_write_byte(obj->flags);
	nd_write_short((short)obj->signature);
	nd_write_shortpos(obj);

	if ((obj->type != OBJ_HOSTAGE) && (obj->type != OBJ_ROBOT) && (obj->type != OBJ_PLAYER) && (obj->type != OBJ_POWERUP) && (obj->type != OBJ_CLUTTER)) {
		nd_write_byte(obj->control_type);
		nd_write_byte(obj->movement_type);
		nd_write_fix(obj->size);
	}
	if (obj->type == OBJ_POWERUP)
		nd_write_byte(obj->movement_type);

	nd_write_vector(&obj->last_pos);

	if ((obj->type == OBJ_WEAPON) && (obj->render_type == RT_WEAPON_VCLIP))
		nd_write_fix(obj->lifeleft);
	else {
		life = (int)obj->lifeleft;
		life = life >> 12;
		if (life > 255)
			life = 255;
		nd_write_byte((ubyte)life);
	}

	if (obj->type == OBJ_ROBOT) {
		if (Robot_info[obj->id].boss_flag) {
			if ((GameTime > Boss_cloak_start_time) && (GameTime < Boss_cloak_end_time))
				nd_write_byte(1);
			else
				nd_write_byte(0);
		}
	}

	switch (obj->movement_type) {

	case MT_PHYSICS:
		nd_write_vector(&obj->mtype.phys_info.velocity);
		nd_write_vector(&obj->mtype.phys_info.thrust);
		break;

	case MT_SPINNING:
		nd_write_vector(&obj->mtype.spin_rate);
		break;

	case MT_NONE:
		break;

	default:
		Int3();
	}

	switch (obj->control_type) {

	case CT_AI:
		break;

	case CT_EXPLOSION:
		nd_write_fix(obj->ctype.expl_info.spawn_time);
		nd_write_fix(obj->ctype.expl_info.delete_time);
		nd_write_short(obj->ctype.expl_info.delete_objnum);
		break;

	case CT_WEAPON:
		break;

	case CT_LIGHT:

		nd_write_fix(obj->ctype.light_info.intensity);
		break;

	case CT_NONE:
	case CT_FLYING:
	case CT_DEBRIS:
	case CT_POWERUP:
	case CT_SLEW:       //the player is generally saved as slew
	case CT_CNTRLCEN:
	case CT_REMOTE:
	case CT_MORPH:
		break;

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

		if ((obj->type != OBJ_ROBOT) && (obj->type != OBJ_PLAYER) && (obj->type != OBJ_CLUTTER)) {
			nd_write_int(obj->rtype.pobj_info.model_num);
			nd_write_int(obj->rtype.pobj_info.subobj_flags);
		}

		if ((obj->type != OBJ_PLAYER) && (obj->type != OBJ_DEBRIS))
#if 0
			for (i=0;i<MAX_SUBMODELS;i++)
				nd_write_angvec(&obj->pobj_info.anim_angles[i]);
#endif
		for (i = 0; i < Polygon_models[obj->rtype.pobj_info.model_num].n_models; i++)
			nd_write_angvec(&obj->rtype.pobj_info.anim_angles[i]);

		nd_write_int(obj->rtype.pobj_info.tmap_override);

		break;
	}

	case RT_POWERUP:
	case RT_WEAPON_VCLIP:
	case RT_FIREBALL:
	case RT_HOSTAGE:
		nd_write_int(obj->rtype.vclip_info.vclip_num);
		nd_write_fix(obj->rtype.vclip_info.frametime);
		nd_write_byte(obj->rtype.vclip_info.framenum);
		break;

	case RT_LASER:
		break;

	default:
		Int3();

	}

}

int RecordFrame=1;
static fix last_rec_time=0;
#define REC_DELAY F1_0/20

void newdemo_record_start_demo()
{
#ifndef SHAREWARE
	int i;
#endif

	last_rec_time=GameTime-REC_DELAY; // make sure first frame is recorded!

	stop_time();
	nd_write_byte(ND_EVENT_START_DEMO);
	nd_write_byte(DEMO_VERSION);
	nd_write_byte(DEMO_GAME_TYPE);
	nd_write_fix(GameTime);
	if (Game_mode & GM_MULTI)
		nd_write_int(Game_mode | (Player_num << 16));
	else
		nd_write_int(Game_mode);

#ifdef NETWORK
#ifdef SHAREWARE
	if (Game_mode & GM_MULTI)
		nd_write_byte(Netgame.team_vector);
#else
	if (Game_mode & GM_TEAM) {
		nd_write_byte(Netgame.team_vector);
		nd_write_string(Netgame.team_name[0]);
		nd_write_string(Netgame.team_name[1]);
	}
#endif
#endif

#ifndef SHAREWARE

	if (Game_mode & GM_MULTI) {
		nd_write_byte((sbyte)N_players);
		for (i = 0; i < N_players; i++) {
			nd_write_string(Players[i].callsign);
			nd_write_byte(Players[i].connected);

			if (Game_mode & GM_MULTI_COOP) {
				nd_write_int(Players[i].score);
			} else {
				nd_write_short((short)Players[i].net_killed_total);
				nd_write_short((short)Players[i].net_kills_total);
			}
		}
	} else
		nd_write_int(Players[Player_num].score);

	for (i = 0; i < MAX_PRIMARY_WEAPONS; i++)
		nd_write_short((short)Players[Player_num].primary_ammo[i]);

	for (i = 0; i < MAX_SECONDARY_WEAPONS; i++)
		nd_write_short((short)Players[Player_num].secondary_ammo[i]);

	nd_write_byte((sbyte)Players[Player_num].laser_level);

//  Support for missions added here

	nd_write_string(Current_mission_filename);

#endif

	nd_write_byte((sbyte)(f2ir(Players[Player_num].energy)));
	nd_write_byte((sbyte)(f2ir(Players[Player_num].shields)));
	nd_write_int(Players[Player_num].flags); // be sure players flags are set
	nd_write_byte((sbyte)Primary_weapon);
	nd_write_byte((sbyte)Secondary_weapon);
	Newdemo_start_frame = Newdemo_frame_number = 0;
	newdemo_set_new_level(Current_level_num);
	start_time();

}

void newdemo_record_start_frame(fix frame_time )
{
	if (Newdemo_no_space) {
		newdemo_stop_playback();
		return;
	}

	// Make demo recording waste a bit less space.
	// First check if if at least REC_DELAY has passed since last recorded frame. If yes, record frame and set RecordFrame true.
	// RecordFrame will be used for various other frame-by-frame events to drop some unnecessary bytes.
	// frame_time must be modified to get the right playback speed.
	if (last_rec_time > GameTime)
		last_rec_time=GameTime-REC_DELAY;

	if (last_rec_time + REC_DELAY <= GameTime || frame_time >= REC_DELAY)
	{
		last_rec_time = GameTime-(GameTime-(last_rec_time + REC_DELAY));
		if (frame_time < REC_DELAY)
			frame_time = REC_DELAY;
		RecordFrame=1;
	
		stop_time();
		Newdemo_frame_number -= Newdemo_start_frame;

		nd_write_byte(ND_EVENT_START_FRAME);
		nd_write_short(frame_bytes_written - 1); // from previous frame
		frame_bytes_written=3;
		nd_write_int(Newdemo_frame_number);
		Newdemo_frame_number++;
		nd_write_int(frame_time);
		start_time();
	}
	else
	{
		RecordFrame=0;
	}
}

void newdemo_record_render_object(object * obj)
{
	if (!RecordFrame)
		return;
	stop_time();
	nd_write_byte(ND_EVENT_RENDER_OBJECT);
	nd_write_object(obj);
	start_time();
}

void newdemo_record_viewer_object(object * obj)
{
	if (!RecordFrame)
		return;
	stop_time();
	nd_write_byte(ND_EVENT_VIEWER_OBJECT);
	nd_write_object(obj);
	start_time();
}

void newdemo_record_sound( int soundno )	{
	stop_time();
	nd_write_byte(ND_EVENT_SOUND);
	nd_write_int( soundno );
	start_time();
}

void newdemo_record_sound_3d( int soundno, int angle, int volume )	{
	stop_time();
	nd_write_byte( ND_EVENT_SOUND_3D );
	nd_write_int( soundno );
	nd_write_int( angle );
	nd_write_int( volume );
	start_time();
}

void newdemo_record_sound_3d_once( int soundno, int angle, int volume )	{
	stop_time();
	nd_write_byte( ND_EVENT_SOUND_3D_ONCE );
	nd_write_int( soundno );
	nd_write_int( angle );
	nd_write_int( volume );
	start_time();
}

void newdemo_record_link_sound_to_object3( int soundno, short objnum, fix max_volume, fix  max_distance, int loop_start, int loop_end )
{
	stop_time();
	nd_write_byte( ND_EVENT_LINK_SOUND_TO_OBJ );
	nd_write_int( soundno );
	nd_write_int( Objects[objnum].signature );
	nd_write_int( max_volume );
	nd_write_int( max_distance );
	nd_write_int( loop_start );
	nd_write_int( loop_end );
	start_time();
}

void newdemo_record_kill_sound_linked_to_object( int objnum )
{
	stop_time();
	nd_write_byte( ND_EVENT_KILL_SOUND_TO_OBJ );
	nd_write_int( Objects[objnum].signature );
	start_time();
}

void newdemo_record_wall_hit_process( int segnum, int side, int damage, int playernum )
{
	stop_time();
	nd_write_byte( ND_EVENT_WALL_HIT_PROCESS );
	nd_write_int( segnum );
	nd_write_int( side );
	nd_write_int( damage );
	nd_write_int( playernum );
	start_time();
}

void newdemo_record_trigger( int segnum, int side, int objnum )
{
	stop_time();
	nd_write_byte( ND_EVENT_TRIGGER );
	nd_write_int( segnum );
	nd_write_int( side );
	nd_write_int( objnum );
	start_time();
}

void newdemo_record_hostage_rescued( int hostage_number )	{
	stop_time();
	nd_write_byte( ND_EVENT_HOSTAGE_RESCUED );
	nd_write_int( hostage_number );
	start_time();
}

void newdemo_record_morph_frame(morph_data *md)	{
	if (!RecordFrame)
		return;
	stop_time();
	nd_write_byte( ND_EVENT_MORPH_FRAME );
	nd_write_object( md->obj );
	start_time();
}

void newdemo_record_wall_toggle( int segnum, int side )	{
	stop_time();
	nd_write_byte( ND_EVENT_WALL_TOGGLE );
	nd_write_int( segnum );
	nd_write_int( side );
	start_time();
}

void newdemo_record_control_center_destroyed()
{
	stop_time();
	nd_write_byte( ND_EVENT_CONTROL_CENTER_DESTROYED );
	nd_write_int( Fuelcen_seconds_left );
	start_time();
}

void newdemo_record_hud_message( char * message )
{
	stop_time();
	nd_write_byte( ND_EVENT_HUD_MESSAGE );
	nd_write_string(message);
	start_time();
}

void newdemo_record_palette_effect(short r, short g, short b )
{
	if (!RecordFrame)
		return;
	stop_time();
	nd_write_byte( ND_EVENT_PALETTE_EFFECT );
	nd_write_short( r );
	nd_write_short( g );
	nd_write_short( b );
	start_time();
}

#ifdef SHAREWARE
void newdemo_record_player_energy(int energy)
#else
void newdemo_record_player_energy(int old_energy, int energy)
#endif
{
	stop_time();
	nd_write_byte( ND_EVENT_PLAYER_ENERGY );
#ifndef SHAREWARE
	nd_write_byte((sbyte) old_energy);
#endif
	nd_write_byte((sbyte) energy);
	start_time();
}

#ifdef SHAREWARE
void newdemo_record_player_shields(int shield)
#else
void newdemo_record_player_shields(int old_shield, int shield)
#endif
{
	stop_time();
	nd_write_byte( ND_EVENT_PLAYER_SHIELD );
#ifndef SHAREWARE
	nd_write_byte((sbyte)old_shield);
#endif
	nd_write_byte((sbyte)shield);
	start_time();
}

void newdemo_record_player_flags(uint oflags, uint flags)
{
	stop_time();
	nd_write_byte( ND_EVENT_PLAYER_FLAGS );
	nd_write_int(((short)oflags << 16) | (short)flags);
	start_time();
}

void newdemo_record_player_weapon(int weapon_type, int weapon_num)
{
	stop_time();
	nd_write_byte( ND_EVENT_PLAYER_WEAPON );
	nd_write_byte((sbyte)weapon_type);
	nd_write_byte((sbyte)weapon_num);
#ifndef SHAREWARE
	if (weapon_type)
		nd_write_byte((sbyte)Secondary_weapon);
	else
		nd_write_byte((sbyte)Primary_weapon);
#endif
	start_time();
}

void newdemo_record_effect_blowup(short segment, int side, vms_vector *pnt)
{
	stop_time();
	nd_write_byte (ND_EVENT_EFFECT_BLOWUP);
	nd_write_short(segment);
	nd_write_byte((sbyte)side);
	nd_write_vector(pnt);
	start_time();
}

void newdemo_record_homing_distance(fix distance)
{
	stop_time();
	nd_write_byte(ND_EVENT_HOMING_DISTANCE);
	nd_write_short((short)(distance>>16));
	start_time();
}

void newdemo_record_letterbox(void)
{
	stop_time();
	nd_write_byte(ND_EVENT_LETTERBOX);
	start_time();
}

void newdemo_record_rearview(void)
{
	stop_time();
	nd_write_byte(ND_EVENT_REARVIEW);
	start_time();
}

void newdemo_record_restore_cockpit(void)
{
	stop_time();
	nd_write_byte(ND_EVENT_RESTORE_COCKPIT);
	start_time();
}

void newdemo_record_restore_rearview(void)
{
	stop_time();
	nd_write_byte(ND_EVENT_RESTORE_REARVIEW);
	start_time();
}

void newdemo_record_wall_set_tmap_num1(short seg,ubyte side,short cseg,ubyte cside,short tmap)
{
	stop_time();
	nd_write_byte(ND_EVENT_WALL_SET_TMAP_NUM1);
	nd_write_short(seg);
	nd_write_byte(side);
	nd_write_short(cseg);
	nd_write_byte(cside);
	nd_write_short(tmap);
	start_time();
}

void newdemo_record_wall_set_tmap_num2(short seg,ubyte side,short cseg,ubyte cside,short tmap)
{
	stop_time();
	nd_write_byte(ND_EVENT_WALL_SET_TMAP_NUM2);
	nd_write_short(seg);
	nd_write_byte(side);
	nd_write_short(cseg);
	nd_write_byte(cside);
	nd_write_short(tmap);
	start_time();
}

void newdemo_record_multi_cloak(int pnum)
{
	stop_time();
	nd_write_byte(ND_EVENT_MULTI_CLOAK);
	nd_write_byte((sbyte)pnum);
	start_time();
}

void newdemo_record_multi_decloak(int pnum)
{
	stop_time();
	nd_write_byte(ND_EVENT_MULTI_DECLOAK);
	nd_write_byte((sbyte)pnum);
	start_time();
}

#ifndef SHAREWARE

void newdemo_record_multi_death(int pnum)
{
	stop_time();
	nd_write_byte(ND_EVENT_MULTI_DEATH);
	nd_write_byte((sbyte)pnum);
	start_time();
}

void newdemo_record_multi_kill(int pnum, sbyte kill)
{
	stop_time();
	nd_write_byte(ND_EVENT_MULTI_KILL);
	nd_write_byte((sbyte)pnum);
	nd_write_byte(kill);
	start_time();
}

void newdemo_record_multi_connect(int pnum, int new_player, char *new_callsign)
{
	stop_time();
	nd_write_byte(ND_EVENT_MULTI_CONNECT);
	nd_write_byte((sbyte)pnum);
	nd_write_byte((sbyte)new_player);
	if (!new_player) {
		nd_write_string(Players[pnum].callsign);
		nd_write_int(Players[pnum].net_killed_total);
		nd_write_int(Players[pnum].net_kills_total);
	}
	nd_write_string(new_callsign);
	start_time();
}

void newdemo_record_multi_reconnect(int pnum)
{
	stop_time();
	nd_write_byte(ND_EVENT_MULTI_RECONNECT);
	nd_write_byte((sbyte)pnum);
	start_time();
}

void newdemo_record_multi_disconnect(int pnum)
{
	stop_time();
	nd_write_byte(ND_EVENT_MULTI_DISCONNECT);
	nd_write_byte((sbyte)pnum);
	start_time();
}

void newdemo_record_player_score(int score)
{
	stop_time();
	nd_write_byte(ND_EVENT_PLAYER_SCORE);
	nd_write_int(score);
	start_time();
}

void newdemo_record_multi_score(int pnum, int score)
{
	stop_time();
	nd_write_byte(ND_EVENT_MULTI_SCORE);
	nd_write_byte((sbyte)pnum);
	nd_write_int(score - Players[pnum].score); // called before score is changed!!!!
	start_time();
}

void newdemo_record_primary_ammo(int old_ammo, int new_ammo)
{
	if (!RecordFrame)
		return;
	stop_time();
	nd_write_byte(ND_EVENT_PRIMARY_AMMO);
	if (old_ammo < 0)
		nd_write_short((short)new_ammo);
	else
		nd_write_short((short)old_ammo);
	nd_write_short((short)new_ammo);
	start_time();
}

void newdemo_record_secondary_ammo(int old_ammo, int new_ammo)
{
	if (!RecordFrame)
		return;
	stop_time();
	nd_write_byte(ND_EVENT_SECONDARY_AMMO);
	if (old_ammo < 0)
		nd_write_short((short)new_ammo);
	else
		nd_write_short((short)old_ammo);
	nd_write_short((short)new_ammo);
	start_time();
}

void newdemo_record_door_opening(int segnum, int side)
{
	stop_time();
	nd_write_byte(ND_EVENT_DOOR_OPENING);
	nd_write_short((short)segnum);
	nd_write_byte((sbyte)side);
	start_time();
}

#endif

void newdemo_record_laser_level(sbyte old_level, sbyte new_level)
{
        stop_time();
        nd_write_byte(ND_EVENT_LASER_LEVEL);
        nd_write_byte(old_level);
        nd_write_byte(new_level);
        start_time();
}


void newdemo_set_new_level(int level_num)
{
	stop_time();
	nd_write_byte(ND_EVENT_NEW_LEVEL);
	nd_write_byte((sbyte)level_num);
	nd_write_byte((sbyte)Current_level_num);
	start_time();
}

int newdemo_read_demo_start(int rnd_demo)
{
        sbyte version=0, game_type=0, c=0;
	ubyte energy=0, shield=0;
        char text[128];
        sbyte i=0, laser_level=0;
        char current_mission[9];

	Rear_view=0;

	nd_read_byte((sbyte *)&c);
	if ((c != ND_EVENT_START_DEMO) || nd_bad_read) {
		newmenu_item m[1];

		sprintf(text, "%s %s", TXT_CANT_PLAYBACK, TXT_DEMO_CORRUPT);
		m[ 0].type = NM_TYPE_TEXT; m[ 0].text = text;
		newmenu_do( NULL, NULL, sizeof(m)/sizeof(*m), m, NULL );
		return 1;
	}
	nd_read_byte(&version);
	if (version < DEMO_VERSION) {
		if (!rnd_demo) {
			newmenu_item m[1];
			sprintf(text, "%s %s", TXT_CANT_PLAYBACK, TXT_DEMO_OLD);
			m[ 0].type = NM_TYPE_TEXT; m[ 0].text = text;
			newmenu_do( NULL, NULL, sizeof(m)/sizeof(*m), m, NULL );
		}
		return 1;
	}
	nd_read_byte(&game_type);
	if (game_type != DEMO_GAME_TYPE) {
		newmenu_item m[3];

		sprintf(text, "%s %s", TXT_CANT_PLAYBACK, TXT_RECORDED);
		m[ 0].type = NM_TYPE_TEXT; m[ 0].text = text;
#ifdef SHAREWARE
		m[ 1].type = NM_TYPE_TEXT; m[ 1].text = TXT_WITH_REGISTERED;
#else
		m[ 1].type = NM_TYPE_TEXT; m[ 1].text = TXT_WITH_SHAREWARE;
#endif
		m[ 2].type = NM_TYPE_TEXT; m[ 2].text = TXT_OF_DESCENT;
	
		newmenu_do( NULL, NULL, sizeof(m)/sizeof(*m), m, NULL );
		return 1;
	}
	nd_read_fix(&GameTime);
	nd_read_int(&Game_mode);
JasonPlaybackTotal=0;
#ifndef NETWORK
	if (Game_mode & GM_MULTI) {
		nm_messagebox( NULL, 1, "Ok", "can't playback net game\nwith this version of code\n" );
		return 1;
	}
#endif

#ifdef NETWORK
	change_playernum_to((Game_mode >> 16) & 0x7);
#ifdef SHAREWARE
	if (Game_mode & GM_TEAM)
		nd_read_byte((sbyte *)&(Netgame.team_vector));

	for (i =0 ; i < MAX_PLAYERS; i++) {
 		Players[i].cloak_time = 0;
 		Players[i].invulnerable_time = 0;
	}
#else
	if (Game_mode & GM_TEAM) {
		nd_read_byte((sbyte *)&(Netgame.team_vector));
		nd_read_string(Netgame.team_name[0]);
		nd_read_string(Netgame.team_name[1]);
	}
	if (Game_mode & GM_MULTI) {

		multi_new_game();
		nd_read_byte((sbyte *)&N_players);
		for (i = 0 ; i < N_players; i++) {
			Players[i].cloak_time = 0;
			Players[i].invulnerable_time = 0;
			nd_read_string(Players[i].callsign);
			nd_read_byte(&(Players[i].connected));

			if (Game_mode & GM_MULTI_COOP) {
				nd_read_int(&(Players[i].score));
			} else {
				nd_read_short((short *)&(Players[i].net_killed_total));
				nd_read_short((short *)&(Players[i].net_kills_total));
			}
		}
		multi_sort_kill_list();
	} else
#endif
#endif	//
		nd_read_int(&(Players[Player_num].score)); // Note link to above if!

	for (i = 0; i < MAX_PRIMARY_WEAPONS; i++)
		nd_read_short((short*)&(Players[Player_num].primary_ammo[i]));

	for (i = 0; i < MAX_SECONDARY_WEAPONS; i++)
			nd_read_short((short*)&(Players[Player_num].secondary_ammo[i]));

	nd_read_byte(&laser_level);
	if (laser_level != Players[Player_num].laser_level) {
		Players[Player_num].laser_level = laser_level;
		update_laser_weapon_info();
	}

// Support for missions

	nd_read_string(current_mission);
#ifndef SHAREWARE
#ifdef DEST_SAT
	if (!strcmp(current_mission, ""))
		strcpy(current_mission, "DESTSAT");
#endif
	if (!load_mission_by_name(current_mission)) {
		newmenu_item m[1];

		sprintf(text, TXT_NOMISSION4DEMO, current_mission);
		m[ 0].type = NM_TYPE_TEXT; m[ 0].text = text;
		newmenu_do( NULL, NULL, sizeof(m)/sizeof(*m), m, NULL );
		return 1;
	}
#endif

//#endif

	nd_recorded_total = 0;
	nd_playback_total = 0;
	nd_read_byte((sbyte *)&energy);
	nd_read_byte((sbyte *)&shield);

	nd_read_int((int *)&(Players[Player_num].flags));
	if (Players[Player_num].flags & PLAYER_FLAGS_CLOAKED) {
		Players[Player_num].cloak_time = GameTime - (CLOAK_TIME_MAX / 2);
		Newdemo_players_cloaked |= (1 << Player_num);
	}
	if (Players[Player_num].flags & PLAYER_FLAGS_INVULNERABLE)
		Players[Player_num].invulnerable_time = GameTime - (INVULNERABLE_TIME_MAX / 2);

	nd_read_byte((sbyte *)&Primary_weapon);
	nd_read_byte((sbyte *)&Secondary_weapon);

// Next bit of code to fix problem that I introduced between 1.0 and 1.1
// check the next byte -- it _will_ be a load_new_level event.  If it is
// not, then we must shift all bytes up by one.

#ifdef SHAREWARE
	{
		unsigned char c;

		c = fgetc(infile);
		if (c != ND_EVENT_NEW_LEVEL) {
			int flags;

			flags = Players[Player_num].flags;
			energy = shield;
			shield = (unsigned char)flags;
			flags = (flags >> 8) & 0x00ffffff;
			flags |= (Primary_weapon << 24);
			Primary_weapon = Secondary_weapon;
			Secondary_weapon = c;
		} else
			ungetc(c, infile);
	}
#endif

	Players[Player_num].energy = i2f(energy);
	Players[Player_num].shields = i2f(shield);
	return 0;
}

void newdemo_pop_ctrlcen_triggers()
{
	int anim_num, n, i;
	int side, cside;
	segment *seg, *csegp;

	for (i = 0; i < ControlCenterTriggers.num_links; i++)	{
		seg = &Segments[ControlCenterTriggers.seg[i]];
		side = ControlCenterTriggers.side[i];
		csegp = &Segments[seg->children[side]];
		cside = find_connect_side(seg, csegp);
		anim_num = Walls[seg->sides[side].wall_num].clip_num;
		n = WallAnims[anim_num].num_frames;
		if (WallAnims[anim_num].flags & WCF_TMAP1)	{
		seg->sides[side].tmap_num = csegp->sides[cside].tmap_num = WallAnims[anim_num].frames[n-1];
		} else {
			seg->sides[side].tmap_num2 = csegp->sides[cside].tmap_num2 = WallAnims[anim_num].frames[n-1];
		}
	}
}

#define N_PLAYER_SHIP_TEXTURES 6


int newdemo_read_frame_information()
{
	int done, segnum, side, objnum, soundno, angle, volume, i;
	object *obj;
	ubyte c;
	static sbyte saved_letter_cockpit;
	static sbyte saved_rearview_cockpit;

	done = 0;

	if (Newdemo_vcr_state != ND_STATE_PAUSED)
		for (segnum=0; segnum <= Highest_segment_index; segnum++)
			Segments[segnum].objects = -1;

	reset_objects(1);
	Players[Player_num].homing_object_dist = -F1_0;

	prev_obj = NULL;

	while( !done )	{
		nd_read_byte((sbyte *)&c);
		if (nd_bad_read) { done = -1; break; }

		switch( c )	{

		case ND_EVENT_START_FRAME:	{ // Followed by an integer frame number, then a fix FrameTime
			short last_frame_length;

			done=1;
			nd_read_short(&last_frame_length);
			nd_read_int(&NewdemoFrameCount);
			nd_read_int((int *)&nd_recorded_time);
			if (Newdemo_vcr_state == ND_STATE_PLAYBACK)
				nd_recorded_total += nd_recorded_time;
			NewdemoFrameCount--;
			if (nd_bad_read) { done = -1; break; }
			break;
		}

		case ND_EVENT_VIEWER_OBJECT: // Followed by an object structure
			nd_read_object(Viewer);
			if (Newdemo_vcr_state != ND_STATE_PAUSED) {
				if (nd_bad_read) { done = -1; break; }
				segnum = Viewer->segnum;
				Viewer->next = Viewer->prev = Viewer->segnum = -1;

/* since we have multiple level recording, it can be the case that when rewinding the demo, the viewer is in a segment
   that is greater than the highest index of segments.  Bash the viewer to segment 0 for bogus view. HACK */

				if (segnum > Highest_segment_index)
					segnum = 0;
				obj_link(Viewer-Objects,segnum);
			}
			break;

		case ND_EVENT_RENDER_OBJECT: // Followed by an object structure
			objnum = obj_allocate();
			if (objnum==-1)
				break;
			obj = &Objects[objnum];
			nd_read_object(obj);
			if (nd_bad_read) { done = -1; break; }
			if (Newdemo_vcr_state != ND_STATE_PAUSED) {
				segnum = obj->segnum;
				obj->next = obj->prev = obj->segnum = -1;

// don't render objects is segments greater than Highest_segment_index (see above)

				if (segnum > Highest_segment_index)
					break;

				obj_link(obj-Objects,segnum);
				#ifdef NETWORK
				if ((obj->type == OBJ_PLAYER) && (Game_mode & GM_MULTI)) {
					int player;

					if (Game_mode & GM_TEAM)
						player = get_team(obj->id);
					else
						player = obj->id;
					if (player == 0)
						break;
					player--;

					for (i=0;i<N_PLAYER_SHIP_TEXTURES;i++)
						multi_player_textures[player][i] = ObjBitmaps[ObjBitmapPtrs[Polygon_models[obj->rtype.pobj_info.model_num].first_texture+i]];

					multi_player_textures[player][4] = ObjBitmaps[ObjBitmapPtrs[First_multi_bitmap_num+(player)*2]];
					multi_player_textures[player][5] = ObjBitmaps[ObjBitmapPtrs[First_multi_bitmap_num+(player)*2+1]];
					obj->rtype.pobj_info.alt_textures = player+1;
				}
				#endif
			}
			break;

		case ND_EVENT_SOUND:
			nd_read_int(&soundno);
			if (nd_bad_read) {done = -1; break; }
			if (Newdemo_vcr_state == ND_STATE_PLAYBACK)
				digi_play_sample( soundno, F1_0 );
			break;

		case ND_EVENT_SOUND_3D:
			nd_read_int(&soundno);
			nd_read_int(&angle);
			nd_read_int(&volume);
			if (nd_bad_read) { done = -1; break; }
			if (Newdemo_vcr_state == ND_STATE_PLAYBACK)
				digi_play_sample_3d( soundno, angle, volume, 0 );
			break;

		case ND_EVENT_SOUND_3D_ONCE:
			nd_read_int(&soundno);
			nd_read_int(&angle);
			nd_read_int(&volume);
			if (nd_bad_read) { done = -1; break; }
			if (Newdemo_vcr_state == ND_STATE_PLAYBACK)
				digi_play_sample_3d( soundno, angle, volume, 1 );
			break;

		case ND_EVENT_WALL_HIT_PROCESS: {
			int player, segnum;
			fix damage;

			nd_read_int(&segnum);
			nd_read_int(&side);
			nd_read_fix(&damage);
			nd_read_int(&player);
			if (nd_bad_read) { done = -1; break; }
			if (Newdemo_vcr_state != ND_STATE_PAUSED)
				wall_hit_process(&Segments[segnum], side, damage, player, &(Objects[0]) );
			break;
		}

		case ND_EVENT_TRIGGER:
			nd_read_int(&segnum);
			nd_read_int(&side);
			nd_read_int(&objnum);
			if (nd_bad_read) { done = -1; break; }
			if (Newdemo_vcr_state != ND_STATE_PAUSED)
				check_trigger(&Segments[segnum], side, objnum);
			break;

		case ND_EVENT_HOSTAGE_RESCUED: {
			int hostage_number;

			nd_read_int(&hostage_number);
			if (nd_bad_read) { done = -1; break; }
			if (Newdemo_vcr_state != ND_STATE_PAUSED)
				hostage_rescue( hostage_number );
			break;
		}

		case ND_EVENT_MORPH_FRAME: {
#if 0
			morph_data *md;

			md = &morph_objects[0];
			if (newdemo_read( md->morph_vecs, sizeof(md->morph_vecs), 1 )!=1) { done=-1; break; }
			if (newdemo_read( md->submodel_active, sizeof(md->submodel_active), 1 )!=1) { done=-1; break; }
			if (newdemo_read( md->submodel_startpoints, sizeof(md->submodel_startpoints), 1 )!=1) { done=-1; break; }
#endif
			objnum = obj_allocate();
			if (objnum==-1)
				break;
			obj = &Objects[objnum];
			nd_read_object(obj);
			obj->render_type = RT_POLYOBJ;
			if (Newdemo_vcr_state != ND_STATE_PAUSED) {
				if (nd_bad_read) { done = -1; break; }
				if (Newdemo_vcr_state != ND_STATE_PAUSED) {
					segnum = obj->segnum;
					obj->next = obj->prev = obj->segnum = -1;
					obj_link(obj-Objects,segnum);
				}
			}
			break;
		}

		case ND_EVENT_WALL_TOGGLE:
			nd_read_int(&segnum);
			nd_read_int(&side);
			if (nd_bad_read) {done = -1; break; }
			if (Newdemo_vcr_state != ND_STATE_PAUSED)
				wall_toggle(&Segments[segnum], side);
			break;

		case ND_EVENT_CONTROL_CENTER_DESTROYED:
			nd_read_int(&Fuelcen_seconds_left);
			Fuelcen_control_center_destroyed = 1;
			if (nd_bad_read) { done = -1; break; }
			if (!Newdemo_cntrlcen_destroyed) {
				newdemo_pop_ctrlcen_triggers();
				Newdemo_cntrlcen_destroyed = 1;
			}
			break;

		case ND_EVENT_HUD_MESSAGE: {
			char hud_msg[60];
			int class = MSGC_UNKNOWN;

			nd_read_string(&(hud_msg[0]));
			if (nd_bad_read) { done = -1; break; }
			#if 0 // adb: would cause noise for non-D1X...
			if (hud_msg[0] == '\x01')
				class = ((unsigned char)hud_msg[1]) |
				 (((unsigned char)hud_msg[2]) << 8) |
				 (((unsigned char)hud_msg[3]) << 16) |
				 (((unsigned char)hud_msg[4]) << 24);
			#endif
			if (Newdemo_vcr_state != ND_STATE_PAUSED)
				hud_message(class, hud_msg);
			break;
		}

		case ND_EVENT_PALETTE_EFFECT: {
			short r, g, b;

			nd_read_short(&r);
			nd_read_short(&g);
			nd_read_short(&b);
			if (nd_bad_read) { done = -1; break; }
			PALETTE_FLASH_SET(r,g,b);
			break;
		}

		case ND_EVENT_PLAYER_ENERGY: {
			ubyte energy;
#ifndef SHAREWARE
			ubyte old_energy;

			nd_read_byte((sbyte *)&old_energy);
#endif
			nd_read_byte((sbyte *)&energy);
			if (nd_bad_read) {done = -1; break; }
#ifdef SHAREWARE
			Players[Player_num].energy = i2f(energy);
#else
			if ((Newdemo_vcr_state == ND_STATE_PLAYBACK) || (Newdemo_vcr_state == ND_STATE_FASTFORWARD) || (Newdemo_vcr_state == ND_STATE_ONEFRAMEFORWARD)) {
				Players[Player_num].energy = i2f(energy);
			} else if ((Newdemo_vcr_state == ND_STATE_REWINDING) || (Newdemo_vcr_state == ND_STATE_ONEFRAMEBACKWARD)) {
				if (old_energy != 255)
					Players[Player_num].energy = i2f(old_energy);
			}
#endif
			break;
		}

		case ND_EVENT_PLAYER_SHIELD: {
			ubyte shield;
#ifndef SHAREWARE
			ubyte old_shield;

			nd_read_byte((sbyte *)&old_shield);
#endif
			nd_read_byte((sbyte *)&shield);
			if (nd_bad_read) {done = -1; break; }
#ifdef SHAREWARE
			Players[Player_num].shields = i2f(shield);
#else
			if ((Newdemo_vcr_state == ND_STATE_PLAYBACK) || (Newdemo_vcr_state == ND_STATE_FASTFORWARD) || (Newdemo_vcr_state == ND_STATE_ONEFRAMEFORWARD)) {
				Players[Player_num].shields = i2f(shield);
			} else if ((Newdemo_vcr_state == ND_STATE_REWINDING) || (Newdemo_vcr_state == ND_STATE_ONEFRAMEBACKWARD)) {
				if (old_shield != 255)
					Players[Player_num].shields = i2f(old_shield);
			}
#endif
			break;
		}

		case ND_EVENT_PLAYER_FLAGS: {
			uint oflags;

			nd_read_int((int *)&(Players[Player_num].flags));
			if (nd_bad_read) {done = -1; break; }

			oflags = Players[Player_num].flags >> 16;
			Players[Player_num].flags &= 0xffff;

			if (((Newdemo_vcr_state == ND_STATE_REWINDING) || (Newdemo_vcr_state == ND_STATE_ONEFRAMEBACKWARD)) && (oflags != 0xffff)) {
				if (!(oflags & PLAYER_FLAGS_CLOAKED) && (Players[Player_num].flags & PLAYER_FLAGS_CLOAKED)) {
					Players[Player_num].cloak_time = 0;
					Newdemo_players_cloaked &= ~(1 << Player_num);
				}
				if ((oflags & PLAYER_FLAGS_CLOAKED) && !(Players[Player_num].flags & PLAYER_FLAGS_CLOAKED)) {
					Players[Player_num].cloak_time = GameTime - (CLOAK_TIME_MAX / 2);
					Newdemo_players_cloaked |= (1 << Player_num);
				}
				if (!(oflags & PLAYER_FLAGS_INVULNERABLE) && (Players[Player_num].flags & PLAYER_FLAGS_INVULNERABLE))
					Players[Player_num].invulnerable_time = 0;
				if ((oflags & PLAYER_FLAGS_INVULNERABLE) && !(Players[Player_num].flags & PLAYER_FLAGS_INVULNERABLE))
					Players[Player_num].invulnerable_time = GameTime - (INVULNERABLE_TIME_MAX / 2);
				Players[Player_num].flags = oflags;
			} else if ((Newdemo_vcr_state == ND_STATE_PLAYBACK) || (Newdemo_vcr_state == ND_STATE_FASTFORWARD) || (Newdemo_vcr_state == ND_STATE_ONEFRAMEFORWARD)) {
				if (!(oflags & PLAYER_FLAGS_CLOAKED) && (Players[Player_num].flags & PLAYER_FLAGS_CLOAKED)) {
					Players[Player_num].cloak_time = GameTime - (CLOAK_TIME_MAX / 2);
					Newdemo_players_cloaked |= (1 << Player_num);
				}
				if ((oflags & PLAYER_FLAGS_CLOAKED) && !(Players[Player_num].flags & PLAYER_FLAGS_CLOAKED)) {
					Players[Player_num].cloak_time = 0;
					Newdemo_players_cloaked &= ~(1 << Player_num);
				}
				if (!(oflags & PLAYER_FLAGS_INVULNERABLE) && (Players[Player_num].flags & PLAYER_FLAGS_INVULNERABLE))
					Players[Player_num].invulnerable_time = GameTime - (INVULNERABLE_TIME_MAX / 2);
				if ((oflags & PLAYER_FLAGS_INVULNERABLE) && !(Players[Player_num].flags & PLAYER_FLAGS_INVULNERABLE))
					Players[Player_num].invulnerable_time = 0;
			}
			update_laser_weapon_info(); // in case of quad laser change
			break;
		}

#ifdef SHAREWARE
		case ND_EVENT_PLAYER_WEAPON: {
			sbyte weapon_type, weapon_num;

			nd_read_byte(&weapon_type);
			nd_read_byte(&weapon_num);

			if (weapon_type == 0)
				Primary_weapon = (int)weapon_num;
			else
				Secondary_weapon = (int)weapon_num;

			break;
		}
#else
		case ND_EVENT_PLAYER_WEAPON: {
			sbyte weapon_type, weapon_num;
			sbyte old_weapon;

			nd_read_byte(&weapon_type);
			nd_read_byte(&weapon_num);
			nd_read_byte(&old_weapon);
			if ((Newdemo_vcr_state == ND_STATE_PLAYBACK) || (Newdemo_vcr_state == ND_STATE_FASTFORWARD) || (Newdemo_vcr_state == ND_STATE_ONEFRAMEFORWARD)) {
				if (weapon_type == 0)
					Primary_weapon = (int)weapon_num;
				else
					Secondary_weapon = (int)weapon_num;
			} else if ((Newdemo_vcr_state == ND_STATE_REWINDING) || (Newdemo_vcr_state == ND_STATE_ONEFRAMEBACKWARD)) {
				if (weapon_type == 0)
					Primary_weapon = (int)old_weapon;
				else
					Secondary_weapon = (int)old_weapon;
			}
			break;
		}
#endif


		case ND_EVENT_EFFECT_BLOWUP: {
			short segnum;
			sbyte side;
			vms_vector pnt;

			nd_read_short(&segnum);
			nd_read_byte(&side);
			nd_read_vector(&pnt);
			if (Newdemo_vcr_state != ND_STATE_PAUSED)
				check_effect_blowup(&(Segments[segnum]), side, &pnt);
			break;
		}

		case ND_EVENT_HOMING_DISTANCE: {
			short distance;

			nd_read_short(&distance);
			Players[Player_num].homing_object_dist = i2f((int)(distance << 16));
			break;
		}

		case ND_EVENT_LETTERBOX:
			if ((Newdemo_vcr_state == ND_STATE_PLAYBACK) || (Newdemo_vcr_state == ND_STATE_FASTFORWARD) || (Newdemo_vcr_state == ND_STATE_ONEFRAMEFORWARD)) {
				saved_letter_cockpit = PlayerCfg.CockpitMode;
				select_cockpit(CM_LETTERBOX);
			} else if ((Newdemo_vcr_state == ND_STATE_REWINDING) || (Newdemo_vcr_state == ND_STATE_ONEFRAMEBACKWARD))
				select_cockpit(saved_letter_cockpit);
			break;

		case ND_EVENT_REARVIEW:
			if ((Newdemo_vcr_state == ND_STATE_PLAYBACK) || (Newdemo_vcr_state == ND_STATE_FASTFORWARD) || (Newdemo_vcr_state == ND_STATE_ONEFRAMEFORWARD)) {
				saved_rearview_cockpit = PlayerCfg.CockpitMode;
				if (PlayerCfg.CockpitMode == CM_FULL_COCKPIT)
					select_cockpit(CM_REAR_VIEW);
				Rear_view=1;
			} else if ((Newdemo_vcr_state == ND_STATE_REWINDING) || (Newdemo_vcr_state == ND_STATE_ONEFRAMEBACKWARD)) {
				if (saved_rearview_cockpit == CM_REAR_VIEW) // hack to be sure we get a good cockpit on restore
					saved_rearview_cockpit = CM_FULL_COCKPIT;
				select_cockpit(saved_rearview_cockpit);
				Rear_view=0;
			}
			break;

		case ND_EVENT_RESTORE_COCKPIT:
			if ((Newdemo_vcr_state == ND_STATE_REWINDING) || (Newdemo_vcr_state == ND_STATE_ONEFRAMEBACKWARD)) {
				saved_letter_cockpit = PlayerCfg.CockpitMode;
				select_cockpit(CM_LETTERBOX);
			} else if ((Newdemo_vcr_state == ND_STATE_PLAYBACK) || (Newdemo_vcr_state == ND_STATE_FASTFORWARD) || (Newdemo_vcr_state == ND_STATE_ONEFRAMEFORWARD))
				select_cockpit(saved_letter_cockpit);
			break;


		case ND_EVENT_RESTORE_REARVIEW:
			if ((Newdemo_vcr_state == ND_STATE_REWINDING) || (Newdemo_vcr_state == ND_STATE_ONEFRAMEBACKWARD)) {
				saved_rearview_cockpit = PlayerCfg.CockpitMode;
				if (PlayerCfg.CockpitMode == CM_FULL_COCKPIT)
					select_cockpit(CM_REAR_VIEW);
				Rear_view=1;
			} else if ((Newdemo_vcr_state == ND_STATE_PLAYBACK) || (Newdemo_vcr_state == ND_STATE_FASTFORWARD) || (Newdemo_vcr_state == ND_STATE_ONEFRAMEFORWARD)) {
				if (saved_rearview_cockpit == CM_REAR_VIEW) // hack to be sure we get a good cockpit on restore
					saved_rearview_cockpit = CM_FULL_COCKPIT;
				select_cockpit(saved_rearview_cockpit);
				Rear_view=0;
			}
			break;


		case ND_EVENT_WALL_SET_TMAP_NUM1:	{
			short seg, cseg, tmap;
			ubyte side,cside;

			nd_read_short(&seg);
			nd_read_byte((sbyte *)&side);
			nd_read_short(&cseg);
			nd_read_byte((sbyte *)&cside);
			nd_read_short( &tmap );
			if ((Newdemo_vcr_state != ND_STATE_PAUSED) && (Newdemo_vcr_state != ND_STATE_REWINDING) && (Newdemo_vcr_state != ND_STATE_ONEFRAMEBACKWARD))
				Segments[seg].sides[side].tmap_num = Segments[cseg].sides[cside].tmap_num = tmap;
			break;
		}

		case ND_EVENT_WALL_SET_TMAP_NUM2:	{
			short seg, cseg, tmap;
			ubyte side,cside;

			nd_read_short(&seg);
			nd_read_byte((sbyte *)&side);
			nd_read_short(&cseg);
			nd_read_byte((sbyte *)&cside);
			nd_read_short( &tmap );
			if ((Newdemo_vcr_state != ND_STATE_PAUSED) && (Newdemo_vcr_state != ND_STATE_REWINDING) && (Newdemo_vcr_state != ND_STATE_ONEFRAMEBACKWARD)) {
				Assert(tmap!=0 && Segments[seg].sides[side].tmap_num2!=0);
				Segments[seg].sides[side].tmap_num2 = Segments[cseg].sides[cside].tmap_num2 = tmap;
			}
			break;
		}

		case ND_EVENT_MULTI_CLOAK: {
			sbyte pnum;

			nd_read_byte(&pnum);
			if ((Newdemo_vcr_state == ND_STATE_REWINDING) || (Newdemo_vcr_state == ND_STATE_ONEFRAMEBACKWARD)) {
				Players[pnum].flags &= ~PLAYER_FLAGS_CLOAKED;
				Players[pnum].cloak_time = 0;
				Newdemo_players_cloaked &= ~(1 << pnum);
			} else if ((Newdemo_vcr_state == ND_STATE_PLAYBACK) || (Newdemo_vcr_state == ND_STATE_FASTFORWARD) || (Newdemo_vcr_state == ND_STATE_ONEFRAMEFORWARD)) {
				Players[pnum].flags |= PLAYER_FLAGS_CLOAKED;
				Players[pnum].cloak_time = GameTime  - (CLOAK_TIME_MAX / 2);
				Newdemo_players_cloaked |= (1 << pnum);
			}
			break;
		}

		case ND_EVENT_MULTI_DECLOAK: {
			sbyte pnum;

			nd_read_byte(&pnum);

			if ((Newdemo_vcr_state == ND_STATE_REWINDING) || (Newdemo_vcr_state == ND_STATE_ONEFRAMEBACKWARD)) {
				Players[pnum].flags |= PLAYER_FLAGS_CLOAKED;
				Players[pnum].cloak_time = GameTime  - (CLOAK_TIME_MAX / 2);
				Newdemo_players_cloaked |= (1 << pnum);
			} else if ((Newdemo_vcr_state == ND_STATE_PLAYBACK) || (Newdemo_vcr_state == ND_STATE_FASTFORWARD) || (Newdemo_vcr_state == ND_STATE_ONEFRAMEFORWARD)) {
				Players[pnum].flags &= ~PLAYER_FLAGS_CLOAKED;
				Players[pnum].cloak_time = 0;
				Newdemo_players_cloaked &= ~(1 << pnum);
			}
			break;
		}

#ifndef SHAREWARE
		case ND_EVENT_MULTI_DEATH: {
			sbyte pnum;

			nd_read_byte(&pnum);
			if ((Newdemo_vcr_state == ND_STATE_REWINDING) || (Newdemo_vcr_state == ND_STATE_ONEFRAMEBACKWARD))
				Players[pnum].net_killed_total--;
			else if ((Newdemo_vcr_state == ND_STATE_PLAYBACK) || (Newdemo_vcr_state == ND_STATE_FASTFORWARD) || (Newdemo_vcr_state == ND_STATE_ONEFRAMEFORWARD))
				Players[pnum].net_killed_total++;
			break;
		}

#ifdef NETWORK	
		case ND_EVENT_MULTI_KILL: {
			sbyte pnum, kill;

			nd_read_byte(&pnum);
			nd_read_byte(&kill);
			if ((Newdemo_vcr_state == ND_STATE_REWINDING) || (Newdemo_vcr_state == ND_STATE_ONEFRAMEBACKWARD)) {
				Players[pnum].net_kills_total -= kill;
 				if (Game_mode & GM_TEAM)
 					team_kills[get_team(pnum)] -= kill;
 			} else if ((Newdemo_vcr_state == ND_STATE_PLAYBACK) || (Newdemo_vcr_state == ND_STATE_FASTFORWARD) || (Newdemo_vcr_state == ND_STATE_ONEFRAMEFORWARD)) {
				Players[pnum].net_kills_total += kill;
 				if (Game_mode & GM_TEAM)
 					team_kills[get_team(pnum)] += kill;
			}
			multi_sort_kill_list();
			break;
		}
		
		case ND_EVENT_MULTI_CONNECT: {
			sbyte pnum, new_player;
			int killed_total, kills_total;
			char new_callsign[CALLSIGN_LEN+1], old_callsign[CALLSIGN_LEN+1];

			nd_read_byte(&pnum);
			nd_read_byte(&new_player);
			if (!new_player) {
				nd_read_string(old_callsign);
				nd_read_int(&killed_total);
				nd_read_int(&kills_total);
			}
			nd_read_string(new_callsign);
			if ((Newdemo_vcr_state == ND_STATE_REWINDING) || (Newdemo_vcr_state == ND_STATE_ONEFRAMEBACKWARD)) {
				Players[pnum].connected = 0;
				if (!new_player) {
					memcpy(Players[pnum].callsign, old_callsign, CALLSIGN_LEN+1);
					Players[pnum].net_killed_total = killed_total;
					Players[pnum].net_kills_total = kills_total;
				} else {
					N_players--;
				}
			} else if ((Newdemo_vcr_state == ND_STATE_PLAYBACK) || (Newdemo_vcr_state == ND_STATE_FASTFORWARD) || (Newdemo_vcr_state == ND_STATE_ONEFRAMEFORWARD)) {
				Players[pnum].connected = 1;
				Players[pnum].net_kills_total = 0;
				Players[pnum].net_killed_total = 0;
				memcpy(Players[pnum].callsign, new_callsign, CALLSIGN_LEN+1);
				if (new_player)
					N_players++;
			}
			break;
		}

		case ND_EVENT_MULTI_RECONNECT: {
			sbyte pnum;

			nd_read_byte(&pnum);
			if ((Newdemo_vcr_state == ND_STATE_REWINDING) || (Newdemo_vcr_state == ND_STATE_ONEFRAMEBACKWARD))
				Players[pnum].connected = 0;
			else if ((Newdemo_vcr_state == ND_STATE_PLAYBACK) || (Newdemo_vcr_state == ND_STATE_FASTFORWARD) || (Newdemo_vcr_state == ND_STATE_ONEFRAMEFORWARD))
				Players[pnum].connected = 1;
			break;
		}

		case ND_EVENT_MULTI_DISCONNECT: {
			sbyte pnum;

			nd_read_byte(&pnum);
			if ((Newdemo_vcr_state == ND_STATE_REWINDING) || (Newdemo_vcr_state == ND_STATE_ONEFRAMEBACKWARD))
				Players[pnum].connected = 1;
			else if ((Newdemo_vcr_state == ND_STATE_PLAYBACK) || (Newdemo_vcr_state == ND_STATE_FASTFORWARD) || (Newdemo_vcr_state == ND_STATE_ONEFRAMEFORWARD))
				Players[pnum].connected = 0;
			break;
		}

		case ND_EVENT_MULTI_SCORE: {
			int score;
			sbyte pnum;

			nd_read_byte(&pnum);
			nd_read_int(&score);
			if ((Newdemo_vcr_state == ND_STATE_REWINDING) || (Newdemo_vcr_state == ND_STATE_ONEFRAMEBACKWARD))
				Players[pnum].score -= score;
			else if ((Newdemo_vcr_state == ND_STATE_PLAYBACK) || (Newdemo_vcr_state == ND_STATE_FASTFORWARD) || (Newdemo_vcr_state == ND_STATE_ONEFRAMEFORWARD))
				Players[pnum].score += score;
			multi_sort_kill_list();
			break;
		}
#endif

		case ND_EVENT_PLAYER_SCORE: {
			int score;

			nd_read_int(&score);
			if ((Newdemo_vcr_state == ND_STATE_REWINDING) || (Newdemo_vcr_state == ND_STATE_ONEFRAMEBACKWARD))
				Players[Player_num].score -= score;
			else if ((Newdemo_vcr_state == ND_STATE_PLAYBACK) || (Newdemo_vcr_state == ND_STATE_FASTFORWARD) || (Newdemo_vcr_state == ND_STATE_ONEFRAMEFORWARD))
				Players[Player_num].score += score;
			break;
		}


		case ND_EVENT_PRIMARY_AMMO: {
			short old_ammo, new_ammo;

			nd_read_short(&old_ammo);
			nd_read_short(&new_ammo);

			if ((Newdemo_vcr_state == ND_STATE_REWINDING) || (Newdemo_vcr_state == ND_STATE_ONEFRAMEBACKWARD))
				Players[Player_num].primary_ammo[Primary_weapon] = old_ammo;
			else if ((Newdemo_vcr_state == ND_STATE_PLAYBACK) || (Newdemo_vcr_state == ND_STATE_FASTFORWARD) || (Newdemo_vcr_state == ND_STATE_ONEFRAMEFORWARD))
				Players[Player_num].primary_ammo[Primary_weapon] = new_ammo;
			break;
		}

		case ND_EVENT_SECONDARY_AMMO: {
			short old_ammo, new_ammo;

			nd_read_short(&old_ammo);
			nd_read_short(&new_ammo);

			if ((Newdemo_vcr_state == ND_STATE_REWINDING) || (Newdemo_vcr_state == ND_STATE_ONEFRAMEBACKWARD))
				Players[Player_num].secondary_ammo[Secondary_weapon] = old_ammo;
			else if ((Newdemo_vcr_state == ND_STATE_PLAYBACK) || (Newdemo_vcr_state == ND_STATE_FASTFORWARD) || (Newdemo_vcr_state == ND_STATE_ONEFRAMEFORWARD))
				Players[Player_num].secondary_ammo[Secondary_weapon] = new_ammo;
			break;
		}

		case ND_EVENT_DOOR_OPENING: {
			short segnum;
			sbyte side;

			nd_read_short(&segnum);
			nd_read_byte(&side);
			if ((Newdemo_vcr_state == ND_STATE_REWINDING) || (Newdemo_vcr_state == ND_STATE_ONEFRAMEBACKWARD)) {
				int anim_num;
				int cside;
				segment *segp, *csegp;

				segp = &Segments[segnum];
				csegp = &Segments[segp->children[side]];
				cside = find_connect_side(segp, csegp);
				anim_num = Walls[segp->sides[side].wall_num].clip_num;

				if (WallAnims[anim_num].flags & WCF_TMAP1)	{
					segp->sides[side].tmap_num = csegp->sides[cside].tmap_num = WallAnims[anim_num].frames[0];
				} else	{
					segp->sides[side].tmap_num2 = csegp->sides[cside].tmap_num2 = WallAnims[anim_num].frames[0];
				}
			}
			break;
		}

		case ND_EVENT_LASER_LEVEL: {
			sbyte old_level, new_level;

			nd_read_byte(&old_level);
			nd_read_byte(&new_level);
			if ((Newdemo_vcr_state == ND_STATE_REWINDING) || (Newdemo_vcr_state == ND_STATE_ONEFRAMEBACKWARD)) {
				Players[Player_num].laser_level = old_level;
				update_laser_weapon_info();
			} else if ((Newdemo_vcr_state == ND_STATE_PLAYBACK) || (Newdemo_vcr_state == ND_STATE_FASTFORWARD) || (Newdemo_vcr_state == ND_STATE_ONEFRAMEFORWARD)) {
				Players[Player_num].laser_level = new_level;
				update_laser_weapon_info();
			}
			break;
		}

#endif

		case ND_EVENT_NEW_LEVEL: 	{
			sbyte new_level, old_level, loaded_level;
 
			nd_read_byte (&new_level);
			nd_read_byte (&old_level);
			if (Newdemo_vcr_state == ND_STATE_PAUSED)
				break;

			stop_time();
			if ((Newdemo_vcr_state == ND_STATE_REWINDING) || (Newdemo_vcr_state == ND_STATE_ONEFRAMEBACKWARD))
				loaded_level = old_level;
			else {
				loaded_level = new_level;
				for (i = 0; i < MAX_PLAYERS; i++) {
					Players[i].cloak_time = 0;
					Players[i].flags &= ~PLAYER_FLAGS_CLOAKED;
				}
			}
#ifdef DEST_SAT
			if ( (loaded_level < Last_secret_level) || (loaded_level > Last_level - 1) ) {
				newmenu_item m[1];

				m[ 0].type = NM_TYPE_TEXT; m[ 0].text = TXT_NO_DESTSAT_LVL;
				newmenu_do( NULL, NULL, sizeof(m)/sizeof(*m), m, NULL );
				return -1;
			}
#else
			if ((loaded_level < Last_secret_level) || (loaded_level > Last_level)) {
				newmenu_item m[3];

				m[ 0].type = NM_TYPE_TEXT; m[ 0].text = TXT_CANT_PLAYBACK;
				m[ 1].type = NM_TYPE_TEXT; m[ 1].text = TXT_LEVEL_CANT_LOAD;
				m[ 2].type = NM_TYPE_TEXT; m[ 2].text = TXT_DEMO_OLD_CORRUPT;
				newmenu_do( NULL, NULL, sizeof(m)/sizeof(*m), m, NULL );
				return -1;
			}
#endif
			LoadLevel((int)loaded_level);
			piggy_load_level_data();
			Newdemo_cntrlcen_destroyed = 0;

			reset_palette_add(); // get palette back to normal
			start_time();
			break;
		}

		case ND_EVENT_EOF: {
			done=-1;
			PHYSFS_seek(infile, PHYSFS_tell(infile) - 1); // get back to the EOF marker
			Newdemo_at_eof = 1;
			NewdemoFrameCount++;
			break;
		}

		case ND_EVENT_LINK_SOUND_TO_OBJ:
			{
				int soundno, objnum, max_volume, max_distance, loop_start, loop_end;
				int signature;
				nd_read_int( &soundno );
				nd_read_int( &signature );
				nd_read_int( &max_volume );
				nd_read_int( &max_distance );
				nd_read_int( &loop_start );
				nd_read_int( &loop_end );
				objnum = newdemo_find_object( signature );
				if ( objnum > -1 && Newdemo_vcr_state == ND_STATE_PLAYBACK)  {   //  @mk, 2/22/96, John told me to.
					digi_link_sound_to_object3( soundno, objnum, 1, max_volume, max_distance, loop_start, loop_end );
				}
			}
			break;

		case ND_EVENT_KILL_SOUND_TO_OBJ:
			{
				int objnum, signature;
				nd_read_int( &signature );
				objnum = newdemo_find_object( signature );
				if ( objnum > -1 && Newdemo_vcr_state == ND_STATE_PLAYBACK)  {   //  @mk, 2/22/96, John told me to.
					digi_kill_sound_linked_to_object(objnum);
				}
			}
			break;

		default:
			Int3();
		}
	}
	if (nd_bad_read) {
		newmenu_item m[2];

		m[ 0].type = NM_TYPE_TEXT; m[ 0].text = TXT_DEMO_ERR_READING;
		m[ 1].type = NM_TYPE_TEXT; m[ 1].text = TXT_DEMO_OLD_CORRUPT;
		newmenu_do( NULL, NULL, sizeof(m)/sizeof(*m), m, NULL );
	}

	return done;
}

void newdemo_goto_beginning()
{
// 	if (NewdemoFrameCount == 0)
// 		return;
	PHYSFS_seek(infile, 0);
	Newdemo_vcr_state = ND_STATE_PLAYBACK;
	if (newdemo_read_demo_start(0))
		newdemo_stop_playback();
	if (newdemo_read_frame_information() == -1)
		newdemo_stop_playback();
	if (newdemo_read_frame_information() == -1)
		newdemo_stop_playback();
	Newdemo_vcr_state = ND_STATE_PAUSED;
	Newdemo_at_eof = 0;
}

#ifdef SHAREWARE
void newdemo_goto_end()
{
	short frame_length=0;
	sbyte level=0;
	int i=0;

	cfseek(infile, -2, SEEK_END);
	nd_read_byte(&level);

	if ((level < Last_secret_level) || (level > Last_level)) {
		newmenu_item m[3];

		m[ 0].type = NM_TYPE_TEXT; m[ 0].text = TXT_CANT_PLAYBACK;
		m[ 1].type = NM_TYPE_TEXT; m[ 1].text = TXT_LEVEL_CANT_LOAD;
		m[ 2].type = NM_TYPE_TEXT; m[ 2].text = TXT_DEMO_OLD_CORRUPT;
		newmenu_do( NULL, NULL, sizeof(m)/sizeof(*m), m, NULL );
		newdemo_stop_playback();
		return;
	}

	if (level != Current_level_num)	{
		LoadLevel(level);
		piggy_load_level_data();
	}
	if (Game_mode & GM_MULTI) {
		cfseek(infile, -10, SEEK_END);
		nd_read_byte(&Newdemo_players_cloaked);
		for (i = 0; i < MAX_PLAYERS; i++) {
			if ((1 << i) & Newdemo_players_cloaked)
				Players[i].flags |= PLAYER_FLAGS_CLOAKED;
				Players[i].cloak_time = GameTime - (CLOAK_TIME_MAX / 2);
		}
	}
	cfseek(infile, -12, SEEK_END);
	nd_read_short(&frame_length);
	cfseek(infile, -frame_length, SEEK_CUR);
	nd_read_int(&NewdemoFrameCount); // get the frame count
	NewdemoFrameCount--;
	cfseek(infile, 4, SEEK_CUR);
	newdemo_read_frame_information(); // then the frame information
	Newdemo_vcr_state = ND_STATE_PAUSED;
	return;
}
#else
void newdemo_goto_end()
{
	short frame_length=0, byte_count=0, bshort=0;
	sbyte level=0, bbyte=0, laser_level=0;
	ubyte energy=0, shield=0;
	int i=0, loc=0, bint=0;

	cfseek(infile, -2, SEEK_END);
	nd_read_byte(&level);

	if ((level < Last_secret_level) || (level > Last_level)) {
		newmenu_item m[3];

		m[ 0].type = NM_TYPE_TEXT; m[ 0].text = TXT_CANT_PLAYBACK;
		m[ 1].type = NM_TYPE_TEXT; m[ 1].text = TXT_LEVEL_CANT_LOAD;
		m[ 2].type = NM_TYPE_TEXT; m[ 2].text = TXT_DEMO_OLD_CORRUPT;
		newmenu_do( NULL, NULL, sizeof(m)/sizeof(*m), m, NULL );
		newdemo_stop_playback();
		return;
	}

	if (level != Current_level_num)	{
		LoadLevel(level);
		piggy_load_level_data();
	}
	cfseek(infile, -4, SEEK_END);
	nd_read_short(&byte_count);
	cfseek(infile, -2 - byte_count, SEEK_CUR);

	nd_read_short(&frame_length);
	loc = cftell(infile);
	if (Game_mode & GM_MULTI)
		nd_read_byte(&Newdemo_players_cloaked);
	else
		nd_read_byte(&bbyte);
	nd_read_byte(&bbyte);
	nd_read_short(&bshort);
	nd_read_int(&bint);
	
	nd_read_byte((sbyte *)&energy);
	nd_read_byte((sbyte *)&shield);
	Players[Player_num].energy = i2f(energy);
	Players[Player_num].shields = i2f(shield);
	nd_read_int((int *)&(Players[Player_num].flags));
	if (Players[Player_num].flags & PLAYER_FLAGS_CLOAKED) {
		Players[Player_num].cloak_time = GameTime - (CLOAK_TIME_MAX / 2);
		Newdemo_players_cloaked |= (1 << Player_num);
	}
	if (Players[Player_num].flags & PLAYER_FLAGS_INVULNERABLE)
		Players[Player_num].invulnerable_time = GameTime - (INVULNERABLE_TIME_MAX / 2);
	nd_read_byte((sbyte *)&Primary_weapon);
	nd_read_byte((sbyte *)&Secondary_weapon);
	for (i = 0; i < MAX_PRIMARY_WEAPONS; i++)
		nd_read_short((short *)&(Players[Player_num].primary_ammo[i]));	
	for (i = 0; i < MAX_SECONDARY_WEAPONS; i++)
		nd_read_short((short *)&(Players[Player_num].secondary_ammo[i]));	
	nd_read_byte(&laser_level);
	if (laser_level != Players[Player_num].laser_level) {
		Players[Player_num].laser_level = laser_level;
		update_laser_weapon_info();
	}

	if (Game_mode & GM_MULTI) {
		nd_read_byte((sbyte *)&N_players);
		for (i = 0; i < N_players; i++) {
			nd_read_string(Players[i].callsign);
			nd_read_byte(&(Players[i].connected));
			if (Game_mode & GM_MULTI_COOP) {
				nd_read_int(&(Players[i].score));
			} else {
				nd_read_short((short *)&(Players[i].net_killed_total));
				nd_read_short((short *)&(Players[i].net_kills_total));
			}
		}
	} else {
		nd_read_int(&(Players[Player_num].score));
	}

	cfseek(infile, loc, SEEK_SET);
	cfseek(infile, -frame_length, SEEK_CUR);
	nd_read_int(&NewdemoFrameCount); // get the frame count
	NewdemoFrameCount--;
	cfseek(infile, 4, SEEK_CUR);
	Newdemo_vcr_state = ND_STATE_PLAYBACK;
	newdemo_read_frame_information(); // then the frame information
	Newdemo_vcr_state = ND_STATE_PAUSED;
	return;
}
#endif

void newdemo_back_frames(int frames)
{
	short last_frame_length;
	int i;

	for (i = 0; i < frames; i++)
	{
		PHYSFS_seek(infile, PHYSFS_tell(infile) - 10);
		nd_read_short(&last_frame_length);			
		PHYSFS_seek(infile, PHYSFS_tell(infile) + 8 - last_frame_length);

		if (!Newdemo_at_eof && newdemo_read_frame_information() == -1) {
			newdemo_stop_playback();
			return;
		}
		if (Newdemo_at_eof)
			Newdemo_at_eof = 0;

		PHYSFS_seek(infile, PHYSFS_tell(infile) - 10);
		nd_read_short(&last_frame_length);			
		PHYSFS_seek(infile, PHYSFS_tell(infile) + 8 - last_frame_length);
	}

}

/*
 *  routine to interpolate the viewer position.  the current position is
 *  stored in the Viewer object.  Save this position, and read the next
 *  frame to get all objects read in.  Calculate the delta playback and
 *  the delta recording frame times between the two frames, then intepolate
 *  the viewers position accordingly.  nd_recorded_time is the time that it
 *  took the recording to render the frame that we are currently looking
 *  at.
*/

void interpolate_frame(fix d_play, fix d_recorded)
{
	int i, j, num_cur_objs;
	fix factor;
	object *cur_objs;

	factor = fixdiv(d_play, d_recorded);
	if (factor > F1_0)
		factor = F1_0;

	num_cur_objs = Highest_object_index;
	cur_objs = (object *)d_malloc(sizeof(object) * (num_cur_objs + 1));
	if (cur_objs == NULL) {
		Int3();
		return;
	}
	for (i = 0; i <= num_cur_objs; i++)
		memcpy(&(cur_objs[i]), &(Objects[i]), sizeof(object));

	Newdemo_vcr_state = ND_STATE_PAUSED;
	if (newdemo_read_frame_information() == -1) {
		d_free(cur_objs);
		newdemo_stop_playback();
		return;
	}

	for (i = 0; i <= num_cur_objs; i++) {
		for (j = 0; j <= Highest_object_index; j++) {
			if (cur_objs[i].signature == Objects[j].signature) {
				ubyte render_type = cur_objs[i].render_type;
				fix	delta_x, delta_y, delta_z;

//  Extract the angles from the object orientation matrix.
//  Some of this code taken from ai_turn_towards_vector
//  Don't do the interpolation on certain render types which don't use an orientation matrix

				if (!((render_type == RT_LASER) || (render_type == RT_FIREBALL) || (render_type == RT_POWERUP))) {

				vms_vector	fvec1, fvec2, rvec1, rvec2;
				fix			mag1;
				
				fvec1 = cur_objs[i].orient.fvec;
				vm_vec_scale(&fvec1, F1_0-factor);
				fvec2 = Objects[j].orient.fvec;
				vm_vec_scale(&fvec2, factor);
				vm_vec_add2(&fvec1, &fvec2);
				mag1 = vm_vec_normalize_quick(&fvec1);
					if (mag1 > F1_0/256) {
						rvec1 = cur_objs[i].orient.rvec;
						vm_vec_scale(&rvec1, F1_0-factor);
						rvec2 = Objects[j].orient.rvec;
						vm_vec_scale(&rvec2, factor);
						vm_vec_add2(&rvec1, &rvec2);
						vm_vec_normalize_quick(&rvec1); // Note: Doesn't matter if this is null, if null, vm_vector_2_matrix will just use fvec1
						vm_vector_2_matrix(&cur_objs[i].orient, &fvec1, NULL, &rvec1);
					}
				}

// Interpolate the object position.  This is just straight linear
// interpolation.

				delta_x = Objects[j].pos.x - cur_objs[i].pos.x;
				delta_y = Objects[j].pos.y - cur_objs[i].pos.y;
				delta_z = Objects[j].pos.z - cur_objs[i].pos.z;

				delta_x = fixmul(delta_x, factor);
				delta_y = fixmul(delta_y, factor);
				delta_z = fixmul(delta_z, factor);

				cur_objs[i].pos.x += delta_x;
				cur_objs[i].pos.y += delta_y;
				cur_objs[i].pos.z += delta_z;
			}
		}
	}

// get back to original position in the demo file.  Reread the current
// frame information again to reset all of the object stuff not covered
// with Highest_object_index and the object array (previously rendered
// objects, etc....)

	newdemo_back_frames(1);
	newdemo_back_frames(1);
	if (newdemo_read_frame_information() == -1)
		newdemo_stop_playback();
	Newdemo_vcr_state = ND_STATE_PLAYBACK;

	for (i = 0; i <= num_cur_objs; i++)
		memcpy(&(Objects[i]), &(cur_objs[i]), sizeof(object));
	Highest_object_index = num_cur_objs;
	d_free(cur_objs);
}

void newdemo_playback_one_frame()
{
	int frames_back, i, level;
	static fix base_interpol_time = 0;
	static fix d_recorded = 0;

	for (i = 0; i < MAX_PLAYERS; i++)
		if (Newdemo_players_cloaked & (1 << i))
			Players[i].cloak_time = GameTime - (CLOAK_TIME_MAX / 2);

	if (Players[Player_num].flags & PLAYER_FLAGS_INVULNERABLE)
		Players[Player_num].invulnerable_time = GameTime - (INVULNERABLE_TIME_MAX / 2);
	
	if (Newdemo_vcr_state == ND_STATE_PAUSED)			// render a frame or not
		return;

	if (Newdemo_vcr_state == ND_STATE_PLAYBACK)
		DoJasonInterpolate(nd_recorded_time);

	Fuelcen_control_center_destroyed = 0;
	Fuelcen_seconds_left = -1;
	PALETTE_FLASH_SET(0,0,0);		//clear flash

	if ((Newdemo_vcr_state == ND_STATE_REWINDING) || (Newdemo_vcr_state == ND_STATE_ONEFRAMEBACKWARD))
	{
		level = Current_level_num;
		if (NewdemoFrameCount == 0)
			return;
		else if ((Newdemo_vcr_state == ND_STATE_REWINDING) && (NewdemoFrameCount < 10)) {
			newdemo_goto_beginning();
			return;
		}
		if (Newdemo_vcr_state == ND_STATE_REWINDING)
			frames_back = 10;
		else
			frames_back = 1;
		if (Newdemo_at_eof) {
#ifdef SHAREWARE
			PHYSFS_seek(infile, PHYSFS_tell(infile) - 2);
#else
			PHYSFS_seek(infile, PHYSFS_tell(infile) + 11);
#endif
		}
		newdemo_back_frames(frames_back);

		if (level != Current_level_num)
			newdemo_pop_ctrlcen_triggers();

		if (Newdemo_vcr_state == ND_STATE_ONEFRAMEBACKWARD) {
			if (level != Current_level_num)
				newdemo_back_frames(1);
			Newdemo_vcr_state = ND_STATE_PAUSED;
		}
	}
	else if (Newdemo_vcr_state == ND_STATE_FASTFORWARD) {
		if (!Newdemo_at_eof)
		{
			for (i = 0; i < 10; i++)
			{
				if (newdemo_read_frame_information() == -1)
				{
					if (Newdemo_at_eof)
						Newdemo_vcr_state = ND_STATE_PAUSED;
					else
						newdemo_stop_playback();
					break;
				}
		  	}
		}
		else
			Newdemo_vcr_state = ND_STATE_PAUSED;
	}
	else if (Newdemo_vcr_state == ND_STATE_ONEFRAMEFORWARD) {
		if (!Newdemo_at_eof) {
			level = Current_level_num;
			if (newdemo_read_frame_information() == -1) {
				if (!Newdemo_at_eof)
					newdemo_stop_playback();
			}
			if (level != Current_level_num) {
				if (newdemo_read_frame_information() == -1) {
					if (!Newdemo_at_eof)
						newdemo_stop_playback();
				}
			}
			Newdemo_vcr_state = ND_STATE_PAUSED;
		} else
			Newdemo_vcr_state = ND_STATE_PAUSED;
	}
	else {

//  First, uptate the total playback time to date.  Then we check to see
//  if we need to change the playback style to interpolate frames or
//  skip frames based on where the playback time is relative to the
//  recorded time.

		if (NewdemoFrameCount <= 0)
			nd_playback_total = nd_recorded_total; // baseline total playback time
		else
			nd_playback_total += FrameTime;

		if (playback_style == NORMAL_PLAYBACK)
		{
			if (nd_playback_total > nd_recorded_total)
				playback_style = SKIP_PLAYBACK;

			//if ((nd_playback_total * INTERPOL_FACTOR) < nd_recorded_total) // no matter how we look at it: this does not make ANY sense!
			if (nd_recorded_total > 0 && nd_recorded_time > 0)
			{
				playback_style = INTERPOLATE_PLAYBACK;
				nd_playback_total = nd_recorded_total + FrameTime; // baseline playback time
				base_interpol_time = nd_recorded_total;
				d_recorded = nd_recorded_time; // baseline delta recorded
			}
		}

		if ((playback_style == INTERPOLATE_PLAYBACK) && Newdemo_do_interpolate) {
			fix d_play = 0;

			if (nd_recorded_total - nd_playback_total < FrameTime) {
				d_recorded = nd_recorded_total - nd_playback_total;

				while (nd_recorded_total - nd_playback_total < FrameTime) {
					object *cur_objs;
					int i, j, num_objs, level;

					num_objs = Highest_object_index;
					cur_objs = (object *)d_malloc(sizeof(object) * (num_objs + 1));
					if (cur_objs == NULL) {
						Warning ("Couldn't get %d bytes for objects in interpolate playback\n", sizeof(object) * num_objs);
						break;
					}
					for (i = 0; i <= num_objs; i++)
						memcpy(&(cur_objs[i]), &(Objects[i]), sizeof(object));

					level = Current_level_num;
					if (newdemo_read_frame_information() == -1) {
						d_free(cur_objs);
						newdemo_stop_playback();
						return;
					}
					if (level != Current_level_num) {
						d_free(cur_objs);
						if (newdemo_read_frame_information() == -1)
							newdemo_stop_playback();
						break;
					}

//  for each new object in the frame just read in, determine if there is
//  a corresponding object that we have been interpolating.  If so, then
//  copy that interpolated object to the new Objects array so that the
//  interpolated position and orientation can be preserved.

					for (i = 0; i <= num_objs; i++) {
						for (j = 0; j <= Highest_object_index; j++) {
							if (cur_objs[i].signature == Objects[j].signature) {
								memcpy(&(Objects[j].orient), &(cur_objs[i].orient), sizeof(vms_matrix));
								memcpy(&(Objects[j].pos), &(cur_objs[i].pos), sizeof(vms_vector));
								break;
							}
						}
					}
					d_free(cur_objs);
					d_recorded += nd_recorded_time;
					base_interpol_time = nd_playback_total - FrameTime;
				}
			}

			d_play = nd_playback_total - base_interpol_time;
			interpolate_frame(d_play, d_recorded);
			return;
		}
		else {
			if (newdemo_read_frame_information() == -1) {
				newdemo_stop_playback();
				return;
			}
			if (playback_style == SKIP_PLAYBACK) {
				while (nd_playback_total > nd_recorded_total) {
					if (newdemo_read_frame_information() == -1) {
						newdemo_stop_playback();
						return;
					}
				}
			}
		}
	}
}

void newdemo_start_recording()
{
	Newdemo_size = PHYSFSX_getFreeDiskSpace();
	con_printf(CON_VERBOSE, "Free space = %d\n", Newdemo_size);

	Newdemo_size -= 100000;

	if ((Newdemo_size+100000) <  2000000000) {
		if (((int)(Newdemo_size)) < 500000) {
#if !(defined(__APPLE__) || defined(macintosh))
			nm_messagebox(NULL, 1, TXT_OK, TXT_DEMO_NO_SPACE);
#else
			nm_messagebox(NULL, 1, TXT_OK, "Not enough space on current\ndrive to start demo recording.");
#endif
			return;
		}
	}

	Newdemo_num_written = 0;
	Newdemo_no_space=0;
	Newdemo_state = ND_STATE_RECORDING;

	if (!cfexist(DEMO_DIR)) //dir doesn't exist?
		PHYSFS_mkdir(DEMO_DIR); //try making directory

	outfile = PHYSFSX_openWriteBuffered(DEMO_FILENAME);

	if (outfile == NULL)
	{
		nm_messagebox(NULL, 1, TXT_OK, "Cannot open demo temp file");
		Newdemo_state = ND_STATE_NORMAL;
	}
	else
		newdemo_record_start_demo();
}

char demoname_allowed_chars[] = "azAZ09__--";
void newdemo_stop_recording()
{
	newmenu_item m[6];
	int l, exit;
	static char filename[15] = "", *s;
	static ubyte tmpcnt = 0;
	ubyte cloaked = 0;
	char fullname[15+FILENAME_LEN] = DEMO_DIR;
#ifndef SHAREWARE
	unsigned short byte_count = 0;
#endif

	exit = 0;

	nd_write_byte(ND_EVENT_EOF);
	nd_write_short(frame_bytes_written - 1);
	if (Game_mode & GM_MULTI) {
		for (l = 0; l < N_players; l++) {
			if (Players[l].flags & PLAYER_FLAGS_CLOAKED)
				cloaked |= (1 << l);
		}
		nd_write_byte(cloaked);
		nd_write_byte(ND_EVENT_EOF);
	} else {
		nd_write_short(ND_EVENT_EOF);
	}
	nd_write_short(ND_EVENT_EOF);
	nd_write_int(ND_EVENT_EOF);

#ifndef SHAREWARE
	byte_count += 10; // from frame_bytes_written
 
	nd_write_byte((sbyte)(f2ir(Players[Player_num].energy)));
	nd_write_byte((sbyte)(f2ir(Players[Player_num].shields)));
	nd_write_int(Players[Player_num].flags); // be sure players flags are set
	nd_write_byte((sbyte)Primary_weapon);
	nd_write_byte((sbyte)Secondary_weapon);
	byte_count += 8;

	for (l = 0; l < MAX_PRIMARY_WEAPONS; l++)
		nd_write_short((short)Players[Player_num].primary_ammo[l]);

	for (l = 0; l < MAX_SECONDARY_WEAPONS; l++)
		nd_write_short((short)Players[Player_num].secondary_ammo[l]);
	byte_count += (sizeof(short) * (MAX_PRIMARY_WEAPONS + MAX_SECONDARY_WEAPONS));

	nd_write_byte(Players[Player_num].laser_level);
	byte_count++;

	if (Game_mode & GM_MULTI) {
		nd_write_byte((sbyte)N_players);
		byte_count++;
		for (l = 0; l < N_players; l++) {
			nd_write_string(Players[l].callsign);
			byte_count += (strlen(Players[l].callsign) + 2);
			nd_write_byte(Players[l].connected);
			if (Game_mode & GM_MULTI_COOP) {
				nd_write_int(Players[l].score);
				byte_count += 5;
			} else {
				nd_write_short((short)Players[l].net_killed_total);
				nd_write_short((short)Players[l].net_kills_total);
				byte_count += 5;
			}
		}
	} else {
		nd_write_int(Players[Player_num].score);
		byte_count += 4;
	}
	nd_write_short(byte_count);
#endif

	nd_write_byte(Current_level_num);
	nd_write_byte(ND_EVENT_EOF);

	l = PHYSFS_tell(outfile);
	PHYSFS_close(outfile);
	outfile = NULL;
	Newdemo_state = ND_STATE_NORMAL;
	gr_palette_load( gr_palette );

	if (filename[0] != '\0') {
 		int num, i = strlen(filename) - 1;
 		char newfile[15];

 		while (isdigit(filename[i])) {
 			i--;
 			if (i == -1)
 				break;
 		}
 		i++;
 		num = atoi(&(filename[i]));
 		num++;
 		filename[i] = '\0';
 		sprintf (newfile, "%s%d", filename, num);
 		strncpy(filename, newfile, 8);
 		filename[8] = '\0';
 	}

try_again:
	;

	Newmenu_allowed_chars = demoname_allowed_chars;
	if (!Newdemo_no_space) {
		m[0].type=NM_TYPE_INPUT; m[0].text_len = 8; m[0].text = filename;
		exit = newmenu_do( NULL, TXT_SAVE_DEMO_AS, 1, &(m[0]), NULL );
	} else if (Newdemo_no_space == 1) {
		m[ 0].type = NM_TYPE_TEXT; m[ 0].text = TXT_DEMO_SAVE_BAD;
		m[ 1].type = NM_TYPE_INPUT;m[ 1].text_len = 8; m[1].text = filename;
		exit = newmenu_do( NULL, NULL, 2, m, NULL );
	} else if (Newdemo_no_space == 2) {
		m[ 0].type = NM_TYPE_TEXT; m[ 0].text = TXT_DEMO_SAVE_NOSPACE;
		m[ 1].type = NM_TYPE_INPUT;m[ 1].text_len = 8; m[1].text = filename;
		exit = newmenu_do( NULL, NULL, 2, m, NULL );
	}
	Newmenu_allowed_chars = NULL;

	if (exit == -2) { // got bumped out from network menu
		char save_file[7+FILENAME_LEN];

		if (filename[0] != '\0') {
			strcpy(save_file, DEMO_DIR);
			strcat(save_file, filename);
			strcat(save_file, ".dem");
		} else 
			sprintf (save_file, "%stmp%d.dem", DEMO_DIR, tmpcnt++);
		remove(save_file);
		PHYSFSX_rename(DEMO_FILENAME, save_file);
		return;
	}
	if (exit == -1) { // pressed ESC
		PHYSFS_delete(DEMO_FILENAME); // might as well remove the file
		return; // return without doing anything
	}
	
	if (filename[0]==0) //null string
		goto try_again;

	//check to make sure name is ok
	for (s=filename;*s;s++)
		if (!isalnum(*s) && *s!='_') {
			nm_messagebox1(NULL, NULL,1,TXT_CONTINUE, TXT_DEMO_USE_LETTERS);
			goto try_again;
		}

	if (Newdemo_no_space)
		strcat(fullname, m[1].text);
	else
		strcat(fullname, m[0].text);
	strcat(fullname, ".dem");
	PHYSFS_delete(fullname);
	PHYSFSX_rename(DEMO_FILENAME, fullname);
}

//returns the number of demo files on the disk
int newdemo_count_demos()
{
	char **find, **i;
	int NumFiles=0;

	find = PHYSFS_enumerateFiles(DEMO_DIR);

	for (i = find; *i != NULL; i++)
		NumFiles++;

	PHYSFS_freeList(find);

	return NumFiles;
}

void newdemo_start_playback(char * filename)
{
	char **find = NULL, **i;
	int rnd_demo = 0;
	char filename2[PATH_MAX+FILENAME_LEN] = DEMO_DIR;

#ifdef NETWORK
	change_playernum_to(0);
#endif
	First_time_playback=1;
	JasonPlaybackTotal=0;

	if (filename)
		strcat(filename2, filename);
	else
	{
		// Randomly pick a filename
		int NumFiles = 0, RandFileNum;
		rnd_demo = 1;

		NumFiles = newdemo_count_demos();

		if ( NumFiles == 0 ) {
			return;     // No files found!
		}
		RandFileNum = d_rand() % NumFiles;
		NumFiles = 0;

		find = PHYSFS_enumerateFiles(DEMO_DIR);

		for (i = find; *i != NULL; i++)
		{
			if (NumFiles == RandFileNum)
			{
				strcat(filename2, *i);

				break;
			}
			NumFiles++;
		}
		PHYSFS_freeList(find);

		if (NumFiles > RandFileNum)
			return;

		// if in random mode, PhysFS may look for all possible files, so check if filename actually points to be a demo file...
		if (strnicmp(".dem",&filename2[strlen(filename2)-4],4))
			return;
	}

	infile = PHYSFSX_openReadBuffered(filename2);

	if (infile==NULL) {
		return;
	}

	nd_bad_read = 0;
#ifdef NETWORK
	change_playernum_to(0);                 // force playernum to 0
#endif
	strncpy(nd_save_callsign, Players[Player_num].callsign, CALLSIGN_LEN);
	Players[Player_num].lives=0;
	Viewer = ConsoleObject = &Objects[0];   // play properly as if console player

	if (newdemo_read_demo_start(rnd_demo)) {
		PHYSFS_close(infile);
		return;
	}

	Newdemo_state = ND_STATE_PLAYBACK;
	Newdemo_vcr_state = ND_STATE_PLAYBACK;
	Newdemo_old_cockpit = PlayerCfg.CockpitMode;
	Newdemo_size = PHYSFS_fileLength(infile);
	nd_bad_read = 0;
	Newdemo_at_eof = 0;
	NewdemoFrameCount = 0;
	Newdemo_players_cloaked = 0;
	playback_style = NORMAL_PLAYBACK;
	Function_mode = FMODE_GAME;
	HUD_clear_messages();
	newdemo_playback_one_frame();       // this one loads new level
	newdemo_playback_one_frame();       // get all of the objects to renderb game
}

void newdemo_stop_playback()
{
	PHYSFS_close( infile );
	Newdemo_state = ND_STATE_NORMAL;
	#ifdef NETWORK
	change_playernum_to(0); //this is reality
	#endif
	strncpy(Players[Player_num].callsign, nd_save_callsign, CALLSIGN_LEN);
	PlayerCfg.CockpitMode = Newdemo_old_cockpit;
	Rear_view=0;
	Game_mode = GM_GAME_OVER;
	Function_mode = FMODE_MENU;
	longjmp(LeaveGame,0); // Exit game loop
}


#ifndef NDEBUG

#define BUF_SIZE 16384

void newdemo_strip_frames(char *outname, int bytes_to_strip)
{
	PHYSFS_file *outfile;
	char *buf;
	int total_size, bytes_done, read_elems, bytes_back;
	int trailer_start, loc1, loc2, stop_loc, bytes_to_read;
	short last_frame_length;

	bytes_done = 0;
	total_size = PHYSFS_fileLength(infile);
	outfile = PHYSFSX_openWriteBuffered(outname);
	if (outfile == NULL) {
		newmenu_item m[1];

		m[ 0].type = NM_TYPE_TEXT; m[ 0].text = "Can't open output file";
		newmenu_do( NULL, NULL, 1, m, NULL );
		newdemo_stop_playback();
		return;
	}
	buf = d_malloc(BUF_SIZE);
	if (buf == NULL) {
		newmenu_item m[1];

		m[ 0].type = NM_TYPE_TEXT; m[ 0].text = "Can't malloc output buffer";
		newmenu_do( NULL, NULL, 1, m, NULL );
		PHYSFS_close(outfile);
		newdemo_stop_playback();
		return;
	}
	newdemo_goto_end();
	trailer_start = PHYSFS_tell(infile);
	PHYSFS_seek(infile, PHYSFS_tell(infile) + 11);
	bytes_back = 0;
	while (bytes_back < bytes_to_strip) {
		loc1 = PHYSFS_tell(infile);
		newdemo_back_frames(1);
		loc2 = PHYSFS_tell(infile);
		bytes_back += (loc1 - loc2);
	}
	PHYSFS_seek(infile, PHYSFS_tell(infile) - 10);
	nd_read_short(&last_frame_length);
	PHYSFS_seek(infile, PHYSFS_tell(infile) - 3);
	stop_loc = PHYSFS_tell(infile);
	PHYSFS_seek(infile, 0);
	while (stop_loc > 0) {
		if (stop_loc < BUF_SIZE)
			bytes_to_read = stop_loc;
		else
			bytes_to_read = BUF_SIZE;
		read_elems = PHYSFS_read(infile, buf, 1, bytes_to_read);
		PHYSFS_write(outfile, buf, 1, read_elems);
		stop_loc -= read_elems;
	}
	stop_loc = PHYSFS_tell(outfile);
	PHYSFS_seek(infile, trailer_start);
	while ((read_elems = PHYSFS_read(infile, buf, 1, BUF_SIZE)) != 0)
		PHYSFS_write(outfile, buf, 1, read_elems);
	PHYSFS_seek(outfile, stop_loc);
	PHYSFS_seek(outfile, PHYSFS_tell(infile) + 1);
	PHYSFS_write(outfile, &last_frame_length, 2, 1);
	PHYSFS_close(outfile);
	newdemo_stop_playback();

}

#endif

void DoJasonInterpolate (fix recorded_time)
{
	fix the_delay;

	JasonPlaybackTotal+=FrameTime;

	if (!First_time_playback)
	{
		the_delay=(recorded_time - FrameTime);

		if (the_delay < f0_0)
		{
			while (JasonPlaybackTotal > nd_recorded_total)
				if (newdemo_read_frame_information() == -1)
				{
					newdemo_stop_playback();
					return;
				}
		}
	}
	First_time_playback=0;
}
