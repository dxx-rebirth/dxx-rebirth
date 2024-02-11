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

#include "wall.h"
#include "text.h"
#include "fireball.h"
#include "textures.h"
#include "newdemo.h"
#include "multi.h"
#include "gameseq.h"
#include "physfs-serial.h"
#include "gameseg.h"
#include "hudmsg.h"
#include "effects.h"

#include "d_enumerate.h"
#include "d_levelstate.h"
#include "d_range.h"
#include "compiler-range_for.h"
#include "partial_range.h"
#include "segiter.h"
#include "d_zip.h"

//	Special door on boss level which is locked if not in multiplayer...sorry for this awful solution --MK.
#define	BOSS_LOCKED_DOOR_LEVEL	7
#define	BOSS_LOCKED_DOOR_SEG		595
#define	BOSS_LOCKED_DOOR_SIDE	sidenum_t::WFRONT

namespace dcx {
unsigned Num_wall_anims;
}

namespace dsx {

namespace {

struct ad_removal_predicate
{
	bool operator()(active_door &) const;
};

struct find_active_door_predicate
{
	const wallnum_t wall_num;
	explicit find_active_door_predicate(const wallnum_t i) :
		wall_num(i)
	{
	}
	bool operator()(active_door &d) const {
		if (d.front_wallnum[0] == wall_num)
			return true;
		if (d.back_wallnum[0] == wall_num)
			return true;
		if (d.n_parts != 2)
			return false;
		if (d.front_wallnum[1] == wall_num)
			return true;
		if (d.back_wallnum[1] == wall_num)
			return true;
		return false;
	}
};

}

}
#if defined(DXX_BUILD_DESCENT_II)
#include "collide.h"
namespace dsx {
constexpr std::integral_constant<unsigned, F1_0> CLOAKING_WALL_TIME{};

namespace {

struct cwframe
{
	wall &w;
	std::array<uvl, 4> &uvls;
	cwframe(fvmsegptr &vmsegptr, wall &wr) :
		w(wr),
		uvls(vmsegptr(w.segnum)->unique_segment::sides[w.sidenum].uvls)
	{
	}
};

struct cwresult
{
	const bool remove;
	const bool record;
};

struct cw_removal_predicate
{
	fvmsegptr &vmsegptr;
	wall_array &Walls;
	unsigned num_cloaking_walls = 0;
	bool operator()(cloaking_wall &d);
};

struct find_cloaked_wall_predicate
{
	const vmwallidx_t w;
	bool operator()(const cloaking_wall &cw) const
	{
		return cw.front_wallnum == w || cw.back_wallnum == w;
	}
};

}

}
#endif

namespace {

static std::pair<uint_fast32_t, uint_fast32_t> get_transparency_check_values(const unique_side &side)
{
	if (const auto masked_tmap_num2 = static_cast<uint_fast32_t>(get_texture_index(side.tmap_num2)))
		return {masked_tmap_num2, BM_FLAG_SUPER_TRANSPARENT};
	return {get_texture_index(side.tmap_num), BM_FLAG_TRANSPARENT};
}

// This function determines whether the current segment/side is transparent
//		1 = YES
//		0 = NO
static uint_fast32_t check_transparency(const GameBitmaps_array &GameBitmaps, const Textures_array &Textures, const unique_side &side)
{
	const auto &&v = get_transparency_check_values(side);
	return GameBitmaps[Textures[v.first]].get_flag_mask(v.second);
}

}

//-----------------------------------------------------------------
// This function checks whether we can fly through the given side.
//	In other words, whether or not we have a 'doorway'
//	 Flags:
//		WALL_IS_DOORWAY_FLAG::FLY				1
//		WALL_IS_DOORWAY_FLAG::RENDER			2
//		WALL_IS_DOORWAY_FLAG::RENDPAST			4
//	 Return values:
//		wall_is_doorway_result::wall					2	// 0/1/0		wall
//		wall_is_doorway_result::transparent_wall		6	// 0/1/1		transparent wall
//		wall_is_doorway_result::illusory_wall			3	// 1/1/0		illusory wall
//		wall_is_doorway_result::transillusory_wall		7	// 1/1/1		transparent illusory wall
//		wall_is_doorway_result::no_wall					5	// 1/0/1		no wall, can fly through
namespace dsx {

namespace {

static wall_is_doorway_result wall_is_doorway(const GameBitmaps_array &GameBitmaps, const Textures_array &Textures, fvcwallptr &vcwallptr, const shared_side &sside, const unique_side &uside)
{
	auto &w = *vcwallptr(sside.wall_num);
	const auto type = w.type;
	if (type == WALL_OPEN)
		return wall_is_doorway_result::no_wall;

#if defined(DXX_BUILD_DESCENT_II)
	if (unlikely(type == WALL_CLOAKED))
		return wall_is_doorway_result::cloaked_wall;
#endif

	const auto flags = w.flags;
	if (type == WALL_ILLUSION) {
		if (flags & wall_flag::illusion_off)
			return wall_is_doorway_result::no_wall;
		else {
			if (check_transparency(GameBitmaps, Textures, uside))
				return wall_is_doorway_result::transillusory_wall;
		 	else
				return wall_is_doorway_result::illusory_wall;
		}
	}

	if (type == WALL_BLASTABLE) {
	 	if (flags & wall_flag::blasted)
			return wall_is_doorway_result::transillusory_wall;
	}	
	else
	{
	if (unlikely(flags & wall_flag::door_opened))
		return wall_is_doorway_result::transillusory_wall;
	if (likely(type == WALL_DOOR) && unlikely(w.state == wall_state::opening))
		return wall_is_doorway_result::transparent_wall;
	}
// If none of the above flags are set, there is no doorway.
	if (check_transparency(GameBitmaps, Textures, uside))
		return wall_is_doorway_result::transparent_wall;
	else
		return wall_is_doorway_result::wall; // There are children behind the door.
}

}

wall_is_doorway_result WALL_IS_DOORWAY(const GameBitmaps_array &GameBitmaps, const Textures_array &Textures, fvcwallptr &vcwallptr, const cscusegment seg, const sidenum_t side)
{
	const auto child = seg.s.children[side];
	if (unlikely(child == segment_none))
		return wall_is_doorway_result::wall;
	if (unlikely(child == segment_exit))
		return wall_is_doorway_result::external;
	auto &sside = seg.s.sides[side];
	if (likely(sside.wall_num == wall_none))
		return wall_is_doorway_result::no_wall;
	auto &uside = seg.u.sides[side];
	return wall_is_doorway(GameBitmaps, Textures, vcwallptr, sside, uside);
}

#if DXX_USE_EDITOR
//-----------------------------------------------------------------
// Initializes all the walls (in other words, no special walls)
void wall_init(d_level_unique_wall_subsystem_state &LevelUniqueWallSubsystemState)
{
	init_exploding_walls();
	auto &Walls = LevelUniqueWallSubsystemState.Walls;
	Walls.set_count(0);
	range_for (auto &w, Walls)
	{
		w.segnum = segment_none;
		w.sidenum = side_none;
		w.type = WALL_NORMAL;
		w.flags = {};
		w.hps = 0;
		w.trigger = trigger_none;
		w.clip_num = -1;
		w.linked_wall = wall_none;
	}
	auto &ActiveDoors = LevelUniqueWallSubsystemState.ActiveDoors;
	ActiveDoors.set_count(0);
#if defined(DXX_BUILD_DESCENT_II)
	auto &CloakingWalls = LevelUniqueWallSubsystemState.CloakingWalls;
	CloakingWalls.set_count(0);
#endif

}
#endif

//set the tmap_num or tmap_num2 field for a wall/door
void wall_set_tmap_num(const wclip &anim, const vmsegptridx_t seg, const sidenum_t side, const vmsegptridx_t csegp, const sidenum_t cside, const unsigned frame_num)
{
	const auto newdemo_state = Newdemo_state;
	if (newdemo_state == ND_STATE_PLAYBACK)
		return;

	const auto tmap = anim.frames[frame_num];
	auto &uside = seg->unique_segment::sides[side];
	auto &cuside = csegp->unique_segment::sides[cside];
	if (anim.flags & WCF_TMAP1)	{
		const texture1_value t1{tmap};
		if (t1 != uside.tmap_num || t1 != cuside.tmap_num)
		{
			uside.tmap_num = cuside.tmap_num = t1;
			if (newdemo_state == ND_STATE_RECORDING)
				newdemo_record_wall_set_tmap_num1(seg,side,csegp,cside,t1);
		}
	} else	{
		const texture2_value t2{tmap};
		if (t2 != uside.tmap_num2 || t2 != cuside.tmap_num2)
		{
			uside.tmap_num2 = cuside.tmap_num2 = t2;
			if (newdemo_state == ND_STATE_RECORDING)
				newdemo_record_wall_set_tmap_num2(seg,side,csegp,cside,t2);
		}
	}
}

}

namespace {

// -------------------------------------------------------------------------------
//when the wall has used all its hitpoints, this will destroy it
static void blast_blastable_wall(const vmsegptridx_t seg, const sidenum_t side, wall &w0)
{
	auto &LevelSharedVertexState = LevelSharedSegmentState.get_vertex_state();
	auto &Objects = LevelUniqueObjectState.Objects;
	auto &Vertices = LevelSharedVertexState.get_vertices();
	auto &vmobjptr = Objects.vmptr;
	auto &sside = seg->shared_segment::sides[side];
	const auto wall_num = sside.wall_num;
	w0.hps = -1;	//say it's blasted

	const auto &&csegp = seg.absolute_sibling(seg->shared_segment::children[side]);
	auto Connectside = find_connect_side(seg, csegp);
	Assert(Connectside != side_none);
	const auto cwall_num = csegp->shared_segment::sides[Connectside].wall_num;
	auto &Walls = LevelUniqueWallSubsystemState.Walls;
	auto &WallAnims = GameSharedState.WallAnims;
	auto &imwallptr = Walls.imptr;
	const auto &&w1 = imwallptr(cwall_num);
	if (w1)
		LevelUniqueStuckObjectState.kill_stuck_objects(vmobjptr, cwall_num);
	LevelUniqueStuckObjectState.kill_stuck_objects(vmobjptr, wall_num);
	flush_fcd_cache();

	const auto a = w0.clip_num;
	auto &wa = WallAnims[a];
	//if this is an exploding wall, explode it
	if (wa.flags & WCF_EXPLODES)
	{
		auto &vcvertptr = Vertices.vcptr;
		explode_wall(vcvertptr, seg, static_cast<sidenum_t>(side), w0);
	}
	else {
		//if not exploding, set final frame, and make door passable
		const auto n = wa.num_frames;
		w0.flags |= wall_flag::blasted;
		if (w1)
			w1->flags |= wall_flag::blasted;
		wall_set_tmap_num(wa, seg, side, csegp, Connectside, n - 1);
	}

}

}

//-----------------------------------------------------------------
// Destroys a blastable wall.
void wall_destroy(const vmsegptridx_t seg, const sidenum_t side)
{
	auto &Walls = LevelUniqueWallSubsystemState.Walls;
	auto &vmwallptr = Walls.vmptr;
	auto &w = *vmwallptr(seg->shared_segment::sides[side].wall_num);
	if (w.type == WALL_BLASTABLE)
		blast_blastable_wall(seg, side, w);
	else
		Error("Hey bub, you are trying to destroy an indestructable wall.");
}

//-----------------------------------------------------------------
// Deteriorate appearance of wall. (Changes bitmap (paste-ons))
void wall_damage(const vmsegptridx_t seg, const sidenum_t side, fix damage)
{
	auto &WallAnims = GameSharedState.WallAnims;
	int i;

	auto &sside = seg->shared_segment::sides[side];
	const auto wall_num = sside.wall_num;
	if (wall_num == wall_none) {
		return;
	}

	auto &Walls = LevelUniqueWallSubsystemState.Walls;
	auto &vmwallptr = Walls.vmptr;
	auto &w0 = *vmwallptr(wall_num);
	if (w0.type != WALL_BLASTABLE)
		return;
	
	if (!(w0.flags & wall_flag::blasted) && w0.hps >= 0)
		{
		const auto &&csegp = seg.absolute_sibling(seg->shared_segment::children[side]);
		auto Connectside = find_connect_side(seg, csegp);
		Assert(Connectside != side_none);
		const auto cwall_num = csegp->shared_segment::sides[Connectside].wall_num;
		auto &imwallptr = Walls.imptr;
		if (const auto &&w1p = imwallptr(cwall_num))
		{
			auto &w1 = *w1p;
			w1.hps -= damage;
		}
		w0.hps -= damage;

		const auto a = w0.clip_num;
		const auto n = WallAnims[a].num_frames;
		
		if (w0.hps < WALL_HPS*1/n) {
			blast_blastable_wall(seg, side, w0);
			if (Game_mode & GM_MULTI)
				multi_send_door_open(seg, side, w0.flags);
		}
		else
			for (i=0;i<n;i++)
				if (w0.hps < WALL_HPS*(n-i)/n)
				{
					wall_set_tmap_num(WallAnims[a], seg, side, csegp, Connectside, i);
				}
		}
}


//-----------------------------------------------------------------
// Opens a door
namespace dsx {
void wall_open_door(const vmsegptridx_t seg, const sidenum_t side)
{
	auto &LevelSharedVertexState = LevelSharedSegmentState.get_vertex_state();
	auto &Objects = LevelUniqueObjectState.Objects;
	auto &Vertices = LevelSharedVertexState.get_vertices();
	auto &WallAnims = GameSharedState.WallAnims;
	auto &vmobjptr = Objects.vmptr;
	active_door *d;

	auto &sside = seg->shared_segment::sides[side];
	const auto wall_num = sside.wall_num;
	auto &Walls = LevelUniqueWallSubsystemState.Walls;
	auto &vmwallptr = Walls.vmptr;
	wall *const w = vmwallptr(wall_num);
	LevelUniqueStuckObjectState.kill_stuck_objects(vmobjptr, wall_num);

	if (w->state == wall_state::opening ||		//already opening
		w->state == wall_state::waiting)		//open, waiting to close
		return;
#if defined(DXX_BUILD_DESCENT_II)
	if (w->state == wall_state::open)			//open, & staying open
		return;
#endif

	auto &ActiveDoors = LevelUniqueWallSubsystemState.ActiveDoors;
	auto &vmactdoorptr = ActiveDoors.vmptr;
	if (w->state == wall_state::closing) {		//closing, so reuse door
		const auto &&r = ranges::subrange(vmactdoorptr);
		const auto &&re = r.end();
		const auto &&i = ranges::find_if(r.begin(), re, find_active_door_predicate(wall_num));
		if (i == re)	// likely in demo playback or multiplayer
		{
			const auto c = ActiveDoors.get_count();
			ActiveDoors.set_count(c + 1);
			d = vmactdoorptr(static_cast<actdoornum_t>(c));
			d->time = 0;
		}
		else
		{
			d = *i;
			d->time = WallAnims[w->clip_num].play_time - d->time;
			if (d->time < 0)
				d->time = 0;
		}
	}
	else {											//create new door
		Assert(w->state == wall_state::closed);
		const auto i = ActiveDoors.get_count();
		ActiveDoors.set_count(i + 1);
		d = vmactdoorptr(static_cast<actdoornum_t>(i));
		d->time = 0;
	}


	w->state = wall_state::opening;

	// So that door can't be shot while opening
	const auto &&csegp = vcsegptr(seg->shared_segment::children[side]);
	auto Connectside = find_connect_side(seg, csegp);
	if (Connectside != side_none)
	{
		const auto cwall_num = csegp->shared_segment::sides[Connectside].wall_num;
		auto &imwallptr = Walls.imptr;
		if (const auto &&w1 = imwallptr(cwall_num))
		{
			w1->state = wall_state::opening;
			d->back_wallnum[0] = cwall_num;
		}
		d->front_wallnum[0] = seg->shared_segment::sides[side].wall_num;
	}
	else
		con_printf(CON_URGENT, "Illegal Connectside %i in wall_open_door on segment %hu. Trying to hop over. Please check your level!", underlying_value(side), seg.get_unchecked_index());

	if (Newdemo_state == ND_STATE_RECORDING) {
		newdemo_record_door_opening(seg, side);
	}

	if (w->linked_wall != wall_none)
	{
		wall *const w2 = vmwallptr(w->linked_wall);

		Assert(w2->linked_wall == seg->shared_segment::sides[side].wall_num);
		//Assert(!(w2->flags & WALL_DOOR_OPENING  ||  w2->flags & WALL_DOOR_OPENED));

		w2->state = wall_state::opening;

		const auto &&seg2 = vcsegptridx(w2->segnum);
		const auto &&csegp2 = vcsegptr(seg2->shared_segment::children[w2->sidenum]);
		Connectside = find_connect_side(seg2, csegp2);
		Assert(Connectside != side_none);
		const auto cwall_num = csegp2->shared_segment::sides[Connectside].wall_num;
		auto &imwallptr = Walls.imptr;
		if (const auto &&w3 = imwallptr(cwall_num))
			w3->state = wall_state::opening;

		d->n_parts = 2;
		d->front_wallnum[1] = w->linked_wall;
		d->back_wallnum[1] = cwall_num;
	}
	else
		d->n_parts = 1;


	if ( Newdemo_state != ND_STATE_PLAYBACK )
	{
		// NOTE THE LINK TO ABOVE!!!!
		auto &vcvertptr = Vertices.vcptr;
		const auto cp{compute_center_point_on_side(vcvertptr, seg, side)};
		const auto open_sound = WallAnims[w->clip_num].open_sound;
		if (open_sound > -1)
			digi_link_sound_to_pos(open_sound, seg, side, cp, 0, F1_0);

	}
}

#if defined(DXX_BUILD_DESCENT_II)
//-----------------------------------------------------------------
// start the transition from closed -> open wall
void start_wall_cloak(const vmsegptridx_t seg, const sidenum_t side)
{
	auto &LevelSharedVertexState = LevelSharedSegmentState.get_vertex_state();
	auto &Vertices = LevelSharedVertexState.get_vertices();
	cloaking_wall *d;

	if ( Newdemo_state==ND_STATE_PLAYBACK ) return;

	auto &Walls = LevelUniqueWallSubsystemState.Walls;
	const auto &&w = Walls.vmptridx(seg->shared_segment::sides[side].wall_num);

	if (w->type == WALL_OPEN || w->state == wall_state::cloaking)		//already open or cloaking
		return;

	const auto &&csegp = vcsegptr(seg->children[side]);
	auto Connectside = find_connect_side(seg, csegp);
	Assert(Connectside != side_none);
	const auto cwall_num = csegp->shared_segment::sides[Connectside].wall_num;

	auto &CloakingWalls = LevelUniqueWallSubsystemState.CloakingWalls;
	if (w->state == wall_state::decloaking)
	{	//decloaking, so reuse door
		const auto &&r = ranges::subrange(CloakingWalls.vmptr);
		const auto &&re = r.end();
		const auto &&i = ranges::find_if(r.begin(), re, find_cloaked_wall_predicate{w});
		if (i == re)
		{
			d_debugbreak();
			return;
		}
		d = *i;
		d->time = CLOAKING_WALL_TIME - d->time;
	}
	else if (w->state == wall_state::closed) {	//create new door
		const auto c = CloakingWalls.get_count();
		if (c >= CloakingWalls.size())
		{
			Int3();		//ran out of cloaking wall slots
			w->type = WALL_OPEN;
			if (const auto &&w1 = Walls.vmptr.check_untrusted(cwall_num))
				(*w1)->type = WALL_OPEN;
			return;
		}
		CloakingWalls.set_count(c + 1);
		d = CloakingWalls.vmptr(clwallnum_t{c});
		d->time = 0;
	}
	else {
		Int3();		//unexpected wall state
		return;
	}

	w->state = wall_state::cloaking;
	if (const auto &&w1 = Walls.imptr(cwall_num))
		w1->state = wall_state::cloaking;

	d->front_wallnum = seg->shared_segment::sides[side].wall_num;
	d->back_wallnum = cwall_num;
	Assert(w->linked_wall == wall_none);

	if ( Newdemo_state != ND_STATE_PLAYBACK ) {
		auto &vcvertptr = Vertices.vcptr;
		digi_link_sound_to_pos(sound_effect::SOUND_WALL_CLOAK_ON, seg, side, compute_center_point_on_side(vcvertptr, seg, side), 0, F1_0);
	}

	for (auto &&[front_ls, back_ls, s0_uvls, s1_uvls] : zip(
			d->front_ls,
			d->back_ls,
			seg->unique_segment::sides[side].uvls,
			csegp->unique_segment::sides[Connectside].uvls
	))
	{
		front_ls = s0_uvls.l;
		back_ls = s1_uvls.l;
	}
}

//-----------------------------------------------------------------
// start the transition from open -> closed wall
void start_wall_decloak(const vmsegptridx_t seg, const sidenum_t side)
{
	auto &LevelSharedVertexState = LevelSharedSegmentState.get_vertex_state();
	auto &Vertices = LevelSharedVertexState.get_vertices();
	cloaking_wall *d;

	if ( Newdemo_state==ND_STATE_PLAYBACK ) return;

	auto &sside = seg->shared_segment::sides[side];
	assert(sside.wall_num != wall_none); 	//Opening door on illegal wall

	auto &Walls = LevelUniqueWallSubsystemState.Walls;
	const auto &&w = Walls.vmptridx(sside.wall_num);

	if (w->type == WALL_CLOSED || w->state == wall_state::decloaking)		//already closed or decloaking
		return;

	auto &CloakingWalls = LevelUniqueWallSubsystemState.CloakingWalls;
	if (w->state == wall_state::cloaking) {	//cloaking, so reuse door
		const auto &&r = ranges::subrange(CloakingWalls.vmptr);
		const auto &&re = r.end();
		const auto &&i = ranges::find_if(r.begin(), re, find_cloaked_wall_predicate{w});
		if (i == re)
		{
			d_debugbreak();
			return;
		}
		d = *i;
		d->time = CLOAKING_WALL_TIME - d->time;
	}
	else if (w->state == wall_state::closed) {	//create new door
		const auto c = CloakingWalls.get_count();
		if (c >= CloakingWalls.size())
		{
			Int3();		//ran out of cloaking wall slots
			/* what is this _doing_ here?
			w->type = WALL_CLOSED;
			Walls[csegp->sides[Connectside].wall_num].type = WALL_CLOSED;
			*/
			return;
		}
		CloakingWalls.set_count(c + 1);
		d = CloakingWalls.vmptr(clwallnum_t{c});
		d->time = 0;
	}
	else {
		Int3();		//unexpected wall state
		return;
	}

	w->state = wall_state::decloaking;

	// So that door can't be shot while opening
	const auto &&csegp = vcsegptr(seg->children[side]);
	auto Connectside = find_connect_side(seg, csegp);
	Assert(Connectside != side_none);
	auto &csside = csegp->shared_segment::sides[Connectside];
	const auto cwall_num = csside.wall_num;
	if (const auto &&w1 = Walls.imptr(cwall_num))
		w1->state = wall_state::decloaking;

	d->front_wallnum = seg->shared_segment::sides[side].wall_num;
	d->back_wallnum = csside.wall_num;
	Assert(w->linked_wall == wall_none);

	if ( Newdemo_state != ND_STATE_PLAYBACK ) {
		auto &vcvertptr = Vertices.vcptr;
		digi_link_sound_to_pos(sound_effect::SOUND_WALL_CLOAK_OFF, seg, side, compute_center_point_on_side(vcvertptr, seg, side), 0, F1_0);
	}

	for (auto &&[front_ls, back_ls, s0_uvls, s1_uvls] : zip(
			d->front_ls,
			d->back_ls,
			seg->unique_segment::sides[side].uvls,
			csegp->unique_segment::sides[Connectside].uvls
	))
	{
		front_ls = s0_uvls.l;
		back_ls = s1_uvls.l;
	}
}
#endif

//-----------------------------------------------------------------
// This function closes the specified door and restores the closed
//  door texture.  This is called when the animation is done
void wall_close_door_ref(fvmsegptridx &vmsegptridx, wall_array &Walls, const wall_animations_array &WallAnims, active_door &d)
{
	range_for (const auto p, partial_const_range(d.front_wallnum, d.n_parts))
	{
		wall &w = *Walls.vmptr(p);

		const auto &&seg = vmsegptridx(w.segnum);
		const auto side = w.sidenum;
		w.state = wall_state::closed;

		assert(seg->shared_segment::sides[side].wall_num != wall_none);		//Closing door on illegal wall

		const auto &&csegp = seg.absolute_sibling(seg->shared_segment::children[side]);
		auto Connectside = find_connect_side(seg, csegp);
		Assert(Connectside != side_none);
		const auto cwall_num = csegp->shared_segment::sides[Connectside].wall_num;
		if (const auto &&w1 = Walls.imptr(cwall_num))
			w1->state = wall_state::closed;

		wall_set_tmap_num(WallAnims[w.clip_num], seg, side, csegp, Connectside, 0);
	}
}

}

namespace dcx {

namespace {

static unsigned check_poke(fvcvertptr &vcvertptr, const object_base &obj, const shared_segment &seg, const sidenum_t side)
{
	//note: don't let objects with zero size block door
	if (!obj.size)
		return 0;
	return get_seg_masks(vcvertptr, obj.pos, seg, obj.size).sidemask & build_sidemask(side);		//pokes through side!
}

}

}

namespace dsx {

namespace {

static unsigned is_door_side_obstructed(fvcobjptridx &vcobjptridx, fvcsegptr &vcsegptr, const cscusegment seg, const sidenum_t side)
{
	auto &LevelSharedVertexState = LevelSharedSegmentState.get_vertex_state();
	auto &Vertices = LevelSharedVertexState.get_vertices();
	auto &vcvertptr = Vertices.vcptr;
	range_for (const object_base &obj, objects_in(seg, vcobjptridx, vcsegptr))
	{
#if defined(DXX_BUILD_DESCENT_II)
		if (obj.type == OBJ_WEAPON)
			continue;
		if (obj.type == OBJ_FIREBALL)
			continue;
#endif
		if (const auto obstructed = check_poke(vcvertptr, obj, seg, side))
			return obstructed;
	}
	return 0;
}

//returns true if door is obstructed (& thus cannot close)
static unsigned is_door_obstructed(fvcobjptridx &vcobjptridx, fvcsegptr &vcsegptr, const vcsegptridx_t seg, const sidenum_t side)
{
	if (const auto obstructed = is_door_side_obstructed(vcobjptridx, vcsegptr, seg, side))
		return obstructed;
	const auto &&csegp = vcsegptr(seg->shared_segment::children[side]);
	const auto &&Connectside = find_connect_side(seg, csegp);
	Assert(Connectside != side_none);
	//go through each object in each of two segments, and see if
	//it pokes into the connecting seg
	return is_door_side_obstructed(vcobjptridx, vcsegptr, csegp, Connectside);
}

}

#if defined(DXX_BUILD_DESCENT_II)
//-----------------------------------------------------------------
// Closes a door
void wall_close_door(wall_array &Walls, const vmsegptridx_t seg, const sidenum_t side)
{
	auto &LevelSharedVertexState = LevelSharedSegmentState.get_vertex_state();
	auto &Objects = LevelUniqueObjectState.Objects;
	auto &Vertices = LevelSharedVertexState.get_vertices();
	auto &WallAnims = GameSharedState.WallAnims;
	auto &vcobjptridx = Objects.vcptridx;
	active_door *d;

	const auto wall_num = seg->shared_segment::sides[side].wall_num;
	wall *const w = Walls.vmptr(wall_num);
	if ((w->state == wall_state::closing) ||		//already closing
		 (w->state == wall_state::waiting)	||		//open, waiting to close
		 (w->state == wall_state::closed))			//closed
		return;

	if (is_door_obstructed(vcobjptridx, vcsegptr, seg, side))
		return;

	auto &ActiveDoors = LevelUniqueWallSubsystemState.ActiveDoors;
	auto &vmactdoorptr = ActiveDoors.vmptr;
	if (w->state == wall_state::opening)
	{	//reuse door
		const auto &&r = ranges::subrange(vmactdoorptr);
		const auto &&re = r.end();
		const auto &&i = ranges::find_if(r.begin(), re, find_active_door_predicate(wall_num));
		if (i == re)
		{
			d_debugbreak();
			return;
		}
		d = *i;
		d->time = WallAnims[w->clip_num].play_time - d->time;

		if (d->time < 0)
			d->time = 0;

	}
	else {											//create new door
		assert(w->state == wall_state::open);
		const auto i = ActiveDoors.get_count();
		ActiveDoors.set_count(i + 1);
		d = vmactdoorptr(static_cast<actdoornum_t>(i));
		d->time = 0;
	}

	w->state = wall_state::closing;

	// So that door can't be shot while opening
	const auto &&csegp = vcsegptr(seg->children[side]);
	const auto &&Connectside = find_connect_side(seg, csegp);
	Assert(Connectside != side_none);
	const auto cwall_num = csegp->shared_segment::sides[Connectside].wall_num;
	if (const auto &&w1 = Walls.imptr(cwall_num))
		w1->state = wall_state::closing;

	d->front_wallnum[0] = seg->shared_segment::sides[side].wall_num;
	d->back_wallnum[0] = cwall_num;
	if (Newdemo_state == ND_STATE_RECORDING) {
		newdemo_record_door_opening(seg, side);
	}

	if (w->linked_wall != wall_none)
	{
		Int3();		//don't think we ever used linked walls
	}
	else
		d->n_parts = 1;


	if ( Newdemo_state != ND_STATE_PLAYBACK )
	{
		// NOTE THE LINK TO ABOVE!!!!
		auto &vcvertptr = Vertices.vcptr;
		const auto cp{compute_center_point_on_side(vcvertptr, seg, side)};
		const auto open_sound = WallAnims[w->clip_num].open_sound;
		if (open_sound > -1)
			digi_link_sound_to_pos(open_sound, seg, side, cp, 0, F1_0);

	}
}
#endif

namespace {

//-----------------------------------------------------------------
// Animates opening of a door.
// Called in the game loop.
static bool do_door_open(active_door &d)
{
	auto &Objects = LevelUniqueObjectState.Objects;
	auto &WallAnims = GameSharedState.WallAnims;
	auto &vmobjptr = Objects.vmptr;
	bool remove = false;
	auto &Walls = LevelUniqueWallSubsystemState.Walls;
	auto &vmwallptr = Walls.vmptr;
	for (unsigned p = 0; p < d.n_parts; ++p)
	{
		fix time_elapsed, time_total, one_frame;
		int i;

		wall &w = *vmwallptr(d.front_wallnum[p]);
		LevelUniqueStuckObjectState.kill_stuck_objects(vmobjptr, d.front_wallnum[p]);
		LevelUniqueStuckObjectState.kill_stuck_objects(vmobjptr, d.back_wallnum[p]);

		const auto &&seg = vmsegptridx(w.segnum);
		const auto side = w.sidenum;

		if (seg->shared_segment::sides[side].wall_num == wall_none)
		{
			con_printf(CON_URGENT, "Trying to do_door_open on illegal wall %i. Please check your level!", underlying_value(side));
			continue;
		}

		const auto &&csegp = seg.absolute_sibling(seg->shared_segment::children[side]);
		const auto &&Connectside = find_connect_side(seg, csegp);
		Assert(Connectside != side_none);

		d.time += FrameTime;

		time_elapsed = d.time;
		auto &wa = WallAnims[w.clip_num];
		const auto n = wa.num_frames;
		time_total = wa.play_time;

		one_frame = time_total/n;	

		i = time_elapsed/one_frame;

		if (i < n)
			wall_set_tmap_num(wa, seg, side, csegp, Connectside, i);

		const auto cwall_num = csegp->shared_segment::sides[Connectside].wall_num;
		auto &w1 = *vmwallptr(cwall_num);
		if (i> n/2) {
			w.flags |= wall_flag::door_opened;
			w1.flags |= wall_flag::door_opened;
		}

		if (i >= n-1) {
			wall_set_tmap_num(wa, seg, side, csegp, Connectside, n - 1);

			// If our door is not automatic just remove it from the list.
			if (!(w.flags & wall_flag::door_auto))
			{
				remove = true;
#if defined(DXX_BUILD_DESCENT_II)
				w.state = wall_state::open;
				w1.state = wall_state::open;
#endif
			}
			else {
				w.state = wall_state::waiting;
				w1.state = wall_state::waiting;
			}
		}

	}
	flush_fcd_cache();
	return remove;
}

//-----------------------------------------------------------------
// Animates and processes the closing of a door.
// Called from the game loop.
static bool do_door_close(active_door &d)
{
	auto &LevelSharedVertexState = LevelSharedSegmentState.get_vertex_state();
	auto &Objects = LevelUniqueObjectState.Objects;
	auto &Vertices = LevelSharedVertexState.get_vertices();
	auto &vcobjptridx = Objects.vcptridx;
	auto &Walls = LevelUniqueWallSubsystemState.Walls;
	auto &WallAnims = GameSharedState.WallAnims;
	auto &vmwallptr = Walls.vmptr;
	auto &w0 = *vmwallptr(d.front_wallnum[0]);
	const auto &&seg0 = vmsegptridx(w0.segnum);

	//check for objects in doorway before closing
	if (w0.flags & wall_flag::door_auto)
		if (is_door_obstructed(vcobjptridx, vcsegptr, seg0, w0.sidenum))
		{
#if defined(DXX_BUILD_DESCENT_II)
			digi_kill_sound_linked_to_segment(w0.segnum, w0.sidenum, -1);
			wall_open_door(seg0, w0.sidenum);		//re-open door
#endif
			return false;
		}

	bool played_sound = false;
	bool remove = false;
	range_for (const auto p, partial_const_range(d.front_wallnum, d.n_parts))
	{
		fix time_elapsed, time_total, one_frame;
		int i;

		auto &wp = *vmwallptr(p);

		const auto &&seg = vmsegptridx(wp.segnum);
		const auto side = wp.sidenum;

		if (seg->shared_segment::sides[side].wall_num == wall_none) {
			return false;
		}

#if defined(DXX_BUILD_DESCENT_I)
		//if here, must be auto door
//don't assert here, because now we have triggers to close non-auto doors
		assert(wp.flags & wall_flag::door_auto);
#endif

		// Otherwise, close it.
		const auto &&csegp = seg.absolute_sibling(seg->shared_segment::children[side]);
		const auto &&Connectside = find_connect_side(seg, csegp);
		Assert(Connectside != side_none);


		auto &wa = WallAnims[wp.clip_num];
		if ( Newdemo_state != ND_STATE_PLAYBACK )
		{
			// NOTE THE LINK TO ABOVE!!
			if (!played_sound)	//only play one sound for linked doors
			{
				played_sound = true;
				if (d.time == 0)
				{		//first time
					const auto close_sound = wa.close_sound;
					if (close_sound > -1)
					{
						auto &vcvertptr = Vertices.vcptr;
						digi_link_sound_to_pos(close_sound, seg, side, compute_center_point_on_side(vcvertptr, seg, side), 0, F1_0);
					}
				}
			}
		}

		d.time += FrameTime;

		time_elapsed = d.time;
		const auto n = wa.num_frames;
		time_total = wa.play_time;

		one_frame = time_total/n;	

		i = n-time_elapsed/one_frame-1;

		const auto cwall_num = csegp->shared_segment::sides[Connectside].wall_num;
		auto &w1 = *vmwallptr(cwall_num);
		if (i < n/2) {
			wp.flags &= ~wall_flag::door_opened;
			w1.flags &= ~wall_flag::door_opened;
		}

		// Animate door.
		if (i > 0) {
			wall_set_tmap_num(wa, seg, side, csegp, Connectside, i);

			wp.state = wall_state::closing;
			w1.state = wall_state::closing;
		} else
			remove = true;
	}

	if (remove)
		wall_close_door_ref(Segments.vmptridx, Walls, WallAnims, d);
	return remove;
}

static std::pair<wall *, wall *> wall_illusion_op(fvmwallptr &vmwallptr, const vcsegptridx_t seg, const sidenum_t side)
{
	const auto wall0 = seg->shared_segment::sides[side].wall_num;
	if (wall0 == wall_none)
		return {nullptr, nullptr};
	const shared_segment &csegp = *seg.absolute_sibling(seg->shared_segment::children[side]);
	const auto &&cside = find_connect_side(seg, csegp);
	if (cside == side_none)
	{
		assert(cside != side_none);
		return {nullptr, nullptr};
	}
	const auto wall1 = csegp.sides[cside].wall_num;
	if (wall1 == wall_none)
		return {nullptr, nullptr};
	return {vmwallptr(wall0), vmwallptr(wall1)};
}

template <typename F>
static void wall_illusion_op(fvmwallptr &vmwallptr, const vcsegptridx_t seg, const sidenum_t side, const F op)
{
	const auto &&r = wall_illusion_op(vmwallptr, seg, side);
	if (r.first)
	{
		op(*r.first);
		op(*r.second);
	}
}

}

//-----------------------------------------------------------------
// Turns off an illusionary wall (This will be used primarily for
//  wall switches or triggers that can turn on/off illusionary walls.)
void wall_illusion_off(fvmwallptr &vmwallptr, const vcsegptridx_t seg, const sidenum_t side)
{
	const auto &&op = [](wall &w) {
		w.flags |= wall_flag::illusion_off;
	};
	wall_illusion_op(vmwallptr, seg, side, op);
}

//-----------------------------------------------------------------
// Turns on an illusionary wall (This will be used primarily for
//  wall switches or triggers that can turn on/off illusionary walls.)
void wall_illusion_on(fvmwallptr &vmwallptr, const vcsegptridx_t seg, const sidenum_t side)
{
	const auto &&op = [](wall &w) {
		w.flags &= ~wall_flag::illusion_off;
	};
	wall_illusion_op(vmwallptr, seg, side, op);
}

}

namespace {

//	-----------------------------------------------------------------------------
//	Allowed to open the normally locked special boss door if in multiplayer mode.
static int special_boss_opening_allowed(const segnum_t segnum, const sidenum_t sidenum)
{
	if (Game_mode & GM_MULTI)
		return (Current_level_num == BOSS_LOCKED_DOOR_LEVEL) && (segnum == BOSS_LOCKED_DOOR_SEG) && (sidenum == BOSS_LOCKED_DOOR_SIDE);
	else
		return 0;
}

}

//-----------------------------------------------------------------
// Determines what happens when a wall is shot
//returns info about wall.  see wall.h for codes
//obj is the object that hit...either a weapon or the player himself
//playernum is the number the player who hit the wall or fired the weapon,
//or -1 if a robot fired the weapon
namespace dsx {
wall_hit_process_t wall_hit_process(const player_flags powerup_flags, const vmsegptridx_t seg, const sidenum_t side, const fix damage, const unsigned playernum, const object &obj)
{
	fix	show_message;

	// If it is not a "wall" then just return.
	const auto wall_num = seg->shared_segment::sides[side].wall_num;
	if (wall_num == wall_none)
		return wall_hit_process_t::WHP_NOT_SPECIAL;

	auto &Walls = LevelUniqueWallSubsystemState.Walls;
	auto &vmwallptr = Walls.vmptr;
	wall *const w = vmwallptr(wall_num);

	if ( Newdemo_state == ND_STATE_RECORDING )
		newdemo_record_wall_hit_process( seg, side, damage, playernum );

	if (w->type == WALL_BLASTABLE) {
#if defined(DXX_BUILD_DESCENT_II)
		if (obj.ctype.laser_info.parent_type == OBJ_PLAYER)
#endif
			wall_damage(seg, side, damage);
		return wall_hit_process_t::WHP_BLASTABLE;
	}

	if (playernum != Player_num)	//return if was robot fire
		return wall_hit_process_t::WHP_NOT_SPECIAL;

	//	Determine whether player is moving forward.  If not, don't say negative
	//	messages because he probably didn't intentionally hit the door.
	if (obj.type == OBJ_PLAYER)
		show_message = (vm_vec_dot(obj.orient.fvec, obj.mtype.phys_info.velocity) > 0);
#if defined(DXX_BUILD_DESCENT_II)
	else if (obj.type == OBJ_ROBOT)
		show_message = 0;
	else if (obj.type == OBJ_WEAPON && obj.ctype.laser_info.parent_type == OBJ_ROBOT)
		show_message = 0;
#endif
	else
		show_message = 1;

	/* Set key_color only after the type matches, since TXT_* are macros
	 * that trigger a load from memory.  Use operator,() to suppress the
	 * truth test on the second branch since the compiler cannot prove
	 * that the loaded value will always be non-null.
	 */
	const char *key_color;
	if (
		(w->keys == wall_key::blue && (key_color = TXT_BLUE, true)) ||
		(w->keys == wall_key::gold && (key_color = TXT_YELLOW, true)) ||
		(w->keys == wall_key::red && (key_color = TXT_RED, true))
	)
	{
		if (!(powerup_flags & static_cast<PLAYER_FLAG>(w->keys)))
		{
			static_assert(static_cast<unsigned>(wall_key::blue) == static_cast<unsigned>(PLAYER_FLAGS_BLUE_KEY), "BLUE key flag mismatch");
			static_assert(static_cast<unsigned>(wall_key::gold) == static_cast<unsigned>(PLAYER_FLAGS_GOLD_KEY), "GOLD key flag mismatch");
			static_assert(static_cast<unsigned>(wall_key::red) == static_cast<unsigned>(PLAYER_FLAGS_RED_KEY), "RED key flag mismatch");
				if (show_message)
					HUD_init_message(HM_DEFAULT, "%s %s",key_color,TXT_ACCESS_DENIED);
			return wall_hit_process_t::WHP_NO_KEY;
		}
	}

	if (w->type == WALL_DOOR)
	{
		if ((w->flags & wall_flag::door_locked) && !special_boss_opening_allowed(seg, side))
		{
				if (show_message)
				{
					const auto &&m = TXT_CANT_OPEN_DOOR;
					HUD_init_message_literal(HM_DEFAULT, {m, strlen(m)});
				}
			return wall_hit_process_t::WHP_NO_KEY;
		}
		else {
			if (w->state != wall_state::opening)
			{
				wall_open_door(seg, side);
				if (Game_mode & GM_MULTI)
				{
#if defined(DXX_BUILD_DESCENT_I)
					const wall_flags flags{};
#elif defined(DXX_BUILD_DESCENT_II)
					const auto flags = w->flags;
#endif
					multi_send_door_open(seg, side,flags);
				}
			}
			return wall_hit_process_t::WHP_DOOR;
			
		}
	}
	return wall_hit_process_t::WHP_NOT_SPECIAL;		//default is treat like normal wall
}

//-----------------------------------------------------------------
// Opens doors/destroys wall/shuts off triggers.
void wall_toggle(fvmwallptr &vmwallptr, const vmsegptridx_t segp, const sidenum_t side)
{
	if (side >= MAX_SIDES_PER_SEGMENT)
	{
#ifndef NDEBUG
		Warning("Can't toggle side %u of segment %d (%u)!\n", underlying_value(side), segp.get_unchecked_index(), Highest_segment_index);
#endif
		return;
	}
	const auto wall_num = segp->shared_segment::sides[side].wall_num;
	if (wall_num == wall_none)
	{
		LevelError("Ignoring attempt to toggle wall in segment %hu, side %u: no wall exists there.", segp.get_unchecked_index(), underlying_value(side));
		return;
	}

	if ( Newdemo_state == ND_STATE_RECORDING )
		newdemo_record_wall_toggle(segp, side);

	wall *const w = vmwallptr(wall_num);
	if (w->type == WALL_BLASTABLE)
		wall_destroy(segp, side);

	if (w->type == WALL_DOOR && w->state == wall_state::closed)
		wall_open_door(segp, side);
}

bool ad_removal_predicate::operator()(active_door &d) const
{
#if defined(DXX_BUILD_DESCENT_II)
	auto &Objects = LevelUniqueObjectState.Objects;
	auto &vcobjptridx = Objects.vcptridx;
#endif
	auto &Walls = LevelUniqueWallSubsystemState.Walls;
	wall &w = *Walls.vmptr(d.front_wallnum[0]);
	if (w.state == wall_state::opening)
		return do_door_open(d);
	else if (w.state == wall_state::closing)
		return do_door_close(d);
	else if (w.state == wall_state::waiting)
	{
		d.time += FrameTime;
		// set flags to fix occasional netgame problem where door is waiting to close but open flag isn't set
		w.flags |= wall_flag::door_opened;
		if (wall *const w1 = Walls.imptr(d.back_wallnum[0]))
			w1->flags |= wall_flag::door_opened;
		if (d.time > DOOR_WAIT_TIME)
#if defined(DXX_BUILD_DESCENT_II)
			if (!is_door_obstructed(vcobjptridx, vcsegptr, vcsegptridx(w.segnum), w.sidenum))
#endif
			{
				w.state = wall_state::closing;
				d.time = 0;
			}
	}
	return false;
}

#if defined(DXX_BUILD_DESCENT_II)
namespace {

static void copy_cloaking_wall_light_to_wall(std::array<uvl, 4> &back_uvls, std::array<uvl, 4> &front_uvls, const cloaking_wall &d)
{
	range_for (const uint_fast32_t i, xrange(4u))
	{
		back_uvls[i].l = d.back_ls[i];
		front_uvls[i].l = d.front_ls[i];
	}
}

static void scale_cloaking_wall_light_to_wall(std::array<uvl, 4> &back_uvls, std::array<uvl, 4> &front_uvls, const cloaking_wall &d, const fix light_scale)
{
	range_for (const uint_fast32_t i, xrange(4u))
	{
		back_uvls[i].l = fixmul(d.back_ls[i], light_scale);
		front_uvls[i].l = fixmul(d.front_ls[i], light_scale);
	}
}

static cwresult do_cloaking_wall_frame(const bool initial, cloaking_wall &d, const cwframe front, const cwframe back)
{
	if (d.time > CLOAKING_WALL_TIME) {
		front.w.type = back.w.type = WALL_OPEN;
		front.w.state = back.w.state = wall_state::closed;		//why closed? why not?
		return {
			.remove = true,
			.record = initial,
		};
	}
	else if (d.time > CLOAKING_WALL_TIME/2) {
		const int8_t cloak_value = ((d.time - CLOAKING_WALL_TIME / 2) * (GR_FADE_LEVELS - 2)) / (CLOAKING_WALL_TIME / 2);
		bool record = initial;
		if (front.w.cloak_value != cloak_value)
		{
			record = true;
			front.w.cloak_value = back.w.cloak_value = cloak_value;
		}

		if (front.w.type != WALL_CLOAKED)
		{		//just switched
			front.w.type = back.w.type = WALL_CLOAKED;
			copy_cloaking_wall_light_to_wall(back.uvls, front.uvls, d);
		}
		return {
			.remove = false,
			.record = record,
		};
	}
	else {		//fading out
		fix light_scale;
		light_scale = fixdiv(CLOAKING_WALL_TIME / 2 - d.time, CLOAKING_WALL_TIME / 2);
		scale_cloaking_wall_light_to_wall(back.uvls, front.uvls, d, light_scale);
		return {
			.remove = false,
			.record = initial,
		};
	}
}

static cwresult do_decloaking_wall_frame(const bool initial, cloaking_wall &d, const cwframe front, const cwframe back)
{
	if (d.time > CLOAKING_WALL_TIME) {

		back.w.state = wall_state::closed;
		front.w.state = wall_state::closed;
		copy_cloaking_wall_light_to_wall(back.uvls, front.uvls, d);
		return {
			.remove = true,
			.record = initial,
		};
	}
	else if (d.time > CLOAKING_WALL_TIME/2) {		//fading in
		fix light_scale;
		front.w.type = back.w.type = WALL_CLOSED;

		light_scale = fixdiv(d.time - CLOAKING_WALL_TIME / 2, CLOAKING_WALL_TIME / 2);
		scale_cloaking_wall_light_to_wall(back.uvls, front.uvls, d, light_scale);
		return {
			.remove = false,
			.record = initial,
		};
	}
	else {		//cloaking in
		const int8_t cloak_value = ((CLOAKING_WALL_TIME / 2 - d.time) * (GR_FADE_LEVELS - 2)) / (CLOAKING_WALL_TIME / 2);
		bool record = initial;
		if (front.w.cloak_value != cloak_value)
		{
			record = true;
			front.w.cloak_value = back.w.cloak_value = cloak_value;
		}
		front.w.type = WALL_CLOAKED;
		back.w.type = WALL_CLOAKED;
		return {
			.remove = false,
			.record = record,
		};
	}
}

bool cw_removal_predicate::operator()(cloaking_wall &d)
{
	const cwframe front(vmsegptr, *Walls.vmptr(d.front_wallnum));
	const auto &&wpback = Walls.imptr(d.back_wallnum);
	const cwframe back = (wpback ? cwframe(vmsegptr, *wpback) : front);
	const bool initial = (d.time == 0);
	d.time += FrameTime;

	const cwresult r{
		(front.w.state == wall_state::cloaking)
			? do_cloaking_wall_frame(initial, d, front, back)
			: (front.w.state == wall_state::decloaking)
			? do_decloaking_wall_frame(initial, d, front, back)
			: (
				d_debugbreak(),	//unexpected wall state
				cwresult{}
			)
	};
	if (r.record)
	{
		// check if the actual cloak_value changed in this frame to prevent redundant recordings and wasted bytes
		if (Newdemo_state == ND_STATE_RECORDING && r.record)
			newdemo_record_cloaking_wall(d.front_wallnum, d.back_wallnum, front.w.type, front.w.state, front.w.cloak_value, front.uvls[0].l, front.uvls[1].l, front.uvls[2].l, front.uvls[3].l);
	}
	if (!r.remove)
		++ num_cloaking_walls;
	return r.remove;
}

}
#endif

namespace {

static void process_exploding_walls(const d_robot_info_array &Robot_info)
{
	if (Newdemo_state == ND_STATE_PLAYBACK)
		return;
	if (unsigned num_exploding_walls = Num_exploding_walls)
	{
		auto &Walls = LevelUniqueWallSubsystemState.Walls;
		range_for (auto &&wp, Walls.vmptr)
		{
			auto &w1 = *wp;
			if (w1.flags & wall_flag::exploding)
			{
				assert(num_exploding_walls);
				const auto n = do_exploding_wall_frame(Robot_info, w1);
				num_exploding_walls -= n;
				if (!num_exploding_walls)
					break;
			}
		}
	}
}

}

void wall_frame_process(const d_robot_info_array &Robot_info)
{
	process_exploding_walls(Robot_info);
	{
		auto &ActiveDoors = LevelUniqueWallSubsystemState.ActiveDoors;
		const auto &&r = partial_range(ActiveDoors, ActiveDoors.get_count());
		auto &&i = std::remove_if(r.begin(), r.end(), ad_removal_predicate());
		ActiveDoors.set_count(std::distance(r.begin(), i));
	}
#if defined(DXX_BUILD_DESCENT_II)
	if (Newdemo_state != ND_STATE_PLAYBACK)
	{
		auto &CloakingWalls = LevelUniqueWallSubsystemState.CloakingWalls;
		const auto &&r = partial_range(CloakingWalls, CloakingWalls.get_count());
		auto &Walls = LevelUniqueWallSubsystemState.Walls;
		cw_removal_predicate rp{Segments.vmptr, Walls};
		static_cast<void>(std::remove_if(r.begin(), r.end(), std::ref(rp)));
		CloakingWalls.set_count(rp.num_cloaking_walls);
	}
#endif
}

d_level_unique_stuck_object_state LevelUniqueStuckObjectState;

//	An object got stuck in a door (like a flare).
//	Add global entry.
void d_level_unique_stuck_object_state::add_stuck_object(fvcwallptr &vcwallptr, const vmobjptridx_t objp, const shared_segment &segp, const sidenum_t sidenum)
{
	const auto wallnum = segp.sides[sidenum].wall_num;
	if (wallnum != wall_none)
	{
		if (vcwallptr(wallnum)->flags & wall_flag::blasted)
		{
			objp->flags |= OF_SHOULD_BE_DEAD;
			return;
		}
		if (Num_stuck_objects >= Stuck_objects.size())
		{
			assert(Num_stuck_objects <= Stuck_objects.size());
			con_printf(CON_NORMAL, "%s:%u: all stuck objects are busy; terminating %hu early", __FILE__, __LINE__, objp.get_unchecked_index());
			objp->flags |= OF_SHOULD_BE_DEAD;
			return;
		}
		auto &so = Stuck_objects[Num_stuck_objects++];
		so.wallnum = wallnum;
		so.objnum = objp;
	}
}

void d_level_unique_stuck_object_state::remove_stuck_object(const vcobjidx_t obj)
{
	auto &&pr = partial_range(Stuck_objects, Num_stuck_objects);
	auto &&pre = pr.end();
	const auto predicate = [obj](const stuckobj &so) { return so.objnum == obj; };
	const auto &&i = ranges::find_if(pr.begin(), pre, predicate);
	if (i == pre)
		/* Objects enter this function if they are able to become stuck,
		 * without regard to whether they actually are stuck.  If the
		 * object terminated without being stuck in a wall, then it will
		 * not be found in Stuck_objects.
		 */
		return;
	/* If pr.begin() == pr.end(), then i == pr.end(), and this line
	 * cannot be reached.
	 *
	 * If pr.begin() != pr.end(), then prev(pr.end()) must point to a
	 * valid element.
	 *
	 * Move that valid element to the location vacated by the removed
	 * object.  This may be a self-move if the removed object is the
	 * last object.
	 */
	auto &last_element = *std::prev(pr.end());
	static_assert(std::is_trivially_move_assignable<stuckobj>::value, "stuckobj move may require a check to prevent self-move");
	*i = std::move(last_element);
	DXX_POISON_VAR(last_element.wallnum, 0xcc);
	DXX_POISON_VAR(last_element.objnum, 0xcc);
	-- Num_stuck_objects;
}

//	----------------------------------------------------------------------------------------------------
//	Door with wall index wallnum is opening, kill all objects stuck in it.
void d_level_unique_stuck_object_state::kill_stuck_objects(fvmobjptr &vmobjptr, const vcwallidx_t wallnum)
{
	if (!Num_stuck_objects)
		return;
	auto &&pr = partial_range(Stuck_objects, Num_stuck_objects);
	const auto predicate = [&vmobjptr, wallnum](const stuckobj &so)
	{
		if (so.wallnum != wallnum)
			return false;
		auto &obj = *vmobjptr(so.objnum);
#if defined(DXX_BUILD_DESCENT_I)
#define DXX_WEAPON_LIFELEFT	F1_0/4
#elif defined(DXX_BUILD_DESCENT_II)
#define DXX_WEAPON_LIFELEFT	F1_0/8
#endif
		assert(obj.type == OBJ_WEAPON);
		assert(obj.movement_source == object::movement_type::physics);
		assert(obj.mtype.phys_info.flags & PF_STICK);
		obj.lifeleft = DXX_WEAPON_LIFELEFT;
		return true;
	};
	const auto i = std::remove_if(pr.begin(), pr.end(), predicate);
	static_assert(std::is_trivially_destructible<stuckobj>::value, "stuckobj destructor not called");
	Num_stuck_objects = std::distance(pr.begin(), i);
	std::array<stuckobj, 1> empty;
	DXX_POISON_VAR(empty, 0xcc);
	std::fill(i, pr.end(), empty[0]);
}

}

namespace dcx {
// -----------------------------------------------------------------------------------
// Initialize stuck objects array.  Called at start of level
void d_level_unique_stuck_object_state::init_stuck_objects()
{
	DXX_POISON_VAR(Stuck_objects, 0xcc);
	Num_stuck_objects = 0;
}
}

#if defined(DXX_BUILD_DESCENT_II)
// -----------------------------------------------------------------------------------
#define	MAX_BLAST_GLASS_DEPTH	5

namespace dsx {
namespace {

class blast_nearby_glass_context
{
	using TmapInfo_array = d_level_unique_tmap_info_state::TmapInfo_array;
	const object &obj;
	const fix damage;
	const d_eclip_array &Effects;
	const GameBitmaps_array &GameBitmaps;
	const Textures_array &Textures;
	const TmapInfo_array &TmapInfo;
	const d_vclip_array &Vclip;
	fvcvertptr &vcvertptr;
	fvcwallptr &vcwallptr;
	visited_segment_bitarray_t visited;
	unsigned can_blast(texture2_value tmap_num2) const;
public:
	blast_nearby_glass_context(const object &obj, const fix damage, const d_eclip_array &Effects, const GameBitmaps_array &GameBitmaps, const Textures_array &Textures, const TmapInfo_array &TmapInfo, const d_vclip_array &Vclip, fvcvertptr &vcvertptr, fvcwallptr &vcwallptr) :
		obj(obj), damage(damage), Effects(Effects), GameBitmaps(GameBitmaps),
		Textures(Textures), TmapInfo(TmapInfo), Vclip(Vclip),
		vcvertptr(vcvertptr), vcwallptr(vcwallptr), visited{}
	{
	}
	blast_nearby_glass_context(const blast_nearby_glass_context &) = delete;
	blast_nearby_glass_context &operator=(const blast_nearby_glass_context &) = delete;
	void process_segment(vmsegptridx_t segp, unsigned steps_remaining);
};

unsigned blast_nearby_glass_context::can_blast(const texture2_value tmap_num2) const
{
	const auto tm = get_texture_index(tmap_num2);			//tm without flags
	auto &ti = TmapInfo[tm];
	const auto ec = ti.eclip_num;
	if (ec == eclip_none)
	{
		return ti.destroyed != -1;
	}
	else
	{
		auto &e = Effects[ec];
		return e.dest_bm_num != ~0u && !(e.flags & EF_ONE_SHOT);
	}
}

void blast_nearby_glass_context::process_segment(const vmsegptridx_t segp, const unsigned steps_remaining)
{
	visited[segp] = true;

	const auto &objp = obj;
	for (const auto &&[sidenum, uside] : enumerate(segp->unique_segment::sides))
	{
		fix			dist;

		//	Process only walls which have glass.
		if (const auto tmap_num2 = uside.tmap_num2; tmap_num2 != texture2_value::None)
		{
			if (can_blast(tmap_num2))
			{
				const auto pnt{compute_center_point_on_side(vcvertptr, segp, sidenum)};
				dist = vm_vec_dist_quick(pnt, objp.pos);
				if (dist < damage/2) {
					dist = find_connected_distance(pnt, segp, objp.pos, segp.absolute_sibling(objp.segnum), MAX_BLAST_GLASS_DEPTH, wall_is_doorway_mask::rendpast);
					if ((dist > 0) && (dist < damage/2))
					{
						assert(objp.type == OBJ_WEAPON);
						check_effect_blowup(LevelSharedSegmentState.DestructibleLights, Vclip, segp, static_cast<sidenum_t>(sidenum), pnt, objp.ctype.laser_info, 1, 0);
					}
				}
			}
		}
	}

	const unsigned next_steps_remaining = steps_remaining - 1;
	if (!next_steps_remaining)
		return;
	for (const auto &&[i, segnum] : enumerate(segp->children))
	{
		if (segnum != segment_none) {
			if (!visited[segnum]) {
				if (WALL_IS_DOORWAY(GameBitmaps, Textures, vcwallptr, segp, static_cast<sidenum_t>(i)) & WALL_IS_DOORWAY_FLAG::fly)
				{
					process_segment(segp.absolute_sibling(segnum), next_steps_remaining);
				}
			}
		}
	}
}

struct d1wclip
{
	wclip *const wc;
	d1wclip(wclip &w) : wc(&w) {}
};

DEFINE_SERIAL_UDT_TO_MESSAGE(d1wclip, dwc, (dwc.wc->play_time, dwc.wc->num_frames, dwc.wc->d1_frames, dwc.wc->open_sound, dwc.wc->close_sound, dwc.wc->flags, dwc.wc->filename, serial::pad<1>()));
ASSERT_SERIAL_UDT_MESSAGE_SIZE(d1wclip, 26 + (sizeof(int16_t) * MAX_CLIP_FRAMES_D1));

}

// -----------------------------------------------------------------------------------
//	objp is going to detonate
//	blast nearby monitors, lights, maybe other things
void blast_nearby_glass(const object &objp, const fix damage)
{
	auto &LevelSharedVertexState = LevelSharedSegmentState.get_vertex_state();
	auto &Effects = LevelUniqueEffectsClipState.Effects;
	auto &TmapInfo = LevelUniqueTmapInfoState.TmapInfo;
	auto &Vertices = LevelSharedVertexState.get_vertices();
	auto &vcvertptr = Vertices.vcptr;
	auto &Walls = LevelUniqueWallSubsystemState.Walls;
	auto &vcwallptr = Walls.vcptr;
	blast_nearby_glass_context{objp, damage, Effects, GameBitmaps, Textures, TmapInfo, Vclip, vcvertptr, vcwallptr}.process_segment(vmsegptridx(objp.segnum), MAX_BLAST_GLASS_DEPTH);
}

}

#endif

DEFINE_SERIAL_UDT_TO_MESSAGE(wclip, wc, (wc.play_time, wc.num_frames, wc.frames, wc.open_sound, wc.close_sound, wc.flags, wc.filename, serial::pad<1>()));
ASSERT_SERIAL_UDT_MESSAGE_SIZE(wclip, 26 + (sizeof(int16_t) * MAX_CLIP_FRAMES));

namespace dsx {
/*
 * reads a wclip structure from a PHYSFS_File
 */
void wclip_read(const NamedPHYSFS_File fp, wclip &wc)
{
	PHYSFSX_serialize_read(fp, wc);
}

#if 0
void wclip_write(PHYSFS_File *fp, const wclip &wc)
{
	PHYSFSX_serialize_write(fp, wc);
}
#endif
}

struct wrap_v16_wall
{
	const wall *w;
	wrap_v16_wall(const wall &t) : w(&t) {}
};

#define _SERIAL_UDT_WALL_V16_MEMBERS(P)	(P type, P flags, P hps, P trigger, P clip_num, P keys)

DEFINE_SERIAL_UDT_TO_MESSAGE(v16_wall, w, _SERIAL_UDT_WALL_V16_MEMBERS(w.));
DEFINE_SERIAL_UDT_TO_MESSAGE(wrap_v16_wall, w, _SERIAL_UDT_WALL_V16_MEMBERS(w.w->));
ASSERT_SERIAL_UDT_MESSAGE_SIZE(wrap_v16_wall, 9);

/*
 * reads a v16_wall structure from a PHYSFS_File
 */
void v16_wall_read(const NamedPHYSFS_File fp, v16_wall &w)
{
	PHYSFSX_serialize_read(fp, w);
}

struct wrap_v19_wall
{
	const wall *w;
	wrap_v19_wall(const wall &t) : w(&t) {}
};

DEFINE_SERIAL_UDT_TO_MESSAGE(v19_wall, w, (w.segnum, serial::pad<2>(), w.sidenum, w.type, w.flags, w.hps, w.trigger, w.clip_num, w.keys, w.linked_wall, serial::pad<2>()));
DEFINE_SERIAL_UDT_TO_MESSAGE(wrap_v19_wall, w, (w.w->segnum, serial::pad<2>(), w.w->sidenum, serial::pad<3>(), w.w->type, w.w->flags, w.w->hps, w.w->trigger, w.w->clip_num, w.w->keys, w.w->linked_wall, serial::pad<2>()));
ASSERT_SERIAL_UDT_MESSAGE_SIZE(v19_wall, 21);
ASSERT_SERIAL_UDT_MESSAGE_SIZE(wrap_v19_wall, 21);

/*
 * reads a v19_wall structure from a PHYSFS_File
 */
void v19_wall_read(const NamedPHYSFS_File fp, v19_wall &w)
{
	PHYSFSX_serialize_read(fp, w);
}

#if defined(DXX_BUILD_DESCENT_I)
#define _SERIAL_UDT_WALL_D2X_MEMBERS	serial::pad<2>()
#elif defined(DXX_BUILD_DESCENT_II)
#define _SERIAL_UDT_WALL_D2X_MEMBERS	w.controlling_trigger, w.cloak_value
#endif
DEFINE_SERIAL_UDT_TO_MESSAGE(wall, w, (serial::sign_extend<int>(w.segnum), w.sidenum, serial::pad<3, 0>(), w.hps, w.linked_wall, serial::pad<2, 0>(), w.type, w.flags, w.state, w.trigger, w.clip_num, w.keys, _SERIAL_UDT_WALL_D2X_MEMBERS));
ASSERT_SERIAL_UDT_MESSAGE_SIZE(wall, 24);

namespace dsx {
/*
 * reads a wall structure from a PHYSFS_File
 */
void wall_read(const NamedPHYSFS_File fp, wall &w)
{
	PHYSFSX_serialize_read(fp, w);
	w.flags &= ~wall_flag::exploding;
}

}

DEFINE_SERIAL_UDT_TO_MESSAGE(active_door, d, (d.n_parts, d.front_wallnum, d.back_wallnum, d.time));
ASSERT_SERIAL_UDT_MESSAGE_SIZE(active_door, 16);

/*
 * reads an active_door structure from a PHYSFS_File
 */
void active_door_read(const NamedPHYSFS_File fp, active_door &ad)
{
	PHYSFSX_serialize_read(fp, ad);
}

void active_door_write(PHYSFS_File *fp, const active_door &ad)
{
	PHYSFSX_serialize_write(fp, ad);
}

namespace dsx {

void wall_write(PHYSFS_File *fp, const wall &w, short version)
{
	if (version <= 16)
		PHYSFSX_serialize_write<wrap_v16_wall>(fp, w);
	else if (version <= 19)
		PHYSFSX_serialize_write<wrap_v19_wall>(fp, w);
	else
		PHYSFSX_serialize_write(fp, w);
}

}

#if defined(DXX_BUILD_DESCENT_II)
DEFINE_SERIAL_UDT_TO_MESSAGE(dsx::cloaking_wall, cw, (cw.front_wallnum, cw.back_wallnum, cw.front_ls, cw.back_ls, cw.time));
ASSERT_SERIAL_UDT_MESSAGE_SIZE(dsx::cloaking_wall, 40);

namespace dsx {

void cloaking_wall_read(cloaking_wall &cw, const NamedPHYSFS_File fp)
{
	PHYSFSX_serialize_read(fp, cw);
}

void cloaking_wall_write(const cloaking_wall &cw, PHYSFS_File *fp)
{
	PHYSFSX_serialize_write(fp, cw);
}
}
#endif
