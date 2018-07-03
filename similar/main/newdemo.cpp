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
 * Code to make a complete demo playback system.
 *
 */

#include <cstdlib>
#include <ctime>
#include <stdio.h>
#include <stdarg.h>
#include <errno.h>
#include <ctype.h>
#include <type_traits>

#include "u_mem.h"
#include "inferno.h"
#include "game.h"
#include "gr.h"
#include "bm.h"
#include "3d.h"
#include "weapon.h"
#include "segment.h"
#include "strutil.h"
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
#include "robot.h"
#include "fireball.h"
#include "laser.h"
#include "dxxerror.h"
#include "ai.h"
#include "hostage.h"
#include "morph.h"
#include "physfs_list.h"

#include "powerup.h"
#include "fuelcen.h"

#include "sounds.h"
#include "collide.h"

#include "lighting.h"
#include "newdemo.h"
#include "gameseq.h"
#include "hudmsg.h"
#include "gamesave.h"
#include "gamemine.h"
#include "switch.h"
#include "gauges.h"
#include "player.h"
#include "vecmat.h"
#include "menu.h"
#include "args.h"
#include "palette.h"
#include "multi.h"
#include "text.h"
#include "cntrlcen.h"
#include "aistruct.h"
#include "mission.h"
#include "piggy.h"
#include "console.h"
#include "controls.h"
#include "playsave.h"

#if DXX_USE_EDITOR
#include "editor/editor.h"
#endif

#include "dxxsconf.h"
#include "dsx-ns.h"
#include "compiler-exchange.h"
#include "compiler-range_for.h"
#include "partial_range.h"

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
#if defined(DXX_BUILD_DESCENT_I)
#define ND_EVENT_LASER_LEVEL			42	// with old/new level
#define ND_EVENT_LINK_SOUND_TO_OBJ		43	// record digi_link_sound_to_object3
#define ND_EVENT_KILL_SOUND_TO_OBJ		44	// record digi_kill_sound_linked_to_object
#elif defined(DXX_BUILD_DESCENT_II)
#define ND_EVENT_LASER_LEVEL			42	// no data
#define ND_EVENT_PLAYER_AFTERBURNER		43	// followed by byte old ab, current ab
#define ND_EVENT_CLOAKING_WALL			44	// info changing while wall cloaking
#define ND_EVENT_CHANGE_COCKPIT			45	// change the cockpit
#define ND_EVENT_START_GUIDED			46	// switch to guided view
#define ND_EVENT_END_GUIDED			47	// stop guided view/return to ship
#define ND_EVENT_SECRET_THINGY			48	// 0/1 = secret exit functional/non-functional
#define ND_EVENT_LINK_SOUND_TO_OBJ		49	// record digi_link_sound_to_object3
#define ND_EVENT_KILL_SOUND_TO_OBJ		50	// record digi_kill_sound_linked_to_object
#endif

#define NORMAL_PLAYBACK 			0
#define SKIP_PLAYBACK				1
#define INTERPOLATE_PLAYBACK			2
#define INTERPOL_FACTOR				(F1_0 + (F1_0/5))

#if defined(DXX_BUILD_DESCENT_I)
#define DEMO_VERSION_SHAREWARE		5
#define DEMO_VERSION				13
#define DEMO_GAME_TYPE_SHAREWARE	1
#define DEMO_GAME_TYPE				2
#elif defined(DXX_BUILD_DESCENT_II)
#define DEMO_VERSION				15      // last D1 version was 13
#define DEMO_GAME_TYPE				3       // 1 was shareware, 2 registered
#endif

#define DEMO_FILENAME				DEMO_DIR "tmpdemo.dem"

#define DEMO_MAX_LEVELS				29

const array<file_extension_t, 1> demo_file_extensions{{DEMO_EXT}};

// In- and Out-files
static RAIIPHYSFS_File infile;
static RAIIPHYSFS_File outfile;

// Some globals
int Newdemo_state = 0;
int Newdemo_game_mode = 0;
int Newdemo_vcr_state = 0;
int Newdemo_show_percentage=1;
sbyte Newdemo_do_interpolate = 1;
int Newdemo_num_written;
#if defined(DXX_BUILD_DESCENT_II)
ubyte DemoDoRight=0,DemoDoLeft=0;
object DemoRightExtra,DemoLeftExtra;

static void nd_render_extras (ubyte which,const vcobjptr_t obj);
#endif

// local var used for swapping endian demos
static int swap_endian = 0;

// playback variables
static unsigned int nd_playback_v_demosize;
static callsign_t nd_playback_v_save_callsign;
static sbyte nd_playback_v_at_eof;
static sbyte nd_playback_v_cntrlcen_destroyed = 0;
static sbyte nd_playback_v_bad_read;
static int nd_playback_v_framecount;
static fix nd_playback_total, nd_recorded_total, nd_recorded_time;
static sbyte nd_playback_v_style;
static ubyte nd_playback_v_dead = 0, nd_playback_v_rear = 0;
#if defined(DXX_BUILD_DESCENT_II)
static ubyte nd_playback_v_guided = 0;
int nd_playback_v_juststarted=0;
#endif

// record variables
#define REC_DELAY F1_0/20
static int nd_record_v_start_frame = -1;
static int nd_record_v_frame_number = -1;
static short nd_record_v_framebytes_written = 0;
static int nd_record_v_recordframe = 1;
static fix64 nd_record_v_recordframe_last_time = 0;
static sbyte nd_record_v_no_space;
#if defined(DXX_BUILD_DESCENT_II)
static int nd_record_v_juststarted = 0;
static array<sbyte, MAX_OBJECTS> nd_record_v_objs,
	nd_record_v_viewobjs;
static array<sbyte, 32> nd_record_v_rendering;
static fix nd_record_v_player_afterburner = -1;
#endif
static int nd_record_v_player_energy = -1;
static int nd_record_v_player_shields = -1;
static uint nd_record_v_player_flags = -1;
static int nd_record_v_weapon_type = -1;
static int nd_record_v_weapon_num = -1;
static fix nd_record_v_homing_distance = -1;
static int nd_record_v_primary_ammo = -1;
static int nd_record_v_secondary_ammo = -1;

namespace dsx {
static void newdemo_record_oneframeevent_update(int wallupdate);
}
#if defined(DXX_BUILD_DESCENT_I)
static int shareware = 0;	// reading shareware demo?
#elif defined(DXX_BUILD_DESCENT_II)
constexpr std::integral_constant<int, 0> shareware{};
#endif

int newdemo_get_percent_done()	{
	if ( Newdemo_state == ND_STATE_PLAYBACK ) {
		return (PHYSFS_tell(infile) * 100) / nd_playback_v_demosize;
	}
	if ( Newdemo_state == ND_STATE_RECORDING ) {
		return PHYSFS_tell(outfile);
	}
	return 0;
}

#define VEL_PRECISION 12

static void my_extract_shortpos(object_base &objp, const shortpos *const spp)
{
	auto sp = spp->bytemat;
	objp.orient.rvec.x = *sp++ << MATRIX_PRECISION;
	objp.orient.uvec.x = *sp++ << MATRIX_PRECISION;
	objp.orient.fvec.x = *sp++ << MATRIX_PRECISION;

	objp.orient.rvec.y = *sp++ << MATRIX_PRECISION;
	objp.orient.uvec.y = *sp++ << MATRIX_PRECISION;
	objp.orient.fvec.y = *sp++ << MATRIX_PRECISION;

	objp.orient.rvec.z = *sp++ << MATRIX_PRECISION;
	objp.orient.uvec.z = *sp++ << MATRIX_PRECISION;
	objp.orient.fvec.z = *sp++ << MATRIX_PRECISION;

	segnum_t segnum = spp->segment;
	objp.segnum = segnum;

	auto &v = *vcvertptr(vcsegptr(segnum)->verts[0]);
	objp.pos.x = (spp->xo << RELPOS_PRECISION) + v.x;
	objp.pos.y = (spp->yo << RELPOS_PRECISION) + v.y;
	objp.pos.z = (spp->zo << RELPOS_PRECISION) + v.z;

	objp.mtype.phys_info.velocity.x = (spp->velx << VEL_PRECISION);
	objp.mtype.phys_info.velocity.y = (spp->vely << VEL_PRECISION);
	objp.mtype.phys_info.velocity.z = (spp->velz << VEL_PRECISION);
}

static int _newdemo_read( void *buffer, int elsize, int nelem )
{
	int num_read;
	num_read = (PHYSFS_read)(infile, buffer, elsize, nelem);
	if (num_read < nelem || PHYSFS_eof(infile))
		nd_playback_v_bad_read = -1;

	return num_read;
}

template <typename T>
static typename std::enable_if<std::is_integral<T>::value, int>::type newdemo_read( T *buffer, int elsize, int nelem )
{
	return _newdemo_read(buffer, elsize, nelem);
}

icobjptridx_t newdemo_find_object(object_signature_t signature)
{
	range_for (const auto &&objp, vcobjptridx)
	{
		if ( (objp->type != OBJ_NONE) && (objp->signature == signature))
			return objp;
	}
	return object_none;
}

static int _newdemo_write(const void *buffer, int elsize, int nelem )
{
	int num_written, total_size;

	if (unlikely(nd_record_v_no_space))
		return -1;

	total_size = elsize * nelem;
	nd_record_v_framebytes_written += total_size;
	Newdemo_num_written += total_size;
	Assert(outfile);
	num_written = (PHYSFS_write)(outfile, buffer, elsize, nelem);

	if (likely(num_written == nelem))
		return num_written;

	nd_record_v_no_space=2;
	newdemo_stop_recording();
	return -1;
}

template <typename T>
static typename std::enable_if<std::is_integral<T>::value, int>::type newdemo_write(const T *buffer, int elsize, int nelem )
{
	return _newdemo_write(buffer, elsize, nelem);
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

static void nd_write_string(const char *str)
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

static void nd_write_vector(const vms_vector &v)
{
	nd_write_fix(v.x);
	nd_write_fix(v.y);
	nd_write_fix(v.z);
}

static void nd_write_angvec(const vms_angvec &v)
{
	nd_write_fixang(v.p);
	nd_write_fixang(v.b);
	nd_write_fixang(v.h);
}

static void nd_write_shortpos(const vcobjptr_t obj)
{
	shortpos sp;
	ubyte render_type;

	create_shortpos_native(vcsegptr, vcvertptr, &sp, obj);

	render_type = obj->render_type;
	if (((render_type == RT_POLYOBJ) || (render_type == RT_HOSTAGE) || (render_type == RT_MORPH)) || (obj->type == OBJ_CAMERA)) {
		uint8_t mask = 0;
		range_for (auto &i, sp.bytemat)
		{
			nd_write_byte(i);
			mask |= i;
		}
		if (!mask)
		{
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

static void nd_read_byte(int8_t *const b)
{
	newdemo_read(b, 1, 1);
}

static void nd_read_byte(uint8_t *const b)
{
	nd_read_byte(reinterpret_cast<int8_t *>(b));
}

static void nd_read_short(int16_t *const s)
{
	newdemo_read(s, 2, 1);
	if (swap_endian)
		*s = SWAPSHORT(*s);
}

static void nd_read_short(uint16_t *const s)
{
	nd_read_short(reinterpret_cast<int16_t *>(s));
}

static void nd_read_segnum16(segnum_t &s)
{
	short i;
	nd_read_short(&i);
	s = i;
}

static void nd_read_objnum16(objnum_t &o)
{
	short s;
	nd_read_short(&s);
	o = s;
}

static void nd_read_int(int *i)
{
	newdemo_read(i, 4, 1);
	if (swap_endian)
		*i = SWAPINT(*i);
}

#if defined(DXX_BUILD_DESCENT_II)
static void nd_read_int(unsigned *i)
{
	newdemo_read(i, 4, 1);
	if (swap_endian)
		*i = SWAPINT(*i);
}
#endif

static void nd_read_segnum32(segnum_t &s)
{
	int i;
	nd_read_int(&i);
	s = i;
}

static void nd_read_objnum32(objnum_t &o)
{
	int i;
	nd_read_int(&i);
	o = i;
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
	if (swap_endian)
		*f = SWAPINT(*f);
}

static void nd_read_fixang(fixang *f)
{
	newdemo_read(f, sizeof(fixang), 1);
	if (swap_endian)
		*f = SWAPSHORT(*f);
}

static void nd_read_vector(vms_vector &v)
{
	nd_read_fix(&v.x);
	nd_read_fix(&v.y);
	nd_read_fix(&v.z);
}

static void nd_read_angvec(vms_angvec &v)
{
	nd_read_fixang(&v.p);
	nd_read_fixang(&v.b);
	nd_read_fixang(&v.h);
}

static void nd_read_shortpos(object_base &obj)
{
	ubyte render_type;

	shortpos sp{};

	render_type = obj.render_type;
	if ((render_type == RT_POLYOBJ || render_type == RT_HOSTAGE || render_type == RT_MORPH) || obj.type == OBJ_CAMERA)
	{
		range_for (auto &i, sp.bytemat)
			nd_read_byte(&i);
	}

	nd_read_short(&sp.xo);
	nd_read_short(&sp.yo);
	nd_read_short(&sp.zo);
	nd_read_short(&sp.segment);
	nd_read_short(&sp.velx);
	nd_read_short(&sp.vely);
	nd_read_short(&sp.velz);

	my_extract_shortpos(obj, &sp);
	if (obj.type == OBJ_FIREBALL && get_fireball_id(obj) == VCLIP_MORPHING_ROBOT && render_type == RT_FIREBALL && obj.control_type == CT_EXPLOSION)
		extract_orient_from_segment(&obj.orient, vcsegptr(obj.segnum));

}

object *prev_obj=NULL;      //ptr to last object read in

namespace dsx {
static void nd_read_object(const vmobjptridx_t obj)
{
	short shortsig = 0;
	const auto &pl_info = get_local_plrobj().ctype.player_info;
	const auto saved = (&pl_info == &obj->ctype.player_info)
		? std::pair<fix, player_info>(obj->shields, pl_info)
		: std::pair<fix, player_info>(0, {});

	*obj = {};
	obj->shields = saved.first;
	obj->ctype.player_info = saved.second;
	obj->next = obj->prev = object_none;
	obj->segnum = segment_none;

	/*
	 * Do render type first, since with render_type == RT_NONE, we
	 * blow by all other object information
	 */
	nd_read_byte(&obj->render_type);
	{
		uint8_t object_type;
		nd_read_byte(&object_type);
		set_object_type(*obj, object_type);
	}
	if ((obj->render_type == RT_NONE) && (obj->type != OBJ_CAMERA))
		return;

	nd_read_byte(&obj->id);
	nd_read_byte(&obj->flags);
	nd_read_short(&shortsig);
	// It's OKAY! We made sure, obj->signature is never has a value which short cannot handle!!! We cannot do this otherwise, without breaking the demo format!
	obj->signature = object_signature_t{static_cast<uint16_t>(shortsig)};
	nd_read_shortpos(obj);

#if defined(DXX_BUILD_DESCENT_II)
	if ((obj->type == OBJ_ROBOT) && (get_robot_id(obj) == SPECIAL_REACTOR_ROBOT))
		Int3();
#endif

	obj->attached_obj = object_none;

	switch(obj->type) {

	case OBJ_HOSTAGE:
		obj->control_type = CT_POWERUP;
		obj->movement_type = MT_NONE;
		obj->size = HOSTAGE_SIZE;
		break;

	case OBJ_ROBOT:
		obj->control_type = CT_AI;
		// (MarkA and MikeK said we should not do the crazy last secret stuff with multiple reactors...
		// This necessary code is our vindication. --MK, 2/15/96)
#if defined(DXX_BUILD_DESCENT_II)
		if (get_robot_id(obj) == SPECIAL_REACTOR_ROBOT)
			obj->movement_type = MT_NONE;
		else
#endif
			obj->movement_type = MT_PHYSICS;
		obj->size = Polygon_models[Robot_info[get_robot_id(obj)].model_num].rad;
		obj->rtype.pobj_info.model_num = Robot_info[get_robot_id(obj)].model_num;
		obj->rtype.pobj_info.subobj_flags = 0;
		obj->ctype.ai_info.CLOAKED = (Robot_info[get_robot_id(obj)].cloak_type?1:0);
		break;

	case OBJ_POWERUP:
		obj->control_type = CT_POWERUP;
		{
			uint8_t movement_type;
			nd_read_byte(&movement_type);        // might have physics movement
			set_object_movement_type(*obj, movement_type);
		}
		obj->size = Powerup_info[get_powerup_id(obj)].size;
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
		nd_read_byte(&obj->control_type);
		{
			uint8_t movement_type;
			nd_read_byte(&movement_type);
			set_object_movement_type(*obj, movement_type);
		}
		nd_read_fix(&(obj->size));
		break;
	}


	nd_read_vector(obj->last_pos);
	if ((obj->type == OBJ_WEAPON) && (obj->render_type == RT_WEAPON_VCLIP))
		nd_read_fix(&(obj->lifeleft));
	else {
		sbyte b;

		// MWA old way -- won't work with big endian machines       nd_read_byte((uint8_t *)&(obj->lifeleft));
		nd_read_byte(&b);
		obj->lifeleft = static_cast<fix>(b);
		if (obj->lifeleft == -1)
			obj->lifeleft = IMMORTAL_TIME;
		else
			obj->lifeleft = obj->lifeleft << 12;
	}

	if ((obj->type == OBJ_ROBOT) && !shareware) {
		if (Robot_info[get_robot_id(obj)].boss_flag) {
			sbyte cloaked;

			nd_read_byte(&cloaked);
			obj->ctype.ai_info.CLOAKED = cloaked;
		}
	}

	switch (obj->movement_type) {

	case MT_PHYSICS:
		nd_read_vector(obj->mtype.phys_info.velocity);
		nd_read_vector(obj->mtype.phys_info.thrust);
		break;

	case MT_SPINNING:
		nd_read_vector(obj->mtype.spin_rate);
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
		nd_read_objnum16(obj->ctype.expl_info.delete_objnum);

		obj->ctype.expl_info.next_attach = obj->ctype.expl_info.prev_attach = obj->ctype.expl_info.attach_parent = object_none;

		if (obj->flags & OF_ATTACHED) {     //attach to previous object
			Assert(prev_obj!=NULL);
			if (prev_obj->control_type == CT_EXPLOSION) {
				if (prev_obj->flags & OF_ATTACHED && prev_obj->ctype.expl_info.attach_parent!=object_none)
					obj_attach(Objects, Objects.vmptridx(prev_obj->ctype.expl_info.attach_parent), obj);
				else
					obj->flags &= ~OF_ATTACHED;
			}
			else
				obj_attach(Objects, Objects.vmptridx(prev_obj), obj);
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
		int tmo;

		if ((obj->type != OBJ_ROBOT) && (obj->type != OBJ_PLAYER) && (obj->type != OBJ_CLUTTER)) {
			nd_read_int(&(obj->rtype.pobj_info.model_num));
			nd_read_int(&(obj->rtype.pobj_info.subobj_flags));
		}

		if ((obj->type != OBJ_PLAYER) && (obj->type != OBJ_DEBRIS))
#if 0
			range_for (auto &i, obj->pobj_info.anim_angles)
				nd_read_angvec(&(i));
#endif
		range_for (auto &i, partial_range(obj->rtype.pobj_info.anim_angles, Polygon_models[obj->rtype.pobj_info.model_num].n_models))
			nd_read_angvec(i);

		nd_read_int(&tmo);

#if !DXX_USE_EDITOR
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
		nd_read_byte(&obj->rtype.vclip_info.framenum);
		break;

	case RT_LASER:
		break;

	default:
		Int3();

	}

	prev_obj = obj;
}
}

namespace dsx {
static void nd_write_object(const vcobjptr_t obj)
{
	int life;
	short shortsig = 0;

#if defined(DXX_BUILD_DESCENT_II)
	if ((obj->type == OBJ_ROBOT) && (get_robot_id(obj) == SPECIAL_REACTOR_ROBOT))
		Int3();
#endif

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
	shortsig = obj->signature.get();  // It's OKAY! We made sure, obj->signature is never has a value which short cannot handle!!! We cannot do this otherwise, without breaking the demo format!
	nd_write_short(shortsig);
	nd_write_shortpos(obj);

	if ((obj->type != OBJ_HOSTAGE) && (obj->type != OBJ_ROBOT) && (obj->type != OBJ_PLAYER) && (obj->type != OBJ_POWERUP) && (obj->type != OBJ_CLUTTER)) {
		nd_write_byte(obj->control_type);
		nd_write_byte(obj->movement_type);
		nd_write_fix(obj->size);
	}
	if (obj->type == OBJ_POWERUP)
		nd_write_byte(obj->movement_type);

	nd_write_vector(obj->last_pos);

	if ((obj->type == OBJ_WEAPON) && (obj->render_type == RT_WEAPON_VCLIP))
		nd_write_fix(obj->lifeleft);
	else {
		life = static_cast<int>(obj->lifeleft);
		life = life >> 12;
		if (life > 255)
			life = 255;
		nd_write_byte(static_cast<uint8_t>(life));
	}

	if (obj->type == OBJ_ROBOT) {
		if (Robot_info[get_robot_id(obj)].boss_flag) {
			if (GameTime64 > Boss_cloak_start_time &&
				GameTime64 < (Boss_cloak_start_time + Boss_cloak_duration))
				nd_write_byte(1);
			else
				nd_write_byte(0);
		}
	}

	switch (obj->movement_type) {

	case MT_PHYSICS:
		nd_write_vector(obj->mtype.phys_info.velocity);
		nd_write_vector(obj->mtype.phys_info.thrust);
		break;

	case MT_SPINNING:
		nd_write_vector(obj->mtype.spin_rate);
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
		if ((obj->type != OBJ_ROBOT) && (obj->type != OBJ_PLAYER) && (obj->type != OBJ_CLUTTER)) {
			nd_write_int(obj->rtype.pobj_info.model_num);
			nd_write_int(obj->rtype.pobj_info.subobj_flags);
		}

		if ((obj->type != OBJ_PLAYER) && (obj->type != OBJ_DEBRIS))
#if 0
			range_for (auto &i, obj->pobj_info.anim_angles)
				nd_write_angvec(&i);
#endif
		range_for (auto &i, partial_const_range(obj->rtype.pobj_info.anim_angles, Polygon_models[obj->rtype.pobj_info.model_num].n_models))
			nd_write_angvec(i);

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
}

static void nd_record_meta(char (&buf)[7], const char *s)
{
	for (auto l = strlen(s) + 1; l;)
	{
		const auto n = std::min(l, sizeof(buf) - 3);
		std::copy_n(s, n, &buf[3]);
		s += n;
		if (!(l -= n))
			/* On final fragment, overwrite any trailing garbage from
			 * previous iteration.
			 */
			std::fill_n(&buf[3 + n], sizeof(buf) - 3 - n, 0);
		newdemo_write(buf, 1, sizeof(buf));
	}
}

static void nd_rdt(char (&buf)[7])
{
	time_t t;
	if (time(&t) == static_cast<time_t>(-1))
		return;
	/* UTC for easy comparison among demos from players in different
	 * timezones
	 */
	if (const auto g = gmtime(&t))
		if (const auto st = asctime(g))
			nd_record_meta(buf, st);
}

static void nd_rbe()
{
	char buf[7]{ND_EVENT_PALETTE_EFFECT, 0x80};
	newdemo_write(buf, 1, sizeof(buf));
	++buf[2];
	nd_rdt(buf);
	++buf[2];
#define DXX_RBE(A)	\
	extern const char g_descent_##A[];	\
	nd_record_meta(buf, g_descent_##A);
	DXX_RBE(CPPFLAGS);
	DXX_RBE(CXX);
	DXX_RBE(CXXFLAGS);
	DXX_RBE(CXX_version);
	DXX_RBE(LINKFLAGS);
	++buf[2];
	DXX_RBE(build_datetime);
	DXX_RBE(git_diffstat);
	DXX_RBE(git_status);
	DXX_RBE(version);
	std::fill_n(&buf[1], sizeof(buf) - 1, 0);
	newdemo_write(buf, 1, sizeof(buf));
}

namespace dsx {
void newdemo_record_start_demo()
{
	auto &player_info = get_local_plrobj().ctype.player_info;
	nd_record_v_recordframe_last_time=GameTime64-REC_DELAY; // make sure first frame is recorded!

	pause_game_world_time p;
	nd_write_byte(ND_EVENT_START_DEMO);
	nd_write_byte(DEMO_VERSION);
	nd_write_byte(DEMO_GAME_TYPE);
	nd_write_fix(0); // NOTE: This is supposed to write GameTime (in fix). Since our GameTime64 is fix64 and the demos do not NEED this time actually, just write 0.

	if (Game_mode & GM_MULTI)
		nd_write_int(Game_mode | (Player_num << 16));
	else
		// NOTE LINK TO ABOVE!!!
		nd_write_int(Game_mode);

	if (Game_mode & GM_TEAM) {
		nd_write_byte(Netgame.team_vector);
		nd_write_string(Netgame.team_name[0]);
		nd_write_string(Netgame.team_name[1]);
	}

	if (Game_mode & GM_MULTI) {
		nd_write_byte(static_cast<int8_t>(N_players));
		range_for (auto &i, partial_const_range(Players, N_players)) {
			nd_write_string(static_cast<const char *>(i.callsign));
			nd_write_byte(i.connected);

			auto &pl_info = vcobjptr(i.objnum)->ctype.player_info;
			if (Game_mode & GM_MULTI_COOP) {
				nd_write_int(pl_info.mission.score);
			} else {
				nd_write_short(pl_info.net_killed_total);
				nd_write_short(pl_info.net_kills_total);
			}
		}
	} else
		// NOTE LINK TO ABOVE!!!
		nd_write_int(get_local_plrobj().ctype.player_info.mission.score);

	nd_record_v_weapon_type = -1;
	nd_record_v_weapon_num = -1;
	nd_record_v_homing_distance = -1;
	nd_record_v_primary_ammo = -1;
	nd_record_v_secondary_ammo = -1;

	for (int i = 0; i < MAX_PRIMARY_WEAPONS; i++)
		nd_write_short(i == primary_weapon_index_t::VULCAN_INDEX ? player_info.vulcan_ammo : 0);

	range_for (auto &i, player_info.secondary_ammo)
		nd_write_short(i);

	nd_write_byte(static_cast<sbyte>(player_info.laser_level));

//  Support for missions added here

	nd_write_string(Current_mission_filename);

	nd_write_byte(nd_record_v_player_energy = static_cast<int8_t>(f2ir(player_info.energy)));
	nd_write_byte(nd_record_v_player_shields = static_cast<int8_t>(f2ir(get_local_plrobj().shields)));
	nd_write_int(nd_record_v_player_flags = player_info.powerup_flags.get_player_flags());        // be sure players flags are set
	nd_write_byte(static_cast<int8_t>(static_cast<primary_weapon_index_t>(player_info.Primary_weapon)));
	nd_write_byte(static_cast<int8_t>(static_cast<secondary_weapon_index_t>(player_info.Secondary_weapon)));
	nd_record_v_start_frame = nd_record_v_frame_number = 0;
#if defined(DXX_BUILD_DESCENT_II)
	nd_record_v_player_afterburner = 0;
	nd_record_v_juststarted=1;
#endif
	nd_rbe();
	newdemo_set_new_level(Current_level_num);
#if defined(DXX_BUILD_DESCENT_I)
	newdemo_record_oneframeevent_update(1);
#elif defined(DXX_BUILD_DESCENT_II)
	newdemo_record_oneframeevent_update(0);
#endif
}
}

namespace dsx {
void newdemo_record_start_frame(fix frame_time )
{
	if (nd_record_v_no_space)
	{
		// Shouldn't happen - we should have stopped demo recording,
		// in which case this function shouldn't have been called in the first place
		Int3();
		return;
	}

	// Make demo recording waste a bit less space.
	// First check if if at least REC_DELAY has passed since last recorded frame. If yes, record frame and set nd_record_v_recordframe true.
	// nd_record_v_recordframe will be used for various other frame-by-frame events to drop some unnecessary bytes.
	// frame_time must be modified to get the right playback speed.
	if (nd_record_v_recordframe_last_time > GameTime64)
		nd_record_v_recordframe_last_time=GameTime64-REC_DELAY;

	if (nd_record_v_recordframe_last_time + REC_DELAY <= GameTime64 || frame_time >= REC_DELAY)
	{
		if (frame_time < REC_DELAY)
			frame_time = REC_DELAY;
		nd_record_v_recordframe_last_time = GameTime64-(GameTime64-(nd_record_v_recordframe_last_time + REC_DELAY));
		nd_record_v_recordframe=1;

		pause_game_world_time p;
#if defined(DXX_BUILD_DESCENT_II)

		for (int i=0;i<MAX_OBJECTS;i++)
		{
			nd_record_v_objs[i]=0;
			nd_record_v_viewobjs[i]=0;
		}
		range_for (auto &i, nd_record_v_rendering)
			i=0;

#endif
		nd_record_v_frame_number -= nd_record_v_start_frame;

		nd_write_byte(ND_EVENT_START_FRAME);
		nd_write_short(nd_record_v_framebytes_written - 1);        // from previous frame
		nd_record_v_framebytes_written=3;
		nd_write_int(nd_record_v_frame_number);
		nd_record_v_frame_number++;
		nd_write_int(frame_time);
	}
	else
	{
		nd_record_v_recordframe=0;
	}

}
}

namespace dsx {
void newdemo_record_render_object(const vmobjptridx_t obj)
{
	if (!nd_record_v_recordframe)
		return;
#if defined(DXX_BUILD_DESCENT_II)
	if (nd_record_v_objs[obj])
		return;
	if (nd_record_v_viewobjs[obj])
		return;

	nd_record_v_objs[obj] = 1;
#endif
	pause_game_world_time p;
	nd_write_byte(ND_EVENT_RENDER_OBJECT);
	nd_write_object(obj);
}
}

namespace dsx {
void newdemo_record_viewer_object(const vmobjptridx_t obj)
{
	if (!nd_record_v_recordframe)
		return;
#if defined(DXX_BUILD_DESCENT_II)
	if (nd_record_v_viewobjs[obj] && (nd_record_v_viewobjs[obj]-1)==RenderingType)
		return;
	if (nd_record_v_rendering[RenderingType])
		return;
#endif

	pause_game_world_time p;
	nd_write_byte(ND_EVENT_VIEWER_OBJECT);
#if defined(DXX_BUILD_DESCENT_II)
	nd_record_v_viewobjs[obj]=RenderingType+1;
	nd_record_v_rendering[RenderingType]=1;
	nd_write_byte(RenderingType);
#endif
	nd_write_object(obj);
}
}

void newdemo_record_sound( int soundno )
{
	pause_game_world_time p;
	nd_write_byte(ND_EVENT_SOUND);
	nd_write_int( soundno );
}

void newdemo_record_sound_3d_once( int soundno, int angle, int volume )
{
	pause_game_world_time p;
	nd_write_byte( ND_EVENT_SOUND_3D_ONCE );
	nd_write_int( soundno );
	nd_write_int( angle );
	nd_write_int( volume );
}


void newdemo_record_link_sound_to_object3( int soundno, objnum_t objnum, fix max_volume, fix  max_distance, int loop_start, int loop_end )
{
	pause_game_world_time p;
	nd_write_byte( ND_EVENT_LINK_SOUND_TO_OBJ );
	nd_write_int( soundno );
	nd_write_int(vcobjptr(objnum)->signature.get());
	nd_write_int( max_volume );
	nd_write_int( max_distance );
	nd_write_int( loop_start );
	nd_write_int( loop_end );
}

void newdemo_record_kill_sound_linked_to_object(const vcobjptridx_t objp)
{
	pause_game_world_time p;
	nd_write_byte( ND_EVENT_KILL_SOUND_TO_OBJ );
	nd_write_int(objp->signature.get());
}


void newdemo_record_wall_hit_process( segnum_t segnum, int side, int damage, int playernum )
{
	pause_game_world_time p;
	nd_write_byte( ND_EVENT_WALL_HIT_PROCESS );
	nd_write_int( segnum );
	nd_write_int( side );
	nd_write_int( damage );
	nd_write_int( playernum );
}

#if defined(DXX_BUILD_DESCENT_II)
void newdemo_record_guided_start ()
{
	nd_write_byte (ND_EVENT_START_GUIDED);
}

void newdemo_record_guided_end ()
{
	nd_write_byte (ND_EVENT_END_GUIDED);
}

void newdemo_record_secret_exit_blown(int truth)
{
	pause_game_world_time p;
	nd_write_byte( ND_EVENT_SECRET_THINGY );
	nd_write_int( truth );
}

void newdemo_record_trigger( segnum_t segnum, int side, objnum_t objnum,int shot )
{
	pause_game_world_time p;
	nd_write_byte( ND_EVENT_TRIGGER );
	nd_write_int( segnum );
	nd_write_int( side );
	nd_write_int( objnum );
	nd_write_int(shot);
}
#endif

void newdemo_record_morph_frame(morph_data *md)
{
	if (!nd_record_v_recordframe)
		return;
	pause_game_world_time p;
	nd_write_byte( ND_EVENT_MORPH_FRAME );
	nd_write_object(vcobjptr(md->obj));
}

void newdemo_record_wall_toggle( segnum_t segnum, int side )
{
	pause_game_world_time p;
	nd_write_byte( ND_EVENT_WALL_TOGGLE );
	nd_write_int( segnum );
	nd_write_int( side );
}

void newdemo_record_control_center_destroyed()
{
	if (!nd_record_v_recordframe)
		return;
	pause_game_world_time p;
	nd_write_byte( ND_EVENT_CONTROL_CENTER_DESTROYED );
	nd_write_int( Countdown_seconds_left );
}

void newdemo_record_hud_message(const char * message )
{
	pause_game_world_time p;
	nd_write_byte( ND_EVENT_HUD_MESSAGE );
	nd_write_string(message);
}

void newdemo_record_palette_effect(short r, short g, short b )
{
	if (!nd_record_v_recordframe)
		return;
	pause_game_world_time p;
	nd_write_byte( ND_EVENT_PALETTE_EFFECT );
	nd_write_short( r );
	nd_write_short( g );
	nd_write_short( b );
}

void newdemo_record_player_energy(int energy)
{
	if (nd_record_v_player_energy == energy)
		return;
	pause_game_world_time p;
	nd_write_byte( ND_EVENT_PLAYER_ENERGY );
	nd_write_byte(static_cast<int8_t>(exchange(nd_record_v_player_energy, energy)));
	nd_write_byte(static_cast<int8_t>(energy));
}

#if defined(DXX_BUILD_DESCENT_II)
void newdemo_record_player_afterburner(fix afterburner)
{
	if ((nd_record_v_player_afterburner>>9) == (afterburner>>9))
		return;
	pause_game_world_time p;
	nd_write_byte( ND_EVENT_PLAYER_AFTERBURNER );
	nd_write_byte(static_cast<int8_t>(exchange(nd_record_v_player_afterburner, afterburner) >> 9));
	nd_write_byte(static_cast<int8_t>(afterburner >> 9));
}
#endif

void newdemo_record_player_shields(int shield)
{
	if (nd_record_v_player_shields == shield)
		return;
	pause_game_world_time p;
	nd_write_byte( ND_EVENT_PLAYER_SHIELD );
	nd_write_byte(static_cast<int8_t>(exchange(nd_record_v_player_shields, shield)));
	nd_write_byte(static_cast<int8_t>(shield));
}

void newdemo_record_player_flags(uint flags)
{
	if (nd_record_v_player_flags == flags)
		return;
	pause_game_world_time p;
	nd_write_byte( ND_EVENT_PLAYER_FLAGS );
	nd_write_int((static_cast<short>(exchange(nd_record_v_player_flags, flags)) << 16) | static_cast<short>(flags));
}

void newdemo_record_player_weapon(int weapon_type, int weapon_num)
{
	if (nd_record_v_weapon_type == weapon_type && nd_record_v_weapon_num == weapon_num)
		return;
	pause_game_world_time p;
	nd_write_byte( ND_EVENT_PLAYER_WEAPON );
	nd_write_byte(static_cast<int8_t>(nd_record_v_weapon_type = weapon_type));
	nd_write_byte(static_cast<int8_t>(nd_record_v_weapon_num = weapon_num));
	auto &player_info = get_local_plrobj().ctype.player_info;
	nd_write_byte(weapon_type
		? static_cast<int8_t>(static_cast<secondary_weapon_index_t>(player_info.Secondary_weapon))
		: static_cast<int8_t>(static_cast<primary_weapon_index_t>(player_info.Primary_weapon))
	);
}

void newdemo_record_effect_blowup(segnum_t segment, int side, const vms_vector &pnt)
{
	pause_game_world_time p;
	nd_write_byte (ND_EVENT_EFFECT_BLOWUP);
	nd_write_short(segment);
	nd_write_byte(static_cast<int8_t>(side));
	nd_write_vector(pnt);
}

void newdemo_record_homing_distance(fix distance)
{
	if ((nd_record_v_homing_distance>>16) == (distance>>16))
		return;
	pause_game_world_time p;
	nd_write_byte(ND_EVENT_HOMING_DISTANCE);
	nd_write_short(static_cast<short>((nd_record_v_homing_distance = distance) >> 16));
}

void newdemo_record_letterbox(void)
{
	pause_game_world_time p;
	nd_write_byte(ND_EVENT_LETTERBOX);
}

void newdemo_record_rearview(void)
{
	pause_game_world_time p;
	nd_write_byte(ND_EVENT_REARVIEW);
}

void newdemo_record_restore_cockpit(void)
{
	pause_game_world_time p;
	nd_write_byte(ND_EVENT_RESTORE_COCKPIT);
}

void newdemo_record_restore_rearview(void)
{
	pause_game_world_time p;
	nd_write_byte(ND_EVENT_RESTORE_REARVIEW);
}

void newdemo_record_wall_set_tmap_num1(short seg,ubyte side,short cseg,ubyte cside,short tmap)
{
	pause_game_world_time p;
	nd_write_byte(ND_EVENT_WALL_SET_TMAP_NUM1);
	nd_write_short(seg);
	nd_write_byte(side);
	nd_write_short(cseg);
	nd_write_byte(cside);
	nd_write_short(tmap);
}

void newdemo_record_wall_set_tmap_num2(short seg,ubyte side,short cseg,ubyte cside,short tmap)
{
	pause_game_world_time p;
	nd_write_byte(ND_EVENT_WALL_SET_TMAP_NUM2);
	nd_write_short(seg);
	nd_write_byte(side);
	nd_write_short(cseg);
	nd_write_byte(cside);
	nd_write_short(tmap);
}

void newdemo_record_multi_cloak(int pnum)
{
	pause_game_world_time p;
	nd_write_byte(ND_EVENT_MULTI_CLOAK);
	nd_write_byte(static_cast<int8_t>(pnum));
}

void newdemo_record_multi_decloak(int pnum)
{
	pause_game_world_time p;
	nd_write_byte(ND_EVENT_MULTI_DECLOAK);
	nd_write_byte(static_cast<int8_t>(pnum));
}

void newdemo_record_multi_death(int pnum)
{
	pause_game_world_time p;
	nd_write_byte(ND_EVENT_MULTI_DEATH);
	nd_write_byte(static_cast<int8_t>(pnum));
}

void newdemo_record_multi_kill(int pnum, sbyte kill)
{
	pause_game_world_time p;
	nd_write_byte(ND_EVENT_MULTI_KILL);
	nd_write_byte(static_cast<int8_t>(pnum));
	nd_write_byte(kill);
}

void newdemo_record_multi_connect(const unsigned pnum, const unsigned new_player, const char *const new_callsign)
{
	pause_game_world_time p;
	nd_write_byte(ND_EVENT_MULTI_CONNECT);
	nd_write_byte(static_cast<int8_t>(pnum));
	nd_write_byte(static_cast<int8_t>(new_player));
	if (!new_player) {
		auto &plr = *vcplayerptr(pnum);
		nd_write_string(static_cast<const char *>(plr.callsign));
		auto &player_info = vcobjptr(plr.objnum)->ctype.player_info;
		nd_write_int(player_info.net_killed_total);
		nd_write_int(player_info.net_kills_total);
	}
	nd_write_string(new_callsign);
}

void newdemo_record_multi_reconnect(int pnum)
{
	pause_game_world_time p;
	nd_write_byte(ND_EVENT_MULTI_RECONNECT);
	nd_write_byte(static_cast<int8_t>(pnum));
}

void newdemo_record_multi_disconnect(int pnum)
{
	pause_game_world_time p;
	nd_write_byte(ND_EVENT_MULTI_DISCONNECT);
	nd_write_byte(static_cast<int8_t>(pnum));
}

void newdemo_record_player_score(int score)
{
	pause_game_world_time p;
	nd_write_byte(ND_EVENT_PLAYER_SCORE);
	nd_write_int(score);
}

void newdemo_record_multi_score(const unsigned pnum, const int score)
{
	pause_game_world_time p;
	nd_write_byte(ND_EVENT_MULTI_SCORE);
	nd_write_byte(static_cast<int8_t>(pnum));
	nd_write_int(score - vcobjptr(vcplayerptr(pnum)->objnum)->ctype.player_info.mission.score);      // called before score is changed!!!!
}

void newdemo_record_primary_ammo(int new_ammo)
{
	if (nd_record_v_primary_ammo == new_ammo)
		return;
	pause_game_world_time p;
	nd_write_byte(ND_EVENT_PRIMARY_AMMO);
	nd_write_short(nd_record_v_primary_ammo < 0 ? static_cast<short>(new_ammo) : static_cast<short>(nd_record_v_primary_ammo));
	nd_write_short(static_cast<short>(nd_record_v_primary_ammo = new_ammo));
}

void newdemo_record_secondary_ammo(int new_ammo)
{
	if (nd_record_v_secondary_ammo == new_ammo)
		return;
	pause_game_world_time p;
	nd_write_byte(ND_EVENT_SECONDARY_AMMO);
	nd_write_short(nd_record_v_secondary_ammo < 0 ? static_cast<short>(new_ammo) : static_cast<short>(nd_record_v_secondary_ammo));
	nd_write_short(static_cast<short>(nd_record_v_secondary_ammo = new_ammo));
}

void newdemo_record_door_opening(segnum_t segnum, int side)
{
	pause_game_world_time p;
	nd_write_byte(ND_EVENT_DOOR_OPENING);
	nd_write_short(segnum);
	nd_write_byte(static_cast<int8_t>(side));
}

void newdemo_record_laser_level(sbyte old_level, sbyte new_level)
{
	pause_game_world_time p;
	nd_write_byte(ND_EVENT_LASER_LEVEL);
	nd_write_byte(old_level);
	nd_write_byte(new_level);
}

#if defined(DXX_BUILD_DESCENT_II)
void newdemo_record_cloaking_wall(int front_wall_num, int back_wall_num, ubyte type, ubyte state, fix cloak_value, fix l0, fix l1, fix l2, fix l3)
{
	Assert(front_wall_num <= 255 && back_wall_num <= 255);

	pause_game_world_time p;
	nd_write_byte(ND_EVENT_CLOAKING_WALL);
	nd_write_byte(front_wall_num);
	nd_write_byte(back_wall_num);
	nd_write_byte(type);
	nd_write_byte(state);
	nd_write_byte(cloak_value);
	nd_write_short(l0>>8);
	nd_write_short(l1>>8);
	nd_write_short(l2>>8);
	nd_write_short(l3>>8);
}
#endif

namespace dsx {
void newdemo_set_new_level(int level_num)
{
	pause_game_world_time p;
	nd_write_byte(ND_EVENT_NEW_LEVEL);
	nd_write_byte(static_cast<int8_t>(level_num));
	nd_write_byte(static_cast<int8_t>(Current_level_num));
#if defined(DXX_BUILD_DESCENT_II)
	if (nd_record_v_juststarted==1)
	{
		nd_write_int(Num_walls);
		range_for (const auto &&wp, vcwallptr)
		{
			auto &w = *wp;
			nd_write_byte (w.type);
			nd_write_byte (w.flags);
			nd_write_byte (w.state);

			const auto &side = vcsegptr(w.segnum)->sides[w.sidenum];
			nd_write_short (side.tmap_num);
			nd_write_short (side.tmap_num2);
			nd_record_v_juststarted=0;
		}
	}
#endif
}
}

/*
 * By design, the demo code does not record certain events when demo recording starts or ends.
 * To not break compability this function can be applied at start/end of demo recording to
 * re-record these events. It will "simulate" those events without using functions older game
 * versions cannot handle.
 */
namespace dsx {
static void newdemo_record_oneframeevent_update(int wallupdate)
{
	if (Player_dead_state != player_dead_state::no)
		newdemo_record_letterbox();
	else
		newdemo_record_restore_cockpit();

	if (Rear_view)
		newdemo_record_rearview();
	else
		newdemo_record_restore_rearview();

#if defined(DXX_BUILD_DESCENT_I)
	// This will record tmaps for all walls and properly show doors which were opened before demo recording started.
	if (wallupdate)
	{
		range_for (const auto &&wp, vcwallptr)
		{
			auto &w = *wp;
			int side;
			auto seg = &Segments[w.segnum];
			side = w.sidenum;
			// actually this is kinda stupid: when playing ther same tmap will be put on front and back side of the wall ... for doors this is stupid so just record the front side which will do for doors just fine ...
			if (auto tmap_num = seg->sides[side].tmap_num)
				newdemo_record_wall_set_tmap_num1(w.segnum,side,w.segnum,side,tmap_num);
			if (auto tmap_num2 = seg->sides[side].tmap_num2)
				newdemo_record_wall_set_tmap_num2(w.segnum,side,w.segnum,side,tmap_num2);
		}
	}
#elif defined(DXX_BUILD_DESCENT_II)
	(void)wallupdate;
	if (Viewer == Guided_missile[Player_num])
		newdemo_record_guided_start();
	else
		newdemo_record_guided_end();
#endif
}
}

enum purpose_type
{
	PURPOSE_CHOSE_PLAY = 0,
	PURPOSE_RANDOM_PLAY,
	PURPOSE_REWRITE
};

namespace dsx {
static int newdemo_read_demo_start(enum purpose_type purpose)
{
	sbyte version=0, game_type=0, c=0;
	ubyte energy=0, shield=0;
	char current_mission[9];
	fix nd_GameTime32 = 0;

	Rear_view=0;
#if defined(DXX_BUILD_DESCENT_I)
	shareware = 0;
#endif

	nd_read_byte(&c);
	if (purpose == PURPOSE_REWRITE)
		nd_write_byte(c);
	if ((c != ND_EVENT_START_DEMO) || nd_playback_v_bad_read) {
		nm_messagebox( NULL, 1, TXT_OK, "%s %s", TXT_CANT_PLAYBACK, TXT_DEMO_CORRUPT );
		return 1;
	}
	nd_read_byte(&version);
	if (purpose == PURPOSE_REWRITE)
		nd_write_byte(version);
#if defined(DXX_BUILD_DESCENT_I)
	if (version == DEMO_VERSION_SHAREWARE)
		shareware = 1;
	else if (version < DEMO_VERSION) {
		if (purpose == PURPOSE_CHOSE_PLAY) {
			nm_messagebox( NULL, 1, TXT_OK, "%s %s", TXT_CANT_PLAYBACK, TXT_DEMO_OLD );
		}
		return 1;
	}
#endif
	nd_read_byte(&game_type);
	if (purpose == PURPOSE_REWRITE)
		nd_write_byte(game_type);
#if defined(DXX_BUILD_DESCENT_I)
	if ((game_type == DEMO_GAME_TYPE_SHAREWARE) && shareware)
		;	// all good
	else if (game_type != DEMO_GAME_TYPE) {
		nm_messagebox( NULL, 1, TXT_OK, "%s %s", TXT_CANT_PLAYBACK, TXT_DEMO_OLD );

		return 1;
	}
#elif defined(DXX_BUILD_DESCENT_II)
	if (game_type < DEMO_GAME_TYPE) {
		nm_messagebox( NULL, 1, TXT_OK, "%s %s\n%s", TXT_CANT_PLAYBACK, TXT_RECORDED, "    In Descent: First Strike" );
		return 1;
	}
	if (game_type != DEMO_GAME_TYPE) {
		nm_messagebox( NULL, 1, TXT_OK, "%s %s\n%s", TXT_CANT_PLAYBACK, TXT_RECORDED, "   In Unknown Descent version" );
		return 1;
	}
	if (version < DEMO_VERSION) {
		if (purpose == PURPOSE_CHOSE_PLAY) {
			nm_messagebox( NULL, 1, TXT_OK, "%s %s", TXT_CANT_PLAYBACK, TXT_DEMO_OLD );
		}
		return 1;
	}
#endif
	nd_read_fix(&nd_GameTime32); // NOTE: Demos write GameTime in fix.
	GameTime64 = nd_GameTime32;
	nd_read_int(&Newdemo_game_mode);
#if defined(DXX_BUILD_DESCENT_II)
	if (purpose == PURPOSE_REWRITE)
	{
		nd_write_fix(nd_GameTime32);
		nd_write_int(Newdemo_game_mode);
	}

	Boss_cloak_start_time=GameTime64;
#endif

	change_playernum_to((Newdemo_game_mode >> 16) & 0x7);
	if (shareware)
	{
		if (Newdemo_game_mode & GM_TEAM)
		{
			nd_read_byte(&Netgame.team_vector);
			if (purpose == PURPOSE_REWRITE)
				nd_write_byte(Netgame.team_vector);
		}

		range_for (auto &i, Players)
		{
			auto &objp = *vmobjptr(i.objnum);
			auto &player_info = objp.ctype.player_info;
			player_info.powerup_flags &= ~(PLAYER_FLAGS_CLOAKED | PLAYER_FLAGS_INVULNERABLE);
			DXX_MAKE_VAR_UNDEFINED(player_info.cloak_time);
			DXX_MAKE_VAR_UNDEFINED(player_info.invulnerable_time);
		}
	}
	else
	{
		if (Newdemo_game_mode & GM_TEAM) {
			nd_read_byte(&Netgame.team_vector);
			nd_read_string(Netgame.team_name[0].buffer());
			nd_read_string(Netgame.team_name[1].buffer());
			if (purpose == PURPOSE_REWRITE)
			{
				nd_write_byte(Netgame.team_vector);
				nd_write_string(Netgame.team_name[0]);
				nd_write_string(Netgame.team_name[1]);
			}
		}
		if (Newdemo_game_mode & GM_MULTI) {

			if (purpose != PURPOSE_REWRITE)
				multi_new_game();
			nd_read_byte(&c);
			N_players = static_cast<int>(c);
			// changed this to above two lines -- breaks on the mac because of
			// endian issues
			//		nd_read_byte(&N_players);
			if (purpose == PURPOSE_REWRITE)
				nd_write_byte(N_players);
			range_for (auto &i, partial_range(Players, N_players)) {
				const auto &&objp = vmobjptr(i.objnum);
				auto &player_info = objp->ctype.player_info;
				player_info.powerup_flags &= ~(PLAYER_FLAGS_CLOAKED | PLAYER_FLAGS_INVULNERABLE);
				DXX_MAKE_VAR_UNDEFINED(player_info.cloak_time);
				DXX_MAKE_VAR_UNDEFINED(player_info.invulnerable_time);
				nd_read_string(i.callsign.buffer());
				nd_read_byte(&i.connected);
				if (purpose == PURPOSE_REWRITE)
				{
					nd_write_string(static_cast<const char *>(i.callsign));
					nd_write_byte(i.connected);
				}

				if (Newdemo_game_mode & GM_MULTI_COOP) {
					nd_read_int(&player_info.mission.score);
					if (purpose == PURPOSE_REWRITE)
						nd_write_int(player_info.mission.score);
				} else {
					nd_read_short(&player_info.net_killed_total);
					nd_read_short(&player_info.net_kills_total);
					if (purpose == PURPOSE_REWRITE)
					{
						nd_write_short(player_info.net_killed_total);
						nd_write_short(player_info.net_kills_total);
					}
				}
			}
			Game_mode = Newdemo_game_mode;
			if (purpose != PURPOSE_REWRITE)
				multi_sort_kill_list();
			Game_mode = GM_NORMAL;
		} else
		{
#if defined(DXX_BUILD_DESCENT_II)
			auto &player_info = get_local_plrobj().ctype.player_info;
			nd_read_int(&player_info.mission.score);      // Note link to above if!
			if (purpose == PURPOSE_REWRITE)
				nd_write_int(player_info.mission.score);
#endif
		}
	}
	auto &player_info = get_local_plrobj().ctype.player_info;
#if defined(DXX_BUILD_DESCENT_I)
	if (!(Newdemo_game_mode & GM_MULTI))
	{
		auto &score = player_info.mission.score;
		nd_read_int(&score);      // Note link to above if!
		if (purpose == PURPOSE_REWRITE)
			nd_write_int(score);
	}
#endif

	for (int i = 0; i < MAX_PRIMARY_WEAPONS; i++)
	{
		short s;
		nd_read_short(&s);
		if (i == primary_weapon_index_t::VULCAN_INDEX)
			player_info.vulcan_ammo = s;
		if (purpose == PURPOSE_REWRITE)
			nd_write_short(s);
	}

	range_for (auto &i, player_info.secondary_ammo)
	{
		uint16_t u;
		nd_read_short(&u);
		i = u;
		if (purpose == PURPOSE_REWRITE)
			nd_write_short(i);
	}

	sbyte i;
	nd_read_byte(&i);
	const stored_laser_level laser_level(i);
	if ((purpose != PURPOSE_REWRITE) && (laser_level != player_info.laser_level)) {
		player_info.laser_level = laser_level;
		update_laser_weapon_info();
	}
	else if (purpose == PURPOSE_REWRITE)
		nd_write_byte(laser_level);

	// Support for missions

	nd_read_string(current_mission);
	if (purpose == PURPOSE_REWRITE)
		nd_write_string(current_mission);
#if defined(DXX_BUILD_DESCENT_I)
	if (!shareware)
	{
		if ((purpose != PURPOSE_REWRITE) && load_mission_by_name(current_mission))
		{
			if (purpose == PURPOSE_CHOSE_PLAY) {
				nm_messagebox( NULL, 1, TXT_OK, TXT_NOMISSION4DEMO, current_mission );
			}
			return 1;
		}
	}
#elif defined(DXX_BUILD_DESCENT_II)
	if (load_mission_by_name(current_mission))
	{
		if (purpose != PURPOSE_RANDOM_PLAY) {
			nm_messagebox( NULL, 1, TXT_OK, TXT_NOMISSION4DEMO, current_mission );
		}
		return 1;
	}
#endif

	nd_recorded_total = 0;
	nd_playback_total = 0;
	nd_read_byte(&energy);
	nd_read_byte(&shield);
	if (purpose == PURPOSE_REWRITE)
	{
		nd_write_byte(energy);
		nd_write_byte(shield);
	}

	int recorded_player_flags;
	nd_read_int(&recorded_player_flags);
	player_info.powerup_flags = player_flags(recorded_player_flags);
	if (purpose == PURPOSE_REWRITE)
		nd_write_int(recorded_player_flags);
	if (player_info.powerup_flags & PLAYER_FLAGS_CLOAKED) {
		player_info.cloak_time = GameTime64 - (CLOAK_TIME_MAX / 2);
	}
	if (player_info.powerup_flags & PLAYER_FLAGS_INVULNERABLE)
		player_info.invulnerable_time = GameTime64 - (INVULNERABLE_TIME_MAX / 2);

	auto &Primary_weapon = player_info.Primary_weapon;
	{
		int8_t v;
		nd_read_byte(&v);
		Primary_weapon = static_cast<primary_weapon_index_t>(v);
	}
	auto &Secondary_weapon = player_info.Secondary_weapon;
	{
		int8_t v;
		nd_read_byte(&v);
		Secondary_weapon = static_cast<secondary_weapon_index_t>(v);
	}
	if (purpose == PURPOSE_REWRITE)
	{
		nd_write_byte(Primary_weapon);
		nd_write_byte(Secondary_weapon);
	}

// Next bit of code to fix problem that I introduced between 1.0 and 1.1
// check the next byte -- it _will_ be a load_new_level event.  If it is
// not, then we must shift all bytes up by one.

#if defined(DXX_BUILD_DESCENT_I)
	if (shareware)
	{
		nd_read_byte(&c);
		if (c != ND_EVENT_NEW_LEVEL) {
			auto flags = player_info.powerup_flags.get_player_flags();
			energy = shield;
			shield = static_cast<uint8_t>(flags);
			Primary_weapon = static_cast<primary_weapon_index_t>(static_cast<uint8_t>(Secondary_weapon));
			Secondary_weapon = static_cast<secondary_weapon_index_t>(c);
		} else
			PHYSFS_seek(infile, PHYSFS_tell(infile) - 1);
	}
#endif

#if defined(DXX_BUILD_DESCENT_II)
	nd_playback_v_juststarted=1;
#endif
	player_info.energy = i2f(energy);
	get_local_plrobj().shields = i2f(shield);
	return 0;
}
}

static void newdemo_pop_ctrlcen_triggers()
{
	for (int i = 0; i < ControlCenterTriggers.num_links; i++) {
		const auto &&seg = vmsegptridx(ControlCenterTriggers.seg[i]);
		const auto side = ControlCenterTriggers.side[i];
		const auto csegi = seg->children[side];
		if (csegi == segment_none)
		{
			/* Some levels specify control center triggers for
			 * segments/sides that have no wall.  `Descent 2:
			 * Counterstrike` level 11, control center trigger 0 would
			 * fault without this test.
			 */
			continue;
		}
		auto &csegp = *vmsegptr(csegi);
		const auto cside = find_connect_side(seg, csegp);
		const auto wall_num = seg->sides[side].wall_num;
		if (wall_num == wall_none)
		{
			/* Some levels specify control center triggers for
			 * segments/sides that have no wall.  `Descent 2:
			 * Counterstrike` level 9, control center trigger 2 would
			 * fault without this test.
			 */
			continue;
		}
		const auto anim_num = vcwallptr(wall_num)->clip_num;
		const auto n = WallAnims[anim_num].num_frames;
		const auto t = WallAnims[anim_num].flags & WCF_TMAP1
			? &side::tmap_num
			: &side::tmap_num2;
		seg->sides[side].*t = csegp.sides[cside].*t = WallAnims[anim_num].frames[n-1];
	}
}

namespace dsx {
static int newdemo_read_frame_information(int rewrite)
{
	int done, angle, volume;
	sbyte c;

	done = 0;

	if (Newdemo_vcr_state != ND_STATE_PAUSED)
		range_for (const auto &&segp, vmsegptr)
		{
			segp->objects = object_none;
		}

	reset_objects(ObjectState, 1);
	auto &plrobj = get_local_plrobj();
	plrobj.ctype.player_info.homing_object_dist = -1;

	prev_obj = NULL;

	while( !done ) {
		nd_read_byte(&c);
		if (nd_playback_v_bad_read) { done = -1; break; }
		if (rewrite && (c != ND_EVENT_EOF))
			nd_write_byte(c);

		switch( c ) {

		case ND_EVENT_START_FRAME: {        // Followed by an integer frame number, then a fix FrameTime
			short last_frame_length;

			done=1;
			nd_read_short(&last_frame_length);
			nd_read_int(&nd_playback_v_framecount);
			nd_read_int(&nd_recorded_time);
			if (nd_playback_v_bad_read) { done = -1; break; }
			if (rewrite)
			{
				nd_write_short(last_frame_length);
				nd_record_v_framebytes_written = 3;
				nd_write_int(nd_playback_v_framecount);
				nd_write_int(nd_recorded_time);
				break;
			}
			if (Newdemo_vcr_state == ND_STATE_PLAYBACK)
				nd_recorded_total += nd_recorded_time;
			nd_playback_v_framecount--;
			break;
		}

		case ND_EVENT_VIEWER_OBJECT:        // Followed by an object structure
		{
#if defined(DXX_BUILD_DESCENT_II)
			sbyte WhichWindow;
			nd_read_byte (&WhichWindow);
			if (rewrite)
				nd_write_byte(WhichWindow);
			if (WhichWindow&15)
			{
				const auto &&obj = vmobjptridx(static_cast<objnum_t>(MAX_OBJECTS - 1));
				nd_read_object(obj);
				if (nd_playback_v_bad_read)
				{
					done = -1;
					break;
				}
				if (rewrite)
				{
					nd_write_object(obj);
					break;
				}
				// offset to compensate inaccuracy between object and viewer
				vm_vec_scale_add(obj->pos, obj->pos, obj->orient.fvec, F1_0*5 );
				nd_render_extras (WhichWindow,obj);
			}
			else
#endif
			{
			nd_read_object(vmobjptridx(Viewer));
			if (nd_playback_v_bad_read) { done = -1; break; }
			if (rewrite)
			{
				nd_write_object(vcobjptr(Viewer));
				break;
			}
			if (Newdemo_vcr_state != ND_STATE_PAUSED) {
				auto segnum = Viewer->segnum;

				// HACK HACK HACK -- since we have multiple level recording, it can be the case
				// HACK HACK HACK -- that when rewinding the demo, the viewer is in a segment
				// HACK HACK HACK -- that is greater than the highest index of segments.  Bash
				// HACK HACK HACK -- the viewer to segment 0 for bogus view.

				if (segnum > Highest_segment_index)
					segnum = 0;
				obj_link_unchecked(Objects.vmptr, Objects.vmptridx(Viewer), Segments.vmptridx(segnum));
			}
			}
		}
			break;

		case ND_EVENT_RENDER_OBJECT:       // Followed by an object structure
		{
			const auto &&obj = obj_allocate(ObjectState);
			if (obj==object_none)
				break;
			nd_read_object(obj);
			if (nd_playback_v_bad_read) { done = -1; break; }
			if (rewrite)
			{
				nd_write_object(obj);
				break;
			}
			if (Newdemo_vcr_state != ND_STATE_PAUSED) {
				auto segnum = obj->segnum;

				// HACK HACK HACK -- don't render objects is segments greater than Highest_segment_index
				// HACK HACK HACK -- (see above)

				if (segnum > Highest_segment_index)
					break;

				obj_link_unchecked(Objects.vmptr, obj, Segments.vmptridx(segnum));
				if ((obj->type == OBJ_PLAYER) && (Newdemo_game_mode & GM_MULTI)) {
					int player;

					if (Newdemo_game_mode & GM_TEAM)
						player = get_team(get_player_id(obj));
					else
						player = get_player_id(obj);
					if (player == 0)
						break;
					player--;

					for (int i=0;i<Polygon_models[obj->rtype.pobj_info.model_num].n_textures;i++)
						multi_player_textures[player][i] = ObjBitmaps[ObjBitmapPtrs[Polygon_models[obj->rtype.pobj_info.model_num].first_texture+i]];

					multi_player_textures[player][4] = ObjBitmaps[ObjBitmapPtrs[First_multi_bitmap_num+(player)*2]];
					multi_player_textures[player][5] = ObjBitmaps[ObjBitmapPtrs[First_multi_bitmap_num+(player)*2+1]];
					obj->rtype.pobj_info.alt_textures = player+1;
				}
			}
		}
			break;

		case ND_EVENT_SOUND:
			{
				int soundno;
			nd_read_int(&soundno);
			if (nd_playback_v_bad_read) {done = -1; break; }
			if (rewrite)
			{
				nd_write_int(soundno);
				break;
			}
			if (Newdemo_vcr_state == ND_STATE_PLAYBACK)
				digi_play_sample( soundno, F1_0 );
			}
			break;

		case ND_EVENT_SOUND_3D:
			{
				int soundno;
			nd_read_int(&soundno);
			nd_read_int(&angle);
			nd_read_int(&volume);
			if (nd_playback_v_bad_read) { done = -1; break; }
			if (rewrite)
			{
				nd_write_int(soundno);
				nd_write_int(angle);
				nd_write_int(volume);
				break;
			}
			if (Newdemo_vcr_state == ND_STATE_PLAYBACK)
				digi_play_sample_3d(soundno, angle, volume);
			}
			break;

		case ND_EVENT_SOUND_3D_ONCE:
			{
				int soundno;
			nd_read_int(&soundno);
			nd_read_int(&angle);
			nd_read_int(&volume);
			if (nd_playback_v_bad_read) { done = -1; break; }
			if (rewrite)
			{
				nd_write_int(soundno);
				nd_write_int(angle);
				nd_write_int(volume);
				break;
			}
			if (Newdemo_vcr_state == ND_STATE_PLAYBACK)
				digi_play_sample_3d(soundno, angle, volume);
			}
			break;

		case ND_EVENT_LINK_SOUND_TO_OBJ:
			{
				int soundno, max_volume, max_distance, loop_start, loop_end;
				int signature;
				nd_read_int( &soundno );
				nd_read_int( &signature );
				nd_read_int( &max_volume );
				nd_read_int( &max_distance );
				nd_read_int( &loop_start );
				nd_read_int( &loop_end );
				if (rewrite)
				{
					nd_write_int( soundno );
					nd_write_int( signature );
					nd_write_int( max_volume );
					nd_write_int( max_distance );
					nd_write_int( loop_start );
					nd_write_int( loop_end );
					break;
				}
				auto objnum = newdemo_find_object(object_signature_t{static_cast<uint16_t>(signature)});
				if ( objnum != object_none && Newdemo_vcr_state == ND_STATE_PLAYBACK)  {   //  @mk, 2/22/96, John told me to.
					digi_link_sound_to_object3(soundno, objnum, 1, max_volume, sound_stack::allow_stacking, vm_distance{max_distance}, loop_start, loop_end);
				}
			}
			break;

		case ND_EVENT_KILL_SOUND_TO_OBJ:
			{
				int signature;
				nd_read_int( &signature );
				if (rewrite)
				{
					nd_write_int( signature );
					break;
				}
				auto objnum = newdemo_find_object(object_signature_t{static_cast<uint16_t>(signature)});
				if ( objnum != object_none && Newdemo_vcr_state == ND_STATE_PLAYBACK)  {   //  @mk, 2/22/96, John told me to.
					digi_kill_sound_linked_to_object(objnum);
				}
			}
			break;

		case ND_EVENT_WALL_HIT_PROCESS: {
			int player;
			int side;
			segnum_t segnum;
			fix damage;

			nd_read_segnum32(segnum);
			nd_read_int(&side);
			nd_read_fix(&damage);
			nd_read_int(&player);
			if (nd_playback_v_bad_read) { done = -1; break; }
			if (rewrite)
			{
				nd_write_int(segnum);
				nd_write_int(side);
				nd_write_fix(damage);
				nd_write_int(player);
				break;
			}
			if (Newdemo_vcr_state != ND_STATE_PAUSED)
			{
				auto &player_info = ConsoleObject->ctype.player_info;
				wall_hit_process(player_info.powerup_flags, vmsegptridx(segnum), side, damage, player, vmobjptr(ConsoleObject));
			}
			break;
		}

		case ND_EVENT_TRIGGER:
		{
			int side;
			segnum_t segnum;
			objnum_t objnum;
			nd_read_segnum32(segnum);
			nd_read_int(&side);
			nd_read_objnum32(objnum);
			int shot;
#if defined(DXX_BUILD_DESCENT_I)
			shot = 0;
#elif defined(DXX_BUILD_DESCENT_II)
			nd_read_int(&shot);
#endif
			if (nd_playback_v_bad_read) { done = -1; break; }
			if (rewrite)
			{
				nd_write_int(segnum);
				nd_write_int(side);
				nd_write_int(objnum);
#if defined(DXX_BUILD_DESCENT_I)
				break;
#elif defined(DXX_BUILD_DESCENT_II)
				nd_write_int(shot);
#endif
			}

                        const auto &&segp = vmsegptridx(segnum);
                        /* Demo recording is buggy.  Descent records
                            * ND_EVENT_TRIGGER for every segment transition, even
                            * if there is no wall.
							*
							* Likewise, ND_EVENT_TRIGGER can be recorded
							* when the wall is valid, but there is no
							* trigger on the wall.
                            */
                        if (segp->sides[side].wall_num != wall_none)
                        {
#if defined(DXX_BUILD_DESCENT_II)
							auto &w = *vcwallptr(segp->sides[side].wall_num);
							if (w.trigger != trigger_none && vctrgptr(w.trigger)->type == TT_SECRET_EXIT)
							{
                                        int truth;

                                        nd_read_byte(&c);
                                        Assert(c == ND_EVENT_SECRET_THINGY);
                                        nd_read_int(&truth);
                                        if (Newdemo_vcr_state == ND_STATE_PAUSED)
                                                break;
                                        if (rewrite)
                                        {
                                                nd_write_byte(c);
                                                nd_write_int(truth);
                                                break;
                                        }
                                        if (!truth)
											check_trigger(segp, side, plrobj, vmobjptridx(objnum), shot);
                                } else if (!rewrite)
#endif
                                {
                                        if (Newdemo_vcr_state != ND_STATE_PAUSED)
											check_trigger(segp, side, plrobj, vmobjptridx(objnum), shot);
                                }
                        }
		}
			break;

		case ND_EVENT_HOSTAGE_RESCUED: {
			int hostage_number;

			nd_read_int(&hostage_number);
			if (nd_playback_v_bad_read) { done = -1; break; }
			if (rewrite)
			{
				nd_write_int(hostage_number);
				break;
			}
			if (Newdemo_vcr_state != ND_STATE_PAUSED)
				hostage_rescue();
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
			const auto &&obj = obj_allocate(ObjectState);
			if (obj==object_none)
				break;
			nd_read_object(obj);
			if (nd_playback_v_bad_read) { done = -1; break; }
			if (rewrite)
			{
				nd_write_object(obj);
				break;
			}
			obj->render_type = RT_POLYOBJ;
			if (Newdemo_vcr_state != ND_STATE_PAUSED) {
				if (Newdemo_vcr_state != ND_STATE_PAUSED) {
					auto segnum = obj->segnum;
					obj_link_unchecked(Objects.vmptr, obj, Segments.vmptridx(segnum));
				}
			}
			break;
		}

		case ND_EVENT_WALL_TOGGLE:
		{
			int side;
			segnum_t segnum;
			nd_read_segnum32(segnum);
			nd_read_int(&side);
			if (nd_playback_v_bad_read) {done = -1; break; }
			if (rewrite)
			{
				nd_write_int(segnum);
				nd_write_int(side);
				break;
			}
			if (Newdemo_vcr_state != ND_STATE_PAUSED)
				wall_toggle(vmsegptridx(segnum), side);
		}
			break;

		case ND_EVENT_CONTROL_CENTER_DESTROYED:
			nd_read_int(&Countdown_seconds_left);
			Control_center_destroyed = 1;
			if (nd_playback_v_bad_read) { done = -1; break; }
			if (rewrite)
			{
				nd_write_int(Countdown_seconds_left);
				break;
			}
			if (!nd_playback_v_cntrlcen_destroyed) {
				newdemo_pop_ctrlcen_triggers();
				nd_playback_v_cntrlcen_destroyed = 1;
			}
			break;

		case ND_EVENT_HUD_MESSAGE: {
			char hud_msg[60];

			nd_read_string(&(hud_msg[0]));
			if (nd_playback_v_bad_read) { done = -1; break; }
			if (rewrite)
			{
				nd_write_string(&(hud_msg[0]));
				break;
			}
			if (Newdemo_vcr_state != ND_STATE_PAUSED)
				HUD_init_message_literal( HM_DEFAULT, hud_msg );
			break;
			}
#if defined(DXX_BUILD_DESCENT_II)
		case ND_EVENT_START_GUIDED:
			if ((Newdemo_vcr_state == ND_STATE_PLAYBACK) || (Newdemo_vcr_state == ND_STATE_FASTFORWARD) || (Newdemo_vcr_state == ND_STATE_ONEFRAMEFORWARD)) {
				nd_playback_v_guided = 1;
			} else if ((Newdemo_vcr_state == ND_STATE_REWINDING) || (Newdemo_vcr_state == ND_STATE_ONEFRAMEBACKWARD)) {
				nd_playback_v_guided = 0;
			}
			break;
		case ND_EVENT_END_GUIDED:
			if ((Newdemo_vcr_state == ND_STATE_REWINDING) || (Newdemo_vcr_state == ND_STATE_ONEFRAMEBACKWARD)) {
				nd_playback_v_guided = 1;
			} else if ((Newdemo_vcr_state == ND_STATE_PLAYBACK) || (Newdemo_vcr_state == ND_STATE_FASTFORWARD) || (Newdemo_vcr_state == ND_STATE_ONEFRAMEFORWARD)) {
				nd_playback_v_guided = 0;
			}
			break;
#endif

		case ND_EVENT_PALETTE_EFFECT: {
			short r, g, b;

			nd_read_short(&r);
			nd_read_short(&g);
			nd_read_short(&b);
			if (nd_playback_v_bad_read) { done = -1; break; }
			if (rewrite)
			{
				nd_write_short(r);
				nd_write_short(g);
				nd_write_short(b);
				break;
			}
			PALETTE_FLASH_SET(r,g,b);
			break;
		}

		case ND_EVENT_PLAYER_ENERGY: {
			ubyte energy;
			ubyte old_energy;

			if (!shareware)
			nd_read_byte(&old_energy);
			nd_read_byte(&energy);

			if (nd_playback_v_bad_read) {done = -1; break; }
			if (rewrite)
			{
				nd_write_byte(old_energy);
				nd_write_byte(energy);
				break;
			}
			auto &player_info = get_local_plrobj().ctype.player_info;
			if (shareware)
				player_info.energy = i2f(energy);
			else
			{
			if ((Newdemo_vcr_state == ND_STATE_PLAYBACK) || (Newdemo_vcr_state == ND_STATE_FASTFORWARD) || (Newdemo_vcr_state == ND_STATE_ONEFRAMEFORWARD)) {
				player_info.energy = i2f(energy);
			} else if ((Newdemo_vcr_state == ND_STATE_REWINDING) || (Newdemo_vcr_state == ND_STATE_ONEFRAMEBACKWARD)) {
				if (old_energy != 255)
					player_info.energy = i2f(old_energy);
			}
			}
			break;
		}

#if defined(DXX_BUILD_DESCENT_II)
		case ND_EVENT_PLAYER_AFTERBURNER: {
			ubyte afterburner;
			ubyte old_afterburner;

			nd_read_byte(&old_afterburner);
			nd_read_byte(&afterburner);
			if (nd_playback_v_bad_read) {done = -1; break; }
			if (rewrite)
			{
				nd_write_byte(old_afterburner);
				nd_write_byte(afterburner);
				break;
			}
			if ((Newdemo_vcr_state == ND_STATE_PLAYBACK) || (Newdemo_vcr_state == ND_STATE_FASTFORWARD) || (Newdemo_vcr_state == ND_STATE_ONEFRAMEFORWARD)) {
				Afterburner_charge = afterburner<<9;
// 				if (Afterburner_charge < 0) Afterburner_charge=f1_0;
			} else if ((Newdemo_vcr_state == ND_STATE_REWINDING) || (Newdemo_vcr_state == ND_STATE_ONEFRAMEBACKWARD)) {
				if (old_afterburner != 255)
					Afterburner_charge = old_afterburner<<9;
			}
			break;
		}
#endif

		case ND_EVENT_PLAYER_SHIELD: {
			ubyte shield;
			ubyte old_shield;

			if (!shareware)
			nd_read_byte(&old_shield);
			nd_read_byte(&shield);
			if (nd_playback_v_bad_read) {done = -1; break; }
			if (rewrite)
			{
				nd_write_byte(old_shield);
				nd_write_byte(shield);
				break;
			}
			if (shareware)
				get_local_plrobj().shields = i2f(shield);
			else
			{
			if ((Newdemo_vcr_state == ND_STATE_PLAYBACK) || (Newdemo_vcr_state == ND_STATE_FASTFORWARD) || (Newdemo_vcr_state == ND_STATE_ONEFRAMEFORWARD)) {
				get_local_plrobj().shields = i2f(shield);
			} else if ((Newdemo_vcr_state == ND_STATE_REWINDING) || (Newdemo_vcr_state == ND_STATE_ONEFRAMEBACKWARD)) {
				if (old_shield != 255)
					get_local_plrobj().shields = i2f(old_shield);
			}
			}
			break;
		}

		case ND_EVENT_PLAYER_FLAGS: {
			int recorded_player_flags;
			nd_read_int(&recorded_player_flags);
			if (nd_playback_v_bad_read) {done = -1; break; }
			if (rewrite)
			{
				nd_write_int(recorded_player_flags);
				break;
			}

			const auto old_player_flags = player_flags(static_cast<unsigned>(recorded_player_flags) >> 16);
			const auto new_player_flags = player_flags(static_cast<unsigned>(recorded_player_flags));

			const auto old_cloaked = old_player_flags & PLAYER_FLAGS_CLOAKED;
			const auto new_cloaked = new_player_flags & PLAYER_FLAGS_CLOAKED;
			const auto old_invul = old_player_flags & PLAYER_FLAGS_INVULNERABLE;
			const auto new_invul = new_player_flags & PLAYER_FLAGS_INVULNERABLE;
			if ((Newdemo_vcr_state == ND_STATE_REWINDING) || ((Newdemo_vcr_state == ND_STATE_ONEFRAMEBACKWARD) && (old_player_flags.get_player_flags() != 0xffff)) ) {
				auto &player_info = get_local_plrobj().ctype.player_info;
				if (old_cloaked != new_cloaked)
				{
					auto &t = player_info.cloak_time;
					if (!old_cloaked)
						DXX_MAKE_VAR_UNDEFINED(t);
					else
						t = GameTime64 - (CLOAK_TIME_MAX / 2);
				}
				if (old_invul != new_invul)
				{
					auto &t = player_info.invulnerable_time;
					if (!old_invul)
						DXX_MAKE_VAR_UNDEFINED(t);
					else
						t = GameTime64 - (INVULNERABLE_TIME_MAX / 2);
				}
				player_info.powerup_flags = old_player_flags;
			} else if ((Newdemo_vcr_state == ND_STATE_PLAYBACK) || (Newdemo_vcr_state == ND_STATE_FASTFORWARD) || (Newdemo_vcr_state == ND_STATE_ONEFRAMEFORWARD)) {
				auto &player_info = get_local_plrobj().ctype.player_info;
				if (old_cloaked != new_cloaked)
				{
					auto &t = player_info.cloak_time;
					if (!old_cloaked)
						t = GameTime64 - (CLOAK_TIME_MAX / 2);
					else
						DXX_MAKE_VAR_UNDEFINED(t);
				}
				if (old_invul != new_invul)
				{
					auto &t = player_info.invulnerable_time;
					if (!old_invul)
						t = GameTime64 - (INVULNERABLE_TIME_MAX / 2);
					else
						DXX_MAKE_VAR_UNDEFINED(t);
				}
				player_info.powerup_flags = new_player_flags;
			}
			update_laser_weapon_info();     // in case of quad laser change
			break;
		}

		case ND_EVENT_PLAYER_WEAPON:
		if (shareware)
		{
			ubyte weapon_type, weapon_num;

			nd_read_byte(&weapon_type);
			nd_read_byte(&weapon_num);
			if (rewrite)
			{
				nd_write_byte(weapon_type);
				nd_write_byte(weapon_num);
				break;
			}

			auto &player_info = get_local_plrobj().ctype.player_info;
			if (weapon_type == 0)
				player_info.Primary_weapon = static_cast<primary_weapon_index_t>(weapon_num);
			else
				player_info.Secondary_weapon = static_cast<secondary_weapon_index_t>(weapon_num);

			break;
		}
		else
			{
			ubyte weapon_type, weapon_num;
			ubyte old_weapon;

			nd_read_byte(&weapon_type);
			nd_read_byte(&weapon_num);
			nd_read_byte(&old_weapon);
			if (rewrite)
			{
				nd_write_byte(weapon_type);
				nd_write_byte(weapon_num);
				nd_write_byte(old_weapon);
				break;
			}
			auto &player_info = get_local_plrobj().ctype.player_info;
			if ((Newdemo_vcr_state == ND_STATE_PLAYBACK) || (Newdemo_vcr_state == ND_STATE_FASTFORWARD) || (Newdemo_vcr_state == ND_STATE_ONEFRAMEFORWARD)) {
				if (weapon_type == 0)
					player_info.Primary_weapon = static_cast<primary_weapon_index_t>(weapon_num);
				else
					player_info.Secondary_weapon = static_cast<secondary_weapon_index_t>(weapon_num);
			} else if ((Newdemo_vcr_state == ND_STATE_REWINDING) || (Newdemo_vcr_state == ND_STATE_ONEFRAMEBACKWARD)) {
				if (weapon_type == 0)
					player_info.Primary_weapon = static_cast<primary_weapon_index_t>(old_weapon);
				else
					player_info.Secondary_weapon = static_cast<secondary_weapon_index_t>(old_weapon);
			}
			break;
		}

		case ND_EVENT_EFFECT_BLOWUP: {
			segnum_t segnum;
			sbyte side;
			vms_vector pnt;

			nd_read_segnum16(segnum);
			nd_read_byte(&side);
			nd_read_vector(pnt);
			if (rewrite)
			{
				nd_write_short(segnum);
				nd_write_byte(side);
				nd_write_vector(pnt);
				break;
			}
			if (Newdemo_vcr_state != ND_STATE_PAUSED)
			{
#if defined(DXX_BUILD_DESCENT_I)
				check_effect_blowup(vmsegptridx(segnum), side, pnt, nullptr, 0, 0);
#elif defined(DXX_BUILD_DESCENT_II)
			//create a dummy object which will be the weapon that hits
			//the monitor. the blowup code wants to know who the parent of the
			//laser is, so create a laser whose parent is the player
				laser_parent dummy;
				dummy.parent_type = OBJ_PLAYER;
				dummy.parent_num = Player_num;
				check_effect_blowup(vmsegptridx(segnum), side, pnt, dummy, 0, 0);
#endif
			}
			break;
		}

		case ND_EVENT_HOMING_DISTANCE: {
			short distance;

			nd_read_short(&distance);
			if (rewrite)
			{
				nd_write_short(distance);
				break;
			}
			get_local_plrobj().ctype.player_info.homing_object_dist = i2f(distance << 16);
			break;
		}

		case ND_EVENT_LETTERBOX:
			if ((Newdemo_vcr_state == ND_STATE_PLAYBACK) || (Newdemo_vcr_state == ND_STATE_FASTFORWARD) || (Newdemo_vcr_state == ND_STATE_ONEFRAMEFORWARD)) {
				nd_playback_v_dead = 1;
			} else if ((Newdemo_vcr_state == ND_STATE_REWINDING) || (Newdemo_vcr_state == ND_STATE_ONEFRAMEBACKWARD))
				nd_playback_v_dead = 0;
			break;

#if defined(DXX_BUILD_DESCENT_II)
		case ND_EVENT_CHANGE_COCKPIT: {
			int dummy;
			nd_read_int (&dummy);
			if (rewrite)
				nd_write_int(dummy);
			break;
		}
#endif
		case ND_EVENT_REARVIEW:
			if ((Newdemo_vcr_state == ND_STATE_PLAYBACK) || (Newdemo_vcr_state == ND_STATE_FASTFORWARD) || (Newdemo_vcr_state == ND_STATE_ONEFRAMEFORWARD)) {
				nd_playback_v_rear = 1;
			} else if ((Newdemo_vcr_state == ND_STATE_REWINDING) || (Newdemo_vcr_state == ND_STATE_ONEFRAMEBACKWARD)) {
				nd_playback_v_rear = 0;
			}
			break;

		case ND_EVENT_RESTORE_COCKPIT:
			if ((Newdemo_vcr_state == ND_STATE_REWINDING) || (Newdemo_vcr_state == ND_STATE_ONEFRAMEBACKWARD)) {
				nd_playback_v_dead = 1;
			} else if ((Newdemo_vcr_state == ND_STATE_PLAYBACK) || (Newdemo_vcr_state == ND_STATE_FASTFORWARD) || (Newdemo_vcr_state == ND_STATE_ONEFRAMEFORWARD))
				nd_playback_v_dead = 0;
			break;


		case ND_EVENT_RESTORE_REARVIEW:
			if ((Newdemo_vcr_state == ND_STATE_REWINDING) || (Newdemo_vcr_state == ND_STATE_ONEFRAMEBACKWARD)) {
				nd_playback_v_rear= 1;
			} else if ((Newdemo_vcr_state == ND_STATE_PLAYBACK) || (Newdemo_vcr_state == ND_STATE_FASTFORWARD) || (Newdemo_vcr_state == ND_STATE_ONEFRAMEFORWARD)) {
				nd_playback_v_rear = 0;
			}
			break;


		case ND_EVENT_WALL_SET_TMAP_NUM1: {
			uint16_t seg, cseg, tmap;
			sbyte side,cside;

			nd_read_short(&seg);
			nd_read_byte(&side);
			nd_read_short(&cseg);
			nd_read_byte(&cside);
			nd_read_short( &tmap );
			if (rewrite)
			{
				nd_write_short(seg);
				nd_write_byte(side);
				nd_write_short(cseg);
				nd_write_byte(cside);
				nd_write_short(tmap);
				break;
			}
			if ((Newdemo_vcr_state != ND_STATE_PAUSED) && (Newdemo_vcr_state != ND_STATE_REWINDING) && (Newdemo_vcr_state != ND_STATE_ONEFRAMEBACKWARD))
				vmsegptr(seg)->sides[side].tmap_num = vmsegptr(cseg)->sides[cside].tmap_num = tmap;
			break;
		}

		case ND_EVENT_WALL_SET_TMAP_NUM2: {
			uint16_t seg, cseg, tmap;
			sbyte side,cside;

			nd_read_short(&seg);
			nd_read_byte(&side);
			nd_read_short(&cseg);
			nd_read_byte(&cside);
			nd_read_short( &tmap );
			if (rewrite)
			{
				nd_write_short(seg);
				nd_write_byte(side);
				nd_write_short(cseg);
				nd_write_byte(cside);
				nd_write_short(tmap);
				break;
			}
			if ((Newdemo_vcr_state != ND_STATE_PAUSED) && (Newdemo_vcr_state != ND_STATE_REWINDING) && (Newdemo_vcr_state != ND_STATE_ONEFRAMEBACKWARD)) {
				assert(tmap != 0);
				auto &s0 = *vmsegptr(seg);
				assert(s0.sides[side].tmap_num2 != 0);
				s0.sides[side].tmap_num2 = vmsegptr(cseg)->sides[cside].tmap_num2 = tmap;
			}
			break;
		}

		case ND_EVENT_MULTI_CLOAK: {
			sbyte pnum;

			nd_read_byte(&pnum);
			if (rewrite)
			{
				nd_write_byte(pnum);
				break;
			}
			auto &player_info = get_local_plrobj().ctype.player_info;
			if ((Newdemo_vcr_state == ND_STATE_REWINDING) || (Newdemo_vcr_state == ND_STATE_ONEFRAMEBACKWARD)) {
				player_info.powerup_flags &= ~PLAYER_FLAGS_CLOAKED;
				DXX_MAKE_VAR_UNDEFINED(player_info.cloak_time);
			} else if ((Newdemo_vcr_state == ND_STATE_PLAYBACK) || (Newdemo_vcr_state == ND_STATE_FASTFORWARD) || (Newdemo_vcr_state == ND_STATE_ONEFRAMEFORWARD)) {
				player_info.powerup_flags |= PLAYER_FLAGS_CLOAKED;
				player_info.cloak_time = GameTime64  - (CLOAK_TIME_MAX / 2);
			}
			break;
		}

		case ND_EVENT_MULTI_DECLOAK: {
			sbyte pnum;

			nd_read_byte(&pnum);
			if (rewrite)
			{
				nd_write_byte(pnum);
				break;
			}

			auto &player_info = get_local_plrobj().ctype.player_info;
			if ((Newdemo_vcr_state == ND_STATE_REWINDING) || (Newdemo_vcr_state == ND_STATE_ONEFRAMEBACKWARD)) {
				player_info.powerup_flags |= PLAYER_FLAGS_CLOAKED;
				player_info.cloak_time = GameTime64 - (CLOAK_TIME_MAX / 2);
			} else if ((Newdemo_vcr_state == ND_STATE_PLAYBACK) || (Newdemo_vcr_state == ND_STATE_FASTFORWARD) || (Newdemo_vcr_state == ND_STATE_ONEFRAMEFORWARD)) {
				player_info.powerup_flags &= ~PLAYER_FLAGS_CLOAKED;
				DXX_MAKE_VAR_UNDEFINED(player_info.cloak_time);
			}
			break;
		}

		case ND_EVENT_MULTI_DEATH: {
			uint8_t pnum;

			nd_read_byte(&pnum);
			if (rewrite)
			{
				nd_write_byte(pnum);
				break;
			}
			auto &player_info = vmobjptr(vcplayerptr(static_cast<unsigned>(pnum))->objnum)->ctype.player_info;
			if ((Newdemo_vcr_state == ND_STATE_REWINDING) || (Newdemo_vcr_state == ND_STATE_ONEFRAMEBACKWARD))
				player_info.net_killed_total--;
			else if ((Newdemo_vcr_state == ND_STATE_PLAYBACK) || (Newdemo_vcr_state == ND_STATE_FASTFORWARD) || (Newdemo_vcr_state == ND_STATE_ONEFRAMEFORWARD))
				player_info.net_killed_total++;
			break;
		}

		case ND_EVENT_MULTI_KILL: {
			uint8_t pnum, kill;

			nd_read_byte(&pnum);
			nd_read_byte(&kill);
			if (rewrite)
			{
				nd_write_byte(pnum);
				nd_write_byte(kill);
				break;
			}
			auto &player_info = vmobjptr(vcplayerptr(static_cast<unsigned>(pnum))->objnum)->ctype.player_info;
			if ((Newdemo_vcr_state == ND_STATE_REWINDING) || (Newdemo_vcr_state == ND_STATE_ONEFRAMEBACKWARD)) {
				player_info.net_kills_total -= kill;
				if (Newdemo_game_mode & GM_TEAM)
					team_kills[get_team(pnum)] -= kill;
			} else if ((Newdemo_vcr_state == ND_STATE_PLAYBACK) || (Newdemo_vcr_state == ND_STATE_FASTFORWARD) || (Newdemo_vcr_state == ND_STATE_ONEFRAMEFORWARD)) {
				player_info.net_kills_total += kill;
				if (Newdemo_game_mode & GM_TEAM)
					team_kills[get_team(pnum)] += kill;
			}
			Game_mode = Newdemo_game_mode;
			multi_sort_kill_list();
			Game_mode = GM_NORMAL;
			break;
		}

		case ND_EVENT_MULTI_CONNECT: {
			uint8_t pnum, new_player;
			int killed_total, kills_total;
			callsign_t new_callsign, old_callsign;

			nd_read_byte(&pnum);
			nd_read_byte(&new_player);
			if (!new_player) {
				nd_read_string(old_callsign.buffer());
				nd_read_int(&killed_total);
				nd_read_int(&kills_total);
			}
			nd_read_string(new_callsign.buffer());
			if (rewrite)
			{
				nd_write_byte(pnum);
				nd_write_byte(new_player);
				if (!new_player) {
					nd_write_string(old_callsign);
					nd_write_int(killed_total);
					nd_write_int(kills_total);
				}
				nd_write_string(static_cast<const char *>(new_callsign));
				break;
			}
			auto &plr = *vmplayerptr(static_cast<unsigned>(pnum));
			auto &player_info = vmobjptr(plr.objnum)->ctype.player_info;
			if ((Newdemo_vcr_state == ND_STATE_REWINDING) || (Newdemo_vcr_state == ND_STATE_ONEFRAMEBACKWARD)) {
				plr.connected = CONNECT_DISCONNECTED;
				if (!new_player) {
					plr.callsign = old_callsign;
					player_info.net_killed_total = killed_total;
					player_info.net_kills_total = kills_total;
				} else {
					N_players--;
				}
			} else if ((Newdemo_vcr_state == ND_STATE_PLAYBACK) || (Newdemo_vcr_state == ND_STATE_FASTFORWARD) || (Newdemo_vcr_state == ND_STATE_ONEFRAMEFORWARD)) {
				plr.connected = CONNECT_PLAYING;
				player_info.net_kills_total = 0;
				player_info.net_killed_total = 0;
				plr.callsign = new_callsign;
				if (new_player)
					N_players++;
			}
			break;
		}

		case ND_EVENT_MULTI_RECONNECT: {
			uint8_t pnum;

			nd_read_byte(&pnum);
			if (rewrite)
			{
				nd_write_byte(pnum);
				break;
			}
			if ((Newdemo_vcr_state == ND_STATE_REWINDING) || (Newdemo_vcr_state == ND_STATE_ONEFRAMEBACKWARD))
				vmplayerptr(static_cast<unsigned>(pnum))->connected = CONNECT_DISCONNECTED;
			else if ((Newdemo_vcr_state == ND_STATE_PLAYBACK) || (Newdemo_vcr_state == ND_STATE_FASTFORWARD) || (Newdemo_vcr_state == ND_STATE_ONEFRAMEFORWARD))
				vmplayerptr(static_cast<unsigned>(pnum))->connected = CONNECT_PLAYING;
			break;
		}

		case ND_EVENT_MULTI_DISCONNECT: {
			uint8_t pnum;

			nd_read_byte(&pnum);
			if (rewrite)
			{
				nd_write_byte(pnum);
				break;
			}
#if defined(DXX_BUILD_DESCENT_I)
			if ((Newdemo_vcr_state == ND_STATE_REWINDING) || (Newdemo_vcr_state == ND_STATE_ONEFRAMEBACKWARD))
				vmplayerptr(static_cast<unsigned>(pnum))->connected = CONNECT_DISCONNECTED;
			else if ((Newdemo_vcr_state == ND_STATE_PLAYBACK) || (Newdemo_vcr_state == ND_STATE_FASTFORWARD) || (Newdemo_vcr_state == ND_STATE_ONEFRAMEFORWARD))
				vmplayerptr(static_cast<unsigned>(pnum))->connected = CONNECT_PLAYING;
#elif defined(DXX_BUILD_DESCENT_II)
			if ((Newdemo_vcr_state == ND_STATE_REWINDING) || (Newdemo_vcr_state == ND_STATE_ONEFRAMEBACKWARD))
				vmplayerptr(static_cast<unsigned>(pnum))->connected = CONNECT_PLAYING;
			else if ((Newdemo_vcr_state == ND_STATE_PLAYBACK) || (Newdemo_vcr_state == ND_STATE_FASTFORWARD) || (Newdemo_vcr_state == ND_STATE_ONEFRAMEFORWARD))
				vmplayerptr(static_cast<unsigned>(pnum))->connected = CONNECT_DISCONNECTED;
#endif
			break;
		}

		case ND_EVENT_MULTI_SCORE: {
			int score;
			sbyte pnum;

			nd_read_byte(&pnum);
			nd_read_int(&score);
			if (rewrite)
			{
				nd_write_byte(pnum);
				nd_write_int(score);
				break;
			}
			auto &player_info = get_local_plrobj().ctype.player_info;
			if ((Newdemo_vcr_state == ND_STATE_REWINDING) || (Newdemo_vcr_state == ND_STATE_ONEFRAMEBACKWARD))
				player_info.mission.score -= score;
			else if ((Newdemo_vcr_state == ND_STATE_PLAYBACK) || (Newdemo_vcr_state == ND_STATE_FASTFORWARD) || (Newdemo_vcr_state == ND_STATE_ONEFRAMEFORWARD))
				player_info.mission.score += score;
			Game_mode = Newdemo_game_mode;
			multi_sort_kill_list();
			Game_mode = GM_NORMAL;
			break;
		}

		case ND_EVENT_PLAYER_SCORE: {
			int score;

			nd_read_int(&score);
			if (rewrite)
			{
				nd_write_int(score);
				break;
			}
			auto &player_info = get_local_plrobj().ctype.player_info;
			if ((Newdemo_vcr_state == ND_STATE_REWINDING) || (Newdemo_vcr_state == ND_STATE_ONEFRAMEBACKWARD))
				player_info.mission.score -= score;
			else if ((Newdemo_vcr_state == ND_STATE_PLAYBACK) || (Newdemo_vcr_state == ND_STATE_FASTFORWARD) || (Newdemo_vcr_state == ND_STATE_ONEFRAMEFORWARD))
				player_info.mission.score += score;
			break;
		}


		case ND_EVENT_PRIMARY_AMMO: {
			unsigned short old_ammo, new_ammo;

			nd_read_short(&old_ammo);
			nd_read_short(&new_ammo);
			if (rewrite)
			{
				nd_write_short(old_ammo);
				nd_write_short(new_ammo);
				break;
			}

			unsigned short value;
			// NOTE: Used (Primary_weapon==GAUSS_INDEX?VULCAN_INDEX:Primary_weapon) because game needs VULCAN_INDEX updated to show Gauss ammo
			if ((Newdemo_vcr_state == ND_STATE_REWINDING) || (Newdemo_vcr_state == ND_STATE_ONEFRAMEBACKWARD))
				value = old_ammo;
			else if ((Newdemo_vcr_state == ND_STATE_PLAYBACK) || (Newdemo_vcr_state == ND_STATE_FASTFORWARD) || (Newdemo_vcr_state == ND_STATE_ONEFRAMEFORWARD))
				value = new_ammo;
			else
				break;
			auto &player_info = get_local_plrobj().ctype.player_info;
#if defined(DXX_BUILD_DESCENT_II)
			if (player_info.Primary_weapon == primary_weapon_index_t::OMEGA_INDEX) // If Omega cannon, we need to update Omega_charge - not stored in primary_ammo
				player_info.Omega_charge = (value<=0?f1_0:value);
			else
#endif
			if (weapon_index_uses_vulcan_ammo(player_info.Primary_weapon))
				player_info.vulcan_ammo = value;
			break;
		}

		case ND_EVENT_SECONDARY_AMMO: {
			short old_ammo, new_ammo;

			nd_read_short(&old_ammo);
			nd_read_short(&new_ammo);
			if (rewrite)
			{
				nd_write_short(old_ammo);
				nd_write_short(new_ammo);
				break;
			}

			auto &player_info = get_local_plrobj().ctype.player_info;
			if ((Newdemo_vcr_state == ND_STATE_REWINDING) || (Newdemo_vcr_state == ND_STATE_ONEFRAMEBACKWARD))
				player_info.secondary_ammo[player_info.Secondary_weapon] = old_ammo;
			else if ((Newdemo_vcr_state == ND_STATE_PLAYBACK) || (Newdemo_vcr_state == ND_STATE_FASTFORWARD) || (Newdemo_vcr_state == ND_STATE_ONEFRAMEFORWARD))
				player_info.secondary_ammo[player_info.Secondary_weapon] = new_ammo;
			break;
		}

		case ND_EVENT_DOOR_OPENING: {
			segnum_t segnum;
			sbyte side;

			nd_read_segnum16(segnum);
			nd_read_byte(&side);
			if (rewrite)
			{
				nd_write_short(segnum);
				nd_write_byte(side);
				break;
			}
			if ((Newdemo_vcr_state == ND_STATE_REWINDING) || (Newdemo_vcr_state == ND_STATE_ONEFRAMEBACKWARD)) {
				const auto &&segp = vmsegptridx(segnum);
				const auto &&csegp = vmsegptr(segp->children[side]);
				const auto &&cside = find_connect_side(segp, csegp);
				const auto anim_num = vmwallptr(segp->sides[side].wall_num)->clip_num;
				const auto t = WallAnims[anim_num].flags & WCF_TMAP1
					? &side::tmap_num
					: &side::tmap_num2;
				segp->sides[side].*t = csegp->sides[cside].*t = WallAnims[anim_num].frames[0];
			}
			break;
		}

		case ND_EVENT_LASER_LEVEL: {
			sbyte old_level, new_level;

			nd_read_byte(&old_level);
			nd_read_byte(&new_level);
			if (rewrite)
			{
				nd_write_byte(old_level);
				nd_write_byte(new_level);
				break;
			}
			auto &player_info = get_local_plrobj().ctype.player_info;
			if ((Newdemo_vcr_state == ND_STATE_REWINDING) || (Newdemo_vcr_state == ND_STATE_ONEFRAMEBACKWARD)) {
				player_info.laser_level = stored_laser_level(old_level);
				update_laser_weapon_info();
			} else if ((Newdemo_vcr_state == ND_STATE_PLAYBACK) || (Newdemo_vcr_state == ND_STATE_FASTFORWARD) || (Newdemo_vcr_state == ND_STATE_ONEFRAMEFORWARD)) {
				player_info.laser_level = stored_laser_level(new_level);
				update_laser_weapon_info();
			}
			break;
		}

#if defined(DXX_BUILD_DESCENT_II)
		case ND_EVENT_CLOAKING_WALL: {
			uint8_t type, state, cloak_value;
			wallnum_t back_wall_num, front_wall_num;
			short l0,l1,l2,l3;

			nd_read_byte(&type);
			front_wall_num = type;
			nd_read_byte(&type);
			back_wall_num = type;
			nd_read_byte(&type);
			nd_read_byte(&state);
			nd_read_byte(&cloak_value);
			nd_read_short(&l0);
			nd_read_short(&l1);
			nd_read_short(&l2);
			nd_read_short(&l3);
			if (rewrite)
			{
				nd_write_byte(front_wall_num);
				nd_write_byte(back_wall_num);
				nd_write_byte(type);
				nd_write_byte(state);
				nd_write_byte(cloak_value);
				nd_write_short(l0);
				nd_write_short(l1);
				nd_write_short(l2);
				nd_write_short(l3);
				break;
			}

			{
				auto &w = *vmwallptr(front_wall_num);
				w.type = type;
				w.state = state;
				w.cloak_value = cloak_value;
				auto &uvl = vmsegptr(w.segnum)->sides[w.sidenum].uvls;
				uvl[0].l = (static_cast<int>(l0)) << 8;
				uvl[1].l = (static_cast<int>(l1)) << 8;
				uvl[2].l = (static_cast<int>(l2)) << 8;
				uvl[3].l = (static_cast<int>(l3)) << 8;
			}
			{
				auto &w = *vmwallptr(back_wall_num);
				w.type = type;
				w.state = state;
				w.cloak_value = cloak_value;
				auto &uvl = vmsegptr(w.segnum)->sides[w.sidenum].uvls;
				uvl[0].l = (static_cast<int>(l0)) << 8;
				uvl[1].l = (static_cast<int>(l1)) << 8;
				uvl[2].l = (static_cast<int>(l2)) << 8;
				uvl[3].l = (static_cast<int>(l3)) << 8;
			}
			break;
		}
#endif

		case ND_EVENT_NEW_LEVEL: {
			sbyte new_level, old_level, loaded_level;

			nd_read_byte (&new_level);
			nd_read_byte (&old_level);
			if (rewrite)
			{
				nd_write_byte (new_level);
				nd_write_byte (old_level);
#if defined(DXX_BUILD_DESCENT_I)
				break;
#elif defined(DXX_BUILD_DESCENT_II)
				load_level_robots(new_level);	// for correct robot info reading (specifically boss flag)
#endif
			}
			else
			{
				if (Newdemo_vcr_state == ND_STATE_PAUSED)
					break;

				pause_game_world_time p;
				if ((Newdemo_vcr_state == ND_STATE_REWINDING) || (Newdemo_vcr_state == ND_STATE_ONEFRAMEBACKWARD))
					loaded_level = old_level;
				else {
					loaded_level = new_level;
					range_for (auto &i, Players)
					{
						const auto &&objp = vmobjptr(i.objnum);
						auto &player_info = objp->ctype.player_info;
						player_info.powerup_flags &= ~PLAYER_FLAGS_CLOAKED;
						DXX_MAKE_VAR_UNDEFINED(player_info.cloak_time);
					}
				}
				if ((loaded_level < Last_secret_level) || (loaded_level > Last_level)) {
					nm_messagebox( NULL, 1, TXT_OK, "%s\n%s\n%s", TXT_CANT_PLAYBACK, TXT_LEVEL_CANT_LOAD, TXT_DEMO_OLD_CORRUPT );
					Current_mission.reset();
					return -1;
				}

				LoadLevel(static_cast<int>(loaded_level),1);
				nd_playback_v_cntrlcen_destroyed = 0;
			}

                        if (!rewrite)
                                stop_time();
#if defined(DXX_BUILD_DESCENT_II)
			if (nd_playback_v_juststarted)
			{
				unsigned num_walls;
				nd_read_int (&num_walls);
				Walls.set_count(num_walls);
				if (rewrite)
					nd_write_int (Num_walls);
				range_for (const auto &&wp, vmwallptr)
				// restore the walls
				{
					auto &w = *wp;
					nd_read_byte(&w.type);
					nd_read_byte(&w.flags);
					nd_read_byte(&w.state);

					auto &side = vmsegptr(w.segnum)->sides[w.sidenum];
					nd_read_short (&side.tmap_num);
					nd_read_short (&side.tmap_num2);

					if (rewrite)
					{
						nd_write_byte (w.type);
						nd_write_byte (w.flags);
						nd_write_byte (w.state);
						nd_write_short (side.tmap_num);
						nd_write_short (side.tmap_num2);
					}
				}

				nd_playback_v_juststarted=0;
				if (rewrite)
					break;

				Game_mode = Newdemo_game_mode;
				if (game_mode_hoard())
					init_hoard_data();

				if (game_mode_capture_flag() || game_mode_hoard())
					multi_apply_goal_textures ();
				Game_mode = GM_NORMAL;
			}

			if (rewrite)
				break;
#endif

			reset_palette_add();                // get palette back to normal
			full_palette_save();				// initialise for palette_restore()

                        if (!rewrite)
                                start_time();
			break;
		}

		case ND_EVENT_EOF: {
			done=-1;
			PHYSFS_seek(infile, PHYSFS_tell(infile) - 1);        // get back to the EOF marker
			nd_playback_v_at_eof = 1;
			nd_playback_v_framecount++;
			break;
		}

		default:
			Int3();
		}
	}

	// Now set up cockpit and views according to what we read out. Note that the demo itself cannot determinate the right views since it does not use a good portion of the real game code.
	if (nd_playback_v_dead)
	{
		Rear_view = 0;
		if (PlayerCfg.CockpitMode[1] != CM_LETTERBOX)
			select_cockpit(CM_LETTERBOX);
	}
#if defined(DXX_BUILD_DESCENT_II)
	else if (nd_playback_v_guided)
	{
		Rear_view = 0;
		if (PlayerCfg.CockpitMode[1] != CM_FULL_SCREEN || PlayerCfg.CockpitMode[1] != CM_STATUS_BAR)
		{
			select_cockpit((PlayerCfg.CockpitMode[0] == CM_FULL_SCREEN)?CM_FULL_SCREEN:CM_STATUS_BAR);
		}
	}
#endif
	else if (nd_playback_v_rear)
	{
		Rear_view = nd_playback_v_rear;
		if (PlayerCfg.CockpitMode[0] == CM_FULL_COCKPIT)
			select_cockpit(CM_REAR_VIEW);
	}
	else
	{
		Rear_view = 0;
		if (PlayerCfg.CockpitMode[1] != PlayerCfg.CockpitMode[0])
			select_cockpit(PlayerCfg.CockpitMode[0]);
	}

	if (nd_playback_v_bad_read) {
		nm_messagebox( NULL, 1, TXT_OK, "%s %s", TXT_DEMO_ERR_READING, TXT_DEMO_OLD_CORRUPT );
		Current_mission.reset();
	}

	return done;
}
}

window_event_result newdemo_goto_beginning()
{
	//if (nd_playback_v_framecount == 0)
	//	return;
	PHYSFS_seek(infile, 0);
	Newdemo_vcr_state = ND_STATE_PLAYBACK;
	if (newdemo_read_demo_start(PURPOSE_CHOSE_PLAY))
		newdemo_stop_playback();
	if (newdemo_read_frame_information(0) == -1)
		newdemo_stop_playback();
	if (newdemo_read_frame_information(0) == -1)
		newdemo_stop_playback();
	Newdemo_vcr_state = ND_STATE_PAUSED;
	nd_playback_v_at_eof = 0;

	// check if we stopped playback
	return Newdemo_state == ND_STATE_NORMAL ? window_event_result::close : window_event_result::handled;
}

namespace dsx {
window_event_result newdemo_goto_end(int to_rewrite)
{
	short frame_length=0, byte_count=0, bshort=0;
	sbyte level=0, bbyte=0, c=0, cloaked=0;
	ubyte energy=0, shield=0;
	int loc=0, bint=0;

	PHYSFSX_fseek(infile, -2, SEEK_END);
	nd_read_byte(&level);

	if (!to_rewrite)
	{
		if ((level < Last_secret_level) || (level > Last_level)) {
			nm_messagebox( NULL, 1, TXT_OK, "%s\n%s\n%s", TXT_CANT_PLAYBACK, TXT_LEVEL_CANT_LOAD, TXT_DEMO_OLD_CORRUPT );
			Current_mission.reset();
			newdemo_stop_playback();
			return window_event_result::close;
		}
		if (level != Current_level_num)
			LoadLevel(level,1);
	}
	else
#if defined(DXX_BUILD_DESCENT_II)
		if (level != Current_level_num)
#endif
	{
#if defined(DXX_BUILD_DESCENT_II)
		load_level_robots(level);	// for correct robot info reading (specifically boss flag)
#endif
		Current_level_num = level;
	}

#if defined(DXX_BUILD_DESCENT_I)
	if (shareware)
	{
		if (Newdemo_game_mode & GM_MULTI) {
			PHYSFSX_fseek(infile, -10, SEEK_END);
			nd_read_byte(&cloaked);
			for (playernum_t i = 0; i < MAX_PLAYERS; i++)
			{
				const auto &&objp = vmobjptr(vcplayerptr(i)->objnum);
				auto &player_info = objp->ctype.player_info;
				if ((1 << i) & cloaked)
				{
					player_info.powerup_flags |= PLAYER_FLAGS_CLOAKED;
				}
				player_info.cloak_time = GameTime64 - (CLOAK_TIME_MAX / 2);
			}
		}

		if (to_rewrite)
			return window_event_result::handled;

		PHYSFSX_fseek(infile, -12, SEEK_END);
		nd_read_short(&frame_length);
	}
	else
#endif
	{
	PHYSFSX_fseek(infile, -4, SEEK_END);
	nd_read_short(&byte_count);
	PHYSFSX_fseek(infile, -2 - byte_count, SEEK_CUR);

	nd_read_short(&frame_length);
	loc = PHYSFS_tell(infile);
	if (Newdemo_game_mode & GM_MULTI)
	{
		nd_read_byte(&cloaked);
		for (unsigned i = 0; i < MAX_PLAYERS; ++i)
			if (cloaked & (1 << i))
				{
					const auto &&objp = vmobjptr(vcplayerptr(i)->objnum);
					objp->ctype.player_info.powerup_flags |= PLAYER_FLAGS_CLOAKED;
				}
	}
	else
		nd_read_byte(&bbyte);
	nd_read_byte(&bbyte);
	nd_read_short(&bshort);
	nd_read_int(&bint);

	nd_read_byte(&energy);
	nd_read_byte(&shield);
	auto &player_info = get_local_plrobj().ctype.player_info;
	player_info.energy = i2f(energy);
	get_local_plrobj().shields = i2f(shield);
	int recorded_player_flags;
	nd_read_int(&recorded_player_flags);
	player_info.powerup_flags = player_flags(recorded_player_flags);
	if (player_info.powerup_flags & PLAYER_FLAGS_CLOAKED) {
		player_info.cloak_time = GameTime64 - (CLOAK_TIME_MAX / 2);
	}
	if (player_info.powerup_flags & PLAYER_FLAGS_INVULNERABLE)
		player_info.invulnerable_time = GameTime64 - (INVULNERABLE_TIME_MAX / 2);
	{
		int8_t v;
		nd_read_byte(&v);
		player_info.Primary_weapon = static_cast<primary_weapon_index_t>(v);
	}
	{
		int8_t v;
		nd_read_byte(&v);
		player_info.Secondary_weapon = static_cast<secondary_weapon_index_t>(v);
	}
	for (int i = 0; i < MAX_PRIMARY_WEAPONS; i++)
	{
		short s;
		nd_read_short(&s);
		if (i == primary_weapon_index_t::VULCAN_INDEX)
			player_info.vulcan_ammo = s;
	}
	range_for (auto &i, player_info.secondary_ammo)
	{
		uint16_t u;
		nd_read_short(&u);
		i = u;
	}
	{
	int8_t i;
	nd_read_byte(&i);
	const stored_laser_level laser_level(i);
	if (player_info.laser_level != laser_level) {
		player_info.laser_level = laser_level;
		if (!to_rewrite)
			update_laser_weapon_info();
	}
	}

	if (Newdemo_game_mode & GM_MULTI) {
		nd_read_byte(&c);
		N_players = static_cast<int>(c);
		// see newdemo_read_start_demo for explanation of
		// why this is commented out
		//		nd_read_byte(&N_players);
		range_for (auto &i, partial_range(Players, N_players)) {
			nd_read_string(i.callsign.buffer());
			nd_read_byte(&i.connected);
			auto &pl_info = vmobjptr(i.objnum)->ctype.player_info;
			if (Newdemo_game_mode & GM_MULTI_COOP) {
				nd_read_int(&pl_info.mission.score);
			} else {
				nd_read_short(&pl_info.net_killed_total);
				nd_read_short(&pl_info.net_kills_total);
			}
		}
	} else {
		nd_read_int(&player_info.mission.score);
	}

	if (to_rewrite)
		return window_event_result::handled;

	PHYSFSX_fseek(infile, loc, SEEK_SET);
	}
	PHYSFSX_fseek(infile, -frame_length, SEEK_CUR);
	nd_read_int(&nd_playback_v_framecount);            // get the frame count
	nd_playback_v_framecount--;
	PHYSFSX_fseek(infile, 4, SEEK_CUR);
	Newdemo_vcr_state = ND_STATE_PLAYBACK;
	newdemo_read_frame_information(0); // then the frame information
	Newdemo_vcr_state = ND_STATE_PAUSED;
	return window_event_result::handled;
}
}

static window_event_result newdemo_back_frames(int frames)
{
	short last_frame_length;
	for (int i = 0; i < frames; i++)
	{
		PHYSFS_seek(infile, PHYSFS_tell(infile) - 10);
		nd_read_short(&last_frame_length);
		PHYSFS_seek(infile, PHYSFS_tell(infile) + 8 - last_frame_length);

		if (!nd_playback_v_at_eof && newdemo_read_frame_information(0) == -1) {
			newdemo_stop_playback();
			return window_event_result::close;
		}
		if (nd_playback_v_at_eof)
			nd_playback_v_at_eof = 0;

		PHYSFS_seek(infile, PHYSFS_tell(infile) - 10);
		nd_read_short(&last_frame_length);
		PHYSFS_seek(infile, PHYSFS_tell(infile) + 8 - last_frame_length);
	}

	return window_event_result::handled;
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

static window_event_result interpolate_frame(fix d_play, fix d_recorded)
{
	fix factor;
	static fix InterpolStep = fl2f(.01);

	if (nd_playback_v_framecount < 1)
		return window_event_result::ignored;

	factor = fixdiv(d_play, d_recorded);
	if (factor > F1_0)
		factor = F1_0;

	const auto num_cur_objs = Objects.get_count();
	std::vector<object> cur_objs(Objects.begin(), Objects.begin() + num_cur_objs);

	Newdemo_vcr_state = ND_STATE_PAUSED;
	if (newdemo_read_frame_information(0) == -1) {
		newdemo_stop_playback();
		return window_event_result::close;
	}

	InterpolStep -= FrameTime;

	// This interpolating looks just more crappy on high FPS, so let's not even waste performance on it.
	if (InterpolStep <= 0)
	{
		range_for (auto &i, partial_range(cur_objs, num_cur_objs)) {
			range_for (const auto &&objp, vmobjptr)
			{
				if (i.signature == objp->signature) {
					sbyte render_type = i.render_type;
					fix delta_x, delta_y, delta_z;

					//  Extract the angles from the object orientation matrix.
					//  Some of this code taken from ai_turn_towards_vector
					//  Don't do the interpolation on certain render types which don't use an orientation matrix

					if (!((render_type == RT_LASER) || (render_type == RT_FIREBALL) || (render_type == RT_POWERUP))) {
						vms_vector  fvec1, fvec2, rvec1, rvec2;
						fix         mag1;

						fvec1 = i.orient.fvec;
						vm_vec_scale(fvec1, F1_0-factor);
						fvec2 = objp->orient.fvec;
						vm_vec_scale(fvec2, factor);
						vm_vec_add2(fvec1, fvec2);
						mag1 = vm_vec_normalize_quick(fvec1);
						if (mag1 > F1_0/256) {
							rvec1 = i.orient.rvec;
							vm_vec_scale(rvec1, F1_0-factor);
							rvec2 = objp->orient.rvec;
							vm_vec_scale(rvec2, factor);
							vm_vec_add2(rvec1, rvec2);
							vm_vec_normalize_quick(rvec1); // Note: Doesn't matter if this is null, if null, vm_vector_2_matrix will just use fvec1
							vm_vector_2_matrix(i.orient, fvec1, nullptr, &rvec1);
						}
					}

					// Interpolate the object position.  This is just straight linear
					// interpolation.

					delta_x = objp->pos.x - i.pos.x;
					delta_y = objp->pos.y - i.pos.y;
					delta_z = objp->pos.z - i.pos.z;

					delta_x = fixmul(delta_x, factor);
					delta_y = fixmul(delta_y, factor);
					delta_z = fixmul(delta_z, factor);

					i.pos.x += delta_x;
					i.pos.y += delta_y;
					i.pos.z += delta_z;
				}
			}
		}
		InterpolStep = fl2f(.01);
	}

	// get back to original position in the demo file.  Reread the current
	// frame information again to reset all of the object stuff not covered
	// with Highest_object_index and the object array (previously rendered
	// objects, etc....)

	auto result = newdemo_back_frames(1);
	result = std::max(newdemo_back_frames(1), result);
	if (newdemo_read_frame_information(0) == -1)
	{
		newdemo_stop_playback();
		result =  window_event_result::close;
	}
	Newdemo_vcr_state = ND_STATE_PLAYBACK;

	std::copy(cur_objs.begin(), cur_objs.begin() + num_cur_objs, Objects.begin());
	Objects.set_count(num_cur_objs);

	return result;
}

window_event_result newdemo_playback_one_frame()
{
	int frames_back;
	static fix base_interpol_time = 0;
	static fix d_recorded = 0;
	auto result = window_event_result::handled;

	range_for (auto &i, Players)
	{
		const auto &&objp = vmobjptr(i.objnum);
		auto &player_info = objp->ctype.player_info;
		if (player_info.powerup_flags & PLAYER_FLAGS_CLOAKED)
			player_info.cloak_time = GameTime64 - (CLOAK_TIME_MAX / 2);
		if (player_info.powerup_flags & PLAYER_FLAGS_INVULNERABLE)
			player_info.invulnerable_time = GameTime64 - (INVULNERABLE_TIME_MAX / 2);
	}

	if (Newdemo_vcr_state == ND_STATE_PAUSED)       // render a frame or not
		return window_event_result::ignored;

	Control_center_destroyed = 0;
	Countdown_seconds_left = -1;
	PALETTE_FLASH_SET(0,0,0);       //clear flash

	if ((Newdemo_vcr_state == ND_STATE_REWINDING) || (Newdemo_vcr_state == ND_STATE_ONEFRAMEBACKWARD))
	{
		const int level = Current_level_num;
		if (nd_playback_v_framecount == 0)
			return window_event_result::ignored;
		else if ((Newdemo_vcr_state == ND_STATE_REWINDING) && (nd_playback_v_framecount < 10)) {
			return newdemo_goto_beginning();
		}
		if (Newdemo_vcr_state == ND_STATE_REWINDING)
			frames_back = 10;
		else
			frames_back = 1;
		if (nd_playback_v_at_eof) {
			PHYSFS_seek(infile, PHYSFS_tell(infile) + (shareware ? -2 : +11));
		}
		result = newdemo_back_frames(frames_back);

		if (level != Current_level_num)
			newdemo_pop_ctrlcen_triggers();

		if (Newdemo_vcr_state == ND_STATE_ONEFRAMEBACKWARD) {
			if (level != Current_level_num)
				result = std::max(newdemo_back_frames(1), result);
			Newdemo_vcr_state = ND_STATE_PAUSED;
		}
	}
	else if (Newdemo_vcr_state == ND_STATE_FASTFORWARD) {
		if (!nd_playback_v_at_eof)
		{
			for (int i = 0; i < 10; i++)
			{
				if (newdemo_read_frame_information(0) == -1)
				{
					if (nd_playback_v_at_eof)
						Newdemo_vcr_state = ND_STATE_PAUSED;
					else
					{
						newdemo_stop_playback();
						result = window_event_result::close;
					}
					break;
				}
			}
		}
		else
			Newdemo_vcr_state = ND_STATE_PAUSED;
	}
	else if (Newdemo_vcr_state == ND_STATE_ONEFRAMEFORWARD) {
		if (!nd_playback_v_at_eof) {
			const int level = Current_level_num;
			if (newdemo_read_frame_information(0) == -1) {
				if (!nd_playback_v_at_eof)
				{
					newdemo_stop_playback();
					result = window_event_result::close;
				}
			}
			if (level != Current_level_num) {
				if (newdemo_read_frame_information(0) == -1) {
					if (!nd_playback_v_at_eof)
					{
						newdemo_stop_playback();
						result = window_event_result::close;
					}
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

		if (nd_playback_v_framecount <= 0)
			nd_playback_total = nd_recorded_total;      // baseline total playback time
		else
			nd_playback_total += FrameTime;

		if (nd_playback_v_style == NORMAL_PLAYBACK)
		{
			if (nd_playback_total > nd_recorded_total)
				nd_playback_v_style = SKIP_PLAYBACK;

			if (nd_recorded_total > 0 && nd_recorded_time > 0)
			{
				nd_playback_v_style = INTERPOLATE_PLAYBACK;
				nd_playback_total = nd_recorded_total + FrameTime; // baseline playback time
				base_interpol_time = nd_recorded_total;
				d_recorded = nd_recorded_time; // baseline delta recorded
			}
		}


		if ((nd_playback_v_style == INTERPOLATE_PLAYBACK) && Newdemo_do_interpolate) {
			fix d_play = 0;

			if (nd_recorded_total - nd_playback_total < FrameTime) {
				d_recorded = nd_recorded_total - nd_playback_total;

				while (nd_recorded_total - nd_playback_total < FrameTime) {
					unsigned num_objs;

					num_objs = Highest_object_index;
					std::vector<object> cur_objs(Objects.begin(), Objects.begin() + num_objs + 1);

					const int level = Current_level_num;
					if (newdemo_read_frame_information(0) == -1) {
						newdemo_stop_playback();
						return window_event_result::close;
					}
					if (level != Current_level_num) {
						if (newdemo_read_frame_information(0) == -1)
						{
							newdemo_stop_playback();
							result = window_event_result::close;
						}
						break;
					}

					//  for each new object in the frame just read in, determine if there is
					//  a corresponding object that we have been interpolating.  If so, then
					//  copy that interpolated object to the new Objects array so that the
					//  interpolated position and orientation can be preserved.

					range_for (auto &i, partial_const_range(cur_objs, 1 + num_objs)) {
						range_for (const auto &&objp, vmobjptr)
						{
							if (i.signature == objp->signature) {
								objp->orient = i.orient;
								objp->pos = i.pos;
								break;
							}
						}
					}
					d_recorded += nd_recorded_time;
					base_interpol_time = nd_playback_total - FrameTime;
				}
			}

			d_play = nd_playback_total - base_interpol_time;
			return std::max(interpolate_frame(d_play, d_recorded), result);
		}
		else {
			if (newdemo_read_frame_information(0) == -1) {
				newdemo_stop_playback();
				return window_event_result::close;
			}
			if (nd_playback_v_style == SKIP_PLAYBACK) {
				while (nd_playback_total > nd_recorded_total) {
					if (newdemo_read_frame_information(0) == -1) {
						newdemo_stop_playback();
						return window_event_result::close;
					}
				}
			}
		}
	}

	return result;
}

void newdemo_start_recording()
{
	Newdemo_num_written = 0;
	nd_record_v_no_space=0;
	Newdemo_state = ND_STATE_RECORDING;

	PHYSFS_mkdir(DEMO_DIR); //always try making directory - could only exist in read-only path

	outfile = PHYSFSX_openWriteBuffered(DEMO_FILENAME);

	if (!outfile)
	{
		Newdemo_state = ND_STATE_NORMAL;
		nm_messagebox(NULL, 1, TXT_OK, "Cannot open demo temp file");
	}
	else
		newdemo_record_start_demo();
}

static void newdemo_write_end()
{
	sbyte cloaked = 0;
	unsigned short byte_count = 0;
	nd_write_byte(ND_EVENT_EOF);
	nd_write_short(nd_record_v_framebytes_written - 1);
	if (Game_mode & GM_MULTI) {
		for (unsigned i = 0; i < N_players; ++i)
		{
			const auto &&objp = vmobjptr(vcplayerptr(i)->objnum);
			if (objp->ctype.player_info.powerup_flags & PLAYER_FLAGS_CLOAKED)
				cloaked |= (1 << i);
		}
		nd_write_byte(cloaked);
		nd_write_byte(ND_EVENT_EOF);
	} else {
		nd_write_short(ND_EVENT_EOF);
	}
	nd_write_short(ND_EVENT_EOF);
	nd_write_int(ND_EVENT_EOF);

	if (!shareware)
	{
	byte_count += 10;       // from nd_record_v_framebytes_written

	auto &player_info = get_local_plrobj().ctype.player_info;
	nd_write_byte(static_cast<int8_t>(f2ir(player_info.energy)));
	nd_write_byte(static_cast<int8_t>(f2ir(get_local_plrobj().shields)));
	nd_write_int(player_info.powerup_flags.get_player_flags());        // be sure players flags are set
	nd_write_byte(static_cast<int8_t>(static_cast<primary_weapon_index_t>(player_info.Primary_weapon)));
	nd_write_byte(static_cast<int8_t>(static_cast<secondary_weapon_index_t>(player_info.Secondary_weapon)));
	byte_count += 8;

	for (int i = 0; i < MAX_PRIMARY_WEAPONS; i++)
		nd_write_short(i == primary_weapon_index_t::VULCAN_INDEX ? player_info.vulcan_ammo : 0);

	range_for (auto &i, player_info.secondary_ammo)
		nd_write_short(i);
	byte_count += (sizeof(short) * (MAX_PRIMARY_WEAPONS + MAX_SECONDARY_WEAPONS));

	nd_write_byte(player_info.laser_level);
	byte_count++;

	if (Game_mode & GM_MULTI) {
		nd_write_byte(static_cast<int8_t>(N_players));
		byte_count++;
		range_for (auto &i, partial_const_range(Players, N_players)) {
			nd_write_string(static_cast<const char *>(i.callsign));
			byte_count += (strlen(static_cast<const char *>(i.callsign)) + 2);
			nd_write_byte(i.connected);
			auto &pl_info = vcobjptr(i.objnum)->ctype.player_info;
			if (Game_mode & GM_MULTI_COOP) {
				nd_write_int(pl_info.mission.score);
				byte_count += 5;
			} else {
				nd_write_short(pl_info.net_killed_total);
				nd_write_short(pl_info.net_kills_total);
				byte_count += 5;
			}
		}
	} else {
		nd_write_int(player_info.mission.score);
		byte_count += 4;
	}
	nd_write_short(byte_count);
	}

	nd_write_byte(Current_level_num);
	nd_write_byte(ND_EVENT_EOF);
}

static bool guess_demo_name(ntstring<PATH_MAX - 1> &filename)
{
	filename[0] = 0;
	const auto &n = CGameArg.SysRecordDemoNameTemplate;
	if (n.empty())
		return false;
	auto p = n.c_str();
	if (!strcmp(p, "."))
		p = "%Y%m%d.%H%M%S-$p-$m";
	std::size_t i = 0;
	time_t t = 0;
	tm *ptm = nullptr;
	for (;; ++p)
	{
		if (*p == '%')
		{
			if (!p[1])
				/* Trailing bare % is ill-formed.  Ignore entire
				 * template.
				 */
				return false;
			/* Time conversions */
			if (unlikely(!t))
				t = time(nullptr);
			if (unlikely(t == -1 || !(ptm = gmtime(&t))))
				continue;
			char sbuf[4];
			sbuf[0] = '%';
			sbuf[1] = *++p;
#ifndef _WIN32
			/* Not supported on Windows */
			if (sbuf[1] == 'O' || sbuf[1] == 'E')
			{
				sbuf[2] = *++p;
				sbuf[3] = 0;
			}
			else
#endif
			{
				sbuf[2] = 0;
			}
			filename[i] = 0;
#ifdef __GNUC__
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wformat-nonliteral"
#endif
			const auto a = strftime(&filename[i], sizeof(filename) - i, sbuf, ptm);
#ifdef __GNUC__
#pragma GCC diagnostic pop
#endif
			if (a >= sizeof(filename) - i)
				return false;
			i += a;
			continue;
		}
		if (*p == '$')
		{
			/* Variable conversions */
			const char *insert;
			switch(*++p)
			{
				case 'm':	/* mission */
					insert = Current_mission_filename;
					break;
				case 'p':	/* pilot */
					insert = get_local_player().callsign;
					break;
				default:
					return false;
			}
			i += filename.copy_if(i, insert);
			continue;
		}
		filename[i++] = *p;
		if (!*p)
			break;
		if (i >= sizeof(filename) - 1)
			return false;
	}
	return filename[0];
}

constexpr char demoname_allowed_chars[] = "azAZ09__--";
#define DEMO_FORMAT_STRING(S)	DEMO_DIR S "." DEMO_EXT
void newdemo_stop_recording()
{
	int exit;
	static sbyte tmpcnt = 0;
	ntstring<PATH_MAX - 1> filename;

	exit = 0;

	if (!nd_record_v_no_space)
	{
		newdemo_record_oneframeevent_update(0);
		newdemo_write_end();
	}

	outfile.reset();
	Newdemo_state = ND_STATE_NORMAL;
	gr_palette_load( gr_palette );
try_again:
	;

	Newmenu_allowed_chars = demoname_allowed_chars;
	if (guess_demo_name(filename))
	{
	}
	else if (!nd_record_v_no_space) {
		array<newmenu_item, 1> m{{
			nm_item_input(filename),
		}};
		exit = newmenu_do( NULL, TXT_SAVE_DEMO_AS, m, unused_newmenu_subfunction, unused_newmenu_userdata );
	} else if (nd_record_v_no_space == 2) {
		array<newmenu_item, 2> m{{
			nm_item_text(TXT_DEMO_SAVE_NOSPACE),
			nm_item_input(filename),
		}};
		exit = newmenu_do( NULL, NULL, m, unused_newmenu_subfunction, unused_newmenu_userdata );
	}
	Newmenu_allowed_chars = NULL;

	if (exit == -2) {                   // got bumped out from network menu
		char save_file[PATH_MAX];

		if (filename[0] != '\0') {
			snprintf(save_file, sizeof(save_file), DEMO_FORMAT_STRING("%s"), filename.data());
		} else
			snprintf(save_file, sizeof(save_file), DEMO_FORMAT_STRING("tmp%d"), tmpcnt++);
		remove(save_file);
		PHYSFSX_rename(DEMO_FILENAME, save_file);
		return;
	}
	if (exit == -1) {               // pressed ESC
		PHYSFS_delete(DEMO_FILENAME);   // might as well remove the file
		return;                     // return without doing anything
	}

	if (filename[0]==0) //null string
		goto try_again;

	//check to make sure name is ok
	range_for (const unsigned c, filename)
	{
		if (!c)
			break;
		if (!isalnum(c) && c != '_' && c != '-' && c != '.')
		{
			nm_messagebox(NULL, 1,TXT_CONTINUE, TXT_DEMO_USE_LETTERS);
			goto try_again;
		}
	}

	char fullname[PATH_MAX];
	snprintf(fullname, sizeof(fullname), DEMO_FORMAT_STRING("%s"), filename.data());
	PHYSFS_delete(fullname);
	PHYSFSX_rename(DEMO_FILENAME, fullname);
}

//returns the number of demo files on the disk
int newdemo_count_demos()
{
	int NumFiles=0;

	range_for (const auto i, PHYSFSX_findFiles(DEMO_DIR, demo_file_extensions))
	{
		(void)i;
		NumFiles++;
	}
	return NumFiles;
}

namespace dsx {

void newdemo_start_playback(const char * filename)
{
	enum purpose_type rnd_demo = PURPOSE_CHOSE_PLAY;
	char filename2[PATH_MAX+FILENAME_LEN] = DEMO_DIR;

	change_playernum_to(0);

	if (filename)
		strcat(filename2, filename);
	else
	{
		// Randomly pick a filename
		int NumFiles = 0, RandFileNum;

		rnd_demo = PURPOSE_RANDOM_PLAY;
		NumFiles = newdemo_count_demos();

		if ( NumFiles == 0 ) {
			CGameArg.SysAutoDemo = false;
			return;     // No files found!
		}
		RandFileNum = d_rand() % NumFiles;
		NumFiles = 0;

		range_for (const auto i, PHYSFSX_findFiles(DEMO_DIR, demo_file_extensions))
		{
			if (NumFiles == RandFileNum)
			{
				strcat(filename2, i);
				break;
			}
			NumFiles++;
		}
		if (NumFiles > RandFileNum)
		{
			CGameArg.SysAutoDemo = false;
			return;
		}
	}

	infile = PHYSFSX_openReadBuffered(filename2);

	if (!infile) {
		return;
	}

	nd_playback_v_bad_read = 0;
	change_playernum_to(0);                 // force playernum to 0
	auto &plr = get_local_player();
	nd_playback_v_save_callsign = plr.callsign;
	plr.lives = 0;
	Viewer = ConsoleObject = &Objects.front();   // play properly as if console player

	if (newdemo_read_demo_start(rnd_demo)) {
		infile.reset();
		return;
	}

	Game_mode = GM_NORMAL;
	Newdemo_state = ND_STATE_PLAYBACK;
	Newdemo_vcr_state = ND_STATE_PLAYBACK;
	nd_playback_v_demosize = PHYSFS_fileLength(infile);
	nd_playback_v_bad_read = 0;
	nd_playback_v_at_eof = 0;
	nd_playback_v_framecount = 0;
	nd_playback_v_style = NORMAL_PLAYBACK;
#if defined(DXX_BUILD_DESCENT_II)
	init_seismic_disturbances();
	PlayerCfg.Cockpit3DView[0] = PlayerCfg.Cockpit3DView[1] = CV_NONE;       //turn off 3d views on cockpit
	DemoDoLeft = DemoDoRight = 0;
	nd_playback_v_guided = 0;
#endif
	nd_playback_v_dead = nd_playback_v_rear = 0;
	HUD_clear_messages();
	if (!Game_wind)
		hide_menus();
	auto result = newdemo_playback_one_frame();       // this one loads new level
	result = std::max(newdemo_playback_one_frame(), result);       // get all of the objects to renderb game

	if (result == window_event_result::close)
		return;	// whoops, there was an error reading the first two frames! Abort!

	if (!Game_wind)
		Game_wind = game_setup();							// create game environment
}

}

namespace dsx {
void newdemo_stop_playback()
{
	infile.reset();
	Newdemo_state = ND_STATE_NORMAL;
	change_playernum_to(0);             //this is reality
	get_local_player().callsign = nd_playback_v_save_callsign;
	Rear_view=0;
	nd_playback_v_dead = nd_playback_v_rear = 0;
#if defined(DXX_BUILD_DESCENT_II)
	nd_playback_v_guided = 0;
#endif
	Newdemo_game_mode = Game_mode = GM_GAME_OVER;
	
	// Required for the editor
	obj_relink_all();
}
}


int newdemo_swap_endian(const char *filename)
{
	char inpath[PATH_MAX+FILENAME_LEN] = DEMO_DIR;
	int complete = 0;

	if (filename)
		strcat(inpath, filename);
	else
		return 0;

	infile = PHYSFSX_openReadBuffered(inpath);
	if (!infile)
		goto read_error;

	nd_playback_v_demosize = PHYSFS_fileLength(infile);	// should be exactly the same size
	outfile = PHYSFSX_openWriteBuffered(DEMO_FILENAME);
	if (!outfile)
	{
		infile.reset();
		goto read_error;
	}

	Newdemo_num_written = 0;
	nd_playback_v_bad_read = 0;
	swap_endian = 1;
	nd_playback_v_at_eof = 0;
	Newdemo_state = ND_STATE_NORMAL;	// not doing anything special really

	if (newdemo_read_demo_start(PURPOSE_REWRITE)) {
		infile.reset();
		outfile.reset();
		swap_endian = 0;
		return 0;
	}

	while (newdemo_read_frame_information(1) == 1) {}	// rewrite all frames

	newdemo_goto_end(1);	// get end of demo data
	newdemo_write_end();	// and write it

	swap_endian = 0;
	complete = nd_playback_v_demosize == Newdemo_num_written;
	infile.reset();
	outfile.reset();

	if (complete)
	{
		char bakpath[PATH_MAX+FILENAME_LEN];

		change_filename_extension(bakpath, inpath, DEMO_BACKUP_EXT);
		PHYSFSX_rename(inpath, bakpath);
		PHYSFSX_rename(DEMO_FILENAME, inpath);
	}
	else
		PHYSFS_delete(DEMO_FILENAME);	// clean up the mess

read_error:
	{
		nm_messagebox( NULL, 1, TXT_OK, complete ? "Demo %s converted%s" : "Error converting demo\n%s\n%s", filename,
					  complete ? "" : (nd_playback_v_at_eof ? TXT_DEMO_CORRUPT : PHYSFS_getLastError()));
	}

	return nd_playback_v_at_eof;
}

#ifndef NDEBUG

#define BUF_SIZE 16384

void newdemo_strip_frames(char *outname, int bytes_to_strip)
{
	char *buf;
	int read_elems, bytes_back;
	int trailer_start, loc1, loc2, stop_loc, bytes_to_read;
	short last_frame_length;

	const auto &&outfp = PHYSFSX_openWriteBuffered(outname);
	if (!outfp) {
		nm_messagebox( NULL, 1, TXT_OK, "Can't open output file" );
		newdemo_stop_playback();
		return;
	}
	MALLOC(buf, char, BUF_SIZE);
	if (buf == NULL) {
		nm_messagebox( NULL, 1, TXT_OK, "Can't malloc output buffer" );
		newdemo_stop_playback();
		return;
	}
	newdemo_goto_end(0);
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
		PHYSFS_write(outfp, buf, 1, read_elems);
		stop_loc -= read_elems;
	}
	stop_loc = PHYSFS_tell(outfp);
	PHYSFS_seek(infile, trailer_start);
	while ((read_elems = PHYSFS_read(infile, buf, 1, BUF_SIZE)) != 0)
		PHYSFS_write(outfp, buf, 1, read_elems);
	PHYSFS_seek(outfp, stop_loc);
	PHYSFS_seek(outfp, PHYSFS_tell(infile) + 1);
	PHYSFS_write(outfp, &last_frame_length, 2, 1);
	newdemo_stop_playback();

}

#endif

#if defined(DXX_BUILD_DESCENT_II)
static void nd_render_extras (ubyte which,const vcobjptr_t obj)
{
	ubyte w=which>>4;
	ubyte type=which&15;

	if (which==255)
	{
		Int3(); // how'd we get here?
		do_cockpit_window_view(w,WBU_WEAPON);
		return;
	}

	if (w)
	{
		DemoRightExtra = *obj;  DemoDoRight=type;
	}
	else
	{
		DemoLeftExtra = *obj; DemoDoLeft=type;
	}

}
#endif
