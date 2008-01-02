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
 * $Source: /cvsroot/dxx-rebirth/d1x-rebirth/main/netmisc.c,v $
 * $Revision: 1.1.1.1 $
 * $Author: zicodxx $
 * $Date: 2006/03/17 19:42:53 $
 * 
 * Misc routines for network.
 * 
 * $Log: netmisc.c,v $
 * Revision 1.1.1.1  2006/03/17 19:42:53  zicodxx
 * initial import
 *
 * Revision 1.1.1.1  1999/06/14 22:09:33  donut
 * Import of d1x 1.37 source.
 *
 * Revision 2.0  1995/02/27  11:27:24  john
 * New version 2.0, which has no anonymous unions, builds with
 * Watcom 10.0, and doesn't require parsing BITMAPS.TBL.
 * 
 * Revision 1.3  1994/11/19  15:19:34  mike
 * rip out unused code and data.
 * 
 * Revision 1.2  1994/08/09  19:31:53  john
 * Networking changes.
 * 
 * Revision 1.1  1994/08/08  11:06:07  john
 * Initial revision
 * 
 * 
 */


#ifdef RCS
static char rcsid[] = "$Id: netmisc.c,v 1.1.1.1 2006/03/17 19:42:53 zicodxx Exp $";
#endif

#include <string.h>

#include "pstypes.h"
#include "mono.h"

// Calculates the checksum of a block of memory.
unsigned short netmisc_calc_checksum( void * vptr, int len )
{
	ubyte * ptr = (ubyte *)vptr;
	unsigned int sum1,sum2;

	sum1 = sum2 = 0;

	while(len--)	{
		sum1 += *ptr++;
		if (sum1 >= 255 ) sum1 -= 255;
		sum2 += sum1;
	}
	sum2 %= 255;
	
	return ((sum1<<8)+ sum2);
}

//--unused-- //Finds the difference between block1 and block2.  Fills in diff_buffer and 
//--unused-- //returns the size of diff_buffer.
//--unused-- int netmisc_find_diff( void *block1, void *block2, int block_size, void *diff_buffer )
//--unused-- {
//--unused-- 	int mode;
//--unused-- 	ushort *c1, *c2, *diff_start, *c3;
//--unused-- 	int i, j, size, diff, n , same;
//--unused-- 
//--unused-- 	size=(block_size+1)/sizeof(ushort);
//--unused-- 	c1 = (ushort *)block1;
//--unused-- 	c2 = (ushort *)block2;
//--unused-- 	c3 = (ushort *)diff_buffer;
//--unused-- 
//--unused-- 	mode = same = diff = n = 0;
//--unused-- 
//--unused-- 	//mprintf( 0, "=================================\n" );
//--unused-- 
//--unused-- 	for (i=0; i<size; i++, c1++, c2++ )	{
//--unused-- 		if (*c1 != *c2 ) {
//--unused-- 			if (mode==0)	{
//--unused-- 				mode = 1;
//--unused-- 				//mprintf( 0, "%ds ", same );
//--unused-- 				c3[n++] = same;
//--unused-- 				same=0; diff=0;
//--unused-- 				diff_start = c2;
//--unused-- 			}
//--unused-- 			*c1 = *c2;
//--unused-- 			diff++;
//--unused-- 			if (diff==65535) {
//--unused-- 				mode = 0;
//--unused-- 				// send how many diff ones.
//--unused-- 				//mprintf( 0, "%dd ", diff );
//--unused-- 				c3[n++]=diff;
//--unused-- 				// send all the diff ones.
//--unused-- 				for (j=0; j<diff; j++ )
//--unused-- 					c3[n++] = diff_start[j];
//--unused-- 				same=0; diff=0;
//--unused-- 				diff_start = c2;
//--unused-- 			}
//--unused-- 		} else {
//--unused-- 			if (mode==1)	{
//--unused-- 				mode=0;
//--unused-- 				// send how many diff ones.
//--unused-- 				//mprintf( 0, "%dd ", diff );
//--unused-- 				c3[n++]=diff;
//--unused-- 				// send all the diff ones.
//--unused-- 				for (j=0; j<diff; j++ )
//--unused-- 					c3[n++] = diff_start[j];
//--unused-- 				same=0; diff=0;
//--unused-- 				diff_start = c2;
//--unused-- 			}
//--unused-- 			same++;
//--unused-- 			if (same==65535)	{
//--unused-- 				mode=1;
//--unused-- 				// send how many the same
//--unused-- 				//mprintf( 0, "%ds ", same );
//--unused-- 				c3[n++] = same;
//--unused-- 				same=0; diff=0;
//--unused-- 				diff_start = c2;
//--unused-- 			}
//--unused-- 		}
//--unused-- 	
//--unused-- 	}
//--unused-- 	if (mode==0)	{
//--unused-- 		// send how many the same
//--unused-- 		//mprintf( 0, "%ds ", same );
//--unused-- 		c3[n++] = same;
//--unused-- 	} else {
//--unused-- 		// send how many diff ones.
//--unused-- 		//mprintf( 0, "%dd ", diff );
//--unused-- 		c3[n++]=diff;
//--unused-- 		// send all the diff ones.
//--unused-- 		for (j=0; j<diff; j++ )
//--unused-- 			c3[n++] = diff_start[j];
//--unused-- 	}
//--unused-- 
//--unused-- 	//mprintf( 0, "=================================\n" );
//--unused-- 	
//--unused-- 	return n*2;
//--unused-- }

//--unused-- //Applies diff_buffer to block1 to create a new block1.  Returns the final
//--unused-- //size of block1.
//--unused-- int netmisc_apply_diff(void *block1, void *diff_buffer, int diff_size )	
//--unused-- {
//--unused-- 	unsigned int i, j, n, size;
//--unused-- 	ushort *c1, *c2;
//--unused-- 
//--unused-- 	//mprintf( 0, "=================================\n" );
//--unused-- 	c1 = (ushort *)diff_buffer;
//--unused-- 	c2 = (ushort *)block1;
//--unused-- 
//--unused-- 	size = diff_size/2;
//--unused-- 
//--unused-- 	i=j=0;
//--unused-- 	while (1)	{
//--unused-- 		j += c1[i];			// Same
//--unused-- 		//mprintf( 0, "%ds ", c1[i] );
//--unused-- 		i++;
//--unused-- 		if ( i>=size) break;
//--unused-- 		n = c1[i];			// ndiff
//--unused-- 		//mprintf( 0, "%dd ", c1[i] );
//--unused-- 		i++;
//--unused-- 		if (n>0)	{
//--unused-- 			//Assert( n* < 256 );
//--unused-- 			memcpy( &c2[j], &c1[i], n*2 );
//--unused-- 			i += n;
//--unused-- 			j += n;
//--unused-- 		}
//--unused-- 		if ( i>=size) break;
//--unused-- 	}
//--unused-- 	//mprintf( 0, "=================================\n" );
//--unused-- 
//--unused-- 	return j*2;
//--unused-- }
