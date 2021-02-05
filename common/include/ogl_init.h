/*
 * This file is part of the DXX-Rebirth project <https://www.dxx-rebirth.com/>.
 * It is copyright by its individual contributors, as recorded in the
 * project's Git history.  See COPYING.txt at the top level for license
 * terms and a link to the Git history.
 */
/* interface to OpenGL functions
 * Added 9/15/99 Matthew Mueller
 * Got rid of OpenGL-internal stuff 2004-5-16 Martin Schaffner
 */

#ifndef _OGL_INIT_H_ 
#define _OGL_INIT_H_

#include "dxxsconf.h"
#ifdef _MSC_VER
#include <windows.h>
#include <stddef.h>
#endif

#include "d_gl.h"
#include "dsx-ns.h"
#include "fwd-gr.h"
#include "fwd-segment.h"
#include "palette.h"
#include "pstypes.h"
#include "3d.h"
#include <array>

#ifdef __cplusplus

/* we need to export ogl_texture for 2d/font.c */
struct ogl_texture
{
	GLuint handle;
	GLint internalformat;
	GLenum format;
	int w,h,tw,th,lw;
	int bytesu;
	int bytes;
	GLfloat u,v;
	GLfloat prio;
	int wrapstate;
	unsigned long numrend;
};

extern ogl_texture* ogl_get_free_texture();
void ogl_init_texture(ogl_texture &t, int w, int h, int flags);

void ogl_init_shared_palette(void);

namespace dcx {

/* These values are written to a file as integers, so they must not be
 * renumbered.
 */
enum class opengl_texture_filter : uint8_t
{
	classic,
	upscale,
	trilinear,
};

#define OGL_FLAG_MIPMAP (1 << 0)
#define OGL_FLAG_NOCOLOR (1 << 1)
#define OGL_FLAG_ALPHA (1 << 31) // not required for ogl_loadbmtexture, since it uses the BM_FLAG_TRANSPARENT, but is needed for ogl_init_texture.
void ogl_loadbmtexture_f(grs_bitmap &bm, opengl_texture_filter texfilt, bool texanis, bool edgepad);
void ogl_freebmtexture(grs_bitmap &bm);

void ogl_start_frame(grs_canvas &);
void ogl_end_frame(void);
void ogl_set_screen_mode(void);

struct ogl_colors
{
	using array_type = std::array<GLfloat, 16>;
	static const array_type white;
	const array_type &init(int c)
	{
		return
#ifdef DXX_CONSTANT_TRUE
			DXX_CONSTANT_TRUE(c == -1)
			? white
			: DXX_CONSTANT_TRUE(c != -1)
				? init_palette(c)
				:
#endif
		init_maybe_white(c);
	}
	const array_type &init_maybe_white(int c);
	const array_type &init_palette(unsigned c);
private:
	array_type a;
};

void ogl_urect(grs_canvas &, int left, int top, int right, int bot, color_palette_index color);
bool ogl_ubitmapm_cs(grs_canvas &, int x, int y,int dw, int dh, grs_bitmap &bm,int c, int scale);
bool ogl_ubitmapm_cs(grs_canvas &, int x, int y,int dw, int dh, grs_bitmap &bm, const ogl_colors::array_type &c, int scale);
bool ogl_ubitblt_i(unsigned dw, unsigned dh, unsigned dx, unsigned dy, unsigned sw, unsigned sh, unsigned sx, unsigned sy, const grs_bitmap &src, grs_bitmap &dest, opengl_texture_filter texfilt);
bool ogl_ubitblt(unsigned w, unsigned h, unsigned dx, unsigned dy, unsigned sx, unsigned sy, const grs_bitmap &src, grs_bitmap &dest);
void ogl_upixelc(const grs_bitmap &, unsigned x, unsigned y, color_palette_index c);
color_palette_index ogl_ugpixel(const grs_bitmap &bitmap, unsigned x, unsigned y);
void ogl_ulinec(grs_canvas &, int left, int top, int right, int bot, int c);
}
#ifdef dsx
namespace dsx {
void ogl_cache_level_textures();
}
#endif

void _g3_draw_tmap_2(grs_canvas &, unsigned nv, const g3s_point *const *const pointlist, const g3s_uvl *uvl_list, const g3s_lrgb *light_rgb, grs_bitmap &bmbot, grs_bitmap &bm, texture2_rotation_low orient);

template <std::size_t N>
static inline void g3_draw_tmap_2(grs_canvas &canvas, const unsigned nv, const std::array<cg3s_point *, N> &pointlist, const std::array<g3s_uvl, N> &uvl_list, const std::array<g3s_lrgb, N> &light_rgb, grs_bitmap &bmbot, grs_bitmap &bm, const texture2_rotation_low orient)
{
	static_assert(N <= MAX_POINTS_PER_POLY, "too many points in tmap");
#ifdef DXX_CONSTANT_TRUE
	if (DXX_CONSTANT_TRUE(nv > N))
		DXX_ALWAYS_ERROR_FUNCTION(dxx_trap_tmap_overread, "reading beyond array");
#endif
	if (nv > N)
		return;
	_g3_draw_tmap_2(canvas, nv, &pointlist[0], &uvl_list[0], &light_rgb[0], bmbot, bm, orient);
}

void ogl_draw_vertex_reticle(int cross,int primary,int secondary,int color,int alpha,int size_offs);
namespace dcx {
void ogl_toggle_depth_test(int enable);
void ogl_set_blending(gr_blend);
unsigned pow2ize(unsigned x);//from ogl.c
}

#endif

#endif /* _OGL_INIT_H_ */
