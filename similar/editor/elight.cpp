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
COPYRIGHT 1993-1998 PARALLAX SOFTWARE CORPORATION.  ALL RIGHTS RESERVED.
*/

/*
 *
 * Editor lighting functions.
 *
 */

#include <stdio.h>
#include "inferno.h"
#include "segment.h"
#include "editor.h"
#include "editor/esegment.h"
#include "seguvs.h"
#include "wall.h"
#include "textures.h"
#include "maths.h"
#include "dxxerror.h"
#include "kdefs.h"
#include "gameseg.h"
#include "texmap.h"

#include "compiler-range_for.h"

// -----------------------------------------------------------------------------
//	Return light intensity at an instance of a vertex on a side in a segment.
static fix get_light_intensity(const unique_side &s, const uint_fast32_t vert)
{
	Assert(vert <= 3);
	return s.uvls[vert].l;
}

static fix get_light_intensity(const vcsegptr_t segp, const uint_fast32_t sidenum, const uint_fast32_t vert)
{
	Assert(sidenum <= MAX_SIDES_PER_SEGMENT);
	return get_light_intensity(segp->sides[sidenum], vert);
}

static fix clamp_light_intensity(const fix intensity)
{
	if (intensity < MIN_LIGHTING_VALUE)
		return MIN_LIGHTING_VALUE;
	if (intensity > MAX_LIGHTING_VALUE)
		return MAX_LIGHTING_VALUE;
	return intensity;
}

// -----------------------------------------------------------------------------
//	Set light intensity at a vertex, saturating in .5 to 15.5
static void set_light_intensity(unique_side &s, const uint_fast32_t vert, const fix intensity)
{
	Assert(vert <= 3);
	s.uvls[vert].l = clamp_light_intensity(intensity);
	Update_flags |= UF_WORLD_CHANGED;
}

static void set_light_intensity(const vmsegptr_t segp, const uint_fast32_t sidenum, const uint_fast32_t vert, const fix intensity)
{
	Assert(sidenum <= MAX_SIDES_PER_SEGMENT);
	set_light_intensity(segp->sides[sidenum], vert, intensity);
}

// -----------------------------------------------------------------------------
//	Add light intensity to a vertex, saturating in .5 to 15.5
static void add_light_intensity_all_verts(unique_side &s, const fix intensity)
{
	range_for (auto &u, s.uvls)
		u.l = clamp_light_intensity(u.l + intensity);
	Update_flags |= UF_WORLD_CHANGED;
}

// -----------------------------------------------------------------------------
//	Recursively apply light to segments.
//	If current side is a wall, apply light there.
//	If not a wall, apply light to child through that wall.
//	Notes:
//		It is possible to enter a segment twice by taking different paths.  It is easy
//		to prevent this by maintaining a list of visited segments, but it is important
//		to reach segments with the greatest light intensity.  This can be done by doing
//		a breadth-first-search, or by storing the applied intensity with a visited segment,
//		and if the current intensity is brighter, then apply the difference between it and
//		the previous intensity.
//		Note that it is also possible to visit the original light-casting segment, for example
//		going from segment 0 to 2, then from 2 to 0.  This is peculiar and probably not
//		desired, but not entirely invalid.  2 reflects some light back to 0.
static void apply_light_intensity(const vmsegptr_t segp, const unsigned sidenum, fix intensity, const unsigned depth)
{
	if (intensity == 0)
		return;

	const auto wid_result = WALL_IS_DOORWAY(GameBitmaps, Textures, vcwallptr, segp, segp, sidenum);
	if (!(wid_result & WID_RENDPAST_FLAG)) {
		add_light_intensity_all_verts(segp->sides[sidenum], intensity);
		return;										// we return because there is a wall here, and light does not shine through walls
	}

	//	No wall here, so apply light recursively
	if (depth < 3) {
		intensity /= 3;
		if (!intensity)
			return;
		const auto &&csegp = vmsegptr(segp->children[sidenum]);
		for (int s=0; s<MAX_SIDES_PER_SEGMENT; s++)
			apply_light_intensity(csegp, s, intensity, depth+1);
	}

}

// -----------------------------------------------------------------------------
//	Top level recursive function for applying light.
//	Calls apply_light_intensity.
//	Uses light value on segp:sidenum (tmap_num2 defines light value) and applies
//	the associated intensity to segp.  It calls apply_light_intensity to apply intensity/3
//	to all neighbors.  apply_light_intensity recursively calls itself to apply light to
//	subsequent neighbors (and forming loops, see above).
static void propagate_light_intensity(const vmsegptr_t segp, int sidenum) 
{
	fix		intensity;
	short		texmap;

	intensity = 0;
	texmap = segp->sides[sidenum].tmap_num;
	intensity += TmapInfo[texmap].lighting;
	texmap = (segp->sides[sidenum].tmap_num2) & 0x3fff;
	intensity += TmapInfo[texmap].lighting;

	if (intensity > 0) {
		add_light_intensity_all_verts(segp->sides[sidenum], intensity);
	
		//	Now, for all sides which are not the same as sidenum (the side casting the light),
		//	add a light value to them (if they have no children, ie, they have a wall there).
		for (int s=0; s<MAX_SIDES_PER_SEGMENT; s++)
			if (s != sidenum)
				apply_light_intensity(segp, s, intensity/2, 1);
	}

}


// -----------------------------------------------------------------------------
//	Highest level function, bound to a key.  Apply ambient light to all segments based
//	on user-defined light sources.
int LightAmbientLighting()
{
	range_for (const auto &&segp, vmsegptr)
	{
		for (int side=0;side<MAX_SIDES_PER_SEGMENT;side++)
			propagate_light_intensity(segp, side);
	}
	return 0;
}


// -----------------------------------------------------------------------------
int LightSelectNextVertex(void)
{
	Curvert++;
	if (Curvert >= 4)
		Curvert = 0;

	Update_flags |= UF_WORLD_CHANGED;

	return	1;
}

// -----------------------------------------------------------------------------
int LightSelectNextEdge(void)
{
	Curedge++;
	if (Curedge >= 4)
		Curedge = 0;

	Update_flags |= UF_WORLD_CHANGED;

	return	1;
}

// -----------------------------------------------------------------------------
//	Copy intensity from current vertex to all other vertices on side.
int LightCopyIntensity(void)
{
	int	intensity;

	const vmsegptr_t segp = Cursegp;
	intensity = get_light_intensity(segp, Curside, Curvert);

	for (int v=0; v<4; v++)
		if (v != Curvert)
			set_light_intensity(segp, Curside, v, intensity);

	return	1;
}

// -----------------------------------------------------------------------------
//	Copy intensity from current vertex to all other vertices on side.
int LightCopyIntensitySegment(void)
{
	int	intensity;

	const vmsegptr_t segp = Cursegp;
	intensity = get_light_intensity(segp, Curside, Curvert);

	for (int s=0; s<MAX_SIDES_PER_SEGMENT; s++)
		for (int v=0; v<4; v++)
			if ((s != Curside) || (v != Curvert))
				set_light_intensity(segp, s, v, intensity);

	return	1;
}

// -----------------------------------------------------------------------------
int LightDecreaseLightVertex(void)
{
	const vmsegptr_t segp = Cursegp;
	set_light_intensity(segp, Curside, Curvert, get_light_intensity(segp, Curside, Curvert) - F1_0 / NUM_LIGHTING_LEVELS);

	return	1;
}

// -----------------------------------------------------------------------------
int LightIncreaseLightVertex(void)
{
	const vmsegptr_t segp = Cursegp;
	set_light_intensity(segp, Curside, Curvert, get_light_intensity(segp, Curside, Curvert) + F1_0 / NUM_LIGHTING_LEVELS);

	return	1;
}

// -----------------------------------------------------------------------------
int LightDecreaseLightSide(void)
{
	const vmsegptr_t segp = Cursegp;
	for (int v=0; v<4; v++)
		set_light_intensity(segp, Curside, v, get_light_intensity(segp, Curside, v) - F1_0 / NUM_LIGHTING_LEVELS);

	return	1;
}

// -----------------------------------------------------------------------------
int LightIncreaseLightSide(void)
{
	const vmsegptr_t segp = Cursegp;
	for (int v=0; v<4; v++)
		set_light_intensity(segp, Curside, v, get_light_intensity(segp, Curside, v) + F1_0 / NUM_LIGHTING_LEVELS);

	return	1;
}

// -----------------------------------------------------------------------------
int LightDecreaseLightSegment(void)
{
	const vmsegptr_t segp = Cursegp;
	for (int s=0; s<MAX_SIDES_PER_SEGMENT; s++)
		for (int v=0; v<4; v++)
			set_light_intensity(segp, s, v, get_light_intensity(segp, s, v) - F1_0 / NUM_LIGHTING_LEVELS);

	return	1;
}

// -----------------------------------------------------------------------------
int LightIncreaseLightSegment(void)
{
	const vmsegptr_t segp = Cursegp;
	for (int s=0; s<MAX_SIDES_PER_SEGMENT; s++)
		for (int v=0; v<4; v++)
			set_light_intensity(segp, s, v, get_light_intensity(segp, s, v) + F1_0 / NUM_LIGHTING_LEVELS);

	return	1;
}

// -----------------------------------------------------------------------------
int LightSetDefault(void)
{
	const vmsegptr_t segp = Cursegp;
	for (int v=0; v<4; v++)
		set_light_intensity(segp, Curside, v, DEFAULT_LIGHTING);

	return	1;
}


// -----------------------------------------------------------------------------
int LightSetMaximum(void)
{
	const vmsegptr_t segp = Cursegp;
	for (int v=0; v<4; v++)
		set_light_intensity(segp, Curside, v, (NUM_LIGHTING_LEVELS - 1) * F1_0 / NUM_LIGHTING_LEVELS);

	return	1;
}


// -----------------------------------------------------------------------------
int LightSetDefaultAll(void)
{

	assign_default_lighting_all();

	Update_flags |= UF_WORLD_CHANGED;

	return	1;
}



