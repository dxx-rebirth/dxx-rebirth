/*
 * $Source: /cvs/cvsroot/d2x/video/ogl_sdl.c,v $
 * $Revision: 1.3 $
 * $Author: bradleyb $
 * $Date: 2001-10-09 02:58:20 $
 *
 * Graphics functions for SDL-GL.
 *
 * $Log: not supported by cvs2svn $
 * Revision 1.2  2001/01/29 13:47:52  bradleyb
 * Fixed build, some minor cleanups.
 *
 */

#ifdef HAVE_CONFIG_H
#include <conf.h>
#endif

#include <SDL/SDL.h>
#include <GL/gl.h>
#include <string.h>
#include "ogl_init.h"
#include "vers_id.h"
#include "error.h"
#include "event.h"
#include "mono.h"
#include "u_mem.h"

static int gl_initialized = 0;

void ogl_do_fullscreen_internal(void){
    if (ogl_fullscreen)
    {
        SDL_Surface *screen;

        screen = SDL_GetVideoSurface();
        SDL_WM_ToggleFullScreen(screen);
    }

}

inline void ogl_swap_buffers_internal(void){
	SDL_GL_SwapBuffers();
}
int ogl_init_window(int x, int y){

	Uint32 video_flags;
	int bpp = 16;


	if (SDL_Init(SDL_INIT_VIDEO) < 0)
        {
                Error("SDL library video initialisation failed: %s.",SDL_GetError());
        }
	video_flags = SDL_OPENGL;
	
	if (ogl_fullscreen) video_flags |= SDL_FULLSCREEN;

        SDL_GL_SetAttribute( SDL_GL_RED_SIZE, 5 );
        SDL_GL_SetAttribute( SDL_GL_GREEN_SIZE, 5 );
        SDL_GL_SetAttribute( SDL_GL_BLUE_SIZE, 5 );
//        SDL_GL_SetAttribute( SDL_GL_ALPHA_SIZE, 1 );
        SDL_GL_SetAttribute( SDL_GL_DEPTH_SIZE, bpp );
        SDL_GL_SetAttribute( SDL_GL_DOUBLEBUFFER, 1 );

	SDL_WM_SetCaption("D2x", "Descent II");

        if ( SDL_SetVideoMode( x, y, bpp, video_flags ) == NULL ) {
                fprintf(stderr, "Couldn't set GL mode: %s\n", SDL_GetError());
                SDL_Quit();
                exit(1);
        }

	SDL_ShowCursor(0);

        printf( "Vendor     : %s\n", glGetString( GL_VENDOR ) );
        printf( "Renderer   : %s\n", glGetString( GL_RENDERER ) );
        printf( "Version    : %s\n", glGetString( GL_VERSION ) );
        printf( "Extensions : %s\n", glGetString( GL_EXTENSIONS ) );
        printf("\n");

		
	return 0;
}
void ogl_destroy_window(void){
	return;
}
void ogl_init(void){
	gl_initialized=1;
}
void ogl_close(void){
	ogl_destroy_window();
}
