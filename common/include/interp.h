/*
 * This file is part of the DXX-Rebirth project <http://www.dxx-rebirth.com/>.
 * It is copyright by its individual contributors, as recorded in the
 * project's Git history.  See COPYING.txt at the top level for license
 * terms and a link to the Git history.
 */
/*
 *
 * took out functions declarations from include/3d.h
 * which are implemented in 3d/interp.c
 *
 */

#ifndef _INTERP_H
#define _INTERP_H

#include "maths.h"
#include "gr.h"
#include "3d.h"

#ifdef __cplusplus
#include "dxxsconf.h"
#include "compiler-array.h"

#if defined(DXX_BUILD_DESCENT_I) || defined(DXX_BUILD_DESCENT_II)
#if defined(DXX_BUILD_DESCENT_I)
static const size_t glow_array_size = 1;
#elif defined(DXX_BUILD_DESCENT_II)
static const size_t glow_array_size = 2;
#endif
struct glow_values_t : public array<fix, glow_array_size> {};
#else
struct glow_values_t;
#endif

//Object functions:

//gives the interpreter an array of points to use
void g3_set_interp_points(g3s_point *pointlist);

//calls the object interpreter to render an object.  The object renderer
//is really a seperate pipeline. returns true if drew
bool g3_draw_polygon_model(ubyte *model_ptr,grs_bitmap **model_bitmaps,vms_angvec *anim_angles,g3s_lrgb light,glow_values_t *glow_values);

//init code for bitmap models
void g3_init_polygon_model(void *model_ptr);

//un-initialize, i.e., convert color entries back to RGB15
static inline void g3_uninit_polygon_model(void *model_ptr)
{
	(void)model_ptr;
}

//alternate interpreter for morphing object
bool g3_draw_morphing_model(ubyte *model_ptr,grs_bitmap **model_bitmaps,vms_angvec *anim_angles,g3s_lrgb light,vms_vector *new_points);

//this remaps the 15bpp colors for the models into a new palette.  It should
//be called whenever the palette changes
void g3_remap_interp_colors(void);

// check a polymodel for it's color and return it
int g3_poly_get_color(ubyte *model_ptr);

#ifdef WORDS_BIGENDIAN
// routine to convert little to big endian in polygon model data
void swap_polygon_model_data(ubyte *data);
//routines to convert little to big endian in vectors
void vms_vector_swap(vms_vector *v);
void vms_angvec_swap(vms_angvec *v);
#endif

#ifdef WORDS_NEED_ALIGNMENT
/*
 * A chunk struct (as used for alignment) contains all relevant data
 * concerning a piece of data that may need to be aligned.
 * To align it, we need to copy it to an aligned position,
 * and update all pointers  to it.
 * (Those pointers are actually offsets
 * relative to start of model_data) to it.
 */
struct chunk
{
	ubyte *old_base; // where the offset sets off from (relative to beginning of model_data)
	ubyte *new_base; // where the base is in the aligned structure
	short offset; // how much to add to base to get the address of the offset
	short correction; // how much the value of the offset must be shifted for alignment
};
#define MAX_CHUNKS 100 // increase if insufficent
/*
 * finds what chunks the data points to, adds them to the chunk_list, 
 * and returns the length of the current chunk
 */
int get_chunks(ubyte *data, ubyte *new_data, chunk *list, int *no);
#endif //def WORDS_NEED_ALIGNMENT

#endif

#endif //_INTERP_H
