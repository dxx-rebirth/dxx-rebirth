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
 * Header for morph.c
 *
 */

#pragma once

#ifdef __cplusplus
#include <cstdint>
#include "dxxsconf.h"
#include "vecmat.h"
#include "pack.h"
#include "polyobj.h"
#include "dsx-ns.h"
#include "compiler-array.h"

#ifdef dsx
#include "object.h"
#include <memory>

namespace dcx {

struct morph_data : prohibit_void_ptr<morph_data>
{
	enum
	{
		MAX_VECS = 5000,
	};
	enum class submodel_state : uint8_t
	{
		invisible,
		animating,
		visible,
	};
	object_base *const obj;                      // object which is morphing
	const object_signature_t Morph_sig;
	uint8_t morph_save_control_type;
	uint8_t morph_save_movement_type;
	uint8_t n_submodels_active;
	physics_info morph_save_phys_info;
	array<submodel_state, MAX_SUBMODELS> submodel_active;         // which submodels are active
	array<vms_vector, MAX_VECS> morph_vecs, morph_deltas;
	array<fix, MAX_VECS> morph_times;
	array<int, MAX_SUBMODELS>
		n_morphing_points,       // how many active points in each part
		submodel_startpoints;    // first point for each submodel
	explicit morph_data(object_base &o) :
		obj(&o), Morph_sig(o.signature)
	{
	}
};

struct d_level_unique_morph_object_state
{
	array<std::unique_ptr<morph_data>, 5> morph_objects;
};

extern d_level_unique_morph_object_state LevelUniqueMorphObjectState;

std::unique_ptr<morph_data> *find_morph_data(object_base &obj);
}

void morph_start(vmobjptr_t obj);

//process the morphing object for one frame
void do_morph_frame(object &obj);

//called at the start of a level
void init_morphs();
#endif

#endif
