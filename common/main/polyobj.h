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

#include "vecmat.h"
#include "3d.h"

struct bitmap_index;

#ifdef __cplusplus
#include <cstddef>
#include <memory>
#include <physfs.h>
#include "inferno.h"
#include "pack.h"

#ifdef dsx
namespace dsx {
struct robot_info;
struct glow_values_t;
#if defined(DXX_BUILD_DESCENT_I)
constexpr std::integral_constant<unsigned, 85> MAX_POLYGON_MODELS{};
#elif defined(DXX_BUILD_DESCENT_II)
constexpr std::integral_constant<unsigned, 200> MAX_POLYGON_MODELS{};
#endif

//for each model, a model number for dying & dead variants, or -1 if none
extern std::array<int, MAX_POLYGON_MODELS> Dying_modelnums, Dead_modelnums;
}
#endif

namespace dcx {
constexpr std::integral_constant<unsigned, 10> MAX_SUBMODELS{};

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
	std::array<fix, MAX_SUBMODELS> submodel_rads;       // radius for each submodel
	std::array<ubyte, MAX_SUBMODELS> submodel_parents;    // what is parent for each submodel
	std::array<vms_vector, MAX_SUBMODELS> submodel_mins;
	std::array<vms_vector, MAX_SUBMODELS> submodel_maxs;
	vms_vector mins,maxs;                       // min,max for whole model
	fix     rad;
	ushort  first_texture;
	ubyte   n_textures;
	ubyte   simpler_model;                      // alternate model with less detail (0 if none, model_num+1 else)
	//vms_vector min,max;
};

class submodel_angles
{
	using array_type = const std::array<vms_angvec, MAX_SUBMODELS>;
	array_type *p;
public:
	submodel_angles(std::nullptr_t) : p(nullptr) {}
	submodel_angles(array_type &a) : p(&a) {}
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
	std::array<polymodel, MAX_POLYGON_MODELS> Polygon_models;
	// array of names of currently-loaded models
	std::array<char[FILENAME_LEN], MAX_POLYGON_MODELS> Pof_names;
#if defined(DXX_BUILD_DESCENT_II)
	//the model number of the marker object
	uint32_t Marker_model_num = UINT32_MAX;
	bool Exit_models_loaded;
#endif
};

// array of pointers to polygon objects
extern d_level_shared_polygon_model_state LevelSharedPolygonModelState;

void free_polygon_models(d_level_shared_polygon_model_state &LevelSharedPolygonModelState);

int load_polygon_model(const char *filename,int n_textures,int first_texture,robot_info *r);
}
#endif

namespace dcx {

class alternate_textures
{
	const bitmap_index *p = nullptr;
public:
	alternate_textures() = default;
	alternate_textures(std::nullptr_t) : p(nullptr) {}
	template <std::size_t N>
		alternate_textures(const std::array<bitmap_index, N> &a) : p(a.data())
	{
	}
	operator const bitmap_index *() const { return p; }
};

}

#ifdef dsx
namespace dsx {
// draw a polygon model
void draw_polygon_model(const std::array<polymodel, MAX_POLYGON_MODELS> &, grs_canvas &, const vms_vector &pos, const vms_matrix &orient, submodel_angles anim_angles, const unsigned model_num, unsigned flags, g3s_lrgb light, const glow_values_t *glow_values, alternate_textures);
void draw_polygon_model(grs_canvas &, const vms_vector &pos, const vms_matrix &orient, submodel_angles anim_angles, const polymodel &model_num, unsigned flags, g3s_lrgb light, const glow_values_t *glow_values, alternate_textures);
}
#endif

// draws the given model in the current canvas.  The distance is set to
// more-or-less fill the canvas.  Note that this routine actually renders
// into an off-screen canvas that it creates, then copies to the current
// canvas.
void draw_model_picture(grs_canvas &, const polymodel &mn, const vms_angvec &orient_angles);

#if defined(DXX_BUILD_DESCENT_I) || defined(DXX_BUILD_DESCENT_II)
#if defined(DXX_BUILD_DESCENT_I)
#define MAX_POLYOBJ_TEXTURES 50
#elif defined(DXX_BUILD_DESCENT_II)

#define MAX_POLYOBJ_TEXTURES 100
constexpr std::integral_constant<unsigned, 166> N_D2_POLYGON_MODELS{};
#endif
#endif

namespace dcx {

#if defined(DXX_BUILD_DESCENT_II)
/* This function exists in both games and has the same implementation in
 * both, but is static in Descent 1.  Declare it in the header only for
 * Descent 2.
 */
// free up a model, getting rid of all its memory
void free_model(polymodel &po);
#endif

/*
 * reads a polymodel structure from a PHYSFS_File
 */
extern void polymodel_read(polymodel *pm, PHYSFS_File *fp);
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

}
#endif
#endif
