#include <stdio.h>

void
main(int argc, char *argv[])
{
	FILE *file, *outfile;
	char equals, ch;
	int code;

	if (argc != 3) {
		printf("TXB2TXT V1.0 Copyright (c) Bryan Aamot, 1995\n"
		       "TXB to Text converter for Descent HOG files.\n"
		       "Converts a *.txb descent hog file to an ascii file.\n"
		       "Usage: TXB2TXT <txb file name> <text file name>\n"
		       "Example: TXT2TXB briefing.txb briefing.txt\n");
		exit(1);
	}
	file = fopen(argv[1], "rb");
	if (!file) {
		printf("Can't open txb file (%s)\n", argv[1]);
		exit(2);
	}
	outfile = fopen(argv[2], "wb");
	if (!outfile) {
		printf("Can't open file (%s)\n", argv[2]);
		fclose(file);
		exit(2);
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
}
