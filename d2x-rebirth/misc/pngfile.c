#include <stdio.h>
#include <png.h>

#include "pngfile.h"
#include "u_mem.h"
#include "pstypes.h"

int read_png(char *filename, png_data *pdata)
{
	ubyte header[8];
	png_structp png_ptr = NULL;
	png_infop info_ptr = NULL;
	png_bytepp row_pointers = NULL;
	png_uint_32 width, height;
	int depth, color_type;
	int i;
	FILE *fp = NULL;

	if (!filename || !pdata)
		return 0;

	if ((fp = fopen(filename, "rb")) == NULL)
		return 0;

	fread(header, 1, 8, fp);
	if (!png_check_sig(header, 8))
		return 0;

	png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
	info_ptr = png_create_info_struct(png_ptr);

	pdata->data = pdata->palette = NULL;
	pdata->num_palette = 0;
	if (setjmp(png_ptr->jmpbuf))
	{
		png_destroy_read_struct(&png_ptr, &info_ptr, png_infopp_NULL);
		if (pdata->data)
			free(pdata->data);
		if (pdata->palette)
			free(pdata->palette);
		if (row_pointers)
			free(row_pointers);
		fclose(fp);
		return 0;
	}

	png_init_io(png_ptr, fp);
	png_set_sig_bytes(png_ptr, 8);
	png_read_info(png_ptr, info_ptr);
	png_get_IHDR(png_ptr, info_ptr, &width, &height, &depth, &color_type, NULL, NULL, NULL);

	pdata->width = width;
	pdata->height = height;
	pdata->depth = depth;

	pdata->data = (ubyte*)malloc(png_get_rowbytes(png_ptr, info_ptr) * height);
	row_pointers = (png_bytep *)malloc(sizeof(png_bytep) * height);
	for (i = 0; i < height; i++)
		row_pointers[i] = &pdata->data[png_get_rowbytes(png_ptr, info_ptr) * i];

	png_read_image(png_ptr, row_pointers);
	free(row_pointers);
	row_pointers=NULL;
	png_read_end(png_ptr, info_ptr);

	if (color_type == PNG_COLOR_TYPE_PALETTE)
	{
		png_colorp palette;

		if (png_get_PLTE(png_ptr, info_ptr, &palette, &pdata->num_palette))
		{
			pdata->palette = (ubyte*)malloc(pdata->num_palette * 3);
			memcpy(pdata->palette, palette, pdata->num_palette * 3);
		}
	}

	pdata->paletted = (color_type & PNG_COLOR_MASK_PALETTE) > 0;
	pdata->color = (color_type & PNG_COLOR_MASK_COLOR) > 0;
	pdata->alpha = (color_type & PNG_COLOR_MASK_ALPHA) > 0;
	if (pdata->color && pdata->alpha)
		pdata->channels = 4;
	else if (pdata->color && !pdata->alpha)
		pdata->channels = 3;
	else if (!pdata->color && pdata->alpha)
		pdata->channels = 2;
	else //if (!pdata->color && !pdata->alpha)
		pdata->channels = 1;

	png_destroy_read_struct(&png_ptr, &info_ptr, png_infopp_NULL);
	fclose(fp);

	return 1;
}
