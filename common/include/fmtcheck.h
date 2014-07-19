/*
 * This file is part of the DXX-Rebirth project <http://www.dxx-rebirth.com/>.
 * It is copyright by its individual contributors, as recorded in the
 * project's Git history.  See COPYING.txt at the top level for license
 * terms and a link to the Git history.
 */
#pragma once

#include "dxxsconf.h"

#define _dxx_call_puts_parameter2(A,B,...)	B

#define _dxx_call_printf_unwrap_args(...)	, ## __VA_ARGS__
#define _dxx_call_printf_delete_comma2(A,...)	__VA_ARGS__
#define _dxx_call_printf_delete_comma(A,...)	_dxx_call_printf_delete_comma2( A , ## __VA_ARGS__ )

#ifdef DXX_HAVE_BUILTIN_CONSTANT_P
#define _dxx_printf_check_has_nontrivial_format_string(V,P,FMT)	\
	((void)((dxx_builtin_constant_p((FMT)) && (FMT)[0] == '%' && (FMT)[1] == 's' && (FMT)[2] == 0) &&	\
		(DXX_ALWAYS_ERROR_FUNCTION(dxx_trap_trivial_string_specifier_argument_##V, "bare %s argument to " #V "; use " #P " directly"), 0)))
#else
#define _dxx_printf_check_has_nontrivial_format_string(V,P,FMT)	\
	((void)(FMT))
#endif

#define dxx_call_printf_checked(V,P,A,FMT,...)	\
	(	\
		_dxx_printf_check_has_nontrivial_format_string(V, P, FMT),	\
		(sizeof(#__VA_ARGS__) == 1)	?	\
			(P(_dxx_call_printf_delete_comma(1 _dxx_call_printf_unwrap_args A, FMT))) :	\
		(V(_dxx_call_printf_delete_comma(1 _dxx_call_printf_unwrap_args A, FMT) ,##__VA_ARGS__))	\
	)
