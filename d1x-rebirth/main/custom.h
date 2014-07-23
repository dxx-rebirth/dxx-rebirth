/*
 * This file is part of the DXX-Rebirth project <http://www.dxx-rebirth.com/>.
 * It is copyright by its individual contributors, as recorded in the
 * project's Git history.  See COPYING.txt at the top level for license
 * terms and a link to the Git history.
 */
#ifndef _CUSTOM_H
#define _CUSTOM_H

#include "pstypes.h"
#include "piggy.h"

#ifdef __cplusplus

/* from piggy.c */
#define DBM_FLAG_LARGE	128		// Flags added onto the flags struct in b
#define DBM_FLAG_ABM            64

extern int GameBitmapOffset[MAX_BITMAP_FILES];
extern ubyte GameBitmapFlags[MAX_BITMAP_FILES];
extern ubyte * Piggy_bitmap_cache_data;

void load_custom_data(const d_fname &level_file);

void custom_close();

#endif

#endif
