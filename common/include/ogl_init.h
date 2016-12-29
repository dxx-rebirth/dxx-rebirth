/*
 * This file is part of the DXX-Rebirth project <http://www.dxx-rebirth.com/>.
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

#ifdef _WIN32
#include "loadgl.h"
#else
# define GL_GLEXT_LEGACY
# if defined(__APPLE__) && defined(__MACH__)
#  include <OpenGL/gl.h>
#  include <OpenGL/glu.h>
# else
#  define GL_GLEXT_PROTOTYPES
#  if DXX_USE_OGLES
#  include <GLES/gl.h>
#  else
#  include <GL/gl.h>
#  endif
# endif
# ifndef GL_CLAMP_TO_EDGE	// hack for Mac OS 9, others?
#  define GL_CLAMP_TO_EDGE GL_CLAMP
# endif
#endif

#include "dsx-ns.h"
#include "fwd-gr.h"
#include "palette.h"
#include "pstypes.h"
#include "3d.h"
#include "compiler-array.h"
#include "ogl_extensions.h"

#define OGL_TEXFILT_CLASSIC  0
#define OGL_TEXFILT_UPSCALE  1
#define OGL_TEXFILT_TRLINEAR 2

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

#define OGL_FLAG_MIPMAP (1 << 0)
#define OGL_FLAG_NOCOLOR (1 << 1)
#define OGL_FLAG_ALPHA (1 << 31) // not required for ogl_loadbmtexture, since it uses the BM_FLAG_TRANSPARENT, but is needed for ogl_init_texture.
void ogl_loadbmtexture_f(grs_bitmap &bm, int texfilt, bool texanis, bool edgepad);
void ogl_freebmtexture(grs_bitmap &bm);

void ogl_start_frame(void);
void ogl_end_frame(void);
void ogl_set_screen_mode(void);

struct ogl_colors
{
	using array_type = array<GLfloat, 16>;
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

void ogl_urect(grs_canvas &, int left, int top, int right, int bot, int color);
bool ogl_ubitmapm_cs(grs_canvas &, int x, int y,int dw, int dh, grs_bitmap &bm,int c, int scale);
bool ogl_ubitmapm_cs(grs_canvas &, int x, int y,int dw, int dh, grs_bitmap &bm, const ogl_colors::array_type &c, int scale);
bool ogl_ubitblt_i(unsigned dw, unsigned dh, unsigned dx, unsigned dy, unsigned sw, unsigned sh, unsigned sx, unsigned sy, const grs_bitmap &src, grs_bitmap &dest, unsigned texfilt);
bool ogl_ubitblt(unsigned w, unsigned h, unsigned dx, unsigned dy, unsigned sx, unsigned sy, const grs_bitmap &src, grs_bitmap &dest);
void ogl_upixelc(const grs_canvas &, unsigned x, unsigned y, unsigned c);
unsigned char ogl_ugpixel(const grs_bitmap &bitmap, unsigned x, unsigned y);
void ogl_ulinec(grs_canvas &, int left, int top, int right, int bot, int c);
}
#ifdef dsx
namespace dsx {
void ogl_cache_level_textures();
}
#endif

#include "3d.h"
void _g3_draw_tmap_2(unsigned nv, const g3s_point *const *const pointlist, const g3s_uvl *uvl_list, const g3s_lrgb *light_rgb, grs_bitmap *bmbot, grs_bitmap *bm, int orient);

template <std::size_t N>
static inline void g3_draw_tmap_2(unsigned nv, const array<cg3s_point *, N> &pointlist, const array<g3s_uvl, N> &uvl_list, const array<g3s_lrgb, N> &light_rgb, grs_bitmap *bmbot, grs_bitmap *bm, int orient)
{
	static_assert(N <= MAX_POINTS_PER_POLY, "too many points in tmap");
#ifdef DXX_CONSTANT_TRUE
	if (DXX_CONSTANT_TRUE(nv > N))
		DXX_ALWAYS_ERROR_FUNCTION(dxx_trap_tmap_overread, "reading beyond array");
#endif
	_g3_draw_tmap_2(nv, &pointlist[0], &uvl_list[0], &light_rgb[0], bmbot, bm, orient);
}

void ogl_draw_vertex_reticle(int cross,int primary,int secondary,int color,int alpha,int size_offs);
namespace dcx {
void ogl_toggle_depth_test(int enable);
void ogl_set_blending(uint8_t);
unsigned pow2ize(unsigned x);//from ogl.c
}

#endif

#endif /* _OGL_INIT_H_ */
