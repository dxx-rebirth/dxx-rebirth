/*
 * This file is part of the DXX-Rebirth project <http://www.dxx-rebirth.com/>.
 * It is copyright by its individual contributors, as recorded in the
 * project's Git history.  See COPYING.txt at the top level for license
 * terms and a link to the Git history.
 */

/*
 *
 * Polygon object interpreter
 *
 */

#include <stdlib.h>
#include "dxxerror.h"

#include "interp.h"
#include "common/3d/globvars.h"
#include "polyobj.h"
#include "gr.h"
#include "byteutil.h"
#include "u_mem.h"

static const unsigned OP_EOF = 0;   //eof
static const unsigned OP_DEFPOINTS = 1;   //defpoints
static const unsigned OP_FLATPOLY = 2;   //flat-shaded polygon
static const unsigned OP_TMAPPOLY = 3;   //texture-mapped polygon
static const unsigned OP_SORTNORM = 4;   //sort by normal
static const unsigned OP_RODBM = 5;   //rod bitmap
static const unsigned OP_SUBCALL = 6;   //call a subobject
static const unsigned OP_DEFP_START = 7;   //defpoints with start
static const unsigned OP_GLOW = 8;   //glow value for next poly

short highest_texture_num;
int g3d_interp_outline;


#define MAX_INTERP_COLORS 100

#define w(p)  (*((short *) (p)))
#define wp(p)  ((short *) (p))
#define fp(p)  ((fix *) (p))
#define vp(p)  ((vms_vector *) (p))

static void rotate_point_list(g3s_point *dest, const vms_vector *src, uint_fast32_t n)
{
	while (n--)
		g3_rotate_point(*dest++,*src++);
}

static const vms_angvec zero_angles = {0,0,0};

#ifdef WORDS_BIGENDIAN
static void short_swap(short *s)
{
	*s = SWAPSHORT(*s);
}

static void fix_swap(fix &f)
{
	f = (fix)SWAPINT((int)f);
}

static void fix_swap(fix *f)
{
	fix_swap(*f);
}

void vms_vector_swap(vms_vector &v)
{
	fix_swap(v.x);
	fix_swap(v.y);
	fix_swap(v.z);
}

void swap_polygon_model_data(ubyte *data)
{
	int i;
	short n;
	g3s_uvl *uvl_val;
	ubyte *p = data;

	short_swap(wp(p));

	while (w(p) != OP_EOF) {
		switch (w(p)) {
			case OP_DEFPOINTS:
				short_swap(wp(p + 2));
				n = w(p+2);
				for (i = 0; i < n; i++)
					vms_vector_swap(*vp((p + 4) + (i * sizeof(vms_vector))));
				p += n*sizeof(struct vms_vector) + 4;
				break;

			case OP_DEFP_START:
				short_swap(wp(p + 2));
				short_swap(wp(p + 4));
				n = w(p+2);
				for (i = 0; i < n; i++)
					vms_vector_swap(*vp((p + 8) + (i * sizeof(vms_vector))));
				p += n*sizeof(struct vms_vector) + 8;
				break;

			case OP_FLATPOLY:
				short_swap(wp(p+2));
				n = w(p+2);
				vms_vector_swap(*vp(p + 4));
				vms_vector_swap(*vp(p + 16));
				short_swap(wp(p+28));
				for (i=0; i < n; i++)
					short_swap(wp(p + 30 + (i * 2)));
				p += 30 + ((n&~1)+1)*2;
				break;

			case OP_TMAPPOLY:
				short_swap(wp(p+2));
				n = w(p+2);
				vms_vector_swap(*vp(p + 4));
				vms_vector_swap(*vp(p + 16));
				for (i=0;i<n;i++) {
					uvl_val = (g3s_uvl *)((p+30+((n&~1)+1)*2) + (i * sizeof(g3s_uvl)));
					fix_swap(&uvl_val->u);
					fix_swap(&uvl_val->v);
				}
				short_swap(wp(p+28));
				for (i=0;i<n;i++)
					short_swap(wp(p + 30 + (i * 2)));
				p += 30 + ((n&~1)+1)*2 + n*12;
				break;

			case OP_SORTNORM:
				vms_vector_swap(*vp(p + 4));
				vms_vector_swap(*vp(p + 16));
				short_swap(wp(p + 28));
				short_swap(wp(p + 30));
				swap_polygon_model_data(p + w(p+28));
				swap_polygon_model_data(p + w(p+30));
				p += 32;
				break;

			case OP_RODBM:
				vms_vector_swap(*vp(p + 20));
				vms_vector_swap(*vp(p + 4));
				short_swap(wp(p+2));
				fix_swap(fp(p + 16));
				fix_swap(fp(p + 32));
				p+=36;
				break;

			case OP_SUBCALL:
				short_swap(wp(p+2));
				vms_vector_swap(*vp(p+4));
				short_swap(wp(p+16));
				swap_polygon_model_data(p + w(p+16));
				p += 20;
				break;

			case OP_GLOW:
				short_swap(wp(p + 2));
				p += 4;
				break;

			default:
				Error("invalid polygon model\n"); //Int3();
		}
		short_swap(wp(p));
	}
}
#endif

#ifdef WORDS_NEED_ALIGNMENT
static void add_chunk(const uint8_t *old_base, uint8_t *new_base, int offset,
	       chunk *chunk_list, int *no_chunks)
{
	Assert(*no_chunks + 1 < MAX_CHUNKS); //increase MAX_CHUNKS if you get this
	chunk_list[*no_chunks].old_base = old_base;
	chunk_list[*no_chunks].new_base = new_base;
	chunk_list[*no_chunks].offset = offset;
	chunk_list[*no_chunks].correction = 0;
	(*no_chunks)++;
}

/*
 * finds what chunks the data points to, adds them to the chunk_list, 
 * and returns the length of the current chunk
 */
int get_chunks(const uint8_t *data, uint8_t *new_data, chunk *list, int *no)
{
	short n;
	auto p = data;

	while (INTEL_SHORT(w(p)) != OP_EOF) {
		switch (INTEL_SHORT(w(p))) {
		case OP_DEFPOINTS:
			n = INTEL_SHORT(w(p+2));
			p += n*sizeof(struct vms_vector) + 4;
			break;
		case OP_DEFP_START:
			n = INTEL_SHORT(w(p+2));
			p += n*sizeof(struct vms_vector) + 8;
			break;
		case OP_FLATPOLY:
			n = INTEL_SHORT(w(p+2));
			p += 30 + ((n&~1)+1)*2;
			break;
		case OP_TMAPPOLY:
			n = INTEL_SHORT(w(p+2));
			p += 30 + ((n&~1)+1)*2 + n*12;
			break;
		case OP_SORTNORM:
			add_chunk(p, p - data + new_data, 28, list, no);
			add_chunk(p, p - data + new_data, 30, list, no);
			p += 32;
			break;
		case OP_RODBM:
			p+=36;
			break;
		case OP_SUBCALL:
			add_chunk(p, p - data + new_data, 16, list, no);
			p+=20;
			break;
		case OP_GLOW:
			p += 4;
			break;
		default:
			Error("invalid polygon model\n");
		}
	}
	return p + 2 - data;
}
#endif //def WORDS_NEED_ALIGNMENT

// check a polymodel for it's color and return it
int g3_poly_get_color(ubyte *p)
{
	int color = 0;

	while (w(p) != OP_EOF)
		switch (w(p)) {
			case OP_DEFPOINTS:
				p += w(p+2)*sizeof(struct vms_vector) + 4;
				break;
			case OP_DEFP_START:
				p += w(p+2)*sizeof(struct vms_vector) + 8;
				break;
			case OP_FLATPOLY: {
				uint_fast32_t nv = w(p+2);
				Assert( nv < MAX_POINTS_PER_POLY );
				if (g3_check_normal_facing(*vp(p+4),*vp(p+16)) > 0) {
#if defined(DXX_BUILD_DESCENT_I)
					color = (w(p+28));
#elif defined(DXX_BUILD_DESCENT_II)
					color = gr_find_closest_color_15bpp(w(p + 28));
#endif
				}
				p += 30 + ((nv&~1)+1)*2;
				break;
			}
			case OP_TMAPPOLY: {
				int nv = w(p+2);
				p += 30 + ((nv&~1)+1)*2 + nv*12;
				break;
			}
			case OP_SORTNORM:
				if (g3_check_normal_facing(*vp(p+16),*vp(p+4)) > 0) //facing
					color = g3_poly_get_color(p+w(p+28));
				else //not facing
					color = g3_poly_get_color(p+w(p+30));
				p += 32;
				break;
			case OP_RODBM:
				p+=36;
				break;
			case OP_SUBCALL:
#if defined(DXX_BUILD_DESCENT_I)
				color = g3_poly_get_color(p+w(p+16));
#endif
				p += 20;
				break;
			case OP_GLOW:
				p += 4;
				break;

			default:
#if defined(DXX_BUILD_DESCENT_I)
				;
#elif defined(DXX_BUILD_DESCENT_II)
				Error("invalid polygon model\n");
#endif
		}
	return color;
}

//calls the object interpreter to render an object.  The object renderer
//is really a seperate pipeline. returns true if drew
void g3_draw_polygon_model(ubyte *p,grs_bitmap **model_bitmaps,const submodel_angles anim_angles,g3s_lrgb model_light,glow_values_t *glow_values, polygon_model_points &Interp_point_list)
{
	unsigned glow_num = ~0;		//glow off by default

	while (w(p) != OP_EOF)

		switch (w(p)) {

			case OP_DEFPOINTS: {
				int n = w(p+2);

				rotate_point_list(&Interp_point_list[0],vp(p+4),n);
				p += n*sizeof(struct vms_vector) + 4;

				break;
			}

			case OP_DEFP_START: {
				int n = w(p+2);
				int s = w(p+4);

				rotate_point_list(&Interp_point_list[s],vp(p+8),n);
				p += n*sizeof(struct vms_vector) + 8;

				break;
			}

			case OP_FLATPOLY: {
				uint_fast32_t nv = w(p+2);

				Assert( nv < MAX_POINTS_PER_POLY );
				if (g3_check_normal_facing(*vp(p+4),*vp(p+16)) > 0) {
					array<cg3s_point *, MAX_POINTS_PER_POLY> point_list;
					for (uint_fast32_t i = 0;i < nv;i++)
						point_list[i] = &Interp_point_list[wp(p+30)[i]];

#if defined(DXX_BUILD_DESCENT_II)
					if (!glow_values || !(glow_num < glow_values->size()) || (*glow_values)[glow_num] != -3)
#endif
					{
//					DPH: Now we treat this color as 15bpp
#if defined(DXX_BUILD_DESCENT_I)
					gr_setcolor(w(p+28));
#elif defined(DXX_BUILD_DESCENT_II)
					if (glow_values && glow_num < glow_values->size() && (*glow_values)[glow_num] == -2)
						gr_setcolor(255);
					else
					{
						gr_setcolor(gr_find_closest_color_15bpp(w(p + 28)));
					}
#endif
						g3_draw_poly(nv,point_list);
					}
				}

				p += 30 + ((nv&~1)+1)*2;
					
				break;
			}

			case OP_TMAPPOLY: {
				uint_fast32_t nv = w(p+2);

				Assert( nv < MAX_POINTS_PER_POLY );
				if (g3_check_normal_facing(*vp(p+4),*vp(p+16)) > 0) {
					g3s_lrgb light;

					//calculate light from surface normal
					if (!glow_values || !(glow_num < glow_values->size())) //no glow
					{
						light.r = light.g = light.b = -vm_vec_dot(View_matrix.fvec,*vp(p+16));
						light.r = f1_0/4 + (light.r*3)/4;
						light.r = fixmul(light.r,model_light.r);
						light.g = f1_0/4 + (light.g*3)/4;
						light.g = fixmul(light.g,model_light.g);
						light.b = f1_0/4 + (light.b*3)/4;
						light.b = fixmul(light.b,model_light.b);
					}
					else //yes glow
					{
						light.r = light.g = light.b = (*glow_values)[glow_num];
						glow_num = -1;
					}

					//now poke light into l values
					array<g3s_uvl, MAX_POINTS_PER_POLY> uvl_list;
					array<g3s_lrgb, MAX_POINTS_PER_POLY> lrgb_list;
					for (uint_fast32_t i = 0; i < nv; i++)
					{
						lrgb_list[i] = light;
						uvl_list[i] = ((g3s_uvl *) (p+30+((nv&~1)+1)*2))[i];
						uvl_list[i].l = (light.r+light.g+light.b)/3;
					}

					array<cg3s_point *, MAX_POINTS_PER_POLY> point_list;
					for (uint_fast32_t i = 0; i < nv; i++)
						point_list[i] = &Interp_point_list[wp(p+30)[i]];

					g3_draw_tmap(nv,point_list,uvl_list,lrgb_list,*model_bitmaps[w(p+28)]);
				}

				p += 30 + ((nv&~1)+1)*2 + nv*12;
					
				break;
			}

			case OP_SORTNORM:

				if (g3_check_normal_facing(*vp(p+16),*vp(p+4)) > 0) {		//facing

					//draw back then front

					g3_draw_polygon_model(p+w(p+30),model_bitmaps,anim_angles,model_light,glow_values, Interp_point_list);
					g3_draw_polygon_model(p+w(p+28),model_bitmaps,anim_angles,model_light,glow_values, Interp_point_list);

				}
				else {			//not facing.  draw front then back

					g3_draw_polygon_model(p+w(p+28),model_bitmaps,anim_angles,model_light,glow_values, Interp_point_list);
					g3_draw_polygon_model(p+w(p+30),model_bitmaps,anim_angles,model_light,glow_values, Interp_point_list);
				}

				p += 32;

				break;


			case OP_RODBM: {
				g3s_lrgb rodbm_light = { f1_0, f1_0, f1_0 };

				const auto rod_bot_p = g3_rotate_point(*vp(p+20));
				const auto rod_top_p = g3_rotate_point(*vp(p+4));

				g3_draw_rod_tmap(*model_bitmaps[w(p+2)],rod_bot_p,w(p+16),rod_top_p,w(p+32),rodbm_light);

				p+=36;
				break;
			}

			case OP_SUBCALL: {
				const vms_angvec *a;

				if (anim_angles)
					a = &anim_angles[w(p+2)];
				else
					a = &zero_angles;

				g3_start_instance_angles(*vp(p+4),a);

				g3_draw_polygon_model(p+w(p+16),model_bitmaps,anim_angles,model_light,glow_values, Interp_point_list);

				g3_done_instance();

				p += 20;

				break;

			}

			case OP_GLOW:

				if (glow_values)
					glow_num = w(p+2);
				p += 4;
				break;

			default:
#if defined(DXX_BUILD_DESCENT_I)
			;
#elif defined(DXX_BUILD_DESCENT_II)
				Error("invalid polygon model\n");
#endif
		}
}

#ifndef NDEBUG
static int nest_count;
#endif

//alternate interpreter for morphing object
void g3_draw_morphing_model(ubyte *p,grs_bitmap **model_bitmaps,const submodel_angles anim_angles,g3s_lrgb model_light,vms_vector *new_points, polygon_model_points &Interp_point_list)
{
	glow_values_t *glow_values = NULL;

	unsigned glow_num = ~0;		//glow off by default

	while (w(p) != OP_EOF)

		switch (w(p)) {

			case OP_DEFPOINTS: {
				int n = w(p+2);

				rotate_point_list(&Interp_point_list[0],new_points,n);
				p += n*sizeof(struct vms_vector) + 4;

				break;
			}

			case OP_DEFP_START: {
				int n = w(p+2);
				int s = w(p+4);

				rotate_point_list(&Interp_point_list[s],new_points,n);
				p += n*sizeof(struct vms_vector) + 8;

				break;
			}

			case OP_FLATPOLY: {
				int nv = w(p+2);
				int i,ntris;

#if defined(DXX_BUILD_DESCENT_I)
				gr_setcolor(55/*w(p+28)*/);
#elif defined(DXX_BUILD_DESCENT_II)
				gr_setcolor(w(p+28));
#endif
				
				array<cg3s_point *, 3> point_list;
				for (i=0;i<2;i++)
					point_list[i] = &Interp_point_list[wp(p+30)[i]];

				for (ntris=nv-2;ntris;ntris--) {
					point_list[2] = &Interp_point_list[wp(p+30)[i++]];
					g3_check_and_draw_poly(point_list);
					point_list[1] = point_list[2];
				}

				p += 30 + ((nv&~1)+1)*2;
					
				break;
			}

			case OP_TMAPPOLY: {
				int nv = w(p+2);
				g3s_lrgb light;
				int i,ntris;

				//calculate light from surface normal
				if (!glow_values || !(glow_num < glow_values->size())) //no glow
				{
					light.r = light.g = light.b = -vm_vec_dot(View_matrix.fvec,*vp(p+16));
					light.r = f1_0/4 + (light.r*3)/4;
					light.r = fixmul(light.r,model_light.r);
					light.g = f1_0/4 + (light.g*3)/4;
					light.g = fixmul(light.g,model_light.g);
					light.b = f1_0/4 + (light.b*3)/4;
					light.b = fixmul(light.b,model_light.b);
				}
				else //yes glow
				{
					light.r = light.g = light.b = (*glow_values)[glow_num];
					glow_num = -1;
				}

				//now poke light into l values

				array<g3s_uvl, 3> uvl_list;
				array<g3s_lrgb, 3> lrgb_list;
				for (unsigned i = 0; i < 3; ++i)
					uvl_list[i] = ((g3s_uvl *) (p+30+((nv&~1)+1)*2))[i];
				lrgb_list.fill(light);

				array<cg3s_point *, 3> point_list;
				for (i=0;i<2;i++) {
					point_list[i] = &Interp_point_list[wp(p+30)[i]];
				}

				for (ntris=nv-2;ntris;ntris--) {
					point_list[2] = &Interp_point_list[wp(p+30)[i]];
					i++;
					g3_check_and_draw_tmap(point_list,uvl_list,lrgb_list,*model_bitmaps[w(p+28)]);
					point_list[1] = point_list[2];
				}

				p += 30 + ((nv&~1)+1)*2 + nv*12;

				break;
			}

			case OP_SORTNORM:

				if (g3_check_normal_facing(*vp(p+16),*vp(p+4)) > 0) {		//facing

					//draw back then front

					g3_draw_morphing_model(p+w(p+30),model_bitmaps,anim_angles,model_light,new_points, Interp_point_list);
					g3_draw_morphing_model(p+w(p+28),model_bitmaps,anim_angles,model_light,new_points, Interp_point_list);

				}
				else {			//not facing.  draw front then back

					g3_draw_morphing_model(p+w(p+28),model_bitmaps,anim_angles,model_light,new_points, Interp_point_list);
					g3_draw_morphing_model(p+w(p+30),model_bitmaps,anim_angles,model_light,new_points, Interp_point_list);
				}

				p += 32;

				break;


			case OP_RODBM: {
				g3s_lrgb rodbm_light = { f1_0, f1_0, f1_0 };

				const auto rod_bot_p = g3_rotate_point(*vp(p+20));
				const auto rod_top_p = g3_rotate_point(*vp(p+4));

				g3_draw_rod_tmap(*model_bitmaps[w(p+2)],rod_bot_p,w(p+16),rod_top_p,w(p+32),rodbm_light);

				p+=36;
				break;
			}

			case OP_SUBCALL: {
				const vms_angvec *a;

				if (anim_angles)
					a = &anim_angles[w(p+2)];
				else
					a = &zero_angles;

				g3_start_instance_angles(*vp(p+4),a);

				g3_draw_polygon_model(p+w(p+16),model_bitmaps,anim_angles,model_light,glow_values, Interp_point_list);

				g3_done_instance();

				p += 20;

				break;

			}

			case OP_GLOW:

				if (glow_values)
					glow_num = w(p+2);
				p += 4;
				break;
		}
}

static void init_model_sub(ubyte *p)
{
	Assert(++nest_count < 1000);

	while (w(p) != OP_EOF) {

		switch (w(p)) {

			case OP_DEFPOINTS: {
				int n = w(p+2);
				p += n*sizeof(struct vms_vector) + 4;
				break;
			}

			case OP_DEFP_START: {
				int n = w(p+2);
				p += n*sizeof(struct vms_vector) + 8;
				break;
			}

			case OP_FLATPOLY: {
				int nv = w(p+2);

				Assert(nv > 2);		//must have 3 or more points

#if defined(DXX_BUILD_DESCENT_I)
				*wp(p+28) = (short)gr_find_closest_color_15bpp(w(p+28));
#endif

				p += 30 + ((nv&~1)+1)*2;
					
				break;
			}

			case OP_TMAPPOLY: {
				int nv = w(p+2);

				Assert(nv > 2);		//must have 3 or more points

				if (w(p+28) > highest_texture_num)
					highest_texture_num = w(p+28);

				p += 30 + ((nv&~1)+1)*2 + nv*12;
					
				break;
			}

			case OP_SORTNORM:

				init_model_sub(p+w(p+28));
				init_model_sub(p+w(p+30));
				p += 32;

				break;


			case OP_RODBM:
				p += 36;
				break;


			case OP_SUBCALL: {
				init_model_sub(p+w(p+16));
				p += 20;
				break;

			}

			case OP_GLOW:
				p += 4;
				break;
#if defined(DXX_BUILD_DESCENT_II)
			default:
				Error("invalid polygon model\n");
#endif
		}
	}
}

//init code for bitmap models
void g3_init_polygon_model(void *model_ptr)
{
	#ifndef NDEBUG
	nest_count = 0;
	#endif

	highest_texture_num = -1;

	init_model_sub((ubyte *) model_ptr);
}
