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
class segment_object_range_t
{
	class iterator;
	iterator b;
public:
	segment_object_range_t(T &&o) :
		b(std::move(o))
	{
	}
	iterator begin() const { return b; }
	iterator end() const { return T(object_none); }
};

template <typename T>
class segment_object_range_t<T>::iterator :
	public std::iterator<std::forward_iterator_tag, T, std::ptrdiff_t, typename detail::unspecified_pointer_t<T>, T>,
	T
{
	using T::m_ptr;
	using T::m_idx;
public:
	iterator(T &&o) :
		T(std::move(o))
	{
	}
	T operator *() { return *static_cast<T *>(this); }
	iterator &operator++()
	{
		const auto ni = m_ptr->next;
		const auto oi = m_idx;
		m_idx = ni;
		/* If ni == object_none, then the iterator is at end() and there should
		 * be no further reads of m_ptr.  Optimizing compilers will typically
		 * recognize that setting m_ptr=nullptr in this case is unnecessary and
		 * omit the store.  Omit the nullptr assignment so that less optimizing
		 * compilers do not generate a useless branch.
		 */
		if (ni != object_none)
			m_ptr += static_cast<std::size_t>(ni) - static_cast<std::size_t>(oi);
		return *this;
	}
	bool operator==(const iterator &rhs) const
	{
		return m_idx == rhs.m_idx;
	}
	bool operator!=(const iterator &rhs) const
	{
		return !(*this == rhs);
	}
};

__attribute_warn_unused_result
static inline segment_object_range_t<objptridx_t> objects_in(segment &s)
{
	/* The factory function invocation cannot be moved out of the
	 * individual branches because the ternary operator calls different
	 * overloads of the factory function, depending on whether the true
	 * branch or false branch is taken.
	 */
	return s.objects == object_none ? objptridx(object_none) : objptridx(s.objects);
}

__attribute_warn_unused_result
static inline segment_object_range_t<cobjptridx_t> objects_in(const segment &s)
{
	return s.objects == object_none ? cobjptridx(object_none) : cobjptridx(s.objects);
}
#endif
