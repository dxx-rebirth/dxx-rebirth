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

// array of names of currently-loaded models
extern array<char[13], MAX_POLYGON_MODELS> Pof_names;

//for each model, a model number for dying & dead variants, or -1 if none
extern array<int, MAX_POLYGON_MODELS> Dying_modelnums, Dead_modelnums;
}
#endif

namespace dcx {
constexpr std::integral_constant<unsigned, 10> MAX_SUBMODELS{};

//used to describe a polygon model
struct polymodel : prohibit_void_ptr<polymodel>
{
	unsigned n_models;
	int     model_data_size;
	std::unique_ptr<ubyte[]>   model_data;
	array<int, MAX_SUBMODELS> submodel_ptrs;
	array<vms_vector, MAX_SUBMODELS> submodel_offsets;
	array<vms_vector, MAX_SUBMODELS> submodel_norms;   // norm for sep plane
	array<vms_vector, MAX_SUBMODELS> submodel_pnts;    // point on sep plane
	array<fix, MAX_SUBMODELS> submodel_rads;       // radius for each submodel
	array<ubyte, MAX_SUBMODELS> submodel_parents;    // what is parent for each submodel
	array<vms_vector, MAX_SUBMODELS> submodel_mins;
	array<vms_vector, MAX_SUBMODELS> submodel_maxs;
	vms_vector mins,maxs;                       // min,max for whole model
	fix     rad;
	ushort  first_texture;
	ubyte   n_textures;
	ubyte   simpler_model;                      // alternate model with less detail (0 if none, model_num+1 else)
	//vms_vector min,max;
};

class submodel_angles
{
	typedef const array<vms_angvec, MAX_SUBMODELS> array_type;
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

// how many polygon objects there are
extern unsigned N_polygon_models;
void init_polygon_models();

}
#ifdef dsx
namespace dsx {
// array of pointers to polygon objects
extern array<polymodel, MAX_POLYGON_MODELS> Polygon_models;

void free_polygon_models();

int load_polygon_model(const char *filename,int n_textures,int first_texture,robot_info *r);
}
#endif

namespace dcx {

class alternate_textures
{
	const bitmap_index *p;
public:
	alternate_textures() : p(nullptr) {}
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
void draw_polygon_model(grs_canvas &, const vms_vector &pos, const vms_matrix &orient, submodel_angles anim_angles, unsigned model_num, unsigned flags, g3s_lrgb light, const glow_values_t *glow_values, alternate_textures);
}
#endif

// draws the given model in the current canvas.  The distance is set to
// more-or-less fill the canvas.  Note that this routine actually renders
// into an off-screen canvas that it creates, then copies to the current
// canvas.
void draw_model_picture(grs_canvas &, uint_fast32_t mn, const vms_angvec &orient_angles);

#if defined(DXX_BUILD_DESCENT_I) || defined(DXX_BUILD_DESCENT_II)
#if defined(DXX_BUILD_DESCENT_I)
#define MAX_POLYOBJ_TEXTURES 50
#elif defined(DXX_BUILD_DESCENT_II)
// free up a model, getting rid of all its memory
void free_model(polymodel &po);

#define MAX_POLYOBJ_TEXTURES 100
constexpr std::integral_constant<unsigned, 166> N_D2_POLYGON_MODELS{};
#endif
extern array<grs_bitmap *, MAX_POLYOBJ_TEXTURES> texture_list;
#endif

namespace dcx {
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
