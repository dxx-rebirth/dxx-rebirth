/*
 * This file is part of the DXX-Rebirth project <http://www.dxx-rebirth.com/>.
 * It is copyright by its individual contributors, as recorded in the
 * project's Git history.  See COPYING.txt at the top level for license
 * terms and a link to the Git history.
 */
#pragma once

#include <type_traits>

#if defined(DXX_HAVE_CXX11_STATIC_ASSERT)
/* native */
#define static_assert(C,M)	static_assert(C,M)
#elif defined(DXX_HAVE_BOOST_STATIC_ASSERT)
#include <boost/static_assert.hpp>
#define static_assert(C,M)	BOOST_STATIC_ASSERT_MSG((C),M)
#else
#define DXX_C_STATIC_ASSERT_PASTE2(A,B,C)	A##_##B##_##C
#define DXX_C_STATIC_ASSERT_PASTE(A,B,C)	DXX_C_STATIC_ASSERT_PASTE2(A,B,C)
#ifdef DXX_HAVE_C_TYPEDEF_STATIC_ASSERT
#define static_assert(C,M)	enum { DXX_C_STATIC_ASSERT_PASTE(static_assertion_check, __LINE__, __COUNTER__) = sizeof(char[(C) ? 1 : -1]) }
#else
#define static_assert(C,M)	enum { DXX_C_STATIC_ASSERT_PASTE(static_assertion_disabled, __LINE__, __COUNTER__) = sizeof((C)) }
#endif
#endif

#define DEFINE_ASSERT_HELPER_CLASS(N,OP,STR)	\
	template <typename T, T L, T R>	\
	class N : public std::integral_constant<bool, (L OP R)>	\
	{	\
		static_assert(L OP R, STR);	\
	}

DEFINE_ASSERT_HELPER_CLASS(assert_equal, ==, "values must be equal");
#define assert_equal(L,R,S) static_assert((assert_equal<decltype((L) + 0), (L) + 0, (R) + 0>::value), S)
