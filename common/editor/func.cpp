/*
 * Portions of this file are copyright Rebirth contributors and licensed as
 * described in COPYING.txt.
 * Portions of this file are copyright Parallax Software and licensed
 * according to the Parallax license below.
 * See COPYING.txt for license details.

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
 *
 * .
 *
 */


#include <stdlib.h>
#include <string.h>

#include "func.h"
#include "strutil.h"

inline namespace dcx {

#define MAX_PARAMS 10

static const FUNCTION * func_table = NULL;
static int func_size = 0;
static int initialized = 0;
static int func_params[MAX_PARAMS];

void func_init( const FUNCTION * funtable, int size )
{
	if (!initialized)
	{
		initialized = 1;
		func_table = funtable;
		func_size = size;
		atexit( func_close );
	}
}


void func_close()
{
	if (initialized)
	{
		initialized = 0;
		func_table = NULL;
		func_size = 0;
	}
}

int (*func_get( char * name, int * numparams ))(void)
{
	for (int i=0; i<func_size; i++ )
		if (!d_stricmp( name, func_table[i].name ))
		{
			*numparams = func_table[i].nparams;
			return func_table[i].cfunction;
		}

	return NULL;
}

int func_get_index( char * name )
{
	for (int i=0; i<func_size; i++ )
		if (!d_stricmp( name, func_table[i].name ))
		{
			return i;
		}

	return -1;
}


int (*func_nget( int func_number, int * numparams, const char **name ))(void)
{
	if (func_number < func_size )
	{
		*name = func_table[func_number].name;
		*numparams = func_table[func_number].nparams;
		return func_table[func_number].cfunction;
	}

	return NULL;
}

int func_get_param( int n )
{
	return func_params[n];
}


}
