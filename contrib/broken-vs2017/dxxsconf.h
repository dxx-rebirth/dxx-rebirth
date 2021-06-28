#pragma once

#define __attribute_alloc_size(A,...)
#define __attribute_always_inline()
#define __attribute_cold
#define __attribute_format_printf(A,B)
#define __attribute_format_arg(A)
#define __attribute_malloc()
#define __attribute_nonnull(...)
#define __attribute_unused
#define __attribute_used
#define likely(X)	(X)
#define unlikely(X)	(X)
#define DXX_HAVE_CXX_ARRAY
#define DXX_HAVE_CXX11_ADDRESSOF
#define DXX_HAVE_CXX11_RANGE_FOR
#define DXX_HAVE_CXX11_STATIC_ASSERT
#define DXX_HAVE_CXX11_TYPE_TRAITS
#define DXX_HAVE_CXX11_BEGIN
#define dxx_builtin_constant_p(A)	(false)
#define DXX_INHERIT_CONSTRUCTORS(D,B,...)	\
	template <typename... Args> \
		D(Args&&... args) : \
			B,##__VA_ARGS__(std::forward<Args>(args)...) {}
#define DXX_ptrdiff_cast_int	static_cast<int>
#define DXX_WORDS_BIGENDIAN	0

#define __func__ __FUNCTION__
typedef signed long ssize_t;
