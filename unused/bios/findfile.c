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
static char rcsid[] = "$Id: findfile.c,v 1.1.1.1 2001-01-19 03:30:14 bradleyb Exp $";
#pragma on (unreferenced)

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

// returns -1 if error
// Gets bytes free on current drive
int GetDiskFree ()
 {
	struct diskfree_t dfree;
	unsigned drive;

	_dos_getdrive(&drive);
	if (!_dos_getdiskfree(drive, &dfree))
		return (dfree.avail_clusters * dfree.sectors_per_cluster * dfree.bytes_per_sector);
   
   return (-1);
 }




