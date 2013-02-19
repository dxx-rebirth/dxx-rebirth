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
 * $Source: /cvsroot/dxx-rebirth/d1x-rebirth/2d/poly.c,v $
 * $Revision: 1.1.1.1 $
 * $Author: zicodxx $
 * $Date: 2006/03/17 19:39:00 $
 *
 * Graphical routines for drawing polygons.
 *
 * $Log: poly.c,v $
 * Revision 1.1.1.1  2006/03/17 19:39:00  zicodxx
 * initial import
 *
 * Revision 1.1.1.1  1999/06/14 21:57:32  donut
 * Import of d1x 1.37 source.
 *
 * Revision 1.5  1994/11/13  13:03:43  john
 * Added paged out bit in bitmap structure.  Commented out the
 * poly code that is never used.
 * 
 * Revision 1.4  1994/03/14  16:56:13  john
 * Changed grs_bitmap structure to include bm_flags.
 * 
 * Revision 1.3  1993/10/15  16:23:14  john
 * y
 * 
 * Revision 1.2  1993/10/08  14:30:39  john
 * *** empty log message ***
 * 
 * Revision 1.1  1993/09/08  11:44:13  john
 * Initial revision
 * 
 *
 */

#include "u_mem.h"
#include "gr.h"
#include "grdef.h"

//#define USE_POLY_CODE 1

#define MAX_SCAN_LINES 1200

#ifdef USE_POLY_CODE

int y_edge_list[MAX_SCAN_LINES];

void gr_upoly(int nverts, int *vert )
{
	int temp;
	int startx, stopx;  // X coordinates of both ends of current edge.
	int firstx, firsty; // Saved copy of the first vertex to connect later.
	int dx_dy;          // Slope of current edge.
	int miny, maxy;

	int starty, stopy;  // Y coordinates of both ends of current edge.

	int x1, x2, i;

	// Find the min and max rows to clear out the minimun y_edge_list.
	// (Is it worth it?)
	maxy = vert[1];
	miny = vert[1];

	for (i=3; i<(nverts*2); i+=2 )
	{
		if (vert[i]>maxy) maxy=vert[i];
		if (vert[i]<miny) miny=vert[i];
	}

	miny >>= 16;
	miny--;             // -1 to be safe
	maxy >>= 16;
	maxy++;             // +1 to be safe

	// Clear only the part of the y_edge_list that w will be using
	if (miny < 0) miny = 0;
	if (maxy > MAX_SCAN_LINES) maxy = MAX_SCAN_LINES;

	for (i=miny; i<maxy; i++ )
		y_edge_list[i] = -1;

	// Save the first vertex so that we can connect to it at the end.
	firstx = vert[0];
	firsty = vert[1] >> 16;

	do
	{
		nverts--;

		// Get the beginning coordinates of the current edge.
		startx = vert[0];
		starty = vert[1] >> 16;

		// Get the ending coordinates of the current edge.
		if (nverts > 0 ) {
			stopx = vert[2];
			stopy = vert[3] >> 16;
			vert += 2;
		} else  {
			stopx = firstx;     // Last edge, uses first vertex as endpoint
			stopy = firsty;
		}

		if (stopy < starty )    {
			temp = stopy;
			stopy = starty;
			starty = temp;
			temp = stopx;
			stopx = startx;
			startx = temp;
		}

		if (stopy == starty )
		{
			// Draw a edge going horizontally across screen
			x1 = startx>>16;
			x2 = stopx>>16;

			if (x2 > x1 )
				//gr_uscanline( x1, x2-1, stopy );
				gr_uscanline( x1, x2, stopy );
			else
				//gr_uscanline( x2, x1-1, stopy );
				gr_uscanline( x2, x1, stopy );

		} else {

			dx_dy = (stopx - startx) / (stopy - starty);

			for (; starty < stopy; starty++ )
			{
				if (y_edge_list[starty]==-1)
					y_edge_list[starty] = startx;
				else    {
					x1 = y_edge_list[starty]>>16;
					x2 = startx>>16;

					if (x2 > x1 )
						//gr_uscanline( x1, x2-1, starty );
						gr_uscanline( x1, x2, starty );
					else
						//gr_uscanline( x2, x1-1, starty );
						gr_uscanline( x2, x1, starty );
				}
				startx += dx_dy;
			}
		}


	} while (nverts > 0);
}


void gr_poly(int nverts, int *vert )
{
	int temp;
	int startx, stopx;  // X coordinates of both ends of current edge.
	int firstx, firsty; // Saved copy of the first vertex to connect later.
	int dx_dy;          // Slope of current edge.
	int miny, maxy;

	int starty, stopy;  // Y coordinates of both ends of current edge.

	int x1, x2, i, j;

	// Find the min and max rows to clear out the minimun y_edge_list.
	// (Is it worth it?)
	maxy = vert[1];
	miny = vert[1];

	j = 0;

	for (i=3; i<(nverts*2); i+=2 )
	{
		if (vert[i]>maxy) {
			if ((maxy=vert[i]) > MAXY) j++;
			//if (j>1) break;
		}

		if (vert[i]<miny) {
			if ((miny=vert[i]) < MINY) j++;
			//if (j>1) break;
		}
	}

	miny >>= 16;
	miny--;         // -1 to be safe
	maxy >>= 16;
	maxy++;          // +1 to be safe

	if (miny < MINY) miny = MINY;
	if (maxy > MAXY) maxy = MAXY+1;

	// Clear only the part of the y_edge_list that w will be using
	for (i=miny; i<maxy; i++ )
	   y_edge_list[i] = -1;

	// Save the first vertex so that we can connect to it at the end.
	firstx = vert[0];
	firsty = vert[1] >> 16;

	do
	{
		nverts--;

		// Get the beginning coordinates of the current edge.
		startx = vert[0];
		starty = vert[1] >> 16;

		// Get the ending coordinates of the current edge.
		if (nverts > 0 ) {
			stopx = vert[2];
			stopy = vert[3] >> 16;
			vert += 2;
		} else  {
			stopx = firstx;     // Last edge, uses first vertex as endpoint
			stopy = firsty;
		}


		if (stopy < starty ) {
			temp = stopy;
			stopy = starty;
			starty = temp;
			temp = stopx;
			stopx = startx;
			startx = temp;
		}

		if (stopy == starty )
		{
			// Draw a edge going horizontally across screen
			if ((stopy >= MINY) && (stopy <=MAXY )) {
				x1 = startx>>16;
				x2 = stopx>>16;

				if (x1 > x2 ) {
					temp = x2;
					x2 = x1;
					x1 = temp;
				}

				if ((x1 <= MAXX ) && (x2 >= MINX))
				{
					if (x1 < MINX ) x1 = MINX;
					if (x2 > MAXX ) x2 = MAXX+1;
					//gr_uscanline( x1, x2-1, stopy );
					gr_scanline( x1, x2, stopy );
				}
			}
		} else {

			dx_dy = (stopx - startx) / (stopy - starty);

			if (starty < MINY ) {
				startx = dx_dy*(MINY-starty)+startx;
				starty = MINY;
			}

			if (stopy > MAXY ) {
				stopx = dx_dy*(MAXY-starty)+startx;
				stopy = MAXY+1;
			}

			for (; starty < stopy; starty++ )
			{   if (y_edge_list[starty]==-1)
					y_edge_list[starty] = startx;
				else {
					x1 = y_edge_list[starty]>>16;
					x2 = startx>>16;

					if (x1 > x2 ) {
						temp = x2;
						x2 = x1;
						x1 = temp;
					}

					if ((x1 <= MAXX ) && (x2 >= MINX))
					{
						if (x1 < MINX ) x1 = MINX;
						if (x2 > MAXX ) x2 = MAXX+1;
						//gr_uscanline( x1, x2-1, starty );
						gr_scanline( x1, x2, starty );
					}
				}
				startx += dx_dy;
			}
		}


	} while (nverts > 0);
}

#endif

