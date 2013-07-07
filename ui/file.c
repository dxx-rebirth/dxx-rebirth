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

#include <stdlib.h>
#include <string.h>

#include "event.h"
#include "physfsx.h"
#include "fix.h"
#include "pstypes.h"
#include "gr.h"
#include "key.h"
#include "mouse.h"
#include "strutil.h"
#include "ui.h"
#include "window.h"
#include "u_mem.h"

int file_sort_func(char **e0, char **e1)
{
	return d_stricmp(*e0, *e1);
}


char **file_getdirlist(int *NumDirs, char *dir)
{
	char	path[PATH_MAX];
	char	**list = PHYSFS_enumerateFiles(dir);
	char	**i, **j = list;
	char	*test_filename;
	unsigned		test_max;

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
		list = (char **)realloc(list, sizeof(char *)*(*NumDirs + 1));
		list[*NumDirs] = NULL;	// terminate
		for (i = list + *NumDirs - 1; i != list; i--)
			*i = i[-1];
		list[0] = d_strdup("..");
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
		if (ext && (!d_stricmp(ext, filespec)))
			*j++ = *i;
		else
			free(*i);
	}
	*j = NULL;


	*NumFiles = j - list;
	qsort(list, *NumFiles, sizeof(char *), (int (*)( const void *, const void * ))file_sort_func);

	return list;
}

typedef struct browser
{
	char		view_dir[PATH_MAX];
	char		*filename;
	char		*filespec;
	char		*message;
	char		**filename_list;
	char		**directory_list;
	UI_GADGET_BUTTON	*button1, *button2, *help_button;
	UI_GADGET_LISTBOX	*listbox1;
	UI_GADGET_LISTBOX	*listbox2;
	UI_GADGET_INPUTBOX	*user_file;
	int			num_files, num_dirs;
	char		spaces[35];
} browser;

static int browser_handler(UI_DIALOG *dlg, d_event *event, browser *b)
{
	int rval = 0;

	if (event->type == EVENT_UI_DIALOG_DRAW)
	{
		ui_dprintf_at( dlg, 10, 5, "%s", b->message );

		ui_dprintf_at( dlg, 20, 32,"N&ame" );
		ui_dprintf_at( dlg, 20, 86,"&Files" );
		ui_dprintf_at( dlg, 210, 86,"&Dirs" );
		
		ui_dprintf_at( dlg, 20, 60, "%s", b->spaces );
		ui_dprintf_at( dlg, 20, 60, "%s", b->view_dir );
		
		return 1;
	}

	if (GADGET_PRESSED(b->button2))
	{
		PHYSFS_freeList(b->filename_list);	b->filename_list = NULL;
		PHYSFS_freeList(b->directory_list);	b->directory_list = NULL;
		ui_close_dialog(dlg);
		return 1;
	}
	
	if (GADGET_PRESSED(b->help_button))
	{
		ui_messagebox( -1, -1, 1, "Sorry, no help is available!", "Ok" );
		rval = 1;
	}
	
	if (event->type == EVENT_UI_LISTBOX_MOVED)
	{
		if ((ui_event_get_gadget(event) == (UI_GADGET *)b->listbox1) && (b->listbox1->current_item >= 0) && b->filename_list[b->listbox1->current_item])
			ui_inputbox_set_text(b->user_file, b->filename_list[b->listbox1->current_item]);

		if ((ui_event_get_gadget(event) == (UI_GADGET *)b->listbox2) && (b->listbox2->current_item >= 0) && b->directory_list[b->listbox2->current_item])
			ui_inputbox_set_text(b->user_file, b->directory_list[b->listbox2->current_item]);

		rval = 1;
	}
	
	if (GADGET_PRESSED(b->button1) || GADGET_PRESSED(b->user_file) || (event->type == EVENT_UI_LISTBOX_SELECTED))
	{
		char *p;
		
		if (ui_event_get_gadget(event) == (UI_GADGET *)b->listbox2)
			strcpy(b->user_file->text, b->directory_list[b->listbox2->current_item]);
		
		strncpy(b->filename, b->view_dir, PATH_MAX);
		
		p = b->user_file->text;
		while (!strncmp(p, "..", 2))	// shorten the path manually
		{
			char *sep = strrchr(b->filename, '/');
			if (sep)
				*sep = 0;
			else
				*b->filename = 0;	// look directly in search paths
			
			p += 2;
			if (*p == '/')
				p++;
		}
		
		if (*b->filename && *p)
			strncat(b->filename, "/", PATH_MAX - strlen(b->filename));
		strncat(b->filename, p, PATH_MAX - strlen(b->filename));
		
		if (!PHYSFS_isDirectory(b->filename))
		{
			PHYSFS_file	*TempFile;
			
			TempFile = PHYSFS_openRead(b->filename);
			if (TempFile)
			{
				// Looks like a valid filename that already exists!
				PHYSFS_close(TempFile);
				ui_close_dialog(dlg);
				return 1;
			}
			
			// File doesn't exist, but can we create it?
			TempFile = PHYSFS_openWrite(b->filename);
			if (TempFile)
			{
				// Looks like a valid filename!
				PHYSFS_close(TempFile);
				PHYSFS_delete(b->filename);
				ui_close_dialog(dlg);
				return 1;
			}
		}
		else
		{
			if (b->filename[strlen(b->filename) - 1] == '/')	// user typed a separator on the end
				b->filename[strlen(b->filename) - 1] = 0;
			
			strcpy(b->view_dir, b->filename);
			
			
			PHYSFS_freeList(b->filename_list);
			b->filename_list = file_getfilelist(&b->num_files, b->filespec, b->view_dir);
			if (!b->filename_list)
			{
				PHYSFS_freeList(b->directory_list);	b->directory_list = NULL;
				ui_close_dialog(dlg);
				return 1;
			}
			
			ui_inputbox_set_text(b->user_file, b->filespec);
			
			PHYSFS_freeList(b->directory_list);
			b->directory_list = file_getdirlist(&b->num_dirs, b->view_dir);
			if (!b->directory_list)
			{
				PHYSFS_freeList(b->filename_list); b->filename_list = NULL;
				ui_close_dialog(dlg);
				return 1;
			}
			
			ui_listbox_change(dlg, b->listbox1, b->num_files, b->filename_list);
			ui_listbox_change(dlg, b->listbox2, b->num_dirs, b->directory_list);
			
			//i = TICKER;
			//while ( TICKER < i+2 );
			
		}
		
		rval = 1;
	}
	
	return rval;
}

int ui_get_filename( char * filename, char * filespec, char * message  )
{
	char		InputText[PATH_MAX];
	char		*p;
	int			i;
	browser		*b;
	UI_DIALOG	*dlg;
	window		*wind;
	int			rval = 0;

	MALLOC(b, browser, 1);
	if (!b)
		return 0;
	
	if ((p = strrchr(filename, '/')))
	{
		*p++ = 0;
		strcpy(b->view_dir, filename);
		strcpy(InputText, p);
	}
	else
	{
		strcpy(b->view_dir, "");
		strcpy(InputText, filename);
	}

	b->filename_list = file_getfilelist(&b->num_files, filespec, b->view_dir);
	if (!b->filename_list)
	{
		d_free(b);
		return 0;
	}
	
	b->directory_list = file_getdirlist(&b->num_dirs, b->view_dir);
	if (!b->directory_list)
	{
		PHYSFS_freeList(b->filename_list);
		d_free(b);
		return 0;
	}

	//ui_messagebox( -2,-2, 1,"DEBUG:0", "Ok" );
	for (i=0; i<35; i++)
		b->spaces[i] = ' ';
	b->spaces[34] = 0;

	dlg = ui_create_dialog( 200, 100, 400, 370, DF_DIALOG | DF_MODAL, (int (*)(UI_DIALOG *, d_event *, void *))browser_handler, b );

	b->user_file  = ui_add_gadget_inputbox( dlg, 60, 30, PATH_MAX, 40, InputText );

	b->listbox1 = ui_add_gadget_listbox(dlg,  20, 110, 125, 200, b->num_files, b->filename_list);
	b->listbox2 = ui_add_gadget_listbox(dlg, 210, 110, 100, 200, b->num_dirs, b->directory_list);

	b->button1 = ui_add_gadget_button( dlg,     20, 330, 60, 25, "Ok", NULL );
	b->button2 = ui_add_gadget_button( dlg,    100, 330, 60, 25, "Cancel", NULL );
	b->help_button = ui_add_gadget_button( dlg, 180, 330, 60, 25, "Help", NULL );

	dlg->keyboard_focus_gadget = (UI_GADGET *)b->user_file;

	b->button1->hotkey = KEY_CTRLED + KEY_ENTER;
	b->button2->hotkey = KEY_ESC;
	b->help_button->hotkey = KEY_F1;
	b->listbox1->hotkey = KEY_ALTED + KEY_F;
	b->listbox2->hotkey = KEY_ALTED + KEY_D;
	b->user_file->hotkey = KEY_ALTED + KEY_A;

	ui_gadget_calc_keys(dlg);

	b->filename = filename;
	b->filespec = filespec;
	b->message = message;
	
	wind = ui_dialog_get_window(dlg);
	
	while (window_exists(wind))
		event_process();

	//key_flush();

	if (b->filename_list)
		PHYSFS_freeList(b->filename_list);
	if (b->directory_list)
		PHYSFS_freeList(b->directory_list);
	
	rval = b->filename_list != NULL;
	d_free(b);

	return rval;
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

