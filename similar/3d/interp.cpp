/*
 * This file is part of the DXX-Rebirth project <http://www.dxx-rebirth.com/>.
 * It is copyright by its individual contributors, as recorded in the
 * project's Git history.  See COPYING.txt at the top level for license
 * terms and a link to the Git history.
 */

/*
 *
 * Polygon object interpreter
 *
 */

#include <stdlib.h>
#include "dxxsconf.h"
#include "dxxerror.h"

#include "interp.h"
#include "common/3d/globvars.h"
#include "polyobj.h"
#include "gr.h"
#include "byteutil.h"
#include "u_mem.h"

static const unsigned OP_EOF = 0;   //eof
static const unsigned OP_DEFPOINTS = 1;   //defpoints
static const unsigned OP_FLATPOLY = 2;   //flat-shaded polygon
static const unsigned OP_TMAPPOLY = 3;   //texture-mapped polygon
static const unsigned OP_SORTNORM = 4;   //sort by normal
static const unsigned OP_RODBM = 5;   //rod bitmap
static const unsigned OP_SUBCALL = 6;   //call a subobject
static const unsigned OP_DEFP_START = 7;   //defpoints with start
static const unsigned OP_GLOW = 8;   //glow value for next poly

static int16_t init_model_sub(uint8_t *p, int16_t);

#ifdef EDITOR
int g3d_interp_outline;
#endif

static inline int16_t *wp(uint8_t *p)
{
	return reinterpret_cast<int16_t *>(p);
}

static inline const int16_t *wp(const uint8_t *p)
{
	return reinterpret_cast<const int16_t *>(p);
}

static inline fix *fp(uint8_t *p)
{
	return reinterpret_cast<fix *>(p);
}

static inline const fix *fp(const uint8_t *p)
{
	return reinterpret_cast<const fix *>(p);
}

static inline vms_vector *vp(uint8_t *p)
{
	return reinterpret_cast<vms_vector *>(p);
}

static inline const vms_vector *vp(const uint8_t *p)
{
	return reinterpret_cast<const vms_vector *>(p);
}

static inline int16_t w(const uint8_t *p)
{
	return *wp(p);
}

static void rotate_point_list(g3s_point *dest, const vms_vector *src, uint_fast32_t n)
{
	while (n--)
		g3_rotate_point(*dest++,*src++);
}

static const vms_angvec zero_angles = {0,0,0};

namespace {

class interpreter_ignore_op_defpoints
{
public:
	static void op_defpoints(const uint8_t *, uint16_t)
	{
	}
};

class interpreter_ignore_op_defp_start
{
public:
	static void op_defp_start(const uint8_t *, uint16_t)
	{
	}
};

class interpreter_ignore_op_flatpoly
{
public:
	static void op_flatpoly(const uint8_t *, uint16_t)
	{
	}
};

class interpreter_ignore_op_tmappoly
{
public:
	static void op_tmappoly(const uint8_t *, uint16_t)
	{
	}
};

class interpreter_ignore_op_rodbm
{
public:
	static void op_rodbm(const uint8_t *)
	{
	}
};

class interpreter_ignore_op_glow
{
public:
	static void op_glow(const uint8_t *)
	{
	}
};

class interpreter_base
{
public:
	static uint16_t get_raw_opcode(const uint8_t *const p)
	{
		return w(p);
	}
	static uint_fast32_t translate_opcode(const uint8_t *, const uint16_t op)
	{
		return op;
	}
	static uint16_t get_op_subcount(const uint8_t *const p)
	{
		return w(p + 2);
	}
	__attribute_cold
	static void op_default()
	{
		throw std::runtime_error("invalid polygon model");
	}
};

class g3_poly_get_color_state :
	public interpreter_ignore_op_defpoints,
	public interpreter_ignore_op_defp_start,
	public interpreter_ignore_op_tmappoly,
	public interpreter_ignore_op_rodbm,
	public interpreter_ignore_op_glow,
	public interpreter_base
{
public:
	int color;
	g3_poly_get_color_state() :
		color(0)
	{
	}
	void op_flatpoly(const uint8_t *const p, const uint_fast32_t nv)
	{
		Assert( nv < MAX_POINTS_PER_POLY );
		if (g3_check_normal_facing(*vp(p+4),*vp(p+16)) > 0) {
#if defined(DXX_BUILD_DESCENT_I)
			color = (w(p+28));
#elif defined(DXX_BUILD_DESCENT_II)
			color = gr_find_closest_color_15bpp(w(p + 28));
#endif
		}
	}
	void op_sortnorm(const uint8_t *const p)
	{
		const bool facing = g3_check_normal_facing(*vp(p+16),*vp(p+4)) > 0;
		color = g3_poly_get_color(facing ? p + w(p + 28) : p + w(p + 30));
	}
	void op_subcall(const uint8_t *const p)
	{
#if defined(DXX_BUILD_DESCENT_I)
		color = g3_poly_get_color(p+w(p+16));
#elif defined(DXX_BUILD_DESCENT_II)
		(void)p;
#endif
	}
};

class g3_draw_polygon_model_state :
	public interpreter_base
{
	grs_bitmap **const model_bitmaps;
	const submodel_angles anim_angles;
	const g3s_lrgb &model_light;
	const glow_values_t *const glow_values;
	polygon_model_points &Interp_point_list;
	unsigned glow_num;
public:
	g3_draw_polygon_model_state(grs_bitmap **mbitmaps, const submodel_angles aangles, const g3s_lrgb &mlight, const glow_values_t *glvalues, polygon_model_points &plist) :
		model_bitmaps(mbitmaps), anim_angles(aangles),
		model_light(mlight), glow_values(glvalues),
		Interp_point_list(plist),
		glow_num(~0u)		//glow off by default
	{
	}
	void op_defpoints(const uint8_t *const p, const uint_fast32_t n)
	{
		rotate_point_list(&Interp_point_list[0],vp(p+4),n);
	}
	void op_defp_start(const uint8_t *const p, const uint_fast32_t n)
	{
		int s = w(p+4);
		rotate_point_list(&Interp_point_list[s],vp(p+8),n);
	}
	void op_flatpoly(const uint8_t *const p, const uint_fast32_t nv)
	{
		Assert( nv < MAX_POINTS_PER_POLY );
		if (g3_check_normal_facing(*vp(p+4),*vp(p+16)) > 0)
		{
			array<cg3s_point *, MAX_POINTS_PER_POLY> point_list;
			for (uint_fast32_t i = 0;i < nv;i++)
				point_list[i] = &Interp_point_list[wp(p+30)[i]];
#if defined(DXX_BUILD_DESCENT_II)
			if (!glow_values || !(glow_num < glow_values->size()) || (*glow_values)[glow_num] != -3)
#endif
			{
				//					DPH: Now we treat this color as 15bpp
#if defined(DXX_BUILD_DESCENT_I)
				gr_setcolor(w(p+28));
#elif defined(DXX_BUILD_DESCENT_II)
				if (glow_values && glow_num < glow_values->size() && (*glow_values)[glow_num] == -2)
					gr_setcolor(255);
				else
				{
					gr_setcolor(gr_find_closest_color_15bpp(w(p + 28)));
				}
#endif
				g3_draw_poly(nv,point_list);
			}
		}
	}
	void op_tmappoly(const uint8_t *const p, const uint_fast32_t nv)
	{
		Assert( nv < MAX_POINTS_PER_POLY );
		if (!(g3_check_normal_facing(*vp(p+4),*vp(p+16)) > 0))
			return;
		g3s_lrgb light;
		//calculate light from surface normal
		if (!glow_values || !(glow_num < glow_values->size())) //no glow
		{
			light.r = light.g = light.b = -vm_vec_dot(View_matrix.fvec,*vp(p+16));
			light.r = f1_0/4 + (light.r*3)/4;
			light.r = fixmul(light.r,model_light.r);
			light.g = f1_0/4 + (light.g*3)/4;
			light.g = fixmul(light.g,model_light.g);
			light.b = f1_0/4 + (light.b*3)/4;
			light.b = fixmul(light.b,model_light.b);
		}
		else //yes glow
		{
			light.r = light.g = light.b = (*glow_values)[glow_num];
			glow_num = -1;
		}
		//now poke light into l values
		array<g3s_uvl, MAX_POINTS_PER_POLY> uvl_list;
		array<g3s_lrgb, MAX_POINTS_PER_POLY> lrgb_list;
		for (uint_fast32_t i = 0; i != nv; i++)
		{
			lrgb_list[i] = light;
			uvl_list[i] = (reinterpret_cast<const g3s_uvl *>(p+30+((nv&~1)+1)*2))[i];
			uvl_list[i].l = (light.r+light.g+light.b)/3;
		}
		array<cg3s_point *, MAX_POINTS_PER_POLY> point_list;
		for (uint_fast32_t i = 0; i != nv; i++)
			point_list[i] = &Interp_point_list[wp(p+30)[i]];
		g3_draw_tmap(nv,point_list,uvl_list,lrgb_list,*model_bitmaps[w(p+28)]);
	}
	void op_sortnorm(const uint8_t *const p)
	{
		const bool facing = g3_check_normal_facing(*vp(p+16),*vp(p+4)) > 0;		//facing
		uint16_t a = w(p+30), b = w(p+28);
		if (facing) {
			//draw back then front
		}
		else {			//not facing.  draw front then back
			std::swap(a, b);
		}
		g3_draw_polygon_model(p + a,model_bitmaps,anim_angles,model_light,glow_values, Interp_point_list);
		g3_draw_polygon_model(p + b,model_bitmaps,anim_angles,model_light,glow_values, Interp_point_list);
	}
	void op_rodbm(const uint8_t *const p)
	{
		const g3s_lrgb rodbm_light{
			f1_0, f1_0, f1_0
		};
		const auto rod_bot_p = g3_rotate_point(*vp(p+20));
		const auto rod_top_p = g3_rotate_point(*vp(p+4));
		g3_draw_rod_tmap(*model_bitmaps[w(p+2)],rod_bot_p,w(p+16),rod_top_p,w(p+32),rodbm_light);
	}
	void op_subcall(const uint8_t *const p)
	{
		const vms_angvec *a;
		if (anim_angles)
			a = &anim_angles[w(p+2)];
		else
			a = &zero_angles;
		g3_start_instance_angles(*vp(p+4),a);
		g3_draw_polygon_model(p+w(p+16),model_bitmaps,anim_angles,model_light,glow_values, Interp_point_list);
		g3_done_instance();
	}
	void op_glow(const uint8_t *const p)
	{
		glow_num = w(p+2);
	}
};

/* Morphing models always ignored light.  Now ignore it more
 * efficiently.
 */
class glow_num_stub : public tt::integral_constant<unsigned, ~0u>
{
public:
	const glow_num_stub &operator=(unsigned) const
	{
		return *this;
	}
};

class g3_draw_morphing_model_state :
	public interpreter_base
{
	grs_bitmap **const model_bitmaps;
	const submodel_angles anim_angles;
	const g3s_lrgb model_light;
	static constexpr const glow_values_t *glow_values = nullptr;
	const vms_vector *const new_points;
	polygon_model_points &Interp_point_list;
	static constexpr glow_num_stub glow_num{};
public:
	g3_draw_morphing_model_state(grs_bitmap **mbitmaps, const submodel_angles aangles, g3s_lrgb mlight, const vms_vector *npoints, polygon_model_points &plist) :
		model_bitmaps(mbitmaps), anim_angles(aangles),
		model_light(mlight), new_points(npoints),
		Interp_point_list(plist)
	{
	}
	void op_defpoints(const uint8_t *, const uint_fast32_t n)
	{
		rotate_point_list(&Interp_point_list[0],new_points,n);
	}
	void op_defp_start(const uint8_t *const p, const uint_fast32_t n)
	{
		int s = w(p+4);
		rotate_point_list(&Interp_point_list[s],new_points,n);
	}
	void op_flatpoly(const uint8_t *const p, const uint_fast32_t nv)
	{
		int i,ntris;
#if defined(DXX_BUILD_DESCENT_I)
		gr_setcolor(55/*w(p+28)*/);
#elif defined(DXX_BUILD_DESCENT_II)
		gr_setcolor(w(p+28));
#endif
		array<cg3s_point *, 3> point_list;
		for (i=0;i<2;i++)
			point_list[i] = &Interp_point_list[wp(p+30)[i]];
		for (ntris=nv-2;ntris;ntris--) {
			point_list[2] = &Interp_point_list[wp(p+30)[i++]];
			g3_check_and_draw_poly(point_list);
			point_list[1] = point_list[2];
		}
	}
	void op_tmappoly(const uint8_t *const p, const uint_fast32_t nv)
	{
		g3s_lrgb light;
		int ntris;
		//calculate light from surface normal
		if (!glow_values || !(glow_num < glow_values->size())) //no glow
		{
			light.r = light.g = light.b = -vm_vec_dot(View_matrix.fvec,*vp(p+16));
			light.r = f1_0/4 + (light.r*3)/4;
			light.r = fixmul(light.r,model_light.r);
			light.g = f1_0/4 + (light.g*3)/4;
			light.g = fixmul(light.g,model_light.g);
			light.b = f1_0/4 + (light.b*3)/4;
			light.b = fixmul(light.b,model_light.b);
		}
		else //yes glow
		{
			light.r = light.g = light.b = (*glow_values)[glow_num];
			glow_num = -1;
		}
		//now poke light into l values
		array<g3s_uvl, 3> uvl_list;
		array<g3s_lrgb, 3> lrgb_list;
		for (unsigned i = 0; i < 3; ++i)
			uvl_list[i] = ((g3s_uvl *) (p+30+((nv&~1)+1)*2))[i];
		lrgb_list.fill(light);
		array<cg3s_point *, 3> point_list;
		unsigned i;
		for (i = 0; i < 2; ++i)
		{
			point_list[i] = &Interp_point_list[wp(p+30)[i]];
		}
		for (ntris=nv-2;ntris;ntris--) {
			point_list[2] = &Interp_point_list[wp(p+30)[i]];
			i++;
			g3_check_and_draw_tmap(point_list,uvl_list,lrgb_list,*model_bitmaps[w(p+28)]);
			point_list[1] = point_list[2];
		}
	}
	void op_sortnorm(const uint8_t *const p)
	{
		const bool facing = g3_check_normal_facing(*vp(p+16),*vp(p+4)) > 0;		//facing
		uint16_t a = w(p+30), b = w(p+28);
		if (facing) {
			//draw back then front
		}
		else {			//not facing.  draw front then back
			std::swap(a, b);
		}
		g3_draw_morphing_model(p + a,model_bitmaps,anim_angles,model_light,new_points, Interp_point_list);
		g3_draw_morphing_model(p + b,model_bitmaps,anim_angles,model_light,new_points, Interp_point_list);
	}
	void op_rodbm(const uint8_t *const p)
	{
		const g3s_lrgb rodbm_light{
			f1_0, f1_0, f1_0
		};
		const auto rod_bot_p = g3_rotate_point(*vp(p+20));
		const auto rod_top_p = g3_rotate_point(*vp(p+4));
		g3_draw_rod_tmap(*model_bitmaps[w(p+2)],rod_bot_p,w(p+16),rod_top_p,w(p+32),rodbm_light);
	}
	void op_subcall(const uint8_t *const p)
	{
		const vms_angvec *a;
		if (anim_angles)
			a = &anim_angles[w(p+2)];
		else
			a = &zero_angles;
		g3_start_instance_angles(*vp(p+4),a);
		g3_draw_polygon_model(p+w(p+16),model_bitmaps,anim_angles,model_light,glow_values, Interp_point_list);
		g3_done_instance();
	}
	void op_glow(const uint8_t *const p)
	{
		glow_num = w(p+2);
	}
};

class init_model_sub_state :
	public interpreter_ignore_op_defpoints,
	public interpreter_ignore_op_defp_start,
	public interpreter_ignore_op_rodbm,
	public interpreter_ignore_op_glow,
	public interpreter_base
{
public:
	int16_t highest_texture_num;
	init_model_sub_state(int16_t h) :
		highest_texture_num(h)
	{
	}
	void op_flatpoly(uint8_t *const p, const uint_fast32_t nv)
	{
		(void)nv;
		Assert(nv > 2);		//must have 3 or more points
#if defined(DXX_BUILD_DESCENT_I)
		*wp(p+28) = (short)gr_find_closest_color_15bpp(w(p+28));
#endif
	}
	void op_tmappoly(const uint8_t *const p, const uint_fast32_t nv)
	{
		(void)nv;
		Assert(nv > 2);		//must have 3 or more points
		if (w(p+28) > highest_texture_num)
			highest_texture_num = w(p+28);
	}
	void op_sortnorm(uint8_t *const p)
	{
		auto h = init_model_sub(p+w(p+28), highest_texture_num);
		highest_texture_num = init_model_sub(p+w(p+30), h);
	}
	void op_subcall(uint8_t *const p)
	{
		highest_texture_num = init_model_sub(p+w(p+16), highest_texture_num);
	}
};

constexpr glow_num_stub g3_draw_morphing_model_state::glow_num;
constexpr const glow_values_t *g3_draw_morphing_model_state::glow_values;

}

template <typename P, typename State>
static std::size_t dispatch_polymodel_op(const P p, State &state, const uint_fast32_t op)
{
	switch (op)
	{
		case OP_DEFPOINTS: {
			const auto n = state.get_op_subcount(p);
			const std::size_t record_size = n * sizeof(vms_vector) + 4;
			state.op_defpoints(p, n);
			return record_size;
		}
		case OP_DEFP_START: {
			const auto n = state.get_op_subcount(p);
			const std::size_t record_size = n * sizeof(vms_vector) + 8;
			state.op_defp_start(p, n);
			return record_size;
		}
		case OP_FLATPOLY: {
			const auto n = state.get_op_subcount(p);
			const std::size_t record_size = 30 + ((n & ~1) + 1) * 2;
			state.op_flatpoly(p, n);
			return record_size;
		}
		case OP_TMAPPOLY: {
			const auto n = state.get_op_subcount(p);
			const std::size_t record_size = 30 + ((n & ~1) + 1) * 2 + n * 12;
			state.op_tmappoly(p, n);
			return record_size;
		}
		case OP_SORTNORM: {
			const std::size_t record_size = 32;
			state.op_sortnorm(p);
			return record_size;
		}
		case OP_RODBM: {
			const std::size_t record_size = 36;
			state.op_rodbm(p);
			return record_size;
		}
		case OP_SUBCALL: {
			const std::size_t record_size = 20;
			state.op_subcall(p);
			return record_size;
		}
		case OP_GLOW: {
			const std::size_t record_size = 4;
			state.op_glow(p);
			return record_size;
		}
		default:
			state.op_default();
			return 2;
	}
}

template <typename P, typename State>
static P iterate_polymodel(P p, State &state)
{
	for (uint16_t op; (op = state.get_raw_opcode(p)) != OP_EOF;)
		p += dispatch_polymodel_op(p, state, state.translate_opcode(p, op));
	return p;
}

#ifdef WORDS_BIGENDIAN
static void short_swap(short *s)
{
	*s = SWAPSHORT(*s);
}

static void fix_swap(fix &f)
{
	f = (fix)SWAPINT((int)f);
}

static void fix_swap(fix *f)
{
	fix_swap(*f);
}

void vms_vector_swap(vms_vector &v)
{
	fix_swap(v.x);
	fix_swap(v.y);
	fix_swap(v.z);
}

namespace {

class swap_polygon_model_data_state : public interpreter_base
{
public:
	static uint_fast32_t translate_opcode(uint8_t *const p, const uint16_t op)
	{
		return *wp(p) = INTEL_SHORT(op);
	}
	static uint16_t get_op_subcount(const uint8_t *const p)
	{
		return SWAPSHORT(w(p + 2));
	}
	static void op_defpoints(uint8_t *const p, const uint_fast32_t n)
	{
		*wp(p + 2) = n;
		for (uint_fast32_t i = 0; i != n; ++i)
			vms_vector_swap(*vp((p + 4) + (i * sizeof(vms_vector))));
	}
	static void op_defp_start(uint8_t *const p, const uint_fast32_t n)
	{
		*wp(p + 2) = n;
		short_swap(wp(p + 4));
		for (uint_fast32_t i = 0; i != n; ++i)
			vms_vector_swap(*vp((p + 8) + (i * sizeof(vms_vector))));
	}
	static void op_flatpoly(uint8_t *const p, const uint_fast32_t n)
	{
		*wp(p + 2) = n;
		vms_vector_swap(*vp(p + 4));
		vms_vector_swap(*vp(p + 16));
		short_swap(wp(p+28));
		for (uint_fast32_t i = 0; i < n; ++i)
			short_swap(wp(p + 30 + (i * 2)));
	}
	static void op_tmappoly(uint8_t *const p, const uint_fast32_t n)
	{
		*wp(p + 2) = n;
		vms_vector_swap(*vp(p + 4));
		vms_vector_swap(*vp(p + 16));
		for (uint_fast32_t i = 0; i != n; ++i) {
			const auto uvl_val = reinterpret_cast<g3s_uvl *>((p+30+((n&~1)+1)*2) + (i * sizeof(g3s_uvl)));
			fix_swap(&uvl_val->u);
			fix_swap(&uvl_val->v);
		}
		short_swap(wp(p+28));
		for (uint_fast32_t i = 0; i != n; ++i)
			short_swap(wp(p + 30 + (i * 2)));
	}
	void op_sortnorm(uint8_t *const p)
	{
		vms_vector_swap(*vp(p + 4));
		vms_vector_swap(*vp(p + 16));
		short_swap(wp(p + 28));
		short_swap(wp(p + 30));
		swap_polygon_model_data(p + w(p+28));
		swap_polygon_model_data(p + w(p+30));
	}
	static void op_rodbm(uint8_t *const p)
	{
		vms_vector_swap(*vp(p + 20));
		vms_vector_swap(*vp(p + 4));
		short_swap(wp(p+2));
		fix_swap(fp(p + 16));
		fix_swap(fp(p + 32));
	}
	void op_subcall(uint8_t *const p)
	{
		short_swap(wp(p+2));
		vms_vector_swap(*vp(p+4));
		short_swap(wp(p+16));
		swap_polygon_model_data(p + w(p+16));
	}
	static void op_glow(uint8_t *const p)
	{
		short_swap(wp(p + 2));
	}
};

}

void swap_polygon_model_data(ubyte *data)
{
	swap_polygon_model_data_state state;
	iterate_polymodel(data, state);
}
#endif

#ifdef WORDS_NEED_ALIGNMENT
static void add_chunk(const uint8_t *old_base, uint8_t *new_base, int offset,
	       chunk *chunk_list, int *no_chunks)
{
	Assert(*no_chunks + 1 < MAX_CHUNKS); //increase MAX_CHUNKS if you get this
	chunk_list[*no_chunks].old_base = old_base;
	chunk_list[*no_chunks].new_base = new_base;
	chunk_list[*no_chunks].offset = offset;
	chunk_list[*no_chunks].correction = 0;
	(*no_chunks)++;
}

namespace {

class get_chunks_state :
	public interpreter_ignore_op_defpoints,
	public interpreter_ignore_op_defp_start,
	public interpreter_ignore_op_flatpoly,
	public interpreter_ignore_op_tmappoly,
	public interpreter_ignore_op_rodbm,
	public interpreter_ignore_op_glow,
	public interpreter_base
{
	const uint8_t *const data;
	uint8_t *const new_data;
	chunk *const list;
	int *const no;
public:
	get_chunks_state(const uint8_t *data, uint8_t *ndata, chunk *l, int *n) :
		data(data), new_data(ndata), list(l), no(n)
	{
	}
	static uint_fast32_t translate_opcode(const uint8_t *, const uint16_t op)
	{
		return INTEL_SHORT(op);
	}
	static uint16_t get_op_subcount(const uint8_t *const p)
	{
		return GET_INTEL_SHORT(p + 2);
	}
	void op_sortnorm(const uint8_t *const p)
	{
		add_chunk(p, p - data + new_data, 28, list, no);
		add_chunk(p, p - data + new_data, 30, list, no);
	}
	void op_subcall(const uint8_t *const p)
	{
		add_chunk(p, p - data + new_data, 16, list, no);
	}
};

}

/*
 * finds what chunks the data points to, adds them to the chunk_list, 
 * and returns the length of the current chunk
 */
int get_chunks(const uint8_t *data, uint8_t *new_data, chunk *list, int *no)
{
	get_chunks_state state(data, new_data, list, no);
	auto p = iterate_polymodel(data, state);
	return p + 2 - data;
}
#endif //def WORDS_NEED_ALIGNMENT

// check a polymodel for it's color and return it
int g3_poly_get_color(const uint8_t *p)
{
	g3_poly_get_color_state state;
	iterate_polymodel(p, state);
	return state.color;
}

//calls the object interpreter to render an object.  The object renderer
//is really a seperate pipeline. returns true if drew
void g3_draw_polygon_model(const uint8_t *p, grs_bitmap **model_bitmaps, const submodel_angles anim_angles, g3s_lrgb model_light, const glow_values_t *glow_values, polygon_model_points &Interp_point_list)
{
	g3_draw_polygon_model_state state(model_bitmaps, anim_angles, model_light, glow_values, Interp_point_list);
	iterate_polymodel(p, state);
}

#ifndef NDEBUG
static int nest_count;
#endif

//alternate interpreter for morphing object
void g3_draw_morphing_model(const uint8_t *p,grs_bitmap **model_bitmaps,const submodel_angles anim_angles,g3s_lrgb model_light,const vms_vector *new_points, polygon_model_points &Interp_point_list)
{
	g3_draw_morphing_model_state state(model_bitmaps, anim_angles, model_light, new_points, Interp_point_list);
	iterate_polymodel(p, state);
}

static int16_t init_model_sub(uint8_t *p, int16_t highest_texture_num)
{
	init_model_sub_state state(highest_texture_num);
	Assert(++nest_count < 1000);
	iterate_polymodel(p, state);
	return state.highest_texture_num;
}

//init code for bitmap models
int16_t g3_init_polygon_model(void *model_ptr)
{
	#ifndef NDEBUG
	nest_count = 0;
	#endif

	return init_model_sub(reinterpret_cast<uint8_t *>(model_ptr), -1);
}
