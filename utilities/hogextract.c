/*
 * Written 1999 Jan 29 by Josh Cogliati
 * I grant this program to public domain.
 *
 * Modified by Bradley Bell, 2002
 * All modifications under GPL, version 2 or later
 */

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

int
main(int argc, char *argv[])
{
	FILE *hogfile, *writefile;
	int len;
	char filename[13];
	char *buf;
	struct stat statbuf;
	int v = 0;

	if (argc > 1 && !strcmp(argv[1], "v")) {
		v = 1;
		argc--;
		argv++;
	}

	if (argc != 2) {
		printf("Usage: hogextract [v] hogfile\n"
		       "extracts all the files in hogfile into the current directory\n"
			   "Options:\n"
			   "  v    View files, don't extract\n");
		exit(0);
	}
	hogfile = fopen(argv[1], "r");
	stat(argv[1], &statbuf);
	printf("%i\n", (int)statbuf.st_size);
	buf = (char *)malloc(3);
	fread(buf, 3, 1, hogfile);
	printf("Extracting from: %s\n", argv[1]);
	free(buf);
	while(ftell(hogfile)<statbuf.st_size) {
		fread(filename, 13, 1, hogfile);
		fread(&len, sizeof(int), 1, hogfile);
		printf("Filename: %s \tLength: %i\n", filename, len);
		if (v)
			fseek(hogfile, len, SEEK_CUR);
		else {
			buf = (char *)malloc(len);
			if (buf == NULL) {
				printf("Unable to allocate memory\n");
			} else {
				fread(buf, len, 1, hogfile);
				writefile = fopen(filename, "w");
				fwrite(buf, len, 1, writefile);
				fclose(writefile);
				free(buf);
			}
		}
	}
	fclose(hogfile);

	return 0;
}
