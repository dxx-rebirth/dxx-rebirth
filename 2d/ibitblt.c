/*
THE COMPUTER CODE CONTAINED HEREIN IS THE SOLE PROPERTY OF PARALLAX
SOFTWARE CORPORATION ("PARALLAX").  PARALLAX, IN DISTRIBUTING THE CODE TO
END-USERS, AND SUBJECT TO ALL OF THE TERMS AND CONDITIONS HEREIN, GRANTS A
ROYALTY-FREE, PERPETUAL LICENSE TO SUCH END-USERS FOR USE BY SUCH END-USERS
IN USING, DISPLAYING,  AND CREATING DERIVATIVE WORKS THEREOF, SO LONG AS
SUCH USE, DISPLAY OR CREATION IS FOR NON-COMMERCIAL, ROYALTY OR REVENUE
FREE PURPOSES.  IN NO EVENT SHALL THE END-USER USE THE COMPUTER CODE
CONTAINED HEREIN FOR REVENUE-BEARING PURPOSES.  THE END-USER UNDERSTANDS
AND AGREES TO THE TERMS HEREIN AND ACCEPTS THE SAME BY USE OF THIS FILE.  
COPYRIGHT 1993-1998 PARALLAX SOFTWARE CORPORATION.  ALL RIGHTS RESERVED.
*/
/*
 *  Routines to to inverse bitblitting -- well not really.
 *  We don't inverse bitblt like in the PC, but this code
 *  does set up a structure that blits around the cockpit
 *
*/

#ifdef HAVE_CONFIG_H
#include <conf.h>
#endif

#include <string.h>
#include "pstypes.h"
#include "gr.h"
#include "grdef.h"
#include "ibitblt.h"
#include "error.h"
//added 05/17/99 Matt Mueller
#include "u_mem.h"
//end addition -MM

#define FIND_START		1
#define FIND_STOP		2

#define MAX_WIDTH			1280	
#define MAX_SCANLINES		1024	
#define MAX_HOLES				10

static short start_points[MAX_SCANLINES][MAX_HOLES];
static short hole_length[MAX_SCANLINES][MAX_HOLES];
static double *scanline = NULL;

// adb: gr_linear_movsd assumes c >= 4
#define gr_linear_movsd(s,d,c) memcpy(d,s,c)

void gr_ibitblt(grs_bitmap *src_bmp, grs_bitmap *dest_bmp, ubyte pixel_double)
{
	int x, y, sw, sh, srowsize, drowsize, dstart, sy, dy;
	ubyte *src, *dest;
	short *current_hole, *current_hole_length;

// variable setup

	sw = src_bmp->bm_w;
	sh = src_bmp->bm_h;
	srowsize = src_bmp->bm_rowsize;
	drowsize = dest_bmp->bm_rowsize;
	src = src_bmp->bm_data;
	dest = dest_bmp->bm_data;

	sy = 0;
	while (start_points[sy][0] == -1) {
		sy++;
		dest += drowsize;
	}
	
 	if (pixel_double) {
		ubyte *scan = (ubyte *)scanline;		// set up for byte processing of scanline
		
		dy = sy;
		for (y = sy; y < sy + sh; y++) {
			gr_linear_rep_movsd_2x(src, scan, sw);
			current_hole = start_points[dy];
			current_hole_length = hole_length[dy];
			for (x = 0; x < MAX_HOLES; x++) {
				if (*current_hole == -1)
					break;
				dstart = *current_hole;
				gr_linear_movsd(&(scan[dstart]), &(dest[dstart]), *current_hole_length);
				current_hole++;
				current_hole_length++;
			}
			dy++;
			dest += drowsize;
			current_hole = start_points[dy];
			current_hole_length = hole_length[dy];
			for (x = 0;x < MAX_HOLES; x++) {
				if (*current_hole == -1)
					break;
				dstart = *current_hole;
				gr_linear_movsd(&(scan[dstart]), &(dest[dstart]), *current_hole_length);
				current_hole++;
				current_hole_length++;
			}
			dy++;
			dest += drowsize;
			src += srowsize;
		}
	} else {
		Assert(sw <= MAX_WIDTH);
		Assert(sh <= MAX_SCANLINES);
		for (y = sy; y < sy + sh; y++) {
			for (x = 0; x < MAX_HOLES; x++) {
				if (start_points[y][x] == -1)
					break;
				dstart = start_points[y][x];
				gr_linear_movsd(&(src[dstart]), &(dest[dstart]), hole_length[y][x]);
			}
			dest += drowsize;
			src += srowsize;
		}
	}
}

void gr_ibitblt_create_mask(grs_bitmap *mask_bmp, int sx, int sy, int sw, int sh, int srowsize)
{
	int x, y;
	ubyte mode;
	int count = 0;
	
	Assert( (!(mask_bmp->bm_flags&BM_FLAG_RLE)) );

	for (y = 0; y < MAX_SCANLINES; y++) {
		for (x = 0; x < MAX_HOLES; x++) {
			start_points[y][x] = -1;
			hole_length[y][x] = -1;
		}
	}
	
	for (y = sy; y < sy+sh; y++) {
		count = 0;
		mode = FIND_START;
		for (x = sx; x < sx + sw; x++) {
			if ((mode == FIND_START) && (mask_bmp->bm_data[mask_bmp->bm_rowsize*y+x] == TRANSPARENCY_COLOR)) {
				start_points[y][count] = x;
				mode = FIND_STOP;
			} else if ((mode == FIND_STOP) && (mask_bmp->bm_data[mask_bmp->bm_rowsize*y+x] != TRANSPARENCY_COLOR)) {
				hole_length[y][count] = x - start_points[y][count];
				count++;
				mode = FIND_START;
			}
		}
		if (mode == FIND_STOP) {
			hole_length[y][count] = x - start_points[y][count];
			count++;
		}
		Assert(count <= MAX_HOLES);
	}
}

//added 7/11/99 by adb to prevent memleaks
static void free_scanline(void)
{
	if (scanline) d_free(scanline);
}
//end additions - adb


void gr_ibitblt_find_hole_size(grs_bitmap *mask_bmp, int *minx, int *miny, int *maxx, int *maxy)
{
	ubyte c;
	int x, y, count = 0;
	
	Assert( (!(mask_bmp->bm_flags&BM_FLAG_RLE)) );
	Assert( mask_bmp->bm_flags&BM_FLAG_TRANSPARENT );
	
	*minx = mask_bmp->bm_w - 1;
	*maxx = 0;
	*miny = mask_bmp->bm_h - 1;
	*maxy = 0;

	//changed 7/11/99 by adb to prevent memleaks
	if (scanline == NULL) {
		scanline = (double *)d_malloc(sizeof(double) * (MAX_WIDTH / sizeof(double)));
		atexit(free_scanline);
	}
	//end changes - adb
		
	for (y = 0; y < mask_bmp->bm_h; y++) {
		for (x = 0; x < mask_bmp->bm_w; x++) {
			c = mask_bmp->bm_data[mask_bmp->bm_rowsize*y+x];
			if (c == TRANSPARENCY_COLOR) {				// don't look for transparancy color here.
				count++;
				if (x < *minx) *minx = x;
				if (y < *miny) *miny = y;
				if (x > *maxx) *maxx = x;
				if (y > *maxy) *maxy = y;
			}
		}
	}
        Assert (count);
}
