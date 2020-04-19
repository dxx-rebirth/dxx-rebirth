/*
 * This file is part of the DXX-Rebirth project <https://www.dxx-rebirth.com/>.
 * It is copyright by its individual contributors, as recorded in the
 * project's Git history.  See COPYING.txt at the top level for license
 * terms and a link to the Git history.
 */
#pragma once
#include <chrono>

namespace dcx {

#define DXX_MENUITEM_AUTOSAVE_LABEL_INPUT(VERB)	\
	DXX_MENUITEM(VERB, TEXT, "Auto-save every M minutes, S seconds:", opt_label_autosave_interval)	\
	DXX_MENUITEM(VERB, INPUT, AutosaveInterval, opt_autosave_interval)	\

	/* Use a custom duration type with Rep=uint16_t because autosaves
	 * are not meant to have very long intervals.  Even with uint16_t,
	 * this type can allow the user to choose to auto-save once every
	 * 18.2 hours.
	 */
using autosave_interval_type = std::chrono::duration<uint16_t, std::chrono::seconds::period>;

struct d_gameplay_options
{
	autosave_interval_type AutosaveInterval;
};

struct d_sp_gameplay_options : d_gameplay_options
{
};

struct d_mp_gameplay_options : d_gameplay_options
{
};

}
