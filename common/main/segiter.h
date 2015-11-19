#pragma once

#if defined(DXX_BUILD_DESCENT_I) || defined(DXX_BUILD_DESCENT_II)
#include <iterator>
#include "dxxsconf.h"
#include "object.h"
#include "segment.h"

namespace detail
{

template <typename>
struct unspecified_pointer_t;

};

template <typename T>
class segment_object_iterator_t : public std::iterator<std::forward_iterator_tag, T, std::ptrdiff_t, typename detail::unspecified_pointer_t<T>, T>, T
{
	using T::m_ptr;
	using T::m_idx;
public:
	segment_object_iterator_t(T o) :
		T(o)
	{
	}
	T operator *() { return *static_cast<T *>(this); }
	segment_object_iterator_t &operator++()
	{
		const auto ni = m_ptr->next;
		m_ptr = (ni == object_none) ? nullptr : m_ptr + (static_cast<std::size_t>(ni) - static_cast<std::size_t>(m_idx));
		m_idx = ni;
		return *this;
	}
	bool operator==(const segment_object_iterator_t &rhs) const
	{
		return m_idx == rhs.m_idx;
	}
	bool operator!=(const segment_object_iterator_t &rhs) const
	{
		return !(*this == rhs);
	}
};

template <typename T>
struct segment_object_range_t
{
	segment_object_iterator_t<T> b;
	segment_object_range_t(T o) :
		b(o)
	{
	}
	segment_object_iterator_t<T> begin() const { return b; }
	segment_object_iterator_t<T> end() const { return T(object_none); }
};

__attribute_warn_unused_result
static inline segment_object_range_t<objptridx_t> objects_in(segment &s)
{
	return s.objects == object_none ? objptridx(object_none) : objptridx(s.objects);
}

__attribute_warn_unused_result
static inline segment_object_range_t<cobjptridx_t> objects_in(const segment &s)
{
	return s.objects == object_none ? cobjptridx(object_none) : cobjptridx(s.objects);
}
#endif
