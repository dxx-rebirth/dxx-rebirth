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
#define GL_GLEXT_LEGACY
#undef GL_ARB_multitexture
#if defined(__APPLE__) && defined(__MACH__)
#include <OpenGL/gl.h>
#else
#include <GL/gl.h>
#endif
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
	int wrapstate[2];
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

#ifndef EXT_texture_env_combine
#define EXT_texture_env_combine 1
#define GL_COMBINE_RGB_EXT                0x8571
#define GL_COMBINE_ALPHA_EXT              0x8572
#define GL_PRIMARY_COLOR_EXT              0x8577
#define GL_PREVIOUS_EXT                   0x8578
#define GL_SOURCE0_RGB_EXT                0x8580
#define GL_SOURCE1_RGB_EXT                0x8581
#define GL_SOURCE2_RGB_EXT                0x8582
#define GL_SOURCE0_ALPHA_EXT              0x8588
#define GL_SOURCE1_ALPHA_EXT              0x8589
#define GL_SOURCE2_ALPHA_EXT              0x858A
#define GL_OPERAND0_RGB_EXT               0x8590
#define GL_OPERAND1_RGB_EXT               0x8591
#define GL_OPERAND2_RGB_EXT               0x8592
#define GL_OPERAND0_ALPHA_EXT             0x8598
#define GL_OPERAND1_ALPHA_EXT             0x8599
#define GL_OPERAND2_ALPHA_EXT             0x859A
#endif

#ifndef GL_NV_texture_env_combine4
#define GL_NV_texture_env_combine4 1
#define GL_COMBINE4_NV                    0x8503
#define GL_SOURCE3_RGB_NV                 0x8583
#define GL_SOURCE3_ALPHA_NV               0x858B
#define GL_OPERAND3_RGB_NV                0x8593
#define GL_OPERAND3_ALPHA_NV              0x859B
#endif
extern int ogl_nv_texture_env_combine4_ok;

#ifndef GL_EXT_texture_filter_anisotropic
#define GL_EXT_texture_filter_anisotropic 1
#define GL_TEXTURE_MAX_ANISOTROPY_EXT     0x84FE
#define GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT 0x84FF
#endif
extern int ogl_ext_texture_filter_anisotropic_ok;

extern int gl_initialized;
extern int GL_texmagfilt,GL_texminfilt,GL_needmipmaps;
extern float GL_texanisofilt;
extern int gl_reticle;

extern int active_texture_unit;
void ogl_setActiveTexture(int t);

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
