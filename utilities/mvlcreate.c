/*
 * Written 1999 Jan 29 by Josh Cogliati
 * I grant this program to public domain.
 *
 * Modified for mvl by Bradley Bell, 2002
 * All modifications under GPL, version 2 or later
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <dirent.h>

#define MAX_FILES 256

int
main(int argc, char *argv[])
{
	FILE *mvlfile, *readfile;
	DIR *dp;
	struct dirent *ep;
	int i, nfiles = 0, len[MAX_FILES];
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
			strcpy(filename[i], ep->d_name);
			stat(filename[i], &statbuf);
			if(! S_ISDIR(statbuf.st_mode)) {
				nfiles++;
				len[i] = (int)statbuf.st_size;
				printf("Filename: %s \tLength: %i\n", filename[i], len[i]);
			}
		}
	}
	closedir(dp);

	printf("Creating: %s\n", argv[1]);
	mvlfile = fopen(argv[1], "w");
	buf = (char *)malloc(4);
	strncpy(buf, "DMVL", 4);
	fwrite(buf, 4, 1, mvlfile);
	free(buf);

	fwrite(&nfiles, 4, 1, mvlfile);

	for (i = 0; i < nfiles; i++) {
		fwrite(filename[i], 13, 1, mvlfile);
		fwrite(&len[i], 4, 1, mvlfile);
	}

	for (i = 0; i < nfiles; i++) {
		readfile = fopen(filename[i], "r");
		buf = (char *)malloc(len[i]);
		if (buf == NULL) {
			printf("Unable to allocate memory\n");
		} else {
			fread(buf, statbuf.st_size, 1, readfile);
			fwrite(buf, statbuf.st_size, 1, mvlfile);
		}
		fclose(readfile);
	}

	fclose(mvlfile);

	return 0;
}
