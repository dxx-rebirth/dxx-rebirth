#include <SDL/SDL.h>

#include "mvelib.h"

extern int g_spdFactorNum;

void initializeMovie(MVESTREAM *mve);
void playMovie(MVESTREAM *mve);
void shutdownMovie(MVESTREAM *mve);

static void usage(void)
{
    fprintf(stderr, "usage: mveplay filename\n");
    exit(1);
}

static int doPlay(const char *filename)
{
    MVESTREAM *mve = mve_open(filename);
    if (mve == NULL)
    {
        fprintf(stderr, "can't open MVE file '%s'\n", filename);
        return 1;
    }

    initializeMovie(mve);
    playMovie(mve);
    shutdownMovie(mve);

    mve_close(mve);

    return 0;
}

int main(int c, char **v)
{
    if (c != 2  &&  c != 3)
        usage();

    if (c == 3)
        g_spdFactorNum = atoi(v[2]);

    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO) < 0)
    {
        fprintf(stderr, "Couldn't initialize SDL: %s\n",SDL_GetError());
        exit(1);
    }
    atexit(SDL_Quit);

    return doPlay(v[1]);
}
