/* $Id: pngfile.h,v 1.1 2004-05-22 23:41:29 btb Exp $ */
#ifndef PNGFILE_H
#define PNGFILE_H

typedef struct _png_data {
	unsigned int width;
	unsigned int height;
	unsigned int depth;
	unsigned int channels;
	unsigned paletted:1;
	unsigned color:1;
	unsigned alpha:1;

	unsigned char *data;
	unsigned char *palette;
	unsigned int num_palette;
} png_data;

extern int read_png(char *filename, png_data *pdata);

#endif
