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
 * Code to render cool external-scene terrain
 *
 */

#include <bitset>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "3d.h"
#include "dxxerror.h"
#include "gr.h"
#include "texmap.h"
#include "iff.h"
#include "u_mem.h"
#include "inferno.h"
#include "textures.h"
#include "object.h"
#include "endlevel.h"
#include "render.h"
#include "player.h"
#include "segment.h"
#include "terrain.h"
#include <memory>

#define GRID_MAX_SIZE   64
#define GRID_SCALE      i2f(2*20)
#define HEIGHT_SCALE    f1_0

namespace dcx {

namespace {

static int grid_w,grid_h;

static RAIIdmem<ubyte[]> height_array;
static std::unique_ptr<uint8_t[]> light_array;

#define HEIGHT(_i,_j) (height_array[(_i)*grid_w+(_j)])
#define LIGHT(_i,_j) light_array[(_i)*grid_w+(_j)]

//!!#define HEIGHT(_i,_j)   height_array[(grid_h-1-j)*grid_w+(_i)]
//!!#define LIGHT(_i,_j)    light_array[(grid_h-1-j)*grid_w+(_i)]

#define LIGHTVAL(_i,_j) (static_cast<fix>(LIGHT(_i, _j)) << 8)

static grs_bitmap *terrain_bm;
static int terrain_outline=0;
static int org_i,org_j;
static void build_light_table();

}

}

// ------------------------------------------------------------------------
static void draw_cell(grs_canvas &canvas, const vms_vector &Viewer_eye, const int i, const int j, cg3s_point &p0, cg3s_point &p1, cg3s_point &p2, cg3s_point &p3, int &mine_tiles_drawn)
{
	std::array<cg3s_point *, 3> pointlist;

	pointlist[0] = &p0;
	pointlist[1] = &p1;
	pointlist[2] = &p3;
	std::array<g3s_lrgb, 3> lrgb_list1;
	std::array<g3s_uvl, 3> uvl_list1;
	lrgb_list1[0].r = lrgb_list1[0].g = lrgb_list1[0].b = uvl_list1[0].l = LIGHTVAL(i,j);
	lrgb_list1[1].r = lrgb_list1[1].g = lrgb_list1[1].b = uvl_list1[1].l = LIGHTVAL(i,j+1);
	lrgb_list1[2].r = lrgb_list1[2].g = lrgb_list1[2].b = uvl_list1[2].l = LIGHTVAL(i+1,j);

	uvl_list1[0].u = (i)*f1_0/4; uvl_list1[0].v = (j)*f1_0/4;
	uvl_list1[1].u = (i)*f1_0/4; uvl_list1[1].v = (j+1)*f1_0/4;
	uvl_list1[2].u = (i+1)*f1_0/4;   uvl_list1[2].v = (j)*f1_0/4;

	g3_check_and_draw_tmap(canvas, pointlist, uvl_list1, lrgb_list1, *terrain_bm);
	if (terrain_outline) {
#if !DXX_USE_OGL
		const int lsave = Lighting_on;
		Lighting_on=0;
#endif
		const uint8_t color = BM_XRGB(31, 0, 0);
		g3_draw_line(canvas, *pointlist[0],*pointlist[1], color);
		g3_draw_line(canvas, *pointlist[2],*pointlist[0], color);
#if !DXX_USE_OGL
		Lighting_on=lsave;
#endif
	}

	pointlist[0] = &p1;
	pointlist[1] = &p2;
	std::array<g3s_uvl, 3> uvl_list2;
	std::array<g3s_lrgb, 3> lrgb_list2;
	lrgb_list2[0].r = lrgb_list2[0].g = lrgb_list2[0].b = uvl_list2[0].l = LIGHTVAL(i,j+1);
	lrgb_list2[1].r = lrgb_list2[1].g = lrgb_list2[1].b = uvl_list2[1].l = LIGHTVAL(i+1,j+1);
	lrgb_list2[2].r = lrgb_list2[2].g = lrgb_list2[2].b = uvl_list2[2].l = LIGHTVAL(i+1,j);

	uvl_list2[0].u = (i)*f1_0/4; uvl_list2[0].v = (j+1)*f1_0/4;
	uvl_list2[1].u = (i+1)*f1_0/4;   uvl_list2[1].v = (j+1)*f1_0/4;
	uvl_list2[2].u = (i+1)*f1_0/4;   uvl_list2[2].v = (j)*f1_0/4;

	g3_check_and_draw_tmap(canvas, pointlist, uvl_list2, lrgb_list2, *terrain_bm);
	if (terrain_outline) {
#if !DXX_USE_OGL
		const int lsave = Lighting_on;
		Lighting_on=0;
#endif
		const uint8_t color = BM_XRGB(31, 0, 0);
		g3_draw_line(canvas, *pointlist[0],*pointlist[1], color);
		g3_draw_line(canvas, *pointlist[1],*pointlist[2], color);
		g3_draw_line(canvas, *pointlist[2],*pointlist[0], color);
#if !DXX_USE_OGL
		Lighting_on=lsave;
#endif
	}

	if (i==org_i && j==org_j)
		mine_tiles_drawn |= 1;
	if (i==org_i-1 && j==org_j)
		mine_tiles_drawn |= 2;
	if (i==org_i && j==org_j-1)
		mine_tiles_drawn |= 4;
	if (i==org_i-1 && j==org_j-1)
		mine_tiles_drawn |= 8;

	if (mine_tiles_drawn == 0xf) {
		//draw_exit_model();
		mine_tiles_drawn=-1;
		window_rendered_data window;
		render_mine(canvas, Viewer_eye, PlayerUniqueEndlevelState.exit_segnum, 0, window);
		//if (ext_expl_playing)
		//	draw_fireball(&external_explosion);
	}

}

namespace {

class terrain_y_cache
{
	static const std::size_t cache_size = 256;
	std::bitset<cache_size> yc_flags;
	std::array<vms_vector, cache_size> y_cache;
public:
	vms_vector &operator()(uint_fast32_t h);
};

}

vms_vector &terrain_y_cache::operator()(uint_fast32_t h)
{
	auto &dyp = y_cache[h];
	if (auto &&ycf = yc_flags[h])
	{
	}
	else
	{
		ycf = 1;
		const auto tv = vm_vec_copy_scale(surface_orient.uvec,h*HEIGHT_SCALE);
		g3_rotate_delta_vec(dyp,tv);
	}
	return dyp;
}

void render_terrain(grs_canvas &canvas, const vms_vector &Viewer_eye, const vms_vector &org_point,int org_2dx,int org_2dy)
{
	vms_vector delta_i,delta_j;		//delta_y;
	g3s_point p,save_p_low,save_p_high;
	g3s_point last_p2;
	int i,j;
	int low_i,high_i,low_j,high_j;
	int viewer_i,viewer_j;
	org_i = org_2dy;
	org_j = org_2dx;

	low_i = 0;  high_i = grid_w-1;
	low_j = 0;  high_j = grid_h-1;

	//@@start_point.x = org_point->x - GRID_SCALE*(org_i - low_i);
	//@@start_point.z = org_point->z - GRID_SCALE*(org_j - low_j);
	//@@start_point.y = org_point->y;
	terrain_y_cache get_dy_vec;

#if !DXX_USE_OGL
	Interpolation_method = 1;
#endif

	{
	const auto tv = vm_vec_copy_scale(surface_orient.rvec,GRID_SCALE);
	g3_rotate_delta_vec(delta_i,tv);
	}
	{
	const auto tv = vm_vec_copy_scale(surface_orient.fvec,GRID_SCALE);
	g3_rotate_delta_vec(delta_j,tv);
	}

	auto start_point = vm_vec_scale_add(org_point,surface_orient.rvec,-(org_i - low_i)*GRID_SCALE);
	vm_vec_scale_add2(start_point,surface_orient.fvec,-(org_j - low_j)*GRID_SCALE);

	{
		const auto tv = vm_vec_sub(Viewer->pos,start_point);
	viewer_i = vm_vec_dot(tv,surface_orient.rvec) / GRID_SCALE;
	viewer_j = vm_vec_dot(tv,surface_orient.fvec) / GRID_SCALE;
	}

	auto last_p = g3_rotate_point(start_point);
	save_p_low = last_p;

	g3s_point save_row[GRID_MAX_SIZE]{};
	// Is this needed?
	for (j=low_j;j<=high_j;j++) {
		g3_add_delta_vec(save_row[j],last_p,get_dy_vec(HEIGHT(low_i,j)));
		if (j==high_j)
			save_p_high = last_p;
		else
			g3_add_delta_vec(last_p,last_p,delta_j);
	}

	int mine_tiles_drawn = 0;    //flags to tell if all 4 tiles under mine have drawn
	for (i=low_i;i<viewer_i;i++) {

		g3_add_delta_vec(save_p_low,save_p_low,delta_i);
		last_p = save_p_low;
		g3_add_delta_vec(last_p2,last_p,get_dy_vec(HEIGHT(i+1,low_j)));
		
		for (j=low_j;j<viewer_j;j++) {
			g3s_point p2;

			g3_add_delta_vec(p,last_p,delta_j);
			g3_add_delta_vec(p2,p,get_dy_vec(HEIGHT(i+1,j+1)));

			draw_cell(canvas, Viewer_eye, i, j, save_row[j], save_row[j+1], p2, last_p2, mine_tiles_drawn);

			last_p = p;
			save_row[j] = last_p2;
			last_p2 = p2;

		}

		vm_vec_negate(delta_j);			//don't have a delta sub...

		g3_add_delta_vec(save_p_high,save_p_high,delta_i);
		last_p = save_p_high;
		g3_add_delta_vec(last_p2,last_p,get_dy_vec(HEIGHT(i+1,high_j)));
		
		for (j=high_j-1;j>=viewer_j;j--) {
			g3s_point p2;

			g3_add_delta_vec(p,last_p,delta_j);
			g3_add_delta_vec(p2,p,get_dy_vec(HEIGHT(i+1,j)));

			draw_cell(canvas, Viewer_eye, i, j, save_row[j], save_row[j+1], last_p2, p2, mine_tiles_drawn);

			last_p = p;
			save_row[j+1] = last_p2;
			last_p2 = p2;

		}

		save_row[j+1] = last_p2;

		vm_vec_negate(delta_j);		//restore sign of j

	}

	//now do i from other end

	vm_vec_negate(delta_i);		//going the other way now...

	//@@start_point.x += (high_i-low_i)*GRID_SCALE;
	vm_vec_scale_add2(start_point,surface_orient.rvec,(high_i-low_i)*GRID_SCALE);
	g3_rotate_point(last_p,start_point);
	save_p_low = last_p;

	for (j=low_j;j<=high_j;j++) {
		g3_add_delta_vec(save_row[j],last_p,get_dy_vec(HEIGHT(high_i,j)));
		if (j==high_j)
			save_p_high = last_p;
		else
			g3_add_delta_vec(last_p,last_p,delta_j);
	}

	for (i=high_i-1;i>=viewer_i;i--) {

		g3_add_delta_vec(save_p_low,save_p_low,delta_i);
		last_p = save_p_low;
		g3_add_delta_vec(last_p2,last_p,get_dy_vec(HEIGHT(i,low_j)));
		
		for (j=low_j;j<viewer_j;j++) {
			g3s_point p2;

			g3_add_delta_vec(p,last_p,delta_j);
			g3_add_delta_vec(p2,p,get_dy_vec(HEIGHT(i,j+1)));

			draw_cell(canvas, Viewer_eye, i, j, last_p2, p2, save_row[j+1], save_row[j], mine_tiles_drawn);

			last_p = p;
			save_row[j] = last_p2;
			last_p2 = p2;

		}

		vm_vec_negate(delta_j);			//don't have a delta sub...

		g3_add_delta_vec(save_p_high,save_p_high,delta_i);
		last_p = save_p_high;
		g3_add_delta_vec(last_p2,last_p,get_dy_vec(HEIGHT(i,high_j)));
		
		for (j=high_j-1;j>=viewer_j;j--) {
			g3s_point p2;

			g3_add_delta_vec(p,last_p,delta_j);
			g3_add_delta_vec(p2,p,get_dy_vec(HEIGHT(i,j)));

			draw_cell(canvas, Viewer_eye, i, j, p2, last_p2, save_row[j+1], save_row[j], mine_tiles_drawn);

			last_p = p;
			save_row[j+1] = last_p2;
			last_p2 = p2;

		}

		save_row[j+1] = last_p2;

		vm_vec_negate(delta_j);		//restore sign of j

	}

}

namespace dcx {

void free_height_array()
{
	height_array.reset();
}

}

void load_terrain(const char *filename)
{
	grs_bitmap height_bitmap;
	int iff_error;
	int i,j;
	ubyte h,min_h,max_h;

	iff_error = iff_read_bitmap(filename, height_bitmap, NULL);
	if (iff_error != IFF_NO_ERROR) {
		Error("File %s - IFF error: %s",filename,iff_errormsg(iff_error));
	}
	grid_w = height_bitmap.bm_w;
	grid_h = height_bitmap.bm_h;

	Assert(grid_w <= GRID_MAX_SIZE);
	Assert(grid_h <= GRID_MAX_SIZE);

	height_array.reset(height_bitmap.get_bitmap_data());

	max_h=0; min_h=255;
	for (i=0;i<grid_w;i++)
		for (j=0;j<grid_h;j++) {

			h = HEIGHT(i,j);

			if (h > max_h)
				max_h = h;

			if (h < min_h)
				min_h = h;
		}

	for (i=0;i<grid_w;i++)
		for (j=0;j<grid_h;j++)
			HEIGHT(i,j) -= min_h;
	

//	d_free(height_bitmap.bm_data);

	terrain_bm = terrain_bitmap;

	build_light_table();
}

namespace dcx {

namespace {

static void get_pnt(vms_vector &p,int i,int j)
{
	// added on 02/20/99 by adb to prevent overflow
	if (i >= grid_h) i = grid_h - 1;
	if (i == grid_h - 1 && j >= grid_w) j = grid_w - 1;
	// end additions by adb
	p.x = GRID_SCALE*i;
	p.z = GRID_SCALE*j;
	p.y = HEIGHT(i,j)*HEIGHT_SCALE;
}

constexpr vms_vector light{0x2e14,0xe8f5,0x5eb8};

static fix get_face_light(const vms_vector &p0,const vms_vector &p1,const vms_vector &p2)
{
	const auto norm = vm_vec_normal(p0,p1,p2);
	return -vm_vec_dot(norm,light);
}

static fix get_avg_light(int i,int j)
{
	vms_vector pp,p[6];
	fix sum;
	int f;

	get_pnt(pp,i,j);
	get_pnt(p[0],i-1,j);
	get_pnt(p[1],i,j-1);
	get_pnt(p[2],i+1,j-1);
	get_pnt(p[3],i+1,j);
	get_pnt(p[4],i,j+1);
	get_pnt(p[5],i-1,j+1);

	for (f=0,sum=0;f<6;f++)
		sum += get_face_light(pp,p[f],p[(f+1)%5]);

	return sum/6;
}

static void build_light_table()
{
	std::size_t alloc = grid_w*grid_h;
	light_array = std::make_unique<uint8_t[]>(alloc);
	memset(light_array.get(), 0, alloc);
	int i,j;
	fix l, l2, min_l = INT32_MAX, max_l = 0;
	for (i=1;i<grid_w;i++)
		for (j=1;j<grid_h;j++) {
			l = get_avg_light(i,j);

			if (l > max_l)
				max_l = l;

			if (l < min_l)
				min_l = l;
		}

	for (i=1;i<grid_w;i++)
		for (j=1;j<grid_h;j++) {

			l = get_avg_light(i,j);

			if (min_l == max_l) {
				LIGHT(i,j) = l>>8;
				continue;
			}

			l2 = fixdiv((l-min_l),(max_l-min_l));

			if (l2==f1_0)
				l2--;

			LIGHT(i,j) = l2>>8;

		}
}

}

void free_light_table()
{
	light_array.reset();
}

}
