/*
 * This file is part of the DXX-Rebirth project <http://www.dxx-rebirth.com/>.
 * It is copyright by its individual contributors, as recorded in the
 * project's Git history.  See COPYING.txt at the top level for license
 * terms and a link to the Git history.
 */
#pragma once

#include <cstddef>
#include <type_traits>

/* valptridx_specialized_types is never defined, but is specialized to
 * define a typedef for a type-specific class suitable for use as a base
 * of valptridx<T>.
 */
template <typename managed_type>
struct valptridx_specialized_types;

template <typename INTEGRAL_TYPE, std::size_t array_size_value>
class valptridx_specialized_type_parameters
{
public:
	using integral_type = INTEGRAL_TYPE;
	static constexpr std::integral_constant<std::size_t, array_size_value> array_size{};
};

#define DXX_VALPTRIDX_DECLARE_SUBTYPE(MANAGED_TYPE,INTEGRAL_TYPE,ARRAY_SIZE_VALUE)	\
	template <>	\
	struct valptridx_specialized_types<MANAGED_TYPE> {	\
		using type = valptridx_specialized_type_parameters<INTEGRAL_TYPE, ARRAY_SIZE_VALUE>;	\
	}
