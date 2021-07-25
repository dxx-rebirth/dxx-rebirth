/*
 * This file is part of the DXX-Rebirth project <https://www.dxx-rebirth.com/>.
 * It is copyright by its individual contributors, as recorded in the
 * project's Git history.  See COPYING.txt at the top level for license
 * terms and a link to the Git history.
 */
/*
 * This code provides a glue layer between PhysicsFS and Simple Directmedia
 *  Layer's (SDL) RWops i/o abstraction.
 *
 * License: this code is public domain. I make no warranty that it is useful,
 *  correct, harmless, or environmentally safe.
 *
 * This particular file may be used however you like, including copying it
 *  verbatim into a closed-source project, exploiting it commercially, and
 *  removing any trace of my name from the source (although I hope you won't
 *  do that). I welcome enhancements and corrections to this file, but I do
 *  not require you to send me patches if you make changes. This code has
 *  NO WARRANTY.
 *
 * Unless otherwise stated, the rest of PhysicsFS falls under the zlib license.
 *  Please see LICENSE in the root of the source tree.
 *
 * SDL falls under the LGPL license. You can get SDL at https://www.libsdl.org/
 *
 *  This file was written by Ryan C. Gordon. (icculus@clutteredmind.org).
 */

#pragma once

#if 1	//!(defined(__APPLE__) && defined(__MACH__))
#include <physfs.h>
#else
#include <physfs/physfs.h>
#endif
#include <SDL.h>

#ifdef __cplusplus
#include <memory>
#include <utility>

struct RWops_delete
{
	void operator()(SDL_RWops *o) const
	{
		SDL_RWclose(o);
	}
};

typedef std::unique_ptr<SDL_RWops, RWops_delete> RWops_ptr;

/**
 * Open a platform-independent filename for reading, and make it accessible
 *  via an SDL_RWops structure. The file will be closed in PhysicsFS when the
 *  RWops is closed. PhysicsFS should be configured to your liking before
 *  opening files through this method.
 *
 *   @param filename File to open in platform-independent notation.
 *  @return A valid SDL_RWops structure on success, NULL on error. Specifics
 *           of the error can be gleaned from PHYSFS_getLastError().
 */
std::pair<RWops_ptr, PHYSFS_ErrorCode> PHYSFSRWOPS_openRead(const char *fname);
std::pair<RWops_ptr, PHYSFS_ErrorCode> PHYSFSRWOPS_openReadBuffered(const char *fname, PHYSFS_uint64);

#endif

/* end of physfsrwops.h ... */

