/*
 * This file is part of the DXX-Rebirth project <https://www.dxx-rebirth.com/>.
 * It is copyright by its individual contributors, as recorded in the
 * project's Git history.  See COPYING.txt at the top level for license
 * terms and a link to the Git history.
 */

#pragma once
#include <memory>
#include <span>

namespace dcx {

/* `unique_span` wraps `unique_ptr<T[]>`, but also remembers the extent of the
 * array `T[]`.  The method `span()` returns a
 * `std::span<T, std::dynamic_extent>` that covers the memory region managed by
 * `unique_span`.  There is no support for a `std::span<T, static_length>`,
 * because that could be more simply handled by
 * `std::unique_ptr<std::array<T, static_length>>`, which would avoid
 * storing the extent in the `unique_span`.
 */
/* Set a default deleter from the underlying unique_ptr, but allow it to be
 * overridden if the caller needs special handling.
 */
template <typename T, typename Deleter = typename std::unique_ptr<T[]>::deleter_type>
class unique_span : std::unique_ptr<T[], Deleter>
{
	using base_type = std::unique_ptr<T[], Deleter>;
	std::size_t extent{};
public:
	constexpr unique_span() = default;
	constexpr unique_span(base_type &&b, const std::size_t e) :
		base_type{std::move(b)},
		extent{e}
	{
	}
	unique_span(const std::size_t e) :
		base_type{new T[e]()},
		extent{e}
	{
	}
	unique_span(unique_span &&) = default;
	unique_span &operator=(unique_span &&) = default;
	using base_type::get;
	/* Require an lvalue input, since the returned pointer is borrowed from
	 * this object.  If the method is called on an rvalue input, then the
	 * unique_ptr would be destroyed and free the memory before the returned
	 * span was destroyed, which would leave the span dangling.
	 */
	[[nodiscard]]
	std::span<T> span() &
	{
		return {get(), extent};
	}
	[[nodiscard]]
	std::span<const T> span() const &
	{
		return {get(), extent};
	}
	std::span<const T> span() const && = delete;
	auto release()
	{
		extent = 0;
		return this->base_type::release();
	}
	std::size_t size() const
	{
		return extent;
	}
};

}
