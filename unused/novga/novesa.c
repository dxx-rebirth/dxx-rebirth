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
COPYRIGHT 1993-1999 PARALLAX SOFTWARE CORPORATION.  ALL RIGHTS RESERVED.
*/


#pragma off (unreferenced)
static char rcsid[] = "$Id: novesa.c,v 1.1.1.1 2001-01-19 03:30:15 bradleyb Exp $";
#pragma on (unreferenced)

#include "pstypes.h"

#include "gr.h"


void gr_vesa_update( grs_bitmap * source1, grs_bitmap * dest, grs_bitmap * source2 )
{

}


int  gr_modex_setmode(short mode)
{
	return 0;
}


void gr_modex_setplane(short plane)
{

}


void gr_modex_setstart(short x, short y, int wait_for_retrace)
{

}


void gr_modex_uscanline( short x1, short x2, short y, unsigned char color )
{

}


int  gr_vesa_setmodea(int mode)
{

}


int  gr_vesa_checkmode(int mode)
{

}


void gr_vesa_setstart(short x, short y )
{

}


void gr_vesa_setpage(int page)
{

}


void gr_vesa_incpage()
{

}


void gr_vesa_scanline(short x1, short x2, short y, unsigned char color )
{

}


int  gr_vesa_setlogical(int pixels_per_scanline)
{

}


void gr_vesa_bitblt( unsigned char * source_ptr, unsigned int vesa_address, int height, int width )
{

}


void gr_vesa_pixel( unsigned char color, unsigned int offset )
{

}


void gr_vesa_bitmap( grs_bitmap * source, grs_bitmap * dest, int x, int y )
{

}


void gr_modex_line()
{

}
