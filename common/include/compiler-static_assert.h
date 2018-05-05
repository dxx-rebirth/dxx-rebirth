/*
 * This file is part of the DXX-Rebirth project <http://www.dxx-rebirth.com/>.
 * It is copyright by its individual contributors, as recorded in the
 * project's Git history.  See COPYING.txt at the top level for license
 * terms and a link to the Git history.
 */
#pragma once

#include <type_traits>

#define DEFINE_ASSERT_HELPER_CLASS(N,OP,STR)	\
	template <typename T, T L, T R>	\
	class N : public std::integral_constant<bool, (L OP R)>	\
	{	\
		static_assert(L OP R, STR);	\
	}

DEFINE_ASSERT_HELPER_CLASS(assert_equal, ==, "values must be equal");
#define assert_equal(L,R,S) static_assert((assert_equal<decltype((L) + 0), (L) + 0, (R) + 0>::value), S)
