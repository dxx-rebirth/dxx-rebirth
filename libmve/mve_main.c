#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

#include <SDL.h>

#include "mvelib.h"

#define MAX_FILES 256

extern int g_spdFactorNum;
extern int g_sdlVidFlags;
extern int g_loop;

void initializeMovie(MVESTREAM *mve);
void playMovie(MVESTREAM *mve);
void shutdownMovie(MVESTREAM *mve);

static void usage(void)
{
    fprintf(stderr, "usage: mveplay [-f] [-l] [-s <n>] [<mvlfile>] <filename>\n"
			"-f\tFullscreen mode\n"
			"-s\tSpeed Factor <n>\n"
			);
    exit(1);
}

static int doPlay(int filehandle)
{
    MVESTREAM *mve = mve_open_filehandle(filehandle);
    if (mve == NULL) {
        fprintf(stderr, "can't open MVE file\n");
		exit(1);
	}

    initializeMovie(mve);
    playMovie(mve);
    shutdownMovie(mve);

    mve_close_filehandle(mve);

    return 1;
}

int main(int argc, char *argv[])
{
	int i, filehandle;
	char *mvlfile = NULL, *mvefile = NULL;

	for (i = 1; i < argc; i++) {
		if (!strcmp(argv[i], "-h"))
			usage();

		if (!strcmp(argv[i], "-f"))
			g_sdlVidFlags |= SDL_FULLSCREEN;

		if (!strcmp(argv[i], "-l"))
			g_loop = 1;

		if (!strcmp(argv[i], "-s")) {
			if (argc < i + 2)
				usage();
			g_spdFactorNum = atoi(argv[i + 1]);
			i++;
		}

		if (strchr(argv[i], '.') && !strcasecmp(strchr(argv[i], '.'), ".mvl"))
			mvlfile = argv[i];

		if (strchr(argv[i], '.') && !strcasecmp(strchr(argv[i], '.'), ".mve"))
			mvefile = argv[i];
	}

	if (mvlfile) {
		int nfiles;
		char filename[MAX_FILES][13];
		int filesize[MAX_FILES];
		char sig[4];

#ifdef O_BINARY
		filehandle = open(mvlfile, O_RDONLY | O_BINARY);
#else
		filehandle = open(mvlfile, O_RDONLY);
#endif
		if (filehandle == -1) {
			fprintf(stderr, "Error opening %s\n", mvlfile);
			exit(1);
		}
		if ((read(filehandle, sig, 4) < 4) ||
			(strncmp(sig, "DMVL", 4)) ||
			(read(filehandle, &nfiles, 4) < 4)) {
			fprintf(stderr, "Error reading %s\n", mvlfile);
			exit(1);
		}
		for (i = 0; i < nfiles; i++) {
			if ((read(filehandle, filename[i], 13) < 13) ||
				(read(filehandle, &filesize[i], 4) < 4) ||
				(strlen(filename[i]) > 12))	{
				fprintf(stderr, "Error reading %s\n", mvlfile);
				exit(1);
			}
		}

		for (i = 0; i < nfiles; i++) {
			if (mvefile) {
				if (!strcasecmp(filename[i], mvefile))
					break;
				else
					lseek(filehandle, filesize[i], SEEK_CUR);
			} else
				printf("%13s\t%d\n", filename[i], filesize[i]);
		}
		if (!mvefile)
			exit(0);

	} else if (mvefile) {
#ifdef O_BINARY
		filehandle = open(mvefile, O_RDONLY | O_BINARY);
#else
		filehandle = open(mvefile, O_RDONLY);
#endif
	} else
		usage();

    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO) < 0)
    {
        fprintf(stderr, "Couldn't initialize SDL: %s\n",SDL_GetError());
        exit(1);
    }
    atexit(SDL_Quit);

	doPlay(filehandle);

	return 0;
}
