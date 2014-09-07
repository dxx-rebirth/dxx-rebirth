#pragma once

#include "dxxsconf.h"
#include "compiler-array.h"

#include "segment.h"

static const unsigned MAX_RENDER_SEGS = 500;
static const unsigned OBJS_PER_SEG = 5;
static const unsigned N_EXTRA_OBJ_LISTS = 50;

struct rect
{
	short left,top,right,bot;
};

struct render_state_t
{
	unsigned N_render_segs;
	array<segnum_t, MAX_RENDER_SEGS> Render_list;
	array<short, MAX_RENDER_SEGS> Seg_depth;		//depth for each seg in Render_list
	array<bool, MAX_RENDER_SEGS> processed;		//whether each entry has been processed
	array<short, MAX_SEGMENTS> render_pos;	//where in render_list does this segment appear?
	array<rect, MAX_RENDER_SEGS> render_windows;
	struct render_obj_array0_t : array<objnum_t, OBJS_PER_SEG> {};
	struct render_obj_array1_t : array<render_obj_array0_t, MAX_RENDER_SEGS+N_EXTRA_OBJ_LISTS> {};
	render_obj_array1_t render_obj_list;
	render_state_t() :
		N_render_segs(0)
	{
	}
};

void set_dynamic_light(render_state_t &);
