/* prototypes for function calls between files within the OpenGL module */

#ifndef _INTERNAL_H_
#define _INTERNAL_H_

#include "ogl_init.h" // interface to OpenGL module

/* I assume this ought to be >= MAX_BITMAP_FILES in piggy.h? */
#define OGL_TEXTURE_LIST_SIZE 20000

extern ogl_texture ogl_texture_list[OGL_TEXTURE_LIST_SIZE];

void ogl_init_texture_list_internal(void);
void ogl_smash_texture_list_internal(void);
void ogl_vivify_texture_list_internal(void);

extern int ogl_brightness_ok;
extern int ogl_brightness_r, ogl_brightness_g, ogl_brightness_b;
extern int ogl_fullscreen;

extern int GL_TEXTURE_2D_enabled;
#define OGL_ENABLE2(a,f) {if (a ## _enabled!=1) {f;a ## _enabled=1;}}
#define OGL_DISABLE2(a,f) {if (a ## _enabled!=0) {f;a ## _enabled=0;}}

//#define OGL_ENABLE(a) OGL_ENABLE2(a,glEnable(a))
//#define OGL_DISABLE(a) OGL_DISABLE2(a,glDisable(a))
#define OGL_ENABLE(a) OGL_ENABLE2(GL_ ## a,glEnable(GL_ ## a))
#define OGL_DISABLE(a) OGL_DISABLE2(GL_ ## a,glDisable(GL_ ## a))

//#define OGL_TEXCLAMP() OGL_ENABLE2(GL_texclamp,glTexParameteri(GL_TEXTURE_2D,  GL_TEXTURE_WRAP_S, GL_CLAMP);glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T,    GL_CLAMP);)
//#define OGL_TEXREPEAT() OGL_DISABLE2(GL_texclamp,glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);)
//#define OGL_SETSTATE(a,s,f) {if (a ## _state!=s) {f;a ## _state=s;}}
//#define OGL_TEXENV(p,m) OGL_SETSTATE(p,m,glTexEnvi(GL_TEXTURE_ENV, p,m));
//#define OGL_TEXPARAM(p,m) OGL_SETSTATE(p,m,glTexParameteri(GL_TEXTURE_2D,p,m))

extern int last_width,last_height;
#define OGL_VIEWPORT(x,y,w,h){if (w!=last_width || h!=last_height){glViewport(x,grd_curscreen->sc_canvas.cv_bitmap.bm_h-y-h,w,h);last_width=w;last_height=h;}}

//platform specific funcs
extern void ogl_swap_buffers_internal(void);

extern unsigned char *ogl_pal;

//whee
//#define PAL2Tr(c) ((gr_palette[c*3]+gr_palette_gamma)/63.0)
//#define PAL2Tg(c) ((gr_palette[c*3+1]+gr_palette_gamma)/63.0)
//#define PAL2Tb(c) ((gr_palette[c*3+2]+gr_palette_gamma)/63.0)
//#define PAL2Tr(c) ((gr_palette[c*3])/63.0)
//#define PAL2Tg(c) ((gr_palette[c*3+1])/63.0)
//#define PAL2Tb(c) ((gr_palette[c*3+2])/63.0)
#define CPAL2Tr(c) ((gr_current_pal[c*3])/63.0)
#define CPAL2Tg(c) ((gr_current_pal[c*3+1])/63.0)
#define CPAL2Tb(c) ((gr_current_pal[c*3+2])/63.0)
#define PAL2Tr(c) ((ogl_pal[c*3])/63.0)
#define PAL2Tg(c) ((ogl_pal[c*3+1])/63.0)
#define PAL2Tb(c) ((ogl_pal[c*3+2])/63.0)
//inline GLfloat PAL2Tr(int c);
//inline GLfloat PAL2Tg(int c);
//inline GLfloat PAL2Tb(int c);

#endif // _INTERNAL_H_
