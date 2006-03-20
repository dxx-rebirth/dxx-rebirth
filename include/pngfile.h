/* $Id: pngfile.h,v 1.1.1.1 2006/03/17 20:01:26 zicodxx Exp $ */
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
