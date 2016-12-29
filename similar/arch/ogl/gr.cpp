/*
 * This file is part of the DXX-Rebirth project <http://www.dxx-rebirth.com/>.
 * It is copyright by its individual contributors, as recorded in the
 * project's Git history.  See COPYING.txt at the top level for license
 * terms and a link to the Git history.
 */
/*
 *
 * OGL video functions. - Added 9/15/99 Matthew Mueller
 *
 */

#define DECLARE_VARS

#ifdef RPI
// extra libraries for the Raspberry Pi
#include  <bcm_host.h>
#endif

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
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

#include <algorithm>
#include <errno.h>
#include <SDL.h>
#include "hudmsg.h"
#include "game.h"
#include "text.h"
#include "gr.h"
#include "gamefont.h"
#include "grdef.h"
#include "palette.h"
#include "u_mem.h"
#include "dxxerror.h"
#include "inferno.h"
#include "screens.h"
#include "strutil.h"
#include "args.h"
#include "key.h"
#include "physfsx.h"
#include "internal.h"
#include "render.h"
#include "console.h"
#include "config.h"
#include "playsave.h"
#include "vers_id.h"

#if defined(__APPLE__) && defined(__MACH__)
#include <OpenGL/glu.h>
#else
#if DXX_USE_OGLES
#include <EGL/egl.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <SDL_syswm.h>
#else
#include <GL/glu.h>
#endif
#endif

#include "ogl_sync.h"

#include "compiler-make_unique.h"

using std::max;

namespace dcx {

static int ogl_brightness_r, ogl_brightness_g, ogl_brightness_b;

#if DXX_USE_OGLES
static int sdl_video_flags;

#ifdef RPI
static EGL_DISPMANX_WINDOW_T nativewindow;
static DISPMANX_ELEMENT_HANDLE_T dispman_element=DISPMANX_NO_HANDLE;
static DISPMANX_DISPLAY_HANDLE_T dispman_display=DISPMANX_NO_HANDLE;
#endif

#else
static int sdl_video_flags = SDL_OPENGL;
#endif
static ogl_sync sync_helper;
static int gr_installed;
static int gl_initialized;
int linedotscale=1; // scalar of glLinewidth and glPointSize - only calculated once when resolution changes
#ifdef RPI
static int sdl_no_modeswitch;
#else
enum { sdl_no_modeswitch = 0 };
#endif

}

#if DXX_USE_OGLES
static EGLDisplay eglDisplay=EGL_NO_DISPLAY;
static EGLConfig eglConfig;
static EGLSurface eglSurface=EGL_NO_SURFACE;
static EGLContext eglContext=EGL_NO_CONTEXT;

static bool TestEGLError(const char* pszLocation)
{
	/*
	 * eglGetError returns the last error that has happened using egl,
	 * not the status of the last called function. The user has to
	 * check after every single egl call or at least once every frame.
	*/
	EGLint iErr = eglGetError();
	if (iErr != EGL_SUCCESS)
	{
		con_printf(CON_URGENT, "%s failed (%d).", pszLocation, iErr);
		return 0;
	}
	
	return 1;
}
#endif

namespace dsx {

void ogl_swap_buffers_internal(void)
{
	sync_helper.before_swap();
#if DXX_USE_OGLES
	eglSwapBuffers(eglDisplay, eglSurface);
#else
	SDL_GL_SwapBuffers();
#endif
	sync_helper.after_swap();
}

#ifdef RPI

// MH: I got the following constants for vc_dispmanx_element_change_attributes() from:
//     http://qt.gitorious.org/qt/qtbase/commit/5933205cfcd73481cb0645fa6183103063fe3e0d
//     I do not know where they got them from, but OTOH, they are quite obvious.

// these constants are not in any headers (yet)
#define ELEMENT_CHANGE_LAYER          (1<<0)
#define ELEMENT_CHANGE_OPACITY        (1<<1)
#define ELEMENT_CHANGE_DEST_RECT      (1<<2)
#define ELEMENT_CHANGE_SRC_RECT       (1<<3)
#define ELEMENT_CHANGE_MASK_RESOURCE  (1<<4)
#define ELEMENT_CHANGE_TRANSFORM      (1<<5)

static void rpi_destroy_element()
{
	if (dispman_element != DISPMANX_NO_HANDLE) {
		DISPMANX_UPDATE_HANDLE_T dispman_update;
		con_printf(CON_DEBUG, "RPi: destroying display manager element");
		dispman_update = vc_dispmanx_update_start( 0 );
		if (vc_dispmanx_element_remove( dispman_update, dispman_element)) {
			 con_printf(CON_URGENT, "RPi: failed to remove dispmanx element!");
		}
		vc_dispmanx_update_submit_sync( dispman_update );
		dispman_element = DISPMANX_NO_HANDLE;
	}
}

static int rpi_setup_element(int x, int y, Uint32 video_flags, int update)
{
	// this code is based on the work of Ben O'Steen
	// http://benosteen.wordpress.com/2012/04/27/using-opengl-es-2-0-on-the-raspberry-pi-without-x-windows/
	// https://github.com/benosteen/opengles-book-samples/tree/master/Raspi
	DISPMANX_UPDATE_HANDLE_T dispman_update;
	VC_RECT_T dst_rect;
	VC_RECT_T src_rect;
	VC_DISPMANX_ALPHA_T alpha_descriptor;

	uint32_t rpi_display_device=DISPMANX_ID_MAIN_LCD;
	uint32_t display_width;
	uint32_t display_height;
	int success;

	success = graphics_get_display_size(rpi_display_device, &display_width, &display_height);
	if ( success < 0 ) {
		con_printf(CON_URGENT, "Could not get RPi display size, assuming 640x480");
		display_width=640;
		display_height=480;
	}

	if (static_cast<uint32_t>(x) > display_width) {
		con_printf(CON_URGENT, "RPi: Requested width %d exceeds display width %u, scaling down!",
			x,display_width);
		x=static_cast<int>(display_width);
	}
	if (static_cast<uint32_t>(y) > display_height) {
		con_printf(CON_URGENT, "RPi: Requested height %d exceeds display height %u, scaling down!",
			y,display_height);
		y=static_cast<int>(display_height);
	}

	con_printf(CON_DEBUG, "RPi: display resolution %ux%u, game resolution: %dx%d (%s)", display_width, display_height, x, y, (video_flags & SDL_FULLSCREEN)?"fullscreen":"windowed");
	if (video_flags & SDL_FULLSCREEN) {
		/* scale to the full display size... */
		dst_rect.x = 0;
		dst_rect.y = 0;
		dst_rect.width = display_width;
		dst_rect.height= display_height;
	} else {
		/* TODO: we could query the position of the X11 window here
		   and try to place the ovelray exactly above that...,
		   we would have to track window movements, though ... */
		dst_rect.x = 0;
		dst_rect.y = 0;
		dst_rect.width = static_cast<uint32_t>(x);
		dst_rect.height= static_cast<uint32_t>(y);
	}

	src_rect.x = 0;
	src_rect.y = 0;
	src_rect.width = (static_cast<uint32_t>(x))<< 16;
	src_rect.height =(static_cast<uint32_t>(y))<< 16;

	/* we do not want our overlay to be blended against the background */
	alpha_descriptor.flags=DISPMANX_FLAGS_ALPHA_FIXED_ALL_PIXELS;
	alpha_descriptor.opacity=0xffffffff;
	alpha_descriptor.mask=0;

	// open display, if we do not already have one ...
	if (dispman_display == DISPMANX_NO_HANDLE) {
		con_printf(CON_DEBUG, "RPi: opening display: %u",rpi_display_device);
		dispman_display = vc_dispmanx_display_open(rpi_display_device);
		if (dispman_display == DISPMANX_NO_HANDLE) {
			con_printf(CON_URGENT,"RPi: failed to open display: %u",rpi_display_device);
		}
	}

	if (dispman_element != DISPMANX_NO_HANDLE) {
		if (!update) {
			// if the element already exists, and we cannot update it, so recreate it
			rpi_destroy_element();
		}
	} else {
		// if the element does not exist, we cannot do an update
		update=0;
	}

	dispman_update = vc_dispmanx_update_start( 0 );

	if (update) {
		con_printf(CON_DEBUG, "RPi: updating display manager element");
		vc_dispmanx_element_change_attributes ( dispman_update, nativewindow.element,
							ELEMENT_CHANGE_DEST_RECT | ELEMENT_CHANGE_SRC_RECT,
							0 /*layer*/, 0 /*opacity*/,
							&dst_rect, &src_rect,
							0 /*mask*/,
							static_cast<DISPMANX_TRANSFORM_T>(VC_IMAGE_ROT0) /*transform*/);
	} else {
		// create a new element
		con_printf(CON_DEBUG, "RPi: creating display manager element");
		dispman_element = vc_dispmanx_element_add ( dispman_update, dispman_display,
								0 /*layer*/, &dst_rect, 0 /*src*/,
								&src_rect, DISPMANX_PROTECTION_NONE,
								&alpha_descriptor, NULL /*clamp*/,
								static_cast<DISPMANX_TRANSFORM_T>(VC_IMAGE_ROT0) /*transform*/);
		if (dispman_element == DISPMANX_NO_HANDLE) {
			con_printf(CON_URGENT,"RPi: failed to creat display manager elemenr");
		}
		nativewindow.element = dispman_element;
	}
	nativewindow.width = display_width;
	nativewindow.height = display_height;
	vc_dispmanx_update_submit_sync( dispman_update );

	return 0;
}

#endif // RPI

#if DXX_USE_OGLES
static void ogles_destroy()
{
	if( eglDisplay != EGL_NO_DISPLAY ) {
		eglMakeCurrent(eglDisplay, NULL, NULL, EGL_NO_CONTEXT);
	}

	if (eglContext != EGL_NO_CONTEXT) {
		con_printf(CON_DEBUG, "EGL: destroyig context");
		eglDestroyContext(eglDisplay, eglContext);
		eglContext = EGL_NO_CONTEXT;
	}

	if (eglSurface != EGL_NO_SURFACE) {
		con_printf(CON_DEBUG, "EGL: destroyig surface");
		eglDestroySurface(eglDisplay, eglSurface);
		eglSurface = EGL_NO_SURFACE;
	}

	if (eglDisplay != EGL_NO_DISPLAY) {
		con_printf(CON_DEBUG, "EGL: terminating");
		eglTerminate(eglDisplay);
		eglDisplay = EGL_NO_DISPLAY;
	}
}
#endif

static int ogl_init_window(int x, int y)
{
	int use_x,use_y,use_bpp;
	Uint32 use_flags;

#if DXX_USE_OGLES
	SDL_SysWMinfo info;
	Window    x11Window = 0;
	Display*  x11Display = 0;
	EGLint    ver_maj, ver_min;
	EGLint configAttribs[] =
	{
		EGL_RED_SIZE, 5,
		EGL_GREEN_SIZE, 6,
		EGL_BLUE_SIZE, 5,
		EGL_DEPTH_SIZE, 16,
		EGL_SURFACE_TYPE, EGL_WINDOW_BIT,
		EGL_RENDERABLE_TYPE, EGL_OPENGL_ES_BIT,
		EGL_NONE, EGL_NONE
	};

	// explicitely request an OpenGL ES 1.x context
        EGLint contextAttribs[] = { EGL_CONTEXT_CLIENT_VERSION, 1, EGL_NONE, EGL_NONE };
	// explicitely request a doublebuffering window
        EGLint winAttribs[] = { EGL_RENDER_BUFFER, EGL_BACK_BUFFER, EGL_NONE, EGL_NONE };

	int iConfigs;
#endif // OGLES

	if (gl_initialized)
		ogl_smash_texture_list_internal();//if we are or were fullscreen, changing vid mode will invalidate current textures

	SDL_WM_SetCaption(DESCENT_VERSION, DXX_SDL_WINDOW_CAPTION);
	SDL_WM_SetIcon( SDL_LoadBMP( DXX_SDL_WINDOW_ICON_BITMAP ), NULL );

	use_x=x;
	use_y=y;
	use_bpp = CGameArg.DbgBpp;
	use_flags=sdl_video_flags;
	if (sdl_no_modeswitch) {
		const SDL_VideoInfo *vinfo=SDL_GetVideoInfo();
		if (vinfo) {	
			use_x=vinfo->current_w;
			use_y=vinfo->current_h;
			use_bpp=vinfo->vfmt->BitsPerPixel;
			use_flags=SDL_SWSURFACE | SDL_ANYFORMAT;
		} else {
			con_printf(CON_URGENT, "Could not query video info");
		}
	}

	if (!SDL_SetVideoMode(use_x, use_y, use_bpp, use_flags))
	{
#ifdef RPI
		con_printf(CON_URGENT, "Could not set %dx%dx%d opengl video mode: %s\n (Ignored for RPI)",
			    x, y, CGameArg.DbgBpp, SDL_GetError());
#else
		Error("Could not set %dx%dx%d opengl video mode: %s\n", x, y, CGameArg.DbgBpp, SDL_GetError());
#endif
	}

#if DXX_USE_OGLES
#ifndef RPI
	// NOTE: on the RPi, the EGL stuff is not connected to the X11 window,
	//       so there is no need to destroy and recreate this
	ogles_destroy();
#endif

	SDL_VERSION(&info.version);
	
	if (SDL_GetWMInfo(&info) > 0) {
		if (info.subsystem == SDL_SYSWM_X11) {
			x11Display = info.info.x11.display;
			x11Window = info.info.x11.window;
			con_printf (CON_DEBUG, "Display: %p, Window: %i ===", x11Display, static_cast<int>(x11Window));
		}
	}

	if (eglDisplay == EGL_NO_DISPLAY) {
#ifdef RPI
		eglDisplay = eglGetDisplay((EGLNativeDisplayType)EGL_DEFAULT_DISPLAY);
#else
		eglDisplay = eglGetDisplay((EGLNativeDisplayType)x11Display);
#endif
		if (eglDisplay == EGL_NO_DISPLAY) {
			con_printf(CON_URGENT, "EGL: Error querying EGL Display");
		}

		if (!eglInitialize(eglDisplay, &ver_maj, &ver_min)) {
			con_printf(CON_URGENT, "EGL: Error initializing EGL");
		} else {
			con_printf(CON_DEBUG, "EGL: Initialized, version: major %i minor %i", ver_maj, ver_min);
		}
	}

	
#ifdef RPI
	if (rpi_setup_element(x,y,sdl_video_flags,1)) {
		Error("RPi: Could not set up a %dx%d element\n", x, y);
	}
#endif

	if (eglSurface == EGL_NO_SURFACE) {
		if (!eglChooseConfig(eglDisplay, configAttribs, &eglConfig, 1, &iConfigs) || (iConfigs != 1)) {
			con_printf(CON_URGENT, "EGL: Error choosing config");
		} else {
			con_printf(CON_DEBUG, "EGL: config chosen");
		}

#ifdef RPI
		eglSurface = eglCreateWindowSurface(eglDisplay, eglConfig, static_cast<EGLNativeWindowType>(&nativewindow), winAttribs);
#else
		eglSurface = eglCreateWindowSurface(eglDisplay, eglConfig, static_cast<NativeWindowType>(x11Window), winAttribs);
#endif
		if ((!TestEGLError("eglCreateWindowSurface")) || eglSurface == EGL_NO_SURFACE) {
			con_printf(CON_URGENT, "EGL: Error creating window surface");
		} else {
			con_printf(CON_DEBUG, "EGL: Created window surface");
		}
	}
	
	if (eglContext == EGL_NO_CONTEXT) {
		eglContext = eglCreateContext(eglDisplay, eglConfig, EGL_NO_CONTEXT, contextAttribs);
		if ((!TestEGLError("eglCreateContext")) || eglContext == EGL_NO_CONTEXT) {
			con_printf(CON_URGENT, "EGL: Error creating context");
		} else {
			con_printf(CON_DEBUG, "EGL: Created context");
		}
	}
	
	eglMakeCurrent(eglDisplay, eglSurface, eglSurface, eglContext);
	if (!TestEGLError("eglMakeCurrent")) {
		con_printf(CON_URGENT, "EGL: Error making current");
	} else {
		con_printf(CON_DEBUG, "EGL: made context current");
	}
#endif

	linedotscale = ((x/640<y/480?x/640:y/480)<1?1:(x/640<y/480?x/640:y/480));

	gl_initialized=1;
	return 0;
}

}

namespace dcx {

int gr_check_fullscreen(void)
{
	return !!(sdl_video_flags & SDL_FULLSCREEN);
}

void gr_toggle_fullscreen()
{
	const auto local_sdl_video_flags = (sdl_video_flags ^= SDL_FULLSCREEN);
	if (gl_initialized)
	{
		if (sdl_no_modeswitch == 0) {
			auto gsm = Game_screen_mode;
			const auto DbgBpp = CGameArg.DbgBpp;
			if (!SDL_VideoModeOK(gsm.width, gsm.height, DbgBpp, local_sdl_video_flags))
			{
				con_printf(CON_URGENT, "Cannot set %ix%i. Fallback to 640x480", gsm.width, gsm.height);
				gsm.width = 640;
				gsm.height = 480;
				Game_screen_mode = gsm;
			}
			if (!SDL_SetVideoMode(gsm.width, gsm.height, DbgBpp, local_sdl_video_flags))
			{
				Error("Could not set %dx%dx%d opengl video mode: %s\n", gsm.width, gsm.height, DbgBpp, SDL_GetError());
			}
		}
#ifdef RPI
		if (rpi_setup_element(SM_W(Game_screen_mode), SM_H(Game_screen_mode), local_sdl_video_flags, 1)) {
			 Error("RPi: Could not set up %dx%d element\n", SM_W(Game_screen_mode), SM_H(Game_screen_mode));
		}
#endif
	}

	gr_remap_color_fonts();

	if (gl_initialized) // update viewing values for menus
	{
		glMatrixMode(GL_PROJECTION);
		glLoadIdentity();
#if DXX_USE_OGLES
		glOrthof(0.0, 1.0, 0.0, 1.0, -1.0, 1.0);
#else
 		glOrtho(0.0, 1.0, 0.0, 1.0, -1.0, 1.0);
#endif
		glMatrixMode(GL_MODELVIEW);
		glLoadIdentity();//clear matrix
		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		ogl_smash_texture_list_internal();//if we are or were fullscreen, changing vid mode will invalidate current textures
	}
	CGameCfg.WindowMode = !(local_sdl_video_flags & SDL_FULLSCREEN);
}

static void ogl_init_state(void)
{
	/* select clearing (background) color   */
	glClearColor(0.0, 0.0, 0.0, 0.0);

	/* initialize viewing values */
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
#if DXX_USE_OGLES
	glOrthof(0.0, 1.0, 0.0, 1.0, -1.0, 1.0);
#else
 	glOrtho(0.0, 1.0, 0.0, 1.0, -1.0, 1.0);
#endif
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();//clear matrix
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	gr_palette_step_up(0,0,0);//in case its left over from in game

	ogl_init_pixel_buffers(grd_curscreen->get_screen_width(), grd_curscreen->get_screen_height());
}

}

namespace dsx {

static void ogl_tune_for_current(void)
{
	const auto gl_vendor = reinterpret_cast<const char *>(glGetString(GL_VENDOR));
	const auto gl_renderer = reinterpret_cast<const char *>(glGetString(GL_RENDERER));
	const auto gl_version = reinterpret_cast<const char *>(glGetString(GL_VERSION));

	con_printf(CON_VERBOSE, "OpenGL: vendor: %s\nOpenGL: renderer: %s", gl_vendor,gl_renderer);

	//add driver specific hacks here.  whee.
	if ((d_stricmp(gl_renderer,"Mesa NVIDIA RIVA 1.0\n")==0 || d_stricmp(gl_renderer,"Mesa NVIDIA RIVA 1.2\n")==0) && d_stricmp(gl_version,"1.2 Mesa 3.0")==0)
	{
		CGameArg.DbgGlIntensity4Ok = false;	//ignores alpha, always black background instead of transparent.
		CGameArg.DbgGlReadPixelsOk = false;	//either just returns all black, or kills the X server entirely
		CGameArg.DbgGlGetTexLevelParamOk = false;	//returns random data..
	}
	if (d_stricmp(gl_vendor,"Matrox Graphics Inc.")==0)
	{
		//displays garbage. reported by
		//  redomen@crcwnet.com (render="Matrox G400" version="1.1.3 5.52.015")
		//  orulz (Matrox G200)
		CGameArg.DbgGlIntensity4Ok = 0;
	}
#ifdef macintosh
	if (d_stricmp(gl_renderer,"3dfx Voodoo 3")==0) // strangely, includes Voodoo 2
		CGameArg.DbgGlGetTexLevelParamOk = false;	// Always returns 0
#endif

#ifndef NDEBUG
	con_printf(CON_VERBOSE,"gl_intensity4:%i gl_luminance4_alpha4:%i gl_rgba2:%i gl_readpixels:%i gl_gettexlevelparam:%i", CGameArg.DbgGlIntensity4Ok, CGameArg.DbgGlLuminance4Alpha4Ok, CGameArg.DbgGlRGBA2Ok, CGameArg.DbgGlReadPixelsOk, CGameArg.DbgGlGetTexLevelParamOk);
#endif
	if ( (ogl_maxanisotropy < 1.0f) && CGameCfg.TexAnisotropy ) {

		con_printf(CON_VERBOSE,"anisotropic texture filter not supported");
		CGameCfg.TexAnisotropy = false;
	}
}

}

namespace dcx {

// returns possible (fullscreen) resolutions if any.
uint_fast32_t gr_list_modes(array<screen_mode, 50> &gsmodes)
{
	SDL_Rect** modes;
	int modesnum = 0;
#if DXX_USE_OGLES
	int sdl_check_flags = SDL_FULLSCREEN; // always use Fullscreen as lead.
#else
	int sdl_check_flags = SDL_OPENGL | SDL_FULLSCREEN; // always use Fullscreen as lead.
#endif

	if (sdl_no_modeswitch) {
		/* TODO: we could use the tvservice to list resolutions on the RPi */
		return 0;
	}

	modes = SDL_ListModes(NULL, sdl_check_flags);

	if (modes == reinterpret_cast<SDL_Rect **>(0)) // check if we get any modes - if not, return 0
		return 0;


	if (modes == reinterpret_cast<SDL_Rect**>(-1))
	{
		return 0; // can obviously use any resolution... strange!
	}
	else
	{
		for (int i = 0; modes[i]; ++i)
		{
			if (modes[i]->w > 0xFFF0 || modes[i]->h > 0xFFF0 // resolutions saved in 32bits. so skip bigger ones (unrealistic in 2010) (changed to 0xFFF0 to fix warning)
				|| modes[i]->w < 320 || modes[i]->h < 200) // also skip everything smaller than 320x200
				continue;
			gsmodes[modesnum].width = modes[i]->w;
			gsmodes[modesnum].height = modes[i]->h;
			modesnum++;
			if (modesnum >= gsmodes.size()) // that really seems to be enough big boy.
				break;
		}
		return modesnum;
	}
}

}

namespace dsx {

static int gr_check_mode(const screen_mode mode)
{
	if (sdl_no_modeswitch == 0) {
		return SDL_VideoModeOK(SM_W(mode), SM_H(mode), CGameArg.DbgBpp, sdl_video_flags);
	} else {
		// just tell the caller that any mode is valid...
		return 32;
	}
}

int gr_set_mode(screen_mode mode)
{
	unsigned char *gr_bm_data;
	if (!gr_check_mode(mode))
	{
		con_printf(CON_URGENT, "Cannot set %ix%i. Fallback to 640x480", mode.width, mode.height);
		mode.width = 640;
		mode.height = 480;
		Game_screen_mode = mode;
	}
	const uint_fast32_t w = SM_W(mode), h = SM_H(mode);

	gr_bm_data = grd_curscreen->sc_canvas.cv_bitmap.get_bitmap_data();//since we use realloc, we want to keep this pointer around.
	auto gr_new_bm_data = reinterpret_cast<uint8_t *>(d_realloc(gr_bm_data, w * h));
	if (!gr_new_bm_data)
		return 0;
	*grd_curscreen = {};
	grd_curscreen->set_screen_width_height(w, h);
	grd_curscreen->sc_aspect = fixdiv(grd_curscreen->get_screen_width() * GameCfg.AspectX, grd_curscreen->get_screen_height() * GameCfg.AspectY);
	gr_init_canvas(grd_curscreen->sc_canvas, gr_new_bm_data, bm_mode::ogl, w, h);
	gr_set_current_canvas(NULL);

	ogl_init_window(w,h);//platform specific code
	ogl_extensions_init();
	ogl_tune_for_current();
	sync_helper.init(CGameArg.OglSyncMethod, CGameArg.OglSyncWait);

	OGL_VIEWPORT(0,0,w,h);
	ogl_init_state();
	gamefont_choose_game_font(w,h);
	gr_remap_color_fonts();

	return 0;
}

#define GLstrcmptestr(a,b) if (d_stricmp(a,#b)==0 || d_stricmp(a,"GL_" #b)==0)return GL_ ## b;

#ifdef _WIN32
constexpr char OglLibPath[]="opengl32.dll";

static int ogl_rt_loaded=0;
static int ogl_init_load_library(void)
{
	int retcode=0;
	if (!ogl_rt_loaded)
	{
		retcode = OpenGL_LoadLibrary(true, OglLibPath);
		if(retcode)
		{
			if(!glEnd)
			{
				Error("Opengl: Functions not imported\n");
			}
		}
		else
		{
			Error("Opengl: error loading %s\n", OglLibPath);
		}
		ogl_rt_loaded=1;
	}
	return retcode;
}
#endif

void gr_set_attributes(void)
{
#if !DXX_USE_OGLES
	SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 16);
	SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE,0);
	SDL_GL_SetAttribute(SDL_GL_ACCUM_RED_SIZE,0);
	SDL_GL_SetAttribute(SDL_GL_ACCUM_GREEN_SIZE,0);
	SDL_GL_SetAttribute(SDL_GL_ACCUM_BLUE_SIZE,0);
	SDL_GL_SetAttribute(SDL_GL_ACCUM_ALPHA_SIZE,0);
	SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER,1);
	SDL_GL_SetAttribute(SDL_GL_SWAP_CONTROL, CGameCfg.VSync);
	int buffers, samples;
	if (CGameCfg.Multisample)
	{
		buffers = 1;
		samples = 4;
	}
	else
	{
		buffers = samples = 0;
	}
	SDL_GL_SetAttribute(SDL_GL_MULTISAMPLEBUFFERS, buffers);
	SDL_GL_SetAttribute(SDL_GL_MULTISAMPLESAMPLES, samples);
#endif
	ogl_smash_texture_list_internal();
	gr_remap_color_fonts();
}

int gr_init()
{
	// Only do this function once!
	if (gr_installed==1)
		return -1;

#ifdef RPI
	// Initialize the broadcom host library
	// we have to call this before we can create an OpenGL ES context
	bcm_host_init();

	// Check if we are running with SDL directfb driver ...
	char sdl_driver[32];
	if (auto sdl_driver_ret = SDL_VideoDriverName(sdl_driver, sizeof(sdl_driver)))
	{
		if (strcmp(sdl_driver_ret,"x11")) {
			con_printf(CON_URGENT,"RPi: activating hack for console driver");
			sdl_no_modeswitch=1;
		}
	}
#endif

#ifdef _WIN32
	ogl_init_load_library();
#endif

	if (!CGameCfg.WindowMode && !CGameArg.SysWindow)
		sdl_video_flags|=SDL_FULLSCREEN;

	if (CGameArg.SysNoBorders)
		sdl_video_flags|=SDL_NOFRAME;

	gr_set_attributes();

	ogl_init_texture_list_internal();

	grd_curscreen = make_unique<grs_screen>();
	*grd_curscreen = {};
	grd_curscreen->sc_canvas.cv_bitmap.bm_data = NULL;

	// Set the mode.
	grd_curscreen->sc_canvas.cv_fade_level = GR_FADE_OFF;
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
		sync_helper.deinit();
	}

	if (grd_curscreen)
	{
		if (grd_curscreen->sc_canvas.cv_bitmap.bm_mdata)
			d_free(grd_curscreen->sc_canvas.cv_bitmap.bm_mdata);
		grd_curscreen.reset();
	}
	ogl_close_pixel_buffers();
#ifdef _WIN32
	if (ogl_rt_loaded)
		OpenGL_LoadLibrary(false, OglLibPath);
#endif

#if DXX_USE_OGLES
	ogles_destroy();
#ifdef RPI
	con_printf(CON_DEBUG, "RPi: cleanuing up");
	if (dispman_display != DISPMANX_NO_HANDLE) {
		rpi_destroy_element();
		con_printf(CON_DEBUG, "RPi: closing display");
		vc_dispmanx_display_close(dispman_display);
		dispman_display = DISPMANX_NO_HANDLE;
	}
#endif
#endif
}

}

namespace dcx {

void ogl_upixelc(const grs_canvas &canv, unsigned x, unsigned y, unsigned c)
{
	GLfloat vertex_array[] = {
		(x + canv.cv_bitmap.bm_x) / static_cast<float>(last_width),
		static_cast<GLfloat>(1.0 - (y + canv.cv_bitmap.bm_y) / static_cast<float>(last_height))
	};
	const auto cr = static_cast<GLfloat>(CPAL2Tr(c));
	const auto cg = static_cast<GLfloat>(CPAL2Tg(c));
	const auto cb = static_cast<GLfloat>(CPAL2Tb(c));
	GLfloat color_array[] = {
		cr, cg, cb, 1.0,
		cr, cg, cb, 1.0,
		cr, cg, cb, 1.0,
		cr, cg, cb, 1.0
	};

	OGL_DISABLE(TEXTURE_2D);
	glPointSize(linedotscale);
	glEnableClientState(GL_VERTEX_ARRAY);
	glEnableClientState(GL_COLOR_ARRAY);
	glVertexPointer(2, GL_FLOAT, 0, vertex_array);
	glColorPointer(4, GL_FLOAT, 0, color_array);
	glDrawArrays(GL_POINTS, 0, 1);
	glDisableClientState(GL_VERTEX_ARRAY);
	glDisableClientState(GL_COLOR_ARRAY);
}

unsigned char ogl_ugpixel(const grs_bitmap &bitmap, unsigned x, unsigned y)
{
	ubyte buf[4];

#if !DXX_USE_OGLES
	GLint gl_draw_buffer;
	glGetIntegerv(GL_DRAW_BUFFER, &gl_draw_buffer);
	glReadBuffer(gl_draw_buffer);
#endif

	glReadPixels(bitmap.bm_x + x, SHEIGHT - bitmap.bm_y - y, 1, 1, GL_RGBA, GL_UNSIGNED_BYTE, buf);
	
	return gr_find_closest_color(buf[0]/4, buf[1]/4, buf[2]/4);
}

void ogl_urect(grs_canvas &canvas, const int left, const int top, const int right, const int bot, const int c)
{
	GLfloat xo, yo, xf, yf, color_r, color_g, color_b, color_a;

	glEnableClientState(GL_VERTEX_ARRAY);
	glEnableClientState(GL_COLOR_ARRAY);

	xo = (left + canvas.cv_bitmap.bm_x) / static_cast<float>(last_width);
	xf = (right + 1 + canvas.cv_bitmap.bm_x) / static_cast<float>(last_width);
	yo = 1.0 - (top + canvas.cv_bitmap.bm_y) / static_cast<float>(last_height);
	yf = 1.0 - (bot + 1 + canvas.cv_bitmap.bm_y) / static_cast<float>(last_height);

	OGL_DISABLE(TEXTURE_2D);

	color_r = CPAL2Tr(c);
	color_g = CPAL2Tg(c);
	color_b = CPAL2Tb(c);

	if (canvas.cv_fade_level >= GR_FADE_OFF)
		color_a = 1.0;
	else
		color_a = 1.0 - static_cast<float>(canvas.cv_fade_level) / (static_cast<float>(GR_FADE_LEVELS) - 1.0);

	array<GLfloat, 16> color_array;
	color_array[0] = color_array[4] = color_array[8] = color_array[12] = color_r;
	color_array[1] = color_array[5] = color_array[9] = color_array[13] = color_g;
	color_array[2] = color_array[6] = color_array[10] = color_array[14] = color_b;
	color_array[3] = color_array[7] = color_array[11] = color_array[15] = color_a;

	array<GLfloat, 8> vertex_array;
	vertex_array[0] = xo;
	vertex_array[1] = yo;
	vertex_array[2] = xo;
	vertex_array[3] = yf;
	vertex_array[4] = xf;
	vertex_array[5] = yf;
	vertex_array[6] = xf;
	vertex_array[7] = yo;
	
	glVertexPointer(2, GL_FLOAT, 0, vertex_array.data());
	glColorPointer(4, GL_FLOAT, 0, color_array.data());
	glDrawArrays(GL_TRIANGLE_FAN, 0, 4);//replaced GL_QUADS
	glDisableClientState(GL_VERTEX_ARRAY);
}

void ogl_ulinec(int left,int top,int right,int bot,int c)
{
	GLfloat xo,yo,xf,yf;
	GLfloat fade_alpha = (grd_curcanv->cv_fade_level >= GR_FADE_OFF)?1.0:1.0 - static_cast<float>(grd_curcanv->cv_fade_level) / (static_cast<float>(GR_FADE_LEVELS) - 1.0);
	GLfloat color_array[] = {
		static_cast<GLfloat>(CPAL2Tr(c)), static_cast<GLfloat>(CPAL2Tg(c)), static_cast<GLfloat>(CPAL2Tb(c)), fade_alpha,
		static_cast<GLfloat>(CPAL2Tr(c)), static_cast<GLfloat>(CPAL2Tg(c)), static_cast<GLfloat>(CPAL2Tb(c)), fade_alpha,
		static_cast<GLfloat>(CPAL2Tr(c)), static_cast<GLfloat>(CPAL2Tg(c)), static_cast<GLfloat>(CPAL2Tb(c)), 1.0,
		static_cast<GLfloat>(CPAL2Tr(c)), static_cast<GLfloat>(CPAL2Tg(c)), static_cast<GLfloat>(CPAL2Tb(c)), fade_alpha
	};
	GLfloat vertex_array[] = { 0.0, 0.0, 0.0, 0.0 };

	glEnableClientState(GL_VERTEX_ARRAY);
	glEnableClientState(GL_COLOR_ARRAY);
	
	xo = (left + grd_curcanv->cv_bitmap.bm_x) / static_cast<float>(last_width);
	xf = (right + grd_curcanv->cv_bitmap.bm_x) / static_cast<float>(last_width);
	yo = 1.0 - (top + grd_curcanv->cv_bitmap.bm_y + 0.5) / static_cast<float>(last_height);
	yf = 1.0 - (bot + grd_curcanv->cv_bitmap.bm_y + 0.5) / static_cast<float>(last_height);
 
	OGL_DISABLE(TEXTURE_2D);

	vertex_array[0] = xo;
	vertex_array[1] = yo;
	vertex_array[2] = xf;
	vertex_array[3] = yf;

	glVertexPointer(2, GL_FLOAT, 0, vertex_array);
	glColorPointer(4, GL_FLOAT, 0, color_array);
	glDrawArrays(GL_LINES, 0, 2);
	glDisableClientState(GL_VERTEX_ARRAY);
	glDisableClientState(GL_COLOR_ARRAY);
}

static GLfloat last_r, last_g, last_b;
static int do_pal_step;

void ogl_do_palfx(void)
{
	GLfloat color_array[] = { last_r, last_g, last_b, 1.0, last_r, last_g, last_b, 1.0, last_r, last_g, last_b, 1.0, last_r, last_g, last_b, 1.0 };
	GLfloat vertex_array[] = { 0,0,0,1,1,1,1,0 };

	OGL_DISABLE(TEXTURE_2D);

	glEnableClientState(GL_VERTEX_ARRAY);
	glEnableClientState(GL_COLOR_ARRAY);
 
	if (do_pal_step)
	{
		glEnable(GL_BLEND);
		glBlendFunc(GL_ONE,GL_ONE);
	}
	else
		return;
 
	glVertexPointer(2, GL_FLOAT, 0, vertex_array);
	glColorPointer(4, GL_FLOAT, 0, color_array);
	glDrawArrays(GL_TRIANGLE_FAN, 0, 4);//replaced GL_QUADS
	glDisableClientState(GL_VERTEX_ARRAY);
	glDisableClientState(GL_COLOR_ARRAY);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
}

static int ogl_brightness_ok;
static int old_b_r, old_b_g, old_b_b;

void gr_palette_step_up(int r, int g, int b)
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

void gr_palette_load( palette_array_t &pal )
{
	copy_bound_palette(gr_current_pal, pal);

	gr_palette_step_up(0, 0, 0); // make ogl_setbrightness_internal get run so that menus get brightened too.
	init_computed_colors();
}

#define GL_BGR_EXT 0x80E0

struct TGA_header
{
      unsigned char TGAheader[12];
      unsigned char header[6];
};

//writes out an uncompressed RGB .tga file
//if we got really spiffy, we could optionally link in libpng or something, and use that.
static void write_bmp(char *savename,unsigned w,unsigned h)
{
	TGA_header TGA;
	GLbyte HeightH,HeightL,WidthH,WidthL;
	RAIIdmem<uint8_t[]> buf;
	const unsigned buffer_size_TGA = w * h * 3;
	CALLOC(buf, uint8_t[], buffer_size_TGA);

	RAIIdmem<uint8_t[]> rgbaBuf;
	CALLOC(rgbaBuf, uint8_t[], w * h * 4);
	glReadPixels(0, 0, w, h, GL_RGBA, GL_UNSIGNED_BYTE, rgbaBuf.get());
	for(unsigned int pixel = 0; pixel < w * h; pixel++) {
		buf[pixel * 3] = rgbaBuf[pixel * 4 + 2];
		buf[pixel * 3 + 1] = rgbaBuf[pixel * 4 + 1];
		buf[pixel * 3 + 2] = rgbaBuf[pixel * 4];
	}

	auto TGAFile = PHYSFSX_openWriteBuffered(savename);
	if (!TGAFile)
	{
		con_printf(CON_URGENT,"Could not create TGA file to dump screenshot!");
		return;
	}

	HeightH = static_cast<GLbyte>(h / 256);
	HeightL = static_cast<GLbyte>(h % 256);
	WidthH  = static_cast<GLbyte>(w / 256);
	WidthL  = static_cast<GLbyte>(w % 256);
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
	TGA.header[0] = WidthL;
	TGA.header[1] = WidthH;
	TGA.header[2] = HeightL;
	TGA.header[3] = HeightH;
	TGA.header[4] = static_cast<GLbyte>(24);
	TGA.header[5] = 0;
	PHYSFS_write(TGAFile,&TGA,sizeof(TGA_header),1);
	PHYSFS_write(TGAFile,buf, buffer_size_TGA * sizeof(unsigned char),1);
}

void save_screen_shot(int automap_flag)
{
	static int savenum=0;
	char savename[13+sizeof(SCRNS_DIR)];

	if (!CGameArg.DbgGlReadPixelsOk)
	{
		if (!automap_flag)
			HUD_init_message_literal(HM_DEFAULT, "glReadPixels not supported on your configuration");
		return;
	}

	pause_game_world_time p;

	if (!PHYSFSX_exists(SCRNS_DIR,0))
		PHYSFS_mkdir(SCRNS_DIR); //try making directory

	do
	{
		snprintf(savename, sizeof(savename), "%sscrn%04d.tga", SCRNS_DIR, savenum++);
                if (savenum >= 9999) break; // that's enough I think.
	} while (PHYSFSX_exists(savename,0));

	if (!automap_flag)
		HUD_init_message(HM_DEFAULT, "%s '%s'", TXT_DUMPING_SCREEN, &savename[sizeof(SCRNS_DIR) - 1]);

#if !DXX_USE_OGLES
	glReadBuffer(GL_FRONT);
#endif

	write_bmp(savename, grd_curscreen->get_screen_width(), grd_curscreen->get_screen_height());
}

}
