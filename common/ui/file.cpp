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

#include "compiler-range_for.h"
#include "d_range.h"
#include <memory>

namespace dcx {

namespace {

#if DXX_USE_EDITOR
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
#endif

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

#if DXX_USE_EDITOR
struct ui_file_browser : UI_DIALOG
{
	char		*filename;
	const char		*filespec;
	const char		*message;
	PHYSFSX_counted_list &filename_list;
	PHYSFSX_counted_list directory_list;
	std::unique_ptr<UI_GADGET_BUTTON> button1, button2, help_button;
	std::unique_ptr<UI_GADGET_LISTBOX> listbox1, listbox2;
	std::unique_ptr<UI_GADGET_INPUTBOX> user_file;
	std::array<char, 35> spaces;
	std::array<char, PATH_MAX> view_dir;
	explicit ui_file_browser(short x, short y, short w, short h, enum dialog_flags flags, const std::array<char, PATH_MAX> &view_dir, PHYSFSX_counted_list &filename, PHYSFSX_counted_list &&directory) :
		UI_DIALOG(x, y, w, h, flags),
		filename_list(filename),
		directory_list(std::move(directory)),
		view_dir(view_dir)
	{
		std::fill(spaces.begin(), std::prev(spaces.end()), ' ');
		spaces.back() = 0;
	}
	virtual window_event_result callback_handler(const d_event &) override;
};

window_event_result ui_file_browser::callback_handler(const d_event &event)
{
	window_event_result rval = window_event_result::ignored;

	if (event.type == EVENT_UI_DIALOG_DRAW)
	{
		ui_dputs_at(this, 10, 5, message);

		ui_dprintf_at(this, 20, 32, "N&ame");
		ui_dprintf_at(this, 20, 86, "&Files");
		ui_dprintf_at(this, 210, 86, "&Dirs");
		
		ui_dputs_at(this, 20, 60, spaces.data());
		ui_dputs_at(this, 20, 60, view_dir.data());
		
		return window_event_result::handled;
	}

	if (GADGET_PRESSED(button2.get()))
	{
		filename_list.reset();
		return window_event_result::close;
	}
	
	if (GADGET_PRESSED(help_button.get()))
	{
		ui_messagebox( -1, -1, 1, "Sorry, no help is available!", "Ok" );
		rval = window_event_result::handled;
	}
	
	if (event.type == EVENT_UI_LISTBOX_MOVED)
	{
		if (&ui_event_get_gadget(event) == listbox1.get() && listbox1->current_item >= 0 && filename_list[listbox1->current_item])
			ui_inputbox_set_text(user_file.get(), filename_list[listbox1->current_item]);

		if (&ui_event_get_gadget(event) == listbox2.get() && listbox2->current_item >= 0 && directory_list[listbox2->current_item])
			ui_inputbox_set_text(user_file.get(), directory_list[listbox2->current_item]);

		rval = window_event_result::handled;
	}
	
	if (GADGET_PRESSED(button1.get()) || GADGET_PRESSED(user_file.get()) || event.type == EVENT_UI_LISTBOX_SELECTED)
	{
		char *p;
		
		if (&ui_event_get_gadget(event) == listbox2.get())
			strcpy(user_file->text.get(), directory_list[listbox2->current_item]);
		
		strncpy(filename, view_dir.data(), PATH_MAX);
		
		p = user_file->text.get();
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
			if (RAIIPHYSFS_File{PHYSFS_openRead(filename)})
			{
				// Looks like a valid filename that already exists!
				return window_event_result::close;
			}
			
			// File doesn't exist, but can we create it?
			if (RAIIPHYSFS_File TempFile{PHYSFS_openWrite(filename)})
			{
				TempFile.reset();
				// Looks like a valid filename!
				PHYSFS_delete(filename);
				return window_event_result::close;
			}
		}
		else
		{
			if (auto &last = filename[strlen(filename) - 1]; last == '/')	// user typed a separator on the end
				last = 0;
			
			strcpy(view_dir.data(), filename);
			filename_list = file_getfilelist(filespec, view_dir.data());
			if (!filename_list)
			{
				return window_event_result::close;
			}
			
			ui_inputbox_set_text(user_file.get(), filespec);
			directory_list = file_getdirlist(view_dir.data());
			if (!directory_list)
			{
				filename_list.reset();
				return window_event_result::close;
			}
			
			ui_listbox_change(this, listbox1.get(), filename_list.get_count(), filename_list.get());
			ui_listbox_change(this, listbox2.get(), directory_list.get_count(), directory_list.get());
		}
		
		rval = window_event_result::handled;
	}
	return rval;
}
#endif

}

#if DXX_USE_EDITOR
int ui_get_filename(std::array<char, PATH_MAX> &filename, const char *const filespec, const char *const message)
{
	std::array<char, PATH_MAX>::iterator InputText;
	std::size_t InputLength;
	int			rval = 0;
	std::array<char, PATH_MAX> view_dir;
	if (const auto p = strrchr(filename.data(), '/'))
	{
		const auto count = std::distance(filename.begin(), p);
		if (count >= view_dir.size())
			/* This should never happen. */
			return 0;
		std::copy(std::begin(filename), p, std::begin(view_dir));
		InputText = std::next(p);
		InputLength = std::distance(InputText, filename.end());
	}
	else
	{
		view_dir.front() = 0;
		InputText = filename.begin();
		InputLength = filename.size();
	}

	PHYSFSX_counted_list filename_list = file_getfilelist(filespec, view_dir.data());
	if (!filename_list)
	{
		return 0;
	}
	
	PHYSFSX_counted_list &&directory_list = file_getdirlist(view_dir.data());
	if (!directory_list)
	{
		return 0;
	}

	//ui_messagebox( -2,-2, 1,"DEBUG:0", "Ok" );

	auto dlg = window_create<ui_file_browser>(200, 100, 400, 370, static_cast<dialog_flags>(DF_DIALOG | DF_MODAL), view_dir, filename_list, std::move(directory_list));

	dlg->user_file = ui_add_gadget_inputbox(*dlg, 60, 30, InputLength, 40, InputText);

	dlg->listbox1 = ui_add_gadget_listbox(*dlg,  20, 110, 125, 200, filename_list.get_count(), filename_list.get());
	dlg->listbox2 = ui_add_gadget_listbox(*dlg, 210, 110, 100, 200, dlg->directory_list.get_count(), dlg->directory_list.get());

	dlg->button1 = ui_add_gadget_button(*dlg,     20, 330, 60, 25, "Ok", nullptr);
	dlg->button2 = ui_add_gadget_button(*dlg,    100, 330, 60, 25, "Cancel", nullptr);
	dlg->help_button = ui_add_gadget_button(*dlg, 180, 330, 60, 25, "Help", nullptr);

	dlg->keyboard_focus_gadget = dlg->user_file.get();

	dlg->button1->hotkey = KEY_CTRLED + KEY_ENTER;
	dlg->button2->hotkey = KEY_ESC;
	dlg->help_button->hotkey = KEY_F1;
	dlg->listbox1->hotkey = KEY_ALTED + KEY_F;
	dlg->listbox2->hotkey = KEY_ALTED + KEY_D;
	dlg->user_file->hotkey = KEY_ALTED + KEY_A;

	ui_gadget_calc_keys(*dlg);

	dlg->filename = filename.data();
	dlg->filespec = filespec;
	dlg->message = message;
	
	event_process_all();
	rval = static_cast<bool>(filename_list);

	//key_flush();
	return rval;
}
#endif

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
