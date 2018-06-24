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
 * Lighting functions.
 *
 */

#include <algorithm>
#include <bitset>
#include <numeric>
#include <stdio.h>
#include <string.h>	// for memset()

#include "render_state.h"
#include "maths.h"
#include "vecmat.h"
#include "gr.h"
#include "inferno.h"
#include "segment.h"
#include "dxxerror.h"
#include "render.h"
#include "game.h"
#include "vclip.h"
#include "lighting.h"
#include "3d.h"
#include "interp.h"
#include "gameseg.h"
#include "laser.h"
#include "timer.h"
#include "player.h"
#include "playsave.h"
#include "weapon.h"
#include "powerup.h"
#include "fvi.h"
#include "object.h"
#include "robot.h"
#include "multi.h"
#include "palette.h"
#include "bm.h"
#include "rle.h"
#include "wall.h"

#include "compiler-range_for.h"
#include "partial_range.h"

using std::min;
using std::max;

static int Do_dynamic_light=1;
static int use_fcd_lighting;
array<g3s_lrgb, MAX_VERTICES> Dynamic_light;

#define	HEADLIGHT_CONE_DOT	(F1_0*9/10)
#define	HEADLIGHT_SCALE		(F1_0*10)

static void add_light_div(g3s_lrgb &d, const g3s_lrgb &light, const fix &scale)
{
	d.r += fixdiv(light.r, scale);
	d.g += fixdiv(light.g, scale);
	d.b += fixdiv(light.b, scale);
}

static void add_light_dot_square(g3s_lrgb &d, const g3s_lrgb &light, const fix &dot)
{
	auto square = fixmul(dot, dot);
	d.r += fixmul(square, light.r)/8;
	d.g += fixmul(square, light.g)/8;
	d.b += fixmul(square, light.b)/8;
}

// ----------------------------------------------------------------------------------------------
namespace dsx {

static void apply_light(fvmsegptridx &vmsegptridx, const g3s_lrgb obj_light_emission, const vcsegptridx_t obj_seg, const vms_vector &obj_pos, const unsigned n_render_vertices, array<unsigned, MAX_VERTICES> &render_vertices, const array<segnum_t, MAX_VERTICES> &vert_segnum_list, const icobjptridx_t objnum)
{
	if (((obj_light_emission.r+obj_light_emission.g+obj_light_emission.b)/3) > 0)
	{
		fix obji_64 = ((obj_light_emission.r+obj_light_emission.g+obj_light_emission.b)/3)*64;
		sbyte is_marker = 0;
#if defined(DXX_BUILD_DESCENT_II)
		if (objnum && objnum->type == OBJ_MARKER)
				is_marker = 1;
#endif

		// for pretty dim sources, only process vertices in object's own segment.
		//	12/04/95, MK, markers only cast light in own segment.
		if ((abs(obji_64) <= F1_0*8) || is_marker) {
			auto &vp = obj_seg->verts;

			range_for (const auto vertnum, vp)
			{
				fix			dist;
				auto &vertpos = *vcvertptr(vertnum);
				dist = vm_vec_dist_quick(obj_pos, vertpos);
				dist = fixmul(dist/4, dist/4);
				if (dist < abs(obji_64)) {
					if (dist < MIN_LIGHT_DIST)
						dist = MIN_LIGHT_DIST;

					add_light_div(Dynamic_light[vertnum], obj_light_emission, dist);
				}
			}
		} else {
			int	headlight_shift = 0;
			fix	max_headlight_dist = F1_0*200;

#if defined(DXX_BUILD_DESCENT_II)
			if (objnum)
			{
				const object &obj = *objnum;
				if (obj.type == OBJ_PLAYER)
					if (obj.ctype.player_info.powerup_flags & PLAYER_FLAGS_HEADLIGHT_ON) {
						headlight_shift = 3;
						if (get_player_id(obj) != Player_num)
						{
							fvi_query	fq;
							fvi_info		hit_data;
							int			fate;

							const auto tvec = vm_vec_scale_add(obj.pos, obj.orient.fvec, F1_0*200);

							fq.startseg				= obj_seg;
							fq.p0						= &obj.pos;
							fq.p1						= &tvec;
							fq.rad					= 0;
							fq.thisobjnum			= objnum;
							fq.ignore_obj_list.first = nullptr;
							fq.flags					= FQ_TRANSWALL;

							fate = find_vector_intersection(fq, hit_data);
							if (fate != HIT_NONE)
								max_headlight_dist = vm_vec_mag_quick(vm_vec_sub(hit_data.hit_pnt, obj.pos)) + F1_0*4;
						}
					}
			}
#endif
			for (int vv=0; vv<n_render_vertices; vv++) {
				fix			dist;
				int			apply_light = 0;

				const auto vertnum = render_vertices[vv];
				auto vsegnum = vert_segnum_list[vv];
				auto &vertpos = *vcvertptr(vertnum);

				if (use_fcd_lighting && abs(obji_64) > F1_0*32)
				{
					dist = find_connected_distance(obj_pos, obj_seg, vertpos, vmsegptridx(vsegnum), n_render_vertices, WID_RENDPAST_FLAG|WID_FLY_FLAG);
					if (dist >= 0)
						apply_light = 1;
				}
				else
				{
					dist = vm_vec_dist_quick(obj_pos, vertpos);
					apply_light = 1;
				}

				if (apply_light && ((dist >> headlight_shift) < abs(obji_64))) {

					if (dist < MIN_LIGHT_DIST)
						dist = MIN_LIGHT_DIST;

					if (headlight_shift && objnum)
					{
						fix dot;
						// MK, Optimization note: You compute distance about 15 lines up, this is partially redundant
						const auto vec_to_point = vm_vec_normalized_quick(vm_vec_sub(vertpos, obj_pos));
						dot = vm_vec_dot(vec_to_point, objnum->orient.fvec);
						if (dot < F1_0/2)
						{
							// Do the normal thing, but darken around headlight.
							add_light_div(Dynamic_light[vertnum], obj_light_emission, fixmul(HEADLIGHT_SCALE, dist));
						}
						else
						{
							if (!(Game_mode & GM_MULTI) || dist < max_headlight_dist)
							{
								add_light_dot_square(Dynamic_light[vertnum], obj_light_emission, dot);
							}
						}
					}
					else
					{
						add_light_div(Dynamic_light[vertnum], obj_light_emission, dist);
					}
				}
			}
		}
	}
}
}

#define FLASH_LEN_FIXED_SECONDS (F1_0/3)
#define FLASH_SCALE             (3*F1_0/FLASH_LEN_FIXED_SECONDS)

// ----------------------------------------------------------------------------------------------
static void cast_muzzle_flash_light(fvmsegptridx &vmsegptridx, int n_render_vertices, array<unsigned, MAX_VERTICES> &render_vertices, const array<segnum_t, MAX_VERTICES> &vert_segnum_list)
{
	fix64 current_time;
	short time_since_flash;

	current_time = timer_query();

	range_for (auto &i, Muzzle_data)
	{
		if (i.create_time)
		{
			time_since_flash = current_time - i.create_time;
			if (time_since_flash < FLASH_LEN_FIXED_SECONDS)
			{
				g3s_lrgb ml;
				ml.r = ml.g = ml.b = ((FLASH_LEN_FIXED_SECONDS - time_since_flash) * FLASH_SCALE);
				apply_light(vmsegptridx, ml, vmsegptridx(i.segnum), i.pos, n_render_vertices, render_vertices, vert_segnum_list, object_none);
			}
			else
			{
				i.create_time = 0; // turn off this muzzle flash
			}
		}
	}
}

// Translation table to make flares flicker at different rates
const array<fix, 16> Obj_light_xlate{{0x1234, 0x3321, 0x2468, 0x1735,
			    0x0123, 0x19af, 0x3f03, 0x232a,
			    0x2123, 0x39af, 0x0f03, 0x132a,
			    0x3123, 0x29af, 0x1f03, 0x032a
}};
#if defined(DXX_BUILD_DESCENT_II)
constexpr std::integral_constant<unsigned, 8> MAX_HEADLIGHTS{};
static unsigned Num_headlights;
static array<const object *, MAX_HEADLIGHTS> Headlights;
#endif

// ---------------------------------------------------------
namespace dsx {
static g3s_lrgb compute_light_emission(const vmobjptridx_t obj)
{
	int compute_color = 0;
	float cscale = 255.0;
	fix light_intensity = 0;
	g3s_lrgb lemission, obj_color = { 255, 255, 255 };
        
	switch (obj->type)
	{
		case OBJ_PLAYER:
#if defined(DXX_BUILD_DESCENT_II)
			uint8_t hoard_orbs;
			if (obj->ctype.player_info.powerup_flags & PLAYER_FLAGS_HEADLIGHT_ON)
			{
				if (Num_headlights < MAX_HEADLIGHTS)
					Headlights[Num_headlights++] = obj;
				light_intensity = HEADLIGHT_SCALE;
			}
			else if (game_mode_hoard() && (hoard_orbs = obj->ctype.player_info.hoard.orbs)) // If hoard game and player, add extra light based on how many orbs you have Pulse as well.
			{
				fix hoardlight = 1 + (i2f(hoard_orbs) / 2);
				auto s = fix_sin(static_cast<fix>(GameTime64 >> 1) & 0xFFFF); // probably a bad way to do it
				s+=F1_0; 
				s>>=1;
				hoardlight=fixmul (s,hoardlight);
				light_intensity = (hoardlight);
			}
			else
#endif
			{
				fix k = fixmuldiv(obj->mtype.phys_info.mass,obj->mtype.phys_info.drag,(f1_0-obj->mtype.phys_info.drag));
				// smooth thrust value like set_thrust_from_velocity()
				auto sthrust = vm_vec_copy_scale(obj->mtype.phys_info.velocity,k);
				light_intensity = max(static_cast<fix>(vm_vec_mag_quick(sthrust) / 4), F1_0*2) + F1_0/2;
			}
			break;
		case OBJ_FIREBALL:
			{
				const auto oid = get_fireball_id(obj);
			if (oid < Vclip.size())
			{
				auto &v = Vclip[oid];
				light_intensity = v.light_value;
				if (obj->lifeleft < F1_0*4)
					light_intensity = fixmul(fixdiv(obj->lifeleft, v.play_time), light_intensity);
			}
			else
				 light_intensity = 0;
			}
			break;
		case OBJ_ROBOT:
#if defined(DXX_BUILD_DESCENT_I)
			light_intensity = F1_0/2;	// F1_0*Robot_info[obj->id].lightcast;
#elif defined(DXX_BUILD_DESCENT_II)
			light_intensity = F1_0*Robot_info[get_robot_id(obj)].lightcast;
#endif
			break;
		case OBJ_WEAPON:
		{
			fix tval = Weapon_info[get_weapon_id(obj)].light;
			if (get_weapon_id(obj) == weapon_id_type::FLARE_ID )
				light_intensity = 2*(min(tval, obj->lifeleft) + ((static_cast<fix>(GameTime64) ^ Obj_light_xlate[obj.get_unchecked_index() % Obj_light_xlate.size()]) & 0x3fff));
			else
				light_intensity = tval;
			break;
		}
#if defined(DXX_BUILD_DESCENT_II)
		case OBJ_MARKER:
		{
			fix lightval = obj->lifeleft;

			lightval &= 0xffff;
			lightval = 8 * abs(F1_0/2 - lightval);

			light_intensity = lightval;
			break;
		}
#endif
		case OBJ_POWERUP:
			light_intensity = Powerup_info[get_powerup_id(obj)].light;
			break;
		case OBJ_DEBRIS:
			light_intensity = F1_0/4;
			break;
		case OBJ_LIGHT:
			light_intensity = obj->ctype.light_info.intensity;
			break;
		default:
			light_intensity = 0;
			break;
	}

	lemission.r = lemission.g = lemission.b = light_intensity;

	if (!PlayerCfg.DynLightColor) // colored lights not desired so use intensity only OR no intensity (== no light == no color) at all
		return lemission;

	switch (obj->type) // find out if given object should cast colored light and compute if so
	{
		default:
			break;
		case OBJ_FIREBALL:
		case OBJ_WEAPON:
#if defined(DXX_BUILD_DESCENT_II)
		case OBJ_MARKER:
#endif
			compute_color = 1;
			break;
		case OBJ_POWERUP:
		{
			switch (get_powerup_id(obj))
			{
				case POW_EXTRA_LIFE:
				case POW_ENERGY:
				case POW_SHIELD_BOOST:
				case POW_KEY_BLUE:
				case POW_KEY_RED:
				case POW_KEY_GOLD:
				case POW_CLOAK:
				case POW_INVULNERABILITY:
#if defined(DXX_BUILD_DESCENT_II)
				case POW_HOARD_ORB:
#endif
					compute_color = 1;
					break;
				default:
					break;
			}
			break;
		}
	}

	if (compute_color)
	{
		int t_idx_s = -1, t_idx_e = -1;

		if (light_intensity < F1_0) // for every effect we want color, increase light_intensity so the effect becomes barely visible
			light_intensity = F1_0;

		obj_color.r = obj_color.g = obj_color.b = 255;

		switch (obj->render_type)
		{
			case RT_NONE:
				break; // no object - no light
			case RT_POLYOBJ:
			{
				polymodel *po = &Polygon_models[obj->rtype.pobj_info.model_num];
				if (po->n_textures <= 0)
				{
					int color = g3_poly_get_color(po->model_data.get());
					if (color)
					{
						obj_color.r = gr_current_pal[color].r;
						obj_color.g = gr_current_pal[color].g;
						obj_color.b = gr_current_pal[color].b;
					}
				}
				else
				{
					t_idx_s = ObjBitmaps[ObjBitmapPtrs[po->first_texture]].index;
					t_idx_e = t_idx_s + po->n_textures - 1;
				}
				break;
			}
			case RT_LASER:
			{
				t_idx_s = t_idx_e = Weapon_info[get_weapon_id(obj)].bitmap.index;
				break;
			}
			case RT_POWERUP:
			{
				t_idx_s = Vclip[obj->rtype.vclip_info.vclip_num].frames[0].index;
				t_idx_e = Vclip[obj->rtype.vclip_info.vclip_num].frames[Vclip[obj->rtype.vclip_info.vclip_num].num_frames-1].index;
				break;
			}
			case RT_WEAPON_VCLIP:
			{
				t_idx_s = Vclip[Weapon_info[get_weapon_id(obj)].weapon_vclip].frames[0].index;
				t_idx_e = Vclip[Weapon_info[get_weapon_id(obj)].weapon_vclip].frames[Vclip[Weapon_info[get_weapon_id(obj)].weapon_vclip].num_frames-1].index;
				break;
			}
			default:
			{
				const auto &vc = Vclip[obj->id];
				t_idx_s = vc.frames[0].index;
				t_idx_e = vc.frames[vc.num_frames-1].index;
				break;
			}
		}

		if (t_idx_s != -1 && t_idx_e != -1)
		{
			obj_color.r = obj_color.g = obj_color.b = 0;
			for (int i = t_idx_s; i <= t_idx_e; i++)
			{
				grs_bitmap *bm = &GameBitmaps[i];
				bitmap_index bi;
				bi.index = i;
				PIGGY_PAGE_IN(bi);
				obj_color.r += bm->avg_color_rgb[0];
				obj_color.g += bm->avg_color_rgb[1];
				obj_color.b += bm->avg_color_rgb[2];
			}
		}

		// obviously this object did not give us any usable color. so let's do our own but with blackjack and hookers!
		if (obj_color.r <= 0 && obj_color.g <= 0 && obj_color.b <= 0)
			obj_color.r = obj_color.g = obj_color.b = 255;

		// scale color to light intensity
		cscale = (static_cast<float>(light_intensity*3)/(obj_color.r+obj_color.g+obj_color.b));
		lemission.r = obj_color.r * cscale;
		lemission.g = obj_color.g * cscale;
		lemission.b = obj_color.b * cscale;
	}

	return lemission;
}
}

// ----------------------------------------------------------------------------------------------
void set_dynamic_light(render_state_t &rstate)
{
	array<unsigned, MAX_VERTICES> render_vertices;
	array<segnum_t, MAX_VERTICES> vert_segnum_list;
	static fix light_time; 

#if defined(DXX_BUILD_DESCENT_II)
	Num_headlights = 0;
#endif

	if (!Do_dynamic_light)
		return;

	light_time += FrameTime;
	if (light_time < (F1_0/60)) // it's enough to stress the CPU 60 times per second
		return;
	light_time = light_time - (F1_0/60);

	std::bitset<MAX_VERTICES> render_vertex_flags;

	//	Create list of vertices that need to be looked at for setting of ambient light.
	uint_fast32_t n_render_vertices = 0;
	range_for (const auto segnum, partial_const_range(rstate.Render_list, rstate.N_render_segs))
	{
		if (segnum != segment_none) {
			auto &vp = Segments[segnum].verts;
			range_for (const auto vnum, vp)
			{
				if (vnum > Highest_vertex_index)
				{
					Int3();		//invalid vertex number
					continue;	//ignore it, and go on to next one
				}
				auto &&b = render_vertex_flags[vnum];
				if (!b)
				{
					b = true;
					render_vertices[n_render_vertices] = vnum;
					vert_segnum_list[n_render_vertices] = segnum;
					n_render_vertices++;
					Dynamic_light[vnum] = {};
				}
			}
		}
	}

	cast_muzzle_flash_light(vmsegptridx, n_render_vertices, render_vertices, vert_segnum_list);

	range_for (const auto &&obj, vmobjptridx)
	{
		const auto &&obj_light_emission = compute_light_emission(obj);

		if (((obj_light_emission.r+obj_light_emission.g+obj_light_emission.b)/3) > 0)
			apply_light(vmsegptridx, obj_light_emission, vmsegptridx(obj->segnum), obj->pos, n_render_vertices, render_vertices, vert_segnum_list, obj);
	}
}

// ---------------------------------------------------------

#if defined(DXX_BUILD_DESCENT_II)
namespace dsx {

void toggle_headlight_active(object &player)
{
	auto &player_info = player.ctype.player_info;
	if (player_info.powerup_flags & PLAYER_FLAGS_HEADLIGHT) {
		player_info.powerup_flags ^= PLAYER_FLAGS_HEADLIGHT_ON;
		if (Game_mode & GM_MULTI)
			multi_send_flags(player.id);
	}
}

static fix compute_headlight_light_on_object(const object_base &objp)
{
	fix	light;

	//	Let's just illuminate players and robots for speed reasons, ok?
	if (objp.type != OBJ_ROBOT && objp.type != OBJ_PLAYER)
		return 0;

	light = 0;

	range_for (const auto light_objp, partial_const_range(Headlights, Num_headlights))
	{
		fix			dot, dist;
		auto vec_to_obj = vm_vec_sub(objp.pos, light_objp->pos);
		dist = vm_vec_normalize_quick(vec_to_obj);
		if (dist > 0) {
			dot = vm_vec_dot(light_objp->orient.fvec, vec_to_obj);

			if (dot < F1_0/2)
				light += fixdiv(HEADLIGHT_SCALE, fixmul(HEADLIGHT_SCALE, dist));	//	Do the normal thing, but darken around headlight.
			else
				light += fixmul(fixmul(dot, dot), HEADLIGHT_SCALE)/8;
		}
	}
	return light;
}

}
#endif

//compute the average dynamic light in a segment.  Takes the segment number
static g3s_lrgb compute_seg_dynamic_light(const shared_segment &seg)
{
	auto op = [](g3s_lrgb r, const unsigned v) {
		r.r += Dynamic_light[v].r;
		r.g += Dynamic_light[v].g;
		r.b += Dynamic_light[v].b;
		return r;
	};
	g3s_lrgb sum = std::accumulate(begin(seg.verts), end(seg.verts), g3s_lrgb{0, 0, 0}, op);
	sum.r >>= 3;
	sum.g >>= 3;
	sum.b >>= 3;
	return sum;
}

static array<g3s_lrgb, MAX_OBJECTS> object_light;
static array<object_signature_t, MAX_OBJECTS> object_sig;
object *old_viewer;
static int reset_lighting_hack;
#define LIGHT_RATE i2f(4) //how fast the light ramps up

void start_lighting_frame(const vmobjptr_t viewer)
{
	reset_lighting_hack = (viewer != old_viewer);
	old_viewer = viewer;
}

//compute the lighting for an object.  Takes a pointer to the object,
//and possibly a rotated 3d point.  If the point isn't specified, the
//object's center point is rotated.
g3s_lrgb compute_object_light(const vcobjptridx_t obj)
{
	g3s_lrgb light;
	const vcobjidx_t objnum = obj;

	//First, get static (mono) light for this segment
	const auto &&objsegp = vcsegptr(obj->segnum);
	light.r = light.g = light.b = objsegp->static_light;

	auto &os = object_sig[objnum];
	auto &ol = object_light[objnum];
	//Now, maybe return different value to smooth transitions
	if (!reset_lighting_hack && os == obj->signature)
	{
		fix frame_delta;
		g3s_lrgb delta_light;

		delta_light.r = light.r - ol.r;
		delta_light.g = light.g - ol.g;
		delta_light.b = light.b - ol.b;

		frame_delta = fixmul(LIGHT_RATE,FrameTime);

		if (abs(((delta_light.r+delta_light.g+delta_light.b)/3)) <= frame_delta)
		{
			ol = light;		//we've hit the goal
		}
		else
		{
			if (((delta_light.r+delta_light.g+delta_light.b)/3) < 0)
				frame_delta = -frame_delta;
			ol.r += frame_delta;
			ol.g += frame_delta;
			ol.b += frame_delta;
			light = ol;
		}

	}
	else //new object, initialize 
	{
		os = obj->signature;
		ol = light;
	}

	//Finally, add in dynamic light for this segment
	const auto &&seg_dl = compute_seg_dynamic_light(objsegp);
#if defined(DXX_BUILD_DESCENT_II)
	//Next, add in (NOTE: WHITE) headlight on this object
	const fix mlight = compute_headlight_light_on_object(obj);
	light.r += mlight;
	light.g += mlight;
	light.b += mlight;
#endif
 
	light.r += seg_dl.r;
	light.g += seg_dl.g;
	light.b += seg_dl.b;

	return light;
}
