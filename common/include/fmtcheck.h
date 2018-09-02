/*
 * This file is part of the DXX-Rebirth project <https://www.dxx-rebirth.com/>.
 * It is copyright by its individual contributors, as recorded in the
 * project's Git history.  See COPYING.txt at the top level for license
 * terms and a link to the Git history.
 */
#pragma once

#include "dxxsconf.h"

#define _dxx_call_puts_parameter2(A,B,...)	B

#define DXX_CALL_PRINTF_UNWRAP_ARGS_(...)	, ## __VA_ARGS__
#define DXX_CALL_PRINTF_DELETE_COMMA2_(A,...)	__VA_ARGS__
#define DXX_CALL_PRINTF_DELETE_COMMA_(A,...)	DXX_CALL_PRINTF_DELETE_COMMA2_( A , ## __VA_ARGS__ )

#ifdef DXX_CONSTANT_TRUE
#define DXX_PRINTF_CHECK_HAS_NONTRIVIAL_FORMAT_STRING_(V,P,FMT)	\
	static_cast<void>(DXX_CONSTANT_TRUE((FMT) && (FMT)[0] == '%' && (FMT)[1] == 's' && (FMT)[2] == 0) &&	\
		(DXX_ALWAYS_ERROR_FUNCTION(dxx_trap_trivial_string_specifier_argument_##V, "bare %s argument to " #V "; use " #P " directly"), 0))
#else
#define DXX_PRINTF_CHECK_HAS_NONTRIVIAL_FORMAT_STRING_(V,P,FMT)	\
	((void)(FMT))
#endif

#define dxx_call_printf_checked(V,P,A,FMT,...)	\
	(	\
		DXX_PRINTF_CHECK_HAS_NONTRIVIAL_FORMAT_STRING_(V, P, FMT),	\
		(sizeof(#__VA_ARGS__) == 1)	?	\
			(P(DXX_CALL_PRINTF_DELETE_COMMA_(1 DXX_CALL_PRINTF_UNWRAP_ARGS_ A, FMT))) :	\
		(V(DXX_CALL_PRINTF_DELETE_COMMA_(1 DXX_CALL_PRINTF_UNWRAP_ARGS_ A, FMT) ,##__VA_ARGS__))	\
	)
