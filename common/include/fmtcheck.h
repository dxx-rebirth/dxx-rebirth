#pragma once

#include "dxxsconf.h"

#if defined(DXX_HAVE_EMBEDDED_COMPOUND_STATEMENT)
#define _dxx_printf_raise_error(N,S)	( \
	({	\
		void N(void) __attribute_error(S);	\
		N();	\
	}), 0)
#else
#define _dxx_printf_raise_error(N,S)	(0)
#endif

#define _dxx_call_printf_unwrap_args(...)	, ## __VA_ARGS__
#define _dxx_call_printf_delete_comma2(A,...)	__VA_ARGS__
#define _dxx_call_printf_delete_comma(A,...)	_dxx_call_printf_delete_comma2( A , ## __VA_ARGS__ )

#define _dxx_printf_check_has_nontrivial_format_string(V,P,FMT)	\
	((void)((dxx_builtin_constant_p((FMT)) && (FMT)[0] == '%' && (FMT)[1] == 's' && (FMT)[2] == 0) &&	\
		_dxx_printf_raise_error(dxx_trap_trivial_string_specifier_argument_##V, "bare %s argument to " #V "; use " #P " directly")))

#define dxx_call_printf_checked(V,P,A,FMT,...)	\
	(	\
		_dxx_printf_check_has_nontrivial_format_string(V, P, FMT),	\
		(sizeof(#__VA_ARGS__) == 1)	?	\
			(P(_dxx_call_printf_delete_comma(1 _dxx_call_printf_unwrap_args A, FMT))) :	\
		(V(_dxx_call_printf_delete_comma(1 _dxx_call_printf_unwrap_args A, FMT) ,##__VA_ARGS__))	\
	)
