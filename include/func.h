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
 * $Source: /cvsroot/dxx-rebirth/d1x-rebirth/include/func.h,v $
 * $Revision: 1.1.1.1 $
 * $Author: zicodxx $
 * $Date: 2006/03/17 19:46:29 $
 * 
 * .
 * 
 * $Log: func.h,v $
 * Revision 1.1.1.1  2006/03/17 19:46:29  zicodxx
 * initial import
 *
 * Revision 1.1.1.1  1999/06/14 22:02:09  donut
 * Import of d1x 1.37 source.
 *
 * Revision 1.1  1994/11/27  14:52:03  matt
 * Initial revision
 * 
 * Revision 1.1  1994/11/21  14:13:31  matt
 * Initial revision
 * 
 * Revision 1.1  1993/11/15  12:28:13  john
 * Initial revision
 * 
 * 
 */



#ifndef _FUNC_H
#define _FUNC_H

typedef struct {
	char *  name;
	int     nparams;
        int   (*cfunction)(void);
} FUNCTION;

void func_init( FUNCTION * funtable, int size );
void func_close();
int (*func_get( char * name, int * numparams ))(void);
int func_howmany();
int (*func_nget( int func_number, int * numparams, char **name ))(void);
void func_set_param( int n, int value );
int func_get_param( int n );
int func_get_index( char * name );

#endif
