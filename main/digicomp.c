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
 * $Source: /cvsroot/dxx-rebirth/d1x-rebirth/main/digicomp.c,v $
 * $Revision: 1.1.1.1 $
 * $Author: zicodxx $
 * $Date: 2006/03/17 19:42:50 $
 * 
 * Routines for manipulating digi samples.
 * 
 * $Log: digicomp.c,v $
 * Revision 1.1.1.1  2006/03/17 19:42:50  zicodxx
 * initial import
 *
 * Revision 1.1.1.1  1999/06/14 22:06:00  donut
 * Import of d1x 1.37 source.
 *
 * Revision 2.0  1995/02/27  11:33:04  john
 * New version 2.0, which has no anonymous unions, builds with
 * Watcom 10.0, and doesn't require parsing BITMAPS.TBL.
 * 
 * Revision 1.2  1994/12/07  18:42:21  john
 * Initial, unused version..
 * 
 * Revision 1.1  1994/12/05  09:37:13  john
 * Initial revision
 * 
 * 
 */


#ifdef RCS
#pragma off (unreferenced)
static char rcsid[] = "$Id: digicomp.c,v 1.1.1.1 2006/03/17 19:42:50 zicodxx Exp $";
#pragma on (unreferenced)
#endif

#include "soscomp.h"

#pragma pack (4);						// Use 32-bit packing!
#pragma off (check_stack);			// No stack checking!
#include "sos.h"
#include "sosm.h"

ubyte digicomp_initialized = 0;

#define MAX_DIGI_BLOCKS 32
ubyte * digicomp_memory[ MAX_DIGI_MEMORY ];
int N_blocks = 0;
int Next_block = 0;

typedef struct digi_block {
	int offset;
	int len;
	int soundno;
} digi_block;

digi_block Digi_blocks[MAX_DIGI_BLOCKS];

void digicomp_init()
{
	int i;

	if ( digicomp_initialized  ) return;
	digicomp_initialized = 1;

	Digi_blocks[0].len = MAX_DIGI_MEMORY;
	Digi_blocks[0].offset = 0;
	Digi_blocks[0].soundno = -1;
	N_blocks = 1;

	for (i=1; i<MAX_DIGI_BLOCKS; i++ )	{
		Digi_blocks[i].len = 0;
		Digi_blocks[i].offset = 0;
		Digi_blocks[i].soundno = -1;
	}	
}

ubyte * digicomp_get_data(int soundnum)
{
	int mysize;
	int i;
	if ( !digicomp_initialized ) digicomp_init();

	// See if this sound already exists...
	for (i=0; i<MAX_DIGI_BLOCKS; i++ )	{
		Digi_blocks[i].len = 0;
		Digi_blocks[i].offset = 0;
		if ( Digi_blocks[i].soundno == soundnum )	{
			return &digicomp_memory[Digi_blocks[i].offset];
		}
	}
	
	// It doesn't exits, so look for the next unused hole that this data can fit into...
	mysize = (Sounds[soundnum].length+1)/2;
	i = Next_block;
	
	while( ((Digi_blocks[i].soundno >-1) && (  Digi_blocks[i].len < mysize ) )	{
		i++;
		if ( i > MAX_DIGI_BLOCKS )
			i = 0;
		if ( i == Next_block ) {
			// An unused block that this can fit into wasn't found, so find the next one it can fit into
			// and stop it...
			return NULL;
		}
	}

	
	
		
	

}



