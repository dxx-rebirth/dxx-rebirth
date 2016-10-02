/*
 * This file is part of the DXX-Rebirth project <http://www.dxx-rebirth.com/>.
 * It is copyright by its individual contributors, as recorded in the
 * project's Git history.  See COPYING.txt at the top level for license
 * terms and a link to the Git history.
 */

#pragma once
#include "dxxsconf.h"

/* This header serves several purposes:
 * - Declare namespace dcx and add it to the list of searched
 *   namespaces.
 * - If the compiler can disambiguate a namespace from a class inside a
 *   searched namespace[1], declare dummy classes scoped so that
 *   attempting to nest a namespace incorrectly causes a redefinition
 *   error.  The build would usually fail even without this forced
 *   check, but the redefinition error forces a clear and immediate
 *   report.
 *
 * If building game-specific code:
 * - #define dsx to a game-specific namespace: d1x or d2x, as
 *   appropriate.
 * - Declare namespace dsx and add it to the list of searched
 *   namespaces.  When the dsx migration is finished, this `using
 *   namespace` statement will be removed.
 * Otherwise:
 * - Declare dummy class dsx to force a redefinition error on attempts
 *   to declare `namespace dsx` in common-only code.
 *
 * [1] gcc handles this in all tested versions.  There are no known
 *	clang versions which handle this.  See the SConstruct test for
 *	DXX_HAVE_CXX_DISAMBIGUATE_USING_NAMESPACE for how this is detected.
 */
#if defined(DXX_BUILD_DESCENT_I) || defined(DXX_BUILD_DESCENT_II)
#	if defined(DXX_BUILD_DESCENT_I)
#		define dsx d1x
#	else
#		define dsx d2x
#	endif
/* The "X declared inside Y" comments are on the same line as their
 * declaration so that, if the compiler reports a redefinition error,
 * the output shows the comment, which explains which type of mistake is
 * being reported.
 */
namespace dsx {	/* Force type mismatch on attempted nesting */
#	ifdef DXX_HAVE_CXX_DISAMBIGUATE_USING_NAMESPACE
	class dcx;	/* dcx declared inside dsx */
	class dsx;	/* dsx declared inside dsx */
#	endif
}
#ifndef DXX_NO_USING_DSX
/* For compatibility during migration, add namespace dsx to the search
 * list.  This conflicts with the long term goal of the dsx project, but
 * is currently required for a successful build.
 *
 * When working on the dsx project, define this preprocessor symbol on a
 * file, then fix everything that breaks with that symbol defined.  Move
 * on to the next file.  When all files build with this symbol set, the
 * symbol and the using statement can be removed.
 */
using namespace dsx;	// deprecated
#endif
#else
/* This dummy class does not need to be guarded by
 * DXX_HAVE_CXX_DISAMBIGUATE_USING_NAMESPACE because it is declared only
 * when no other uses of `dsx` are present, so there is no ambiguity for
 * the compiler to resolve.
 */
class dsx;	/* dsx declared in common-only code */
#endif

#ifdef DXX_HAVE_CXX_DISAMBIGUATE_USING_NAMESPACE
namespace dcx {	/* Force type mismatch on attempted nesting */
	class dcx;	/* dcx declared inside dcx */
	class dsx;	/* dsx declared inside dcx */
}

namespace {
	class dcx;	/* dcx declared inside anonymous */
	class dsx;	/* dsx declared inside anonymous */
}
#else
/* This empty namespace is required because, if
 * !DXX_HAVE_CXX_DISAMBIGUATE_USING_NAMESPACE, then no other declaration
 * of `namespace dcx` has been seen in this file.
 */
namespace dcx {
}
#endif

using namespace dcx;
