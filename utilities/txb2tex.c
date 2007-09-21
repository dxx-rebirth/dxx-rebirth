/*
 * Modified by Bradley Bell, 2002, 2003
 * This program is licensed under the terms of the GPL, version 2 or later
 */

#include <stdio.h>
#include <string.h>

int
main(int argc, char *argv[])
{
	FILE *file, *outfile;
	char outfilename[64];
	char ch;
	int code;

	if (argc < 2) {
		printf("TXB2TEX V1.0 Copyright (c) Bryan Aamot, 1995\n"
			   "Modified by Bradley Bell, 2002, 2003\n"
		       "TXB to Text converter for Descent HOG files.\n"
		       "Converts a *.txb descent hog file to an ascii file.\n"
		       "Usage: TXB2TEX <txb file name> <text file name>\n"
		       "Example: TXB2TEX briefing.txb briefing.tex\n");
		return 1;
	}
	file = fopen(argv[1], "rb");
	if (!file) {
		printf("Can't open txb file (%s)\n", argv[1]);
		return 2;
	}

	if (argc > 2)
		strcpy(outfilename, argv[2]);
	else {
		strcpy(outfilename, argv[1]);
		strcpy(strrchr(outfilename, '.'), ".tex");
	}

	outfile = fopen(outfilename, "wb");
	if (!outfile) {
		printf("Can't open file (%s)\n", outfilename);
		fclose(file);
		return 2;
	}
	for (;;) {
		code = getc(file);
		if (feof(file)) break;
		if (code == 0x0a) {
			fprintf(outfile, "\x0d\x0a");
		} else {
			ch = (  ( (code&0x3f) << 2 ) + ( (code&0xc0) >> 6 )  ) ^ 0xa7;
			fprintf(outfile, "%c", ch);
		}
	}

	fclose(outfile);
	fclose(file);

	return 0;
}
