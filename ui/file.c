/* $Id: file.c,v 1.12 2005-07-03 03:08:25 chris Exp $ */
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
static char rcsid[] = "$Id: file.c,v 1.12 2005-07-03 03:08:25 chris Exp $";
#endif

#include <stdlib.h>
#include <string.h>
#include <physfs.h>

#include "fix.h"
#include "pstypes.h"
#include "gr.h"
#include "key.h"
#include "strutil.h"

#include "ui.h"
#include "mono.h"

#include "u_mem.h"

int file_sort_func(char **e0, char **e1)
{
	return stricmp(*e0, *e1);
}


char **file_getdirlist(int *NumDirs, char *dir)
{
	char	path[PATH_MAX];
	char	**list = PHYSFS_enumerateFiles(dir);
	char	**i, **j = list;
	char	*test_filename;
	int		test_max;

	if (!list)
		return NULL;

	strcpy(path, dir);
	if (*path)
		strncat(path, "/", PATH_MAX - strlen(dir));

	test_filename = path + strlen(path);
	test_max = PATH_MAX - (test_filename - path);

	for (i = list; *i; i++)
	{
		if (strlen(*i) >= test_max)
			continue;

		strcpy(test_filename, *i);
		if (PHYSFS_isDirectory(path))
			*j++ = *i;
		else
			free(*i);
	}
	*j = NULL;

	*NumDirs = j - list;
	qsort(list, *NumDirs, sizeof(char *), (int (*)( const void *, const void * ))file_sort_func);

	if (*dir)
	{
		// Put the 'go to parent directory' sequence '..' first
		(*NumDirs)++;
		list = realloc(list, sizeof(char *)*(*NumDirs + 1));
		list[*NumDirs] = NULL;	// terminate
		for (i = list + *NumDirs - 1; i != list; i--)
			*i = i[-1];
		list[0] = strdup("..");
	}

	return list;
}

char **file_getfilelist(int *NumFiles, char *filespec, char *dir)
{
	char **list = PHYSFS_enumerateFiles(dir);
	char **i, **j = list, *ext;

	if (!list)
		return NULL;

	if (*filespec == '*')
		filespec++;

	for (i = list; *i; i++)
	{
		ext = strrchr(*i, '.');
		if (ext && (!stricmp(ext, filespec)))
			*j++ = *i;
		else
			free(*i);
	}
	*j = NULL;


	*NumFiles = j - list;
	qsort(list, *NumFiles, sizeof(char *), (int (*)( const void *, const void * ))file_sort_func);

	return list;
}

int ui_get_filename( char * filename, char * Filespec, char * message  )
{
	char		ViewDir[PATH_MAX];
	char		InputText[PATH_MAX];
	char		*p;
	PHYSFS_file	*TempFile;
	int			NumFiles, NumDirs,i;
	char		**filename_list;
	char		**directory_list;
	char		Spaces[35];
	UI_WINDOW			*wnd;
	UI_GADGET_BUTTON	*Button1, *Button2, *HelpButton;
	UI_GADGET_LISTBOX	*ListBox1;
	UI_GADGET_LISTBOX	*ListBox2;
	UI_GADGET_INPUTBOX	*UserFile;
	int 				new_listboxes;

	if ((p = strrchr(filename, '/')))
	{
		*p++ = 0;
		strcpy(ViewDir, filename);
		strcpy(InputText, p);
	}
	else
	{
		strcpy(ViewDir, "");
		strcpy(InputText, filename);
	}

	filename_list = file_getfilelist(&NumFiles, Filespec, ViewDir);
	directory_list = file_getdirlist(&NumDirs, ViewDir);

	// Running out of memory may become likely in the future
	if (!filename_list && !directory_list)
		return 0;

	if (!filename_list)
	{
		PHYSFS_freeList(directory_list);
		return 0;
	}

	if (!directory_list)
	{
		PHYSFS_freeList(filename_list);
		return 0;
	}

	//MessageBox( -2,-2, 1,"DEBUG:0", "Ok" );
	for (i=0; i<35; i++)
		Spaces[i] = ' ';
	Spaces[34] = 0;

	wnd = ui_open_window( 200, 100, 400, 370, WIN_DIALOG );

	ui_wprintf_at( wnd, 10, 5, message );

	ui_wprintf_at( wnd, 20, 32,"N&ame" );
	UserFile  = ui_add_gadget_inputbox( wnd, 60, 30, PATH_MAX, 40, InputText );

	ui_wprintf_at( wnd, 20, 86,"&Files" );
	ui_wprintf_at( wnd, 210, 86,"&Dirs" );

	ListBox1 = ui_add_gadget_listbox(wnd,  20, 110, 125, 200, NumFiles, filename_list);
	ListBox2 = ui_add_gadget_listbox(wnd, 210, 110, 100, 200, NumDirs, directory_list);

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
	ui_wprintf_at( wnd, 20, 60, "%s", ViewDir );

	new_listboxes = 0;

	while( 1 )
	{
		ui_mega_process();
		ui_window_do_gadgets(wnd);

		if ( Button2->pressed )
		{
			PHYSFS_freeList(filename_list);
			PHYSFS_freeList(directory_list);
			ui_close_window(wnd);
			return 0;
		}

		if ( HelpButton->pressed )
			MessageBox( -1, -1, 1, "Sorry, no help is available!", "Ok" );

		if (ListBox1->moved || new_listboxes)
		{
			if (ListBox1->current_item >= 0 )
				ui_inputbox_set_text(UserFile, filename_list[ListBox1->current_item]);
		}

		if (ListBox2->moved || new_listboxes)
		{
			if (ListBox2->current_item >= 0 )
				ui_inputbox_set_text(UserFile, directory_list[ListBox2->current_item]);
		}
		new_listboxes = 0;

		if (Button1->pressed || UserFile->pressed || (ListBox1->selected_item > -1 ) || (ListBox2->selected_item > -1 ))
		{
			ui_mouse_hide();

			if (ListBox2->selected_item > -1 )
				strcpy(UserFile->text, directory_list[ListBox2->selected_item]);

			strncpy(filename, ViewDir, PATH_MAX);

			p = UserFile->text;
			while (!strncmp(p, "..", 2))	// shorten the path manually
			{
				char *sep = strrchr(filename, '/');
				if (sep)
					*sep = 0;
				else
					*filename = 0;	// look directly in search paths

				p += 2;
				if (*p == '/')
					p++;
			}

			if (*filename && *p)
				strncat(filename, "/", PATH_MAX - strlen(filename));
			strncat(filename, p, PATH_MAX - strlen(filename));

			if (!PHYSFS_isDirectory(filename))
			{
				TempFile = PHYSFS_openRead(filename);
				if (TempFile)
				{
					// Looks like a valid filename that already exists!
					PHYSFS_close(TempFile);
					break;
				}
	
				// File doesn't exist, but can we create it?
				TempFile = PHYSFS_openWrite(filename);
				if (TempFile)
				{
					// Looks like a valid filename!
					PHYSFS_close(TempFile);
					PHYSFS_delete(filename);
					break;
				}
			}
			else
			{
				if (filename[strlen(filename) - 1] == '/')	// user typed a separator on the end
					filename[strlen(filename) - 1] = 0;
	
				strcpy(ViewDir, filename);
	
				//mprintf( 0, "----------------------------\n" );
				//mprintf( 0, "Full dir: '%s'\n", ViewDir );
	
				PHYSFS_freeList(filename_list);
				filename_list = file_getfilelist(&NumFiles, Filespec, ViewDir);
				if (!filename_list)
				{
					PHYSFS_freeList(directory_list);
					return 0;
				}

				ui_inputbox_set_text(UserFile, Filespec);

				PHYSFS_freeList(directory_list);
				directory_list = file_getdirlist(&NumDirs, ViewDir);
				if (!directory_list)
				{
					PHYSFS_freeList(filename_list);
					return 0;
				}

				ui_listbox_change(wnd, ListBox1, NumFiles, filename_list);
				ui_listbox_change(wnd, ListBox2, NumDirs, directory_list);
				new_listboxes = 0;

				ui_wprintf_at( wnd, 20, 60, "%s", Spaces );
				ui_wprintf_at( wnd, 20, 60, "%s", ViewDir );

				//i = TICKER;
				//while ( TICKER < i+2 );

			}

			ui_mouse_show();

		}

		gr_update();
	}

	//key_flush();

	ui_close_window(wnd);
	if (filename_list)
		PHYSFS_freeList(filename_list);
	if (directory_list)
		PHYSFS_freeList(directory_list);

	return 1;
}



int ui_get_file( char * filename, char * Filespec  )
{
	int x, NumFiles;
	char **list = file_getfilelist(&NumFiles, Filespec, "");

	if (!list) return 0;

	x = MenuX(-1, -1, NumFiles, list);

	if (x > 0)
		strcpy(filename, list[x - 1]);

	PHYSFS_freeList(list);

	return (x > 0);
}

