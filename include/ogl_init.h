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
#  include <GL/gl.h>
# endif
# ifndef GL_CLAMP_TO_EDGE	// hack for Mac OS 9, others?
#  define GL_CLAMP_TO_EDGE GL_CLAMP
# endif
#endif

#ifndef GL_VERSION_1_1
#ifdef GL_EXT_texture
#define GL_INTENSITY4 GL_INTENSITY4_EXT
#define GL_INTENSITY8 GL_INTENSITY8_EXT
#endif
#endif

#include "gr.h"
#include "palette.h"
#include "pstypes.h"

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
	fix lastrend;
	unsigned long numrend;
	char wantmip;
} ogl_texture;

extern ogl_texture* ogl_get_free_texture();
void ogl_init_texture(ogl_texture* t, int w, int h, int flags);

extern int ogl_rgba_internalformat;
extern int ogl_rgb_internalformat;

void ogl_init_shared_palette(void);

extern int gl_initialized;
extern int GL_needmipmaps;
extern float OglTexMagFilt;
extern float OglTexMinFilt;

extern int active_texture_unit;
void ogl_setActiveTexture(int t);

int ogl_init_window(int x, int y);//create a window/switch modes/etc

#define OGL_FLAG_MIPMAP (1 << 0)
#define OGL_FLAG_NOCOLOR (1 << 1)
#define OGL_FLAG_ALPHA (1 << 31) // not required for ogl_loadbmtexture, since it uses the BM_FLAG_TRANSPARENT, but is needed for ogl_init_texture.
void ogl_loadbmtexture_f(grs_bitmap *bm, int flags);
void ogl_freebmtexture(grs_bitmap *bm);

void ogl_start_frame(void);
void ogl_end_frame(void);
void ogl_swap_buffers_internal(void);
void ogl_set_screen_mode(void);
void ogl_cache_level_textures(void);

void ogl_urect(int left, int top, int right, int bot);
bool ogl_ubitmapm_c(int x, int y, grs_bitmap *bm, int c);
bool ogl_ubitmapm_cs(int x, int y,int dw, int dh, grs_bitmap *bm,int c, int scale);
bool ogl_ubitmapm(int x, int y, grs_bitmap *bm);
bool ogl_ubitblt_i(int dw, int dh, int dx, int dy, int sw, int sh, int sx, int sy, grs_bitmap * src, grs_bitmap * dest, int mipmap);
bool ogl_ubitblt(int w, int h, int dx, int dy, int sx, int sy, grs_bitmap * src, grs_bitmap * dest);
void ogl_upixelc(int x, int y, int c);
void ogl_ulinec(int left, int top, int right, int bot, int c);

#include "3d.h"
bool g3_draw_tmap_2(int nv,g3s_point **pointlist,g3s_uvl *uvl_list,grs_bitmap *bmbot,grs_bitmap *bm, int orient);

void ogl_draw_reticle(int cross, int primary, int secondary);

#endif /* _OGL_INIT_H_ */
