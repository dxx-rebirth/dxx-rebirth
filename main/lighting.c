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


#ifdef RCS
static char rcsid[] = "$Id: lighting.c,v 1.2 2001-01-22 13:22:39 bradleyb Exp $";
#endif

#include <conf.h>
#include <stdio.h>
#include <string.h>	// for memset()

#include "fix.h"
#include "vecmat.h"
#include "gr.h"
#include "inferno.h"
#include "segment.h"
#include "error.h"
#include "mono.h"
#include "render.h"
#include "game.h"
#include "vclip.h"
#include "lighting.h"
#include "3d.h"
#include "laser.h"
#include "timer.h"
#include "player.h"
#include "weapon.h"
#include "powerup.h"
#include "fvi.h"
#include "robot.h"
#include "multi.h"

int	Do_dynamic_light=1;
//int	Use_fvi_lighting = 0;

fix	Dynamic_light[MAX_VERTICES];

#define	LIGHTING_CACHE_SIZE	4096	//	Must be power of 2!
#define	LIGHTING_FRAME_DELTA	256	//	Recompute cache value every 8 frames.
#define	LIGHTING_CACHE_SHIFT	8

int	Lighting_frame_delta = 1;

int	Lighting_cache[LIGHTING_CACHE_SIZE];

int Cache_hits=0, Cache_lookups=1;

//	Return true if we think vertex vertnum is visible from segment segnum.
//	If some amount of time has gone by, then recompute, else use cached value.
int lighting_cache_visible(int vertnum, int segnum, int objnum, vms_vector *obj_pos, int obj_seg, vms_vector *vertpos)
{
	int	cache_val, cache_frame, cache_vis;

	cache_val = Lighting_cache[((segnum << LIGHTING_CACHE_SHIFT) ^ vertnum) & (LIGHTING_CACHE_SIZE-1)];

	cache_frame = cache_val >> 1;
	cache_vis = cache_val & 1;

//mprintf((0, "%i %i %5i %i ", vertnum, segnum, cache_frame, cache_vis));

Cache_lookups++;
	if ((cache_frame == 0) || (cache_frame + Lighting_frame_delta <= FrameCount)) {
		int			apply_light=0;
		fvi_query	fq;
		fvi_info		hit_data;
		int			segnum, hit_type;

		segnum = -1;

		#ifndef NDEBUG
		segnum = find_point_seg(obj_pos, obj_seg);
		if (segnum == -1) {
			Int3();		//	Obj_pos is not in obj_seg!
			return 0;		//	Done processing this object.
		}
		#endif

		fq.p0						= obj_pos;
		fq.startseg				= obj_seg;
		fq.p1						= vertpos;
		fq.rad					= 0;
		fq.thisobjnum			= objnum;
		fq.ignore_obj_list	= NULL;
		fq.flags					= FQ_TRANSWALL;

		hit_type = find_vector_intersection(&fq, &hit_data);

		// Hit_pos = Hit_data.hit_pnt;
		// Hit_seg = Hit_data.hit_seg;

		if (hit_type == HIT_OBJECT)
			Int3();	//	Hey, we're not supposed to be checking objects!

		if (hit_type == HIT_NONE)
			apply_light = 1;
		else if (hit_type == HIT_WALL) {
			fix	dist_dist;
			dist_dist = vm_vec_dist_quick(&hit_data.hit_pnt, obj_pos);
			if (dist_dist < F1_0/4) {
				apply_light = 1;
				// -- Int3();	//	Curious, did fvi detect intersection with wall containing vertex?
			}
		}
		Lighting_cache[((segnum << LIGHTING_CACHE_SHIFT) ^ vertnum) & (LIGHTING_CACHE_SIZE-1)] = apply_light + (FrameCount << 1);
//mprintf((0, "%i\n", apply_light));
		return apply_light;
	} else {
//mprintf((0, "\n"));
Cache_hits++;
		return cache_vis;
	}	
}

#define	HEADLIGHT_CONE_DOT	(F1_0*9/10)
#define	HEADLIGHT_SCALE		(F1_0*10)

// ----------------------------------------------------------------------------------------------
void apply_light(fix obj_intensity, int obj_seg, vms_vector *obj_pos, int n_render_vertices, short *render_vertices, int objnum)
{
	int	vv;

	if (obj_intensity) {
		fix	obji_64 = obj_intensity*64;

		// for pretty dim sources, only process vertices in object's own segment.
		//	12/04/95, MK, markers only cast light in own segment.
		if ((abs(obji_64) <= F1_0*8) || (Objects[objnum].type == OBJ_MARKER)) {
			short *vp = Segments[obj_seg].verts;

			for (vv=0; vv<MAX_VERTICES_PER_SEGMENT; vv++) {
				int			vertnum;
				vms_vector	*vertpos;
				fix			dist;

				vertnum = vp[vv];
				if ((vertnum ^ FrameCount) & 1) {
					vertpos = &Vertices[vertnum];
					dist = vm_vec_dist_quick(obj_pos, vertpos);
					dist = fixmul(dist/4, dist/4);
					if (dist < abs(obji_64)) {
						if (dist < MIN_LIGHT_DIST)
							dist = MIN_LIGHT_DIST;
	
						Dynamic_light[vertnum] += fixdiv(obj_intensity, dist);
					}
				}
			}
		} else {
			int	headlight_shift = 0;
			fix	max_headlight_dist = F1_0*200;

			if (Objects[objnum].type == OBJ_PLAYER)
				if (Players[Objects[objnum].id].flags & PLAYER_FLAGS_HEADLIGHT_ON) {
					headlight_shift = 3;
					if (Objects[objnum].id != Player_num) {
						vms_vector	tvec;
						fvi_query	fq;
						fvi_info		hit_data;
						int			fate;

						vm_vec_scale_add(&tvec, &Objects[objnum].pos, &Objects[objnum].orient.fvec, F1_0*200);

						fq.startseg				= Objects[objnum].segnum;
						fq.p0						= &Objects[objnum].pos;
						fq.p1						= &tvec;
						fq.rad					= 0;
						fq.thisobjnum			= objnum;
						fq.ignore_obj_list	= NULL;
						fq.flags					= FQ_TRANSWALL;

						fate = find_vector_intersection(&fq, &hit_data);
						if (fate != HIT_NONE)
							max_headlight_dist = vm_vec_mag_quick(vm_vec_sub(&tvec, &hit_data.hit_pnt, &Objects[objnum].pos)) + F1_0*4;
					}
				}
			// -- for (vv=FrameCount&1; vv<n_render_vertices; vv+=2) {
			for (vv=0; vv<n_render_vertices; vv++) {
				int			vertnum;
				vms_vector	*vertpos;
				fix			dist;
				int			apply_light;

				vertnum = render_vertices[vv];
				if ((vertnum ^ FrameCount) & 1) {
					vertpos = &Vertices[vertnum];
					dist = vm_vec_dist_quick(obj_pos, vertpos);
					apply_light = 0;

					if ((dist >> headlight_shift) < abs(obji_64)) {

						if (dist < MIN_LIGHT_DIST)
							dist = MIN_LIGHT_DIST;

						//if (Use_fvi_lighting) {
						//	if (lighting_cache_visible(vertnum, obj_seg, objnum, obj_pos, obj_seg, vertpos)) {
						//		apply_light = 1;
						//	}
						//} else
							apply_light = 1;

						if (apply_light) {
							if (headlight_shift) {
								fix			dot;
								vms_vector	vec_to_point;

								vm_vec_sub(&vec_to_point, vertpos, obj_pos);
								vm_vec_normalize_quick(&vec_to_point);		//	MK, Optimization note: You compute distance about 15 lines up, this is partially redundant
								dot = vm_vec_dot(&vec_to_point, &Objects[objnum].orient.fvec);
								if (dot < F1_0/2)
									Dynamic_light[vertnum] += fixdiv(obj_intensity, fixmul(HEADLIGHT_SCALE, dist));	//	Do the normal thing, but darken around headlight.
								else {
									if (Game_mode & GM_MULTI) {
										if (dist < max_headlight_dist)
											Dynamic_light[vertnum] += fixmul(fixmul(dot, dot), obj_intensity)/8;
									} else
										Dynamic_light[vertnum] += fixmul(fixmul(dot, dot), obj_intensity)/8;
								}
							} else
								Dynamic_light[vertnum] += fixdiv(obj_intensity, dist);
						}
					}
				}
			}
		}
	}
}

#define	FLASH_LEN_FIXED_SECONDS	(F1_0/3)
#define	FLASH_SCALE					(3*F1_0/FLASH_LEN_FIXED_SECONDS)

// ----------------------------------------------------------------------------------------------
void cast_muzzle_flash_light(int n_render_vertices, short *render_vertices)
{
	fix current_time;
	int	i;
	short	time_since_flash;

	current_time = timer_get_fixed_seconds();

	for (i=0; i<MUZZLE_QUEUE_MAX; i++) {
		if (Muzzle_data[i].create_time) {
			time_since_flash = current_time - Muzzle_data[i].create_time;
			if (time_since_flash < FLASH_LEN_FIXED_SECONDS)
				apply_light((FLASH_LEN_FIXED_SECONDS - time_since_flash) * FLASH_SCALE, Muzzle_data[i].segnum, &Muzzle_data[i].pos, n_render_vertices, render_vertices, -1);
			else
				Muzzle_data[i].create_time = 0;		// turn off this muzzle flash
		}
	}
}

//	Translation table to make flares flicker at different rates
fix	Obj_light_xlate[16] =
	{0x1234, 0x3321, 0x2468, 0x1735,
	 0x0123, 0x19af, 0x3f03, 0x232a,
	 0x2123, 0x39af, 0x0f03, 0x132a,
	 0x3123, 0x29af, 0x1f03, 0x032a};

//	Flag array of objects lit last frame.  Guaranteed to process this frame if lit last frame.
byte	Lighting_objects[MAX_OBJECTS];

#define	MAX_HEADLIGHTS	8
object	*Headlights[MAX_HEADLIGHTS];
int		Num_headlights;

// ---------------------------------------------------------
fix compute_light_intensity(int objnum)
{
	object		*obj = &Objects[objnum];
	int			objtype = obj->type;
   fix hoardlight,s;
         
	switch (objtype) {
		case OBJ_PLAYER:
			 if (Players[obj->id].flags & PLAYER_FLAGS_HEADLIGHT_ON) {
				if (Num_headlights < MAX_HEADLIGHTS)
					Headlights[Num_headlights++] = obj;
			 	return HEADLIGHT_SCALE;
			 } else if ((Game_mode & GM_HOARD) && Players[obj->id].secondary_ammo[PROXIMITY_INDEX]) {
			
		   // If hoard game and player, add extra light based on how many orbs you have
			// Pulse as well.

		  	   hoardlight=i2f(Players[obj->id].secondary_ammo[PROXIMITY_INDEX])/2; //i2f(12));
				hoardlight++;
		      fix_sincos ((GameTime/2) & 0xFFFF,&s,NULL); // probably a bad way to do it
			   s+=F1_0;  
				s>>=1;
			   hoardlight=fixmul (s,hoardlight);
		 //     mprintf ((0,"Hoardlight is %f!\n",f2fl(hoardlight)));
		      return (hoardlight);
			  }
			else
				return max(vm_vec_mag_quick(&obj->mtype.phys_info.thrust)/4, F1_0*2) + F1_0/2;
			break;
		case OBJ_FIREBALL:
			if (obj->id != 0xff) {
				if (obj->lifeleft < F1_0*4)
					return fixmul(fixdiv(obj->lifeleft, Vclip[obj->id].play_time), Vclip[obj->id].light_value);
				else
					return Vclip[obj->id].light_value;
			} else
				 return 0;
			break;
		case OBJ_ROBOT:
			return F1_0*Robot_info[obj->id].lightcast;
			break;
		case OBJ_WEAPON: {
			fix tval = Weapon_info[obj->id].light;
			if (Game_mode & GM_MULTI)
				if (obj->id == OMEGA_ID)
					if (d_rand() > 8192)
						return 0;		//	3/4 of time, omega blobs will cast 0 light!

			if (obj->id == FLARE_ID )
				return 2* (min(tval, obj->lifeleft) + ((GameTime ^ Obj_light_xlate[objnum&0x0f]) & 0x3fff));
			else
				return tval;
		}

		case OBJ_MARKER: {
			fix	lightval = obj->lifeleft;

			lightval &= 0xffff;

			lightval = 8 * abs(F1_0/2 - lightval);

			if (obj->lifeleft < F1_0*1000)
				obj->lifeleft += F1_0;	//	Make sure this object doesn't go out.

			return lightval;
		}

		case OBJ_POWERUP:
			return Powerup_info[obj->id].light;
			break;
		case OBJ_DEBRIS:
			return F1_0/4;
			break;
		case OBJ_LIGHT:
			return obj->ctype.light_info.intensity;
			break;
		default:
			return 0;
			break;
	}
}

// ----------------------------------------------------------------------------------------------
void set_dynamic_light(void)
{
	int	vv;
	int	objnum;
	int	n_render_vertices;
	short	render_vertices[MAX_VERTICES];
	byte	render_vertex_flags[MAX_VERTICES];
	int	render_seg,segnum, v;
	byte	new_lighting_objects[MAX_OBJECTS];

	Num_headlights = 0;

	if (!Do_dynamic_light)
		return;

//if (Use_fvi_lighting)
//	mprintf((0, "hits = %8i, misses = %8i, lookups = %8i, hit ratio = %7.4f\n", Cache_hits, Cache_lookups - Cache_hits, Cache_lookups, (float) Cache_hits / Cache_lookups));

	memset(render_vertex_flags, 0, Highest_vertex_index+1);

	//	Create list of vertices that need to be looked at for setting of ambient light.
	n_render_vertices = 0;
	for (render_seg=0; render_seg<N_render_segs; render_seg++) {
		segnum = Render_list[render_seg];
		if (segnum != -1) {
			short	*vp = Segments[segnum].verts;
			for (v=0; v<MAX_VERTICES_PER_SEGMENT; v++) {
				int	vnum = vp[v];
				if (vnum<0 || vnum>Highest_vertex_index) {
					Int3();		//invalid vertex number
					continue;	//ignore it, and go on to next one
				}
				if (!render_vertex_flags[vnum]) {
					render_vertex_flags[vnum] = 1;
					render_vertices[n_render_vertices++] = vnum;
				}
				//--old way-- for (s=0; s<n_render_vertices; s++)
				//--old way-- 	if (render_vertices[s] == vnum)
				//--old way-- 		break;
				//--old way-- if (s == n_render_vertices)
				//--old way-- 	render_vertices[n_render_vertices++] = vnum;
			}
		}
	}

	// -- for (vertnum=FrameCount&1; vertnum<n_render_vertices; vertnum+=2) {
	for (vv=0; vv<n_render_vertices; vv++) {
		int	vertnum;

		vertnum = render_vertices[vv];
		Assert(vertnum >= 0 && vertnum <= Highest_vertex_index);
		if ((vertnum ^ FrameCount) & 1)
			Dynamic_light[vertnum] = 0;
	}

	cast_muzzle_flash_light(n_render_vertices, render_vertices);

	for (objnum=0; objnum<=Highest_object_index; objnum++)
		new_lighting_objects[objnum] = 0;

	//	July 5, 1995: New faster dynamic lighting code.  About 5% faster on the PC (un-optimized).
	//	Only objects which are in rendered segments cast dynamic light.  We might wad6 to extend this
	//	one or two segments if we notice light changing as objects go offscreen.  I couldn't see any
	//	serious visual degradation.  In fact, I could see no humorous degradation, either. --MK
	for (render_seg=0; render_seg<N_render_segs; render_seg++) {
		int	segnum = Render_list[render_seg];

		objnum = Segments[segnum].objects;

		while (objnum != -1) {
			object		*obj = &Objects[objnum];
			vms_vector	*objpos = &obj->pos;
			fix			obj_intensity;

			obj_intensity = compute_light_intensity(objnum);

			if (obj_intensity) {
				apply_light(obj_intensity, obj->segnum, objpos, n_render_vertices, render_vertices, obj-Objects);
				new_lighting_objects[objnum] = 1;
			}

			objnum = obj->next;
		}
	}

	//	Now, process all lights from last frame which haven't been processed this frame.
	for (objnum=0; objnum<=Highest_object_index; objnum++) {
		//	In multiplayer games, process even unprocessed objects every 4th frame, else don't know about player sneaking up.
		if ((Lighting_objects[objnum]) || ((Game_mode & GM_MULTI) && (((objnum ^ FrameCount) & 3) == 0))) {
			if (!new_lighting_objects[objnum]) {
				//	Lighted last frame, but not this frame.  Get intensity...
				object		*obj = &Objects[objnum];
				vms_vector	*objpos = &obj->pos;
				fix			obj_intensity;

				obj_intensity = compute_light_intensity(objnum);

				if (obj_intensity) {
					apply_light(obj_intensity, obj->segnum, objpos, n_render_vertices, render_vertices, objnum);
					Lighting_objects[objnum] = 1;
				} else
					Lighting_objects[objnum] = 0;
			}
		} else {
			//	Not lighted last frame, so we don't need to light it.  (Already lit if casting light this frame.)
			//	But copy value from new_lighting_objects to update Lighting_objects array.
			Lighting_objects[objnum] = new_lighting_objects[objnum];
		}
	}
}

// ---------------------------------------------------------

void toggle_headlight_active()
{
	if (Players[Player_num].flags & PLAYER_FLAGS_HEADLIGHT) {
		Players[Player_num].flags ^= PLAYER_FLAGS_HEADLIGHT_ON;			
#ifdef NETWORK
		if (Game_mode & GM_MULTI)
			multi_send_flags(Player_num);		
#endif
	}
}

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


// -- Unused -- //Compute the lighting from the headlight for a given vertex on a face.
// -- Unused -- //Takes:
// -- Unused -- //  point - the 3d coords of the point
// -- Unused -- //  face_light - a scale factor derived from the surface normal of the face
// -- Unused -- //If no surface normal effect is wanted, pass F1_0 for face_light
// -- Unused -- fix compute_headlight_light(vms_vector *point,fix face_light)
// -- Unused -- {
// -- Unused -- 	fix light;
// -- Unused -- 	int use_beam = 0;		//flag for beam effect
// -- Unused -- 
// -- Unused -- 	light = Beam_brightness;
// -- Unused -- 
// -- Unused -- 	if ((Players[Player_num].flags & PLAYER_FLAGS_HEADLIGHT) && (Players[Player_num].flags & PLAYER_FLAGS_HEADLIGHT_ON) && Viewer==&Objects[Players[Player_num].objnum] && Players[Player_num].energy > 0) {
// -- Unused -- 		light *= HEADLIGHT_BOOST_SCALE;
// -- Unused -- 		use_beam = 1;	//give us beam effect
// -- Unused -- 	}
// -- Unused -- 
// -- Unused -- 	if (light) {				//if no beam, don't bother with the rest of this
// -- Unused -- 		fix point_dist;
// -- Unused -- 
// -- Unused -- 		point_dist = vm_vec_mag_quick(point);
// -- Unused -- 
// -- Unused -- 		if (point_dist >= MAX_DIST)
// -- Unused -- 
// -- Unused -- 			light = 0;
// -- Unused -- 
// -- Unused -- 		else {
// -- Unused -- 			fix dist_scale,face_scale;
// -- Unused -- 
// -- Unused -- 			dist_scale = (MAX_DIST - point_dist) >> MAX_DIST_LOG;
// -- Unused -- 			light = fixmul(light,dist_scale);
// -- Unused -- 
// -- Unused -- 			if (face_light < 0)
// -- Unused -- 				face_light = 0;
// -- Unused -- 
// -- Unused -- 			face_scale = f1_0/4 + face_light/2;
// -- Unused -- 			light = fixmul(light,face_scale);
// -- Unused -- 
// -- Unused -- 			if (use_beam) {
// -- Unused -- 				fix beam_scale;
// -- Unused -- 
// -- Unused -- 				if (face_light > f1_0*3/4 && point->z > i2f(12)) {
// -- Unused -- 					beam_scale = fixdiv(point->z,point_dist);
// -- Unused -- 					beam_scale = fixmul(beam_scale,beam_scale);	//square it
// -- Unused -- 					light = fixmul(light,beam_scale);
// -- Unused -- 				}
// -- Unused -- 			}
// -- Unused -- 		}
// -- Unused -- 	}
// -- Unused -- 
// -- Unused -- 	return light;
// -- Unused -- }

//compute the average dynamic light in a segment.  Takes the segment number
fix compute_seg_dynamic_light(int segnum)
{
	fix sum;
	segment *seg;
	short *verts;

	seg = &Segments[segnum];

	verts = seg->verts;
	sum = 0;

	sum += Dynamic_light[*verts++];
	sum += Dynamic_light[*verts++];
	sum += Dynamic_light[*verts++];
	sum += Dynamic_light[*verts++];
	sum += Dynamic_light[*verts++];
	sum += Dynamic_light[*verts++];
	sum += Dynamic_light[*verts++];
	sum += Dynamic_light[*verts];

	return sum >> 3;

}

fix object_light[MAX_OBJECTS];
int object_sig[MAX_OBJECTS];
object *old_viewer;
int reset_lighting_hack;

#define LIGHT_RATE i2f(4)		//how fast the light ramps up

void start_lighting_frame(object *viewer)
{
	reset_lighting_hack = (viewer != old_viewer);

	old_viewer = viewer;
}

//compute the lighting for an object.  Takes a pointer to the object,
//and possibly a rotated 3d point.  If the point isn't specified, the
//object's center point is rotated.
fix compute_object_light(object *obj,vms_vector *rotated_pnt)
{
	fix light;
	g3s_point objpnt;
	int objnum = obj-Objects;

	if (!rotated_pnt) {
		g3_rotate_point(&objpnt,&obj->pos);
		rotated_pnt = &objpnt.p3_vec;
	}

	//First, get static light for this segment

	light = Segment2s[obj->segnum].static_light;

	//return light;


	//Now, maybe return different value to smooth transitions

	if (!reset_lighting_hack && object_sig[objnum] == obj->signature) {
		fix delta_light,frame_delta;

		delta_light = light - object_light[objnum];

		frame_delta = fixmul(LIGHT_RATE,FrameTime);

		if (abs(delta_light) <= frame_delta)

			object_light[objnum] = light;		//we've hit the goal

		else

			if (delta_light < 0)
				light = object_light[objnum] -= frame_delta;
			else
				light = object_light[objnum] += frame_delta;

	}
	else {		//new object, initialize

		object_sig[objnum] = obj->signature;
		object_light[objnum] = light;
	}



	//Next, add in headlight on this object

	// -- Matt code: light += compute_headlight_light(rotated_pnt,f1_0);
	light += compute_headlight_light_on_object(obj);
  
	//Finally, add in dynamic light for this segment

	light += compute_seg_dynamic_light(obj->segnum);


	return light;
}


