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

template <typename T>
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
