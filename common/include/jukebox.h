/*
 * This file is part of the DXX-Rebirth project <https://github.com/dxx-rebirth/dxx-rebirth/>.
 * It is copyright by its individual contributors, as recorded in the
 * project's Git history.  See COPYING.txt at the top level for license
 * terms and a link to the Git history.
 */

#pragma once

#include "physfsx.h"

namespace dcx {

enum class level_music_request;

extern const std::array<file_extension_t, 5> jukebox_exts;

void jukebox_unload();
void jukebox_load();
int jukebox_play();
void jukebox_select_track(level_music_request request);

}
