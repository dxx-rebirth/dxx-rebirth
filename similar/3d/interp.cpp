/*
 * This file is part of the DXX-Rebirth project <https://www.dxx-rebirth.com/>.
 * It is copyright by its individual contributors, as recorded in the
 * project's Git history.  See COPYING.txt at the top level for license
 * terms and a link to the Git history.
 */

/*
 *
 * Polygon object interpreter
 *
 */

#include <stdexcept>
#include <stdlib.h>
#include "dxxsconf.h"
#include "dsx-ns.h"
#include "dxxerror.h"

#include "interp.h"
#include "console.h"
#include "common/3d/globvars.h"
#include "polyobj.h"
#include "gr.h"
#include "byteutil.h"
#include "u_mem.h"

#include "compiler-range_for.h"
#include "d_range.h"
#include "d_zip.h"
#include "partial_range.h"

namespace dcx {

#if DXX_USE_EDITOR
int g3d_interp_outline;
#endif

namespace {

constexpr std::integral_constant<unsigned, 0> OP_EOF{};   //eof
constexpr std::integral_constant<unsigned, 1> OP_DEFPOINTS{};   //defpoints
constexpr std::integral_constant<unsigned, 2> OP_FLATPOLY{};   //flat-shaded polygon
constexpr std::integral_constant<unsigned, 3> OP_TMAPPOLY{};   //texture-mapped polygon
constexpr std::integral_constant<unsigned, 4> OP_SORTNORM{};   //sort by normal
constexpr std::integral_constant<unsigned, 5> OP_RODBM{};   //rod bitmap
constexpr std::integral_constant<unsigned, 6> OP_SUBCALL{};   //call a subobject
constexpr std::integral_constant<unsigned, 7> OP_DEFP_START{};   //defpoints with start
constexpr std::integral_constant<unsigned, 8> OP_GLOW{};   //glow value for next poly

static inline int16_t *wp(uint8_t *p)
{
	return reinterpret_cast<int16_t *>(p);
}

static inline const int16_t *wp(const uint8_t *p)
{
	return reinterpret_cast<const int16_t *>(p);
}

static inline const vms_vector *vp(const uint8_t *p)
{
	return reinterpret_cast<const vms_vector *>(p);
}

static inline int16_t w(const uint8_t *p)
{
	return *wp(p);
}

static void rotate_point_list(const zip<g3s_point * /* dest */, const vms_vector * /* src */> zr)
{
	range_for (auto &&z, zr)
	{
		auto &dest = std::get<0>(z);
		auto &src = std::get<1>(z);
		g3_rotate_point(dest, src);
	}
}

constexpr vms_angvec zero_angles = {0,0,0};

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

}

}

class interpreter_track_model_extent
{
protected:
	const uint8_t *const model_base;
	const std::size_t model_length;
public:
	constexpr interpreter_track_model_extent(const uint8_t *const b, const std::size_t l) :
		model_base(b), model_length(l)
	{
	}
	uint8_t truncate_invalid_model(const unsigned line, uint8_t *const p, const std::ptrdiff_t d, const std::size_t size) const
	{
		const std::ptrdiff_t offset_from_base = p - model_base;
		const std::ptrdiff_t offset_of_value = offset_from_base + d;
		if (offset_of_value >= model_length || offset_of_value + size >= model_length)
		{
			auto &opref = *wp(p);
			const auto badop = opref;
			opref = OP_EOF;
			const unsigned long uml = model_length;
			const long ofb = offset_from_base;
			const long oov = offset_of_value;
			con_printf(CON_URGENT, "%s:%u: warning: invalid polymodel at %p with length %lu; opcode %u at offset %li references invalid offset %li; replacing invalid operation with EOF", __FILE__, line, model_base, uml, badop, ofb, oov);
			return 1;
		}
		return 0;
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
	static void op_default(const unsigned op, const uint8_t *const p)
	{
		char buf[64];
		snprintf(buf, sizeof(buf), "invalid polygon model opcode %u at %p", op, p);
		throw std::runtime_error(buf);
	}
};

namespace dsx {

namespace {

static int16_t init_model_sub(uint8_t *model_sub_ptr, const uint8_t *model_base_ptr, std::size_t model_size);
#if defined(DXX_BUILD_DESCENT_I)
static void validate_model_sub(uint8_t *model_sub_ptr, const uint8_t *model_base_ptr, std::size_t model_size);
#endif

class g3_poly_get_color_state :
	public interpreter_ignore_op_defpoints,
	public interpreter_ignore_op_defp_start,
	public interpreter_ignore_op_tmappoly,
	public interpreter_ignore_op_rodbm,
	public interpreter_ignore_op_glow,
	public interpreter_base
{
public:
	int color = 0;
	void op_flatpoly(const uint8_t *const p, const uint_fast32_t nv)
	{
		if (nv > MAX_POINTS_PER_POLY)
			return;
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

class g3_interpreter_draw_base
{
protected:
	grs_bitmap *const *const model_bitmaps;
	polygon_model_points &Interp_point_list;
	grs_canvas &canvas;
	const submodel_angles anim_angles;
	const g3s_lrgb model_light;
private:
	void rotate(uint_fast32_t i, const vms_vector *const src, const uint_fast32_t n)
	{
		rotate_point_list(zip(partial_range(Interp_point_list, i, i + n), unchecked_partial_range(src, n)));
	}
	void set_color_by_model_light(fix g3s_lrgb::*const c, g3s_lrgb &o, const fix color) const
	{
		o.*c = fixmul(color, model_light.*c);
	}
protected:
	template <std::size_t N>
		std::array<cg3s_point *, N> prepare_point_list(const uint_fast32_t nv, const uint8_t *const p)
		{
			std::array<cg3s_point *, N> point_list;
			for (uint_fast32_t i = 0; i < nv; ++i)
				point_list[i] = &Interp_point_list[wp(p + 30)[i]];
			return point_list;
		}
	g3s_lrgb get_noglow_light(const uint8_t *const p) const
	{
		g3s_lrgb light;
		const auto negdot = -vm_vec_dot(View_matrix.fvec, *vp(p + 16));
		const auto color = (f1_0 / 4) + ((negdot * 3) / 4);
		set_color_by_model_light(&g3s_lrgb::r, light, color);
		set_color_by_model_light(&g3s_lrgb::g, light, color);
		set_color_by_model_light(&g3s_lrgb::b, light, color);
		return light;
	}
	g3_interpreter_draw_base(grs_bitmap *const *const mbitmaps, polygon_model_points &plist, grs_canvas &ccanvas, const submodel_angles aangles, const g3s_lrgb &mlight) :
		model_bitmaps(mbitmaps), Interp_point_list(plist),
		canvas(ccanvas),
		anim_angles(aangles), model_light(mlight)
	{
	}
	void op_defpoints(const vms_vector *const src, const uint_fast32_t n)
	{
		rotate(0, src, n);
	}
	void op_defp_start(const uint8_t *const p, const vms_vector *const src, const uint_fast32_t n)
	{
		rotate(static_cast<int>(w(p + 4)), src, n);
	}
	static std::pair<uint16_t, uint16_t> get_sortnorm_offsets(const uint8_t *const p)
	{
		const uint16_t a = w(p + 30), b = w(p + 28);
		return (g3_check_normal_facing(*vp(p + 16), *vp(p + 4)) > 0)
			? std::make_pair(a, b)	//draw back then front
			: std::make_pair(b, a)	//not facing.  draw front then back
		;
	}
	void op_rodbm(const uint8_t *const p)
	{
		const auto &&rod_bot_p = g3_rotate_point(*vp(p + 20));
		const auto &&rod_top_p = g3_rotate_point(*vp(p + 4));
		const g3s_lrgb rodbm_light{
			f1_0, f1_0, f1_0
		};
		g3_draw_rod_tmap(canvas, *model_bitmaps[w(p + 2)], rod_bot_p, w(p + 16), rod_top_p, w(p + 32), rodbm_light);
	}
	void op_subcall(const uint8_t *const p, const glow_values_t *const glow_values)
	{
		g3_start_instance_angles(*vp(p + 4), anim_angles ? anim_angles[w(p + 2)] : zero_angles);
		g3_draw_polygon_model(model_bitmaps, Interp_point_list, canvas, anim_angles, model_light, glow_values, p + w(p + 16));
		g3_done_instance();
	}
};

class g3_draw_polygon_model_state :
	public interpreter_base,
	g3_interpreter_draw_base
{
	const glow_values_t *const glow_values;
	unsigned glow_num = ~0u;		//glow off by default
public:
	g3_draw_polygon_model_state(grs_bitmap *const *const mbitmaps, polygon_model_points &plist, grs_canvas &ccanvas, const submodel_angles aangles, const g3s_lrgb &mlight, const glow_values_t *const glvalues) :
		g3_interpreter_draw_base(mbitmaps, plist, ccanvas, aangles, mlight),
		glow_values(glvalues)
	{
	}
	void op_defpoints(const uint8_t *const p, const uint_fast32_t n)
	{
		g3_interpreter_draw_base::op_defpoints(vp(p + 4), n);
	}
	void op_defp_start(const uint8_t *const p, const uint_fast32_t n)
	{
		g3_interpreter_draw_base::op_defp_start(p, vp(p + 8), n);
	}
	void op_flatpoly(const uint8_t *const p, const uint_fast32_t nv)
	{
		if (nv > MAX_POINTS_PER_POLY)
			return;
		if (g3_check_normal_facing(*vp(p+4),*vp(p+16)) > 0)
		{
#if defined(DXX_BUILD_DESCENT_II)
			if (!glow_values || !(glow_num < glow_values->size()) || (*glow_values)[glow_num] != -3)
#endif
			{
				//					DPH: Now we treat this color as 15bpp
#if defined(DXX_BUILD_DESCENT_I)
				const uint8_t color = w(p + 28);
#elif defined(DXX_BUILD_DESCENT_II)
				const uint8_t color = (glow_values && glow_num < glow_values->size() && (*glow_values)[glow_num] == -2)
					? 255
					: gr_find_closest_color_15bpp(w(p + 28));
#endif
				const auto point_list = prepare_point_list<MAX_POINTS_PER_POLY>(nv, p);
				g3_draw_poly(*grd_curcanv, nv, point_list, color);
			}
		}
	}
	static g3s_lrgb get_glow_light(const fix c)
	{
		return {c, c, c};
	}
	void op_tmappoly(const uint8_t *const p, const uint_fast32_t nv)
	{
		if (nv > MAX_POINTS_PER_POLY)
			return;
		if (!(g3_check_normal_facing(*vp(p+4),*vp(p+16)) > 0))
			return;
		//calculate light from surface normal
		const auto &&light = (glow_values && glow_num < glow_values->size())
			? get_glow_light((*glow_values)[std::exchange(glow_num, -1)]) //yes glow
			: get_noglow_light(p); //no glow
		//now poke light into l values
		std::array<g3s_uvl, MAX_POINTS_PER_POLY> uvl_list;
		std::array<g3s_lrgb, MAX_POINTS_PER_POLY> lrgb_list;
		const fix average_light = (light.r + light.g + light.b) / 3;
		range_for (const uint_fast32_t i, xrange(nv))
		{
			lrgb_list[i] = light;
			uvl_list[i] = (reinterpret_cast<const g3s_uvl *>(p+30+((nv&~1)+1)*2))[i];
			uvl_list[i].l = average_light;
		}
		const auto point_list = prepare_point_list<MAX_POINTS_PER_POLY>(nv, p);
		g3_draw_tmap(canvas, nv, point_list, uvl_list, lrgb_list, *model_bitmaps[w(p + 28)]);
	}
	void op_sortnorm(const uint8_t *const p)
	{
		const auto &&offsets = get_sortnorm_offsets(p);
		const auto a = offsets.first;
		const auto b = offsets.second;
		g3_draw_polygon_model(model_bitmaps, Interp_point_list, canvas, anim_angles, model_light, glow_values, p + a);
		g3_draw_polygon_model(model_bitmaps, Interp_point_list, canvas, anim_angles, model_light, glow_values, p + b);
	}
	using g3_interpreter_draw_base::op_rodbm;
	void op_subcall(const uint8_t *const p)
	{
		g3_interpreter_draw_base::op_subcall(p, glow_values);
	}
	void op_glow(const uint8_t *const p)
	{
		glow_num = w(p+2);
	}
};

class g3_draw_morphing_model_state :
	public interpreter_ignore_op_glow,
	public interpreter_base,
	g3_interpreter_draw_base
{
	const vms_vector *const new_points;
	static constexpr const glow_values_t *glow_values = nullptr;
public:
	g3_draw_morphing_model_state(grs_bitmap *const *const mbitmaps, polygon_model_points &plist, grs_canvas &ccanvas, const submodel_angles aangles, const g3s_lrgb &mlight, const vms_vector *const npoints) :
		g3_interpreter_draw_base(mbitmaps, plist, ccanvas, aangles, mlight),
		new_points(npoints)
	{
	}
	void op_defpoints(const uint8_t *, const uint_fast32_t n)
	{
		g3_interpreter_draw_base::op_defpoints(new_points, n);
	}
	void op_defp_start(const uint8_t *const p, const uint_fast32_t n)
	{
		g3_interpreter_draw_base::op_defp_start(p, new_points, n);
	}
	void op_flatpoly(const uint8_t *const p, const uint_fast32_t nv)
	{
		int ntris;
		const uint8_t color = w(p+28);
		unsigned i;
		auto point_list = prepare_point_list<3>(i = 2, p);
		for (ntris=nv-2;ntris;ntris--) {
			point_list[2] = &Interp_point_list[wp(p+30)[i++]];
			g3_check_and_draw_poly(canvas, point_list, color);
			point_list[1] = point_list[2];
		}
	}
	void op_tmappoly(const uint8_t *const p, const uint_fast32_t nv)
	{
		if (nv > MAX_POINTS_PER_POLY)
			return;
		std::array<g3s_uvl, MAX_POINTS_PER_POLY> uvl_list;
		std::array<g3s_lrgb, MAX_POINTS_PER_POLY> lrgb_list;
		lrgb_list.fill(get_noglow_light(p));
		range_for (const uint_fast32_t i, xrange(nv))
			uvl_list[i] = (reinterpret_cast<const g3s_uvl *>(p+30+((nv&~1)+1)*2))[i];
		const auto point_list = prepare_point_list<MAX_POINTS_PER_POLY>(nv, p);
		g3_draw_tmap(canvas, nv, point_list, uvl_list, lrgb_list, *model_bitmaps[w(p + 28)]);
	}
	void op_sortnorm(const uint8_t *const p)
	{
		const auto &&offsets = get_sortnorm_offsets(p);
		auto &a = offsets.first;
		auto &b = offsets.second;
		g3_draw_morphing_model(canvas, p + a, model_bitmaps, anim_angles, model_light, new_points, Interp_point_list);
		g3_draw_morphing_model(canvas, p + b, model_bitmaps, anim_angles, model_light, new_points, Interp_point_list);
	}
	using g3_interpreter_draw_base::op_rodbm;
	void op_subcall(const uint8_t *const p)
	{
		g3_interpreter_draw_base::op_subcall(p, glow_values);
	}
};

template <typename T>
class model_load_state :
	public interpreter_track_model_extent,
	public interpreter_ignore_op_defpoints,
	public interpreter_ignore_op_defp_start,
	public interpreter_ignore_op_rodbm,
	public interpreter_ignore_op_glow,
	public interpreter_base
{
public:
	using interpreter_track_model_extent::interpreter_track_model_extent;
	int16_t init_bounded_model_sub(const unsigned line, uint8_t *const p, const std::ptrdiff_t d) const
	{
		if (truncate_invalid_model(line, p, d, sizeof(uint16_t)))
			return 0;
		return static_cast<const T *>(this)->init_sub_model(p + d);
	}
	void op_tmappoly(uint8_t *const p, const uint_fast32_t nv)
	{
		constexpr unsigned offset_texture = 28;
		(void)nv;
		Assert(nv > 2);		//must have 3 or more points
		if (truncate_invalid_model(__LINE__, p, offset_texture, sizeof(uint16_t)))
			return;
		static_cast<T *>(this)->update_texture(w(p + offset_texture));
	}
	void op_sortnorm(uint8_t *const p)
	{
		constexpr unsigned offset_submodel0 = 28;
		constexpr unsigned offset_submodel1 = offset_submodel0 + sizeof(uint16_t);
		if (truncate_invalid_model(__LINE__, p, offset_submodel1, sizeof(uint16_t)))
			return;
		const auto n0 = w(p + offset_submodel0);
		const auto h0 = init_bounded_model_sub(__LINE__, p, n0);
		const auto n1 = w(p + offset_submodel1);
		const auto h1 = init_bounded_model_sub(__LINE__, p, n1);
		const auto hm = std::max(h0, h1);
		static_cast<T *>(this)->update_texture(hm);
	}
	void op_subcall(uint8_t *const p)
	{
		constexpr unsigned offset_displacement = 16;
		if (truncate_invalid_model(__LINE__, p, offset_displacement, sizeof(uint16_t)))
			return;
		const auto n0 = w(p + offset_displacement);
		const auto h0 = init_bounded_model_sub(__LINE__, p, n0);
		static_cast<T *>(this)->update_texture(h0);
	}
};

class init_model_sub_state :
	public model_load_state<init_model_sub_state>
#if defined(DXX_BUILD_DESCENT_II)
	, public interpreter_ignore_op_flatpoly
#endif
{
public:
	int16_t highest_texture_num = -1;
	using model_load_state::model_load_state;
	int16_t init_sub_model(uint8_t *const p) const
	{
		return init_model_sub(p, model_base, model_length);
	}
	void update_texture(const int16_t t)
	{
		if (highest_texture_num < t)
			highest_texture_num = t;
	}
#if defined(DXX_BUILD_DESCENT_I)
	void op_flatpoly(uint8_t *const p, const uint_fast32_t nv) const
	{
		//must have 3 or more points
		if (nv <= 2)
			return;
		const auto p16 = wp(p + 28);
		*p16 = static_cast<short>(gr_find_closest_color_15bpp(*p16));
	}
#endif
};

#if defined(DXX_BUILD_DESCENT_I)
class validate_model_sub_state :
	public model_load_state<validate_model_sub_state>,
	public interpreter_ignore_op_flatpoly
{
public:
	using model_load_state::model_load_state;
	unsigned init_sub_model(uint8_t *const p) const
	{
		validate_model_sub(p, model_base, model_length);
		return 0;
	}
	void update_texture(int16_t)
	{
	}
};
#endif

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
			state.op_default(op, p);
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

}

}

#if DXX_WORDS_BIGENDIAN
namespace dcx {

namespace {

static inline fix *fp(uint8_t *p)
{
	return reinterpret_cast<fix *>(p);
}

static inline vms_vector *vp(uint8_t *p)
{
	return reinterpret_cast<vms_vector *>(p);
}

static void short_swap(short *s)
{
	*s = SWAPSHORT(*s);
}

static void fix_swap(fix &f)
{
	f = SWAPINT(f);
}

static void fix_swap(fix *f)
{
	fix_swap(*f);
}

static void vms_vector_swap(vms_vector &v)
{
	fix_swap(v.x);
	fix_swap(v.y);
	fix_swap(v.z);
}

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
		range_for (const uint_fast32_t i, xrange(n))
			vms_vector_swap(*vp((p + 4) + (i * sizeof(vms_vector))));
	}
	static void op_defp_start(uint8_t *const p, const uint_fast32_t n)
	{
		*wp(p + 2) = n;
		short_swap(wp(p + 4));
		range_for (const uint_fast32_t i, xrange(n))
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
		range_for (const uint_fast32_t i, xrange(n)) {
			const auto uvl_val = reinterpret_cast<g3s_uvl *>((p+30+((n&~1)+1)*2) + (i * sizeof(g3s_uvl)));
			fix_swap(&uvl_val->u);
			fix_swap(&uvl_val->v);
		}
		short_swap(wp(p+28));
		range_for (const uint_fast32_t i, xrange(n))
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

}
#endif

#if DXX_WORDS_NEED_ALIGNMENT

#define MAX_CHUNKS 100 // increase if insufficent
namespace dcx {

namespace {

/*
 * A chunk struct (as used for alignment) contains all relevant data
 * concerning a piece of data that may need to be aligned.
 * To align it, we need to copy it to an aligned position,
 * and update all pointers  to it.
 * (Those pointers are actually offsets
 * relative to start of model_data) to it.
 */
struct chunk
{
	const uint8_t *old_base; // where the offset sets off from (relative to beginning of model_data)
	uint8_t *new_base; // where the base is in the aligned structure
	short offset; // how much to add to base to get the address of the offset
	short correction; // how much the value of the offset must be shifted for alignment
};

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
	get_chunks_state(const uint8_t *const p, uint8_t *const ndata, chunk *const l, int *const n) :
		data(p), new_data(ndata), list(l), no(n)
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

static const uint8_t *old_dest(const chunk &o) // return where chunk is (in unaligned struct)
{
	return GET_INTEL_SHORT(&o.old_base[o.offset]) + o.old_base;
}
static uint8_t *new_dest(const chunk &o) // return where chunk is (in aligned struct)
{
	return GET_INTEL_SHORT(&o.old_base[o.offset]) + o.new_base + o.correction;
}

/*
 * find chunk with smallest address
 */
static int get_first_chunks_index(chunk *chunk_list, int no_chunks)
{
	int first_index = 0;
	Assert(no_chunks >= 1);
	for (int i = 1; i < no_chunks; i++)
		if (old_dest(chunk_list[i]) < old_dest(chunk_list[first_index]))
			first_index = i;
	return first_index;
}

}

void align_polygon_model_data(polymodel *pm)
{
	int chunk_len;
	int total_correction = 0;
	chunk cur_ch;
	chunk ch_list[MAX_CHUNKS];
	int no_chunks = 0;
	constexpr unsigned SHIFT_SPACE = 500;	// increase if insufficient
	int tmp_size = pm->model_data_size + SHIFT_SPACE;
	RAIIdmem<uint8_t[]> tmp;
	MALLOC(tmp, uint8_t[], tmp_size); // where we build the aligned version of pm->model_data

	Assert(tmp != NULL);
	//start with first chunk (is always aligned!)
	const uint8_t *cur_old = pm->model_data.get();
	auto cur_new = tmp.get();
	chunk_len = get_chunks(cur_old, cur_new, ch_list, &no_chunks);
	memcpy(cur_new, cur_old, chunk_len);
	while (no_chunks > 0) {
		int first_index = get_first_chunks_index(ch_list, no_chunks);
		cur_ch = ch_list[first_index];
		// remove first chunk from array:
		no_chunks--;
		for (int i = first_index; i < no_chunks; i++)
			ch_list[i] = ch_list[i + 1];
		// if (new) address unaligned:
		const uintptr_t u = reinterpret_cast<uintptr_t>(new_dest(cur_ch));
		if (u % 4L != 0) {
			// calculate how much to move to be aligned
			short to_shift = 4 - u % 4L;
			// correct chunks' addresses
			cur_ch.correction += to_shift;
			for (int i = 0; i < no_chunks; i++)
				ch_list[i].correction += to_shift;
			total_correction += to_shift;
			Assert(reinterpret_cast<uintptr_t>(new_dest(cur_ch)) % 4L == 0);
			Assert(total_correction <= SHIFT_SPACE); // if you get this, increase SHIFT_SPACE
		}
		//write (corrected) chunk for current chunk:
		*(reinterpret_cast<short *>(cur_ch.new_base + cur_ch.offset))
			= INTEL_SHORT(static_cast<short>(cur_ch.correction + GET_INTEL_SHORT(cur_ch.old_base + cur_ch.offset)));
		//write (correctly aligned) chunk:
		cur_old = old_dest(cur_ch);
		cur_new = new_dest(cur_ch);
		chunk_len = get_chunks(cur_old, cur_new, ch_list, &no_chunks);
		memcpy(cur_new, cur_old, chunk_len);
		//correct submodel_ptr's for pm, too
		for (auto &sp : pm->submodel_ptrs)
			if (&pm->model_data[sp] >= cur_old &&
				&pm->model_data[sp] < cur_old + chunk_len)
				sp += (cur_new - tmp.get()) - (cur_old - pm->model_data.get());
	}
	pm->model_data_size += total_correction;
	pm->model_data = std::make_unique<uint8_t[]>(pm->model_data_size);
	memcpy(pm->model_data.get(), tmp.get(), pm->model_data_size);
}

}
#endif //def WORDS_NEED_ALIGNMENT

namespace dsx {

// check a polymodel for it's color and return it
int g3_poly_get_color(const uint8_t *p)
{
	g3_poly_get_color_state state;
	iterate_polymodel(p, state);
	return state.color;
}

//calls the object interpreter to render an object.  The object renderer
//is really a seperate pipeline. returns true if drew
void g3_draw_polygon_model(grs_bitmap *const *const model_bitmaps, polygon_model_points &Interp_point_list, grs_canvas &canvas, const submodel_angles anim_angles, const g3s_lrgb model_light, const glow_values_t *const glow_values, const uint8_t *const p)
{
	g3_draw_polygon_model_state state(model_bitmaps, Interp_point_list, canvas, anim_angles, model_light, glow_values);
	iterate_polymodel(p, state);
}

#ifndef NDEBUG
static int nest_count;
#endif

//alternate interpreter for morphing object
void g3_draw_morphing_model(grs_canvas &canvas, const uint8_t *const p, grs_bitmap *const *const model_bitmaps, const submodel_angles anim_angles, const g3s_lrgb model_light, const vms_vector *new_points, polygon_model_points &Interp_point_list)
{
	g3_draw_morphing_model_state state(model_bitmaps, Interp_point_list, canvas, anim_angles, model_light, new_points);
	iterate_polymodel(p, state);
}

namespace {

static int16_t init_model_sub(uint8_t *const model_sub_ptr, const uint8_t *const model_base_ptr, const std::size_t model_size)
{
	init_model_sub_state state(model_base_ptr, model_size);
	Assert(++nest_count < 1000);
	iterate_polymodel(model_sub_ptr, state);
	return state.highest_texture_num;
}

}

//init code for bitmap models
int16_t g3_init_polygon_model(uint8_t *const model_ptr, const std::size_t model_size)
{
	#ifndef NDEBUG
	nest_count = 0;
	#endif
	return init_model_sub(model_ptr, model_ptr, model_size);
}

#if defined(DXX_BUILD_DESCENT_I)
namespace {

static void validate_model_sub(uint8_t *const model_sub_ptr, const uint8_t *const model_base_ptr, const std::size_t model_size)
{
	validate_model_sub_state state(model_base_ptr, model_size);
	assert(++nest_count < 1000);
	iterate_polymodel(model_sub_ptr, state);
}

}

void g3_validate_polygon_model(uint8_t *const model_ptr, const std::size_t model_size)
{
#ifndef NDEBUG
	nest_count = 0;
#endif
	return validate_model_sub(model_ptr, model_ptr, model_size);
}
#endif

}
