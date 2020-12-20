/*
 * This file is part of the DXX-Rebirth project <https://www.dxx-rebirth.com/>.
 * It is copyright by its individual contributors, as recorded in the
 * project's Git history.  See COPYING.txt at the top level for license
 * terms and a link to the Git history.
 */

#pragma once
#include <cstdio>
#include <inttypes.h>
#include "valptridx.h"
#include <memory>

namespace {
namespace untyped_index_mismatch_exception
{
#ifdef DXX_VALPTRIDX_ENABLE_REPORT_FILENAME
#define REPORT_STANDARD_LEADER_TEXT	"%s:%u: "
#else
#define REPORT_STANDARD_LEADER_TEXT
#endif
#define REPORT_STANDARD_FORMAT	" base=%p size=%lu"
#define REPORT_STANDARD_ARGUMENTS	array_base, array_size
#define REPORT_STANDARD_SIZE	(	\
		sizeof(REPORT_FORMAT_STRING) +	\
		42 /* length of longest filename in `git ls-files` */ +	\
		sizeof("65535") + /* USHORT_MAX for lineno */	\
		sizeof("0x0000000000000000") + /* for pointer from base=%p */	\
		sizeof("18446744073709551615") /* for size from size=%lu */	\
	)
#define REPORT_FORMAT_STRING	REPORT_STANDARD_LEADER_TEXT "pointer/index mismatch:" REPORT_STANDARD_FORMAT " index=%li expected=%p actual=%p"
	static constexpr std::size_t report_buffer_size = REPORT_STANDARD_SIZE + (sizeof("0x0000000000000000") * 2) + sizeof("18446744073709551615");
	__attribute_cold
	__attribute_unused
	static void prepare_report(DXX_VALPTRIDX_REPORT_STANDARD_LEADER_COMMA_R_DEFN_VARS const void *const array_base, const long supplied_index, const void *const expected_pointer, const void *const actual_pointer, char (&buf)[report_buffer_size], const unsigned long array_size)
	{
		snprintf(buf, sizeof(buf), REPORT_FORMAT_STRING, DXX_VALPTRIDX_REPORT_STANDARD_LEADER_COMMA_R_PASS_VARS REPORT_STANDARD_ARGUMENTS, supplied_index, expected_pointer, actual_pointer);
	}
#undef REPORT_FORMAT_STRING
}

namespace untyped_index_range_exception
{
#define REPORT_FORMAT_STRING	REPORT_STANDARD_LEADER_TEXT "invalid index used in array subscript:" REPORT_STANDARD_FORMAT " index=%li"
	static constexpr std::size_t report_buffer_size = REPORT_STANDARD_SIZE + sizeof("18446744073709551615");
	__attribute_cold
	__attribute_unused
	static void prepare_report(DXX_VALPTRIDX_REPORT_STANDARD_LEADER_COMMA_R_DEFN_VARS const void *const array_base, const long supplied_index, char (&buf)[report_buffer_size], const unsigned long array_size)
	{
		snprintf(buf, sizeof(buf), REPORT_FORMAT_STRING, DXX_VALPTRIDX_REPORT_STANDARD_LEADER_COMMA_R_PASS_VARS REPORT_STANDARD_ARGUMENTS, supplied_index);
	}
#undef REPORT_FORMAT_STRING
}

namespace untyped_null_pointer_conversion_exception
{
#define REPORT_FORMAT_STRING	REPORT_STANDARD_LEADER_TEXT "NULL pointer converted"
	static constexpr std::size_t report_buffer_size = REPORT_STANDARD_SIZE;
	__attribute_cold
	__attribute_unused
	static void prepare_report(DXX_VALPTRIDX_REPORT_STANDARD_LEADER_COMMA_R_DEFN_VARS char (&buf)[report_buffer_size])
	{
		snprintf(buf, sizeof(buf), REPORT_FORMAT_STRING DXX_VALPTRIDX_REPORT_STANDARD_LEADER_COMMA_L_PASS_VARS);
	}
#undef REPORT_FORMAT_STRING
}

namespace untyped_null_pointer_exception
{
#define REPORT_FORMAT_STRING	REPORT_STANDARD_LEADER_TEXT "NULL pointer used:" REPORT_STANDARD_FORMAT
	static constexpr std::size_t report_buffer_size = REPORT_STANDARD_SIZE;
	__attribute_cold
	__attribute_unused
	static void prepare_report(DXX_VALPTRIDX_REPORT_STANDARD_LEADER_COMMA_R_DEFN_VARS const void *const array_base, char (&buf)[report_buffer_size], const unsigned long array_size)
	{
		snprintf(buf, sizeof(buf), REPORT_FORMAT_STRING, DXX_VALPTRIDX_REPORT_STANDARD_LEADER_COMMA_R_PASS_VARS REPORT_STANDARD_ARGUMENTS);
	}
#undef REPORT_FORMAT_STRING
#undef REPORT_STANDARD_LEADER_TEXT
#undef REPORT_STANDARD_SIZE
#undef REPORT_STANDARD_ARGUMENTS
#undef REPORT_STANDARD_FORMAT
}
}

template <typename managed_type>
void valptridx<managed_type>::index_mismatch_exception::report(DXX_VALPTRIDX_REPORT_STANDARD_LEADER_COMMA_R_DEFN_VARS const array_managed_type *const array, const index_type supplied_index, const const_pointer_type expected_pointer, const const_pointer_type actual_pointer)
{
	using namespace untyped_index_mismatch_exception;
	char buf[report_buffer_size];
	std::size_t si;
	if constexpr (std::is_enum<index_type>::value)
		si = static_cast<std::size_t>(supplied_index);
	else
		si = supplied_index;
	prepare_report(DXX_VALPTRIDX_REPORT_STANDARD_LEADER_COMMA_R_PASS_VARS array, si, expected_pointer, actual_pointer, buf, array_size);
	throw index_mismatch_exception(buf);
}

template <typename managed_type>
void valptridx<managed_type>::index_range_exception::report(DXX_VALPTRIDX_REPORT_STANDARD_LEADER_COMMA_R_DEFN_VARS const array_managed_type *array, const long supplied_index)
{
	using namespace untyped_index_range_exception;
	char buf[report_buffer_size];
	prepare_report(DXX_VALPTRIDX_REPORT_STANDARD_LEADER_COMMA_R_PASS_VARS array, supplied_index, buf, array_size);
	throw index_range_exception(buf);
}

template <typename managed_type>
void valptridx<managed_type>::null_pointer_exception::report(DXX_VALPTRIDX_REPORT_STANDARD_LEADER_COMMA_N_DEFN_VARS)
{
	using namespace untyped_null_pointer_conversion_exception;
	char buf[report_buffer_size];
	prepare_report(DXX_VALPTRIDX_REPORT_STANDARD_LEADER_COMMA_R_PASS_VARS buf);
	throw null_pointer_exception(buf);
}

template <typename managed_type>
void valptridx<managed_type>::null_pointer_exception::report(DXX_VALPTRIDX_REPORT_STANDARD_LEADER_COMMA_R_DEFN_VARS const array_managed_type *const array)
{
	using namespace untyped_null_pointer_exception;
	char buf[report_buffer_size];
	prepare_report(DXX_VALPTRIDX_REPORT_STANDARD_LEADER_COMMA_R_PASS_VARS array, buf, array_size);
	throw null_pointer_exception(buf);
}
