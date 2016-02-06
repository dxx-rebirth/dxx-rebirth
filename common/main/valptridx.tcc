/*
 * This file is part of the DXX-Rebirth project <http://www.dxx-rebirth.com/>.
 * It is copyright by its individual contributors, as recorded in the
 * project's Git history.  See COPYING.txt at the top level for license
 * terms and a link to the Git history.
 */

#pragma once
#include <cstdio>
#include "valptridx.h"

namespace untyped_index_mismatch_exception
{
#define REPORT_STANDARD_FORMAT	" base=%p size=%lu"
#define REPORT_STANDARD_ARGUMENTS	array_base, static_cast<unsigned long>(array_size)
#define REPORT_STANDARD_SIZE	(sizeof(REPORT_FORMAT_STRING) + sizeof("0x0000000000000000") + sizeof("18446744073709551615"))
#define REPORT_FORMAT_STRING	"pointer/index mismatch:" REPORT_STANDARD_FORMAT " index=%li expected=%p actual=%p"
	static constexpr std::size_t report_buffer_size = REPORT_STANDARD_SIZE + (sizeof("0x0000000000000000") * 2) + sizeof("18446744073709551615");
	__attribute_cold
	static void prepare_report(char (&buf)[report_buffer_size], const void *const array_base, const std::size_t array_size, const long supplied_index, const void *const expected_pointer, const void *const actual_pointer)
	{
		snprintf(buf, sizeof(buf), REPORT_FORMAT_STRING, REPORT_STANDARD_ARGUMENTS, supplied_index, expected_pointer, actual_pointer);
	}
#undef REPORT_FORMAT_STRING
};

namespace untyped_index_range_exception
{
#define REPORT_FORMAT_STRING	"invalid index used in array subscript:" REPORT_STANDARD_FORMAT " index=%li"
	static constexpr std::size_t report_buffer_size = REPORT_STANDARD_SIZE + sizeof("18446744073709551615");
	__attribute_cold
	static void prepare_report(char (&buf)[report_buffer_size], const void *const array_base, const std::size_t array_size, const long supplied_index)
	{
		snprintf(buf, sizeof(buf), REPORT_FORMAT_STRING, REPORT_STANDARD_ARGUMENTS, supplied_index);
	}
#undef REPORT_FORMAT_STRING
};

namespace untyped_null_pointer_exception
{
#define REPORT_FORMAT_STRING	"NULL pointer used:" REPORT_STANDARD_FORMAT
	static constexpr std::size_t report_buffer_size = REPORT_STANDARD_SIZE;
	__attribute_cold
	static void prepare_report(char (&buf)[report_buffer_size], const void *const array_base, const std::size_t array_size)
	{
		snprintf(buf, sizeof(buf), REPORT_FORMAT_STRING, REPORT_STANDARD_ARGUMENTS);
	}
#undef REPORT_FORMAT_STRING
#undef REPORT_STANDARD_SIZE
#undef REPORT_STANDARD_ARGUMENTS
#undef REPORT_STANDARD_FORMAT
};

template <typename managed_type>
void valptridx<managed_type>::index_mismatch_exception::report(const array_managed_type &array, const index_type supplied_index, const const_pointer_type expected_pointer, const const_pointer_type actual_pointer)
{
	using namespace untyped_index_mismatch_exception;
	char buf[report_buffer_size];
	prepare_report(buf, static_cast<const managed_type *>(&array[0]), array.size(), supplied_index, expected_pointer, actual_pointer);
	throw index_mismatch_exception(buf);
}

template <typename managed_type>
void valptridx<managed_type>::index_range_exception::report(const array_managed_type *array, const long supplied_index)
{
	using namespace untyped_index_range_exception;
	char buf[report_buffer_size];
	prepare_report(buf, array, array_size, supplied_index);
	throw index_range_exception(buf);
}

template <typename managed_type>
void valptridx<managed_type>::null_pointer_exception::report()
{
	throw null_pointer_exception("NULL pointer converted");
}

template <typename managed_type>
void valptridx<managed_type>::null_pointer_exception::report(const array_managed_type &array)
{
	using namespace untyped_null_pointer_exception;
	char buf[report_buffer_size];
	prepare_report(buf, static_cast<const managed_type *>(&array[0]), array.size());
	throw null_pointer_exception(buf);
}
