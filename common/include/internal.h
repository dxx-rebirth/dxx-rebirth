/*
 * This file is part of the DXX-Rebirth project <https://www.dxx-rebirth.com/>.
 * It is copyright by its individual contributors, as recorded in the
 * project's Git history.  See COPYING.txt at the top level for license
 * terms and a link to the Git history.
 */
/* prototypes for function calls between files within the OpenGL module */

#pragma once

#include "dxxsconf.h"
#if DXX_USE_OGL
#include "ogl_init.h" // interface to OpenGL module
#include "ogl_extensions.h"
#include "gr.h"

namespace dcx {
void ogl_init_texture_list_internal(void);
void ogl_smash_texture_list_internal(void);

extern int linedotscale;

extern int GL_TEXTURE_2D_enabled;

static constexpr const rgb_t &CPAL2T(const color_palette_index c)
{
	return gr_current_pal[static_cast<std::size_t>(c)];
}

static constexpr GLfloat CPAL2Tr(const color_palette_index c)
{
	return CPAL2T(c).r / 63.0;
}

static constexpr GLfloat CPAL2Tg(const color_palette_index c)
{
	return CPAL2T(c).g / 63.0;
}

static constexpr GLfloat CPAL2Tb(const color_palette_index c)
{
	return CPAL2T(c).b / 63.0;
}

}

#define OGL_SET_FEATURE_STATE(G,V,F)	static_cast<void>(G != V && (G = V, F, 0))
#define OGL_ENABLE(a)	OGL_SET_FEATURE_STATE(GL_##a##_enabled, 1, glEnable(GL_##a))
#define OGL_DISABLE(a)	OGL_SET_FEATURE_STATE(GL_##a##_enabled, 0, glDisable(GL_##a))

//#define OGL_TEXCLAMP() OGL_ENABLE2(GL_texclamp,glTexParameteri(GL_TEXTURE_2D,  GL_TEXTURE_WRAP_S, GL_CLAMP);glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T,    GL_CLAMP);)
//#define OGL_TEXREPEAT() OGL_DISABLE2(GL_texclamp,glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);)
//#define OGL_SETSTATE(a,s,f) {if (a ## _state!=s) {f;a ## _state=s;}}
//#define OGL_TEXENV(p,m) OGL_SETSTATE(p,m,glTexEnvi(GL_TEXTURE_ENV, p,m));
//#define OGL_TEXPARAM(p,m) OGL_SETSTATE(p,m,glTexParameteri(GL_TEXTURE_2D,p,m))

namespace dcx {
extern unsigned last_width,last_height;
static inline void OGL_VIEWPORT(const unsigned x, const unsigned y, const unsigned w, const unsigned h)
{
	if (w!=last_width || h!=last_height)
	{
		last_width = w;
		last_height = h;
		glViewport(x,grd_curscreen->sc_canvas.cv_bitmap.bm_h-y-h,w,h);
	}
}

#ifdef DXX_BUILD_DESCENT
//platform specific funcs
void ogl_swap_buffers_internal();
#endif
}

#endif
