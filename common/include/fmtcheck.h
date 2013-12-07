#pragma once

#include "dxxsconf.h"

#define _dxx_call_printf_unwrap_args(...)	, ## __VA_ARGS__
#define _dxx_call_printf_delete_comma2(A,...)	__VA_ARGS__
#define _dxx_call_printf_delete_comma(A,...)	_dxx_call_printf_delete_comma2( A , ## __VA_ARGS__ )

#define dxx_call_printf_checked(V,P,A,FMT,...)	\
	(	\
		(sizeof(#__VA_ARGS__) == 1)	?	\
			(P(_dxx_call_printf_delete_comma(1 _dxx_call_printf_unwrap_args A, FMT))) :	\
		(V(_dxx_call_printf_delete_comma(1 _dxx_call_printf_unwrap_args A, FMT) ,##__VA_ARGS__))	\
	)
