/* interface to OpenGL functions
 * Added 9/15/99 Matthew Mueller
 * Got rid of OpenGL-internal stuff 2004-5-16 Martin Schaffner
 */

#ifndef _OGL_INIT_H_ 
#define _OGL_INIT_H_

#ifdef _MSC_VER
# undef MAC // dirty feckin hack
#include <windows.h>
#include <stddef.h>
# define MAC(x)
#endif

//#ifdef _WIN32
//#define OGL_RUNTIME_LOAD
//#endif

#ifdef OGL_RUNTIME_LOAD
#include "loadgl.h"
int ogl_init_load_library(void);
#else
#if defined(__APPLE__) && defined(__MACH__)
#include <OpenGL/gl.h>
#else
#include <GL/gl.h>
#endif
//######hack, since multi texture support is not working
#undef GL_ARB_multitexture
#undef GL_SGIS_multitexture
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

extern int ogl_alttexmerge;//merge textures by just printing the seperate textures?
extern int ogl_rgba_format;
extern int ogl_setgammaramp_ok;
extern int ogl_intensity4_ok;
extern int ogl_luminance4_alpha4_ok;
extern int ogl_rgba2_ok;
extern int ogl_readpixels_ok;
extern int ogl_gettexlevelparam_ok;

extern int gl_initialized;
extern int GL_texmagfilt,GL_texminfilt,GL_needmipmaps;
extern int gl_reticle;

int ogl_check_mode(int x, int y); // check if mode is valid
int ogl_init_window(int x, int y);//create a window/switch modes/etc
void ogl_destroy_window(void);//destroy window/etc
void ogl_init(void);//one time initialization
void ogl_close(void);//one time shutdown

void ogl_loadbmtexture_m(grs_bitmap *bm,int domipmap);
void ogl_freebmtexture(grs_bitmap *bm);

void ogl_start_offscreen_render(int x, int y, int w, int h);
void ogl_end_offscreen_render(void);
void ogl_start_frame(void);
void ogl_end_frame(void);
void ogl_swap_buffers(void);
void ogl_set_screen_mode(void);
void ogl_cache_level_textures(void);

void ogl_urect(int left, int top, int right, int bot);
bool ogl_ubitmapm_c(int x, int y, grs_bitmap *bm, int c);
bool ogl_ubitmapm(int x, int y, grs_bitmap *bm);
bool ogl_ubitblt_i(int dw, int dh, int dx, int dy, int sw, int sh, int sx, int sy, grs_bitmap * src, grs_bitmap * dest);
bool ogl_ubitblt(int w, int h, int dx, int dy, int sx, int sy, grs_bitmap * src, grs_bitmap * dest);
bool ogl_ubitblt_tolinear(int w, int h, int dx, int dy, int sx, int sy, grs_bitmap * src, grs_bitmap * dest);
bool ogl_ubitblt_copy(int w, int h, int dx, int dy, int sx, int sy, grs_bitmap * src, grs_bitmap * dest);
void ogl_upixelc(int x, int y, int c);
void ogl_ulinec(int left, int top, int right, int bot, int c);

#include "3d.h"
bool g3_draw_tmap_2(int nv,g3s_point **pointlist,g3s_uvl *uvl_list,grs_bitmap *bmbot,grs_bitmap *bm, int orient);

void ogl_draw_reticle(int cross, int primary, int secondary);

#endif /* _OGL_INIT_H_ */
