// OGL video functions. - Added 9/15/99 Matthew Mueller

#define DECLARE_VARS

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#ifdef _WIN32
#include <windows.h>
#endif
#ifndef macintosh
#ifndef _MSC_VER
#include <unistd.h>
#endif
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#endif
#include <errno.h>
#include <limits.h>
#include <SDL/SDL.h>
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
#include "args.h"
#include "key.h"
#include "u_mem.h"
#include "gamefont.h"
#include "render.h"
#include "internal.h"
#include "physfsx.h"
#include "console.h"
#if defined(__APPLE__) && defined(__MACH__)
#include <OpenGL/glu.h>
#else
#include <GL/glu.h>
#endif
#include "config.h"
#include "playsave.h"
#include "vers_id.h"
#include "gr.h"

int gr_installed = 0;
int gl_initialized=0;
int ogl_fullscreen=0;
static int curx=-1,cury=-1,curfull=0;
int linedotscale=1; // scalar of glLinewidth and glPointSize - only calculated once when resolution changes

void ogl_swap_buffers_internal(void)
{
	SDL_GL_SwapBuffers();
}

int ogl_init_window(int x, int y)
{
	if (gl_initialized){
		if (x==curx && y==cury && curfull==ogl_fullscreen)
			return 0;
		ogl_smash_texture_list_internal();//if we are or were fullscreen, changing vid mode will invalidate current textures
	}

	SDL_WM_SetCaption(DESCENT_VERSION, "Descent");
	if (!SDL_SetVideoMode(x, y, GameArg.DbgGlBpp, SDL_OPENGL | (ogl_fullscreen ? SDL_FULLSCREEN : 0)))
	{
		Error("Could not set %dx%dx%d opengl video mode: %s\n", x, y, GameArg.DbgGlBpp, SDL_GetError());
	}
	SDL_ShowCursor(0);

	curx=x;cury=y;curfull=ogl_fullscreen;
	
	linedotscale = ((x/640<y/480?x/640:y/480)<1?1:(x/640<y/480?x/640:y/480));

	gl_initialized=1;
	return 0;
}

int gr_check_fullscreen(void)
{
	return ogl_fullscreen;
}

void gr_do_fullscreen(int f)
{
	ogl_fullscreen=f;
	if (gl_initialized)
	{
		if (!SDL_VideoModeOK(curx, cury, GameArg.DbgGlBpp, SDL_OPENGL | (ogl_fullscreen?SDL_FULLSCREEN:0)))
		{
			con_printf(CON_URGENT,"Cannot set %ix%i. Fallback to 640x480\n",curx,cury);
			curx=640;
			cury=480;
			Game_screen_mode=SM(curx,cury);
		}
	
		ogl_init_window(curx,cury);
	}
}

int gr_toggle_fullscreen(void)
{
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
	GameCfg.WindowMode = !ogl_fullscreen;
	return ogl_fullscreen;
}

extern void ogl_init_pixel_buffers(int w, int h);
 extern void ogl_close_pixel_buffers(void);

void ogl_init_state(void)
{
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

// Set the buffer to draw to. 0 is front, 1 is back
void gr_set_draw_buffer(int buf)
{
	glDrawBuffer((buf == 0) ? GL_FRONT : GL_BACK);
}

const char *gl_vendor,*gl_renderer,*gl_version,*gl_extensions;

void ogl_get_verinfo(void)
{
	gl_vendor=(const char *)glGetString(GL_VENDOR);
	gl_renderer=(const char *)glGetString(GL_RENDERER);
	gl_version=(const char *)glGetString(GL_VERSION);
	gl_extensions=(const char *)glGetString(GL_EXTENSIONS);

	con_printf(CON_VERBOSE, "OpenGL: vendor: %s\nOpenGL: renderer: %s\nOpenGL: version: %s\n",gl_vendor,gl_renderer,gl_version);

#ifdef _WIN32
	dglMultiTexCoord2fARB = (glMultiTexCoord2fARB_fp)wglGetProcAddress("glMultiTexCoord2fARB");
	dglActiveTextureARB = (glActiveTextureARB_fp)wglGetProcAddress("glActiveTextureARB");
	dglMultiTexCoord2fSGIS = (glMultiTexCoord2fSGIS_fp)wglGetProcAddress("glMultiTexCoord2fSGIS");
	dglSelectTextureSGIS = (glSelectTextureSGIS_fp)wglGetProcAddress("glSelectTextureSGIS");
#endif

	//add driver specific hacks here.  whee.
	if ((stricmp(gl_renderer,"Mesa NVIDIA RIVA 1.0\n")==0 || stricmp(gl_renderer,"Mesa NVIDIA RIVA 1.2\n")==0) && stricmp(gl_version,"1.2 Mesa 3.0")==0)
	{
		GameArg.DbgGlIntensity4Ok=0;//ignores alpha, always black background instead of transparent.
		GameArg.DbgGlReadPixelsOk=0;//either just returns all black, or kills the X server entirely
		GameArg.DbgGlGetTexLevelParamOk=0;//returns random data..
	}

#ifdef macintosh
	if (stricmp(gl_renderer,"3dfx Voodoo 3")==0) // strangely, includes Voodoo 2
		GameArg.DbgGlGetTexLevelParamOk=0; // Always returns 0
#endif

#ifndef NDEBUG
	con_printf(CON_VERBOSE,"gl_intensity4:%i gl_luminance4_alpha4:%i gl_rgba2:%i gl_readpixels:%i gl_gettexlevelparam:%i\n",GameArg.DbgGlIntensity4Ok,GameArg.DbgGlLuminance4Alpha4Ok,GameArg.DbgGlRGBA2Ok,GameArg.DbgGlReadPixelsOk,GameArg.DbgGlGetTexLevelParamOk);
#endif
}

int gr_set_mode(u_int32_t mode)
{
	unsigned int w, h;
	char *gr_bm_data;

	if (mode<=0)
		return 0;

	w=SM_W(mode);
	h=SM_H(mode);

	if (!SDL_VideoModeOK(w, h, GameArg.DbgGlBpp, SDL_OPENGL | (ogl_fullscreen?SDL_FULLSCREEN:0)))
	{
		con_printf(CON_URGENT,"Cannot set %ix%i. Fallback to 640x480\n",w,h);
		w=640;
		h=480;
		Game_screen_mode=mode=SM(w,h);
	}

	gr_bm_data=(char *)grd_curscreen->sc_canvas.cv_bitmap.bm_data;//since we use realloc, we want to keep this pointer around.
	memset( grd_curscreen, 0, sizeof(grs_screen));
	grd_curscreen->sc_mode = mode;
	grd_curscreen->sc_w = w;
	grd_curscreen->sc_h = h;
	grd_curscreen->sc_aspect = fixdiv(grd_curscreen->sc_w*GameCfg.AspectX,grd_curscreen->sc_h*GameCfg.AspectY);
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
	gamefont_choose_game_font(w,h);
	
	return 0;
}

#define GLstrcmptestr(a,b) if (stricmp(a,#b)==0 || stricmp(a,"GL_" #b)==0)return GL_ ## b;
int ogl_atotexfilti(char *a,int min)
{
	GLstrcmptestr(a,NEAREST);
	GLstrcmptestr(a,LINEAR);
	if (min)
	{//mipmaps are valid only for the min filter
		GLstrcmptestr(a,NEAREST_MIPMAP_NEAREST);
		GLstrcmptestr(a,NEAREST_MIPMAP_LINEAR);
		GLstrcmptestr(a,LINEAR_MIPMAP_NEAREST);
		GLstrcmptestr(a,LINEAR_MIPMAP_LINEAR);
	}
	Error("unknown/invalid texture filter %s\n",a);
}

int ogl_testneedmipmaps(int i)
{
	switch (i)
	{
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

#ifdef _WIN32
char *OglLibPath="opengl32.dll";

int ogl_rt_loaded=0;
int ogl_init_load_library(void)
{
	int retcode=0;
	if (!ogl_rt_loaded)
	{
		retcode = OpenGL_LoadLibrary(true);
		if(retcode)
		{
			if(!glEnd)
			{
				Error("Opengl: Functions not imported\n");
			}
		}
		else
		{
			Error("Opengl: error loading %s\n",OglLibPath);
		}
		ogl_rt_loaded=1;
	}
	return retcode;
}
#endif

void gr_set_attributes(void)
{
	switch (GameCfg.TexFilt)
	{
		case 2:
			OglTexMagFilt = GL_LINEAR;
			OglTexMinFilt = GL_LINEAR_MIPMAP_LINEAR;
			break;
		case 1:
			OglTexMagFilt = GL_LINEAR;
			OglTexMinFilt = GL_LINEAR_MIPMAP_NEAREST;
			break;
		default:
			OglTexMagFilt = GL_NEAREST;
			OglTexMinFilt = GL_NEAREST;
			break;
	}

	GL_needmipmaps=ogl_testneedmipmaps(OglTexMinFilt);

	SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 16);
	SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE,0);
	SDL_GL_SetAttribute(SDL_GL_ACCUM_RED_SIZE,0);
	SDL_GL_SetAttribute(SDL_GL_ACCUM_GREEN_SIZE,0);
	SDL_GL_SetAttribute(SDL_GL_ACCUM_BLUE_SIZE,0);
	SDL_GL_SetAttribute(SDL_GL_ACCUM_ALPHA_SIZE,0);
	SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER,1);
	SDL_GL_SetAttribute(SDL_GL_SWAP_CONTROL,GameCfg.VSync);
	if (GameCfg.Multisample)
	{
		SDL_GL_SetAttribute(SDL_GL_MULTISAMPLEBUFFERS, 1);
		SDL_GL_SetAttribute(SDL_GL_MULTISAMPLESAMPLES, 4);
	}
	else
	{
		SDL_GL_SetAttribute(SDL_GL_MULTISAMPLEBUFFERS, 0);
		SDL_GL_SetAttribute(SDL_GL_MULTISAMPLESAMPLES, 0);
	}
	ogl_smash_texture_list_internal();
}

int gr_init(int mode)
{
	int retcode;

	// Only do this function once!
	if (gr_installed==1)
		return -1;

#ifdef _WIN32
	ogl_init_load_library();
#endif

	if (!GameCfg.WindowMode && !GameArg.SysWindow)
		gr_toggle_fullscreen();

	gr_set_attributes();

	ogl_init_texture_list_internal();

	MALLOC( grd_curscreen,grs_screen,1 );
	memset( grd_curscreen, 0, sizeof(grs_screen));
	grd_curscreen->sc_canvas.cv_bitmap.bm_data = NULL;

	// Set the mode.
	if ((retcode=gr_set_mode(mode)))
		return retcode;

	grd_curscreen->sc_canvas.cv_color = 0;
	grd_curscreen->sc_canvas.cv_drawmode = 0;
	grd_curscreen->sc_canvas.cv_font = NULL;
	grd_curscreen->sc_canvas.cv_font_fg_color = 0;
	grd_curscreen->sc_canvas.cv_font_bg_color = 0;
	gr_set_current_canvas( &grd_curscreen->sc_canvas );

	ogl_init_pixel_buffers(256, 128);       // for gamefont_init

	gr_installed = 1;
	

	return 0;
}

void gr_close()
{
	ogl_brightness_r = ogl_brightness_g = ogl_brightness_b = 0;

	if (gl_initialized)
	{
		ogl_smash_texture_list_internal();
		SDL_ShowCursor(1);
	}

	if (grd_curscreen)
	{
		if (grd_curscreen->sc_canvas.cv_bitmap.bm_data)
			d_free(grd_curscreen->sc_canvas.cv_bitmap.bm_data);
		d_free(grd_curscreen);
	}
	ogl_close_pixel_buffers();
#ifdef _WIN32
	if (ogl_rt_loaded)
		OpenGL_LoadLibrary(false);
#endif
}

extern int r_upixelc;
void ogl_upixelc(int x, int y, int c)
{
	r_upixelc++;

	OGL_DISABLE(TEXTURE_2D);
	glPointSize(linedotscale);
	glBegin(GL_POINTS);
	glColor3f(CPAL2Tr(c),CPAL2Tg(c),CPAL2Tb(c));
	glVertex2f((x+grd_curcanv->cv_bitmap.bm_x)/(float)last_width,1.0-(y+grd_curcanv->cv_bitmap.bm_y)/(float)last_height);
	glEnd();
}

void ogl_urect(int left,int top,int right,int bot)
{
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

void ogl_ulinec(int left,int top,int right,int bot,int c)
{
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

void ogl_do_palfx(void)
{
	OGL_DISABLE(TEXTURE_2D);

	if (do_pal_step)
	{
		glEnable(GL_BLEND);
		glBlendFunc(GL_ONE,GL_ONE);
		glColor3f(last_r,last_g,last_b);
	}
	else
		return;

	glBegin(GL_QUADS);
	glVertex2f(0,0);
	glVertex2f(0,1);
	glVertex2f(1,1);
	glVertex2f(1,0);
	glEnd();
	glEnable(GL_BLEND);	
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
}

int ogl_brightness_ok = 0;
int ogl_brightness_r = 0, ogl_brightness_g = 0, ogl_brightness_b = 0;
static int old_b_r = 0, old_b_g = 0, old_b_b = 0;

void gr_palette_step_up( int r, int g, int b )
{
	old_b_r = ogl_brightness_r;
	old_b_g = ogl_brightness_g;
	old_b_b = ogl_brightness_b;

	ogl_brightness_r = max(r + gr_palette_gamma, 0);
	ogl_brightness_g = max(g + gr_palette_gamma, 0);
	ogl_brightness_b = max(b + gr_palette_gamma, 0);

	if (!ogl_brightness_ok)
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
	
	for (i=0; i<768; i++ )
	{
		gr_current_pal[i] = pal[i];
		if (gr_current_pal[i] > 63)
			gr_current_pal[i] = 63;
	}

	gr_palette_step_up(0, 0, 0); // make ogl_setbrightness_internal get run so that menus get brightened too.
	init_computed_colors();
}

void gr_palette_read(ubyte * pal)
{
	int i;
	for (i=0; i<768; i++ )
	{
		pal[i]=gr_current_pal[i];
		if (pal[i] > 63)
			pal[i] = 63;
	}
}

#define GL_BGR_EXT 0x80E0

typedef struct
{
      unsigned char TGAheader[12];
      unsigned char header[6];
} TGA_header;

//writes out an uncompressed RGB .tga file
//if we got really spiffy, we could optionally link in libpng or something, and use that.
void write_bmp(char *savename,int w,int h,unsigned char *buf)
{
	PHYSFS_file* TGAFile;
	TGA_header TGA;
	GLbyte HeightH,HeightL,WidthH,WidthL;

	buf = (unsigned char*)d_calloc(w*h*3,sizeof(unsigned char));

	glReadPixels(0,0,w,h,GL_BGR_EXT,GL_UNSIGNED_BYTE,buf);

	if (!(TGAFile = PHYSFSX_openWriteBuffered(savename)))
	{
		con_printf(CON_URGENT,"Could not create TGA file to dump screenshot!");
		d_free(buf);
		return;
	}

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
	d_free(buf);
}

void save_screen_shot(int automap_flag)
{
	char message[100];
	static int savenum=0;
	char savename[13+sizeof(SCRNS_DIR)];
	unsigned char *buf;
	
	if (!GameArg.DbgGlReadPixelsOk){
		if (!automap_flag)
			hud_message(MSGC_GAME_FEEDBACK,"glReadPixels not supported on your configuration");
		return;
	}

	stop_time();

	if (!cfexist(SCRNS_DIR))
		PHYSFS_mkdir(SCRNS_DIR); //try making directory

	do
	{
		sprintf(savename, "%sscrn%04d.tga",SCRNS_DIR, savenum++);
	} while (PHYSFS_exists(savename));

	sprintf( message, "%s 'scrn%04d.tga'", TXT_DUMPING_SCREEN, savenum-1 );

	if (!automap_flag)
		hud_message(MSGC_GAME_FEEDBACK,message);

	glReadBuffer(GL_FRONT);

	buf = d_malloc(grd_curscreen->sc_w*grd_curscreen->sc_h*3);
	write_bmp(savename,grd_curscreen->sc_w,grd_curscreen->sc_h,buf);
	d_free(buf);

	key_flush();
	start_time();
}
