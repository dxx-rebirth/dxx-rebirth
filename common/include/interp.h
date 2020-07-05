/*
 * This file is part of the DXX-Rebirth project <https://www.dxx-rebirth.com/>.
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
#include <array>

namespace dcx {
class submodel_angles;

constexpr std::integral_constant<std::size_t, 1000> MAX_POLYGON_VECS{};
struct polygon_model_points : std::array<g3s_point, MAX_POLYGON_VECS> {};
}

#ifdef dsx
namespace dsx {
#if defined(DXX_BUILD_DESCENT_I)
#define glow_array_size	1
#elif defined(DXX_BUILD_DESCENT_II)
#define glow_array_size	2
#endif
struct glow_values_t : public std::array<fix, glow_array_size> {};
#undef glow_array_size

//Object functions:

//calls the object interpreter to render an object.  The object renderer
//is really a seperate pipeline. returns true if drew
void g3_draw_polygon_model(grs_bitmap *const *model_bitmaps, polygon_model_points &Interp_point_list, grs_canvas &, submodel_angles anim_angles, g3s_lrgb model_light, const glow_values_t *glow_values, const uint8_t *p);

//init code for bitmap models
int16_t g3_init_polygon_model(uint8_t *model_ptr, std::size_t model_size);
#if defined(DXX_BUILD_DESCENT_I)
void g3_validate_polygon_model(uint8_t *model_ptr, std::size_t model_size);
#endif
}
#endif

#ifdef dsx
namespace dsx {
//alternate interpreter for morphing object
void g3_draw_morphing_model(grs_canvas &, const uint8_t *model_ptr, grs_bitmap *const *model_bitmaps, submodel_angles anim_angles, g3s_lrgb light, const vms_vector *new_points, polygon_model_points &Interp_point_list);

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
}

#endif
