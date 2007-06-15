//prototypes opengl functions - Added 9/15/99 Matthew Mueller
#ifndef _OGL_INIT_H_
#define _OGL_INIT_H_

#ifdef __WINDOWS__
#include <windows.h>
#include <stddef.h>
#endif

#ifdef OGL_RUNTIME_LOAD
#include "loadgl.h"
int ogl_init_load_library(void);
#else
#include <GL/gl.h>
#endif

#ifndef GL_VERSION_1_1
#ifdef GL_EXT_texture
#define GL_INTENSITY4 GL_INTENSITY4_EXT
#define GL_INTENSITY8 GL_INTENSITY8_EXT
#endif
#endif

#include "gr.h"
#include "palette.h"
#include "types.h"


#define OGL_TEXTURE_LIST_SIZE 20000

#define OGL_FLAG_MIPMAP (1 << 0)
#define OGL_FLAG_NOCOLOR (1 << 1)
#define OGL_FLAG_ALPHA (1 << 31) // not required for ogl_loadbmtexture, since it uses the BM_FLAG_TRANSPARENT, but is needed for ogl_init_texture.

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

extern ogl_texture ogl_texture_list[OGL_TEXTURE_LIST_SIZE];

extern int ogl_mem_target;
ogl_texture* ogl_get_free_texture(void);
void ogl_init_texture(ogl_texture* t, int w, int h, int flags);
void ogl_init_texture_list_internal(void);
void ogl_smash_texture_list_internal(void);
void ogl_vivify_texture_list_internal(void);

extern int ogl_fullscreen;

extern int ogl_setgammaramp_ok;
extern int ogl_brightness_ok;
extern int ogl_brightness_r, ogl_brightness_g, ogl_brightness_b;
int ogl_setbrightness_internal(void);

void ogl_do_fullscreen_internal(void);

extern int ogl_voodoohack;
extern int ogl_alttexmerge;//merge textures by just printing the seperate textures?
extern int ogl_rgba_format;
extern int ogl_intensity4_ok;
extern int ogl_luminance4_alpha4_ok;
extern int ogl_rgba2_ok;
extern int ogl_readpixels_ok;
extern int ogl_gettexlevelparam_ok;
extern int gl_initialized;
extern int GL_texmagfilt,GL_texminfilt,GL_needmipmaps;
extern int gl_reticle;
extern int ogl_scissor_ok;

#ifndef GL_EXT_texture_filter_anisotropic
#define GL_EXT_texture_filter_anisotropic 1
#define GL_TEXTURE_MAX_ANISOTROPY_EXT     0x84FE
#define GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT 0x84FF
#endif
extern int ogl_ext_texture_filter_anisotropic_ok;
extern float GL_texanisofilt;

extern int GL_TEXTURE_2D_enabled;
#define OGL_ENABLE2(a,f) {if (a ## _enabled!=1) {f;a ## _enabled=1;}}
#define OGL_DISABLE2(a,f) {if (a ## _enabled!=0) {f;a ## _enabled=0;}}
#define OGL_ENABLE(a) OGL_ENABLE2(GL_ ## a,glEnable(GL_ ## a))
#define OGL_DISABLE(a) OGL_DISABLE2(GL_ ## a,glDisable(GL_ ## a))

extern int last_width,last_height;
#define OGL_VIEWPORT(x,y,w,h){if (w!=last_width || h!=last_height){glViewport(x,grd_curscreen->sc_canvas.cv_bitmap.bm_h-y-h,w,h);last_width=w;last_height=h;}}

//platform specific funcs
//MSVC seems to have problems with inline funcs not being found during linking
#ifndef _MSC_VER
inline 
#endif
void ogl_swap_buffers_internal(void);
int ogl_init_window(int x, int y);//create a window/switch modes/etc
void ogl_destroy_window(void);//destroy window/etc
void ogl_init(void);//one time initialization
void ogl_close(void);//one time shutdown

//generic funcs
void ogl_loadbmtexture_f(grs_bitmap *bm, int flags);
void ogl_loadbmtexture(grs_bitmap *bm);
void ogl_freetexture(ogl_texture *gltexture);
void ogl_freebmtexture(grs_bitmap *bm);
void ogl_do_palfx(void);
void ogl_start_frame(void);
void ogl_end_frame(void);
void ogl_set_screen_mode(void);
void ogl_cache_level_textures(void);
void ogl_urect(int left,int top,int right,int bot);
bool ogl_ubitmapm_c(int x, int y,grs_bitmap *bm,int c);
bool ogl_ubitmapm_cs(int x, int y,int dw, int dh, grs_bitmap *bm,int c, int scale);
bool ogl_ubitmapm(int x, int y,grs_bitmap *bm);
bool ogl_ubitblt_i(int dw,int dh,int dx,int dy, int sw, int sh, int sx, int sy, grs_bitmap * src, grs_bitmap * dest, int mipmap);
bool ogl_ubitblt(int w,int h,int dx,int dy, int sx, int sy, grs_bitmap * src, grs_bitmap * dest);
void ogl_upixelc(int x, int y, int c);
void ogl_ulinec(int left,int top,int right,int bot,int c);

extern unsigned char *ogl_pal;

#include "3d.h"
bool g3_draw_tmap_2(int nv,g3s_point **pointlist,g3s_uvl *uvl_list,grs_bitmap *bmbot,grs_bitmap *bm,int orient);
void ogl_draw_reticle(int cross,int primary,int secondary);

#define CPAL2Tr(c) ((gr_current_pal[c*3])/63.0)
#define CPAL2Tg(c) ((gr_current_pal[c*3+1])/63.0)
#define CPAL2Tb(c) ((gr_current_pal[c*3+2])/63.0)
#define PAL2Tr(c) ((ogl_pal[c*3])/63.0)
#define PAL2Tg(c) ((ogl_pal[c*3+1])/63.0)
#define PAL2Tb(c) ((ogl_pal[c*3+2])/63.0)

#endif
