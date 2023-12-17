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

namespace dsx {

template <typename T>
class segment_object_range_t
{
	class iterator;
	class sentinel;
	const iterator b;
public:
	segment_object_range_t(const unique_segment &s, auto &object_factory, auto &segment_factory) :
		b(construct(s, object_factory, segment_factory))
	{
	}
	const iterator &begin() const { return b; }
	static sentinel end() { return {}; }
	static T construct(const unique_segment &s, auto &of, auto &sf)
		requires(
			std::convertible_to<decltype(of(s.objects)), const object_base &> &&
			requires(const object_base o) {
			/* Require that the supplied `sf` can produce a value whose address
			 * can be compared to the address of a `const unique_segment &`.
			 *
			 * In !NDEBUG builds, this is required as a consequence of the
			 * assert conditions.  In NDEBUG builds, it would not be checked,
			 * so an erroneous factory would compile successfully.  Use the
			 * `requires` clause to catch this error when assertions are not
			 * enabled.
			 */
				&*sf(o.segnum) == &s;
			}
		)
		{
			if (s.objects == object_none)
				return T(object_none, static_cast<typename T::allow_none_construction *>(nullptr));
			auto opi = of(s.objects);
#if DXX_SEGITER_DEBUG_OBJECT_LINKAGE
			const object_base &o = opi;
			/* Assert that the first object in the segment claims to be
			 * in the segment that claims to have that object.
			 */
			assert(&*sf(o.segnum) == &s);
			/* Assert that the first object in the segment agrees that there
			 * are no objects before it in the segment.
			 */
			assert(o.prev == object_none);
#endif
			return opi;
		}
};

template <typename T>
class segment_object_range_t<T>::sentinel
{
};

template <typename T>
class segment_object_range_t<T>::iterator :
	T
{
	using T::m_ptr;
	using T::m_idx;
public:
	using iterator_category = std::forward_iterator_tag;
	using value_type = T;
	using difference_type = std::ptrdiff_t;
	using pointer = void;
	using reference = T;
	iterator(T o) :
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
	constexpr bool operator==(const iterator &rhs) const
	{
		return m_idx == rhs.m_idx;
	}
	constexpr bool operator==(const sentinel &) const
	{
		return m_idx == object_none;
	}
};

[[nodiscard]]
static inline auto objects_in(const unique_segment &s, auto &object_factory, auto &segment_factory) -> segment_object_range_t<decltype(object_factory(object_first))>
{
	return {s, object_factory, segment_factory};
}

}
#endif
