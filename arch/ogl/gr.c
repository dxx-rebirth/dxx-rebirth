/*
 * $Source: /cvs/cvsroot/d2x/arch/ogl/gr.c,v $
 * $Revision: 1.5 $
 * $Author: bradleyb $
 * $Date: 2001-11-09 06:53:37 $
 *
 * // OGL video functions. - Added 9/15/99 Matthew Mueller
 *
 * $Log: not supported by cvs2svn $
 * Revision 1.4  2001/11/08 10:19:52  bradleyb
 * use new d_realloc function, so mem manager doesn't die
 *
 * Revision 1.3  2001/11/04 09:00:25  bradleyb
 * Enable d1x-style hud_message
 *
 * Revision 1.2  2001/10/31 07:35:47  bradleyb
 * Sync with d1x
 *
 * Revision 1.1  2001/10/25 08:25:34  bradleyb
 * Finished moving stuff to arch/blah.  I know, it's ugly, but It'll be easier to sync with d1x.
 *
 * Revision 1.7  2001/10/25 02:23:48  bradleyb
 * formatting fix
 *
 * Revision 1.6  2001/10/18 23:59:23  bradleyb
 * Changed __ENV_LINUX__ to __linux__
 *
 * Revision 1.5  2001/10/12 00:18:40  bradleyb
 * Switched from Cygwin to mingw32 on MS boxes.  Vastly improved compilability.
 *
 * Revision 1.4  2001/01/29 13:47:52  bradleyb
 * Fixed build, some minor cleanups.
 *
 */

#ifdef HAVE_CONFIG_H
#include <conf.h>
#endif

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#ifdef __WINDOWS__
#include <windows.h>
#endif

//#include <GL/gl.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
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

#define DECLARE_VARS
#include "ogl_init.h"
#include <GL/glu.h>

int ogl_voodoohack=0;

int gr_installed = 0;


void gr_palette_clear(); // Function prototype for gr_init;
int gl_initialized=0;
int gl_reticle=1;

int ogl_fullscreen=0;

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
	//	grd_curscreen->sc_mode=0;//hack to get it to reset screen mode
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
//		glWritePixels(0,0,grd_curscreen->sc_w,grd_curscreen->sc_h,GL_RGB,GL_UNSIGNED_BYTE,buf);
		glRasterPos2f(0,0);
		glDrawPixels(grd_curscreen->sc_w,grd_curscreen->sc_h,GL_RGB,GL_UNSIGNED_BYTE,buf);
		free(buf);
	}
	//	grd_curscreen->sc_mode=0;//hack to get it to reset screen mode

	return ogl_fullscreen;
}

void ogl_init_state(void){
	/* select clearing (background) color   */
	glClearColor(0.0, 0.0, 0.0, 0.0);
	glShadeModel(GL_SMOOTH);

	/* initialize viewing values */
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glOrtho(0.0, 1.0, 0.0, 1.0, -1.0, 1.0);
	glScalef(1.0, -1.0, 1.0);
	glTranslatef(0.0, -1.0, 0.0);
	gr_palette_step_up(0,0,0);//in case its left over from in game
}

int last_screen_mode=-1;

void ogl_set_screen_mode(void){
	if (last_screen_mode==Screen_mode)
		return;
	OGL_VIEWPORT(0,0,grd_curscreen->sc_w,grd_curscreen->sc_h);
//	OGL_VIEWPORT(grd_curcanv->cv_bitmap.bm_x,grd_curcanv->cv_bitmap.bm_y,grd_curcanv->cv_bitmap.bm_w,grd_curcanv->cv_bitmap.bm_h);
	if (Screen_mode==SCREEN_GAME){
		glDrawBuffer(GL_BACK);
	}else{
		glClearColor(0.0, 0.0, 0.0, 0.0);
		glDrawBuffer(GL_FRONT);
		glClear(GL_COLOR_BUFFER_BIT);
		glMatrixMode(GL_PROJECTION);
		glLoadIdentity();//clear matrix
		glOrtho(0.0, 1.0, 0.0, 1.0, -1.0, 1.0);
		glMatrixMode(GL_MODELVIEW);
		glLoadIdentity();//clear matrix
		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	}
	last_screen_mode=Screen_mode;
}

void gr_update()
{
	if (gl_initialized){

		if(Screen_mode != SCREEN_GAME){
			glFlush();
		}
	}
}

const char *gl_vendor,*gl_renderer,*gl_version,*gl_extensions;

void ogl_get_verinfo(void){
	int t;
	gl_vendor=glGetString(GL_VENDOR);
	gl_renderer=glGetString(GL_RENDERER);
	gl_version=glGetString(GL_VERSION);
	gl_extensions=glGetString(GL_EXTENSIONS);

	con_printf(CON_VERBOSE, "gl vendor:%s renderer:%s version:%s extensions:%s\n",gl_vendor,gl_renderer,gl_version,gl_extensions);

	ogl_intensity4_ok=1;ogl_luminance4_alpha4_ok=1;ogl_rgba2_ok=1;ogl_gettexlevelparam_ok=1;

#ifdef __WINDOWS__
	dglMultiTexCoord2fARB = (glMultiTexCoord2fARB_fp)wglGetProcAddress("glMultiTexCoord2fARB");
	dglActiveTextureARB = (glActiveTextureARB_fp)wglGetProcAddress("glActiveTextureARB");
	dglMultiTexCoord2fSGIS = (glMultiTexCoord2fSGIS_fp)wglGetProcAddress("glMultiTexCoord2fSGIS");
	dglSelectTextureSGIS = (glSelectTextureSGIS_fp)wglGetProcAddress("glSelectTextureSGIS");
#endif

	//multitexturing doesn't work yet.
#ifdef GL_ARB_multitexture
	ogl_arb_multitexture_ok=0;//(strstr(gl_extensions,"GL_ARB_multitexture")!=0 && glActiveTextureARB!=0 && 0);
	mprintf((0,"c:%p d:%p e:%p\n",strstr(gl_extensions,"GL_ARB_multitexture"),glActiveTextureARB,glBegin));
#endif
#ifdef GL_SGIS_multitexture
	ogl_sgis_multitexture_ok=0;//(strstr(gl_extensions,"GL_SGIS_multitexture")!=0 && glSelectTextureSGIS!=0 && 0);
	mprintf((0,"a:%p b:%p\n",strstr(gl_extensions,"GL_SGIS_multitexture"),glSelectTextureSGIS));
#endif

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

	//allow overriding of stuff.
#ifdef GL_ARB_multitexture
	if ((t=FindArg("-gl_arb_multitexture_ok"))){
		ogl_arb_multitexture_ok=atoi(Args[t+1]);
	}
#endif
#ifdef GL_SGIS_multitexture
	if ((t=FindArg("-gl_sgis_multitexture_ok"))){
		ogl_sgis_multitexture_ok=atoi(Args[t+1]);
	}
#endif
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

	con_printf(CON_VERBOSE, "gl_arb_multitexture:%i gl_sgis_multitexture:%i\n",ogl_arb_multitexture_ok,ogl_sgis_multitexture_ok);
	con_printf(CON_VERBOSE, "gl_intensity4:%i gl_luminance4_alpha4:%i gl_rgba2:%i gl_readpixels:%i gl_gettexlevelparam:%i\n",ogl_intensity4_ok,ogl_luminance4_alpha4_ok,ogl_rgba2_ok,ogl_readpixels_ok,ogl_gettexlevelparam_ok);
}

int gr_set_mode(u_int32_t mode)
{
	unsigned int w,h;
	char *gr_bm_data;

#ifdef NOGRAPH
return 0;
#endif
//	mode=0;
	if (mode<=0)
		return 0;

	w=SM_W(mode);
	h=SM_H(mode);

	//if (screen != NULL) gr_palette_clear();

//	ogl_init_state();
	
	gr_bm_data=grd_curscreen->sc_canvas.cv_bitmap.bm_data;//since we use realloc, we want to keep this pointer around.
	memset( grd_curscreen, 0, sizeof(grs_screen));
	grd_curscreen->sc_mode = mode;
	grd_curscreen->sc_w = w;
	grd_curscreen->sc_h = h;
	grd_curscreen->sc_aspect = fixdiv(grd_curscreen->sc_w*3,grd_curscreen->sc_h*4);
	grd_curscreen->sc_canvas.cv_bitmap.bm_x = 0;
	grd_curscreen->sc_canvas.cv_bitmap.bm_y = 0;
	grd_curscreen->sc_canvas.cv_bitmap.bm_w = w;
	grd_curscreen->sc_canvas.cv_bitmap.bm_h = h;
	//grd_curscreen->sc_canvas.cv_bitmap.bm_rowsize = screen->pitch;
	grd_curscreen->sc_canvas.cv_bitmap.bm_rowsize = w;
	grd_curscreen->sc_canvas.cv_bitmap.bm_type = BM_OGL;
	//grd_curscreen->sc_canvas.cv_bitmap.bm_data = (unsigned char *)screen->pixels;
//	mprintf((0,"ogl/gr.c: reallocing %p to %i\n",grd_curscreen->sc_canvas.cv_bitmap.bm_data,w*h));
	grd_curscreen->sc_canvas.cv_bitmap.bm_data = d_realloc(gr_bm_data,w*h);
	gr_set_current_canvas(NULL);
	//gr_enable_default_palette_loading();
	
	ogl_init_window(w,h);//platform specific code

	ogl_get_verinfo();

	OGL_VIEWPORT(0,0,w,h);

	ogl_set_screen_mode();

//	gamefont_choose_game_font(w,h);
	
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
//	return GL_NEAREST;
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
//	return -1;
}
#ifdef OGL_RUNTIME_LOAD
#if defined(__WINDOWS__) || defined(__MINGW32__)
char *OglLibPath="opengl32.dll";
#endif
#ifdef __linux__
char *OglLibPath="libGL.so";
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
			Error("Opengl: error loading %s\n",OglLibPath);
		}
		ogl_rt_loaded=1;
	}
	return retcode;
}
#endif

int gr_init()
{
 int mode = SM(640,480);
 int retcode,t,glt=0;
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
	if (FindArg("-fullscreen"))
		gr_toggle_fullscreen();
#endif
	if ((glt=FindArg("-gl_alttexmerge")))
		ogl_alttexmerge=1;
	if ((t=FindArg("-gl_stdtexmerge")))
		if (t>=glt)//allow overriding of earlier args
			ogl_alttexmerge=0;
			
	if ((glt=FindArg("-gl_16bittextures")))
		ogl_rgba_format=GL_RGB5_A1;

	if ((glt=FindArg("-gl_mipmap"))){
		GL_texmagfilt=GL_LINEAR;
		GL_texminfilt=GL_LINEAR_MIPMAP_NEAREST;
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
	mprintf((0,"gr_init: texmagfilt:%x texminfilt:%x needmipmaps=%i\n",GL_texmagfilt,GL_texminfilt,GL_needmipmaps));
	
	if ((t=FindArg("-gl_vidmem"))){
		ogl_mem_target=atoi(Args[t+1])*1024*1024;
	}
	if ((t=FindArg("-gl_reticle"))){
		gl_reticle=atoi(Args[t+1]);
	}
	//printf("ogl_mem_target=%i\n",ogl_mem_target);
	
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

	gr_installed = 1;
	
	atexit(gr_close);

	return 0;
}

void gr_close()
{
//	mprintf((0,"ogl init: %s %s %s - %s\n",glGetString(GL_VENDOR),glGetString(GL_RENDERER),glGetString(GL_VERSION),glGetString,(GL_EXTENSIONS)));

	ogl_close();//platform specific code
	if (grd_curscreen){
		if (grd_curscreen->sc_canvas.cv_bitmap.bm_data)
			d_free(grd_curscreen->sc_canvas.cv_bitmap.bm_data);
		d_free(grd_curscreen);
	}
#ifdef OGL_RUNTIME_LOAD
	if (ogl_rt_loaded)
		OpenGL_LoadLibrary(false);
#endif
}
extern int r_upixelc;
void ogl_upixelc(int x, int y, int c){
	r_upixelc++;
//	printf("gr_upixelc(%i,%i,%i)%i\n",x,y,c,Function_mode==FMODE_GAME);
//	if(Function_mode != FMODE_GAME){
//		grd_curcanv->cv_bitmap.bm_data[y*grd_curscreen->sc_canvas.cv_bitmap.bm_w+x]=c;
//	}else{
		OGL_DISABLE(TEXTURE_2D);
		glPointSize(1.0);
		glBegin(GL_POINTS);
//		glBegin(GL_LINES);
//	ogl_pal=gr_current_pal;
		glColor3f(CPAL2Tr(c),CPAL2Tg(c),CPAL2Tb(c));
//	ogl_pal=gr_palette;
		glVertex2f((x+grd_curcanv->cv_bitmap.bm_x)/(float)last_width,1.0-(y+grd_curcanv->cv_bitmap.bm_y)/(float)last_height);
//		glVertex2f(x/((float)last_width+1),1.0-y/((float)last_height+1));
		glEnd();
//	}
}
void ogl_urect(int left,int top,int right,int bot){
	GLfloat xo,yo,xf,yf;
	int c=COLOR;
	
	xo=(left+grd_curcanv->cv_bitmap.bm_x)/(float)last_width;
	xf=(right+grd_curcanv->cv_bitmap.bm_x)/(float)last_width;
	yo=1.0-(top+grd_curcanv->cv_bitmap.bm_y)/(float)last_height;
	yf=1.0-(bot+grd_curcanv->cv_bitmap.bm_y)/(float)last_height;
	
	OGL_DISABLE(TEXTURE_2D);
	glColor3f(CPAL2Tr(c),CPAL2Tg(c),CPAL2Tb(c));
	glBegin(GL_QUADS);
	glVertex2f(xo,yo);
	glVertex2f(xo,yf);
	glVertex2f(xf,yf);
	glVertex2f(xf,yo);
	glEnd();
}
void ogl_ulinec(int left,int top,int right,int bot,int c){
	GLfloat xo,yo,xf,yf;
	
	xo=(left+grd_curcanv->cv_bitmap.bm_x)/(float)last_width;
	xf=(right+grd_curcanv->cv_bitmap.bm_x)/(float)last_width;
	yo=1.0-(top+grd_curcanv->cv_bitmap.bm_y)/(float)last_height;
	yf=1.0-(bot+grd_curcanv->cv_bitmap.bm_y)/(float)last_height;
	
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
//	GLfloat r,g,b,a;
	OGL_DISABLE(TEXTURE_2D);
	if (gr_palette_faded_out){
/*		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);*/
		glColor3f(0,0,0);
//		r=g=b=0.0;a=1.0;
	}else{
		if (do_pal_step){
			//glBlendFunc(GL_SRC_COLOR, GL_DST_COLOR);
			glEnable(GL_BLEND);
			glBlendFunc(GL_ONE,GL_ONE);
			glColor3f(last_r,last_g,last_b);
//			r=f2fl(last_r);g=f2fl(last_g);b=f2fl(last_b);a=0.5;
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


void gr_palette_step_up( int r, int g, int b )
{
	if (gr_palette_faded_out) return;

//	if ( (r==last_r) && (g==last_g) && (b==last_b) ) return;

/*	last_r = r/63.0;
	last_g = g/63.0;
	last_b = b/63.0;
	do_pal_step=(r || g || b);*/
	
	last_r = (r+gr_palette_gamma)/63.0;
	last_g = (g+gr_palette_gamma)/63.0;
	last_b = (b+gr_palette_gamma)/63.0;

	do_pal_step=(r || g || b || gr_palette_gamma);
	
}

//added on 980913 by adb to fix palette problems
// need a min without side effects...
#undef min
static inline int min(int x, int y) { return x < y ? x : y; }
//end changes by adb

void gr_palette_load( ubyte *pal )	
{
 int i;//, j;

 for (i=0; i<768; i++ ) {
     gr_current_pal[i] = pal[i];
     if (gr_current_pal[i] > 63) gr_current_pal[i] = 63;
 }
 //palette = screen->format->palette;

 gr_palette_faded_out=0;

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

//writes out an uncompressed RGB .tga file
//if we got really spiffy, we could optionally link in libpng or something, and use that.
void write_bmp(char *savename,int w,int h,unsigned char *buf){
	int f;
#if defined(__WINDOWS__) || defined(__MINGW32__)
	f=open(savename,O_CREAT|O_EXCL|O_WRONLY,S_IRUSR|S_IWUSR);
#else
	f=open(savename,O_CREAT|O_EXCL|O_WRONLY,S_IRUSR|S_IWUSR|S_IRGRP|S_IROTH);
#endif
	if (f>=0){
		GLubyte    targaMagic[12] = { 0, //no identification field
			 0,//no colormap
			 2,//RGB image (well, BGR, actually)
			 0, 0, 0, 0, 0, 0, 0, 0, 0 };//no colormap or image origin stuff.
		GLubyte blah;
		int r;
		GLubyte *s;
		int x,y;
		
		//write .TGA header.
		write (f,targaMagic,sizeof(targaMagic));
		blah=w%256;write (f,&blah,1);//w, low
		blah=w/256;write (f,&blah,1);//w, high
		blah=h%256;write (f,&blah,1);//h, low
		blah=h/256;write (f,&blah,1);//h, high
		blah=24;write (f,&blah,1);//24 bpp
		blah=0;write (f,&blah,1);//no attribute bits, origin is lowerleft, no interleave
		
		s=buf;
		for (y=0;y<h;y++){//TGAs use BGR ordering of data.
			for (x=0;x<w;x++){
				blah=s[0];
				s[0]=s[2];
				s[2]=blah;
				s+=3;				
			}
		}
		x=0;y=w*h*3;
		while (x<y){
			r=write(f,buf+x,y);
			if (r<=0){
				mprintf((0,"screenshot error, couldn't write to %s (err %i)\n",savename,errno));
				break;
			}
			x+=r;y-=r;
		}
		close(f);
	}else{
		mprintf((0,"screenshot error, couldn't open %s (err %i)\n",savename,errno));
	}
}
void save_screen_shot(int automap_flag)
{
//	fix t1;
	char message[100];
	static int savenum=0;
	char savename[13];
	unsigned char *buf;
	
	if (!ogl_readpixels_ok){
		if (!automap_flag)
			hud_message(MSGC_GAME_FEEDBACK,"glReadPixels not supported on your configuration");
		return;
	}

	stop_time();

//added/changed on 10/31/98 by Victor Rachels to fix overwrite each new game
	if ( savenum == 9999 ) savenum = 0;
	sprintf(savename,"scrn%04d.tga",savenum++);

	while(!access(savename,0))
	{
		if ( savenum == 9999 ) savenum = 0;
		sprintf(savename,"scrn%04d.tga",savenum++);
	}
	sprintf( message, "%s '%s'", TXT_DUMPING_SCREEN, savename );
//end this section addition/change - Victor Rachels

	if (automap_flag) {
//	save_font = grd_curcanv->cv_font;
//	gr_set_curfont(GAME_FONT);
//	gr_set_fontcolor(gr_find_closest_color_current(0,31,0),-1);
//	gr_get_string_size(message,&w,&h,&aw);
//		modex_print_message(32, 2, message);
	} else {
		hud_message(MSGC_GAME_FEEDBACK,message);
	}
	
	buf = d_malloc(grd_curscreen->sc_w*grd_curscreen->sc_h*3);
	glReadBuffer(GL_FRONT);
	glReadPixels(0,0,grd_curscreen->sc_w,grd_curscreen->sc_h,GL_RGB,GL_UNSIGNED_BYTE,buf);
	write_bmp(savename,grd_curscreen->sc_w,grd_curscreen->sc_h,buf);
	d_free(buf);

	key_flush();
	start_time();
}
