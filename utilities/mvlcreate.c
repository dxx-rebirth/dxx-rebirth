/*
 * Written 1999 Jan 29 by Josh Cogliati
 * Modified by Bradley Bell, 2002, 2003
 * This program is licensed under the terms of the GPL, version 2 or later
 */

#ifdef HAVE_CONFIG_H
#include <conf.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <dirent.h>

#define MAX_FILES 256

#define SWAPINT(x)   (((x)<<24) | (((uint)(x)) >> 24) | (((x) &0x0000ff00) << 8) | (((x) & 0x00ff0000) >> 8))

int
main(int argc, char *argv[])
{
	FILE *mvlfile, *readfile;
	DIR *dp;
	struct dirent *ep;
	int i, nfiles = 0, len[MAX_FILES], tmp;
	char filename[MAX_FILES][13];
	char *buf;
	struct stat statbuf;

	if (argc != 2) {
		printf("Usage: mvlcreate mvlfile\n"
		       "creates mvlfile using all the files in the current directory\n");
		exit(0);
	}

	dp = opendir("./");
	if (dp != NULL) {
		while ((ep = readdir(dp))) {
			if (strlen(ep->d_name) > 12) {
				fprintf(stderr, "error: filename %s too long! (12 chars max!)\n", ep->d_name);
				return 1;
			}
			memset(filename[nfiles], 0, 13);
			strcpy(filename[nfiles], ep->d_name);
			stat(filename[nfiles], &statbuf);
			if(! S_ISDIR(statbuf.st_mode)) {
				len[nfiles] = (int)statbuf.st_size;
				printf("Filename: %s \tLength: %i\n", filename[nfiles], len[nfiles]);
				nfiles++;
			}
		}
	}
	closedir(dp);

	printf("Creating: %s\n", argv[1]);
	mvlfile = fopen(argv[1], "wb");
	buf = (char *)malloc(4);
	strncpy(buf, "DMVL", 4);
	fwrite(buf, 4, 1, mvlfile);
	free(buf);

	tmp = nfiles;
#ifdef WORDS_BIGENDIAN
	tmp = SWAPINT(tmp);
#endif
	fwrite(&tmp, 4, 1, mvlfile);

	for (i = 0; i < nfiles; i++) {
		fwrite(filename[i], 13, 1, mvlfile);
		tmp = len[i];
#ifdef WORDS_BIGENDIAN
		tmp = SWAPINT(tmp);
#endif
		fwrite(&tmp, 4, 1, mvlfile);
	}

	for (i = 0; i < nfiles; i++) {
		readfile = fopen(filename[i], "rb");
		buf = (char *)malloc(len[i]);
		if (buf == NULL) {
			printf("Unable to allocate memory\n");
		} else {
			fread(buf, len[i], 1, readfile);
			fwrite(buf, len[i], 1, mvlfile);
		}
		fclose(readfile);
	}

	fclose(mvlfile);

	return 0;
}
