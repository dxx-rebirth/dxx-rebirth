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
#include "player.h"
#include "text.h"
#include "fireball.h"
#include "textures.h"
#include "newdemo.h"
#include "multi.h"
#include "gameseq.h"
#include "physfs-serial.h"
#include "gameseg.h"
#include "hudmsg.h"
#include "laser.h"		//	For seeing if a flare is stuck in a wall.
#include "effects.h"

#include "compiler-range_for.h"
#include "partial_range.h"
#include "segiter.h"

//	Special door on boss level which is locked if not in multiplayer...sorry for this awful solution --MK.
#define	BOSS_LOCKED_DOOR_LEVEL	7
#define	BOSS_LOCKED_DOOR_SEG		595
#define	BOSS_LOCKED_DOOR_SIDE	5

namespace dcx {
unsigned Num_wall_anims;
}

namespace dsx {
array<wclip, MAX_WALL_ANIMS> WallAnims;		// Wall animations

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
	array<uvl, 4> &uvls;
	cwframe(wall &wr) :
		w(wr),
		uvls(vmsegptr(w.segnum)->sides[w.sidenum].uvls)
	{
	}
};

struct cwresult
{
	bool remove;
	bool record;
	cwresult() = default;
	explicit cwresult(bool r) :
		remove(false), record(r)
	{
	}
};

struct cw_removal_predicate
{
	unsigned num_cloaking_walls = 0;
	bool operator()(cloaking_wall &d);
};

struct find_cloaked_wall_predicate
{
	const vmwallidx_t w;
	find_cloaked_wall_predicate(const vmwallidx_t i) :
		w(i)
	{
	}
	bool operator()(const cloaking_wall &cw) const
	{
		return cw.front_wallnum == w || cw.back_wallnum == w;
	}
};

}

}
#endif

static std::pair<uint_fast32_t, uint_fast32_t> get_transparency_check_values(const unique_side &side)
{
	if (const uint_fast32_t masked_tmap_num2 = side.tmap_num2 & 0x3FFF)
		return {masked_tmap_num2, BM_FLAG_SUPER_TRANSPARENT};
	return {side.tmap_num, BM_FLAG_TRANSPARENT};
}

// This function determines whether the current segment/side is transparent
//		1 = YES
//		0 = NO
static uint_fast32_t check_transparency(const GameBitmaps_array &GameBitmaps, const Textures_array &Textures, const unique_side &side)
{
	const auto &&v = get_transparency_check_values(side);
	return GameBitmaps[Textures[v.first].index].get_flag_mask(v.second);
}

//-----------------------------------------------------------------
// This function checks whether we can fly through the given side.
//	In other words, whether or not we have a 'doorway'
//	 Flags:
//		WID_FLY_FLAG				1
//		WID_RENDER_FLAG			2
//		WID_RENDPAST_FLAG			4
//	 Return values:
//		WID_WALL						2	// 0/1/0		wall	
//		WID_TRANSPARENT_WALL		6	//	0/1/1		transparent wall
//		WID_ILLUSORY_WALL			3	//	1/1/0		illusory wall
//		WID_TRANSILLUSORY_WALL	7	//	1/1/1		transparent illusory wall
//		WID_NO_WALL					5	//	1/0/1		no wall, can fly through
namespace dsx {

WALL_IS_DOORWAY_result_t wall_is_doorway(const GameBitmaps_array &GameBitmaps, const Textures_array &Textures, fvcwallptr &vcwallptr, const shared_side &sside, const unique_side &uside)
{
	auto &w = *vcwallptr(sside.wall_num);
	const auto type = w.type;
	if (type == WALL_OPEN)
		return WID_NO_WALL;

#if defined(DXX_BUILD_DESCENT_II)
	if (unlikely(type == WALL_CLOAKED))
		return WID_CLOAKED_WALL;
#endif

	const auto flags = w.flags;
	if (type == WALL_ILLUSION) {
		if (flags & WALL_ILLUSION_OFF)
			return WID_NO_WALL;
		else {
			if (check_transparency(GameBitmaps, Textures, uside))
				return WID_TRANSILLUSORY_WALL;
		 	else
				return WID_ILLUSORY_WALL;
		}
	}

	if (type == WALL_BLASTABLE) {
	 	if (flags & WALL_BLASTED)
			return WID_TRANSILLUSORY_WALL;
	}	
	else
	{
	if (unlikely(flags & WALL_DOOR_OPENED))
		return WID_TRANSILLUSORY_WALL;
	if (likely(type == WALL_DOOR) && unlikely(w.state == WALL_DOOR_OPENING))
		return WID_TRANSPARENT_WALL;
	}
// If none of the above flags are set, there is no doorway.
	if (check_transparency(GameBitmaps, Textures, uside))
		return WID_TRANSPARENT_WALL;
	else
		return WID_WALL; // There are children behind the door.
}
}

#if DXX_USE_EDITOR
//-----------------------------------------------------------------
// Initializes all the walls (in other words, no special walls)
namespace dsx {
void wall_init()
{
	init_exploding_walls();
	Walls.set_count(0);
	range_for (auto &w, Walls)
	{
		w.segnum = segment_none;
		w.sidenum = -1;
		w.type = WALL_NORMAL;
		w.flags = 0;
		w.hps = 0;
		w.trigger = -1;
		w.clip_num = -1;
		w.linked_wall = -1;
	}
	ActiveDoors.set_count(0);
#if defined(DXX_BUILD_DESCENT_II)
	CloakingWalls.set_count(0);
#endif

}
}
#endif

//set the tmap_num or tmap_num2 field for a wall/door
void wall_set_tmap_num(const wclip &anim, const vmsegptridx_t seg, const unsigned side, const vmsegptridx_t csegp, const unsigned cside, const unsigned frame_num)
{
	const auto newdemo_state = Newdemo_state;
	if (newdemo_state == ND_STATE_PLAYBACK)
		return;

	const auto tmap = anim.frames[frame_num];
	if (anim.flags & WCF_TMAP1)	{
		if (tmap != seg->sides[side].tmap_num || tmap != csegp->sides[cside].tmap_num)
		{
			seg->sides[side].tmap_num = csegp->sides[cside].tmap_num = tmap;
			if (newdemo_state == ND_STATE_RECORDING)
				newdemo_record_wall_set_tmap_num1(seg,side,csegp,cside,tmap);
		}
	} else	{
		Assert(tmap!=0 && seg->sides[side].tmap_num2!=0);
		if (tmap != seg->sides[side].tmap_num2 || tmap != csegp->sides[cside].tmap_num2)
		{
			seg->sides[side].tmap_num2 = csegp->sides[cside].tmap_num2 = tmap;
			if (newdemo_state == ND_STATE_RECORDING)
				newdemo_record_wall_set_tmap_num2(seg,side,csegp,cside,tmap);
		}
	}
}


// -------------------------------------------------------------------------------
//when the wall has used all its hitpoints, this will destroy it
static void blast_blastable_wall(const vmsegptridx_t seg, int side)
{
	auto &w0 = *vmwallptr(seg->sides[side].wall_num);
	w0.hps = -1;	//say it's blasted

	const auto &&csegp = seg.absolute_sibling(seg->children[side]);
	auto Connectside = find_connect_side(seg, csegp);
	Assert(Connectside != side_none);
	const auto cwall_num = csegp->sides[Connectside].wall_num;
	const auto &&w1 = imwallptr(cwall_num);
	if (w1)
		LevelUniqueStuckObjectState.kill_stuck_objects(vmobjptr, cwall_num);
	LevelUniqueStuckObjectState.kill_stuck_objects(vmobjptr, seg->sides[side].wall_num);
	flush_fcd_cache();

	const auto a = w0.clip_num;
	//if this is an exploding wall, explode it
	if (WallAnims[a].flags & WCF_EXPLODES)
		explode_wall(vcvertptr, seg, side, w0);
	else {
		//if not exploding, set final frame, and make door passable
		const auto n = WallAnims[a].num_frames;
		w0.flags |= WALL_BLASTED;
		if (w1)
			w1->flags |= WALL_BLASTED;
		wall_set_tmap_num(WallAnims[a], seg, side, csegp, Connectside, n - 1);
	}

}


//-----------------------------------------------------------------
// Destroys a blastable wall.
void wall_destroy(const vmsegptridx_t seg, int side)
{
	auto &w = *vmwallptr(seg->sides[side].wall_num);
	if (w.type == WALL_BLASTABLE)
		blast_blastable_wall( seg, side );
	else
		Error("Hey bub, you are trying to destroy an indestructable wall.");
}

//-----------------------------------------------------------------
// Deteriorate appearance of wall. (Changes bitmap (paste-ons))
void wall_damage(const vmsegptridx_t seg, int side, fix damage)
{
	int i;

	if (seg->sides[side].wall_num == wall_none) {
		return;
	}

	auto &w0 = *vmwallptr(seg->sides[side].wall_num);
	if (w0.type != WALL_BLASTABLE)
		return;
	
	if (!(w0.flags & WALL_BLASTED) && w0.hps >= 0)
		{
		const auto &&csegp = seg.absolute_sibling(seg->children[side]);
		auto Connectside = find_connect_side(seg, csegp);
		Assert(Connectside != side_none);
		const auto cwall_num = csegp->sides[Connectside].wall_num;
		if (const auto &&w1p = imwallptr(cwall_num))
		{
			auto &w1 = *w1p;
			w1.hps -= damage;
		}
		w0.hps -= damage;

		const auto a = w0.clip_num;
		const auto n = WallAnims[a].num_frames;
		
		if (w0.hps < WALL_HPS*1/n) {
			blast_blastable_wall( seg, side );			
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
void wall_open_door(const vmsegptridx_t seg, int side)
{
	active_door *d;

	const auto wall_num = seg->sides[side].wall_num;
	wall *const w = vmwallptr(wall_num);
	LevelUniqueStuckObjectState.kill_stuck_objects(vmobjptr, seg->sides[side].wall_num);

	if ((w->state == WALL_DOOR_OPENING) ||		//already opening
		 (w->state == WALL_DOOR_WAITING))		//open, waiting to close
		return;
#if defined(DXX_BUILD_DESCENT_II)
	if (w->state == WALL_DOOR_OPEN)			//open, & staying open
		return;
#endif

	if (w->state == WALL_DOOR_CLOSING) {		//closing, so reuse door
		const auto &&r = make_range(vmactdoorptr);
		const auto &&i = std::find_if(r.begin(), r.end(), find_active_door_predicate(wall_num));
		if (i == r.end())	// likely in demo playback or multiplayer
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
		Assert(w->state == WALL_DOOR_CLOSED);
		const auto i = ActiveDoors.get_count();
		ActiveDoors.set_count(i + 1);
		d = vmactdoorptr(static_cast<actdoornum_t>(i));
		d->time = 0;
	}


	w->state = WALL_DOOR_OPENING;

	// So that door can't be shot while opening
	const auto &&csegp = vcsegptr(seg->children[side]);
	auto Connectside = find_connect_side(seg, csegp);
	if (Connectside != side_none)
	{
		const auto cwall_num = csegp->sides[Connectside].wall_num;
		if (const auto &&w1 = imwallptr(cwall_num))
		{
			w1->state = WALL_DOOR_OPENING;
			d->back_wallnum[0] = cwall_num;
		}
		d->front_wallnum[0] = seg->sides[side].wall_num;
	}
	else
		con_printf(CON_URGENT, "Illegal Connectside %i in wall_open_door. Trying to hop over. Please check your level!", side);

	if (Newdemo_state == ND_STATE_RECORDING) {
		newdemo_record_door_opening(seg, side);
	}

	if (w->linked_wall != wall_none)
	{
		wall *const w2 = vmwallptr(w->linked_wall);

		Assert(w2->linked_wall == seg->sides[side].wall_num);
		//Assert(!(w2->flags & WALL_DOOR_OPENING  ||  w2->flags & WALL_DOOR_OPENED));

		w2->state = WALL_DOOR_OPENING;

		const auto &&seg2 = vcsegptridx(w2->segnum);
		Connectside = find_connect_side(seg2, vcsegptr(seg2->children[w2->sidenum]));
		Assert(Connectside != side_none);
		const auto cwall_num = csegp->sides[Connectside].wall_num;
		if (const auto &&w3 = imwallptr(cwall_num))
			w3->state = WALL_DOOR_OPENING;

		d->n_parts = 2;
		d->front_wallnum[1] = w->linked_wall;
		d->back_wallnum[1] = cwall_num;
	}
	else
		d->n_parts = 1;


	if ( Newdemo_state != ND_STATE_PLAYBACK )
	{
		// NOTE THE LINK TO ABOVE!!!!
		const auto &&cp = compute_center_point_on_side(vcvertptr, seg, side);
		if (WallAnims[w->clip_num].open_sound > -1 )
			digi_link_sound_to_pos( WallAnims[w->clip_num].open_sound, seg, side, cp, 0, F1_0 );

	}
}
}

#if defined(DXX_BUILD_DESCENT_II)
//-----------------------------------------------------------------
// start the transition from closed -> open wall
void start_wall_cloak(const vmsegptridx_t seg, int side)
{
	cloaking_wall *d;

	if ( Newdemo_state==ND_STATE_PLAYBACK ) return;

	const auto &&w = vmwallptridx(seg->sides[side].wall_num);

	if (w->type == WALL_OPEN || w->state == WALL_DOOR_CLOAKING)		//already open or cloaking
		return;

	const auto &&csegp = vcsegptr(seg->children[side]);
	auto Connectside = find_connect_side(seg, csegp);
	Assert(Connectside != side_none);
	const auto cwall_num = csegp->sides[Connectside].wall_num;

	if (w->state == WALL_DOOR_DECLOAKING) {	//decloaking, so reuse door
		const auto &&r = make_range(vmclwallptr);
		const auto i = std::find_if(r.begin(), r.end(), find_cloaked_wall_predicate(w));
		if (i == r.end())
		{
			d_debugbreak();
			return;
		}
		d = *i;
		d->time = CLOAKING_WALL_TIME - d->time;
	}
	else if (w->state == WALL_DOOR_CLOSED) {	//create new door
		const clwallnum_t c = CloakingWalls.get_count();
		if (c >= CloakingWalls.size())
		{
			Int3();		//ran out of cloaking wall slots
			w->type = WALL_OPEN;
			if (const auto &&w1 = imwallptr(cwall_num))
				w1->type = WALL_OPEN;
			return;
		}
		CloakingWalls.set_count(c + 1);
		d = vmclwallptr(c);
		d->time = 0;
	}
	else {
		Int3();		//unexpected wall state
		return;
	}

	w->state = WALL_DOOR_CLOAKING;
	if (const auto &&w1 = imwallptr(cwall_num))
		w1->state = WALL_DOOR_CLOAKING;

	d->front_wallnum = seg->sides[side].wall_num;
	d->back_wallnum = cwall_num;
	Assert(w->linked_wall == wall_none);

	if ( Newdemo_state != ND_STATE_PLAYBACK ) {
		const auto &&cp = compute_center_point_on_side(vcvertptr, seg, side);
		digi_link_sound_to_pos( SOUND_WALL_CLOAK_ON, seg, side, cp, 0, F1_0 );
	}

	auto &df = d->front_ls;
	auto &db = d->back_ls;
	auto &s0_uvls = seg->sides[side].uvls;
	auto &s1_uvls = csegp->sides[Connectside].uvls;
	for (unsigned i = 0; i < 4; ++i)
	{
		df[i] = s0_uvls[i].l;
		db[i] = s1_uvls[i].l;
	}
}

//-----------------------------------------------------------------
// start the transition from open -> closed wall
void start_wall_decloak(const vmsegptridx_t seg, int side)
{
	cloaking_wall *d;

	if ( Newdemo_state==ND_STATE_PLAYBACK ) return;

	Assert(seg->sides[side].wall_num != wall_none); 	//Opening door on illegal wall

	const auto &&w = vmwallptridx(seg->sides[side].wall_num);

	if (w->type == WALL_CLOSED || w->state == WALL_DOOR_DECLOAKING)		//already closed or decloaking
		return;

	if (w->state == WALL_DOOR_CLOAKING) {	//cloaking, so reuse door
		const auto &&r = make_range(vmclwallptr);
		const auto i = std::find_if(r.begin(), r.end(), find_cloaked_wall_predicate(w));
		if (i == r.end())
		{
			d_debugbreak();
			return;
		}
		d = *i;
		d->time = CLOAKING_WALL_TIME - d->time;
	}
	else if (w->state == WALL_DOOR_CLOSED) {	//create new door
		const clwallnum_t c = CloakingWalls.get_count();
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
		d = vmclwallptr(c);
		d->time = 0;
	}
	else {
		Int3();		//unexpected wall state
		return;
	}

	w->state = WALL_DOOR_DECLOAKING;

	// So that door can't be shot while opening
	const auto &&csegp = vcsegptr(seg->children[side]);
	auto Connectside = find_connect_side(seg, csegp);
	Assert(Connectside != side_none);
	const auto cwall_num = csegp->sides[Connectside].wall_num;
	if (const auto &&w1 = imwallptr(cwall_num))
		w1->state = WALL_DOOR_DECLOAKING;

	d->front_wallnum = seg->sides[side].wall_num;
	d->back_wallnum = csegp->sides[Connectside].wall_num;
	Assert(w->linked_wall == wall_none);

	if ( Newdemo_state != ND_STATE_PLAYBACK ) {
		const auto &&cp = compute_center_point_on_side(vcvertptr, seg, side);
		digi_link_sound_to_pos( SOUND_WALL_CLOAK_OFF, seg, side, cp, 0, F1_0 );
	}

	auto &df = d->front_ls;
	auto &db = d->back_ls;
	auto &s0_uvls = seg->sides[side].uvls;
	auto &s1_uvls = csegp->sides[Connectside].uvls;
	for (unsigned i = 0; i < 4; ++i)
	{
		df[i] = s0_uvls[i].l;
		db[i] = s1_uvls[i].l;
	}
}
#endif

//-----------------------------------------------------------------
// This function closes the specified door and restores the closed
//  door texture.  This is called when the animation is done
void wall_close_door_ref(active_door &d)
{
	range_for (const auto p, partial_const_range(d.front_wallnum, d.n_parts))
	{
		int side;

		wall *const w = vmwallptr(p);

		const auto &&seg = vmsegptridx(w->segnum);
		side = w->sidenum;
		w->state = WALL_DOOR_CLOSED;

		Assert(seg->sides[side].wall_num != wall_none);		//Closing door on illegal wall

		const auto &&csegp = seg.absolute_sibling(seg->children[side]);
		auto Connectside = find_connect_side(seg, csegp);
		Assert(Connectside != side_none);
		const auto cwall_num = csegp->sides[Connectside].wall_num;
		if (const auto &&w1 = imwallptr(cwall_num))
			w1->state = WALL_DOOR_CLOSED;

		wall_set_tmap_num(WallAnims[w->clip_num], seg, side, csegp, Connectside, 0);
	}
}

static unsigned check_poke(fvcvertptr &vcvertptr, const object_base &obj, const shared_segment &seg, const unsigned side)
{
	//note: don't let objects with zero size block door
	if (!obj.size)
		return 0;
	return get_seg_masks(vcvertptr, obj.pos, seg, obj.size).sidemask & (1 << side);		//pokes through side!
}

namespace dsx {
static unsigned is_door_side_obstructed(fvcobjptridx &vcobjptridx, fvcsegptr &vcsegptr, const vcsegptr_t seg, const unsigned side)
{
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
static unsigned is_door_obstructed(fvcobjptridx &vcobjptridx, fvcsegptr &vcsegptr, const vcsegptridx_t seg, const unsigned side)
{
	if (const auto obstructed = is_door_side_obstructed(vcobjptridx, vcsegptr, seg, side))
		return obstructed;
	const auto &&csegp = vcsegptr(seg->children[side]);
	const auto &&Connectside = find_connect_side(seg, csegp);
	Assert(Connectside != side_none);
	//go through each object in each of two segments, and see if
	//it pokes into the connecting seg
	return is_door_side_obstructed(vcobjptridx, vcsegptr, csegp, Connectside);
}

#if defined(DXX_BUILD_DESCENT_II)
//-----------------------------------------------------------------
// Closes a door
void wall_close_door(const vmsegptridx_t seg, int side)
{
	active_door *d;

	const auto wall_num = seg->sides[side].wall_num;
	wall *const w = vmwallptr(wall_num);
	if ((w->state == WALL_DOOR_CLOSING) ||		//already closing
		 (w->state == WALL_DOOR_WAITING)	||		//open, waiting to close
		 (w->state == WALL_DOOR_CLOSED))			//closed
		return;

	if (is_door_obstructed(vcobjptridx, vcsegptr, seg, side))
		return;

	if (w->state == WALL_DOOR_OPENING) {	//reuse door
		const auto &&r = make_range(vmactdoorptr);
		const auto &&i = std::find_if(r.begin(), r.end(), find_active_door_predicate(wall_num));
		if (i == r.end())
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
		Assert(w->state == WALL_DOOR_OPEN);
		const auto i = ActiveDoors.get_count();
		ActiveDoors.set_count(i + 1);
		d = vmactdoorptr(static_cast<actdoornum_t>(i));
		d->time = 0;
	}

	w->state = WALL_DOOR_CLOSING;

	// So that door can't be shot while opening
	const auto &&csegp = vcsegptr(seg->children[side]);
	const auto &&Connectside = find_connect_side(seg, csegp);
	Assert(Connectside != side_none);
	const auto cwall_num = csegp->sides[Connectside].wall_num;
	if (const auto &&w1 = imwallptr(cwall_num))
		w1->state = WALL_DOOR_CLOSING;

	d->front_wallnum[0] = seg->sides[side].wall_num;
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
		const auto &&cp = compute_center_point_on_side(vcvertptr, seg, side);
		if (WallAnims[w->clip_num].open_sound > -1 )
			digi_link_sound_to_pos( WallAnims[w->clip_num].open_sound, seg, side, cp, 0, F1_0 );

	}
}
#endif

//-----------------------------------------------------------------
// Animates opening of a door.
// Called in the game loop.
static bool do_door_open(active_door &d)
{
	bool remove = false;
	for (unsigned p = 0; p < d.n_parts; ++p)
	{
		int side;
		fix time_elapsed, time_total, one_frame;
		int i;

		LevelUniqueStuckObjectState.kill_stuck_objects(vmobjptr, d.front_wallnum[p]);
		LevelUniqueStuckObjectState.kill_stuck_objects(vmobjptr, d.back_wallnum[p]);
		wall *const w = vmwallptr(d.front_wallnum[p]);

		const auto &&seg = vmsegptridx(w->segnum);
		side = w->sidenum;

// 		Assert(seg->sides[side].wall_num != -1);		//Trying to do_door_open on illegal wall
		if (seg->sides[side].wall_num == wall_none)
		{
			con_printf(CON_URGENT, "Trying to do_door_open on illegal wall %i. Please check your level!",side);
			continue;
		}

		const auto &&csegp = seg.absolute_sibling(seg->children[side]);
		const auto &&Connectside = find_connect_side(seg, csegp);
		Assert(Connectside != side_none);

		d.time += FrameTime;

		time_elapsed = d.time;
		const auto n = WallAnims[w->clip_num].num_frames;
		time_total = WallAnims[w->clip_num].play_time;

		one_frame = time_total/n;	

		i = time_elapsed/one_frame;

		if (i < n)
			wall_set_tmap_num(WallAnims[w->clip_num], seg, side, csegp, Connectside, i);

		const auto cwall_num = csegp->sides[Connectside].wall_num;
		const auto &&w1 = vmwallptr(cwall_num);
		if (i> n/2) {
			w->flags |= WALL_DOOR_OPENED;
			w1->flags |= WALL_DOOR_OPENED;
		}

		if (i >= n-1) {
			wall_set_tmap_num(WallAnims[w->clip_num], seg, side, csegp, Connectside, n - 1);

			// If our door is not automatic just remove it from the list.
			if (!(w->flags & WALL_DOOR_AUTO)) {
				remove = true;
#if defined(DXX_BUILD_DESCENT_II)
				w->state = WALL_DOOR_OPEN;
				w1->state = WALL_DOOR_OPEN;
#endif
			}
			else {

				w->state = WALL_DOOR_WAITING;
				w1->state = WALL_DOOR_WAITING;
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
	auto &w0 = *vmwallptr(d.front_wallnum[0]);
	const auto &&wsegp = vmsegptridx(w0.segnum);

	//check for objects in doorway before closing
	if (w0.flags & WALL_DOOR_AUTO)
		if (is_door_obstructed(vcobjptridx, vcsegptr, wsegp, w0.sidenum))
		{
#if defined(DXX_BUILD_DESCENT_II)
			digi_kill_sound_linked_to_segment(w0.segnum, w0.sidenum, -1);
			wall_open_door(wsegp, w0.sidenum);		//re-open door
#endif
			return false;
		}

	bool played_sound = false;
	bool remove = false;
	range_for (const auto p, partial_const_range(d.front_wallnum, d.n_parts))
	{
		int side;
		fix time_elapsed, time_total, one_frame;
		int i;

		auto &wp = *vmwallptr(p);

		const auto &seg = wsegp;
		side = wp.sidenum;

		if (seg->sides[side].wall_num == wall_none) {
			return false;
		}

#if defined(DXX_BUILD_DESCENT_I)
		//if here, must be auto door
//don't assert here, because now we have triggers to close non-auto doors
		Assert(wp.flags & WALL_DOOR_AUTO);
#endif

		// Otherwise, close it.
		const auto &&csegp = seg.absolute_sibling(seg->children[side]);
		const auto &&Connectside = find_connect_side(seg, csegp);
		Assert(Connectside != side_none);


		if ( Newdemo_state != ND_STATE_PLAYBACK )
		{
			// NOTE THE LINK TO ABOVE!!
			if (!played_sound)	//only play one sound for linked doors
			{
				played_sound = true;
				if (d.time == 0)
				{		//first time
					if (WallAnims[wp.clip_num].close_sound > -1 )
					{
						digi_link_sound_to_pos(WallAnims[wp.clip_num].close_sound, seg, side, compute_center_point_on_side(vcvertptr, seg, side), 0, F1_0);
					}
				}
			}
		}

		d.time += FrameTime;

		time_elapsed = d.time;
		const auto n = WallAnims[wp.clip_num].num_frames;
		time_total = WallAnims[wp.clip_num].play_time;

		one_frame = time_total/n;	

		i = n-time_elapsed/one_frame-1;

		const auto cwall_num = csegp->sides[Connectside].wall_num;
		auto &w1 = *vmwallptr(cwall_num);
		if (i < n/2) {
			wp.flags &= ~WALL_DOOR_OPENED;
			w1.flags &= ~WALL_DOOR_OPENED;
		}

		// Animate door.
		if (i > 0) {
			wall_set_tmap_num(WallAnims[wp.clip_num], seg, side, csegp, Connectside, i);

			wp.state = WALL_DOOR_CLOSING;
			w1.state = WALL_DOOR_CLOSING;
		} else
		{
			wall_close_door_ref(d);
			remove = true;
		}
	}
	return remove;
}
}

template <typename F>
static void wall_illusion_op(const vmsegptridx_t seg, unsigned side, F op)
{
	const auto wall0 = seg->sides[side].wall_num;
	if (wall0 == wall_none)
		return;
	const auto &&csegp = vcsegptr(seg->children[side]);
	const auto &&cside = find_connect_side(seg, csegp);
	if (cside == side_none)
	{
		assert(cside != side_none);
		return;
	}
	const auto wall1 = csegp->sides[cside].wall_num;
	op(wall0);
	op(wall1);
}

//-----------------------------------------------------------------
// Turns off an illusionary wall (This will be used primarily for
//  wall switches or triggers that can turn on/off illusionary walls.)
void wall_illusion_off(const vmsegptridx_t seg, int side)
{
	const auto op = [](const wallnum_t wall_num) {
		vmwallptr(wall_num)->flags |= WALL_ILLUSION_OFF;
	};
	wall_illusion_op(seg, side, op);
}

//-----------------------------------------------------------------
// Turns on an illusionary wall (This will be used primarily for
//  wall switches or triggers that can turn on/off illusionary walls.)
void wall_illusion_on(const vmsegptridx_t seg, int side)
{
	const auto op = [](const wallnum_t wall_num) {
		vmwallptr(wall_num)->flags &= ~WALL_ILLUSION_OFF;
	};
	wall_illusion_op(seg, side, op);
}

//	-----------------------------------------------------------------------------
//	Allowed to open the normally locked special boss door if in multiplayer mode.
static int special_boss_opening_allowed(segnum_t segnum, int sidenum)
{
	if (Game_mode & GM_MULTI)
		return (Current_level_num == BOSS_LOCKED_DOOR_LEVEL) && (segnum == BOSS_LOCKED_DOOR_SEG) && (sidenum == BOSS_LOCKED_DOOR_SIDE);
	else
		return 0;
}

//-----------------------------------------------------------------
// Determines what happens when a wall is shot
//returns info about wall.  see wall.h for codes
//obj is the object that hit...either a weapon or the player himself
//playernum is the number the player who hit the wall or fired the weapon,
//or -1 if a robot fired the weapon
namespace dsx {
wall_hit_process_t wall_hit_process(const player_flags powerup_flags, const vmsegptridx_t seg, int side, fix damage, int playernum, const vmobjptr_t obj)
{
	fix	show_message;

	// If it is not a "wall" then just return.
	if ( seg->sides[side].wall_num == wall_none )
		return wall_hit_process_t::WHP_NOT_SPECIAL;

	wall *const w = vmwallptr(seg->sides[side].wall_num);

	if ( Newdemo_state == ND_STATE_RECORDING )
		newdemo_record_wall_hit_process( seg, side, damage, playernum );

	if (w->type == WALL_BLASTABLE) {
#if defined(DXX_BUILD_DESCENT_II)
		if (obj->ctype.laser_info.parent_type == OBJ_PLAYER)
#endif
			wall_damage(seg, side, damage);
		return wall_hit_process_t::WHP_BLASTABLE;
	}

	if (playernum != Player_num)	//return if was robot fire
		return wall_hit_process_t::WHP_NOT_SPECIAL;

	Assert( playernum > -1 );
	
	//	Determine whether player is moving forward.  If not, don't say negative
	//	messages because he probably didn't intentionally hit the door.
	if (obj->type == OBJ_PLAYER)
		show_message = (vm_vec_dot(obj->orient.fvec, obj->mtype.phys_info.velocity) > 0);
#if defined(DXX_BUILD_DESCENT_II)
	else if (obj->type == OBJ_ROBOT)
		show_message = 0;
	else if ((obj->type == OBJ_WEAPON) && (obj->ctype.laser_info.parent_type == OBJ_ROBOT))
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
		(w->keys == KEY_BLUE && (key_color = TXT_BLUE, true)) ||
		(w->keys == KEY_GOLD && (key_color = TXT_YELLOW, true)) ||
		(w->keys == KEY_RED && (key_color = TXT_RED, true))
	)
	{
		if (!(powerup_flags & static_cast<PLAYER_FLAG>(w->keys)))
		{
			static_assert(KEY_BLUE == static_cast<unsigned>(PLAYER_FLAGS_BLUE_KEY), "BLUE key flag mismatch");
			static_assert(KEY_GOLD == static_cast<unsigned>(PLAYER_FLAGS_GOLD_KEY), "GOLD key flag mismatch");
			static_assert(KEY_RED == static_cast<unsigned>(PLAYER_FLAGS_RED_KEY), "RED key flag mismatch");
				if (show_message)
					HUD_init_message(HM_DEFAULT, "%s %s",key_color,TXT_ACCESS_DENIED);
			return wall_hit_process_t::WHP_NO_KEY;
		}
	}

	if (w->type == WALL_DOOR)
	{
		if ((w->flags & WALL_DOOR_LOCKED ) && !(special_boss_opening_allowed(seg, side)) ) {
				if (show_message)
					HUD_init_message_literal(HM_DEFAULT, TXT_CANT_OPEN_DOOR);
			return wall_hit_process_t::WHP_NO_KEY;
		}
		else {
			if (w->state != WALL_DOOR_OPENING)
			{
				wall_open_door(seg, side);
				if (Game_mode & GM_MULTI)
				{
					int flags;
#if defined(DXX_BUILD_DESCENT_I)
					flags = 0;
#elif defined(DXX_BUILD_DESCENT_II)
					flags = w->flags;
#endif
					multi_send_door_open(seg, side,flags);
				}
			}
			return wall_hit_process_t::WHP_DOOR;
			
		}
	}
	return wall_hit_process_t::WHP_NOT_SPECIAL;		//default is treat like normal wall
}
}

//-----------------------------------------------------------------
// Opens doors/destroys wall/shuts off triggers.
void wall_toggle(const vmsegptridx_t segp, unsigned side)
{
	if (side >= MAX_SIDES_PER_SEGMENT)
	{
#ifndef NDEBUG
		Warning("Can't toggle side %u of segment %d (%u)!\n", side, static_cast<segnum_t>(segp), Highest_segment_index);
#endif
		return;
	}
	const auto wall_num = segp->sides[side].wall_num;
	if (wall_num == wall_none)
	{
		LevelError("Ignoring attempt to toggle wall in segment %hu, side %u: no wall exists there.", segp.get_unchecked_index(), side);
		return;
	}

	if ( Newdemo_state == ND_STATE_RECORDING )
		newdemo_record_wall_toggle(segp, side);

	wall *const w = vmwallptr(wall_num);
	if (w->type == WALL_BLASTABLE)
		wall_destroy(segp, side);

	if (w->type == WALL_DOOR && w->state == WALL_DOOR_CLOSED)
		wall_open_door(segp, side);
}

bool ad_removal_predicate::operator()(active_door &d) const
{
	wall &w = *vmwallptr(d.front_wallnum[0]);
	if (w.state == WALL_DOOR_OPENING)
		return do_door_open(d);
	else if (w.state == WALL_DOOR_CLOSING)
		return do_door_close(d);
	else if (w.state == WALL_DOOR_WAITING) {
		d.time += FrameTime;
		// set flags to fix occasional netgame problem where door is waiting to close but open flag isn't set
		w.flags |= WALL_DOOR_OPENED;
		if (wall *const w1 = imwallptr(d.back_wallnum[0]))
			w1->flags |= WALL_DOOR_OPENED;
		if (d.time > DOOR_WAIT_TIME)
#if defined(DXX_BUILD_DESCENT_II)
			if (!is_door_obstructed(vcobjptridx, vcsegptr, vcsegptridx(w.segnum), w.sidenum))
#endif
			{
				w.state = WALL_DOOR_CLOSING;
				d.time = 0;
			}
	}
	return false;
}

#if defined(DXX_BUILD_DESCENT_II)
static void copy_cloaking_wall_light_to_wall(array<uvl, 4> &back_uvls, array<uvl, 4> &front_uvls, const cloaking_wall &d)
{
	for (uint_fast32_t i = 0; i != 4; ++i)
	{
		back_uvls[i].l = d.back_ls[i];
		front_uvls[i].l = d.front_ls[i];
	}
}

static void scale_cloaking_wall_light_to_wall(array<uvl, 4> &back_uvls, array<uvl, 4> &front_uvls, const cloaking_wall &d, const fix light_scale)
{
	for (uint_fast32_t i = 0; i != 4; ++i)
	{
		back_uvls[i].l = fixmul(d.back_ls[i], light_scale);
		front_uvls[i].l = fixmul(d.front_ls[i], light_scale);
	}
}

static cwresult do_cloaking_wall_frame(const bool initial, cloaking_wall &d, const cwframe front, const cwframe back)
{
	cwresult r(initial);
	if (d.time > CLOAKING_WALL_TIME) {
		front.w.type = back.w.type = WALL_OPEN;
		front.w.state = back.w.state = WALL_DOOR_CLOSED;		//why closed? why not?
		r.remove = true;
	}
	else if (d.time > CLOAKING_WALL_TIME/2) {
		const int8_t cloak_value = ((d.time - CLOAKING_WALL_TIME / 2) * (GR_FADE_LEVELS - 2)) / (CLOAKING_WALL_TIME / 2);
		if (front.w.cloak_value != cloak_value)
		{
			r.record = true;
			front.w.cloak_value = back.w.cloak_value = cloak_value;
		}

		if (front.w.type != WALL_CLOAKED)
		{		//just switched
			front.w.type = back.w.type = WALL_CLOAKED;
			copy_cloaking_wall_light_to_wall(back.uvls, front.uvls, d);
		}
	}
	else {		//fading out
		fix light_scale;
		light_scale = fixdiv(CLOAKING_WALL_TIME / 2 - d.time, CLOAKING_WALL_TIME / 2);
		scale_cloaking_wall_light_to_wall(back.uvls, front.uvls, d, light_scale);
	}
	return r;
}

static cwresult do_decloaking_wall_frame(const bool initial, cloaking_wall &d, const cwframe front, const cwframe back)
{
	cwresult r(initial);
	if (d.time > CLOAKING_WALL_TIME) {

		back.w.state = WALL_DOOR_CLOSED;
		front.w.state = WALL_DOOR_CLOSED;
		copy_cloaking_wall_light_to_wall(back.uvls, front.uvls, d);
		r.remove = true;
	}
	else if (d.time > CLOAKING_WALL_TIME/2) {		//fading in
		fix light_scale;
		front.w.type = back.w.type = WALL_CLOSED;

		light_scale = fixdiv(d.time - CLOAKING_WALL_TIME / 2, CLOAKING_WALL_TIME / 2);
		scale_cloaking_wall_light_to_wall(back.uvls, front.uvls, d, light_scale);
	}
	else {		//cloaking in
		const int8_t cloak_value = ((CLOAKING_WALL_TIME / 2 - d.time) * (GR_FADE_LEVELS - 2)) / (CLOAKING_WALL_TIME / 2);
		if (front.w.cloak_value != cloak_value)
		{
			front.w.cloak_value = back.w.cloak_value = cloak_value;
			r.record = true;
		}
		front.w.type = WALL_CLOAKED;
		back.w.type = WALL_CLOAKED;
	}
	return r;
}

bool cw_removal_predicate::operator()(cloaking_wall &d)
{
	const cwframe front(*vmwallptr(d.front_wallnum));
	const auto &&wpback = imwallptr(d.back_wallnum);
	const cwframe back = (wpback ? cwframe(*wpback) : front);
	const bool initial = (d.time == 0);
	d.time += FrameTime;

	cwresult r;
	if (front.w.state == WALL_DOOR_CLOAKING)
		r = do_cloaking_wall_frame(initial, d, front, back);
	else if (front.w.state == WALL_DOOR_DECLOAKING)
		r = do_decloaking_wall_frame(initial, d, front, back);
	else
	{
		d_debugbreak();	//unexpected wall state
		return false;
	}
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
#endif

namespace dsx {
void wall_frame_process()
{
	if (unsigned num_exploding_walls = Num_exploding_walls)
	{
		range_for (auto &&wp, Walls.vmptr)
		{
			auto &w1 = *wp;
			if (w1.flags & WALL_EXPLODING)
			{
				assert(num_exploding_walls);
				do_exploding_wall_frame(w1);
				if (! -- num_exploding_walls)
				{
					/* In debug builds, iterate over all remaining walls
					 * to verify that none are marked as WALL_EXPLODING.
					 */
#ifdef NDEBUG
					break;
#endif
				}
			}
		}
		assert(!num_exploding_walls);
	}

	{
		const auto &&r = partial_range(ActiveDoors, ActiveDoors.get_count());
		auto &&i = std::remove_if(r.begin(), r.end(), ad_removal_predicate());
		ActiveDoors.set_count(std::distance(r.begin(), i));
	}
#if defined(DXX_BUILD_DESCENT_II)
	if (Newdemo_state != ND_STATE_PLAYBACK)
	{
		const auto &&r = partial_range(CloakingWalls, CloakingWalls.get_count());
		cw_removal_predicate rp;
		std::remove_if(r.begin(), r.end(), std::ref(rp));
		CloakingWalls.set_count(rp.num_cloaking_walls);
	}
#endif
}

d_level_unique_stuck_object_state LevelUniqueStuckObjectState;

//	An object got stuck in a door (like a flare).
//	Add global entry.
void d_level_unique_stuck_object_state::add_stuck_object(fvcwallptr &vcwallptr, const vmobjptridx_t objp, const vcsegptr_t segp, const unsigned sidenum)
{
	const auto wallnum = segp->sides[sidenum].wall_num;
	if (wallnum != wall_none)
	{
		if (vcwallptr(wallnum)->flags & WALL_BLASTED)
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
	const auto predicate = [obj](const stuckobj &so) { return so.objnum == obj; };
	const auto i = std::find_if(pr.begin(), pr.end(), predicate);
	if (i == pr.end())
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
		assert(obj.movement_type == MT_PHYSICS);
		assert(obj.mtype.phys_info.flags & PF_STICK);
		obj.lifeleft = DXX_WEAPON_LIFELEFT;
		return true;
	};
	const auto i = std::remove_if(pr.begin(), pr.end(), predicate);
	static_assert(std::is_trivially_destructible<stuckobj>::value, "stuckobj destructor not called");
	Num_stuck_objects = std::distance(pr.begin(), i);
	array<stuckobj, 1> empty;
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

static void bng_process_segment(const object &objp, fix damage, const vmsegptridx_t segp, int depth, visited_segment_bitarray_t &visited)
{
	visited[segp] = true;
	int	i, sidenum;

	if (depth > MAX_BLAST_GLASS_DEPTH)
		return;

	depth++;

	for (sidenum=0; sidenum<MAX_SIDES_PER_SEGMENT; sidenum++) {
		int			tm;
		fix			dist;

		//	Process only walls which have glass.
		if ((tm=segp->sides[sidenum].tmap_num2) != 0) {
			tm &= 0x3fff;			//tm without flags

			const auto ec = TmapInfo[tm].eclip_num;
			if ((ec != eclip_none &&
				 (Effects[ec].dest_bm_num != ~0u && !(Effects[ec].flags & EF_ONE_SHOT))) || (ec == eclip_none && TmapInfo[tm].destroyed != -1))
			{
				const auto &&pnt = compute_center_point_on_side(vcvertptr, segp, sidenum);
				dist = vm_vec_dist_quick(pnt, objp.pos);
				if (dist < damage/2) {
					dist = find_connected_distance(pnt, segp, objp.pos, segp.absolute_sibling(objp.segnum), MAX_BLAST_GLASS_DEPTH, WID_RENDPAST_FLAG);
					if ((dist > 0) && (dist < damage/2))
						check_effect_blowup(segp, sidenum, pnt, vcobjptr(objp.ctype.laser_info.parent_num)->ctype.laser_info, 1, 0);
				}
			}
		}
	}

	for (i=0; i<MAX_SIDES_PER_SEGMENT; i++) {
		auto segnum = segp->children[i];

		if (segnum != segment_none) {
			if (!visited[segnum]) {
				if (WALL_IS_DOORWAY(GameBitmaps, Textures, vcwallptr, segp, segp, i) & WID_FLY_FLAG)
				{
					bng_process_segment(objp, damage, segp.absolute_sibling(segnum), depth, visited);
				}
			}
		}
	}
}

// -----------------------------------------------------------------------------------
//	objp is going to detonate
//	blast nearby monitors, lights, maybe other things
void blast_nearby_glass(const object &objp, fix damage)
{
	visited_segment_bitarray_t visited;
	bng_process_segment(objp, damage, vmsegptridx(objp.segnum), 0, visited);
}

struct d1wclip
{
	wclip *wc;
	d1wclip(wclip &w) : wc(&w) {}
};

DEFINE_SERIAL_UDT_TO_MESSAGE(d1wclip, dwc, (dwc.wc->play_time, dwc.wc->num_frames, dwc.wc->d1_frames, dwc.wc->open_sound, dwc.wc->close_sound, dwc.wc->flags, dwc.wc->filename, serial::pad<1>()));
ASSERT_SERIAL_UDT_MESSAGE_SIZE(d1wclip, 26 + (sizeof(int16_t) * MAX_CLIP_FRAMES_D1));
#endif

DEFINE_SERIAL_UDT_TO_MESSAGE(wclip, wc, (wc.play_time, wc.num_frames, wc.frames, wc.open_sound, wc.close_sound, wc.flags, wc.filename, serial::pad<1>()));
ASSERT_SERIAL_UDT_MESSAGE_SIZE(wclip, 26 + (sizeof(int16_t) * MAX_CLIP_FRAMES));

/*
 * reads a wclip structure from a PHYSFS_File
 */
void wclip_read(PHYSFS_File *fp, wclip &wc)
{
	PHYSFSX_serialize_read(fp, wc);
}

#if 0
void wclip_write(PHYSFS_File *fp, const wclip &wc)
{
	PHYSFSX_serialize_write(fp, wc);
}
#endif

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
void v16_wall_read(PHYSFS_File *fp, v16_wall &w)
{
	PHYSFSX_serialize_read(fp, w);
}

struct wrap_v19_wall
{
	const wall *w;
	wrap_v19_wall(const wall &t) : w(&t) {}
};

DEFINE_SERIAL_UDT_TO_MESSAGE(v19_wall, w, (w.segnum, serial::pad<2>(), w.sidenum, w.type, w.flags, w.hps, w.trigger, w.clip_num, w.keys, w.linked_wall));
DEFINE_SERIAL_UDT_TO_MESSAGE(wrap_v19_wall, w, (w.w->segnum, serial::pad<2>(), w.w->sidenum, serial::pad<3>(), w.w->type, w.w->flags, w.w->hps, w.w->trigger, w.w->clip_num, w.w->keys, w.w->linked_wall, serial::pad<2>()));
ASSERT_SERIAL_UDT_MESSAGE_SIZE(v19_wall, 21);
ASSERT_SERIAL_UDT_MESSAGE_SIZE(wrap_v19_wall, 21);

/*
 * reads a v19_wall structure from a PHYSFS_File
 */
void v19_wall_read(PHYSFS_File *fp, v19_wall &w)
{
	PHYSFSX_serialize_read(fp, w);
}

#if defined(DXX_BUILD_DESCENT_I)
#define _SERIAL_UDT_WALL_D2X_MEMBERS	serial::pad<2>()
#elif defined(DXX_BUILD_DESCENT_II)
#define _SERIAL_UDT_WALL_D2X_MEMBERS	w.controlling_trigger, w.cloak_value
#endif
DEFINE_SERIAL_UDT_TO_MESSAGE(wall, w, (serial::sign_extend<int>(w.segnum), w.sidenum, serial::pad<3, 0>(), w.hps, serial::sign_extend<int>(w.linked_wall), w.type, w.flags, w.state, w.trigger, w.clip_num, w.keys, _SERIAL_UDT_WALL_D2X_MEMBERS));
ASSERT_SERIAL_UDT_MESSAGE_SIZE(wall, 24);

/*
 * reads a wall structure from a PHYSFS_File
 */
void wall_read(PHYSFS_File *fp, wall &w)
{
	PHYSFSX_serialize_read(fp, w);
	w.flags &= ~WALL_EXPLODING;
}

DEFINE_SERIAL_UDT_TO_MESSAGE(active_door, d, (d.n_parts, d.front_wallnum, d.back_wallnum, d.time));
ASSERT_SERIAL_UDT_MESSAGE_SIZE(active_door, 16);

/*
 * reads an active_door structure from a PHYSFS_File
 */
void active_door_read(PHYSFS_File *fp, active_door &ad)
{
	PHYSFSX_serialize_read(fp, ad);
}

void active_door_write(PHYSFS_File *fp, const active_door &ad)
{
	PHYSFSX_serialize_write(fp, ad);
}

void wall_write(PHYSFS_File *fp, const wall &w, short version)
{
	if (version <= 16)
		PHYSFSX_serialize_write<wrap_v16_wall>(fp, w);
	else if (version <= 19)
		PHYSFSX_serialize_write<wrap_v19_wall>(fp, w);
	else
		PHYSFSX_serialize_write(fp, w);
}

#if defined(DXX_BUILD_DESCENT_II)
DEFINE_SERIAL_UDT_TO_MESSAGE(cloaking_wall, cw, (cw.front_wallnum, cw.back_wallnum, cw.front_ls, cw.back_ls, cw.time));
ASSERT_SERIAL_UDT_MESSAGE_SIZE(cloaking_wall, 40);

void cloaking_wall_read(cloaking_wall &cw, PHYSFS_File *fp)
{
	PHYSFSX_serialize_read(fp, cw);
}

void cloaking_wall_write(const cloaking_wall &cw, PHYSFS_File *fp)
{
	PHYSFSX_serialize_write(fp, cw);
}
#endif
