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
 * Header for polyobj.c, the polygon object code
 *
 */

#pragma once

#include "fwd-piggy.h"
#include "vecmat.h"
#include "3d.h"
#include <cstddef>
#include <cstdint>
#include <memory>
#include "d_array.h"
#include "inferno.h"
#include "pack.h"
#include "physfsx.h"

namespace dcx {

enum class polygon_model_index : uint8_t
{
	None = UINT8_MAX
};

enum class polygon_simpler_model_index : uint8_t
{
	None = 0,
};

}

#ifdef dsx
namespace dsx {
struct robot_info;
struct glow_values_t;
#if DXX_BUILD_DESCENT == 1
constexpr std::integral_constant<unsigned, 85> MAX_POLYGON_MODELS{};
#elif DXX_BUILD_DESCENT == 2
constexpr std::integral_constant<unsigned, 200> MAX_POLYGON_MODELS{};
#endif

//for each model, a model number for dying & dead variants, or -1 if none
extern enumerated_array<polygon_model_index, MAX_POLYGON_MODELS, polygon_model_index> Dying_modelnums, Dead_modelnums;
}
#endif

namespace dcx {

constexpr std::size_t MAX_SUBMODELS{10};
enum class submodel_index : uint8_t
{
	/* Valid values are [0 .. (MAX_SUBMODELS - 1)]. */
};

//used to describe a polygon model
struct polymodel : prohibit_void_ptr<polymodel>
{
	unsigned n_models;
	unsigned model_data_size;
	std::unique_ptr<uint8_t[]>   model_data;
	std::array<int, MAX_SUBMODELS> submodel_ptrs;
	std::array<vms_vector, MAX_SUBMODELS> submodel_offsets;
	std::array<vms_vector, MAX_SUBMODELS> submodel_norms;   // norm for sep plane
	std::array<vms_vector, MAX_SUBMODELS> submodel_pnts;    // point on sep plane
	enumerated_array<fix, MAX_SUBMODELS, submodel_index> submodel_rads;       // radius for each submodel
	std::array<ubyte, MAX_SUBMODELS> submodel_parents;    // what is parent for each submodel
	std::array<vms_vector, MAX_SUBMODELS> submodel_mins;
	std::array<vms_vector, MAX_SUBMODELS> submodel_maxs;
	vms_vector mins,maxs;                       // min,max for whole model
	fix     rad;
	ushort  first_texture;
	ubyte   n_textures;
	polygon_simpler_model_index simpler_model;                      // alternate model with less detail (0 if none, model_num+1 else)
	//vms_vector min,max;
};

class submodel_angles
{
	using array_type = const std::array<vms_angvec, MAX_SUBMODELS>;
	array_type *p{};
public:
	submodel_angles(std::nullptr_t) : p{nullptr} {}
	submodel_angles(array_type &a) : p{&a} {}
	explicit operator bool() const { return p != nullptr; }
	typename array_type::const_reference operator[](std::size_t i) const
	{
		array_type &a = *p;
		return a[i];
	}
};

struct d_level_shared_polygon_model_state
{
// how many polygon objects there are
	unsigned N_polygon_models;
};

void init_polygon_models(d_level_shared_polygon_model_state &);

/* Only defined if DXX_WORDS_NEED_ALIGNMENT, but always declared, so
 * that the header preprocesses to the same text regardless of the
 * setting.
 */
void align_polygon_model_data(polymodel *pm);

}
#ifdef dsx
namespace dsx {

/* Individual levels can customize the polygon models through robot overrides,
 * so this must be scoped to the level, not to the mission.
 */
struct d_level_shared_polygon_model_state : ::dcx::d_level_shared_polygon_model_state
{
	enumerated_array<polymodel, MAX_POLYGON_MODELS, polygon_model_index> Polygon_models;
	// array of names of currently-loaded models
	enumerated_array<char[FILENAME_LEN], MAX_POLYGON_MODELS, polygon_model_index> Pof_names;
#if DXX_BUILD_DESCENT == 2
	//the model number of the marker object
	polygon_model_index Marker_model_num = polygon_model_index::None;
	bool Exit_models_loaded;
#endif
};

// array of pointers to polygon objects
extern d_level_shared_polygon_model_state LevelSharedPolygonModelState;

void free_polygon_models(d_level_shared_polygon_model_state &LevelSharedPolygonModelState);

polygon_model_index load_polygon_model(const char *filename, int n_textures, int first_texture, robot_info *r);
}
#endif

namespace dcx {

class alternate_textures
{
	std::span<const bitmap_index> p{};
public:
	constexpr alternate_textures() = default;
	template <std::size_t N>
		constexpr alternate_textures(const std::array<bitmap_index, N> &a) :
			p{a}
	{
	}
	constexpr operator std::span<const bitmap_index>() const { return p; }
};

}

#ifdef dsx
namespace dsx {
// draw a polygon model
void draw_polygon_model(const enumerated_array<polymodel, MAX_POLYGON_MODELS, polygon_model_index> &, grs_canvas &, tmap_drawer_type tmap_drawer_ptr, const vms_vector &pos, const vms_matrix &orient, submodel_angles anim_angles, const polygon_model_index model_num, unsigned flags, g3s_lrgb light, const glow_values_t *glow_values, alternate_textures);
void draw_polygon_model(grs_canvas &, tmap_drawer_type tmap_drawer_ptr, const vms_vector &pos, const vms_matrix &orient, submodel_angles anim_angles, const polymodel &model_num, unsigned flags, g3s_lrgb light, const glow_values_t *glow_values, alternate_textures);
}
#endif

// draws the given model in the current canvas.  The distance is set to
// more-or-less fill the canvas.  Note that this routine actually renders
// into an off-screen canvas that it creates, then copies to the current
// canvas.
void draw_model_picture(grs_canvas &, const polymodel &mn, const vms_angvec &orient_angles);

#if defined(DXX_BUILD_DESCENT_I) || defined(DXX_BUILD_DESCENT_II)
#if DXX_BUILD_DESCENT == 1
#define MAX_POLYOBJ_TEXTURES 50
#elif DXX_BUILD_DESCENT == 2

#define MAX_POLYOBJ_TEXTURES 100
constexpr std::integral_constant<unsigned, 166> N_D2_POLYGON_MODELS{};
#endif
#endif

namespace dcx {

#ifdef DXX_BUILD_DESCENT
#if DXX_BUILD_DESCENT == 2
/* This function exists in both games and has the same implementation in
 * both, but is static in Descent 1.  Declare it in the header only for
 * Descent 2.
 */
// free up a model, getting rid of all its memory
void free_model(polymodel &po);
#endif
#endif

/*
 * reads a polymodel structure from a PHYSFS_File
 */
void polymodel_read(polymodel &pm, NamedPHYSFS_File fp);
}
#if 0
void polymodel_write(PHYSFS_File *fp, const polymodel &pm);
#endif

/*
 * routine which allocates, reads, and inits a polymodel's model_data
 */
#ifdef dsx
namespace dsx {
void polygon_model_data_read(polymodel *pm, PHYSFS_File *fp);
polygon_model_index build_polygon_model_index_from_untrusted(unsigned i);
#if DXX_USE_OGL
void ogl_cache_polymodel_textures(polygon_model_index model_num);
#endif
}
#endif
