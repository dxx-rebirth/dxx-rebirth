/*
 *
 * took out functions declarations from include/3d.h
 * which are implemented in 3d/interp.c
 *
 */

#ifndef _INTERP_H
#define _INTERP_H

#include "fix.h"
#include "gr.h"
#include "3d.h"

//Object functions:

//gives the interpreter an array of points to use
void g3_set_interp_points(g3s_point *pointlist);

//calls the object interpreter to render an object.  The object renderer
//is really a seperate pipeline. returns true if drew
bool g3_draw_polygon_model(ubyte *model_ptr,grs_bitmap **model_bitmaps,vms_angvec *anim_angles,g3s_lrgb light,fix *glow_values);

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
typedef struct chunk {
	ubyte *old_base; // where the offset sets off from (relative to beginning of model_data)
	ubyte *new_base; // where the base is in the aligned structure
	short offset; // how much to add to base to get the address of the offset
	short correction; // how much the value of the offset must be shifted for alignment
} chunk;
#define MAX_CHUNKS 100 // increase if insufficent
/*
 * finds what chunks the data points to, adds them to the chunk_list, 
 * and returns the length of the current chunk
 */
int get_chunks(ubyte *data, ubyte *new_data, chunk *list, int *no);
#endif //def WORDS_NEED_ALIGNMENT

#endif //_INTERP_H
