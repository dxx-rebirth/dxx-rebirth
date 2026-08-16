#pragma once

#include <memory>
#include <type_traits>

template <typename T>
class exact_type
{
	T *p;
public:
	operator bool() const = delete;
	// Conversion to void* variants is prohibited
	operator void *() const = delete;
	operator volatile void *() const = delete;
	operator const void *() const = delete;
	operator const volatile void *() const = delete;
	bool operator<(auto &&) const = delete;
	bool operator<=(auto &&) const = delete;
	bool operator>(auto &&) const = delete;
	bool operator>=(auto &&) const = delete;
	constexpr exact_type(T *t) : p(t) {}
	// Conversion to the exact type is permitted
	[[nodiscard]]
	constexpr operator const T *() const { return p; }
	[[nodiscard]]
	constexpr operator typename std::remove_const<T>::type *() const { return p; }
	[[nodiscard]]
	constexpr bool operator==(const T *rhs) const { return p == rhs; }
	[[nodiscard]]
	constexpr bool operator==(const exact_type<T> &rhs) const = default;
};

/* In some cases, this class may be inherited and also be the base class of the
 * first member.  The Empty Base Optimization is not allowed when multiple
 * instances of the same type would overlap.  Provide a dummy template
 * parameter so that the base class and first member can be distinct types,
 * allowing EBO to apply.  Assign a default since most users of this class do
 * not overlay two instances of `prohibit_void_ptr`.  For those cases,
 * `prohibit_void_ptr<void>` is sufficient, and using the default reduces the
 * number of equivalent specializations.
 */
template <typename /* empty_base_tag */ = void>
class prohibit_void_ptr
{
public:
	// Return a proxy when the address is taken
	[[nodiscard]]
	constexpr auto operator&(this auto &self)
	{
		return exact_type{std::addressof(self)};
	}
};
