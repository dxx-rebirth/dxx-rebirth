/*
 * This file is part of the DXX-Rebirth project <https://www.dxx-rebirth.com/>.
 * It is copyright by its individual contributors, as recorded in the
 * project's Git history.  See COPYING.txt at the top level for license
 * terms and a link to the Git history.
 */
/** \file ignorecase.c */

#include <cstddef>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <optional>

#include "physfsx.h"
#include "physfs_list.h"
#include "ignorecase.h"

/**
 * Please see ignorecase.h for details.
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
 *  \author Ryan C. Gordon.
 */

namespace dcx {

namespace {

/* I'm not screwing around with stricmp vs. strcasecmp... */
static std::optional<std::size_t> caseInsensitiveStringCompare(const char *x, const char *y)
{
	const auto sx = x;
	for (;;)
    {
		const auto ux = toupper(static_cast<unsigned>(*x));
		const auto uy = toupper(static_cast<unsigned>(*y));
        if (ux != uy)
			return std::nullopt;
		if (!ux)
			return std::distance(sx, x);
        x++;
        y++;
	}
} /* caseInsensitiveStringCompare */

static PHYSFSX_case_search_result locateOneElement(char *const sptr, char *const ptr, const char *buf)
{
    if (const auto r = PHYSFS_exists(buf))
        return PHYSFSX_case_search_result::success;  /* quick rejection: exists in current case. */

	struct search_result : PHYSFSX_uncounted_list
	{
		search_result(char *ptr, const char *buf) :
			PHYSFSX_uncounted_list(PHYSFS_enumerateFiles(ptr ? (*ptr = 0, buf) : "/"))
		{
			if (ptr)
				*ptr = '/';
		}
	};
	const search_result s{ptr, buf};
	if (!s)
		/* Failed to list the directory.  No names are available, so there is
		 * no list to iterate.
		 */
		return PHYSFSX_case_search_result::file_missing;
	for (const auto i : s)
    {
		if (const auto o = caseInsensitiveStringCompare(i, sptr))
        {
			std::memcpy(sptr, i, *o); /* found a match. Overwrite with this case. */
            return PHYSFSX_case_search_result::success;
        } /* if */
    } /* for */

    /* no match at all... */
    return PHYSFSX_case_search_result::file_missing;
} /* locateOneElement */

}

PHYSFSX_case_search_result PHYSFSEXT_locateCorrectCase(char *buf)
{
    while (*buf == '/')  /* skip any '/' at start of string... */
        buf++;
	if (!*buf)
        return PHYSFSX_case_search_result::success;  /* Uh...I guess that's success. */

	char *prevptr = nullptr;
	const auto step{[&prevptr, buf]{
		return locateOneElement(prevptr ? prevptr + 1 : buf, prevptr, buf);
	}};
	for (auto ptr = buf; (ptr = strchr(ptr + 1, '/')); prevptr = ptr)
    {
        *ptr = '\0';  /* block this path section off */
		const auto rc{step()};
        *ptr = '/'; /* restore path separator */
        if (rc != PHYSFSX_case_search_result::success)
            return PHYSFSX_case_search_result::directory_missing;  /* missing element in path. */
    } /* while */

    /* check final element... */
    return step();
} /* PHYSFSEXT_locateCorrectCase */

}

#ifdef TEST_PHYSFSEXT_LOCATECORRECTCASE
#define con_printf(A,B,...)	printf(B "\n", ##__VA_ARGS__)
int main(int argc, char **argv)
{
    PHYSFSX_case_search_result rc;
    char buf[128];
    PHYSFS_File *f;

    if (!PHYSFS_init(argv[0]))
    {
        con_printf(CON_CRITICAL, "PHYSFS_init(): %s", PHYSFS_getLastError());
        return(1);
    } /* if */

    if (!PHYSFS_addToSearchPath(".", 1))
    {
        con_printf(CON_CRITICAL, "PHYSFS_addToSearchPath(): %s", PHYSFS_getLastError());
        PHYSFS_deinit();
        return(1);
    } /* if */

    if (!PHYSFS_setWriteDir("."))
    {
        con_printf(CON_CRITICAL, "PHYSFS_setWriteDir(): %s", PHYSFS_getLastError());
        PHYSFS_deinit();
        return(1);
    } /* if */

    if (!PHYSFS_mkdir("/a/b/c"))
    {
        con_printf(CON_CRITICAL, "PHYSFS_mkdir(): %s", PHYSFS_getLastError());
        PHYSFS_deinit();
        return(1);
    } /* if */

    if (!PHYSFS_mkdir("/a/b/C"))
    {
        con_printf(CON_CRITICAL, "PHYSFS_mkdir(): %s", PHYSFS_getLastError());
        PHYSFS_deinit();
        return(1);
    } /* if */

    f = PHYSFS_openWrite("/a/b/c/x.txt");
    PHYSFS_close(f);
    if (f == NULL)
    {
        con_printf(CON_CRITICAL, "PHYSFS_openWrite(): %s", PHYSFS_getLastError());
        PHYSFS_deinit();
        return(1);
    } /* if */

    f = PHYSFS_openWrite("/a/b/C/X.txt");
    PHYSFS_close(f);
    if (f == NULL)
    {
        con_printf(CON_CRITICAL, "PHYSFS_openWrite(): %s", PHYSFS_getLastError());
        PHYSFS_deinit();
        return(1);
    } /* if */

    strcpy(buf, "/a/b/c/x.txt");
    rc = PHYSFSEXT_locateCorrectCase(buf);
    if ((rc != PHYSFSX_case_search_result::success) || (strcmp(buf, "/a/b/c/x.txt") != 0))
        con_printf(CON_DEBUG,"test 1 failed");

    strcpy(buf, "/a/B/c/x.txt");
    rc = PHYSFSEXT_locateCorrectCase(buf);
    if ((rc != PHYSFSX_case_search_result::success) || (strcmp(buf, "/a/b/c/x.txt") != 0))
        con_printf(CON_DEBUG,"test 2 failed");

    strcpy(buf, "/a/b/C/x.txt");
    rc = PHYSFSEXT_locateCorrectCase(buf);
    if ((rc != PHYSFSX_case_search_result::success) || (strcmp(buf, "/a/b/C/X.txt") != 0))
        con_printf(CON_DEBUG,"test 3 failed");

    strcpy(buf, "/a/b/c/X.txt");
    rc = PHYSFSEXT_locateCorrectCase(buf);
    if ((rc != PHYSFSX_case_search_result::success) || (strcmp(buf, "/a/b/c/x.txt") != 0))
        con_printf(CON_DEBUG,"test 4 failed");

    strcpy(buf, "/a/b/c/z.txt");
    rc = PHYSFSEXT_locateCorrectCase(buf);
    if ((rc != PHYSFSX_case_search_result::file_missing) || (strcmp(buf, "/a/b/c/z.txt") != 0))
        con_printf(CON_DEBUG,"test 5 failed");

    strcpy(buf, "/A/B/Z/z.txt");
    rc = PHYSFSEXT_locateCorrectCase(buf);
    if ((rc != PHYSFSX_case_search_result::directory_missing) || (strcmp(buf, "/a/b/Z/z.txt") != 0))
        con_printf(CON_DEBUG,"test 6 failed");

    con_printf(CON_DEBUG,"Testing completed.");
    con_printf(CON_DEBUG,"  If no errors were reported, you're good to go.");

    PHYSFS_delete("/a/b/c/x.txt");
    PHYSFS_delete("/a/b/C/X.txt");
    PHYSFS_delete("/a/b/c");
    PHYSFS_delete("/a/b/C");
    PHYSFS_delete("/a/b");
    PHYSFS_delete("/a");
    PHYSFS_deinit();
    return(0);
} /* main */
#endif

/* end of ignorecase.c ... */

