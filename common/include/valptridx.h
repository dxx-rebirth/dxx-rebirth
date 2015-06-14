/*
 * This file is part of the DXX-Rebirth project <http://www.dxx-rebirth.com/>.
 * It is copyright by its individual contributors, as recorded in the
 * project's Git history.  See COPYING.txt at the top level for license
 * terms and a link to the Git history.
 */
#pragma once

#include <cstddef>
#include <stdexcept>
#include <string>
#include "dxxsconf.h"
#include "compiler-type_traits.h"
#include "pack.h"

#ifdef DXX_HAVE_BUILTIN_CONSTANT_P
#define DXX_VALPTRIDX_STATIC_CHECK(SUCCESS_CONDITION,FAILURE_FUNCTION,FAILURE_STRING)	\
	(	\
		static_cast<void>(dxx_builtin_constant_p(!(SUCCESS_CONDITION)) && !(SUCCESS_CONDITION) &&	\
			(DXX_ALWAYS_ERROR_FUNCTION(FAILURE_FUNCTION, FAILURE_STRING), 0)	\
		)	\
	)
#else
#define DXX_VALPTRIDX_STATIC_CHECK(E,F,S)	\
	((void)0)
#endif

#ifdef DXX_HAVE_CXX11_REF_QUALIFIER
#define DXX_VALPTRIDX_REF_QUALIFIER_LVALUE &
#else
#define DXX_VALPTRIDX_REF_QUALIFIER_LVALUE
#endif

#define DXX_VALPTRIDX_CHECK(SUCCESS_CONDITION,EXCEPTION,FAILURE_STRING,...)	\
	(	\
		static_cast<void>(DXX_VALPTRIDX_STATIC_CHECK((SUCCESS_CONDITION), dxx_trap_##EXCEPTION, FAILURE_STRING),	\
			likely((SUCCESS_CONDITION)) || (report_##EXCEPTION(__VA_ARGS__), 0)	\
		)	\
	)

/* Never used with an actual void, but needed for proper two-stage
 * lookup.
 */
template <typename T>
typename tt::enable_if<tt::is_void<T>::value, void>::type get_global_array(T *);

class valptridxutil_untyped_base
{
protected:
#define REPORT_STANDARD_FORMAT	" base=%p size=%lu"
#define REPORT_STANDARD_ARGUMENTS	array_base, array_size
#define REPORT_STANDARD_SIZE	(sizeof(REPORT_FORMAT_STRING) + sizeof("0x0000000000000000") + sizeof("18446744073709551615"))
#define REPORT_FORMAT_STRING	"pointer/index mismatch:" REPORT_STANDARD_FORMAT " index=%li expected=%p actual=%p"
	static constexpr std::size_t index_mismatch_report_buffer_size = REPORT_STANDARD_SIZE + (sizeof("0x0000000000000000") * 2) + sizeof("18446744073709551615");
	__attribute_cold
	static void prepare_report_index_mismatch_exception(char (&buf)[index_mismatch_report_buffer_size], const void *const array_base, const unsigned long array_size, const long supplied_index, const void *const expected_pointer, const void *const actual_pointer)
	{
		snprintf(buf, sizeof(buf), REPORT_FORMAT_STRING, REPORT_STANDARD_ARGUMENTS, supplied_index, expected_pointer, actual_pointer);
	}
#undef REPORT_FORMAT_STRING
#define REPORT_FORMAT_STRING	"invalid index used in array subscript:" REPORT_STANDARD_FORMAT " index=%li"
	static constexpr std::size_t index_range_report_buffer_size = REPORT_STANDARD_SIZE + sizeof("18446744073709551615");
	__attribute_cold
	static void prepare_report_index_range_exception(char (&buf)[index_range_report_buffer_size], const void *const array_base, const unsigned long array_size, const long supplied_index)
	{
		snprintf(buf, sizeof(buf), REPORT_FORMAT_STRING, REPORT_STANDARD_ARGUMENTS, supplied_index);
	}
#undef REPORT_FORMAT_STRING
#define REPORT_FORMAT_STRING	"NULL pointer used:" REPORT_STANDARD_FORMAT
	static constexpr std::size_t null_pointer_report_buffer_size = REPORT_STANDARD_SIZE;
	__attribute_cold
	static void prepare_report_null_pointer_exception(char (&buf)[null_pointer_report_buffer_size], const void *const array_base, const unsigned long array_size)
	{
		snprintf(buf, sizeof(buf), REPORT_FORMAT_STRING, REPORT_STANDARD_ARGUMENTS);
	}
#undef REPORT_FORMAT_STRING
#undef REPORT_STANDARD_SIZE
#undef REPORT_STANDARD_ARGUMENTS
#undef REPORT_STANDARD_FORMAT
};

template <typename P, typename I>
class valbaseptridxutil_t : protected valptridxutil_untyped_base
{
protected:
	using valptridxutil_untyped_base::prepare_report_index_mismatch_exception;
	using valptridxutil_untyped_base::prepare_report_index_range_exception;
	using valptridxutil_untyped_base::prepare_report_null_pointer_exception;
	using valptridxutil_untyped_base::index_mismatch_report_buffer_size;
	using valptridxutil_untyped_base::index_range_report_buffer_size;
	using valptridxutil_untyped_base::null_pointer_report_buffer_size;
	struct null_pointer_exception : std::logic_error {
		DXX_INHERIT_CONSTRUCTORS(null_pointer_exception, logic_error);
	};
	struct index_mismatch_exception : std::logic_error {
		DXX_INHERIT_CONSTRUCTORS(index_mismatch_exception, logic_error);
	};
	struct index_range_exception : std::out_of_range {
		DXX_INHERIT_CONSTRUCTORS(index_range_exception, out_of_range);
	};
	typedef P *pointer_type;
	typedef I index_type;
	typedef typename tt::remove_const<P>::type Prc;
	typedef const P *cpointer_type;
	typedef Prc *mpointer_type;
	static constexpr decltype(get_global_array(pointer_type())) get_array()
	{
		return get_global_array(pointer_type());
	}
	static constexpr bool check_allowed_invalid_index(index_type s)
	{
		return s == static_cast<index_type>(~static_cast<index_type>(0));
	}
	template <typename A>
		static cpointer_type check_null_pointer(const A &, std::nullptr_t) = delete;
	template <typename T>
		static inline __attribute_warn_unused_result T check_null_pointer(T p)
		{
			return check_null_pointer(get_array(), p);
		}
	template <typename A, typename T>
		static inline __attribute_warn_unused_result T check_null_pointer(const A &a, T p)
		{
			DXX_VALPTRIDX_CHECK(p, null_pointer_exception, "NULL pointer used", &a[0], a.size());
			return p;
		}
	template <typename A>
		static inline __attribute_warn_unused_result index_type check_index_match(const A &a, pointer_type p, index_type s)
		{
			const auto as = &a[s];
			DXX_VALPTRIDX_CHECK(as == p, index_mismatch_exception, "pointer/index mismatch", &a[0], a.size(), s, as, p);
			return s;
		}
	template <typename A>
		static inline __attribute_warn_unused_result index_type check_index_range(const A &a, index_type s)
		{
			const std::size_t ss = s;
			const std::size_t as = a.size();
			DXX_VALPTRIDX_CHECK(ss < as, index_range_exception, "invalid index used in array subscript", &a[0], as, ss);
			return s;
		}
	__attribute_cold
	__attribute_noreturn
	static void report_index_mismatch_exception(cpointer_type, std::size_t, index_type, cpointer_type, cpointer_type);
	__attribute_cold
	__attribute_noreturn
	static void report_index_range_exception(cpointer_type, std::size_t, long);
	__attribute_cold
	__attribute_noreturn
	static void report_null_pointer_exception(cpointer_type, std::size_t);
};

/* Out of line to avoid implicit inline on in-class definitions */
template <typename P, typename I>
void valbaseptridxutil_t<P, I>::report_index_mismatch_exception(const cpointer_type array_base, const std::size_t array_size, const index_type supplied_index, const cpointer_type expected_pointer, const cpointer_type actual_pointer)
{
	char buf[index_mismatch_report_buffer_size];
	prepare_report_index_mismatch_exception(buf, array_base, array_size, supplied_index, expected_pointer, actual_pointer);
	throw index_mismatch_exception(buf);
}

template <typename P, typename I>
void valbaseptridxutil_t<P, I>::report_index_range_exception(const cpointer_type array_base, const std::size_t array_size, const long supplied_index)
{
	char buf[index_range_report_buffer_size];
	prepare_report_index_range_exception(buf, array_base, array_size, supplied_index);
	throw index_range_exception(buf);
}

template <typename P, typename I>
void valbaseptridxutil_t<P, I>::report_null_pointer_exception(const cpointer_type array_base, const std::size_t array_size)
{
	char buf[null_pointer_report_buffer_size];
	prepare_report_null_pointer_exception(buf, array_base, array_size);
	throw null_pointer_exception(buf);
}

template <typename P, typename I>
class vvalptr_t;

template <typename P, typename I, template <I> class magic_constant>
class vvalidx_t;

template <typename P, typename I, template <I> class magic_constant>
class validx_t;

template <
	bool require_valid,
	typename P, typename I, template <I> class magic_constant,
	typename Prc = typename tt::remove_const<P>::type>
struct valptridx_template_t;

template <typename P, typename I>
class valptr_t : protected valbaseptridxutil_t<P, I>
{
	typedef valbaseptridxutil_t<P, I> valutil;
protected:
	typedef typename valutil::Prc Prc;
	typedef valptr_t valptr_type;
	using valutil::check_allowed_invalid_index;
	using valutil::check_null_pointer;
	using valutil::check_index_range;
public:
	const P *unchecked_const_pointer() const { return p; }
	/* Indirect through the extra version to get a good compiler
	 * backtrace when misuse happens.
	 */
	Prc *unchecked_mutable_pointer(const P *) const = delete;
	Prc *unchecked_mutable_pointer(Prc *) const { return p; }
	Prc *unchecked_mutable_pointer() const { return unchecked_mutable_pointer(static_cast<P *>(nullptr)); }
	typedef typename valutil::pointer_type pointer_type;
	typedef typename valutil::cpointer_type cpointer_type;
	typedef typename valutil::mpointer_type mpointer_type;
	typedef P &reference;
	valptr_t() = delete;
	valptr_t(I) = delete;
	valptr_t(vvalptr_t<const P, I> &&) = delete;
	valptr_t(vvalptr_t<Prc, I> &&) = delete;
	valptr_t(std::nullptr_t) : p(nullptr) {}
	template <template <I> class magic_constant>
	valptr_t(valptridx_template_t<true, const P, I, magic_constant> v) :
		p(v)
	{
	}
	template <template <I> class magic_constant>
	valptr_t(valptridx_template_t<true, Prc, I, magic_constant> v) :
		p(v)
	{
	}
	template <template <I> class magic_constant>
	valptr_t(valptridx_template_t<false, const P, I, magic_constant> v) :
		p(v)
	{
	}
	template <template <I> class magic_constant>
	valptr_t(valptridx_template_t<false, Prc, I, magic_constant> v) :
		p(v)
	{
	}
	valptr_t(const valptr_t<const P, I> &t) :
		p(t.unchecked_const_pointer())
	{
	}
	valptr_t(const valptr_t<Prc, I> &t) :
		p(t.unchecked_mutable_pointer())
	{
	}
	valptr_t(const vvalptr_t<const P, I> &t);
	valptr_t(const vvalptr_t<Prc, I> &t);
	template <typename A>
		valptr_t(A &, pointer_type p) :
			p(p)
		{
		}
	valptr_t(pointer_type p) :
		p(p)
	{
	}
	pointer_type operator->() const DXX_VALPTRIDX_REF_QUALIFIER_LVALUE { return *this; }
	operator cpointer_type() const DXX_VALPTRIDX_REF_QUALIFIER_LVALUE { return check_null_pointer(unchecked_const_pointer()); }
	operator mpointer_type() const DXX_VALPTRIDX_REF_QUALIFIER_LVALUE { return check_null_pointer(unchecked_mutable_pointer()); }
#ifdef DXX_HAVE_CXX11_REF_QUALIFIER
	pointer_type operator->() const && = delete;
	operator cpointer_type() const && = delete;
	operator mpointer_type() const && = delete;
#endif
	reference operator*() const { return *check_null_pointer(p); }
	/* Only vvalptr_t can implicitly convert to reference */
	operator reference() const = delete;
	explicit operator bool() const { return *this != nullptr; }
	template <typename R>
		bool operator!=(const R &rhs) const
		{
			return !(*this == rhs);
		}
	bool operator==(const valptr_t<Prc, I> &rhs) const { return p == rhs.p; }
	bool operator==(const valptr_t<P const, I> &rhs) const { return p == rhs.p; }
	bool operator==(Prc *rhs) const { return p == rhs; }
	bool operator==(P const *rhs) const { return p == rhs; }
	bool operator==(std::nullptr_t) const { return p == nullptr; }
	template <typename U>
		typename tt::enable_if<!tt::is_base_of<valptr_t, U>::value, bool>::type operator==(U) const = delete;
	template <typename U>
		long operator-(U) const = delete;
	template <typename R>
		bool operator<(R) const = delete;
	template <typename R>
		bool operator>(R) const = delete;
	template <typename R>
		bool operator<=(R) const = delete;
	template <typename R>
		bool operator>=(R) const = delete;
protected:
	template <typename A>
		valptr_t(A &a, I i) :
			p(check_allowed_invalid_index(i) ? nullptr : &a[check_index_range(a, i)])
		{
		}
	pointer_type p;
};

template <typename P, typename I, template <I> class magic_constant>
class validx_t : protected valbaseptridxutil_t<P, I>
{
	typedef valbaseptridxutil_t<P, I> valutil;
protected:
	using valutil::check_allowed_invalid_index;
	using valutil::check_index_match;
	using valutil::check_index_range;
	typedef validx_t validx_type;
public:
	typedef typename valutil::pointer_type pointer_type;
	typedef typename valutil::index_type index_type;
	typedef I integral_type;
	validx_t() = delete;
	validx_t(P) = delete;
	template <integral_type v>
		validx_t(const magic_constant<v> &) :
			i(v)
	{
	}
	validx_t(vvalidx_t<P, I, magic_constant> &&) = delete;
	template <typename A>
		validx_t(A &a, pointer_type p, index_type s) :
			i(check_allowed_invalid_index(s) ? s : check_index_match(a, p, check_index_range(a, s)))
	{
	}
	template <typename A>
		validx_t(A &a, index_type s) :
			i(check_allowed_invalid_index(s) ? s : check_index_range(a, s))
	{
	}
	operator const index_type&() const { return i; }
	template <integral_type v>
		bool operator==(const magic_constant<v> &) const
		{
			return i == v;
		}
	bool operator==(const index_type &rhs) const { return i == rhs; }
	// Compatibility hack since some object numbers are in int, not
	// short
	bool operator==(const int &rhs) const { return i == rhs; }
	template <typename R>
		bool operator!=(const R &rhs) const
		{
			return !(*this == rhs);
		}
	bool operator==(const validx_t &rhs) const
	{
		return i == rhs.i;
	}
	template <typename U>
		typename tt::enable_if<!tt::is_base_of<validx_t, U>::value && !tt::is_base_of<U, validx_t>::value, bool>::type operator==(U) const = delete;
	template <typename R>
		bool operator<(R) const = delete;
	template <typename R>
		bool operator>(R) const = delete;
	template <typename R>
		bool operator<=(R) const = delete;
	template <typename R>
		bool operator>=(R) const = delete;
protected:
	validx_t(index_type s) :
		i(s)
	{
	}
	index_type i;
};

template <bool, typename P, typename I, template <I> class magic_constant>
struct valptridx_dispatch_require_valid
{
	typedef valptr_t<P, I> vptr_type;
	typedef validx_t<P, I, magic_constant> vidx_type;
};

template <typename P, typename I, template <I> class magic_constant>
struct valptridx_dispatch_require_valid<true, P, I, magic_constant>
{
	typedef vvalptr_t<P, I> vptr_type;
	typedef vvalidx_t<P, I, magic_constant> vidx_type;
};

/*
 * A data type for passing both a pointer and its offset in an
 * agreed-upon array.  Useful for Segments, Objects.
 */
template <
	bool require_valid,
	typename P, typename I, template <I> class magic_constant,
	typename Prc>
struct valptridx_template_t :
	valptridx_dispatch_require_valid<require_valid, P, I, magic_constant>::vptr_type,
	valptridx_dispatch_require_valid<require_valid, P, I, magic_constant>::vidx_type
{
	typedef typename valptridx_dispatch_require_valid<require_valid, P, I, magic_constant>::vptr_type vptr_type;
	typedef typename valptridx_dispatch_require_valid<require_valid, P, I, magic_constant>::vidx_type vidx_type;
	typedef valptridx_template_t valptridx_type;
protected:
	using vidx_type::get_array;
public:
	typedef typename vptr_type::pointer_type pointer_type;
	typedef typename vptr_type::cpointer_type cpointer_type;
	typedef typename vptr_type::mpointer_type mpointer_type;
	typedef typename vptr_type::reference reference;
	typedef typename vidx_type::index_type index_type;
	typedef typename vidx_type::integral_type integral_type;
	operator const vptr_type &() const { return *this; }
	operator const vidx_type &() const { return *this; }
	using vptr_type::operator==;
	using vidx_type::operator==;
	using vidx_type::operator<;
	using vidx_type::operator<=;
	using vidx_type::operator>;
	using vidx_type::operator>=;
	using vidx_type::operator const index_type&;
	valptridx_template_t() = delete;
	valptridx_template_t(std::nullptr_t) = delete;
	/* Swapped V0/V1 matches rvalue construction to/from always-valid type */
	valptridx_template_t(valptridx_template_t<!require_valid, const P, I, magic_constant> &&) = delete;
	valptridx_template_t(valptridx_template_t<!require_valid, Prc, I, magic_constant> &&) = delete;
	/* Convenience conversion to/from always-valid.  Throws on attempt
	 * to make an always-valid from an invalid maybe-valid.
	 */
	valptridx_template_t(const valptridx_template_t<!require_valid, const P, I, magic_constant> &t) :
		vptr_type(t.operator const typename valptridx_dispatch_require_valid<!require_valid, const P, I, magic_constant>::vptr_type &()),
		vidx_type(t.operator const typename valptridx_dispatch_require_valid<!require_valid, const P, I, magic_constant>::vidx_type &())
	{
	}
	valptridx_template_t(const valptridx_template_t<!require_valid, Prc, I, magic_constant> &t) :
		vptr_type(t.operator const typename valptridx_dispatch_require_valid<!require_valid, Prc, I, magic_constant>::vptr_type &()),
		vidx_type(t.operator const typename valptridx_dispatch_require_valid<!require_valid, Prc, I, magic_constant>::vidx_type &())
	{
	}
	/* Copy construction from like type, possibly const-qualified */
	valptridx_template_t(const valptridx_template_t<require_valid, const P, I, magic_constant> &t) :
		vptr_type(t.operator const typename valptridx_dispatch_require_valid<require_valid, const P, I, magic_constant>::vptr_type &()),
		vidx_type(t.operator const typename valptridx_dispatch_require_valid<require_valid, const P, I, magic_constant>::vidx_type &())
	{
	}
	valptridx_template_t(const valptridx_template_t<require_valid, Prc, I, magic_constant> &t) :
		vptr_type(t.operator const typename valptridx_dispatch_require_valid<require_valid, Prc, I, magic_constant>::vptr_type &()),
		vidx_type(t.operator const typename valptridx_dispatch_require_valid<require_valid, Prc, I, magic_constant>::vidx_type &())
	{
	}
	template <integral_type v>
		valptridx_template_t(const magic_constant<v> &m) :
			vptr_type(get_array(), static_cast<std::size_t>(v) < get_array().size() ? &get_array()[v] : nullptr),
			vidx_type(m)
	{
	}
	template <typename A>
	valptridx_template_t(A &a, pointer_type p, index_type s) :
		vptr_type(a, p),
		vidx_type(a, p, s)
	{
	}
	template <typename A>
	valptridx_template_t(A &a, pointer_type p) :
		vptr_type(a, p),
		vidx_type(a, p, p-a)
	{
	}
	valptridx_template_t(pointer_type p) :
		vptr_type(p),
		vidx_type(get_array(), p, p-get_array())
	{
	}
	template <typename A>
	valptridx_template_t(A &a, index_type s) :
		vptr_type(a, s),
		vidx_type(a, s)
	{
	}
	valptridx_template_t(index_type s) :
		vptr_type(get_array(), s),
		vidx_type(get_array(), s)
	{
	}
	template <
		bool erv,
		typename EP>
	/* Reuse Prc to enforce is_same<remove_const<EP>::type,
	 * remove_const<P>::type>.
	 */
	bool operator==(const valptridx_template_t<erv, EP, I, magic_constant, Prc> &rhs) const
	{
		typedef valptridx_template_t<erv, EP, I, magic_constant, Prc> rhs_t;
		return vptr_type::operator==(rhs.operator const typename rhs_t::vptr_type &()) &&
			vidx_type::operator==(rhs.operator const typename rhs_t::vidx_type &());
	}
	template <typename R>
		bool operator!=(const R &rhs) const
		{
			return !(*this == rhs);
		}
};

template <typename P, typename I>
class vvalptr_t : public valptr_t<P, I>
{
	typedef valptr_t<P, I> base_t;
	using base_t::check_index_range;
	using base_t::check_null_pointer;
	typedef typename base_t::Prc Prc;
protected:
	typedef vvalptr_t valptr_type;
public:
	typedef typename base_t::pointer_type pointer_type;
	typedef typename base_t::cpointer_type cpointer_type;
	typedef typename base_t::mpointer_type mpointer_type;
	typedef typename base_t::reference reference;
	using base_t::operator==;
	pointer_type operator->() const DXX_VALPTRIDX_REF_QUALIFIER_LVALUE { return *this; }
	operator cpointer_type() const DXX_VALPTRIDX_REF_QUALIFIER_LVALUE { return base_t::unchecked_const_pointer(); }
	operator mpointer_type() const DXX_VALPTRIDX_REF_QUALIFIER_LVALUE { return base_t::unchecked_mutable_pointer(); }
	reference operator*() const { return *this; }
	operator reference() const { return *static_cast<pointer_type>(*this); }
	vvalptr_t() = delete;
	vvalptr_t(I) = delete;
	vvalptr_t(std::nullptr_t) = delete;
	vvalptr_t(base_t &&) = delete;
	vvalptr_t(const valptr_t<const P, I> &t) :
		base_t(check_null_pointer(static_cast<const P *>(t)))
	{
	}
	vvalptr_t(const valptr_t<Prc, I> &t) :
		base_t(check_null_pointer(static_cast<Prc *>(t)))
	{
	}
	vvalptr_t(const vvalptr_t<const P, I> &t) :
		base_t(t)
	{
	}
	vvalptr_t(const vvalptr_t<Prc, I> &t) :
		base_t(t)
	{
	}
	template <typename A>
		vvalptr_t(A &a, pointer_type p) :
			base_t(a, check_null_pointer(a, p))
	{
	}
	template <typename A>
		vvalptr_t(A &a, I i) :
			base_t(a, check_index_range(a, i))
	{
	}
	vvalptr_t(pointer_type p) :
		base_t(check_null_pointer(p))
	{
	}
	template <template <I> class magic_constant>
	vvalptr_t(valptridx_template_t<true, const P, I, magic_constant> v) :
		base_t(v)
	{
	}
	template <template <I> class magic_constant>
	vvalptr_t(valptridx_template_t<true, Prc, I, magic_constant> v) :
		base_t(v)
	{
	}
	template <template <I> class magic_constant>
	vvalptr_t(valptridx_template_t<false, const P, I, magic_constant> v) :
		base_t(check_null_pointer(static_cast<const P *>(v)))
	{
	}
	template <template <I> class magic_constant>
	vvalptr_t(valptridx_template_t<false, Prc, I, magic_constant> v) :
		base_t(check_null_pointer(static_cast<Prc *>(v)))
	{
	}
	bool operator==(std::nullptr_t) const = delete;
	template <typename R>
		bool operator!=(const R &rhs) const
		{
			return !(*this == rhs);
		}
};

template <typename P, typename I>
valptr_t<P, I>::valptr_t(const vvalptr_t<const P, I> &t) :
	p(t)
{
}

template <typename P, typename I>
valptr_t<P, I>::valptr_t(const vvalptr_t<Prc, I> &t) :
	p(t)
{
}

template <typename P, typename I, template <I> class magic_constant>
class vvalidx_t : public validx_t<P, I, magic_constant>
{
	typedef validx_t<P, I, magic_constant> base_t;
protected:
	using base_t::get_array;
	using base_t::check_index_match;
	using base_t::check_index_range;
	typedef vvalidx_t validx_type;
public:
	typedef typename base_t::index_type index_type;
	typedef typename base_t::integral_type integral_type;
	typedef typename base_t::pointer_type pointer_type;
	template <integral_type v>
		vvalidx_t(const magic_constant<v> &m) :
			base_t(check_constant_index(m))
	{
	}
	vvalidx_t() = delete;
	vvalidx_t(P) = delete;
	vvalidx_t(base_t &&) = delete;
	vvalidx_t(const base_t &t) :
		base_t(check_index_range(get_array(), t))
	{
	}
	template <typename A>
		vvalidx_t(A &a, index_type s) :
			base_t(check_index_range(a, s))
	{
	}
	template <typename A>
	vvalidx_t(A &a, pointer_type p, index_type s) :
		base_t(check_index_match(a, p, check_index_range(a, s)))
	{
	}
	vvalidx_t(index_type s) :
		base_t(check_index_range(get_array(), s))
	{
	}
	template <integral_type v>
		static constexpr const magic_constant<v> &check_constant_index(const magic_constant<v> &m)
		{
#ifndef constexpr
			static_assert(static_cast<std::size_t>(v) < get_array().size(), "invalid index used");
#endif
			return m;
		}
	using base_t::operator==;
	template <integral_type v>
		bool operator==(const magic_constant<v> &m) const
		{
			return base_t::operator==(check_constant_index(m));
		}
	template <typename R>
		bool operator!=(const R &rhs) const
		{
			return !(*this == rhs);
		}
};

template <typename A, A &a, typename vptr, typename vptridx>
struct vvalptr_functions
{
	vptr operator()(typename vptr::cpointer_type p) const
	{
		return {a, p};
	}
	vptr operator()(typename vptr::mpointer_type p) const
	{
		return {a, p};
	}
	vptr operator()(typename vptridx::integral_type i) const
	{
		return {a, i};
	}
	template <typename T>
		vptr operator()(T) const = delete;
	void *operator &() const = delete;
};

template <typename A, A &a, typename ptridx>
struct valptridx_functions
{
	ptridx operator()(typename ptridx::index_type i) const
	{
		return {a, i};
	}
	template <typename T>
		typename tt::enable_if<!tt::is_convertible<T, typename ptridx::index_type>::value, ptridx>::type operator()(T) const = delete;
	void *operator &() const = delete;
};

template <typename A, A &a, typename vptridx>
struct vvalptridx_functions : valptridx_functions<A, a, vptridx>
{
	typedef valptridx_functions<A, a, vptridx> base_t;
	using base_t::operator();
	vptridx operator()(typename vptridx::cpointer_type p) const
	{
		return {a, p};
	}
	vptridx operator()(typename vptridx::mpointer_type p) const
	{
		return {a, p};
	}
};

#define _DEFINE_VALPTRIDX_SUBTYPE_USERTYPE(N,P,I,A,prefix,Pconst)	\
	struct prefix##ptr_t :	\
		prohibit_void_ptr<prefix##ptr_t>,	\
		valptr_t<P Pconst, I>	\
	{	\
		DXX_INHERIT_CONSTRUCTORS(prefix##ptr_t, valptr_type);	\
	};	\
	struct v##prefix##ptr_t :	\
		prohibit_void_ptr<v##prefix##ptr_t>,	\
		vvalptr_t<P Pconst, I>	\
	{	\
		DXX_INHERIT_CONSTRUCTORS(v##prefix##ptr_t, valptr_type);	\
	};	\
	\
	struct prefix##ptridx_t :	\
		prohibit_void_ptr<prefix##ptridx_t>,	\
		valptridx_template_t<false, P Pconst, I, P##_magic_constant_t>	\
	{	\
		DXX_INHERIT_CONSTRUCTORS(prefix##ptridx_t, valptridx_type);	\
	};	\
	\
	struct v##prefix##ptridx_t :	\
		prohibit_void_ptr<v##prefix##ptridx_t>,	\
		valptridx_template_t<true, P Pconst, I, P##_magic_constant_t>	\
	{	\
		DXX_INHERIT_CONSTRUCTORS(v##prefix##ptridx_t, valptridx_type);	\
	};	\
	\
	const vvalptr_functions<Pconst decltype(A), A, v##prefix##ptr_t, v##prefix##ptridx_t> v##prefix##ptr{};	\
	const valptridx_functions<Pconst decltype(A), A, prefix##ptridx_t> prefix##ptridx{};	\
	const vvalptridx_functions<Pconst decltype(A), A, v##prefix##ptridx_t> v##prefix##ptridx{};	\
	static inline v##prefix##ptridx_t operator-(P Pconst *o, decltype(A) Pconst &O)	\
	{	\
		return {A, o, static_cast<v##prefix##ptridx_t::integral_type>(const_cast<const P *>(o) - &(const_cast<const decltype(A) &>(O).front()))};	\
	}	\

#define DEFINE_VALPTRIDX_SUBTYPE(N,P,I,A)	\
	static inline constexpr decltype(A) &get_global_array(P *) { return A; }	\
	/* "decltype(A) const &" parses correctly, but fails to match */	\
	static inline constexpr typename tt::add_const<decltype(A)>::type &get_global_array(P const *) { return A; }	\
	\
	_DEFINE_VALPTRIDX_SUBTYPE_USERTYPE(N,P,I,A,N,);	\
	_DEFINE_VALPTRIDX_SUBTYPE_USERTYPE(N,P,I,A,c##N,const)	\

