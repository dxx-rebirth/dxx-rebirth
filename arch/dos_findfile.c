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

/*
 * $Source: /cvs/cvsroot/d2x/arch/dos_findfile.c,v $
 * $Revision: 1.2 $
 * $Author: bradleyb $
 * $Date: 2001-01-29 13:35:08 $
 *
 * Dos findfile functions
 *
 * $Log: not supported by cvs2svn $
 */

#ifdef HAVE_CONFIG_H
#include <conf.h>
#endif

#include <dos.h>
#include <string.h>

#include "findfile.h"


//	Global Variables	----------------------------------------------------------

static int 				_FileFindFlag = 0;
static struct find_t _FileFindStruct;



//	Functions

int	FileFindFirst(char *search_str, FILEFINDSTRUCT *ffstruct)
{
	unsigned retval;
	
	if (_FileFindFlag) return -1;
	
	retval = _dos_findfirst(search_str, 0, &_FileFindStruct);
	if (retval) return (int)retval;
	else {
		ffstruct->size = _FileFindStruct.size;
		strcpy(ffstruct->name, _FileFindStruct.name);
		_FileFindFlag = 1;
		return (int)retval;
	}
}


int	FileFindNext(FILEFINDSTRUCT *ffstruct)
{
	unsigned retval;

	if (!_FileFindFlag) return -1;

	retval = _dos_findnext(&_FileFindStruct);
	if (retval) return (int)retval;
	else {
		ffstruct->size = _FileFindStruct.size;
		strcpy(ffstruct->name, _FileFindStruct.name);
		return (int)retval;
	}	
}
 

int	FileFindClose(void)
{
	unsigned retval = 0;

	if (!_FileFindFlag) return -1;
	
	if (retval) return (int)retval;
	else {
		_FileFindFlag = 0;
		return (int)retval;
	}
}


//returns 0 if no error
int GetFileDateTime(int filehandle, FILETIMESTRUCT *ftstruct)
{
	return _dos_getftime(filehandle, &ftstruct->date, &ftstruct->time);

}


//returns 0 if no error
int SetFileDateTime(int filehandle, FILETIMESTRUCT *ftstruct)
{
	return _dos_setftime(filehandle, ftstruct->date, ftstruct->time);
}
