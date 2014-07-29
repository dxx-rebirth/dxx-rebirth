#pragma once

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
	using T::p;
	using T::i;
public:
	segment_object_iterator_t(T o) :
		T(o)
	{
	}
	T operator *() { return *static_cast<T *>(this); }
	segment_object_iterator_t &operator++()
	{
		auto ni = p->next;
		p = (ni == object_none) ? NULL : p + (static_cast<std::size_t>(ni) - static_cast<std::size_t>(i));
		i = ni;
		return *this;
	}
	bool operator==(const segment_object_iterator_t &rhs) const
	{
		return i == rhs.i;
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

static inline segment_object_range_t<objptridx_t> objects_in(segment &s) __attribute_warn_unused_result;
static inline segment_object_range_t<objptridx_t> objects_in(segment &s)
{
	return s.objects == object_none ? objptridx_t(object_none) : objptridx_t(s.objects);
}
