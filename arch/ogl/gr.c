/* $Id: gr.c,v 1.1.1.1 2006/03/17 19:53:31 zicodxx Exp $ */
/*
 *
 * OGL video functions. - Added 9/15/99 Matthew Mueller
 *
 *
 */

#ifdef HAVE_CONFIG_H
#include <conf.h>
#endif

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <limits.h>
#ifdef _MSC_VER
#include <windows.h>
#endif

#if !defined(_MSC_VER) && !defined(macintosh)
#include <unistd.h>
#endif
#if !defined(macintosh)
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#endif

#include <errno.h>

#include "hudmsg.h"
#include "game.h"
#include "text.h"
#include "gr.h"
#include "gamefont.h"
#include "grdef.h"
#include "palette.h"
#include "u_mem.h"
#include "error.h"
#include "inferno.h"
#include "screens.h"
#include "strutil.h"
#include "mono.h"
#include "args.h"
#include "key.h"
#include "physfsx.h"
#include "internal.h"


#define DECLARE_VARS

#if defined(__APPLE__) && defined(__MACH__)
#include <OpenGL/glu.h>
#else
#include <GL/glu.h>
#endif

int ogl_voodoohack=0;
int gr_installed = 0;
int gl_initialized=0;
int gl_reticle = 0;
int ogl_fullscreen;
int ogl_scissor_ok=1;
int glalpha_effects=0;

void gr_palette_clear(); // Function prototype for gr_init;

int gr_check_fullscreen(void){
	return ogl_fullscreen;
}

void gr_do_fullscreen(int f){
	if (ogl_voodoohack)
		ogl_fullscreen=1;//force fullscreen mode on voodoos.
	else
		ogl_fullscreen=f;
	if (gl_initialized){
		ogl_do_fullscreen_internal();
	}
}

int gr_toggle_fullscreen(void){
	gr_do_fullscreen(!ogl_fullscreen);

	if (gl_initialized && Screen_mode != SCREEN_GAME) // update viewing values for menus
	{
		glMatrixMode(GL_PROJECTION);
		glLoadIdentity();
		glOrtho(0.0, 1.0, 0.0, 1.0, -1.0, 1.0);
		glMatrixMode(GL_MODELVIEW);
		glLoadIdentity();//clear matrix
		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	}

	return ogl_fullscreen;
}

int arch_toggle_fullscreen_menu(void){
	unsigned char *buf=NULL;

	if (ogl_readpixels_ok){
		MALLOC(buf,unsigned char,grd_curscreen->sc_w*grd_curscreen->sc_h*3);
		glReadBuffer(GL_FRONT);
		glReadPixels(0,0,grd_curscreen->sc_w,grd_curscreen->sc_h,GL_RGB,GL_UNSIGNED_BYTE,buf);
	}

	gr_do_fullscreen(!ogl_fullscreen);

	if (ogl_readpixels_ok){
		glRasterPos2f(0,0);
		glDrawPixels(grd_curscreen->sc_w,grd_curscreen->sc_h,GL_RGB,GL_UNSIGNED_BYTE,buf);
		free(buf);
	}

	return ogl_fullscreen;
}

extern void ogl_init_pixel_buffers(int w, int h);
extern void ogl_close_pixel_buffers(void);

void ogl_init_state(void){
	/* select clearing (background) color   */
	glClearColor(0.0, 0.0, 0.0, 0.0);

	/* initialize viewing values */
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glOrtho(0.0, 1.0, 0.0, 1.0, -1.0, 1.0);
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();//clear matrix
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	gr_palette_step_up(0,0,0);//in case its left over from in game

	ogl_init_pixel_buffers(grd_curscreen->sc_w, grd_curscreen->sc_h);
}

void gr_update()
{
}

// Set the buffer to draw to. 0 is front, 1 is back
void gr_set_draw_buffer(int buf)
{
	glDrawBuffer((buf == 0) ? GL_FRONT : GL_BACK);
}

const char *gl_vendor, *gl_renderer, *gl_version, *gl_extensions;

void ogl_get_verinfo(void)
{
	long t;

	float anisotropic_max = 0;

	gl_vendor = (const char *) glGetString (GL_VENDOR);
	gl_renderer = (const char *) glGetString (GL_RENDERER);
	gl_version = (const char *) glGetString (GL_VERSION);
	gl_extensions = (const char *) glGetString (GL_EXTENSIONS);

	con_printf(CON_VERBOSE, "OpenGL: vendor: %s\nOpenGL: renderer: %s\nOpenGL: version: %s\n",gl_vendor,gl_renderer,gl_version);

	ogl_intensity4_ok = 1;
	ogl_luminance4_alpha4_ok = 1;
	ogl_rgba2_ok = 1;
	ogl_gettexlevelparam_ok = 1;
	ogl_setgammaramp_ok = 0;

#ifdef _WIN32
	dglMultiTexCoord2fARB = (glMultiTexCoord2fARB_fp)wglGetProcAddress("glMultiTexCoord2fARB");
	dglActiveTextureARB = (glActiveTextureARB_fp)wglGetProcAddress("glActiveTextureARB");
	dglMultiTexCoord2fSGIS = (glMultiTexCoord2fSGIS_fp)wglGetProcAddress("glMultiTexCoord2fSGIS");
	dglSelectTextureSGIS = (glSelectTextureSGIS_fp)wglGetProcAddress("glSelectTextureSGIS");
#endif

	ogl_ext_texture_filter_anisotropic_ok = (strstr(gl_extensions, "GL_EXT_texture_filter_anisotropic") != 0);
	if (ogl_ext_texture_filter_anisotropic_ok)
		glGetFloatv(GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT, &anisotropic_max);

	//add driver specific hacks here.  whee.
	if ((stricmp(gl_renderer,"Mesa NVIDIA RIVA 1.0\n")==0 || stricmp(gl_renderer,"Mesa NVIDIA RIVA 1.2\n")==0) && stricmp(gl_version,"1.2 Mesa 3.0")==0){
		ogl_intensity4_ok=0;//ignores alpha, always black background instead of transparent.
		ogl_readpixels_ok=0;//either just returns all black, or kills the X server entirely
		ogl_gettexlevelparam_ok=0;//returns random data..
	}
	if (stricmp(gl_vendor,"Matrox Graphics Inc.")==0){
		//displays garbage. reported by
		//  redomen@crcwnet.com (render="Matrox G400" version="1.1.3 5.52.015")
		//  orulz (Matrox G200)
		ogl_intensity4_ok=0;
	}
#ifdef macintosh
	if (stricmp(gl_renderer,"3dfx Voodoo 3")==0){ // strangely, includes Voodoo 2
		ogl_gettexlevelparam_ok=0; // Always returns 0
	}
#endif

	//allow overriding of stuff.
	if ((t=FindArg("-gl_intensity4_ok"))){
		ogl_intensity4_ok=atoi(Args[t+1]);
	}
	if ((t=FindArg("-gl_luminance4_alpha4_ok"))){
		ogl_luminance4_alpha4_ok=atoi(Args[t+1]);
	}
	if ((t=FindArg("-gl_rgba2_ok"))){
		ogl_rgba2_ok=atoi(Args[t+1]);
	}
	if ((t=FindArg("-gl_readpixels_ok"))){
		ogl_readpixels_ok=atoi(Args[t+1]);
	}
	if ((t=FindArg("-gl_gettexlevelparam_ok"))){
		ogl_gettexlevelparam_ok=atoi(Args[t+1]);
	}
	if ((t=FindArg("-gl_setgammaramp_ok")))
	{
		ogl_setgammaramp_ok = atoi(Args[t + 1]);
	}
	if ((t=FindArg("-gl_scissor_ok")))
	{
		ogl_scissor_ok = atoi(Args[t + 1]);
	}

	con_printf(CON_VERBOSE, "gl_intensity4:%i gl_luminance4_alpha4:%i gl_rgba2:%i gl_readpixels:%i gl_gettexlevelparam:%i gl_setgammaramp_ok:%i gl_ext_texture_filter_anisotropic:%i(%f max) gl_scissor:%i\n", ogl_intensity4_ok, ogl_luminance4_alpha4_ok, ogl_rgba2_ok, ogl_readpixels_ok, ogl_gettexlevelparam_ok, ogl_setgammaramp_ok, ogl_ext_texture_filter_anisotropic_ok, anisotropic_max,ogl_scissor_ok);
}


int gr_check_mode(u_int32_t mode)
{
	int w, h;

	w = SM_W(mode);
	h = SM_H(mode);
	return ogl_check_mode(w, h); // platform specific code
}


int gr_set_mode(u_int32_t mode)
{
	unsigned int w, h;
	int aw, ah;
	char *gr_bm_data;
	float awidth = 3, aheight = 4;
	int i, argnum = INT_MAX;

#ifdef NOGRAPH
	return 0;
#endif

	if (mode<=0)
		return 0;

	w=SM_W(mode);
	h=SM_H(mode);

	if ((i=FindResArg("aspect", &ah, &aw)) && (i < argnum)) { argnum = i; awidth=aw; aheight=ah; }
	
	gr_bm_data=(char *)grd_curscreen->sc_canvas.cv_bitmap.bm_data;//since we use realloc, we want to keep this pointer around.
	memset( grd_curscreen, 0, sizeof(grs_screen));
	grd_curscreen->sc_mode = mode;
	grd_curscreen->sc_w = w;
	grd_curscreen->sc_h = h;
	grd_curscreen->sc_aspect = fixdiv(grd_curscreen->sc_w*awidth,grd_curscreen->sc_h*aheight);
	grd_curscreen->sc_canvas.cv_bitmap.bm_x = 0;
	grd_curscreen->sc_canvas.cv_bitmap.bm_y = 0;
	grd_curscreen->sc_canvas.cv_bitmap.bm_w = w;
	grd_curscreen->sc_canvas.cv_bitmap.bm_h = h;
	grd_curscreen->sc_canvas.cv_bitmap.bm_rowsize = w;
	grd_curscreen->sc_canvas.cv_bitmap.bm_type = BM_OGL;
	grd_curscreen->sc_canvas.cv_bitmap.bm_data = d_realloc(gr_bm_data,w*h);
	gr_set_current_canvas(NULL);
	
	ogl_init_window(w,h);//platform specific code

	ogl_get_verinfo();

	OGL_VIEWPORT(0,0,w,h);

	ogl_init_state();
	
	gr_update();


	return 0;
}

#define GLstrcmptestr(a,b) if (stricmp(a,#b)==0 || stricmp(a,"GL_" #b)==0)return GL_ ## b;
int ogl_atotexfilti(char *a,int min){
	GLstrcmptestr(a,NEAREST);
	GLstrcmptestr(a,LINEAR);
	if (min){//mipmaps are valid only for the min filter
		GLstrcmptestr(a,NEAREST_MIPMAP_NEAREST);
		GLstrcmptestr(a,NEAREST_MIPMAP_LINEAR);
		GLstrcmptestr(a,LINEAR_MIPMAP_NEAREST);
		GLstrcmptestr(a,LINEAR_MIPMAP_LINEAR);
	}
	Error("unknown/invalid texture filter %s\n",a);
}

int ogl_testneedmipmaps(int i){
	switch (i){
		case GL_NEAREST:
		case GL_LINEAR:
			return 0;
		case GL_NEAREST_MIPMAP_NEAREST:
		case GL_NEAREST_MIPMAP_LINEAR:
		case GL_LINEAR_MIPMAP_NEAREST:
		case GL_LINEAR_MIPMAP_LINEAR:
			return 1;
	}
	Error("unknown texture filter %x\n",i);
}

#ifdef OGL_RUNTIME_LOAD
#ifdef _WIN32
char *OglLibPath="opengl32.dll";
#endif
#ifdef __unix__
char *OglLibPath="libGL.so";
#endif
#ifdef macintosh
char *OglLibPath = NULL;
#endif

int ogl_rt_loaded=0;
int ogl_init_load_library(void)
{
	int retcode=0;
	if (!ogl_rt_loaded){
		int t;
		if ((t=FindArg("-gl_library")))
			OglLibPath=Args[t+1];

		retcode = OpenGL_LoadLibrary(true);
		if(retcode)
		{
			mprintf((0,"Opengl loaded ok\n"));
	
			if(!glEnd)
			{
				Error("Opengl: Functions not imported\n");
			}
		}else{
			Error("Opengl: error loading %s\n", OglLibPath? OglLibPath : SDL_GetError());
		}
		ogl_rt_loaded=1;
	}
	return retcode;
}
#endif

int gr_init(int mode)
{
	int retcode, t, glt = 0;

 	// Only do this function once!
	if (gr_installed==1)
		return -1;

#ifdef OGL_RUNTIME_LOAD
	ogl_init_load_library();
#endif

#ifdef GR_SUPPORTS_FULLSCREEN_TOGGLE
	if (FindArg("-gl_voodoo")){
		ogl_voodoohack=1;
		gr_toggle_fullscreen();
	}

	if (!(FindArg("-window")))
		gr_toggle_fullscreen();
#endif
	if ((glt=FindArg("-gl_alttexmerge")))
		ogl_alttexmerge=1;
	if ((t=FindArg("-gl_stdtexmerge")))
		if (t>=glt)//allow overriding of earlier args
			ogl_alttexmerge=0;

	if ((glt = FindArg("-gl_16bittextures")))
	{
		ogl_rgba_internalformat = GL_RGB5_A1;
		ogl_rgb_internalformat = GL_RGB5;
	}

	if ((glt=FindArg("-gl_mipmap"))){
		GL_texmagfilt=GL_LINEAR;
		GL_texminfilt=GL_LINEAR_MIPMAP_NEAREST;
	}
	if ((glt=FindArg("-gl_trilinear")))
	{
		GL_texmagfilt = GL_LINEAR;
		GL_texminfilt = GL_LINEAR_MIPMAP_LINEAR;
	}
	if ((t=FindArg("-gl_simple"))){
		if (t>=glt){//allow overriding of earlier args
			glt=t;
			GL_texmagfilt=GL_NEAREST;
			GL_texminfilt=GL_NEAREST;
		}
	}
	if ((t=FindArg("-gl_texmagfilt")) || (t=FindArg("-gl_texmagfilter"))){
		if (t>=glt)//allow overriding of earlier args
			GL_texmagfilt=ogl_atotexfilti(Args[t+1],0);
	}
	if ((t=FindArg("-gl_texminfilt")) || (t=FindArg("-gl_texminfilter"))){
		if (t>=glt)//allow overriding of earlier args
			GL_texminfilt=ogl_atotexfilti(Args[t+1],1);
	}
	GL_needmipmaps=ogl_testneedmipmaps(GL_texminfilt);

	if ((t = FindArg("-gl_anisotropy")) || (t = FindArg("-gl_anisotropic")))
	{
		GL_texanisofilt=atof(Args[t + 1]);
	}

	mprintf((0,"gr_init: texmagfilt:%x texminfilt:%x needmipmaps=%i anisotropic:%f\n",GL_texmagfilt,GL_texminfilt,GL_needmipmaps,GL_texanisofilt));

	
	if ((t=FindArg("-gl_vidmem"))){
		ogl_mem_target=atoi(Args[t+1])*1024*1024;
	}
	if ((t=FindArg("-gl_reticle"))){
		gl_reticle=atoi(Args[t+1]);
	}
	if (FindArg("-gl_transparency"))
		glalpha_effects=1;

	ogl_init();//platform specific initialization

	ogl_init_texture_list_internal();
		
	MALLOC( grd_curscreen,grs_screen,1 );
	memset( grd_curscreen, 0, sizeof(grs_screen));
	grd_curscreen->sc_canvas.cv_bitmap.bm_data = NULL;

	// Set the mode.
	if ((retcode=gr_set_mode(mode)))
	{
		return retcode;
	}

	grd_curscreen->sc_canvas.cv_color = 0;
	grd_curscreen->sc_canvas.cv_drawmode = 0;
	grd_curscreen->sc_canvas.cv_font = NULL;
	grd_curscreen->sc_canvas.cv_font_fg_color = 0;
	grd_curscreen->sc_canvas.cv_font_bg_color = 0;
	gr_set_current_canvas( &grd_curscreen->sc_canvas );
	
	ogl_init_pixel_buffers(256, 128);	// for gamefont_init

	gr_installed = 1;
	
	atexit(gr_close);

	return 0;
}

void gr_close()
{
	ogl_brightness_r = ogl_brightness_g = ogl_brightness_b = 0;
	ogl_setbrightness_internal();

	ogl_close();//platform specific code
	if (grd_curscreen){
		if (grd_curscreen->sc_canvas.cv_bitmap.bm_data)
			d_free(grd_curscreen->sc_canvas.cv_bitmap.bm_data);
		d_free(grd_curscreen);
	}
	ogl_close_pixel_buffers();
#ifdef OGL_RUNTIME_LOAD
	if (ogl_rt_loaded)
		OpenGL_LoadLibrary(false);
#endif
}
extern int r_upixelc;
void ogl_upixelc(int x, int y, int c){
	r_upixelc++;
	OGL_DISABLE(TEXTURE_2D);
	glPointSize(grd_curscreen->sc_w/320);
	glBegin(GL_POINTS);
	glColor3f(CPAL2Tr(c),CPAL2Tg(c),CPAL2Tb(c));
	glVertex2f((x + grd_curcanv->cv_bitmap.bm_x + 0.5) / (float)last_width, 1.0 - (y + grd_curcanv->cv_bitmap.bm_y + 0.5) / (float)last_height);
	glEnd();
}

void ogl_urect(int left,int top,int right,int bot){
	GLfloat xo,yo,xf,yf;
	int c=COLOR;
	
	xo=(left+grd_curcanv->cv_bitmap.bm_x)/(float)last_width;
	xf = (right + 1 + grd_curcanv->cv_bitmap.bm_x) / (float)last_width;
	yo=1.0-(top+grd_curcanv->cv_bitmap.bm_y)/(float)last_height;
	yf = 1.0 - (bot + 1 + grd_curcanv->cv_bitmap.bm_y) / (float)last_height;

	OGL_DISABLE(TEXTURE_2D);
	if (Gr_scanline_darkening_level >= GR_FADE_LEVELS)
		glColor3f(CPAL2Tr(c), CPAL2Tg(c), CPAL2Tb(c));
	else
		glColor4f(CPAL2Tr(c), CPAL2Tg(c), CPAL2Tb(c), 1.0 - (float)Gr_scanline_darkening_level / ((float)GR_FADE_LEVELS - 1.0));
	glBegin(GL_QUADS);
	glVertex2f(xo,yo);
	glVertex2f(xo,yf);
	glVertex2f(xf,yf);
	glVertex2f(xf,yo);
	glEnd();
}

void ogl_ulinec(int left,int top,int right,int bot,int c){
	GLfloat xo,yo,xf,yf;

	xo = (left + grd_curcanv->cv_bitmap.bm_x + 0.5) / (float)last_width;
	xf = (right + grd_curcanv->cv_bitmap.bm_x + 0.5) / (float)last_width;
	yo = 1.0 - (top + grd_curcanv->cv_bitmap.bm_y + 0.5) / (float)last_height;
	yf = 1.0 - (bot + grd_curcanv->cv_bitmap.bm_y + 0.5) / (float)last_height;

	OGL_DISABLE(TEXTURE_2D);
	glColor3f(CPAL2Tr(c),CPAL2Tg(c),CPAL2Tb(c));
	glBegin(GL_LINES);
	glVertex2f(xo,yo);
	glVertex2f(xf,yf);
	glEnd();
}
	

GLfloat last_r=0, last_g=0, last_b=0;
int do_pal_step=0;

void ogl_do_palfx(void){
	OGL_DISABLE(TEXTURE_2D);
	if (gr_palette_faded_out){
		glColor3f(0,0,0);
	}else{
		if (do_pal_step){
			glEnable(GL_BLEND);
			glBlendFunc(GL_ONE,GL_ONE);
			glColor3f(last_r,last_g,last_b);
		}else
			return;
	}
	
	
	glBegin(GL_QUADS);
	glVertex2f(0,0);
	glVertex2f(0,1);
	glVertex2f(1,1);
	glVertex2f(1,0);
	glEnd();
	
	glEnable(GL_BLEND);	
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
}

void gr_palette_clear()
{
	gr_palette_faded_out=1;
}


int ogl_brightness_ok = 0;
int ogl_setgammaramp_ok = 0;
int ogl_brightness_r = 0, ogl_brightness_g = 0, ogl_brightness_b = 0;
static int old_b_r = 0, old_b_g = 0, old_b_b = 0;

void gr_palette_step_up(int r, int g, int b)
{
	if (gr_palette_faded_out)
		return;

	old_b_r = ogl_brightness_r;
	old_b_g = ogl_brightness_g;
	old_b_b = ogl_brightness_b;

	ogl_brightness_r = max(r + gr_palette_gamma, 0);
	ogl_brightness_g = max(g + gr_palette_gamma, 0);
	ogl_brightness_b = max(b + gr_palette_gamma, 0);

	if (ogl_setgammaramp_ok &&
	    (old_b_r != ogl_brightness_r ||
	     old_b_g != ogl_brightness_g ||
	     old_b_b != ogl_brightness_b))
		ogl_brightness_ok = !ogl_setbrightness_internal();

	if (!ogl_setgammaramp_ok || !ogl_brightness_ok)
	{
		last_r = ogl_brightness_r / 63.0;
		last_g = ogl_brightness_g / 63.0;
		last_b = ogl_brightness_b / 63.0;

		do_pal_step = (r || g || b || gr_palette_gamma);
	}
	else
	{
		do_pal_step = 0;
	}
}

#undef min
static inline int min(int x, int y) { return x < y ? x : y; }

void gr_palette_load( ubyte *pal )
{
	int i;

	for (i=0; i<768; i++ ) {
		gr_current_pal[i] = pal[i];
		if (gr_current_pal[i] > 63) gr_current_pal[i] = 63;
	}

	gr_palette_faded_out=0;

	gr_palette_step_up(0, 0, 0); // make ogl_setbrightness_internal get run so that menus get brightened too.

	init_computed_colors();
}

int gr_palette_fade_out(ubyte *pal, int nsteps, int allow_keys)
{
	gr_palette_faded_out=1;
	return 0;
}

int gr_palette_fade_in(ubyte *pal, int nsteps, int allow_keys)
{
	gr_palette_faded_out=0;
	return 0;
}

void gr_palette_read(ubyte * pal)
{
	int i;
	for (i=0; i<768; i++ ) {
		pal[i]=gr_current_pal[i];
		if (pal[i] > 63) pal[i] = 63;
	}
}

#define GL_BGR_EXT 0x80E0

typedef struct {
      unsigned char TGAheader[12];
      unsigned char header[6];
} TGA_header;

//writes out an uncompressed RGB .tga file
//if we got really spiffy, we could optionally link in libpng or something, and use that.
void write_bmp(char *savename,int w,int h,unsigned char *buf){
	PHYSFS_file* TGAFile;
	TGA_header TGA;
	GLbyte HeightH,HeightL,WidthH,WidthL;

	buf = (unsigned char*)calloc(w*h*3,sizeof(unsigned char));

	glReadPixels(0,0,w,h,GL_BGR_EXT,GL_UNSIGNED_BYTE,buf);

	TGAFile = PHYSFSX_openWriteBuffered(savename);

	HeightH = (GLbyte)(h / 256);
	HeightL = (GLbyte)(h % 256);
	WidthH  = (GLbyte)(w / 256);
	WidthL  = (GLbyte)(w % 256);
	// Write TGA Header
	TGA.TGAheader[0] = 0;
	TGA.TGAheader[1] = 0;
	TGA.TGAheader[2] = 2;
	TGA.TGAheader[3] = 0;
	TGA.TGAheader[4] = 0;
	TGA.TGAheader[5] = 0;
	TGA.TGAheader[6] = 0;
	TGA.TGAheader[7] = 0;
	TGA.TGAheader[8] = 0;
	TGA.TGAheader[9] = 0;
	TGA.TGAheader[10] = 0;
	TGA.TGAheader[11] = 0;
	TGA.header[0] = (GLbyte) WidthL;
	TGA.header[1] = (GLbyte) WidthH;
	TGA.header[2] = (GLbyte) HeightL;
	TGA.header[3] = (GLbyte) HeightH;
	TGA.header[4] = (GLbyte) 24;
	TGA.header[5] = 0;
	PHYSFS_write(TGAFile,&TGA,sizeof(TGA_header),1);
	PHYSFS_write(TGAFile,buf,w*h*3*sizeof(unsigned char),1);
	PHYSFS_close(TGAFile);
	free(buf);
}

void save_screen_shot(int automap_flag)
{
	char message[100];
	static int savenum=0;
	char savename[13+sizeof(SCRNS_DIR)];
	unsigned char *buf;
	
	if (!ogl_readpixels_ok){
		if (!automap_flag)
			hud_message(MSGC_GAME_FEEDBACK,"glReadPixels not supported on your configuration");
		return;
	}

	stop_time();

	if (!cfexist(SCRNS_DIR))
		PHYSFS_mkdir(SCRNS_DIR); //try making directory

	do
	{
		if (savenum == 9999)
			savenum = 0;
		sprintf(savename, "%sscrn%04d.tga",SCRNS_DIR, savenum++);
	} while (PHYSFS_exists(savename));

	sprintf( message, "%s '%s'", TXT_DUMPING_SCREEN, savename );

	if (automap_flag) {
	} else {
		hud_message(MSGC_GAME_FEEDBACK,message);
	}
	
	buf = malloc(grd_curscreen->sc_w*grd_curscreen->sc_h*3);
	glReadBuffer(GL_FRONT);
	write_bmp(savename,grd_curscreen->sc_w,grd_curscreen->sc_h,buf);
	free(buf);

	key_flush();
	start_time();
}
