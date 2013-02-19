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
		printf("TEX2TXB V1.0 Copyright (c) Bryan Aamot, 1995\n"
			   "Modified by Bradley Bell, 2002, 2003\n"
		       "Text to TXB converter for Descent HOG files.\n"
		       "Converts an ascii text file to *.txb descent hog file.\n"
		       "Usage: TEX2TXB <text file name> <txb file name>\n"
		       "Example: TEX2TXB briefing.tex briefing.txb\n");
		return 1;
	}
	file = fopen(argv[1], "rb");
	if (!file) {
		printf("Can't open file (%s)\n", argv[1]);
		return 2;
	}

	if (argc > 2)
		strcpy(outfilename, argv[2]);
	else {
		strcpy(outfilename, argv[1]);
		strcpy(strrchr(outfilename, '.'), ".txb");
	}

	outfile = fopen(outfilename, "wb");
	if (!outfile) {
		printf("Can't open file (%s)\n", outfilename);
		fclose(file);
		return 2;
	}

	for (;;) {
		ch = getc(file);
		if (feof(file)) break;
		if (ch!=0x0d) {
			if (ch==0x0a) {
				fprintf(outfile, "\x0a");
			} else {
				code = ( ( (ch &0xfC) >> 2) + ( (ch &0x03) << 6 ) ) ^ 0xe9;
				fprintf(outfile, "%c", code);
			}
		}
	}

	fclose(outfile);
	fclose(file);

	return 0;
}
