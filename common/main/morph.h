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


#ifndef _MORPH_H
#define _MORPH_H

#include "object.h"

#ifdef __cplusplus
#include "dxxsconf.h"
#include "compiler-array.h"

#define MAX_VECS 5000

struct morph_data : prohibit_void_ptr<morph_data>
{
	object *obj;                                // object which is morphing
	int n_submodels_active;
	ubyte morph_save_control_type;
	ubyte morph_save_movement_type;
	physics_info morph_save_phys_info;
	object_signature_t Morph_sig;
	array<vms_vector, MAX_VECS> morph_vecs, morph_deltas;
	array<fix, MAX_VECS> morph_times;
	array<int, MAX_SUBMODELS> submodel_active,         // which submodels are active
		n_morphing_points,       // how many active points in each part
		submodel_startpoints;    // first point for each submodel
};

#define MAX_MORPH_OBJECTS 5

extern array<morph_data, MAX_MORPH_OBJECTS> morph_objects;

void morph_start(vobjptr_t obj);
void draw_morph_object(vobjptridx_t obj);

//process the morphing object for one frame
void do_morph_frame(vobjptr_t obj);

//called at the start of a level
void init_morphs();

morph_data *find_morph_data(vobjptr_t obj);

#endif

#endif /* _MORPH_H */
