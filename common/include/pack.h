#pragma once

#include <type_traits>
#include "dxxsconf.h"

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
	template <typename U>
		bool operator<(U &&) const = delete;
	template <typename U>
		bool operator<=(U &&) const = delete;
	template <typename U>
		bool operator>(U &&) const = delete;
	template <typename U>
		bool operator>=(U &&) const = delete;
	template <typename U>
		bool operator!=(U &&rhs) const { return !operator==(static_cast<U &&>(rhs)); }
	constexpr exact_type(T *t) : p(t) {}
	// Conversion to the exact type is permitted
	constexpr operator const T *() const { return p; }
	constexpr operator typename std::remove_const<T>::type *() const { return p; }
	constexpr bool operator==(const T *rhs) const { return p == rhs; }
	constexpr bool operator==(const exact_type<T> &rhs) const { return p == rhs.p; }
};

template <typename T>
class prohibit_void_ptr
{
public:
	// Return a proxy when the address is taken
	constexpr exact_type<T> operator&() { return static_cast<T*>(this); }
	constexpr exact_type<T const> operator&() const { return static_cast<T const*>(this); }
};
