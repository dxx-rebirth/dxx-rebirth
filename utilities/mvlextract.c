/*
 * Written 1999 Jan 29 by Josh Cogliati
 * Modified for mvl by Bradley Bell, 2002, 2003
 * This program is licensed under the terms of the GPL, version 2 or later
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#define SWAPINT(x)   (((x)<<24) | (((unsigned int)(x)) >> 24) | (((x) &0x0000ff00) << 8) | (((x) & 0x00ff0000) >> 8))

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
	int bigendian = 0;

	if (argc > 1 && !strcmp(argv[1], "v")) {
		v = 1;
		argc--;
		argv++;
	}

	if (argc < 2) {
		printf("Usage: mvlextract [v] mvlfile [filename]\n"
		       "extracts all the files in mvlfile into the current directory\n"
			   "Options:\n"
			   "  v    View files, don't extract\n");
		exit(0);
	}
	mvlfile = fopen(argv[1], "rb");
	stat(argv[1], &statbuf);
	printf("%i\n", (int)statbuf.st_size);
	buf = (char *)malloc(4);
	fread(buf, 4, 1, mvlfile);
	fread(&nfiles, 4, 1, mvlfile);
	printf("%d files\n", nfiles);
	if (nfiles > MAX_FILES) { // must be a bigendian mvl
		fprintf(stderr, "warning: nfiles>%d, trying reverse byte order...",
				MAX_FILES);
		bigendian = 1;
	}
	if (bigendian)
		nfiles = SWAPINT(nfiles);
	printf("Extracting from: %s\n", argv[1]);
	free(buf);
	for (i = 0; i < nfiles; i++) {
		fread(filename[i], 13, 1, mvlfile);
		fread(&len[i], 4, 1, mvlfile);
		if (bigendian)
			len[i] = SWAPINT(len[i]);
		if (argc == 2 || !strcmp(argv[2], filename[i]))
			printf("Filename: %s \tLength: %i\n", filename[i], len[i]);
	}

	if (!v) {
		for (i = 0; i < nfiles; i++) {
			if (argc > 2 && strcmp(argv[2], filename[i]))
				fseek(mvlfile, len[i], SEEK_CUR);
			else {
				if (ftell(mvlfile) > statbuf.st_size) {
					printf("Error, end of file\n");
					exit(1);
				}
				buf = (char *)malloc(len[i]);
				if (buf == NULL) {
					printf("Unable to allocate memory\n");
				} else {
					fread(buf, len[i], 1, mvlfile);
					writefile = fopen(filename[i], "wb");
					fwrite(buf, len[i], 1, writefile);
					fclose(writefile);
					free(buf);
				}
			}
		}
	}
	fclose(mvlfile);

	return 0;
}
