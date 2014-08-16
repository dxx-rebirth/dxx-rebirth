#pragma once

#include <memory>

#ifdef DXX_HAVE_CXX14_MAKE_UNIQUE
using std::make_unique;
#else
namespace detail {
	template <typename T>
	struct unique_enable
	{
		typedef std::unique_ptr<T> scalar_type;
	};

	template <typename T>
	struct unique_enable<T[]>
	{
		typedef std::unique_ptr<T[]> vector_type;
		typedef T element_type;
	};

	template <typename T, std::size_t N>
	struct unique_enable<T[N]> {};
}

template <typename T, typename... Args>
static inline typename detail::unique_enable<T>::scalar_type make_unique(Args&&... args)
{
	return typename detail::unique_enable<T>::scalar_type{new T{std::forward<Args>(args)...}};
}

template <typename T>
static inline typename detail::unique_enable<T>::vector_type make_unique(std::size_t N)
{
	return typename detail::unique_enable<T>::vector_type{new typename detail::unique_enable<T>::element_type[N]()};
}
#endif
