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

#ifdef HAVE_CONFIG_H
#include <conf.h>
#endif

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include "findfile.h"


//	Global Variables	----------------------------------------------------------

static HANDLE	_FindFileHandle = INVALID_HANDLE_VALUE;



//	Functions

int	FileFindFirst(char *search_str, FILEFINDSTRUCT *ffstruct)
{
	WIN32_FIND_DATA find;

	_FindFileHandle =  FindFirstFile((LPSTR)search_str, &find);
	if (_FindFileHandle == INVALID_HANDLE_VALUE) return 1;
	else {
		ffstruct->size = find.nFileSizeLow;
		strcpy(ffstruct->name, find.cFileName);
		return 0; 
	}
}


int	FileFindNext(FILEFINDSTRUCT *ffstruct)
{
	WIN32_FIND_DATA find;

	if (!FindNextFile(_FindFileHandle, &find)) return 1;
	else {
		ffstruct->size = find.nFileSizeLow;
		strcpy(ffstruct->name, find.cFileName);
		return 0;
	}
}
 

int	FileFindClose(void)
{
	if (!FindClose(_FindFileHandle)) return 1;
	else return 0;
}


int GetFileDateTime(int filehandle, FILETIMESTRUCT *ftstruct)
{
	FILETIME filetime;
	int retval;

	retval = GetFileTime((HANDLE)filehandle, NULL, NULL, &filetime);
	if (retval) {
		FileTimeToDosDateTime(&filetime, &ftstruct->date, &ftstruct->time);
		return 0;
	}
	else return 1;
}


//returns 0 if no error
int SetFileDateTime(int filehandle, FILETIMESTRUCT *ftstruct)
{
	FILETIME ft;
	int retval;

	DosDateTimeToFileTime(ftstruct->date, ftstruct->time, &ft);
	retval = SetFileTime((HANDLE)filehandle, NULL, NULL, &ft);	
	if (retval) return 0;
	else return 1;
}
