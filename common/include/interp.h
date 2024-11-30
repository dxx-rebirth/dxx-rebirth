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

#include <span>
#include "maths.h"
#include "3d.h"

#include "dxxsconf.h"
#include "dsx-ns.h"
#include <array>

namespace dcx {
class submodel_angles;

struct polygon_model_points : std::array<g3s_point, 1000> {};
}

#ifdef DXX_BUILD_DESCENT
namespace dsx {
#if DXX_BUILD_DESCENT == 1
#define glow_array_size	1
#elif DXX_BUILD_DESCENT == 2
#define glow_array_size	2
#endif
struct glow_values_t : public std::array<fix, glow_array_size> {};
#undef glow_array_size

//Object functions:

//calls the object interpreter to render an object.  The object renderer
//is really a seperate pipeline. returns true if drew
void g3_draw_polygon_model(grs_bitmap *const *model_bitmaps, polygon_model_points &Interp_point_list, grs_canvas &, tmap_drawer_type tmap_drawer_ptr, submodel_angles anim_angles, g3s_lrgb model_light, const glow_values_t *glow_values, const uint8_t *p);

//init code for bitmap models
int16_t g3_init_polygon_model(std::span<uint8_t> model);
#if DXX_BUILD_DESCENT == 1
void g3_validate_polygon_model(std::span<uint8_t> model);
#endif
}
#endif

#ifdef DXX_BUILD_DESCENT
namespace dsx {
//alternate interpreter for morphing object
void g3_draw_morphing_model(grs_canvas &, tmap_drawer_type tmap_drawer_ptr, const uint8_t *model_ptr, grs_bitmap *const *model_bitmaps, submodel_angles anim_angles, g3s_lrgb light, const vms_vector *new_points, polygon_model_points &Interp_point_list);

// check a polymodel for it's color and return it
int g3_poly_get_color(const uint8_t *model_ptr);
}
#endif

namespace dcx {
// routine to convert little to big endian in polygon model data
void swap_polygon_model_data(uint8_t *data);
}
