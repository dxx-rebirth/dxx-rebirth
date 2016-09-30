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

#pragma once

#include "maths.h"
#include "3d.h"

#ifdef __cplusplus
#include "dxxsconf.h"
#include "dsx-ns.h"
#include "compiler-array.h"

namespace dcx {
class submodel_angles;

constexpr std::size_t MAX_POLYGON_VECS = 1000;
struct polygon_model_points : array<g3s_point, MAX_POLYGON_VECS> {};
}

#ifdef dsx
namespace dsx {
#if defined(DXX_BUILD_DESCENT_I)
constexpr std::size_t glow_array_size = 1;
#elif defined(DXX_BUILD_DESCENT_II)
constexpr std::size_t glow_array_size = 2;
#endif
struct glow_values_t : public array<fix, glow_array_size> {};

//Object functions:

//calls the object interpreter to render an object.  The object renderer
//is really a seperate pipeline. returns true if drew
void g3_draw_polygon_model(const uint8_t *model_ptr,grs_bitmap **model_bitmaps,submodel_angles anim_angles,g3s_lrgb light,const glow_values_t *glow_values, polygon_model_points &Interp_point_list);

//init code for bitmap models
int16_t g3_init_polygon_model(void *model_ptr);
}
#endif

//un-initialize, i.e., convert color entries back to RGB15
static inline void g3_uninit_polygon_model(void *model_ptr)
{
	(void)model_ptr;
}

#ifdef dsx
namespace dsx {
//alternate interpreter for morphing object
void g3_draw_morphing_model(const uint8_t *model_ptr,grs_bitmap **model_bitmaps,submodel_angles anim_angles,g3s_lrgb light,const vms_vector *new_points, polygon_model_points &Interp_point_list);

// check a polymodel for it's color and return it
int g3_poly_get_color(const uint8_t *model_ptr);
}
#endif

namespace dcx {
#if DXX_WORDS_BIGENDIAN
// routine to convert little to big endian in polygon model data
void swap_polygon_model_data(ubyte *data);
#else
static inline void swap_polygon_model_data(uint8_t *) {}
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
	const uint8_t *old_base; // where the offset sets off from (relative to beginning of model_data)
	uint8_t *new_base; // where the base is in the aligned structure
	short offset; // how much to add to base to get the address of the offset
	short correction; // how much the value of the offset must be shifted for alignment
};
#define MAX_CHUNKS 100 // increase if insufficent
/*
 * finds what chunks the data points to, adds them to the chunk_list, 
 * and returns the length of the current chunk
 */
int get_chunks(const uint8_t *data, uint8_t *new_data, chunk *list, int *no);
#endif //def WORDS_NEED_ALIGNMENT
}

#endif
