/*
 * Written 1999 Jan 29 by Josh Cogliati
 * I grant this program to public domain.
 *
 * Modified for mvl by Bradley Bell, 2002
 * All modifications under GPL, version 2 or later
 */

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#define MAX_FILES 256

int
main(int argc, char *argv[])
{
	FILE *mvlfile, *writefile;
	int i, nfiles, len[MAX_FILES];
	char filename[MAX_FILES][13];
	char *buf;
	struct stat statbuf;
	int v = 0;

	if (argc > 1 && !strcmp(argv[1], "v")) {
		v = 1;
		argc--;
		argv++;
	}

	if (argc != 2) {
		printf("Usage: mvlextract [v] mvlfile\n"
		       "extracts all the files in mvlfile into the current directory\n"
			   "Options:\n"
			   "  v    View files, don't extract\n");
		exit(0);
	}
	mvlfile = fopen(argv[1], "r");
	stat(argv[1], &statbuf);
	printf("%i\n", (int)statbuf.st_size);
	buf = (char *)malloc(4);
	fread(buf, 4, 1, mvlfile);
	fread(&nfiles, 4, 1, mvlfile);
	printf("Extracting from: %s\n", argv[1]);
	free(buf);
	for (i = 0; i < nfiles; i++) {
		fread(filename[i], 13, 1, mvlfile);
		fread(&len[i], 4, 1, mvlfile);
		printf("Filename: %s \tLength: %i\n", filename[i], len[i]);
	}

	if (!v) {
		for (i = 0; i < nfiles; i++) {
			if (ftell(mvlfile) > statbuf.st_size) {
				printf("Error, end of file\n");
				exit(1);
			}
			buf = (char *)malloc(len[i]);
			if (buf == NULL) {
				printf("Unable to allocate memory\n");
			} else {
				fread(buf, len[i], 1, mvlfile);
				writefile = fopen(filename[i], "w");
				fwrite(buf, len[i], 1, writefile);
				fclose(writefile);
				free(buf);
			}
		}
	}
	fclose(mvlfile);

	return 0;
}
