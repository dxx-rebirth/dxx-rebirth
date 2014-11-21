#pragma once

#include <unordered_map>
#include <vector>
#include "dxxsconf.h"
#include "compiler-array.h"

#include "segment.h"

static const unsigned MAX_RENDER_SEGS = 500;

struct rect
{
	short left,top,right,bot;
};

struct render_state_t
{
	struct per_segment_state_t
	{
		struct distant_object
		{
			objnum_t objnum;
		};
		std::vector<distant_object> objects;
	};
	unsigned N_render_segs;
	array<segnum_t, MAX_RENDER_SEGS> Render_list;
	array<short, MAX_RENDER_SEGS> Seg_depth;		//depth for each seg in Render_list
	array<bool, MAX_RENDER_SEGS> processed;		//whether each entry has been processed
	array<short, MAX_SEGMENTS> render_pos;	//where in render_list does this segment appear?
	array<rect, MAX_RENDER_SEGS> render_windows;
	std::unordered_map<segnum_t, per_segment_state_t> render_seg_map;
	render_state_t() :
		N_render_segs(0)
	{
	}
};

void set_dynamic_light(render_state_t &);
