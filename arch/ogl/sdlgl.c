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

static Uint16 gammaramp[512];

static void init_gammaramp(void)
{
	int i;

	for (i = 0; i < 256; ++i)
		gammaramp[i] = i * 256;
	for (i = 256; i < 512; ++i)
		gammaramp[i] = 0xffff;
}

int ogl_setbrightness_internal(void)
{
	return SDL_SetGammaRamp(gammaramp + ogl_brightness_r * 4,
	                        gammaramp + ogl_brightness_g * 4,
	                        gammaramp + ogl_brightness_b * 4
							);
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
	SDL_WM_SetCaption(DESCENT_VERSION, "Descent");

        if (!SDL_SetVideoMode(x,y, GameArg.GlBpp, SDL_OPENGL | (ogl_fullscreen?SDL_FULLSCREEN:0))) {
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
	SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE,16);
	SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE,0);
	SDL_GL_SetAttribute(SDL_GL_ACCUM_RED_SIZE,0);
	SDL_GL_SetAttribute(SDL_GL_ACCUM_GREEN_SIZE,0);
	SDL_GL_SetAttribute(SDL_GL_ACCUM_BLUE_SIZE,0);
	SDL_GL_SetAttribute(SDL_GL_ACCUM_ALPHA_SIZE,0);
	SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER,1);

	init_gammaramp();
}
void ogl_close(void){
	ogl_destroy_window();
}
