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

#ifdef _WIN32_WCE
static char *UnicodeToAsc(const wchar_t *w_str)
{
	char *str = NULL;

	if (w_str != NULL)
    {
		int len = wcslen(w_str) + 1;
		str = (char *)malloc(len);

		if (WideCharToMultiByte(CP_ACP, 0, w_str, -1, str, len, NULL, NULL) == 0)
		{   //Conversion failed
			free(str);
			return NULL;
		}
		else
		{   //Conversion successful
			return(str);
		}
	}
	else
	{   //Given NULL string
		return NULL;
	}
}

static wchar_t *AscToUnicode(const char *str)
{
	wchar_t *w_str = NULL;
	if (str != NULL)
	{
		int len = strlen(str) + 1;
		w_str = (wchar_t *)malloc(sizeof(wchar_t) * len);
		if (MultiByteToWideChar(CP_ACP, 0, str, -1, w_str, len) == 0)
		{
			free(w_str);
			return NULL;
		}
		else
		{
			return(w_str);
		}
	}
	else
	{
		return NULL;
	}
}
#else
# define UnicodeToAsc(x) (x)
# define AscToUnicode(x) ((LPSTR)x)
#endif

//	Functions

int	FileFindFirst(char *search_str, FILEFINDSTRUCT *ffstruct)
{
	WIN32_FIND_DATA find;

	_FindFileHandle = FindFirstFile(AscToUnicode(search_str), &find);
	if (_FindFileHandle == INVALID_HANDLE_VALUE) return 1;
	else {
		ffstruct->size = find.nFileSizeLow;
		strcpy(ffstruct->name, UnicodeToAsc(find.cFileName));
		return 0; 
	}
}


int	FileFindNext(FILEFINDSTRUCT *ffstruct)
{
	WIN32_FIND_DATA find;

	if (!FindNextFile(_FindFileHandle, &find)) return 1;
	else {
		ffstruct->size = find.nFileSizeLow;
		strcpy(ffstruct->name, UnicodeToAsc(find.cFileName));
		return 0;
	}
}
 

int	FileFindClose(void)
{
	if (!FindClose(_FindFileHandle)) return 1;
	else return 0;
}


#if 0
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
#endif
