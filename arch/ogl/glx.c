/* $Id: glx.c,v 1.4 2003-01-15 02:40:54 btb Exp $ */
/*
 *
 * opengl platform specific functions for GLX - Added 9/15/99 Matthew Mueller
 *
 *
 */

#ifdef HAVE_CONFIG_H
#include <conf.h>
#endif

#include <X11/Xlib.h>
#include <GL/glx.h>
#include <string.h>
#include "ogl_init.h"
#include "vers_id.h"
#include "error.h"
#include "event.h"
#include "mono.h"
#include "u_mem.h"
#ifdef XFREE86_DGA
#include <X11/extensions/xf86dga.h>
#include <X11/extensions/xf86vmode.h>
#endif


#include <X11/Xatom.h>

//#define HAVE_MOTIF
#ifdef HAVE_MOTIF

#include <X11/Xm/MwmUtil.h>

#else

/* bit definitions for MwmHints.flags */
#define MWM_HINTS_FUNCTIONS (1L << 0)
#define MWM_HINTS_DECORATIONS (1L << 1)
#define MWM_HINTS_INPUT_MODE (1L << 2)
#define MWM_HINTS_STATUS (1L << 3)

/* bit definitions for MwmHints.functions */
#define MWM_FUNC_ALL            (1L << 0)
#define MWM_FUNC_RESIZE         (1L << 1)
#define MWM_FUNC_MOVE           (1L << 2)
#define MWM_FUNC_MINIMIZE       (1L << 3)
#define MWM_FUNC_MAXIMIZE       (1L << 4)
#define MWM_FUNC_CLOSE          (1L << 5)


/* bit definitions for MwmHints.decorations */
#define MWM_DECOR_ALL  (1L << 0)
#define MWM_DECOR_BORDER (1L << 1)
#define MWM_DECOR_RESIZEH (1L << 2)
#define MWM_DECOR_TITLE  (1L << 3)
#define MWM_DECOR_MENU  (1L << 4)
#define MWM_DECOR_MINIMIZE (1L << 5)
#define MWM_DECOR_MAXIMIZE (1L << 6)

typedef struct
{
	unsigned long flags;
	unsigned long functions;
	unsigned long decorations;
	long          inputMode;
	unsigned long status;
} PropMotifWmHints;

#define PROP_MOTIF_WM_HINTS_ELEMENTS 5

#endif



/*
 * Specify which Motif window manager border decorations to put on a * top-level window.  For example, you can specify that
 a window is not * resizabe, or omit the titlebar, or completely remove all decorations. * Input:  dpy - the X display
 *        w - the X window
 *        flags - bitwise-OR of the MWM_DECOR_xxx symbols in
 X11/Xm/MwmUtil.h
 *                indicating what decoration elements to enable.  Zero would
 *                be no decoration.
 */
void set_mwm_border( Display *dpy, Window w, unsigned long dflags,unsigned long fflags ) {
	PropMotifWmHints motif_hints;
	Atom prop, proptype;

	/* setup the property */
	motif_hints.flags = MWM_HINTS_DECORATIONS|MWM_HINTS_FUNCTIONS;
	motif_hints.decorations = dflags;
	motif_hints.functions = fflags;

	/* get the atom for the property */
	prop = XInternAtom( dpy, "_MOTIF_WM_HINTS", True );   if (!prop) {
		mprintf((0,"set_mwm_border: prop==0\n"));
		/* something went wrong! */
		return;
	}

	/* not sure this is correct, seems to work, XA_WM_HINTS didn't work */   proptype = prop;

	XChangeProperty( dpy, w, /* display, window */ prop, proptype, /* property, type */ 
			32, /* format: 32-bit datums */ PropModeReplace, /* mode */
			(unsigned char *) &motif_hints, /* data */ PROP_MOTIF_WM_HINTS_ELEMENTS /* nelements */);
}

int glx_erbase,glx_evbase;
Display *dpy;
XSetWindowAttributes swa;
Window win;
XVisualInfo *visinfo;
GLXContext glxcontext;
Pixmap blankpixmap=None;
Cursor blankcursor=None;


void set_wm_hints(int fullscreen){
	XSizeHints *hints=NULL;
		
	
//	return;//seems screwed.
	if (!hints) hints=XAllocSizeHints();
	hints->width=hints->min_width=hints->max_width=hints->base_width=grd_curscreen->sc_w;
	hints->height=hints->min_height=hints->max_height=hints->base_height=grd_curscreen->sc_h;
	hints->flags=PSize|PMinSize|PMaxSize|PBaseSize;
//	hints->min_width=hints->max_width=grd_curscreen->sc_w;
//	hints->min_height=hints->max_height=grd_curscreen->sc_h;
//	hints->flags=PMinSize|PMaxSize;
	XSetWMNormalHints(dpy,win,hints);
	XFree(hints);
	if (fullscreen){
		set_mwm_border(dpy,win,0,0);
	}else{
		set_mwm_border(dpy,win,MWM_DECOR_TITLE|MWM_DECOR_BORDER,MWM_FUNC_MOVE|MWM_FUNC_CLOSE);
	}
}

static int attribs[]={GLX_RGBA,GLX_DOUBLEBUFFER,GLX_DEPTH_SIZE,0,GLX_STENCIL_SIZE,0,
	GLX_ACCUM_RED_SIZE,0,GLX_ACCUM_GREEN_SIZE,0,GLX_ACCUM_BLUE_SIZE,0,GLX_ACCUM_ALPHA_SIZE,0,None};

void ogl_do_fullscreen_internal(void){
//	ogl_smash_texture_list_internal();//not needed
	if (ogl_fullscreen){
		set_wm_hints(1);
		XMoveWindow(dpy,win,0,0);
		//			XDefineCursor(dpy,win,blankcursor);
		//XGrabPointer(dpy,win,0,swa.event_mask,GrabModeAsync,GrabModeAsync,win,blankcursor,CurrentTime);
		XGrabPointer(dpy,win,1,ButtonPressMask | ButtonReleaseMask | PointerMotionMask,GrabModeAsync,GrabModeAsync,win,blankcursor,CurrentTime);
		//			XGrabKeyboard(dpy,win,1,GrabModeAsync,GrabModeAsync,CurrentTime);//grabbing keyboard doesn't seem to do much good anyway.

#ifdef XFREE86_DGA
		//make ogl_fullscreen
		//can you even do this with DGA ?  just resizing with ctrl-alt-(-/+) caused X to die a horrible death.
		//might have to kill the window/context/whatever first?  HRm.
#endif
	}else{
		set_wm_hints(0);
		//			XUndefineCursor(dpy,win);
		XUngrabPointer(dpy,CurrentTime);
		//			XUngrabKeyboard(dpy,CurrentTime);
#ifdef XFREE86_DGA
		//return to normal
#endif
	}
}

inline void ogl_swap_buffers_internal(void){
	glXSwapBuffers(dpy,win);
}
int ogl_init_window(int x, int y){
	if (gl_initialized){
		XResizeWindow(dpy,win,x,y);
		set_wm_hints(ogl_fullscreen);

	}else {
		glxcontext=glXCreateContext(dpy,visinfo,0,GL_TRUE);

		//create colormap
		swa.colormap=XCreateColormap(dpy,RootWindow(dpy,visinfo->screen),visinfo->visual,AllocNone);
		//create window
		swa.border_pixel=0;
		swa.event_mask=ExposureMask | StructureNotifyMask | KeyPressMask | KeyReleaseMask| ButtonPressMask | ButtonReleaseMask | PointerMotionMask;
		//win = XCreateWindow(dpy,RootWindow(dpy,visinfo->screen),0,0,x,y,0,visinfo->depth,InputOutput,visinfo->visual,CWBorderPixel|CWColormap|CWEventMask,&swa);
		win = XCreateWindow(dpy,RootWindow(dpy,visinfo->screen),0,0,x,y,0,visinfo->depth,InputOutput,visinfo->visual,CWColormap|CWEventMask,&swa);
	
		XStoreName(dpy,win,DESCENT_VERSION);
//		XStoreName(dpy,win,"agry");
		
		XMapWindow(dpy,win);
		
		glXMakeCurrent(dpy,win,glxcontext);
		
		set_wm_hints(ogl_fullscreen);

		gl_initialized=1;

		{
			XColor blankcolor;
			unsigned char *blankdata;
			int w,h;
			XQueryBestCursor(dpy,win,1,1,&w,&h);
//			mprintf((0,"bestcursor %ix%i\n",w,h));
			blankdata=d_malloc(w*h/8);
			memset(blankdata,0,w*h/8);
			memset(&blankcolor,0,sizeof(XColor));
			blankpixmap=XCreateBitmapFromData(dpy,win,blankdata,w,h);
			blankcursor=XCreatePixmapCursor(dpy,blankpixmap,blankpixmap,&blankcolor,&blankcolor,w,h);
			d_free(blankdata);
//			sleep(1);
		}
		
		if (ogl_fullscreen)
			ogl_do_fullscreen_internal();
//			gr_do_fullscreen(ogl_fullscreen);
	}
#ifdef GII_XWIN
	init_gii_xwin(dpy,win);
#endif
	return 0;
}
void ogl_destroy_window(void){
	if (gl_initialized){
		glXDestroyContext(dpy,glxcontext);
		XDestroyWindow(dpy,win);
		XFreeColormap(dpy,swa.colormap);
		gl_initialized=0;
	}
	return;
}
void ogl_init(void){
	dpy=XOpenDisplay(0);
	if(!dpy)
		Error("no display\n");
	else if (glXQueryExtension(dpy,&glx_erbase,&glx_evbase)==False)
		Error("no glx\n");
	else if (!(visinfo = glXChooseVisual(dpy,DefaultScreen(dpy),attribs)))
		Error("no visual\n");
}
void ogl_close(void){
	if (ogl_fullscreen){
		ogl_fullscreen=0;
		ogl_do_fullscreen_internal();
	}
	ogl_destroy_window();
	if (blankcursor!=None){
		XFreeCursor(dpy,blankcursor);
		XFreePixmap(dpy,blankpixmap);
	}
	XCloseDisplay(dpy);
}
