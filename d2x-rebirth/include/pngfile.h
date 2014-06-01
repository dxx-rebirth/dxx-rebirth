/*
 * This file is part of the DXX-Rebirth project <http://www.dxx-rebirth.com/>.
 * It is copyright by its individual contributors, as recorded in the
 * project's Git history.  See COPYING.txt at the top level for license
 * terms and a link to the Git history.
 */
#ifndef PNGFILE_H
#define PNGFILE_H

struct png_data
{
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
};

extern int read_png(char *filename, png_data *pdata);

#endif
