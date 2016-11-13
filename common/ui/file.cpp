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
COPYRIGHT 1993-1999 PARALLAX SOFTWARE CORPORATION.  ALL RIGHTS RESERVED.
*/

#include <stdlib.h>
#include <string.h>

#include "event.h"
#include "maths.h"
#include "pstypes.h"
#include "gr.h"
#include "key.h"
#include "mouse.h"
#include "strutil.h"
#include "ui.h"
#include "window.h"
#include "u_mem.h"
#include "physfsx.h"
#include "physfs_list.h"

#include "compiler-make_unique.h"

namespace dcx {

static PHYSFSX_counted_list file_getdirlist(const char *dir)
{
	ntstring<PATH_MAX - 1> path;
	auto dlen = path.copy_if(dir);
	if ((!dlen && dir[0] != '\0') || !path.copy_if(dlen, "/"))
		return nullptr;
	++ dlen;
	PHYSFSX_counted_list list{PHYSFS_enumerateFiles(dir)};
	if (!list)
		return nullptr;
	const auto predicate = [&](char *i) -> bool {
		if (path.copy_if(dlen, i) && PHYSFS_isDirectory(path))
			return false;
		free(i);
		return true;
	};
	auto j = std::remove_if(list.begin(), list.end(), predicate);
	*j = NULL;
	auto NumDirs = j.get() - list.get();
	qsort(list.get(), NumDirs, sizeof(char *), string_array_sort_func);
	if (*dir)
	{
		// Put the 'go to parent directory' sequence '..' first
		++NumDirs;
		auto r = reinterpret_cast<char **>(realloc(list.get(), sizeof(char *)*(NumDirs + 1)));
		if (!r)
			return list;
		list.release();
		list.reset(r);
		std::move_backward(r, r + NumDirs, r + NumDirs + 1);
		list[0] = d_strdup("..");
	}
	list.set_count(NumDirs);
	return list;
}

static PHYSFSX_counted_list file_getfilelist(const char *filespec, const char *dir)
{
	PHYSFSX_counted_list list{PHYSFS_enumerateFiles(dir)};
	if (!list)
		return nullptr;

	if (*filespec == '*')
		filespec++;

	const auto predicate = [&](char *i) -> bool {
		auto ext = strrchr(i, '.');
		if (ext && (!d_stricmp(ext, filespec)))
			return false;
		free(i);
		return true;
	};
	auto j = std::remove_if(list.begin(), list.end(), predicate);
	*j = NULL;
	auto NumFiles = j.get() - list.get();
	list.set_count(NumFiles);
	qsort(list.get(), NumFiles, sizeof(char *), string_array_sort_func);
	return list;
}

namespace {

struct ui_file_browser
{
	char		*filename;
	const char		*filespec;
	const char		*message;
	PHYSFSX_counted_list filename_list, directory_list;
	std::unique_ptr<UI_GADGET_BUTTON> button1, button2, help_button;
	std::unique_ptr<UI_GADGET_LISTBOX> listbox1, listbox2;
	std::unique_ptr<UI_GADGET_INPUTBOX> user_file;
	array<char, 35> spaces;
	char		view_dir[PATH_MAX];
};

}

static window_event_result browser_handler(UI_DIALOG *const dlg, const d_event &event, ui_file_browser *const b)
{
	window_event_result rval = window_event_result::ignored;

	if (event.type == EVENT_UI_DIALOG_DRAW)
	{
		ui_dputs_at( dlg, 10, 5, b->message );

		ui_dprintf_at( dlg, 20, 32,"N&ame" );
		ui_dprintf_at( dlg, 20, 86,"&Files" );
		ui_dprintf_at( dlg, 210, 86,"&Dirs" );
		
		ui_dputs_at(dlg, 20, 60, b->spaces.data());
		ui_dputs_at( dlg, 20, 60, b->view_dir );
		
		return window_event_result::handled;
	}

	if (GADGET_PRESSED(b->button2.get()))
	{
		b->filename_list.reset();
		b->directory_list.reset();
		return window_event_result::close;
	}
	
	if (GADGET_PRESSED(b->help_button.get()))
	{
		ui_messagebox( -1, -1, 1, "Sorry, no help is available!", "Ok" );
		rval = window_event_result::handled;
	}
	
	if (event.type == EVENT_UI_LISTBOX_MOVED)
	{
		if ((ui_event_get_gadget(event) == b->listbox1.get()) && (b->listbox1->current_item >= 0) && b->filename_list[b->listbox1->current_item])
			ui_inputbox_set_text(b->user_file.get(), b->filename_list[b->listbox1->current_item]);

		if ((ui_event_get_gadget(event) == b->listbox2.get()) && (b->listbox2->current_item >= 0) && b->directory_list[b->listbox2->current_item])
			ui_inputbox_set_text(b->user_file.get(), b->directory_list[b->listbox2->current_item]);

		rval = window_event_result::handled;
	}
	
	if (GADGET_PRESSED(b->button1.get()) || GADGET_PRESSED(b->user_file.get()) || event.type == EVENT_UI_LISTBOX_SELECTED)
	{
		char *p;
		
		if (ui_event_get_gadget(event) == b->listbox2.get())
			strcpy(b->user_file->text.get(), b->directory_list[b->listbox2->current_item]);
		
		strncpy(b->filename, b->view_dir, PATH_MAX);
		
		p = b->user_file->text.get();
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
			if (RAIIPHYSFS_File{PHYSFS_openRead(b->filename)})
			{
				// Looks like a valid filename that already exists!
				return window_event_result::close;
			}
			
			// File doesn't exist, but can we create it?
			if (RAIIPHYSFS_File TempFile{PHYSFS_openWrite(b->filename)})
			{
				TempFile.reset();
				// Looks like a valid filename!
				PHYSFS_delete(b->filename);
				return window_event_result::close;
			}
		}
		else
		{
			if (b->filename[strlen(b->filename) - 1] == '/')	// user typed a separator on the end
				b->filename[strlen(b->filename) - 1] = 0;
			
			strcpy(b->view_dir, b->filename);
			b->filename_list = file_getfilelist(b->filespec, b->view_dir);
			if (!b->filename_list)
			{
				b->directory_list.reset();
				return window_event_result::close;
			}
			
			ui_inputbox_set_text(b->user_file.get(), b->filespec);
			b->directory_list = file_getdirlist(b->view_dir);
			if (!b->directory_list)
			{
				b->filename_list.reset();
				return window_event_result::close;
			}
			
			ui_listbox_change(dlg, b->listbox1.get(), b->filename_list.get_count(), b->filename_list.get());
			ui_listbox_change(dlg, b->listbox2.get(), b->directory_list.get_count(), b->directory_list.get());
			
			//i = TICKER;
			//while ( TICKER < i+2 );
			
		}
		
		rval = window_event_result::handled;
	}
	
	return rval;
}

int ui_get_filename(char (&filename)[PATH_MAX], const char *const filespec, const char *const message)
{
	char		InputText[PATH_MAX];
	char		*p;
	UI_DIALOG	*dlg;
	window		*wind;
	int			rval = 0;
	auto b = make_unique<ui_file_browser>();
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

	b->filename_list = file_getfilelist(filespec, b->view_dir);
	if (!b->filename_list)
	{
		return 0;
	}
	
	b->directory_list = file_getdirlist(b->view_dir);
	if (!b->directory_list)
	{
		b->filename_list.reset();
		return 0;
	}

	//ui_messagebox( -2,-2, 1,"DEBUG:0", "Ok" );
	for (int i=0; i<35; i++)
		b->spaces[i] = ' ';
	b->spaces[34] = 0;

	dlg = ui_create_dialog( 200, 100, 400, 370, static_cast<dialog_flags>(DF_DIALOG | DF_MODAL), browser_handler, b.get());

	b->user_file  = ui_add_gadget_inputbox<40>(dlg, 60, 30, InputText);

	b->listbox1 = ui_add_gadget_listbox(dlg,  20, 110, 125, 200, b->filename_list.get_count(), b->filename_list.get());
	b->listbox2 = ui_add_gadget_listbox(dlg, 210, 110, 100, 200, b->directory_list.get_count(), b->directory_list.get());

	b->button1 = ui_add_gadget_button( dlg,     20, 330, 60, 25, "Ok", NULL );
	b->button2 = ui_add_gadget_button( dlg,    100, 330, 60, 25, "Cancel", NULL );
	b->help_button = ui_add_gadget_button( dlg, 180, 330, 60, 25, "Help", NULL );

	dlg->keyboard_focus_gadget = b->user_file.get();

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
	
	event_process_all();

	//key_flush();

	rval = static_cast<bool>(b->filename_list);
	b->filename_list.reset();
	b->directory_list.reset();
	return rval;
}

int ui_get_file( char * filename, const char * Filespec  )
{
	int x;
	auto list = file_getfilelist(Filespec, "");
	if (!list) return 0;
	x = MenuX(-1, -1, list.get_count(), list.get());
	if (x > 0)
		strcpy(filename, list[x - 1]);
	return (x > 0);
}

}
