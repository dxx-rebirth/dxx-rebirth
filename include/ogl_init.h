/* interface to OpenGL functions
 * Added 9/15/99 Matthew Mueller
 * Got rid of OpenGL-internal stuff 2004-5-16 Martin Schaffner
 */

#ifndef _OGL_INIT_H_ 
#define _OGL_INIT_H_

#ifdef _MSC_VER
#include <windows.h>
#include <stddef.h>
#endif

#ifdef _WIN32
#include "loadgl.h"
int ogl_init_load_library(void);
#else
# define GL_GLEXT_LEGACY
# if defined(__APPLE__) && defined(__MACH__)
#  include <OpenGL/gl.h>
#  include <OpenGL/glu.h>
# else
#  define GL_GLEXT_PROTOTYPES
#  ifdef OGLES
#  include <GLES/gl.h>
#  else
#  include <GL/gl.h>
#  endif
# endif
# ifndef GL_CLAMP_TO_EDGE	// hack for Mac OS 9, others?
#  define GL_CLAMP_TO_EDGE GL_CLAMP
# endif
#endif

#include "gr.h"
#include "palette.h"
#include "pstypes.h"

#ifndef GL_VERSION_1_1
#ifdef GL_EXT_texture
#define GL_INTENSITY4 GL_INTENSITY4_EXT
#define GL_INTENSITY8 GL_INTENSITY8_EXT
#endif
#endif

#ifndef GL_TEXTURE_MAX_ANISOTROPY_EXT
#define GL_TEXTURE_MAX_ANISOTROPY_EXT 0x84FE
#endif
#ifndef GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT
#define GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT 0x84FF
#endif

/* we need to export ogl_texture for 2d/font.c */
typedef struct _ogl_texture {
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
} ogl_texture;

extern ogl_texture* ogl_get_free_texture();
void ogl_init_texture(ogl_texture* t, int w, int h, int flags);

extern int ogl_rgba_internalformat;
extern int ogl_rgb_internalformat;

void ogl_init_shared_palette(void);

extern int gl_initialized;

extern int active_texture_unit;
extern GLfloat ogl_maxanisotropy;

void ogl_setActiveTexture(int t);

int ogl_init_window(int x, int y);//create a window/switch modes/etc

#define OGL_FLAG_MIPMAP (1 << 0)
#define OGL_FLAG_NOCOLOR (1 << 1)
#define OGL_FLAG_ALPHA (1 << 31) // not required for ogl_loadbmtexture, since it uses the BM_FLAG_TRANSPARENT, but is needed for ogl_init_texture.
void ogl_loadbmtexture_f(grs_bitmap *bm, int texfilt);
void ogl_freebmtexture(grs_bitmap *bm);

void ogl_start_frame(void);
void ogl_end_frame(void);
void ogl_swap_buffers_internal(void);
void ogl_set_screen_mode(void);
void ogl_cache_level_textures(void);

void ogl_urect(int left, int top, int right, int bot);
bool ogl_ubitmapm_cs(int x, int y,int dw, int dh, grs_bitmap *bm,int c, int scale);
bool ogl_ubitblt_i(int dw, int dh, int dx, int dy, int sw, int sh, int sx, int sy, grs_bitmap * src, grs_bitmap * dest, int texfilt);
bool ogl_ubitblt(int w, int h, int dx, int dy, int sx, int sy, grs_bitmap * src, grs_bitmap * dest);
void ogl_upixelc(int x, int y, int c);
unsigned char ogl_ugpixel( grs_bitmap * bitmap, int x, int y );
void ogl_ulinec(int left, int top, int right, int bot, int c);

#include "3d.h"
bool g3_draw_tmap_2(int nv,const g3s_point **pointlist,g3s_uvl *uvl_list,g3s_lrgb *light_rgb, grs_bitmap *bmbot,grs_bitmap *bm, int orient);

void ogl_draw_vertex_reticle(int cross,int primary,int secondary,int color,int alpha,int size_offs);
void ogl_toggle_depth_test(int enable);
void ogl_set_blending();
int pow2ize(int x);//from ogl.c

#endif /* _OGL_INIT_H_ */
