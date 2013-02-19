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

#define SWAPINT(x)   (((x)<<24) | (((uint)(x)) >> 24) | (((x) &0x0000ff00) << 8) | (((x) & 0x00ff0000) >> 8))

int
main(int argc, char *argv[])
{
	FILE *hogfile, *readfile;
	DIR *dp;
	struct dirent *ep;
	char filename[13];
	char *buf;
	struct stat statbuf;
	int tmp;

	if (argc != 2) {
		printf("Usage: hogcreate hogfile\n"
		       "creates hogfile using all the files in the current directory\n");
		exit(0);
	}
	hogfile = fopen(argv[1], "wb");
	buf = (char *)malloc(3);
	strncpy(buf, "DHF", 3);
	fwrite(buf, 3, 1, hogfile);
	printf("Creating: %s\n", argv[1]);
	free(buf);
	dp = opendir("./");
	if (dp != NULL) {
		while ((ep = readdir(dp))) {
			if (strlen(ep->d_name) > 12) {
				fprintf(stderr, "error: filename %s too long! (12 chars max!)\n", ep->d_name);
				return 1;
			}
			memset(filename, 0, 13);
			strcpy(filename, ep->d_name);
			stat(filename, &statbuf);
			if(! S_ISDIR(statbuf.st_mode)) {
				printf("Filename: %s \tLength: %i\n", filename, (int)statbuf.st_size);
				readfile = fopen(filename, "rb");
				buf = (char *)malloc(statbuf.st_size);
				if (buf == NULL) {
					printf("Unable to allocate memery\n");
				} else {
					fwrite(filename, 13, 1, hogfile);
					tmp = (int)statbuf.st_size;
#ifdef WORDS_BIGENDIAN
					tmp = SWAPINT(tmp);
#endif
					fwrite(&tmp, 4, 1, hogfile);
					fread(buf, statbuf.st_size, 1, readfile);
					fwrite(buf, statbuf.st_size, 1, hogfile);
				}
				fclose(readfile);

			}
		}
		closedir(dp);
	}
	fclose(hogfile);

	return 0;
}
