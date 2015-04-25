#ifndef DXXSCONF_H_SEEN
#define DXXSCONF_H_SEEN

#define __attribute_error(M) /* not supported */
#define DXX_HAVE_BUILTIN_BSWAP
#define DXX_HAVE_BUILTIN_BSWAP16
#define dxx_builtin_constant_p(A) ((void)(A),0)
#define likely(A) __builtin_expect(!!(A), 1)
#define unlikely(A) __builtin_expect(!!(A), 0)
#define DXX_BEGIN_COMPOUND_STATEMENT 
#define DXX_END_COMPOUND_STATEMENT 
#define DXX_ALWAYS_ERROR_FUNCTION(F,S) ( DXX_BEGIN_COMPOUND_STATEMENT {	\
	void F() __attribute_error(S);	\
	F();	\
} DXX_END_COMPOUND_STATEMENT )
#define __attribute_always_inline() __attribute__((__always_inline__))
#define __attribute_alloc_size(A,...) /* not supported */
#define __attribute_cold __attribute__((cold))
#define __attribute_format_arg(A) __attribute__((format_arg(A)))
#define __attribute_format_printf(A,B) __attribute__((format(printf,A,B)))
#define __attribute_malloc() __attribute__((malloc))
#define __attribute_nonnull(...) __attribute__((nonnull __VA_ARGS__))
#define __attribute_used __attribute__((used))
#define __attribute_unused __attribute__((unused))
#define __attribute_warn_unused_result __attribute__((warn_unused_result))
#define DXX_HAVE_CXX_ARRAY
#define DXX_HAVE_CXX11_STATIC_ASSERT
#define DXX_HAVE_CXX11_TYPE_TRAITS
#define DXX_HAVE_TYPE_TRAITS
#define DXX_HAVE_CXX11_RANGE_FOR
#define DXX_HAVE_CXX11_CONSTEXPR
#define dxx_explicit_operator_bool explicit
#define DXX_HAVE_EXPLICIT_OPERATOR_BOOL
#define DXX_CXX11_EXPLICIT_DELETE =delete
#define DXX_HAVE_CXX11_EXPLICIT_DELETE
#define DXX_HAVE_CXX11_BEGIN
#define DXX_HAVE_CXX11_ADDRESSOF
#define DXX_INHERIT_CONSTRUCTORS(D,B,...) typedef B,##__VA_ARGS__ _dxx_constructor_base_type; \
	using _dxx_constructor_base_type::_dxx_constructor_base_type;
#define DXX_HAVE_CXX11_TEMPLATE_ALIAS
#define DXX_HAVE_CXX11_REF_QUALIFIER
#define DXX_HAVE_STRCASECMP

#endif /* DXXSCONF_H_SEEN */
