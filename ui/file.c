/* $Id: file.c,v 1.10 2005-03-05 09:18:46 chris Exp $ */
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

#ifdef RCS
static char rcsid[] = "$Id: file.c,v 1.10 2005-03-05 09:18:46 chris Exp $";
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <direct.h>
#include <dos.h>
#include <ctype.h>
#include <conio.h>
#include <fcntl.h>
#include <io.h>
#include <sys\types.h>
#include <sys\stat.h>

#include "fix.h"
#include "pstypes.h"
#include "gr.h"
#include "key.h"
#include "strutil.h"

#include "ui.h"
#include "mono.h"

#include "u_mem.h"

char filename_list[300][13];
char directory_list[100][13];

int file_sort_func(char *e0, char *e1)
{
	return stricmp(e0, e1);
}


int SingleDrive()
{
	int FloppyPresent, FloppyNumber;
	unsigned char b;

	b = *((unsigned char *)0x410);

	FloppyPresent = b & 1;
	FloppyNumber = ((b&0xC0)>>6)+1;
	if (FloppyPresent && (FloppyNumber==1) )
		return 1;
	else
		return 0;
}

void SetFloppy(int d)
{
	if (SingleDrive())
	{
	if (d==1)
		*((unsigned char *)0x504) = 0;
	else if (d==2)
		*((unsigned char *)0x504) = 1;
	}
}


void file_capitalize( char * s )
{
	while( *s++ = toupper(*s) );
}


// Changes to a drive if valid.. 1=A, 2=B, etc
// If flag, then changes to it.
// Returns 0 if not-valid, 1 if valid.
int file_chdrive( int DriveNum, int flag )
{
	unsigned NumDrives, n, org;
	int Valid = 0;

	if (!flag)
		_dos_getdrive( &org );

	_dos_setdrive( DriveNum, &NumDrives );
	_dos_getdrive( &n );

	if (n == DriveNum )
		Valid = 1;

	if ( (!flag) && (n != org) )
		_dos_setdrive( org, &NumDrives );

	return Valid;
}


// Changes to directory in dir.  Even drive is changed.
// Returns 1 if failed.
//  0 = Changed ok.
//  1 = Invalid disk drive.
//  2 = Invalid directory.

int file_chdir( char * dir )
{
	int e;
	char OriginalDirectory[100];
	char * Drive, * Path;
	char NoDir[] = "\.";

	getcwd( OriginalDirectory, 100 );

	file_capitalize( dir );

	Drive = strchr(dir, ':');

	if (Drive)
	{
		if (!file_chdrive( *(Drive - 1) - 'A' + 1, 1))
			return 1;

		Path = Drive+1;

		SetFloppy(*(Drive - 1) - 'A' + 1);
	}
	else
	{
		Path = dir;

	}

	if (!(*Path))
	{
		Path = NoDir;
	}

	// This chdir might get a critical error!
	e = chdir( Path );
	if (e)
	{
		file_chdrive( OriginalDirectory[0] - 'A'+1, 1 );
		return 2;
	}

	return 0;

}


int file_getdirlist( int MaxNum, char list[][13] )
{
	struct find_t find;
	int NumDirs = 0, i, CurDrive;
	char cwd[129];
	MaxNum = MaxNum;

	getcwd(cwd, 128 );

	if (strlen(cwd) >= 4)
	{
		sprintf( list[NumDirs++], ".." );
	}

	CurDrive = cwd[0] - 'A' + 1;

	if( !_dos_findfirst( "*.", _A_SUBDIR, &find ) )
	{
		if ( find.attrib & _A_SUBDIR )	{
			if (strcmp( "..", find.name) && strcmp( ".", find.name))
				strncpy(list[NumDirs++], find.name, 13 );
		}

		while( !_dos_findnext( &find ) )
		{
			if ( find.attrib & _A_SUBDIR )	{
				if (strcmp( "..", find.name) && strcmp( ".", find.name))
				{
					if (NumDirs==74)
					{
						MessageBox( -2,-2, 1, "Only the first 74 directories will be displayed.", "Ok" );
						break;
					} else {
						strncpy(list[NumDirs++], find.name, 13 );
					}
				}
			}
		}
	}

	qsort(list, NumDirs, 13, (int (*)( const void *, const void * ))file_sort_func);

	for (i=1; i<=26; i++ )
	{
		if (file_chdrive(i,0) && (i!=CurDrive))
		{
			if (!((i==2) && SingleDrive()))
				sprintf( list[NumDirs++], "%c:", i+'A'-1 );
		}
	}

	return NumDirs;
}

int file_getfilelist( int MaxNum, char list[][13], char * filespec )
{
	struct find_t find;
	int NumFiles = 0;

	MaxNum = MaxNum;

	if( !_dos_findfirst( filespec, 0, &find ) )
	{
		//if ( !(find.attrib & _A_SUBDIR) )
			strncpy(list[NumFiles++], find.name, 13 );

		while( !_dos_findnext( &find ) )
		{
			//if ( !(find.attrib & _A_SUBDIR) )
			//{
				if (NumFiles==300)
				{
					MessageBox( -2,-2, 1, "Only the first 300 files will be displayed.", "Ok" );
					break;
				} else {
					strncpy(list[NumFiles++], find.name, 13 );
				}
			//}

		}
	}

	qsort(list, NumFiles, 13, (int (*)( const void *, const void * ))file_sort_func);

	return NumFiles;
}

static int FirstTime = 1;
static char CurDir[128];

int ui_get_filename( char * filename, char * Filespec, char * message  )
{
	FILE * TempFile;
	int NumFiles, NumDirs,i;
	char InputText[100];
	char Spaces[35];
	UI_WINDOW * wnd;
	UI_GADGET_BUTTON * Button1, * Button2, * HelpButton;
	UI_GADGET_LISTBOX * ListBox1;
	UI_GADGET_LISTBOX * ListBox2;
	UI_GADGET_INPUTBOX * UserFile;
	int new_listboxes;

	char drive[ _MAX_DRIVE ];
	char dir[ _MAX_DIR ];
	char fname[ _MAX_FNAME ];
	char ext[ _MAX_EXT ];
	char fulldir[ _MAX_DIR + _MAX_DRIVE ];
	char fullfname[ _MAX_FNAME + _MAX_EXT ];


	char OrgDir[128];

	getcwd( OrgDir, 128 );

	if (FirstTime)
		getcwd( CurDir, 128 );
	FirstTime=0;

	file_chdir( CurDir );
	
	//MessageBox( -2,-2, 1,"DEBUG:0", "Ok" );
	for (i=0; i<35; i++)
		Spaces[i] = ' ';
	Spaces[34] = 0;

	NumFiles = file_getfilelist( 300, filename_list, Filespec );

	NumDirs = file_getdirlist( 100, directory_list );

	wnd = ui_open_window( 200, 100, 400, 370, WIN_DIALOG );

	ui_wprintf_at( wnd, 10, 5, message );

	_splitpath( filename, drive, dir, fname, ext );

	sprintf( InputText, "%s%s", fname, ext );

	ui_wprintf_at( wnd, 20, 32,"N&ame" );
	UserFile  = ui_add_gadget_inputbox( wnd, 60, 30, 40, 40, InputText );

	ui_wprintf_at( wnd, 20, 86,"&Files" );
	ui_wprintf_at( wnd, 210, 86,"&Dirs" );

	ListBox1 = ui_add_gadget_listbox( wnd,  20, 110, 125, 200, NumFiles, filename_list, 13 );
	ListBox2 = ui_add_gadget_listbox( wnd, 210, 110, 100, 200, NumDirs, directory_list, 13 );

	Button1 = ui_add_gadget_button( wnd,     20, 330, 60, 25, "Ok", NULL );
	Button2 = ui_add_gadget_button( wnd,    100, 330, 60, 25, "Cancel", NULL );
	HelpButton = ui_add_gadget_button( wnd, 180, 330, 60, 25, "Help", NULL );

	wnd->keyboard_focus_gadget = (UI_GADGET *)UserFile;

	Button1->hotkey = KEY_CTRLED + KEY_ENTER;
	Button2->hotkey = KEY_ESC;
	HelpButton->hotkey = KEY_F1;
	ListBox1->hotkey = KEY_ALTED + KEY_F;
	ListBox2->hotkey = KEY_ALTED + KEY_D;
	UserFile->hotkey = KEY_ALTED + KEY_A;

	ui_gadget_calc_keys(wnd);

	ui_wprintf_at( wnd, 20, 60, "%s", Spaces );
	ui_wprintf_at( wnd, 20, 60, "%s", CurDir );

	new_listboxes = 0;

	while( 1 )
	{
		ui_mega_process();
		ui_window_do_gadgets(wnd);

		if ( Button2->pressed )
		{
			file_chdir( OrgDir );
			ui_close_window(wnd);
			return 0;
		}

		if ( HelpButton->pressed )
			MessageBox( -1, -1, 1, "Sorry, no help is available!", "Ok" );

		if (ListBox1->moved || new_listboxes)
		{
			if (ListBox1->current_item >= 0 )
			{
				strcpy(UserFile->text, filename_list[ListBox1->current_item] );
				UserFile->position = strlen(UserFile->text);
				UserFile->oldposition = UserFile->position;
				UserFile->status=1;
				UserFile->first_time = 1;
			}
		}

		if (ListBox2->moved || new_listboxes)
		{
			if (ListBox2->current_item >= 0 )
			{
				if (strrchr( directory_list[ListBox2->current_item], ':' ))
					sprintf( UserFile->text, "%s%s", directory_list[ListBox2->current_item], Filespec );
				else
					sprintf( UserFile->text, "%s\\%s", directory_list[ListBox2->current_item], Filespec );
				UserFile->position = strlen(UserFile->text);
				UserFile->oldposition = UserFile->position;
				UserFile->status=1;
				UserFile->first_time = 1;
			}
		}
		new_listboxes = 0;

		if (Button1->pressed || UserFile->pressed || (ListBox1->selected_item > -1 ) || (ListBox2->selected_item > -1 ))
		{
			ui_mouse_hide();

			if (ListBox2->selected_item > -1 )	{
				if (strrchr( directory_list[ListBox2->selected_item], ':' ))
					sprintf( UserFile->text, "%s%s", directory_list[ListBox2->selected_item], Filespec );
				else
					sprintf( UserFile->text, "%s\\%s", directory_list[ListBox2->selected_item], Filespec );
			}

			TempFile = fopen( UserFile->text, "r" );
			if (TempFile)
			{
				// Looks like a valid filename that already exists!
				fclose( TempFile );
				break;
			}

			// File doesn't exist, but can we create it?
			TempFile = fopen( UserFile->text, "w" );
			if (TempFile)
			{
				// Looks like a valid filename!
				fclose( TempFile );
				remove( UserFile->text );
				break;
			}

			_splitpath( UserFile->text, drive, dir, fname, ext );
			sprintf( fullfname, "%s%s", fname, ext );

			//mprintf( 0, "----------------------------\n" );
			//mprintf( 0, "Full text: '%s'\n", UserFile->text );
			//mprintf( 0, "Drive: '%s'\n", drive );
			//mprintf( 0, "Dir: '%s'\n", dir );
		  	//mprintf( 0, "Filename: '%s'\n", fname );
			//mprintf( 0, "Extension: '%s'\n", ext );
			//mprintf( 0, "Full dir: '%s'\n", fulldir );
			//mprintf( 0, "Full fname: '%s'\n", fname );

			if (strrchr( fullfname, '?' ) || strrchr( fullfname, '*' ) )	
			{
				sprintf( fulldir, "%s%s.", drive, dir );
			} else {
				sprintf( fullfname, "%s", Filespec );
				sprintf( fulldir, "%s", UserFile->text );
			}

			//mprintf( 0, "----------------------------\n" );
			//mprintf( 0, "Full dir: '%s'\n", fulldir );
			//mprintf( 0, "Full fname: '%s'\n", fullfname );

			if (file_chdir( fulldir )==0)
			{
				NumFiles = file_getfilelist( 300, filename_list, fullfname );

				strcpy(UserFile->text, fullfname );
				UserFile->position = strlen(UserFile->text);
				UserFile->oldposition = UserFile->position;
				UserFile->status=1;
				UserFile->first_time = 1;

				NumDirs = file_getdirlist( 100, directory_list );

				ui_listbox_change( wnd, ListBox1, NumFiles, filename_list, 13 );
				ui_listbox_change( wnd, ListBox2, NumDirs, directory_list, 13 );
				new_listboxes = 0;

				getcwd( CurDir, 35 );
				ui_wprintf_at( wnd, 20, 60, "%s", Spaces );
				ui_wprintf_at( wnd, 20, 60, "%s", CurDir );

				//i = TICKER;
				//while ( TICKER < i+2 );

			}

			ui_mouse_show();

		}

		gr_update();
	}

	//key_flush();

	_splitpath( UserFile->text, drive, dir, fname, ext );
	sprintf( fulldir, "%s%s.", drive, dir );
	sprintf( fullfname, "%s%s", fname, ext );
	
	if ( strlen(fulldir) > 1 )
		file_chdir( fulldir );
	
	getcwd( CurDir, 35 );
		
	if ( strlen(CurDir) > 0 )
	{
			if ( CurDir[strlen(CurDir)-1] == '\\' )
				CurDir[strlen(CurDir)-1] = 0;
	}
		
	sprintf( filename, "%s\\%s", CurDir, fullfname );
	//MessageBox( -2, -2, 1, filename, "Ok" );
	
	file_chdir( OrgDir );
		
	ui_close_window(wnd);

	return 1;
}



int ui_get_file( char * filename, char * Filespec  )
{
	int x, i, NumFiles;
	char * text[200];

	NumFiles = file_getfilelist( 200, filename_list, Filespec );

	for (i=0; i< NumFiles; i++ )
	{
		MALLOC( text[i], char, 15 );
		strcpy(text[i], filename_list[i] );
	}

	x = MenuX( -1, -1, NumFiles, text );

	if ( x > 0 )
		strcpy(filename, filename_list[x-1] );

	for (i=0; i< NumFiles; i++ )
	{
		d_free( text[i] );
	}

	if ( x>0 )
		return 1;
	else
		return 0;

}

