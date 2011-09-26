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
 * Lighting functions.
 *
 */

#include <stdio.h>
#include <string.h>	// for memset()

#include "fix.h"
#include "vecmat.h"
#include "gr.h"
#include "inferno.h"
#include "segment.h"
#include "error.h"
#include "render.h"
#include "game.h"
#include "vclip.h"
#include "lighting.h"
#include "3d.h"
#include "laser.h"
#include "timer.h"
#include "player.h"
#include "playsave.h"
#include "weapon.h"
#include "powerup.h"
#include "fvi.h"
#include "robot.h"
#include "multi.h"
#include "palette.h"
#include "bm.h"
#include "rle.h"
#include "wall.h"

int	Do_dynamic_light=1;
int use_fcd_lighting = 0;
static int light_frame_count = 0;
g3s_lrgb Dynamic_light[MAX_VERTICES];

#define	HEADLIGHT_CONE_DOT	(F1_0*9/10)
#define	HEADLIGHT_SCALE		(F1_0*10)

// ----------------------------------------------------------------------------------------------
void apply_light(g3s_lrgb obj_light_emission, int obj_seg, vms_vector *obj_pos, int n_render_vertices, int *render_vertices, int *vert_segnum_list, int objnum)
{
	int	vv;

	if (((obj_light_emission.r+obj_light_emission.g+obj_light_emission.b)/3) > 0)
	{
		fix obji_64 = ((obj_light_emission.r+obj_light_emission.g+obj_light_emission.b)/3)*64;

		// for pretty dim sources, only process vertices in object's own segment.
		//	12/04/95, MK, markers only cast light in own segment.
		if (abs(obji_64) <= F1_0*8) {
			int *vp = Segments[obj_seg].verts;

			for (vv=0; vv<MAX_VERTICES_PER_SEGMENT; vv++) {
				int			vertnum;
				vms_vector	*vertpos;
				fix			dist;

				vertnum = vp[vv];
				if ((vertnum ^ light_frame_count) & 1) {
					vertpos = &Vertices[vertnum];
					dist = vm_vec_dist_quick(obj_pos, vertpos);
					dist = fixmul(dist/4, dist/4);
					if (dist < abs(obji_64)) {
						if (dist < MIN_LIGHT_DIST)
							dist = MIN_LIGHT_DIST;
	
						Dynamic_light[vertnum].r += fixdiv(obj_light_emission.r, dist);
						Dynamic_light[vertnum].g += fixdiv(obj_light_emission.g, dist);
						Dynamic_light[vertnum].b += fixdiv(obj_light_emission.b, dist);
					}
				}
			}
		} else {
			int	headlight_shift = 0;
			fix	max_headlight_dist = F1_0*200;

			// -- for (vv=light_frame_count&1; vv<n_render_vertices; vv+=2) {
			for (vv=0; vv<n_render_vertices; vv++) {
				int			vertnum, vsegnum;
				vms_vector	*vertpos;
				fix			dist;
				int			apply_light = 0;

				vertnum = render_vertices[vv];
				vsegnum = vert_segnum_list[vv];
				if ((vertnum ^ light_frame_count) & 1) {
					vertpos = &Vertices[vertnum];

					if (use_fcd_lighting && abs(obji_64) > F1_0*32)
					{
						dist = find_connected_distance(obj_pos, obj_seg, vertpos, vsegnum, n_render_vertices, WID_RENDPAST_FLAG+WID_FLY_FLAG);
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

						if (headlight_shift && objnum != -1)
						{
							fix dot;
							vms_vector vec_to_point;

							vm_vec_sub(&vec_to_point, vertpos, obj_pos);
							vm_vec_normalize_quick(&vec_to_point); // MK, Optimization note: You compute distance about 15 lines up, this is partially redundant
							dot = vm_vec_dot(&vec_to_point, &Objects[objnum].orient.fvec);
							if (dot < F1_0/2)
							{
								// Do the normal thing, but darken around headlight.
								Dynamic_light[vertnum].r += fixdiv(obj_light_emission.r, fixmul(HEADLIGHT_SCALE, dist));
								Dynamic_light[vertnum].g += fixdiv(obj_light_emission.g, fixmul(HEADLIGHT_SCALE, dist));
								Dynamic_light[vertnum].b += fixdiv(obj_light_emission.b, fixmul(HEADLIGHT_SCALE, dist));
							}
							else
							{
								if (Game_mode & GM_MULTI)
								{
									if (dist < max_headlight_dist)
									{
										Dynamic_light[vertnum].r += fixmul(fixmul(dot, dot), obj_light_emission.r)/8;
										Dynamic_light[vertnum].g += fixmul(fixmul(dot, dot), obj_light_emission.g)/8;
										Dynamic_light[vertnum].b += fixmul(fixmul(dot, dot), obj_light_emission.b)/8;
									}
								}
								else
								{
									Dynamic_light[vertnum].r += fixmul(fixmul(dot, dot), obj_light_emission.r)/8;
									Dynamic_light[vertnum].g += fixmul(fixmul(dot, dot), obj_light_emission.g)/8;
									Dynamic_light[vertnum].b += fixmul(fixmul(dot, dot), obj_light_emission.b)/8;
								}
							}
						}
						else
						{
							Dynamic_light[vertnum].r += fixdiv(obj_light_emission.r, dist);
							Dynamic_light[vertnum].g += fixdiv(obj_light_emission.g, dist);
							Dynamic_light[vertnum].b += fixdiv(obj_light_emission.b, dist);
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
void cast_muzzle_flash_light(int n_render_vertices, int *render_vertices, int *vert_segnum_list)
{
	fix64 current_time;
	int i;
	short time_since_flash;

	current_time = timer_query();

	for (i=0; i<MUZZLE_QUEUE_MAX; i++)
	{
		if (Muzzle_data[i].create_time)
		{
			time_since_flash = current_time - Muzzle_data[i].create_time;
			if (time_since_flash < FLASH_LEN_FIXED_SECONDS)
			{
				g3s_lrgb ml;
				ml.r = ml.g = ml.b = ((FLASH_LEN_FIXED_SECONDS - time_since_flash) * FLASH_SCALE);
				apply_light(ml, Muzzle_data[i].segnum, &Muzzle_data[i].pos, n_render_vertices, render_vertices, vert_segnum_list, -1);
			}
			else
			{
				Muzzle_data[i].create_time = 0; // turn off this muzzle flash
			}
		}
	}
}

// Translation table to make flares flicker at different rates
fix Obj_light_xlate[16] = { 0x1234, 0x3321, 0x2468, 0x1735,
			    0x0123, 0x19af, 0x3f03, 0x232a,
			    0x2123, 0x39af, 0x0f03, 0x132a,
			    0x3123, 0x29af, 0x1f03, 0x032a };
// Flag array of objects lit last frame. Guaranteed to process this frame if lit last frame.
sbyte   Lighting_objects[MAX_OBJECTS];
#define MAX_HEADLIGHTS	8
object *Headlights[MAX_HEADLIGHTS];
int Num_headlights;

// ---------------------------------------------------------
g3s_lrgb compute_light_emission(int objnum)
{
	object *obj = &Objects[objnum];
	int compute_color = 0;
	float cscale = 255.0;
	fix light_intensity = 0;
	g3s_lrgb lemission, obj_color = { 255, 255, 255 };
        
	switch (obj->type)
	{
		case OBJ_PLAYER:
		{
			vms_vector sthrust = obj->mtype.phys_info.thrust;
			fix k = fixmuldiv(obj->mtype.phys_info.mass,obj->mtype.phys_info.drag,(f1_0-obj->mtype.phys_info.drag));
			// smooth thrust value like set_thrust_from_velocity()
			vm_vec_copy_scale(&sthrust,&obj->mtype.phys_info.velocity,k);
			light_intensity = max(vm_vec_mag_quick(&sthrust)/4, F1_0*2) + F1_0/2;
			break;
		}
		case OBJ_FIREBALL:
			if (obj->id != 0xff)
			{
				if (obj->lifeleft < F1_0*4)
					light_intensity = fixmul(fixdiv(obj->lifeleft, Vclip[obj->id].play_time), Vclip[obj->id].light_value);
				else
					light_intensity = Vclip[obj->id].light_value;
			}
			else
				 light_intensity = 0;
			break;
		case OBJ_ROBOT:
			light_intensity = F1_0/2;	// F1_0*Robot_info[obj->id].lightcast;
			break;
		case OBJ_WEAPON:
		{
			fix tval = Weapon_info[obj->id].light;

			if (obj->id == FLARE_ID )
				light_intensity = 2* (min(tval, obj->lifeleft) + ((((fix)GameTime64) ^ Obj_light_xlate[objnum&0x0f]) & 0x3fff));
			else
				light_intensity = tval;
			break;
		}
		case OBJ_POWERUP:
			light_intensity = Powerup_info[obj->id].light;
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
		case OBJ_FIREBALL:
		case OBJ_WEAPON:
		case OBJ_FLARE:
			compute_color = 1;
			break;
		case OBJ_POWERUP:
		{
			switch (obj->id)
			{
				case POW_EXTRA_LIFE:
				case POW_ENERGY:
				case POW_SHIELD_BOOST:
				case POW_KEY_BLUE:
				case POW_KEY_RED:
				case POW_KEY_GOLD:
				case POW_CLOAK:
				case POW_INVULNERABILITY:
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
		int i, t_idx_s = -1, t_idx_e = -1;

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
					int color = g3_poly_get_color(po->model_data);
					if (color)
					{
						obj_color.r = gr_current_pal[color*3];
						obj_color.g = gr_current_pal[color*3+1];
						obj_color.b = gr_current_pal[color*3+2];
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
				t_idx_s = t_idx_e = Weapon_info[obj->id].bitmap.index;
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
				t_idx_s = Vclip[Weapon_info[obj->id].weapon_vclip].frames[0].index;
				t_idx_e = Vclip[Weapon_info[obj->id].weapon_vclip].frames[Vclip[Weapon_info[obj->id].weapon_vclip].num_frames-1].index;
				break;
			}
			default:
			{
				t_idx_s = Vclip[obj->id].frames[0].index;
				t_idx_e = Vclip[obj->id].frames[Vclip[obj->id].num_frames-1].index;
				break;
			}
		}

		if (t_idx_s != -1 && t_idx_e != -1)
		{
			obj_color.r = obj_color.g = obj_color.b = 0;
			for (i = t_idx_s; i <= t_idx_e; i++)
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
		cscale = ((float)(light_intensity*3)/(obj_color.r+obj_color.g+obj_color.b));
		lemission.r = obj_color.r * cscale;
		lemission.g = obj_color.g * cscale;
		lemission.b = obj_color.b * cscale;
	}

	return lemission;
}

// ----------------------------------------------------------------------------------------------
void set_dynamic_light(void)
{
	int	vv;
	int	objnum;
	int	n_render_vertices;
	int	render_vertices[MAX_VERTICES];
	int     vert_segnum_list[MAX_VERTICES];
	sbyte   render_vertex_flags[MAX_VERTICES];
	int	render_seg,segnum, v;

	Num_headlights = 0;

	if (!Do_dynamic_light)
		return;

	light_frame_count++;
	if (light_frame_count > F1_0)
		light_frame_count = 0;

	memset(render_vertex_flags, 0, Highest_vertex_index+1);

	//	Create list of vertices that need to be looked at for setting of ambient light.
	n_render_vertices = 0;
	for (render_seg=0; render_seg<N_render_segs; render_seg++) {
		segnum = Render_list[render_seg];
		if (segnum != -1) {
			int	*vp = Segments[segnum].verts;
			for (v=0; v<MAX_VERTICES_PER_SEGMENT; v++) {
				int	vnum = vp[v];
				if (vnum<0 || vnum>Highest_vertex_index) {
					Int3();		//invalid vertex number
					continue;	//ignore it, and go on to next one
				}
				if (!render_vertex_flags[vnum]) {
					render_vertex_flags[vnum] = 1;
					render_vertices[n_render_vertices] = vnum;
					vert_segnum_list[n_render_vertices] = segnum;
					n_render_vertices++;
				}
			}
		}
	}

	for (vv=0; vv<n_render_vertices; vv++) {
		int	vertnum;

		vertnum = render_vertices[vv];
		Assert(vertnum >= 0 && vertnum <= Highest_vertex_index);
		if ((vertnum ^ light_frame_count) & 1)
			Dynamic_light[vertnum].r = Dynamic_light[vertnum].g = Dynamic_light[vertnum].b = 0;
	}

	cast_muzzle_flash_light(n_render_vertices, render_vertices, vert_segnum_list);

	for (objnum=0; objnum<=Highest_object_index; objnum++)
	{
		object		*obj = &Objects[objnum];
		vms_vector	*objpos = &obj->pos;
		g3s_lrgb	obj_light_emission;

		obj_light_emission = compute_light_emission(objnum);

		if (((obj_light_emission.r+obj_light_emission.g+obj_light_emission.b)/3) > 0)
			apply_light(obj_light_emission, obj->segnum, objpos, n_render_vertices, render_vertices, vert_segnum_list, objnum);
	}
}

// ---------------------------------------------------------

#define HEADLIGHT_BOOST_SCALE 8		//how much to scale light when have headlight boost

fix	Beam_brightness = (F1_0/2);	//global saying how bright the light beam is

#define MAX_DIST_LOG	6							//log(MAX_DIST-expressed-as-integer)
#define MAX_DIST		(f1_0<<MAX_DIST_LOG)	//no light beyond this dist

fix compute_headlight_light_on_object(object *objp)
{
	int	i;
	fix	light;

	//	Let's just illuminate players and robots for speed reasons, ok?
	if ((objp->type != OBJ_ROBOT) && (objp->type	!= OBJ_PLAYER))
		return 0;

	light = 0;

	for (i=0; i<Num_headlights; i++) {
		fix			dot, dist;
		vms_vector	vec_to_obj;
		object		*light_objp;

		light_objp = Headlights[i];

		vm_vec_sub(&vec_to_obj, &objp->pos, &light_objp->pos);
		dist = vm_vec_normalize_quick(&vec_to_obj);
		if (dist > 0) {
			dot = vm_vec_dot(&light_objp->orient.fvec, &vec_to_obj);

			if (dot < F1_0/2)
				light += fixdiv(HEADLIGHT_SCALE, fixmul(HEADLIGHT_SCALE, dist));	//	Do the normal thing, but darken around headlight.
			else
				light += fixmul(fixmul(dot, dot), HEADLIGHT_SCALE)/8;
		}
	}

	return light;
}

//compute the average dynamic light in a segment.  Takes the segment number
g3s_lrgb compute_seg_dynamic_light(int segnum)
{
	g3s_lrgb sum, seg_lrgb;
	segment *seg;
	int *verts;

	seg = &Segments[segnum];

	verts = seg->verts;
	sum.r = sum.g = sum.b = 0;

	sum.r += Dynamic_light[*verts].r;
	sum.g += Dynamic_light[*verts].g;
	sum.b += Dynamic_light[*verts++].b;
	sum.r += Dynamic_light[*verts].r;
	sum.g += Dynamic_light[*verts].g;
	sum.b += Dynamic_light[*verts++].b;
	sum.r += Dynamic_light[*verts].r;
	sum.g += Dynamic_light[*verts].g;
	sum.b += Dynamic_light[*verts++].b;
	sum.r += Dynamic_light[*verts].r;
	sum.g += Dynamic_light[*verts].g;
	sum.b += Dynamic_light[*verts++].b;
	sum.r += Dynamic_light[*verts].r;
	sum.g += Dynamic_light[*verts].g;
	sum.b += Dynamic_light[*verts++].b;
	sum.r += Dynamic_light[*verts].r;
	sum.g += Dynamic_light[*verts].g;
	sum.b += Dynamic_light[*verts++].b;
	sum.r += Dynamic_light[*verts].r;
	sum.g += Dynamic_light[*verts].g;
	sum.b += Dynamic_light[*verts++].b;
	sum.r += Dynamic_light[*verts].r;
	sum.g += Dynamic_light[*verts].g;
	sum.b += Dynamic_light[*verts].b;
	
	seg_lrgb.r = sum.r >> 3;
	seg_lrgb.g = sum.g >> 3;
	seg_lrgb.b = sum.b >> 3;

	return seg_lrgb;
}

g3s_lrgb object_light[MAX_OBJECTS];
int object_sig[MAX_OBJECTS];
object *old_viewer;
int reset_lighting_hack;
#define LIGHT_RATE i2f(4) //how fast the light ramps up

void start_lighting_frame(object *viewer)
{
	reset_lighting_hack = (viewer != old_viewer);
	old_viewer = viewer;
}

//compute the lighting for an object.  Takes a pointer to the object,
//and possibly a rotated 3d point.  If the point isn't specified, the
//object's center point is rotated.
g3s_lrgb compute_object_light(object *obj,vms_vector *rotated_pnt)
{
	g3s_lrgb light, seg_dl;
	fix mlight;
	g3s_point objpnt;
	int objnum = obj-Objects;

	if (!rotated_pnt)
	{
		g3_rotate_point(&objpnt,&obj->pos);
		rotated_pnt = &objpnt.p3_vec;
	}

	//First, get static (mono) light for this segment
	light.r = light.g = light.b = Segments[obj->segnum].static_light;

	//Now, maybe return different value to smooth transitions
	if (!reset_lighting_hack && object_sig[objnum] == obj->signature)
	{
		fix frame_delta;
		g3s_lrgb delta_light;

		delta_light.r = light.r - object_light[objnum].r;
		delta_light.g = light.g - object_light[objnum].g;
		delta_light.b = light.b - object_light[objnum].b;

		frame_delta = fixmul(LIGHT_RATE,FrameTime);

		if (abs(((delta_light.r+delta_light.g+delta_light.b)/3)) <= frame_delta)
		{
			object_light[objnum] = light;		//we've hit the goal
		}
		else
		{
			if (((delta_light.r+delta_light.g+delta_light.b)/3) < 0)
			{
				light.r = object_light[objnum].r -= frame_delta;
				light.g = object_light[objnum].g -= frame_delta;
				light.b = object_light[objnum].b -= frame_delta;
			}
			else
			{
				light.r = object_light[objnum].r += frame_delta;
				light.g = object_light[objnum].g += frame_delta;
				light.b = object_light[objnum].b += frame_delta;
			}
		}

	}
	else //new object, initialize 
	{
		object_sig[objnum] = obj->signature;
		object_light[objnum].r = light.r;
		object_light[objnum].g = light.g;
		object_light[objnum].b = light.b;
	}

	//Next, add in (NOTE: WHITE) headlight on this object
	mlight = compute_headlight_light_on_object(obj);
	light.r += mlight;
	light.g += mlight;
	light.b += mlight;
 
	//Finally, add in dynamic light for this segment
	seg_dl = compute_seg_dynamic_light(obj->segnum);
	light.r += seg_dl.r;
	light.g += seg_dl.g;
	light.b += seg_dl.b;

	return light;
}
