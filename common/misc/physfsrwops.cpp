/*
 * This file is part of the DXX-Rebirth project <https://github.com/dxx-rebirth/dxx-rebirth/>.
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

#include <stdio.h>  /* used for SEEK_SET, SEEK_CUR, SEEK_END ... */
#include <limits>
#include "physfsrwops.h"
#include "physfsx.h"

#if SDL_MAJOR_VERSION == 1
#define SDL_RWops_callback_seek_position	int
#define SDL_RWops_callback_read_position	int
#define SDL_RWops_callback_write_position	int
#else
#define SDL_RWops_callback_seek_position	Sint64
#define SDL_RWops_callback_read_position	size_t
#define SDL_RWops_callback_write_position	size_t
#endif

namespace {

#if SDL_MAJOR_VERSION == 2
static Sint64 physfsrwops_size(SDL_RWops *rw)
{
    PHYSFS_File *handle = reinterpret_cast<PHYSFS_File *>(rw->hidden.unknown.data1);
	const auto len{PHYSFS_fileLength(handle)};
	if (len == -1)
		SDL_SetError("Can't find end of file: %s", PHYSFS_getLastError());
	return len;
} /* physfsrwops_size */
#endif

static SDL_RWops_callback_seek_position physfsrwops_seek(SDL_RWops *rw, const SDL_RWops_callback_seek_position offset, const int whence)
{
    PHYSFS_File *handle = reinterpret_cast<PHYSFS_File *>(rw->hidden.unknown.data1);
	SDL_RWops_callback_seek_position pos;

    if (whence == SEEK_SET)
    {
        pos = offset;
    } /* if */

    else if (whence == SEEK_CUR)
    {
        PHYSFS_sint64 current = PHYSFS_tell(handle);
        if (current == -1)
        {
			[[unlikely]];
            SDL_SetError("Can't find position in file: %s",
                          PHYSFS_getLastError());
            return(-1);
        } /* if */

		if (!std::in_range<SDL_RWops_callback_seek_position>(current))
        {
			[[unlikely]];
            SDL_SetError("Can't fit current file position in an int!");
            return(-1);
        } /* if */
        if (offset == 0)  /* this is a "tell" call. We're done. */
			return current;

		/* When `SDL_RWops_callback_seek_position` is `int`, this assignment
		 * converts to a narrower type, but the call to std::in_range above
		 * rejected any values for which the narrowing would change the
		 * observed value.  An assignment which prohibits narrowing would be
		 * ill-formed, since the compile-time check for narrowing is
		 * context-free and assumes the worst case.  Therefore, an
		 * initialization that prohibited narrowing would trigger a compile
		 * error.
		 *
		 * When `SDL_RWops_callback_seek_position` is `Sint64`, then on
		 * x86_64-w64-mingw32,
		 * `static_cast<SDL_RWops_callback_seek_position>(v)` triggers
		 * `-Wuseless-cast`.
		 *
		 * When `SDL_RWops_callback_seek_position` is `Sint64`, then on
		 * x86_64-pc-linux-gnu,
		 * `static_cast<SDL_RWops_callback_seek_position>(v)` is accepted
		 * without issue.
		 */
		const SDL_RWops_callback_seek_position rwcurrent = current;
		pos = rwcurrent + offset;
    } /* else if */

    else if (whence == SEEK_END)
    {
        PHYSFS_sint64 len = PHYSFS_fileLength(handle);
        if (len == -1)
        {
			[[unlikely]];
            SDL_SetError("Can't find end of file: %s", PHYSFS_getLastError());
            return(-1);
        } /* if */

		if (!std::in_range<SDL_RWops_callback_seek_position>(len))
        {
			[[unlikely]];
            SDL_SetError("Can't fit end-of-file position in an int!");
            return(-1);
        } /* if */

		const SDL_RWops_callback_seek_position rwlen = len;
		pos = rwlen + offset;
    } /* else if */

    else
    {
		[[unlikely]];
        SDL_SetError("Invalid 'whence' parameter.");
        return(-1);
    } /* else */

    if ( pos < 0 )
    {
		[[unlikely]];
        SDL_SetError("Attempt to seek past start of file.");
        return(-1);
    } /* if */
    
    if (!PHYSFS_seek(handle, static_cast<PHYSFS_uint64>(pos)))
    {
		[[unlikely]];
        SDL_SetError("PhysicsFS error: %s", PHYSFS_getLastError());
        return(-1);
    } /* if */

    return(pos);
} /* physfsrwops_seek */

template <typename T>
static bool physfsrwops_calculate_byte_count(const T size, const T number, PHYSFS_uint64 &count)
{
	if (!size || !number)
	{
		count = 0;
		return true;
	}
	const auto object_size{static_cast<PHYSFS_uint64>(size)};
	const auto object_count{static_cast<PHYSFS_uint64>(number)};
	constexpr auto maximum_count{static_cast<PHYSFS_uint64>(std::numeric_limits<PHYSFS_sint64>::max())};
	if (object_size > maximum_count || object_count > maximum_count / object_size)
	{
		SDL_SetError("I/O request is too large.");
		return false;
	}
	count = object_size * object_count;
	return true;
}

static SDL_RWops_callback_read_position physfsrwops_read(SDL_RWops *const rw, void *const ptr, const SDL_RWops_callback_read_position size, const SDL_RWops_callback_read_position maxnum)
{
    PHYSFS_File *handle = reinterpret_cast<PHYSFS_File *>(rw->hidden.unknown.data1);
	PHYSFS_uint64 count;
	if (!physfsrwops_calculate_byte_count(size, maxnum, count) || !count)
		return 0;
	const auto rc{PHYSFS_readBytes(handle, ptr, count)};
	if (rc < 0)
	{
		SDL_SetError("PhysicsFS error: %s", PHYSFS_getLastError());
		return 0;
	}
	if (rc != static_cast<PHYSFS_sint64>(count))
    {
		[[unlikely]];
        if (!PHYSFS_eof(handle)) /* not EOF? Must be an error. */
		{
			[[unlikely]];
            SDL_SetError("PhysicsFS error: %s", PHYSFS_getLastError());
		}
    } /* if */
	return rc / size;
} /* physfsrwops_read */


static SDL_RWops_callback_write_position physfsrwops_write(SDL_RWops *const rw, const void *const ptr, const SDL_RWops_callback_write_position size, const SDL_RWops_callback_write_position num)
{
    PHYSFS_File *handle = reinterpret_cast<PHYSFS_File *>(rw->hidden.unknown.data1);
	PHYSFS_uint64 count;
	if (!physfsrwops_calculate_byte_count(size, num, count) || !count)
		return 0;
	const auto rc{PHYSFS_writeBytes(handle, reinterpret_cast<const uint8_t *>(ptr), count)};
	if (rc < 0)
	{
		[[unlikely]];
		SDL_SetError("PhysicsFS error: %s", PHYSFS_getLastError());
		return 0;
	}
	if (rc != static_cast<PHYSFS_sint64>(count))
	{
		[[unlikely]];
        SDL_SetError("PhysicsFS error: %s", PHYSFS_getLastError());
	}
	return rc / size;
} /* physfsrwops_write */


static int physfsrwops_close(SDL_RWops *rw)
{
    PHYSFS_File *handle = reinterpret_cast<PHYSFS_File *>(rw->hidden.unknown.data1);
    if (!PHYSFS_close(handle))
    {
		[[unlikely]];
        SDL_SetError("PhysicsFS error: %s", PHYSFS_getLastError());
        return(-1);
    } /* if */

    SDL_FreeRW(rw);
    return(0);
} /* physfsrwops_close */

}

std::pair<RWops_ptr, PHYSFS_ErrorCode> PHYSFSRWOPS_openRead(const char *fname)
{
	RAIIPHYSFS_File handle{PHYSFS_openRead(fname)};
    if (!handle)
	{
		[[unlikely]];
		const auto err = PHYSFS_getLastErrorCode();
        SDL_SetError("PhysicsFS error: %s", PHYSFS_getErrorByCode(err));
		return {nullptr, err};
	}
    else
    {
		RWops_ptr retval{SDL_AllocRW()};
		if (retval)
        {
			[[likely]];
#if SDL_MAJOR_VERSION == 2
            retval->size  = physfsrwops_size;
#endif
            retval->seek  = physfsrwops_seek;
            retval->read  = physfsrwops_read;
            retval->write = physfsrwops_write;
            retval->close = physfsrwops_close;
            retval->hidden.unknown.data1 = handle.release();
        } /* if */
		else
			return {nullptr, PHYSFS_ERR_OTHER_ERROR};
		return {std::move(retval), PHYSFS_ERR_OK};
    } /* else */
} /* PHYSFSRWOPS_openRead */

/* end of physfsrwops.c ... */
