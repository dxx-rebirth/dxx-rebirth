/* $Id: mve_main.c,v 1.3 2003-06-10 04:46:16 btb Exp $ */

#include <stdlib.h>
#include <assert.h>

#include <SDL.h>

#include "libmve.h"


static SDL_Surface *g_screen;
static unsigned char g_palette[768];
static int g_truecolor;

static int doPlay(const char *filename);

static void usage(void)
{
	fprintf(stderr, "usage: mveplay filename\n");
	exit(1);
}

int main(int c, char **v)
{
	if (c != 2)
		usage();

	if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO) < 0)
	{
		fprintf(stderr, "Couldn't initialize SDL: %s\n",SDL_GetError());
		exit(1);
	}
	atexit(SDL_Quit);

	return doPlay(v[1]);
}

static unsigned int fileRead(void *handle, void *buf, unsigned int count)
{
	unsigned numread;

	numread = fread(buf, 1, count, (FILE *)handle);
	return (numread == count);
}

static void showFrame(unsigned char *buf, unsigned int bufw, unsigned int bufh,
					  unsigned int sx, unsigned int sy,
					  unsigned int w, unsigned int h,
					  unsigned int dstx, unsigned int dsty)
{
	int i;
	unsigned char *pal;
	SDL_Surface *sprite;
	SDL_Rect srcRect, destRect;

	assert(bufw == w && bufh == h);

	if (g_truecolor)
		sprite = SDL_CreateRGBSurfaceFrom(buf, bufw, bufh, 16, 2 * bufw, 0x7C00, 0x03E0, 0x001F, 0);
	else
	{
		sprite = SDL_CreateRGBSurfaceFrom(buf, bufw, bufh, 8, bufw, 0x7C00, 0x03E0, 0x001F, 0);

		pal = g_palette;
		for(i = 0; i < 256; i++)
		{
			sprite->format->palette->colors[i].r = (*pal++) << 2;
			sprite->format->palette->colors[i].g = (*pal++) << 2;
			sprite->format->palette->colors[i].b = (*pal++) << 2;
			sprite->format->palette->colors[i].unused = 0;
		}
	}

	srcRect.x = sx;
	srcRect.y = sy;
	srcRect.w = w;
	srcRect.h = h;
	destRect.x = dstx;
	destRect.y = dsty;
	destRect.w = w;
	destRect.h = h;

	SDL_BlitSurface(sprite, &srcRect, g_screen, &destRect);
	if ( (g_screen->flags & SDL_DOUBLEBUF) == SDL_DOUBLEBUF )
		SDL_Flip(g_screen);
	else
		SDL_UpdateRects(g_screen, 1, &destRect);
	SDL_FreeSurface(sprite);
}

static void setPalette(unsigned char *p, unsigned start, unsigned count)
{
	//Set color 0 to be black
	g_palette[0] = g_palette[1] = g_palette[2] = 0;

	//Set color 255 to be our subtitle color
	g_palette[765] = g_palette[766] = g_palette[767] = 50;

	//movie libs palette into our array
	memcpy(g_palette + start*3, p+start*3, count*3);
}

static int pollEvents()
{
	SDL_Event event;

	while (SDL_PollEvent(&event))
	{
		switch(event.type)
		{
		case SDL_QUIT:
			return 1;
		case SDL_KEYDOWN:
			switch (event.key.keysym.sym)
			{
			case SDLK_ESCAPE:
			case SDLK_q:
				return 1;
			case SDLK_f:
				SDL_WM_ToggleFullScreen(g_screen);
				break;
			default:
				break;
			}
			break;
		default:
			break;
		}
	}

	return 0;
}

static int doPlay(const char *filename)
{
	int result;
	int done = 0;
	FILE *mve;
	MVE_videoSpec vSpec;

	mve = fopen(filename, "rb");
	if (mve == NULL) {
		fprintf(stderr, "can't open MVE file\n");
		return 1;
	}

	memset(g_palette, 0, 768);

	MVE_sndInit(1);
	MVE_memCallbacks(malloc, free);
	MVE_ioCallbacks(fileRead);
	MVE_sfCallbacks(showFrame);
	MVE_palCallbacks(setPalette);

	MVE_rmPrepMovie(mve, -1, -1, 1);

	MVE_getVideoSpec(&vSpec);

	g_screen = SDL_SetVideoMode(vSpec.screenWidth, vSpec.screenHeight, vSpec.truecolor?16:8, SDL_ANYFORMAT);

	g_truecolor = vSpec.truecolor;

	while (!done && (result = MVE_rmStepMovie()) == 0)
	{
		done = pollEvents();
	}

	MVE_rmEndMovie();

	fclose(mve);

	return 0;
}
