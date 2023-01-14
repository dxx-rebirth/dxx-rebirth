/*
 * This file is part of the DXX-Rebirth project <https://www.dxx-rebirth.com/>.
 * It is copyright by its individual contributors, as recorded in the
 * project's Git history.  See COPYING.txt at the top level for license
 * terms and a link to the Git history.
 */

/*
 * std::ranges is a C++20 feature.
 *
 * gcc-10.1 (released 2020-05-07) shipped std::ranges support.
 *
 * clang-14 (released 2022-03-25) lacks sufficient std::ranges support, and
 * requires a preprocessor symbol to unlock the parts it provides.
 *
 * clang-15 (released 2022-09-06) defines sufficient std::ranges support, but
 * is still incomplete, and still requires a preprocessor symbol to unlock
 * them.
 *
 * clang-14 is currently standard on OS X.
 *
 * When Rebirth is built with clang, this header provides a minimal
 * implementation to allow calling code to use those features, but does not
 * attempt to provide the constraints that a standards-conforming
 * implementation would provide.
 *
 * When Rebirth is built with gcc, this header provides `using` declarations to
 * pull in the compiler's `std::ranges` implementation, which will include the
 * standards-conforming constraints.
 */

#pragma once
#include <algorithm>
#include <ranges>

namespace ranges {

#ifdef __clang__
template <typename R>
	concept range = true;

template <typename R>
	concept borrowed_range = true;

template <typename iterator, typename sentinel = iterator>
class subrange
{
	iterator b;
	sentinel e;
public:
	subrange(iterator b, sentinel e) :
		b(b), e(e)
	{
	}
	subrange(auto &&r) :
		b(std::ranges::begin(r)), e(std::ranges::end(r))
	{
	}
	iterator begin() const { return b; }
	sentinel end() const { return e; }
	auto size() const
	{
		return std::distance(b, e);
	}
	bool empty() const
	{
		return b == e;
	}
};

subrange(auto &r) -> subrange<decltype(std::ranges::begin(r)), decltype(std::ranges::end(r))>;

template <typename IB, typename V>
requires(
	requires(IB b, V v) {
		*b == v;
	}
)
auto find(IB b, auto e, V v)
{
	for (; b != e; ++b)
		if (*b == v)
			break;
	return b;
}

auto find(auto b, auto e, auto v, auto project)
{
	for (; b != e; ++b)
		if (std::invoke(project, *b) == v)
			break;
	return b;
}

auto find(auto &r, auto v)
{
	return find(std::ranges::begin(r), std::ranges::end(r), v);
}

template <typename R, typename P>
requires(
	requires(R &r, P p) {
		std::invoke(p, *std::ranges::begin(r));
	}
)
auto find(R &r, auto v, P project)
{
	return find(std::ranges::begin(r), std::ranges::end(r), v, project);
}

auto find_if(auto b, auto e, auto predicate)
{
	for (; b != e; ++b)
		if (predicate(*b))
			break;
	return b;
}

auto find_if(auto &r, auto predicate)
{
	return find_if(std::ranges::begin(r), std::ranges::end(r), predicate);
}

#else

using std::ranges::range;
using std::ranges::borrowed_range;
using std::ranges::subrange;
using std::ranges::find;
using std::ranges::find_if;

#endif

}

#ifdef __clang__

template <typename iterator, typename sentinel>
inline constexpr bool std::ranges::enable_borrowed_range<ranges::subrange<iterator, sentinel>> = true;

#endif
