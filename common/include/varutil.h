#pragma once

#include <cstddef>
#include <algorithm>
#include "dxxsconf.h"
#include "compiler-array.h"

template <std::size_t N>
class cstring_tie
{
	void init_array(const char *s) __attribute_nonnull()
	{
		p[m_count++] = s;
	}
	template <typename... Args>
		void init_array(const char *s, Args&&... args)
		{
			init_array(s);
			init_array(std::forward<Args>(args)...);
		}
public:
	template <typename... Args>
		cstring_tie(Args&&... args) : m_count(0)
	{
		static_assert(sizeof...(Args) <= maximum_arity, "too many arguments to cstring_tie");
		init_array(std::forward<Args>(args)...);
	}
	unsigned count() const { return m_count; }
	const char *string(std::size_t i) const { return p[i]; }
	enum { maximum_arity = N };
private:
	array<const char *, maximum_arity> p;
	unsigned m_count;
};
