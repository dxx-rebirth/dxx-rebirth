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
 * Rendering Stuff
 *
 */

#include <algorithm>
#include <limits>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include "inferno.h"
#include "segment.h"
#include "dxxerror.h"
#include "bm.h"
#include "texmap.h"
#include "render.h"
#include "render_state.h"
#include "game.h"
#include "object.h"
#include "laser.h"
#include "textures.h"
#include "screens.h"
#include "segpoint.h"
#include "wall.h"
#include "texmerge.h"
#include "physics.h"
#include "3d.h"
#include "gameseg.h"
#include "vclip.h"
#include "lighting.h"
#include "cntrlcen.h"
#include "newdemo.h"
#include "automap.h"
#include "endlevel.h"
#include "key.h"
#include "newmenu.h"
#include "u_mem.h"
#include "piggy.h"
#include "timer.h"
#include "effects.h"
#include "playsave.h"
#ifdef OGL
#include "ogl_init.h"
#endif
#include "args.h"

#include "compiler-range_for.h"
#include "segiter.h"

#ifdef EDITOR
#include "editor/editor.h"
#include "editor/esegment.h"
#endif

using std::min;
using std::max;

// (former) "detail level" values
#ifdef OGL
int Render_depth = MAX_RENDER_SEGS; //how many segments deep to render
#else
int Render_depth = 20; //how many segments deep to render
#endif
int Max_linear_depth = 50; // Deepest segment at which linear interpolation will be used.
int Max_linear_depth_objects = 20;

//used for checking if points have been rotated
int	Clear_window_color=-1;
int	Clear_window=2;	// 1 = Clear whole background window, 2 = clear view portals into rest of world, 0 = no clear

static uint16_t s_current_generation;

// When any render function needs to know what's looking at it, it should 
// access Viewer members.
object * Viewer = NULL;

vms_vector Viewer_eye;  //valid during render

int	N_render_segs;

fix Render_zoom = 0x9000;					//the player's zoom factor

#ifndef NDEBUG
ubyte object_rendered[MAX_OBJECTS];
#endif

#ifdef EDITOR
int	Render_only_bottom=0;
int	Bottom_bitmap_num = 9;
#endif

//Global vars for window clip test
int Window_clip_left,Window_clip_top,Window_clip_right,Window_clip_bot;

#ifdef EDITOR
int _search_mode = 0;			//true if looking for curseg,side,face
short _search_x,_search_y;	//pixel we're looking at
int found_seg,found_side,found_face,found_poly;
#else
static const int _search_mode = 0;
#endif

#ifdef NDEBUG		//if no debug code, set these vars to constants

static const int Outline_mode = 0, Show_only_curside = 0;

#else

int Outline_mode=0,Show_only_curside=0;

int toggle_outline_mode(void)
{
	return Outline_mode = !Outline_mode;
}

int toggle_show_only_curside(void)
{
	return Show_only_curside = !Show_only_curside;
}
#endif

#ifndef NDEBUG
static void draw_outline(int nverts,g3s_point **pointlist)
{
	int i;

	gr_setcolor(BM_XRGB(63,63,63));

	for (i=0;i<nverts-1;i++)
		g3_draw_line(pointlist[i],pointlist[i+1]);

	g3_draw_line(pointlist[i],pointlist[0]);

}
#endif

fix flash_scale;

#define FLASH_CYCLE_RATE f1_0

static const fix Flash_rate = FLASH_CYCLE_RATE;

//cycle the flashing light for when mine destroyed
void flash_frame()
{
	static fixang flash_ang=0;

	if (!Control_center_destroyed && !Seismic_tremor_magnitude)
		return;

	if (Endlevel_sequence)
		return;

	if (PaletteBlueAdd > 10 )		//whiting out
		return;

//	flash_ang += fixmul(FLASH_CYCLE_RATE,FrameTime);
#if defined(DXX_BUILD_DESCENT_II)
	if (Seismic_tremor_magnitude) {
		fix	added_flash;

		added_flash = abs(Seismic_tremor_magnitude);
		if (added_flash < F1_0)
			added_flash *= 16;

		flash_ang += fixmul(Flash_rate, fixmul(FrameTime, added_flash+F1_0));
		fix_fastsincos(flash_ang,&flash_scale,NULL);
		flash_scale = (flash_scale + F1_0*3)/4;	//	gets in range 0.5 to 1.0
	} else
#endif
	{
		flash_ang += fixmul(Flash_rate,FrameTime);
		fix_fastsincos(flash_ang,&flash_scale,NULL);
		flash_scale = (flash_scale + f1_0)/2;
#if defined(DXX_BUILD_DESCENT_II)
		if (Difficulty_level == 0)
			flash_scale = (flash_scale+F1_0*3)/4;
#endif
	}


}

static inline int is_alphablend_eclip(int eclip_num)
{
#if defined(DXX_BUILD_DESCENT_II)
	if (eclip_num == ECLIP_NUM_FORCE_FIELD)
		return 1;
#endif
	return eclip_num == ECLIP_NUM_FUELCEN;
}

// ----------------------------------------------------------------------------
//	Render a face.
//	It would be nice to not have to pass in segnum and sidenum, but
//	they are used for our hideously hacked in headlight system.
//	vp is a pointer to vertex ids.
//	tmap1, tmap2 are texture map ids.  tmap2 is the pasty one.
static void render_face(segnum_t segnum, int sidenum, int nv, int *vp, int tmap1, int tmap2, uvl *uvlp, int wid_flags)
{
	grs_bitmap  *bm;
#ifdef OGL
	grs_bitmap  *bm2 = NULL;
#endif

	g3s_uvl			uvl_copy[8];
	g3s_lrgb		dyn_light[8];
	int			i;
	g3s_point		*pointlist[8];

	Assert(nv <= 8);

	for (i=0; i<nv; i++) {
		uvl_copy[i].u = uvlp[i].u;
		uvl_copy[i].v = uvlp[i].v;
		dyn_light[i].r = dyn_light[i].g = dyn_light[i].b = uvl_copy[i].l = uvlp[i].l;
		pointlist[i] = &Segment_points[vp[i]];
	}

#if defined(DXX_BUILD_DESCENT_II)
	//handle cloaked walls
	if (wid_flags & WID_CLOAKED_FLAG) {
		int wall_num = Segments[segnum].sides[sidenum].wall_num;
		Assert(wall_num != -1);
		gr_settransblend(Walls[wall_num].cloak_value, GR_BLEND_NORMAL);
		gr_setcolor(BM_XRGB(0, 0, 0));  // set to black (matters for s3)

		g3_draw_poly(nv, pointlist);    // draw as flat poly

		gr_settransblend(GR_FADE_OFF, GR_BLEND_NORMAL);

		return;
	}
#endif

	if (tmap1 >= NumTextures) {
#ifndef RELEASE
		Int3();
#endif
		Segments[segnum].sides[sidenum].tmap_num = 0;
	}

#ifdef OGL
	if (!GameArg.DbgUseOldTextureMerge){
		PIGGY_PAGE_IN(Textures[tmap1]);
		bm = &GameBitmaps[Textures[tmap1].index];
		if (tmap2){
			PIGGY_PAGE_IN(Textures[tmap2&0x3FFF]);
			bm2 = &GameBitmaps[Textures[tmap2&0x3FFF].index];
		}
		if (bm2 && (bm2->bm_flags&BM_FLAG_SUPER_TRANSPARENT)){
			bm = texmerge_get_cached_bitmap( tmap1, tmap2 );
			bm2 = NULL;
		}
	}else
#endif

		// New code for overlapping textures...
		if (tmap2 != 0) {
			bm = texmerge_get_cached_bitmap( tmap1, tmap2 );
		} else {
			bm = &GameBitmaps[Textures[tmap1].index];
			PIGGY_PAGE_IN(Textures[tmap1]);
		}

	Assert( !(bm->bm_flags & BM_FLAG_PAGED_OUT) );

	//set light values for each vertex & build pointlist
	for (i=0;i<nv;i++)
	{
		//the uvl struct has static light already in it

		//scale static light for destruction effect
		if (Control_center_destroyed || Seismic_tremor_magnitude)	//make lights flash
			uvl_copy[i].l = fixmul(flash_scale,uvl_copy[i].l);
		//add in dynamic light (from explosions, etc.)
		uvl_copy[i].l += (Dynamic_light[vp[i]].r+Dynamic_light[vp[i]].g+Dynamic_light[vp[i]].b)/3;
		//saturate at max value
		if (uvl_copy[i].l > MAX_LIGHT)
			uvl_copy[i].l = MAX_LIGHT;

		// And now the same for the ACTUAL (rgb) light we want to use

		//scale static light for destruction effect
		if (Seismic_tremor_magnitude)	//make lights flash
			dyn_light[i].r = dyn_light[i].g = dyn_light[i].b = fixmul(flash_scale,uvl_copy[i].l);
		else if (Control_center_destroyed)	//make lights flash
		{
			if (PlayerCfg.DynLightColor) // let the mine glow red a little
			{
				dyn_light[i].r = fixmul(flash_scale>=f0_5*1.5?flash_scale:f0_5*1.5,uvl_copy[i].l);
				dyn_light[i].g = dyn_light[i].b = fixmul(flash_scale,uvl_copy[i].l);
			}
			else
				dyn_light[i].r = dyn_light[i].g = dyn_light[i].b = fixmul(flash_scale,uvl_copy[i].l);
		}

		// add light color
		dyn_light[i].r += Dynamic_light[vp[i]].r;
		dyn_light[i].g += Dynamic_light[vp[i]].g;
		dyn_light[i].b += Dynamic_light[vp[i]].b;
		// saturate at max value
		if (dyn_light[i].r > MAX_LIGHT)
			dyn_light[i].r = MAX_LIGHT;
		if (dyn_light[i].g > MAX_LIGHT)
			dyn_light[i].g = MAX_LIGHT;
		if (dyn_light[i].b > MAX_LIGHT)
			dyn_light[i].b = MAX_LIGHT;
		if (PlayerCfg.AlphaEffects) // due to additive blending, transparent sprites will become invivible in font of white surfaces (lamps). Fix that with a little desaturation
		{
			dyn_light[i].r *= .93;
			dyn_light[i].g *= .93;
			dyn_light[i].b *= .93;
		}
	}

	if ( PlayerCfg.AlphaEffects && is_alphablend_eclip(TmapInfo[tmap1].eclip_num) ) // set nice transparency/blending for some special effects (if we do more, we should maybe use switch here)
		gr_settransblend(GR_FADE_OFF, GR_BLEND_ADDITIVE_C);

#ifdef EDITOR
	if ((Render_only_bottom) && (sidenum == WBOTTOM))
		g3_draw_tmap(nv,pointlist,uvl_copy,dyn_light,&GameBitmaps[Textures[Bottom_bitmap_num].index]);
	else
#endif

#ifdef OGL
		if (bm2){
			g3_draw_tmap_2(nv,pointlist,uvl_copy,dyn_light,bm,bm2,((tmap2&0xC000)>>14) & 3);
		}else
#endif
			g3_draw_tmap(nv,pointlist,uvl_copy,dyn_light,bm);

	gr_settransblend(GR_FADE_OFF, GR_BLEND_NORMAL); // revert any transparency/blending setting back to normal

#ifndef NDEBUG
	if (Outline_mode) draw_outline(nv, pointlist);
#endif
}

#ifdef EDITOR
// ----------------------------------------------------------------------------
//	Only called if editor active.
//	Used to determine which face was clicked on.
static void check_face(segnum_t segnum, int sidenum, int facenum, int nv, int *vp, int tmap1, int tmap2, uvl *uvlp)
{
	int	i;

	if (_search_mode) {
		int save_lighting;
#ifndef OGL
		grs_bitmap *bm;
#endif
		g3s_uvl uvl_copy[8];
		g3s_lrgb dyn_light[8];
		g3s_point *pointlist[4];

		memset(dyn_light, 0, sizeof(dyn_light));
#ifndef OGL
		if (tmap2 > 0 )
			bm = texmerge_get_cached_bitmap( tmap1, tmap2 );
		else
			bm = &GameBitmaps[Textures[tmap1].index];
#endif
		for (i=0; i<nv; i++) {
			uvl_copy[i].u = uvlp[i].u;
			uvl_copy[i].v = uvlp[i].v;
			dyn_light[i].r = dyn_light[i].g = dyn_light[i].b = uvl_copy[i].l = uvlp[i].l;
			pointlist[i] = &Segment_points[vp[i]];
		}

		gr_setcolor(0);
#ifdef OGL
		ogl_end_frame();
#endif
		gr_pixel(_search_x,_search_y);	//set our search pixel to color zero
#ifdef OGL
		ogl_start_frame();
#endif
		gr_setcolor(1);					//and render in color one
		save_lighting = Lighting_on;
		Lighting_on = 2;
#ifdef OGL
		g3_draw_poly(nv,&pointlist[0]);
#else
		g3_draw_tmap(nv,&pointlist[0], uvl_copy, dyn_light, bm);
#endif
		Lighting_on = save_lighting;

		if (gr_ugpixel(&grd_curcanv->cv_bitmap,_search_x,_search_y) == 1) {
			found_seg = segnum;
			found_side = sidenum;
			found_face = facenum;
		}
	}
}
#endif

static const fix	Tulate_min_dot = (F1_0/4);
//--unused-- fix	Tulate_min_ratio = (2*F1_0);
static const fix	Min_n0_n1_dot	= (F1_0*15/16);

// -----------------------------------------------------------------------------------
//	Render a side.
//	Check for normal facing.  If so, render faces on side dictated by sidep->type.
static void render_side(segment *segp, int sidenum)
{
	side		*sidep = &segp->sides[sidenum];
	vms_vector	tvec;
	fix		v_dot_n0, v_dot_n1;
	uvl		temp_uvls[3];
	fix		min_dot, max_dot;
	vms_vector	normals[2];
	int		wid_flags;


	wid_flags = WALL_IS_DOORWAY(segp,sidenum);

	if (!(wid_flags & WID_RENDER_FLAG))		//if (WALL_IS_DOORWAY(segp, sidenum) == WID_NO_WALL)
		return;

	normals[0] = segp->sides[sidenum].normals[0];
	normals[1] = segp->sides[sidenum].normals[1];
	side_vertnum_list_t vertnum_list;
	get_side_verts(vertnum_list,segp-Segments,sidenum);

#if defined(DXX_BUILD_DESCENT_I)
	//	Regardless of whether this side is comprised of a single quad, or two triangles, we need to know one normal, so
	//	deal with it, get the dot product.
	if (sidep->type == SIDE_IS_TRI_13) {
		vm_vec_normalized_dir(&tvec, &Viewer_eye, &Vertices[vertnum_list[1]]);
	} else {
		vm_vec_normalized_dir(&tvec, &Viewer_eye, &Vertices[vertnum_list[0]]);
	}

	v_dot_n0 = vm_vec_dot(&tvec, &normals[0]);
#endif

	//	========== Mark: Here is the change...beginning here: ==========

	if (sidep->type == SIDE_IS_QUAD) {

#if defined(DXX_BUILD_DESCENT_II)
		vm_vec_sub(&tvec, &Viewer_eye, &Vertices[vertnum_list[0]]);

		v_dot_n0 = vm_vec_dot(&tvec, &normals[0]);
#endif

		if (v_dot_n0 >= 0) {
			render_face(segp-Segments, sidenum, 4, &vertnum_list[0], sidep->tmap_num, sidep->tmap_num2, sidep->uvls, wid_flags);
			#ifdef EDITOR
			check_face(segp-Segments, sidenum, 0, 4, &vertnum_list[0], sidep->tmap_num, sidep->tmap_num2, sidep->uvls);
			#endif
		}
	} else {
#if defined(DXX_BUILD_DESCENT_II)
		//	Regardless of whether this side is comprised of a single quad, or two triangles, we need to know one normal, so
		//	deal with it, get the dot product.
		if (sidep->type == SIDE_IS_TRI_13)
			vm_vec_normalized_dir_quick(&tvec, &Viewer_eye, &Vertices[vertnum_list[1]]);
		else
			vm_vec_normalized_dir_quick(&tvec, &Viewer_eye, &Vertices[vertnum_list[0]]);

		v_dot_n0 = vm_vec_dot(&tvec, &normals[0]);
#endif

		//	========== Mark: The change ends here. ==========

		//	Although this side has been triangulated, because it is not planar, see if it is acceptable
		//	to render it as a single quadrilateral.  This is a function of how far away the viewer is, how non-planar
		//	the face is, how normal to the surfaces the view is.
		//	Now, if both dot products are close to 1.0, then render two triangles as a single quad.
		v_dot_n1 = vm_vec_dot(&tvec, &normals[1]);

		if (v_dot_n0 < v_dot_n1) {
			min_dot = v_dot_n0;
			max_dot = v_dot_n1;
		} else {
			min_dot = v_dot_n1;
			max_dot = v_dot_n0;
		}

		//	Determine whether to detriangulate side: (speed hack, assumes Tulate_min_ratio == F1_0*2, should fixmul(min_dot, Tulate_min_ratio))
		if (DETRIANGULATION && ((min_dot+F1_0/256 > max_dot) || ((Viewer->segnum != segp-Segments) &&  (min_dot > Tulate_min_dot) && (max_dot < min_dot*2)))) {
			fix	n0_dot_n1;

			//	The other detriangulation code doesn't deal well with badly non-planar sides.
			n0_dot_n1 = vm_vec_dot(&normals[0], &normals[1]);
			if (n0_dot_n1 < Min_n0_n1_dot)
				goto im_so_ashamed;

			render_face(segp-Segments, sidenum, 4, &vertnum_list[0], sidep->tmap_num, sidep->tmap_num2, sidep->uvls, wid_flags);
			#ifdef EDITOR
			check_face(segp-Segments, sidenum, 0, 4, &vertnum_list[0], sidep->tmap_num, sidep->tmap_num2, sidep->uvls);
			#endif
		} else {
im_so_ashamed: ;
			if (sidep->type == SIDE_IS_TRI_02) {
				if (v_dot_n0 >= 0) {
					render_face(segp-Segments, sidenum, 3, &vertnum_list[0], sidep->tmap_num, sidep->tmap_num2, sidep->uvls, wid_flags);
					#ifdef EDITOR
					check_face(segp-Segments, sidenum, 0, 3, &vertnum_list[0], sidep->tmap_num, sidep->tmap_num2, sidep->uvls);
					#endif
				}

				if (v_dot_n1 >= 0) {
					temp_uvls[0] = sidep->uvls[0];		temp_uvls[1] = sidep->uvls[2];		temp_uvls[2] = sidep->uvls[3];
					vertnum_list[1] = vertnum_list[2];	vertnum_list[2] = vertnum_list[3];	// want to render from vertices 0, 2, 3 on side
					render_face(segp-Segments, sidenum, 3, &vertnum_list[0], sidep->tmap_num, sidep->tmap_num2, temp_uvls, wid_flags);
					#ifdef EDITOR
					check_face(segp-Segments, sidenum, 1, 3, &vertnum_list[0], sidep->tmap_num, sidep->tmap_num2, sidep->uvls);
					#endif
				}
			} else if (sidep->type ==  SIDE_IS_TRI_13) {
				if (v_dot_n1 >= 0) {
					render_face(segp-Segments, sidenum, 3, &vertnum_list[1], sidep->tmap_num, sidep->tmap_num2, &sidep->uvls[1], wid_flags);	// rendering 1,2,3, so just skip 0
					#ifdef EDITOR
					check_face(segp-Segments, sidenum, 1, 3, &vertnum_list[1], sidep->tmap_num, sidep->tmap_num2, sidep->uvls);
					#endif
				}

				if (v_dot_n0 >= 0) {
					temp_uvls[0] = sidep->uvls[0];		temp_uvls[1] = sidep->uvls[1];		temp_uvls[2] = sidep->uvls[3];
					vertnum_list[2] = vertnum_list[3];		// want to render from vertices 0,1,3
					render_face(segp-Segments, sidenum, 3, &vertnum_list[0], sidep->tmap_num, sidep->tmap_num2, temp_uvls, wid_flags);
					#ifdef EDITOR
					check_face(segp-Segments, sidenum, 0, 3, &vertnum_list[0], sidep->tmap_num, sidep->tmap_num2, sidep->uvls);
					#endif
				}

			} else
				Error("Illegal side type in render_side, type = %i, segment # = %i, side # = %i\n", sidep->type, (int)(segp-Segments), sidenum);
		}
	}

}

#ifdef EDITOR
static void render_object_search(vobjptridx_t obj)
{
	int changed=0;

	//note that we draw each pixel object twice, since we cannot control
	//what color the object draws in, so we try color 0, then color 1,
	//in case the object itself is rendering color 0

	gr_setcolor(0);	//set our search pixel to color zero
#ifdef OGL
	ogl_end_frame();

	// For OpenGL we use gr_rect instead of gr_pixel,
	// because in some implementations (like my Macbook Pro 5,1)
	// point smoothing can't be turned off.
	// Point smoothing would change the pixel to dark grey, but it MUST be black.
	// Making a 3x3 rectangle wouldn't matter
	// (but it only seems to draw a single pixel anyway)
	gr_rect(_search_x - 1, _search_y - 1, _search_x + 1, _search_y + 1);

	ogl_start_frame();
#else
	gr_pixel(_search_x,_search_y);
#endif
	render_object(obj);
	if (gr_ugpixel(&grd_curcanv->cv_bitmap,_search_x,_search_y) != 0)
		changed=1;

	gr_setcolor(1);
#ifdef OGL
	ogl_end_frame();
	gr_rect(_search_x - 1, _search_y - 1, _search_x + 1, _search_y + 1);
	ogl_start_frame();
#else
	gr_pixel(_search_x,_search_y);
#endif
	render_object(obj);
	if (gr_ugpixel(&grd_curcanv->cv_bitmap,_search_x,_search_y) != 1)
		changed=1;

	if (changed) {
		if (obj->segnum != segment_none)
			Cursegp = &Segments[obj->segnum];
		found_seg = -(static_cast<short>(obj)+1);
	}
}
#endif

static void do_render_object(vobjptridx_t obj, int window_num)
{
	#ifdef EDITOR
	int save_3d_outline=0;
	#endif
	int count = 0;
	Assert(obj < MAX_OBJECTS);

	#ifndef NDEBUG
	if (object_rendered[obj]) {		//already rendered this...
		Int3();		//get Matt!!!
		return;
	}

	object_rendered[obj] = 1;
	#endif

#if defined(DXX_BUILD_DESCENT_II)
   if (Newdemo_state==ND_STATE_PLAYBACK)  
	 {
	  if ((DemoDoingLeft==6 || DemoDoingRight==6) && obj->type==OBJ_PLAYER)
		{
			// A nice fat hack: keeps the player ship from showing up in the
			// small extra view when guiding a missile in the big window
			
  			return; 
		}
	 }
#endif

	//	Added by MK on 09/07/94 (at about 5:28 pm, CDT, on a beautiful, sunny late summer day!) so
	//	that the guided missile system will know what objects to look at.
	//	I didn't know we had guided missiles before the release of D1. --MK
	if ((obj->type == OBJ_ROBOT) || (obj->type == OBJ_PLAYER)) {
		Window_rendered_data[window_num].rendered_robots.emplace_back(obj);
	}

	if ((count++ > MAX_OBJECTS) || (obj->next == obj)) {
		Int3();					// infinite loop detected
		obj->next = object_none;		// won't this clean things up?
		return;					// get out of this infinite loop!
	}

		//g3_draw_object(obj->class_id,&obj->pos,&obj->orient,obj->size);

	//check for editor object

	#ifdef EDITOR
	if (EditorWindow && obj==Cur_object_index) {
		save_3d_outline = g3d_interp_outline;
		g3d_interp_outline=1;
	}
	#endif

	#ifdef EDITOR
	if (_search_mode)
		render_object_search(obj);
	else
	#endif
		//NOTE LINK TO ABOVE
		render_object(obj);

	for (objnum_t n=obj->attached_obj; n != object_none; n = Objects[n].ctype.expl_info.next_attach) {

		Assert(Objects[n].type == OBJ_FIREBALL);
		Assert(Objects[n].control_type == CT_EXPLOSION);
		Assert(Objects[n].flags & OF_ATTACHED);

		render_object(&Objects[n]);
	}


	#ifdef EDITOR
	if (EditorWindow && obj==Cur_object_index)
		g3d_interp_outline = save_3d_outline;
	#endif


}

#ifndef NDEBUG
int	draw_boxes=0;
int draw_edges=0,new_seg_sorting=1,pre_draw_segs=0;
int no_migrate_segs=1,migrate_objects=1;
int check_window_check=0;
#else
static const int draw_boxes = 0;
static const int draw_edges = 0;
static const int new_seg_sorting = 1;
static const int pre_draw_segs = 0;
static const int no_migrate_segs = 1;
static const int migrate_objects = 1;
static const int check_window_check = 0;
#endif

//increment counter for checking if points rotated
//This must be called at the start of the frame if rotate_list() will be used
void render_start_frame()
{
	if (s_current_generation == std::numeric_limits<decltype(s_current_generation)>::max())
	{
		Segment_points = {};
		s_current_generation = 0;
	}
	++ s_current_generation;
}

//Given a lit of point numbers, rotate any that haven't been rotated this frame
g3s_codes rotate_list(int nv,int *pointnumlist)
{
	int i,pnum;
	g3s_point *pnt;
	g3s_codes cc;

	for (i=0;i<nv;i++) {

		pnum = pointnumlist[i];

		pnt = &Segment_points[pnum];

		if (pnt->p3_last_generation != s_current_generation)
		{
			pnt->p3_last_generation = s_current_generation;
			if (cheats.acid)
			{
				float f = (float) timer_query() / F1_0;
				vms_vector tmpv = Vertices[pnum];
				tmpv.x += fl2f(sinf(f * 2.0f + f2fl(tmpv.x)));
				tmpv.y += fl2f(sinf(f * 3.0f + f2fl(tmpv.y)));
				tmpv.z += fl2f(sinf(f * 5.0f + f2fl(tmpv.z)));
				g3_rotate_point(pnt,&tmpv);
			}
			else
				g3_rotate_point(pnt,&Vertices[pnum]);
		}

		cc.uand &= pnt->p3_codes;
		cc.uor  |= pnt->p3_codes;
	}

	return cc;

}

//Given a lit of point numbers, project any that haven't been projected
static void project_list(array<int, 8> &pointnumlist)
{
	range_for (auto pnum, pointnumlist)
	{
		if (!(Segment_points[pnum].p3_flags & PF_PROJECTED))
			g3_project_point(&Segment_points[pnum]);
	}
}


// -----------------------------------------------------------------------------------
#if !defined(NDEBUG) || !defined(OGL)
static void render_segment(segnum_t segnum, int window_num)
{
	segment		*seg = &Segments[segnum];
	int			sn;

	Assert(segnum!=segment_none && segnum<=Highest_segment_index);
	g3s_codes 	cc=rotate_list(seg->verts);
	if (! cc.uand) {		//all off screen?

#if defined(DXX_BUILD_DESCENT_II)
      if (Viewer->type!=OBJ_ROBOT)
#endif
  	   	Automap_visited[segnum]=1;

		for (sn=0; sn<MAX_SIDES_PER_SEGMENT; sn++)
			render_side(seg, sn);
	}

	//draw any objects that happen to be in this segment

	//sort objects!
	//object_sort_segment_objects( seg );
		
	#ifndef NDEBUG
	if (!migrate_objects) {
		range_for (auto objnum, objects_in(*seg))
			do_render_object(objnum, window_num);
	}
	#endif

}
#endif

// ----- This used to be called when Show_only_curside was set.
// ----- It is wholly and superiorly replaced by render_side.
// -- //render one side of one segment
// -- void render_seg_side(segment *seg,int _side)
// -- {
// -- 	g3s_codes cc;
// -- 	short vertnum_list[4];
// -- 
// -- 	cc=g3_rotate_list(8,&seg->verts);
// -- 
// -- 	if (! cc.uand) {		//all off screen?
// -- 		int fn,pn,i;
// -- 		side *s;
// -- 		face *f;
// -- 		poly *p;
// -- 
// -- 		s=&seg->sides[_side];
// -- 
// -- 		for (f=s->faces,fn=s->num_faces;fn;fn--,f++)
// -- 			for (p=f->polys,pn=f->num_polys;pn;pn--,p++) {
// -- 				grs_bitmap *tmap;
// -- 	
// -- 				for (i=0;i<p->num_vertices;i++) vertnum_list[i] = seg->verts[p->verts[i]];
// -- 	
// -- 				if (p->tmap_num >= NumTextures) {
// -- 					Warning("Invalid tmap number %d, NumTextures=%d\n...Changing in poly structure to tmap 0",p->tmap_num,NumTextures);
// -- 					p->tmap_num = 0;		//change it permanantly
// -- 				}
// -- 	
// -- 				tmap = Textures[p->tmap_num];
// -- 	
// -- 				g3_check_and_draw_tmap(p->num_vertices,vertnum_list,(g3s_uvl *) &p->uvls,tmap,&f->normal);
// -- 	
// -- 				if (Outline_mode) draw_outline(p->num_vertices,vertnum_list);
// -- 			}
// -- 		}
// -- 
// -- }

static const fix CROSS_WIDTH = i2f(8);
static const fix CROSS_HEIGHT = i2f(8);

#ifdef EDITOR
#ifndef NDEBUG

//draw outline for curside
static void outline_seg_side(segment *seg,int _side,int edge,int vert)
{
	g3s_codes cc=rotate_list(seg->verts);

	if (! cc.uand) {		//all off screen?
		g3s_point *pnt;

		//render curedge of curside of curseg in green

		gr_setcolor(BM_XRGB(0,63,0));
		g3_draw_line(&Segment_points[seg->verts[Side_to_verts[_side][edge]]],&Segment_points[seg->verts[Side_to_verts[_side][(edge+1)%4]]]);

		//draw a little cross at the current vert

		pnt = &Segment_points[seg->verts[Side_to_verts[_side][vert]]];

		g3_project_point(pnt);		//make sure projected

//		gr_setcolor(BM_XRGB(0,0,63));
//		gr_line(pnt->p3_sx-CROSS_WIDTH,pnt->p3_sy,pnt->p3_sx+CROSS_WIDTH,pnt->p3_sy);
//		gr_line(pnt->p3_sx,pnt->p3_sy-CROSS_HEIGHT,pnt->p3_sx,pnt->p3_sy+CROSS_HEIGHT);

		gr_line(pnt->p3_sx-CROSS_WIDTH,pnt->p3_sy,pnt->p3_sx,pnt->p3_sy-CROSS_HEIGHT);
		gr_line(pnt->p3_sx,pnt->p3_sy-CROSS_HEIGHT,pnt->p3_sx+CROSS_WIDTH,pnt->p3_sy);
		gr_line(pnt->p3_sx+CROSS_WIDTH,pnt->p3_sy,pnt->p3_sx,pnt->p3_sy+CROSS_HEIGHT);
		gr_line(pnt->p3_sx,pnt->p3_sy+CROSS_HEIGHT,pnt->p3_sx-CROSS_WIDTH,pnt->p3_sy);
	}
}

#endif
#endif

#if 0		//this stuff could probably just be deleted

#define DEFAULT_PERSPECTIVE_DEPTH 6

int Perspective_depth=DEFAULT_PERSPECTIVE_DEPTH;	//how many levels deep to render in perspective

int inc_perspective_depth(void)
{
	return ++Perspective_depth;

}

int dec_perspective_depth(void)
{
	return Perspective_depth==1?Perspective_depth:--Perspective_depth;

}

int reset_perspective_depth(void)
{
	return Perspective_depth = DEFAULT_PERSPECTIVE_DEPTH;

}
#endif

static ubyte code_window_point(fix x,fix y,rect *w)
{
	ubyte code=0;

	if (x <= w->left)  code |= 1;
	if (x >= w->right) code |= 2;

	if (y <= w->top) code |= 4;
	if (y >= w->bot) code |= 8;

	return code;
}

#ifndef NDEBUG
static void draw_window_box(color_t color,short left,short top,short right,short bot)
{
	short l,t,r,b;

	gr_setcolor(color);

	l=left; t=top; r=right; b=bot;

	if ( r<0 || b<0 || l>=grd_curcanv->cv_bitmap.bm_w || (t>=grd_curcanv->cv_bitmap.bm_h && b>=grd_curcanv->cv_bitmap.bm_h))
		return;

	if (l<0) l=0;
	if (t<0) t=0;
	if (r>=grd_curcanv->cv_bitmap.bm_w) r=grd_curcanv->cv_bitmap.bm_w-1;
	if (b>=grd_curcanv->cv_bitmap.bm_h) b=grd_curcanv->cv_bitmap.bm_h-1;

	gr_line(i2f(l),i2f(t),i2f(r),i2f(t));
	gr_line(i2f(r),i2f(t),i2f(r),i2f(b));
	gr_line(i2f(r),i2f(b),i2f(l),i2f(b));
	gr_line(i2f(l),i2f(b),i2f(l),i2f(t));

}
#endif

#ifndef NDEBUG
char visited2[MAX_SEGMENTS];
#endif

struct visited_twobit_array_t : visited_segment_multibit_array_t<2> {};
int	lcnt_save,scnt_save;

#define RED   BM_XRGB(63,0,0)
#define WHITE BM_XRGB(63,63,63)

//Given two sides of segment, tell the two verts which form the 
//edge between them
typedef array<int_fast8_t, 2> se_array0;
typedef array<se_array0, 6> se_array1;
typedef array<se_array1, 6> se_array2;
static const se_array2 Two_sides_to_edge = {{
	{{  {{-1,-1}},	 {{3,7}},	 {{-1,-1}},	 {{2,6}},	 {{6,7}},	 {{2,3}}	}},
	{{  {{3,7}},	 {{-1,-1}},	 {{0,4}},	 {{-1,-1}},	 {{4,7}},	 {{0,3}}	}},
	{{  {{-1,-1}},	 {{0,4}},	 {{-1,-1}},	 {{1,5}},	 {{4,5}},	 {{0,1}}	}},
	{{  {{2,6}},	 {{-1,-1}},	 {{1,5}},	 {{-1,-1}},	 {{5,6}},	 {{1,2}}	}},
	{{  {{6,7}},	 {{4,7}},	 {{4,5}},	 {{5,6}},	 {{-1,-1}},	 {{-1,-1}}	}},
	{{  {{2,3}},	 {{0,3}},	 {{0,1}},	 {{1,2}},	 {{-1,-1}},	 {{-1,-1}}	}}
}};

//given an edge specified by two verts, give the two sides on that edge
typedef array<int_fast8_t, 2> es_array0;
typedef array<es_array0, 8> es_array1;
typedef array<es_array1, 8> es_array2;
static const es_array2 Edge_to_sides = {{
	{{  {{-1,-1}},	 {{2,5}},	 {{-1,-1}},	 {{1,5}},	 {{1,2}},	 {{-1,-1}},	 {{-1,-1}},	 {{-1,-1}}	}},
	{{  {{2,5}},	 {{-1,-1}},	 {{3,5}},	 {{-1,-1}},	 {{-1,-1}},	 {{2,3}},	 {{-1,-1}},	 {{-1,-1}}	}},
	{{  {{-1,-1}},	 {{3,5}},	 {{-1,-1}},	 {{0,5}},	 {{-1,-1}},	 {{-1,-1}},	 {{0,3}},	 {{-1,-1}}	}},
	{{  {{1,5}},	 {{-1,-1}},	 {{0,5}},	 {{-1,-1}},	 {{-1,-1}},	 {{-1,-1}},	 {{-1,-1}},	 {{0,1}}	}},
	{{  {{1,2}},	 {{-1,-1}},	 {{-1,-1}},	 {{-1,-1}},	 {{-1,-1}},	 {{2,4}},	 {{-1,-1}},	 {{1,4}}	}},
	{{  {{-1,-1}},	 {{2,3}},	 {{-1,-1}},	 {{-1,-1}},	 {{2,4}},	 {{-1,-1}},	 {{3,4}},	 {{-1,-1}}	}},
	{{  {{-1,-1}},	 {{-1,-1}},	 {{0,3}},	 {{-1,-1}},	 {{-1,-1}},	 {{3,4}},	 {{-1,-1}},	 {{0,4}}	}},
	{{  {{-1,-1}},	 {{-1,-1}},	 {{-1,-1}},	 {{0,1}},	 {{1,4}},	 {{-1,-1}},	 {{0,4}},	 {{-1,-1}}	}},
}};

//@@//perform simple check on tables
//@@check_check()
//@@{
//@@	int i,j;
//@@
//@@	for (i=0;i<8;i++)
//@@		for (j=0;j<8;j++)
//@@			Assert(Edge_to_sides[i][j][0] == Edge_to_sides[j][i][0] && 
//@@					Edge_to_sides[i][j][1] == Edge_to_sides[j][i][1]);
//@@
//@@	for (i=0;i<6;i++)
//@@		for (j=0;j<6;j++)
//@@			Assert(Two_sides_to_edge[i][j][0] == Two_sides_to_edge[j][i][0] && 
//@@					Two_sides_to_edge[i][j][1] == Two_sides_to_edge[j][i][1]);
//@@
//@@
//@@}


//given an edge, tell what side is on that edge
static int find_seg_side(segment *seg,const array<int, 2> &verts,unsigned notside)
{
	if (notside >= MAX_SIDES_PER_SEGMENT)
		throw std::logic_error("invalid notside");
	int side0,side1;
	int	v0,v1;

//@@	check_check();

	v0 = verts[0];
	v1 = verts[1];

	auto b = begin(seg->verts);
	auto e = end(seg->verts);
	auto iv0 = e;
	auto iv1 = e;
	for (auto i = b;;)
	{
		if (iv0 == e && *i == v0)
		{
			iv0 = i;
			if (iv1 != e)
				break;
		}
		if (iv1 == e && *i == v1)
		{
			iv1 = i;
			if (iv0 != e)
				break;
		}
		if (++i == e)
			return -1;
	}

	const auto &eptr = Edge_to_sides[std::distance(b, iv0)][std::distance(b, iv1)];

	side0 = eptr[0];
	side1 = eptr[1];

	Assert(side0!=-1 && side1!=-1);

	if (side0 != notside) {
		Assert(side1==notside);
		return side0;
	}
	else {
		Assert(side0==notside);
		return side1;
	}

}

//find the two segments that join a given seg though two sides, and
//the sides of those segments the abut. 
static int find_joining_side_norms(vms_vector *norm0_0,vms_vector *norm0_1,vms_vector *norm1_0,vms_vector *norm1_1,vms_vector **pnt0,vms_vector **pnt1,segment *seg,int s0,int s1)
{
	segment *seg0,*seg1;
	int edgeside0,edgeside1;

	Assert(s0!=-1 && s1!=-1);

	const array<int, 2> edge_verts = {
		{seg->verts[Two_sides_to_edge[s0][s1][0]], seg->verts[Two_sides_to_edge[s0][s1][1]]}
	};
	if (edge_verts[0] == -1 || edge_verts[1] == -1)
		throw std::logic_error("invalid edge vert");

	seg0 = &Segments[seg->children[s0]];
	seg1 = &Segments[seg->children[s1]];

	edgeside0 = find_seg_side(seg0,edge_verts,find_connect_side(seg,seg0));
	if (edgeside0 == -1) return 0;
	edgeside1 = find_seg_side(seg1,edge_verts,find_connect_side(seg,seg1));
	if (edgeside1 == -1) return 0;

		*norm0_0 = seg0->sides[edgeside0].normals[0];
		*norm0_1 = seg0->sides[edgeside0].normals[1];
		*norm1_0 = seg1->sides[edgeside1].normals[0];
		*norm1_1 = seg1->sides[edgeside1].normals[1];

	*pnt0 = &Vertices[seg0->verts[Side_to_verts[edgeside0][seg0->sides[edgeside0].type==3?1:0]]];
	*pnt1 = &Vertices[seg1->verts[Side_to_verts[edgeside1][seg1->sides[edgeside1].type==3?1:0]]];

	return 1;
}

//see if the order matters for these two children.
//returns 0 if order doesn't matter, 1 if c0 before c1, -1 if c1 before c0
static int compare_children(segment *seg,short c0,short c1)
{
	vms_vector norm0_0,norm0_1,*pnt0,temp;
	vms_vector norm1_0,norm1_1,*pnt1;
	fix d0_0,d0_1,d1_0,d1_1,d0,d1;
	int t;

	if (Side_opposite[c0] == c1) return 0;

	Assert(c0!=-1 && c1!=-1);

	//find normals of adjoining sides

	t = find_joining_side_norms(&norm0_0,&norm0_1,&norm1_0,&norm1_1,&pnt0,&pnt1,seg,c0,c1);

	if (!t) // can happen - 4D rooms!
		return 0;

	vm_vec_sub(&temp,&Viewer_eye,pnt0);
	d0_0 = vm_vec_dot(&norm0_0,&temp);
	d0_1 = vm_vec_dot(&norm0_1,&temp);

	vm_vec_sub(&temp,&Viewer_eye,pnt1);
	d1_0 = vm_vec_dot(&norm1_0,&temp);
	d1_1 = vm_vec_dot(&norm1_1,&temp);

	d0 = (d0_0 < 0 || d0_1 < 0)?-1:1;
	d1 = (d1_0 < 0 || d1_1 < 0)?-1:1;

	if (d0 < 0 && d1 < 0)
		return 0;

	if (d0 < 0)
		return 1;
	else if (d1 < 0)
		return -1;
	else
		return 0;

}

int ssc_total=0,ssc_swaps=0;

//short the children of segment to render in the correct order
//returns non-zero if swaps were made
static int sort_seg_children(segment *seg,int n_children,short *child_list)
{
	int i,j;
	int r;
	int made_swaps,count;

	if (n_children == 0) return 0;

 ssc_total++;

	//for each child,  compare with other children and see if order matters
	//if order matters, fix if wrong

	count = 0;

	do {
		made_swaps = 0;

		for (i=0;i<n_children-1;i++)
			for (j=i+1;child_list[i]!=-1 && j<n_children;j++)
				if (child_list[j]!=-1) {
					r = compare_children(seg,child_list[i],child_list[j]);

					if (r == 1) {
						int temp = child_list[i];
						child_list[i] = child_list[j];
						child_list[j] = temp;
						made_swaps=1;
					}
				}

	} while (made_swaps && ++count<n_children);

 if (count)
  ssc_swaps++;

	return count;
}

static void add_obj_to_seglist(render_state_t &rstate, objnum_t objnum,int listnum)
{
	int i,checkn;
	objnum_t marker;

	checkn = listnum;

	//first, find a slot

	do {

		for (i=0; rstate.render_obj_list[checkn][i] >= 0; ++i);
	
		Assert(i < OBJS_PER_SEG);
	
		marker = rstate.render_obj_list[checkn][i];

		if (marker != object_none) {
			checkn = -marker;
			//Assert(checkn < MAX_RENDER_SEGS+N_EXTRA_OBJ_LISTS);
			if (checkn >= MAX_RENDER_SEGS+N_EXTRA_OBJ_LISTS) {
				Int3();
				return;
			}
		}

	} while (marker != object_none);

	//now we have found a slot.  put object in it

	if (i != OBJS_PER_SEG-1) {

		rstate.render_obj_list[checkn][i] = objnum;
		rstate.render_obj_list[checkn][i+1] = object_none;
	}
	else {				//chain to additional list
		int lookn;

		//find an available sublist

		for (lookn=MAX_RENDER_SEGS;rstate.render_obj_list[lookn][0]!=object_none && lookn < MAX_RENDER_SEGS+N_EXTRA_OBJ_LISTS;lookn++);

		//Assert(lookn<MAX_RENDER_SEGS+N_EXTRA_OBJ_LISTS);
		if (lookn >= MAX_RENDER_SEGS+N_EXTRA_OBJ_LISTS) {
			Int3();
			return;
		}

		rstate.render_obj_list[checkn][i] = -lookn;
		rstate.render_obj_list[lookn][0] = objnum;
		rstate.render_obj_list[lookn][1] = object_none;

	}

}
#ifdef __sun__
// the following is a drop-in replacement for the broken libc qsort on solaris
// taken from http://www.snippets.org/snippets/portable/RG_QSORT+C.php3

#define qsort qsort_dropin

/******************************************************************/
/* qsort.c  --  Non-Recursive ANSI Quicksort function             */
/* Public domain by Raymond Gardner, Englewood CO  February 1991  */
/******************************************************************/
#define  COMP(a, b)  ((*comp)((void *)(a), (void *)(b)))
#define  T 7 // subfiles of <= T elements will be insertion sorteded (T >= 3)
#define  SWAP(a, b)  (swap_bytes((char *)(a), (char *)(b), size))

static void swap_bytes(char *a, char *b, size_t nbytes)
{
   char tmp;
   do {
      tmp = *a; *a++ = *b; *b++ = tmp;
   } while ( --nbytes );
}

void qsort(void *basep, size_t nelems, size_t size,
                            int (*comp)(const void *, const void *))
{
   char *stack[40], **sp;       /* stack and stack pointer        */
   char *i, *j, *limit;         /* scan and limit pointers        */
   size_t thresh;               /* size of T elements in bytes    */
   char *base;                  /* base pointer as char *         */
   base = (char *)basep;        /* set up char * base pointer     */
   thresh = T * size;           /* init threshold                 */
   sp = stack;                  /* init stack pointer             */
   limit = base + nelems * size;/* pointer past end of array      */
   for ( ;; ) {                 /* repeat until break...          */
      if ( limit - base > thresh ) {  /* if more than T elements  */
                                      /*   swap base with middle  */
         SWAP((((limit-base)/size)/2)*size+base, base);
         i = base + size;             /* i scans left to right    */
         j = limit - size;            /* j scans right to left    */
         if ( COMP(i, j) > 0 )        /* Sedgewick's              */
            SWAP(i, j);               /*    three-element sort    */
         if ( COMP(base, j) > 0 )     /*        sets things up    */
            SWAP(base, j);            /*            so that       */
         if ( COMP(i, base) > 0 )     /*      *i <= *base <= *j   */
            SWAP(i, base);            /* *base is pivot element   */
         for ( ;; ) {                 /* loop until break         */
            do                        /* move i right             */
               i += size;             /*        until *i >= pivot */
            while ( COMP(i, base) < 0 );
            do                        /* move j left              */
               j -= size;             /*        until *j <= pivot */
            while ( COMP(j, base) > 0 );
            if ( i > j )              /* if pointers crossed      */
               break;                 /*     break loop           */
            SWAP(i, j);       /* else swap elements, keep scanning*/
         }
         SWAP(base, j);         /* move pivot into correct place  */
         if ( j - base > limit - i ) {  /* if left subfile larger */
            sp[0] = base;             /* stack left subfile base  */
            sp[1] = j;                /*    and limit             */
            base = i;                 /* sort the right subfile   */
         } else {                     /* else right subfile larger*/
            sp[0] = i;                /* stack right subfile base */
            sp[1] = limit;            /*    and limit             */
            limit = j;                /* sort the left subfile    */
         }
         sp += 2;                     /* increment stack pointer  */
      } else {      /* else subfile is small, use insertion sort  */
         for ( j = base, i = j+size; i < limit; j = i, i += size )
            for ( ; COMP(j, j+size) > 0; j -= size ) {
               SWAP(j, j+size);
               if ( j == base )
                  break;
            }
         if ( sp != stack ) {         /* if any entries on stack  */
            sp -= 2;                  /* pop the base and limit   */
            base = sp[0];
            limit = sp[1];
         } else                       /* else stack empty, done   */
            break;
      }
   }
}
#endif // __sun__ qsort drop-in replacement

#if defined(DXX_BUILD_DESCENT_I)
#define SORT_LIST_SIZE 50
#elif defined(DXX_BUILD_DESCENT_II)
#define SORT_LIST_SIZE 100
#endif

struct sort_item
{
	int objnum;
	fix dist;
};

sort_item sort_list[SORT_LIST_SIZE];
int n_sort_items;

//compare function for object sort. 
static int sort_func(const sort_item *a,const sort_item *b)
{
	fix delta_dist;
	delta_dist = a->dist - b->dist;

#if defined(DXX_BUILD_DESCENT_II)
	object *obj_a,*obj_b;

	obj_a = &Objects[a->objnum];
	obj_b = &Objects[b->objnum];

	if (abs(delta_dist) < (obj_a->size + obj_b->size)) {		//same position

		//these two objects are in the same position.  see if one is a fireball
		//or laser or something that should plot on top.  Don't do this for
		//the afterburner blobs, though.

		if (obj_a->type == OBJ_WEAPON || (obj_a->type == OBJ_FIREBALL && obj_a->id != VCLIP_AFTERBURNER_BLOB))
			if (!(obj_b->type == OBJ_WEAPON || obj_b->type == OBJ_FIREBALL))
				return -1;	//a is weapon, b is not, so say a is closer
			else;				//both are weapons 
		else
			if (obj_b->type == OBJ_WEAPON || (obj_b->type == OBJ_FIREBALL && obj_b->id != VCLIP_AFTERBURNER_BLOB))
				return 1;	//b is weapon, a is not, so say a is farther

		//no special case, fall through to normal return
	}
#endif
	return delta_dist;	//return distance
}

static void build_object_lists(render_state_t &rstate, int n_segs)
{
	int nn;

	for (nn=0;nn<MAX_RENDER_SEGS+N_EXTRA_OBJ_LISTS;nn++)
		rstate.render_obj_list[nn][0] = object_none;

	for (nn=0;nn<n_segs;nn++) {
		segnum_t segnum = rstate.Render_list[nn];
		if (segnum != segment_none) {
			range_for (auto obj, objects_in(Segments[segnum]))
			{
				int list_pos;
				if (obj->type == OBJ_NONE)
					continue;

				Assert( obj->segnum == segnum );

				if (obj->flags & OF_ATTACHED)
					continue;		//ignore this object

				segnum_t new_segnum = segnum;
				list_pos = nn;

#if defined(DXX_BUILD_DESCENT_I)
				int did_migrate;
				if (obj->type != OBJ_CNTRLCEN)		//don't migrate controlcen
#elif defined(DXX_BUILD_DESCENT_II)
				const int did_migrate = 0;
				if (obj->type != OBJ_CNTRLCEN && !(obj->type==OBJ_ROBOT && get_robot_id(obj)==65))		//don't migrate controlcen
#endif
				do {
					segmasks m;

#if defined(DXX_BUILD_DESCENT_I)
					did_migrate = 0;
#endif
					m = get_seg_masks(&obj->pos, new_segnum, obj->size, __FILE__, __LINE__);
	
					if (m.sidemask) {
						int sn,sf;

						for (sn=0,sf=1;sn<6;sn++,sf<<=1)
							if (m.sidemask & sf) {
#if defined(DXX_BUILD_DESCENT_I)
								segment *seg = &Segments[obj->segnum];
#elif defined(DXX_BUILD_DESCENT_II)
								segment *seg = &Segments[new_segnum];
#endif
		
								if (WALL_IS_DOORWAY(seg,sn) & WID_FLY_FLAG) {		//can explosion migrate through
									int child = seg->children[sn];
									int checknp;
		
									for (checknp=list_pos;checknp--;)
										if (rstate.Render_list[checknp] == child) {
											new_segnum = child;
											list_pos = checknp;
#if defined(DXX_BUILD_DESCENT_I)
											did_migrate = 1;
#endif
										}
								}
							}
					}
	
				} while (did_migrate);

				add_obj_to_seglist(rstate, obj,list_pos);
	
			}

		}
	}

	//now that there's a list for each segment, sort the items in those lists
	for (nn=0;nn<n_segs;nn++) {
		segnum_t segnum = rstate.Render_list[nn];
		if (segnum != segment_none) {
			int t,lookn,i,n;

			//first count the number of objects & copy into sort list

			lookn = nn;
			i = n_sort_items = 0;
			while ((t=rstate.render_obj_list[lookn][i++])!=object_none)
				if (t<0)
					{lookn = -t; i=0;}
				else
					if (n_sort_items < SORT_LIST_SIZE-1) {		//add if room
						sort_list[n_sort_items].objnum = t;
						//NOTE: maybe use depth, not dist - quicker computation
						sort_list[n_sort_items].dist = vm_vec_dist(&Objects[t].pos,&Viewer_eye);
						n_sort_items++;
					}
#if defined(DXX_BUILD_DESCENT_II)
					else {			//no room for object
						int ii;

						#ifndef NDEBUG
						PHYSFS_file *tfile=PHYSFSX_openWriteBuffered("sortlist.out");

						//I find this strange, so I'm going to write out
						//some information to look at later
						if (tfile) {
							for (ii=0;ii<SORT_LIST_SIZE;ii++) {
								int objnum = sort_list[ii].objnum;

								PHYSFSX_printf(tfile,"Obj %3d  Type = %2d  Id = %2d  Dist = %08x  Segnum = %3d\n",
									objnum,Objects[objnum].type,Objects[objnum].id,sort_list[ii].dist,Objects[objnum].segnum);
							}
							PHYSFS_close(tfile);
						}
						#endif

						Int3();	//Get Matt!!!

						//Now try to find a place for this object by getting rid
						//of an object we don't care about

						for (ii=0;ii<SORT_LIST_SIZE;ii++) {
							int objnum = sort_list[ii].objnum;
							object *obj = &Objects[objnum];
							int type = obj->type;

							//replace debris & fireballs
							if (type == OBJ_DEBRIS || type == OBJ_FIREBALL) {
								fix dist = vm_vec_dist(&Objects[t].pos,&Viewer_eye);

								//don't replace same kind of object unless new 
								//one is closer

								if (Objects[t].type != type || dist < sort_list[ii].dist) {
									sort_list[ii].objnum = t;
									sort_list[ii].dist = dist;
									break;
								}
							}
						}

						Int3();	//still couldn't find a slot
					}
#endif

			//now call qsort
			qsort(sort_list,n_sort_items,sizeof(*sort_list),
					(int (*)(const void*,const void*))sort_func);

			//now copy back into list

			lookn = nn;
			i = 0;
			n = n_sort_items;
			while ((t=rstate.render_obj_list[lookn][i])!=object_none && n>0)
				if (t<0)
					{lookn = -t; i=0;}
				else
					rstate.render_obj_list[lookn][i++] = sort_list[--n].objnum;
			rstate.render_obj_list[lookn][i] = object_none;	//mark (possibly new) end
		}
	}
}

vms_angvec Player_head_angles;

//--unused-- int Total_num_tmaps_drawn=0;

int Rear_view=0;

#ifdef JOHN_ZOOM
fix Zoom_factor=F1_0;
#endif
//renders onto current canvas
void render_frame(fix eye_offset, int window_num)
{
	if (Endlevel_sequence) {
		render_endlevel_frame(eye_offset);
		return;
	}

	if ( Newdemo_state == ND_STATE_RECORDING && eye_offset >= 0 )	{
     
      if (RenderingType==0)
   		newdemo_record_start_frame(FrameTime );
      if (RenderingType!=255)
   		newdemo_record_viewer_object(Viewer);
	}
  
   //Here:

	start_lighting_frame(Viewer);		//this is for ugly light-smoothing hack
  
	g3_start_frame();

	Viewer_eye = Viewer->pos;

//	if (Viewer->type == OBJ_PLAYER && (PlayerCfg.CockpitMode[1]!=CM_REAR_VIEW))
//		vm_vec_scale_add2(&Viewer_eye,&Viewer->orient.fvec,(Viewer->size*3)/4);

	if (eye_offset)	{
		vm_vec_scale_add2(&Viewer_eye,&Viewer->orient.rvec,eye_offset);
	}

	#ifdef EDITOR
	if (EditorWindow)
		Viewer_eye = Viewer->pos;
	#endif

	segnum_t start_seg_num = find_point_seg(&Viewer_eye,Viewer->segnum);

	if (start_seg_num==segment_none)
		start_seg_num = Viewer->segnum;

	if (Rear_view && (Viewer==ConsoleObject)) {
		vms_matrix headm,viewm;
		Player_head_angles.p = Player_head_angles.b = 0;
		Player_head_angles.h = 0x7fff;
		vm_angles_2_matrix(&headm,&Player_head_angles);
		vm_matrix_x_matrix(&viewm,&Viewer->orient,&headm);
		g3_set_view_matrix(&Viewer_eye,&viewm,Render_zoom);
	} else	{
#ifdef JOHN_ZOOM
		if (keyd_pressed[KEY_RSHIFT] )	{
			Zoom_factor += FrameTime*4;
			if (Zoom_factor > F1_0*5 ) Zoom_factor=F1_0*5;
		} else {
			Zoom_factor -= FrameTime*4;
			if (Zoom_factor < F1_0 ) Zoom_factor = F1_0;
		}
		g3_set_view_matrix(&Viewer_eye,&Viewer->orient,fixdiv(Render_zoom,Zoom_factor));
#else
		g3_set_view_matrix(&Viewer_eye,&Viewer->orient,Render_zoom);
#endif
	}

	if (Clear_window == 1) {
		if (Clear_window_color == -1)
			Clear_window_color = BM_XRGB(0, 0, 0);	//BM_XRGB(31, 15, 7);
		gr_clear_canvas(Clear_window_color);
	}
#if defined(DXX_BUILD_DESCENT_II)
	#ifndef NDEBUG
	if (Show_only_curside)
		gr_clear_canvas(Clear_window_color);
	#endif
#endif

	render_mine(start_seg_num, eye_offset, window_num);

	g3_end_frame();

   //RenderingType=0;

	// -- Moved from here by MK, 05/17/95, wrong if multiple renders/frame! FrameCount++;		//we have rendered a frame
}

int first_terminal_seg;

#if defined(DXX_BUILD_DESCENT_II)
void update_rendered_data(int window_num, object *viewer, int rear_view_flag)
{
	Assert(window_num < MAX_RENDERED_WINDOWS);
	Window_rendered_data[window_num].time = timer_query();
	Window_rendered_data[window_num].viewer = viewer;
	Window_rendered_data[window_num].rear_view = rear_view_flag;
}
#endif

//build a list of segments to be rendered
//fills in Render_list & N_render_segs
static void build_segment_list(render_state_t &rstate, visited_twobit_array_t &visited, short start_seg_num, int window_num)
{
	int	lcnt,scnt,ecnt;
	int	l,c;
	int	ch;

	rstate.render_pos.fill(-1);
	rstate.processed = {};

	#ifndef NDEBUG
	memset(visited2, 0, sizeof(visited2[0])*(Highest_segment_index+1));
	#endif

	lcnt = scnt = 0;

	rstate.Render_list[lcnt] = start_seg_num;
	visited[start_seg_num]=1;
	rstate.Seg_depth[lcnt] = 0;
	lcnt++;
	ecnt = lcnt;
	rstate.render_pos[start_seg_num] = 0;

	#ifndef NDEBUG
	if (pre_draw_segs)
		render_segment(start_seg_num, window_num);
	#endif

	rstate.render_windows[0].left = rstate.render_windows[0].top = 0;
	rstate.render_windows[0].right = grd_curcanv->cv_bitmap.bm_w-1;
	rstate.render_windows[0].bot = grd_curcanv->cv_bitmap.bm_h-1;

	//breadth-first renderer

	//build list

	for (l=0;l<Render_depth;l++) {

		//while (scnt < ecnt) {
		for (scnt=0;scnt < ecnt;scnt++) {
			int rotated;
			short child_list[MAX_SIDES_PER_SEGMENT];		//list of ordered sides to process
			int n_children;										//how many sides in child_list
			segment *seg;

			if (rstate.processed[scnt])
				continue;

			rstate.processed[scnt] = true;

			segnum_t segnum = rstate.Render_list[scnt];
			rect *check_w = &rstate.render_windows[scnt];

			#ifndef NDEBUG
			if (draw_boxes)
				draw_window_box(RED,check_w->left,check_w->top,check_w->right,check_w->bot);
			#endif

			if (segnum == segment_none) continue;

			seg = &Segments[segnum];
			rotated=0;

			//look at all sides of this segment.
			//tricky code to look at sides in correct order follows

			for (c=n_children=0;c<MAX_SIDES_PER_SEGMENT;c++) {		//build list of sides
				int wid;

				wid = WALL_IS_DOORWAY(seg, c);
				if (wid & WID_RENDPAST_FLAG)
				{
					{
						ubyte codes_and=0xff;
						rotate_list(seg->verts);
						rotated=1;

						range_for (auto i, Side_to_verts[c])
							codes_and &= Segment_points[seg->verts[i]].p3_codes;

						if (codes_and & CC_BEHIND) continue;

					}
					child_list[n_children++] = c;
				}
			}

			//now order the sides in some magical way

			if (new_seg_sorting)
				sort_seg_children(seg,n_children,child_list);

			//for (c=0;c<MAX_SIDES_PER_SEGMENT;c++)	{
			//	ch=seg->children[c];

			for (c=0;c<n_children;c++) {
				int siden;

				siden = child_list[c];
				ch=seg->children[siden];
				{
					{
						int i;
						ubyte codes_and_3d,codes_and_2d;
						short _x,_y,min_x=32767,max_x=-32767,min_y=32767,max_y=-32767;
						int no_proj_flag=0;	//a point wasn't projected

						if (rotated<2) {
							if (!rotated)
								rotate_list(seg->verts);
							project_list(seg->verts);
							rotated=2;
						}

						for (i=0,codes_and_3d=codes_and_2d=0xff;i<4;i++) {
							int p = seg->verts[Side_to_verts[siden][i]];
							g3s_point *pnt = &Segment_points[p];

							if (! (pnt->p3_flags&PF_PROJECTED)) {no_proj_flag=1; break;}

							_x = f2i(pnt->p3_sx);
							_y = f2i(pnt->p3_sy);

							codes_and_3d &= pnt->p3_codes;
							codes_and_2d &= code_window_point(_x,_y,check_w);

							#ifndef NDEBUG
							if (draw_edges) {
								gr_setcolor(BM_XRGB(31,0,31));
								gr_line(pnt->p3_sx,pnt->p3_sy,
									Segment_points[seg->verts[Side_to_verts[siden][(i+1)%4]]].p3_sx,
									Segment_points[seg->verts[Side_to_verts[siden][(i+1)%4]]].p3_sy);
							}
							#endif

							if (_x < min_x) min_x = _x;
							if (_x > max_x) max_x = _x;

							if (_y < min_y) min_y = _y;
							if (_y > max_y) max_y = _y;

						}

						#ifndef NDEBUG
						if (draw_boxes)
							draw_window_box(WHITE,min_x,min_y,max_x,max_y);
						#endif

						if (no_proj_flag || (!codes_and_3d && !codes_and_2d)) {	//maybe add this segment
							auto rp = rstate.render_pos[ch];
							rect *new_w = &rstate.render_windows[lcnt];

							if (no_proj_flag) *new_w = *check_w;
							else {
								new_w->left  = max(check_w->left,min_x);
								new_w->right = min(check_w->right,max_x);
								new_w->top   = max(check_w->top,min_y);
								new_w->bot   = min(check_w->bot,max_y);
							}

							//see if this seg already visited, and if so, does current window
							//expand the old window?
							if (rp != -1) {
								if (new_w->left < rstate.render_windows[rp].left ||
										 new_w->top < rstate.render_windows[rp].top ||
										 new_w->right > rstate.render_windows[rp].right ||
										 new_w->bot > rstate.render_windows[rp].bot) {

									new_w->left  = min(new_w->left, rstate.render_windows[rp].left);
									new_w->right = max(new_w->right, rstate.render_windows[rp].right);
									new_w->top   = min(new_w->top, rstate.render_windows[rp].top);
									new_w->bot   = max(new_w->bot, rstate.render_windows[rp].bot);

									if (no_migrate_segs) {
										//no_render_flag[lcnt] = 1;
										rstate.Render_list[lcnt] = segment_none;
										rstate.render_windows[rp] = *new_w;		//get updated window
										rstate.processed[rp] = false;		//force reprocess
										goto no_add;
									}
									else
										rstate.Render_list[rp] = segment_none;
								}
								else goto no_add;
							}

							#ifndef NDEBUG
							if (draw_boxes)
								draw_window_box(5,new_w->left,new_w->top,new_w->right,new_w->bot);
							#endif

							rstate.render_pos[ch] = lcnt;
							rstate.Render_list[lcnt] = ch;
							rstate.Seg_depth[lcnt] = l;
							lcnt++;
							if (lcnt >= MAX_RENDER_SEGS) {goto done_list;}
							visited[ch] = 1;

							#ifndef NDEBUG
							if (pre_draw_segs)
								render_segment(ch, window_num);
							#endif
no_add:
	;

						}
					}
				}
			}
		}

		scnt = ecnt;
		ecnt = lcnt;

	}
done_list:

	lcnt_save = lcnt;
	scnt_save = scnt;

	first_terminal_seg = scnt;
	N_render_segs = lcnt;

}

//renders onto current canvas
void render_mine(segnum_t start_seg_num,fix eye_offset, int window_num)
{
#ifndef NDEBUG
	int		i;
#endif
	int		nn;

	render_state_t rstate;
	//	Initialize number of objects (actually, robots!) rendered this frame.
	Window_rendered_data[window_num].rendered_robots.clear();

	#ifndef NDEBUG
	for (i=0;i<=Highest_object_index;i++)
		object_rendered[i] = 0;
	#endif

	//set up for rendering

	render_start_frame();

	visited_twobit_array_t visited;

	#if defined(EDITOR)
	if (Show_only_curside) {
		rotate_list(Cursegp->verts);
		render_side(Cursegp,Curside);
		goto done_rendering;
	}
	#endif


	#ifdef EDITOR
#if defined(DXX_BUILD_DESCENT_I)
	if (_search_mode || eye_offset>0)
#elif defined(DXX_BUILD_DESCENT_II)
	if (_search_mode)
#endif
	{
		//lcnt = lcnt_save;
		//scnt = scnt_save;
	}
	else
	#endif
		//NOTE LINK TO ABOVE!!
		build_segment_list(rstate, visited, start_seg_num, window_num);		//fills in Render_list & N_render_segs

	//render away
	#ifndef NDEBUG
#if defined(DXX_BUILD_DESCENT_I)
	if (!(_search_mode || eye_offset>0))
#elif defined(DXX_BUILD_DESCENT_II)
	if (!(_search_mode))
#endif
	{
		int i;

		for (i=0;i<N_render_segs;i++) {
			segnum_t segnum = rstate.Render_list[i];
			if (segnum != segment_none)
			{
				if (visited2[segnum])
					Int3();		//get Matt
				else
					visited2[segnum] = 1;
			}
		}
	}
	#endif

	if (!(_search_mode))
		build_object_lists(rstate, N_render_segs);

	if (eye_offset<=0) // Do for left eye or zero.
		set_dynamic_light(rstate);

	if (!_search_mode && Clear_window == 2) {
		if (first_terminal_seg < N_render_segs) {
			int i;

			if (Clear_window_color == -1)
				Clear_window_color = BM_XRGB(0, 0, 0);	//BM_XRGB(31, 15, 7);
	
			gr_setcolor(Clear_window_color);
	
			for (i=first_terminal_seg; i<N_render_segs; i++) {
				if (rstate.Render_list[i] != segment_none) {
					#ifndef NDEBUG
					if ((rstate.render_windows[i].left == -1) || (rstate.render_windows[i].top == -1) || (rstate.render_windows[i].right == -1) || (rstate.render_windows[i].bot == -1))
						Int3();
					else
					#endif
						//NOTE LINK TO ABOVE!
						gr_rect(rstate.render_windows[i].left, rstate.render_windows[i].top, rstate.render_windows[i].right, rstate.render_windows[i].bot);
				}
			}
		}
	}

#ifndef OGL
	for (nn=N_render_segs;nn--;) {
		int objnp;

		// Interpolation_method = 0;
		segnum_t segnum = rstate.Render_list[nn];
		Current_seg_depth = rstate.Seg_depth[nn];

		//if (!no_render_flag[nn])
		if (segnum!=segment_none && (_search_mode || visited[segnum]!=3)) {
			//set global render window vars

			{
				Window_clip_left  = rstate.render_windows[nn].left;
				Window_clip_top   = rstate.render_windows[nn].top;
				Window_clip_right = rstate.render_windows[nn].right;
				Window_clip_bot   = rstate.render_windows[nn].bot;
			}

			render_segment(segnum, window_num);
			visited[segnum]=3;

			{		//reset for objects
				Window_clip_left  = Window_clip_top = 0;
				Window_clip_right = grd_curcanv->cv_bitmap.bm_w-1;
				Window_clip_bot   = grd_curcanv->cv_bitmap.bm_h-1;
			}

			if (migrate_objects) {
				//int n_expl_objs=0,expl_objs[5],i;
				int listnum;
				int save_linear_depth = Max_linear_depth;

				Max_linear_depth = Max_linear_depth_objects;

				listnum = nn;

				for (objnp=0;rstate.render_obj_list[listnum][objnp]!=object_none;)	{
					int ObjNumber = rstate.render_obj_list[listnum][objnp];

					if (ObjNumber >= 0) {
						do_render_object(ObjNumber, window_num);	// note link to above else
						objnp++;
					}
					else {

						listnum = -ObjNumber;
						objnp = 0;

					}

				}

				Max_linear_depth = save_linear_depth;

			}

		}
	}
#else
	// Sorting elements for Alpha - 3 passes
	// First Pass: render opaque level geometry + transculent level geometry with high Alpha-Test func
	for (nn=N_render_segs;nn--;)
	{
		segnum_t segnum = rstate.Render_list[nn];
		Current_seg_depth = rstate.Seg_depth[nn];

#if defined(DXX_BUILD_DESCENT_I)
		if (segnum!=segment_none && (_search_mode || eye_offset>0 || visited[segnum]!=3))
#elif defined(DXX_BUILD_DESCENT_II)
		if (segnum!=segment_none && (_search_mode || visited[segnum]!=3))
#endif
		{
			//set global render window vars

			{
				Window_clip_left  = rstate.render_windows[nn].left;
				Window_clip_top   = rstate.render_windows[nn].top;
				Window_clip_right = rstate.render_windows[nn].right;
				Window_clip_bot   = rstate.render_windows[nn].bot;
			}

			// render segment
			{
				segment		*seg = &Segments[segnum];
				int			sn;
				Assert(segnum!=segment_none && segnum<=Highest_segment_index);
				g3s_codes 	cc=rotate_list(seg->verts);

				if (! cc.uand) {		//all off screen?

				  if (Viewer->type!=OBJ_ROBOT)
					Automap_visited[segnum]=1;

					for (sn=0; sn<MAX_SIDES_PER_SEGMENT; sn++)
					{
						auto wid = WALL_IS_DOORWAY(seg, sn);
						if (wid == WID_TRANSPARENT_WALL || wid == WID_TRANSILLUSORY_WALL
#if defined(DXX_BUILD_DESCENT_II)
							|| (wid & WID_CLOAKED_FLAG)
#endif
							)
						{
							glAlphaFunc(GL_GEQUAL,0.8);
							render_side(seg, sn);
							glAlphaFunc(GL_GEQUAL,0.02);
						}
						else
							render_side(seg, sn);
					}
				}
			}
			visited[segnum]=3;
		}
	}

	visited.clear();

	// Second Pass: Objects
	for (nn=N_render_segs;nn--;)
	{
		int objnp;

		segnum_t segnum = rstate.Render_list[nn];
		Current_seg_depth = rstate.Seg_depth[nn];

#if defined(DXX_BUILD_DESCENT_I)
		if (segnum!=segment_none && (_search_mode || eye_offset>0 || visited[segnum]!=3))
#elif defined(DXX_BUILD_DESCENT_II)
		if (segnum!=segment_none && (_search_mode || visited[segnum]!=3))
#endif
		{
			//set global render window vars

			{
				Window_clip_left  = rstate.render_windows[nn].left;
				Window_clip_top   = rstate.render_windows[nn].top;
				Window_clip_right = rstate.render_windows[nn].right;
				Window_clip_bot   = rstate.render_windows[nn].bot;
			}

			visited[segnum]=3;

			{		//reset for objects
				Window_clip_left  = Window_clip_top = 0;
				Window_clip_right = grd_curcanv->cv_bitmap.bm_w-1;
				Window_clip_bot   = grd_curcanv->cv_bitmap.bm_h-1;
			}

			// render objects
			{
				int listnum;
				int save_linear_depth = Max_linear_depth;

				Max_linear_depth = Max_linear_depth_objects;

				listnum = nn;

				for (objnp=0;rstate.render_obj_list[listnum][objnp]!=object_none;)
				{
					int ObjNumber = rstate.render_obj_list[listnum][objnp];

					if (ObjNumber >= 0)
					{
						do_render_object(ObjNumber, window_num);	// note link to above else
						objnp++;
					}
					else
					{
						listnum = -ObjNumber;
						objnp = 0;

					}
				}
				Max_linear_depth = save_linear_depth;
			}
		}
	}

	visited.clear();

	// Third Pass - Render Transculent level geometry with normal Alpha-Func
	for (nn=N_render_segs;nn--;)
	{
		segnum_t segnum = rstate.Render_list[nn];
		Current_seg_depth = rstate.Seg_depth[nn];

#if defined(DXX_BUILD_DESCENT_I)
		if (segnum!=segment_none && (_search_mode || eye_offset>0 || visited[segnum]!=3))
#elif defined(DXX_BUILD_DESCENT_II)
		if (segnum!=segment_none && (_search_mode || visited[segnum]!=3))
#endif
		{
			//set global render window vars

			{
				Window_clip_left  = rstate.render_windows[nn].left;
				Window_clip_top   = rstate.render_windows[nn].top;
				Window_clip_right = rstate.render_windows[nn].right;
				Window_clip_bot   = rstate.render_windows[nn].bot;
			}

			// render segment
			{
				segment		*seg = &Segments[segnum];
				int			sn;
				Assert(segnum!=segment_none && segnum<=Highest_segment_index);
				g3s_codes 	cc=rotate_list(seg->verts);

				if (! cc.uand) {		//all off screen?

				  if (Viewer->type!=OBJ_ROBOT)
					Automap_visited[segnum]=1;

					for (sn=0; sn<MAX_SIDES_PER_SEGMENT; sn++)
					{
						auto wid = WALL_IS_DOORWAY(seg, sn);
						if (wid == WID_TRANSPARENT_WALL || wid == WID_TRANSILLUSORY_WALL
#if defined(DXX_BUILD_DESCENT_II)
							|| (wid & WID_CLOAKED_FLAG)
#endif
							)
							render_side(seg, sn);
					}
				}
			}
			visited[segnum]=3;
		}
	}
#endif

	// -- commented out by mk on 09/14/94...did i do a good thing??  object_render_targets();

#ifdef EDITOR
	#ifndef NDEBUG
	//draw curedge stuff
	if (Outline_mode) outline_seg_side(Cursegp,Curside,Curedge,Curvert);
	#endif

done_rendering:
	;

#endif

}
#ifdef EDITOR
//finds what segment is at a given x&y -  seg,side,face are filled in
//works on last frame rendered. returns true if found
//if seg<0, then an object was found, and the object number is -seg-1
int find_seg_side_face(short x,short y,int *seg,int *side,int *face,int *poly)
{
	_search_mode = -1;

	_search_x = x; _search_y = y;

	found_seg = -1;

	if (render_3d_in_big_window) {
		gr_set_current_canvas(LargeView.ev_canv);

		render_frame(0, 0);
	}
	else {
		gr_set_current_canvas(Canv_editor_game);
		render_frame(0, 0);
	}

	_search_mode = 0;

	*seg = found_seg;
	*side = found_side;
	*face = found_face;
	*poly = found_poly;

	return (found_seg!=-1);

}

#endif
