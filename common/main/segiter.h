#pragma once

#include "dsx-ns.h"
#ifdef DXX_BUILD_DESCENT
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

template <typename iterator_value_type>
class objects_in
{
	/* These are never defined, and exist only for use in the decltype expression. */
	static valptridx<object>::vcptridx iterator_base_dispatch(const object_base &);
	static valptridx<object>::vmptridx iterator_base_dispatch(object_base &);
	static valptridx<object>::vcptridx iterator_base_dispatch(valptridx<object>::vcptridx);
	static valptridx<object>::vmptridx iterator_base_dispatch(valptridx<object>::vmptridx);
	using iterator_base_type = decltype(iterator_base_dispatch(std::declval<iterator_value_type &>()));
	class iterator;
	class sentinel
	{
	};
	const iterator b;
public:
	[[nodiscard]]
	objects_in(const unique_segment &s, auto &object_factory, auto &segment_factory) :
		b{construct(s, object_factory, segment_factory)}
	{
	}
	const iterator &begin() const { return b; }
	static sentinel end() { return {}; }
	static iterator_base_type construct(const unique_segment &s, auto &of, auto &sf)
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
				return iterator_base_type{object_none, typename iterator_base_type::allow_none_construction{}};
			iterator_base_type opi{of(s.objects)};
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

template <typename iterator_value_type>
class objects_in<iterator_value_type>::iterator :
	objects_in<iterator_value_type>::iterator_base_type
{
	using iterator_base_type = objects_in<iterator_value_type>::iterator_base_type;
	using iterator_base_type::m_ptr;
	using iterator_base_type::m_idx;
public:
	using iterator_category = std::forward_iterator_tag;
	using value_type = iterator_value_type;
	using difference_type = std::ptrdiff_t;
	using pointer = void;
	/* If the result of `operator*()` is a reference, set `reference` as a type
	 * alias for it.
	 * If the result of `operator*()` is not a reference, set `reference` to
	 * `void` to prevent using algorithms that may behave badly when given a
	 * proxy object.
	 */
	using reference = typename std::conditional_t<std::is_same_v<iterator_base_type, iterator_value_type>, void, value_type &>;
	[[nodiscard]]
	constexpr iterator(iterator_base_type o) :
		iterator_base_type{std::move(o)}
	{
	}
	/* If `std::is_same_v` is true, then return a copy of the underlying
	 * iterator, so that the caller cannot modify the iterator of `this`.  If
	 * `std::is_same_v` is false, then return the result of converting `this`
	 * to `value_type &`, which will typically be `const object &` or
	 * `object &`.
	 */
	[[nodiscard]]
	typename std::conditional_t<std::is_same_v<iterator_base_type, value_type>, value_type, value_type &> operator *() const { return *this; }
	iterator &operator++()
	{
		const auto oi{m_idx};
		const auto op{m_ptr};
		const auto ni{op->next};
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
	[[nodiscard]]
	constexpr bool operator==(const iterator &rhs) const
	{
		return m_idx == rhs.m_idx;
	}
	[[nodiscard]]
	constexpr bool operator==(const sentinel &) const
	{
		return m_idx == object_none;
	}
};

static_assert(std::same_as<decltype(std::declval<objects_in<object>>().begin().operator *()), object &>);
static_assert(std::same_as<decltype(std::declval<objects_in<const object>>().begin().operator *()), const object &>);
static_assert(std::same_as<decltype(std::declval<objects_in<vcobjptridx_t>>().begin().operator *()), vcobjptridx_t>);
static_assert(std::same_as<decltype(std::declval<objects_in<vmobjptridx_t>>().begin().operator *()), vmobjptridx_t>);

objects_in(const unique_segment &s, auto &object_factory, auto & /* segment_factory */) -> objects_in<decltype(object_factory(s.objects))>;

}
#endif
