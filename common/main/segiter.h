#pragma once

#include "dsx-ns.h"
#ifdef dsx
#include <iterator>
#include <type_traits>
#include "dxxsconf.h"
#include "object.h"
#include "segment.h"

#ifndef DXX_SEGITER_DEBUG_OBJECT_LINKAGE
#	ifdef NDEBUG
#		define DXX_SEGITER_DEBUG_OBJECT_LINKAGE	0
#	else
#		define DXX_SEGITER_DEBUG_OBJECT_LINKAGE	1
#	endif
#endif

#if DXX_SEGITER_DEBUG_OBJECT_LINKAGE
#include <cassert>
#endif

namespace detail
{

template <typename>
struct unspecified_pointer_t;

};

namespace dsx {

template <typename T>
class segment_object_range_t
{
	class iterator;
	const iterator b;
public:
	segment_object_range_t(iterator &&o) :
		b(std::move(o))
	{
	}
	const iterator &begin() const { return b; }
	static iterator end() { return T(object_none, static_cast<typename T::allow_none_construction *>(nullptr)); }
	template <typename OF, typename SF>
		static segment_object_range_t construct(const unique_segment &s, OF &of, SF &sf)
		{
			if (s.objects == object_none)
				return end();
			auto &&opi = of(s.objects);
			const object_base &o = opi;
#if DXX_SEGITER_DEBUG_OBJECT_LINKAGE
			/* Assert that the first object in the segment claims to be
			 * in the segment that claims to have that object.
			 */
			assert(&*sf(o.segnum) == &s);
			/* Assert that the first object in the segment agrees that there
			 * are no objects before it in the segment.
			 */
			assert(o.prev == object_none);
#endif
			/* Check that the supplied SF can produce `const unique_segment &`.
			 * In !NDEBUG builds, this would be checked as a side effect of the
			 * assert conditions.  In NDEBUG builds, it would not be checked.
			 *
			 * Wrap the expression in a sizeof to prevent the compiler from
			 * emitting code to implement the test.
			 */
			static_cast<void>(sizeof(&*sf(o.segnum) == &s));
			return iterator(std::move(opi));
		}
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
	T operator *() const { return *this; }
	iterator &operator++()
	{
		const auto oi = m_idx;
		const auto op = m_ptr;
		const auto ni = op->next;
		m_idx = ni;
		/* If ni == object_none, then the iterator is at end() and there should
		 * be no further reads of m_ptr.  Optimizing compilers will typically
		 * recognize that setting m_ptr=nullptr in this case is unnecessary and
		 * omit the store.  Omit the nullptr assignment so that less optimizing
		 * compilers do not generate a useless branch.
		 */
		m_ptr += static_cast<std::size_t>(ni) - static_cast<std::size_t>(oi);
		if (ni != object_none)
		{
			/* If ni == object_none, then `m_ptr` is now invalid and these
			 * tests would be undefined.
			 */
#if DXX_SEGITER_DEBUG_OBJECT_LINKAGE
			/* Assert that the next object in the segment agrees that
			 * the preceding object is the previous object.
			 */
			assert(oi == m_ptr->prev);
			/* Assert that the old object and new object are in the same
			 * segment.
			 */
			assert(op->segnum == m_ptr->segnum);
#endif
		}
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

template <typename OF, typename SF, typename R = segment_object_range_t<decltype(std::declval<OF &>()(object_first))>>
__attribute_warn_unused_result
static inline R objects_in(const unique_segment &s, OF &of, SF &sf)
{
	return R::construct(s, of, sf);
}

}
#endif
