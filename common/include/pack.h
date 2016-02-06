#pragma once

#include "dxxsconf.h"
#include "compiler-type_traits.h"

template <typename T, typename Trc = typename tt::remove_const<T>::type>
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
	exact_type(T *t) : p(t) {}
	// Conversion to the exact type is permitted
	operator const T *() const { return p; }
	operator Trc *() const { return p; }
	bool operator==(const T *rhs) const { return p == rhs; }
};

template <typename T>
class prohibit_void_ptr
{
public:
	// Return a proxy when the address is taken
	exact_type<T> operator&() { return static_cast<T*>(this); }
	exact_type<T const, T> operator&() const { return static_cast<T const*>(this); }
};

struct allow_void_ptr {};

template <typename T>
struct has_prohibit_void_ptr : tt::is_base_of<prohibit_void_ptr<T>, T> {};

template <typename T>
struct has_prohibit_void_ptr<T[]> {};

template <typename T, typename D>
struct inherit_void_ptr_handler : public
	tt::conditional<has_prohibit_void_ptr<T>::value, prohibit_void_ptr<D>, allow_void_ptr> {};
