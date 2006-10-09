//opengl platform specific functions for SDL - Added 2000/06/19 Matthew Mueller
#include <SDL/SDL.h>
#include "ogl_init.h"
#include "vers_id.h"
#include "error.h"
#include "u_mem.h"
#include "args.h"

static int curx=-1,cury=-1,curfull=0;

void ogl_do_fullscreen_internal(void){
	ogl_init_window(curx,cury);
}

void ogl_swap_buffers_internal(void){
	SDL_GL_SwapBuffers();
}
int ogl_init_window(int x, int y){
	if (gl_initialized){
		if (x==curx && y==cury && curfull==ogl_fullscreen)
			return 0;
#ifdef __LINUX__
		if (ogl_fullscreen || curfull)
#endif
			ogl_smash_texture_list_internal();//if we are or were fullscreen, changing vid mode will invalidate current textures
	}
	SDL_WM_SetCaption(DESCENT_VERSION " " D1X_DATE, "Descent");

        if (!SDL_SetVideoMode(x,y, 16, SDL_OPENGL | (ogl_fullscreen?SDL_FULLSCREEN:0))) {
           Error("Could not set %dx%dx16 opengl video mode\n",x,y);
        }
	
	SDL_ShowCursor(0);

	curx=x;cury=y;curfull=ogl_fullscreen;
	gl_initialized=1;

	return 0;
}
void ogl_destroy_window(void){
	if (gl_initialized){
		ogl_smash_texture_list_internal();
		SDL_ShowCursor(1);
		//gl_initialized=0;
		//well..SDL doesn't really let you kill the window.. so we just need to wait for sdl_quit
	}
	return;
}

void ogl_init(void){
	int t;
	if ((t=FindArg("-gl_red")))
		SDL_GL_SetAttribute( SDL_GL_RED_SIZE, atoi(Args[t+1]) );
	if ((t=FindArg("-gl_green")))
		SDL_GL_SetAttribute( SDL_GL_GREEN_SIZE, atoi(Args[t+1]) );
	if ((t=FindArg("-gl_blue")))
		SDL_GL_SetAttribute( SDL_GL_BLUE_SIZE, atoi(Args[t+1]) );
	if ((t=FindArg("-gl_alpha")))
		SDL_GL_SetAttribute( SDL_GL_ALPHA_SIZE, atoi(Args[t+1]) );
	if ((t=FindArg("-gl_buffer")))
		SDL_GL_SetAttribute( SDL_GL_BUFFER_SIZE, atoi(Args[t+1]) );
	if ((t=FindArg("-gl_fsaa")) && atoi(Args[t+1]) == (2 || 4))
	{
		SDL_GL_SetAttribute(SDL_GL_MULTISAMPLEBUFFERS,1);
		SDL_GL_SetAttribute(SDL_GL_MULTISAMPLESAMPLES,atoi(Args[t+1]));
	}

	SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE,16);
	SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE,0);
	SDL_GL_SetAttribute(SDL_GL_ACCUM_RED_SIZE,0);
	SDL_GL_SetAttribute(SDL_GL_ACCUM_GREEN_SIZE,0);
	SDL_GL_SetAttribute(SDL_GL_ACCUM_BLUE_SIZE,0);
	SDL_GL_SetAttribute(SDL_GL_ACCUM_ALPHA_SIZE,0);
	SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER,1);

}
void ogl_close(void){
	ogl_destroy_window();
}
