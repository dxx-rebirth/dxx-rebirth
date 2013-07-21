
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <string.h>

#ifdef _WIN32
# include <windows.h>
#endif

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

static void showFrame(unsigned char *buf, int dstx, int dsty, int bufw, int bufh, int sw, int sh)
{
	int i;
	unsigned char *pal;
	SDL_Surface *sprite;
	SDL_Rect srcRect, destRect;

	if (dstx == -1) // center it
		dstx = (sw - bufw) / 2;
	if (dsty == -1) // center it
		dsty = (sh - bufh) / 2;

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

	srcRect.x = 0;
	srcRect.y = 0;
	srcRect.w = bufw;
	srcRect.h = bufh;
	destRect.x = dstx;
	destRect.y = dsty;
	destRect.w = bufw;
	destRect.h = bufh;

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
		case SDL_MOUSEBUTTONDOWN:
		case SDL_MOUSEBUTTONUP:
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
	int bpp = 0;
	FILE *mve;
	MVE_videoSpec vSpec;

	mve = fopen(filename, "rb");
	if (mve == NULL) {
		fprintf(stderr, "can't open MVE file\n");
		return 1;
	}

	memset(g_palette, 0, 768);

	MVE_sndInit(1);
	MVE_memCallbacks((mve_cb_Alloc)malloc, free);
	MVE_ioCallbacks(fileRead);
	MVE_sfCallbacks(showFrame);
	MVE_palCallbacks(setPalette);

	MVE_rmPrepMovie(mve, -1, -1, 1);

	MVE_getVideoSpec(&vSpec);

	bpp = vSpec.truecolor?16:8;

	g_screen = SDL_SetVideoMode(vSpec.screenWidth, vSpec.screenHeight, bpp, SDL_ANYFORMAT);

	g_truecolor = vSpec.truecolor;

	while (!done && (result = MVE_rmStepMovie()) == 0)
	{
		done = pollEvents();
	}

	MVE_rmEndMovie();

	fclose(mve);

	return 0;
}
