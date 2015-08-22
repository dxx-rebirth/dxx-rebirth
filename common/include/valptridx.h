/*
 * This file is part of the DXX-Rebirth project <http://www.dxx-rebirth.com/>.
 * It is copyright by its individual contributors, as recorded in the
 * project's Git history.  See COPYING.txt at the top level for license
 * terms and a link to the Git history.
 */
#pragma once

#include <stdexcept>
#include <string>
#include "fwdvalptridx.h"
#include "compiler-static_assert.h"
#include "compiler-type_traits.h"
#include "pack.h"
#include "poison.h"

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
			(SUCCESS_CONDITION) || (EXCEPTION::report(__VA_ARGS__), 0)	\
		)	\
	)

class valptridxutil_untyped_base
{
public:
	class index_mismatch_exception;
	class index_range_exception;
	class null_pointer_exception;
};

class valptridxutil_untyped_base::index_mismatch_exception
{
protected:
#define REPORT_STANDARD_FORMAT	" base=%p size=%lu"
#define REPORT_STANDARD_ARGUMENTS	array_base, static_cast<unsigned long>(array_size)
#define REPORT_STANDARD_SIZE	(sizeof(REPORT_FORMAT_STRING) + sizeof("0x0000000000000000") + sizeof("18446744073709551615"))
#define REPORT_FORMAT_STRING	"pointer/index mismatch:" REPORT_STANDARD_FORMAT " index=%li expected=%p actual=%p"
	static constexpr std::size_t report_buffer_size = REPORT_STANDARD_SIZE + (sizeof("0x0000000000000000") * 2) + sizeof("18446744073709551615");
	__attribute_cold
	static void prepare_report(char (&buf)[report_buffer_size], const void *const array_base, const std::size_t array_size, const long supplied_index, const void *const expected_pointer, const void *const actual_pointer)
	{
		snprintf(buf, sizeof(buf), REPORT_FORMAT_STRING, REPORT_STANDARD_ARGUMENTS, supplied_index, expected_pointer, actual_pointer);
	}
#undef REPORT_FORMAT_STRING
};

class valptridxutil_untyped_base::index_range_exception
{
protected:
#define REPORT_FORMAT_STRING	"invalid index used in array subscript:" REPORT_STANDARD_FORMAT " index=%li"
	static constexpr std::size_t report_buffer_size = REPORT_STANDARD_SIZE + sizeof("18446744073709551615");
	__attribute_cold
	static void prepare_report(char (&buf)[report_buffer_size], const void *const array_base, const std::size_t array_size, const long supplied_index)
	{
		snprintf(buf, sizeof(buf), REPORT_FORMAT_STRING, REPORT_STANDARD_ARGUMENTS, supplied_index);
	}
#undef REPORT_FORMAT_STRING
};

class valptridxutil_untyped_base::null_pointer_exception
{
protected:
#define REPORT_FORMAT_STRING	"NULL pointer used:" REPORT_STANDARD_FORMAT
	static constexpr std::size_t report_buffer_size = REPORT_STANDARD_SIZE;
	__attribute_cold
	static void prepare_report(char (&buf)[report_buffer_size], const void *const array_base, const std::size_t array_size)
	{
		snprintf(buf, sizeof(buf), REPORT_FORMAT_STRING, REPORT_STANDARD_ARGUMENTS);
	}
#undef REPORT_FORMAT_STRING
#undef REPORT_STANDARD_SIZE
#undef REPORT_STANDARD_ARGUMENTS
#undef REPORT_STANDARD_FORMAT
};

template <typename P>
class valptridx<P>::index_mismatch_exception :
	valptridxutil_untyped_base::index_mismatch_exception,
	public std::logic_error
{
	DXX_INHERIT_CONSTRUCTORS(index_mismatch_exception, logic_error);
public:
	__attribute_cold
	__attribute_noreturn
	static void report(const_pointer_type, std::size_t, index_type, const_pointer_type, const_pointer_type);
};

template <typename P>
class valptridx<P>::index_range_exception :
	valptridxutil_untyped_base::index_range_exception,
	public std::out_of_range
{
	DXX_INHERIT_CONSTRUCTORS(index_range_exception, out_of_range);
public:
	__attribute_cold
	__attribute_noreturn
	static void report(const_pointer_type, std::size_t, long);
};

template <typename P>
class valptridx<P>::null_pointer_exception :
	valptridxutil_untyped_base::null_pointer_exception,
	public std::logic_error
{
	DXX_INHERIT_CONSTRUCTORS(null_pointer_exception, logic_error);
public:
	__attribute_cold
	__attribute_noreturn
	static void report(const_pointer_type, std::size_t);
};

template <typename managed_type>
void valptridx<managed_type>::index_mismatch_exception::report(const const_pointer_type array_base, const std::size_t array_size, const index_type supplied_index, const const_pointer_type expected_pointer, const const_pointer_type actual_pointer)
{
	char buf[report_buffer_size];
	prepare_report(buf, array_base, array_size, supplied_index, expected_pointer, actual_pointer);
	throw index_mismatch_exception(buf);
}

template <typename managed_type>
void valptridx<managed_type>::index_range_exception::report(const const_pointer_type array_base, const std::size_t array_size, const long supplied_index)
{
	char buf[report_buffer_size];
	prepare_report(buf, array_base, array_size, supplied_index);
	throw index_range_exception(buf);
}

template <typename managed_type>
void valptridx<managed_type>::null_pointer_exception::report(const const_pointer_type array_base, const std::size_t array_size)
{
	char buf[report_buffer_size];
	prepare_report(buf, array_base, array_size);
	throw null_pointer_exception(buf);
}

template <typename managed_type>
void valptridx<managed_type>::check_index_match(const managed_type &r, index_type i, const array_managed_type &a)
{
	const auto pi = &a[i];
	DXX_VALPTRIDX_CHECK(pi == &r, index_mismatch_exception, "pointer/index mismatch", &a[0], get_array_size(), i, pi, &r);
}

template <typename managed_type>
typename valptridx<managed_type>::index_type valptridx<managed_type>::check_index_range(index_type i, const array_managed_type &a)
{
	const std::size_t ss = i;
	const std::size_t as = get_array_size();
	DXX_VALPTRIDX_CHECK(ss < as, index_range_exception, "invalid index used in array subscript", &a[0], as, ss);
	return i;
}

template <typename managed_type>
void valptridx<managed_type>::check_null_pointer_conversion(const_pointer_type p)
{
	DXX_VALPTRIDX_CHECK(p, null_pointer_exception, "NULL pointer converted", p, get_array_size());
}

template <typename managed_type>
void valptridx<managed_type>::check_null_pointer(const_pointer_type p, const array_managed_type &a)
{
	DXX_VALPTRIDX_CHECK(p, null_pointer_exception, "NULL pointer used", &a[0], get_array_size());
}

template <typename managed_type>
void valptridx<managed_type>::check_implicit_index_range_ref(const managed_type &r, const array_managed_type &a)
{
	check_explicit_index_range_ref(r, &r - a, a);
}

template <typename managed_type>
void valptridx<managed_type>::check_explicit_index_range_ref(const managed_type &r, std::size_t i, const array_managed_type &a)
{
	check_index_match(r, i, a);
	check_index_range(i, a);
}

template <typename managed_type>
class valptridx<managed_type>::partial_policy::require_valid
{
public:
	static constexpr tt::false_type allow_nullptr{};
	static constexpr tt::false_type check_allowed_invalid_index(index_type) { return {}; }
};

template <typename managed_type>
class valptridx<managed_type>::partial_policy::allow_invalid
{
public:
	static constexpr tt::true_type allow_nullptr{};
	static constexpr bool check_allowed_invalid_index(index_type i)
	{
		return i == static_cast<index_type>(~0);
	}
};

template <typename managed_type>
constexpr tt::false_type valptridx<managed_type>::partial_policy::require_valid::allow_nullptr;

template <typename managed_type>
constexpr tt::true_type valptridx<managed_type>::partial_policy::allow_invalid::allow_nullptr;

template <typename managed_type>
class valptridx<managed_type>::partial_policy::const_policy
{
protected:
	template <typename T>
		using apply_cv_qualifier = const T;
};

template <typename managed_type>
class valptridx<managed_type>::partial_policy::mutable_policy
{
protected:
	template <typename T>
		using apply_cv_qualifier = T;
};

template <typename managed_type>
template <typename policy>
class valptridx<managed_type>::partial_policy::apply_cv_policy :
	policy
{
	template <typename T>
		using apply_cv_qualifier = typename policy::template apply_cv_qualifier<T>;
public:
	using array_managed_type = apply_cv_qualifier<valptridx<managed_type>::array_managed_type>;
	using pointer_type = apply_cv_qualifier<managed_type> *;
	using reference_type = apply_cv_qualifier<managed_type> &;
};

template <typename managed_type>
class valptridx<managed_type>::vc :
	public partial_policy::require_valid,
	public partial_policy::template apply_cv_policy<typename partial_policy::const_policy>
{
};

template <typename managed_type>
class valptridx<managed_type>::vm :
	public partial_policy::require_valid,
	public partial_policy::template apply_cv_policy<typename partial_policy::mutable_policy>
{
};

template <typename managed_type>
class valptridx<managed_type>::ic :
	public partial_policy::allow_invalid,
	public partial_policy::template apply_cv_policy<typename partial_policy::const_policy>
{
};

template <typename managed_type>
class valptridx<managed_type>::im :
	public partial_policy::allow_invalid,
	public partial_policy::template apply_cv_policy<typename partial_policy::mutable_policy>
{
};

template <typename managed_type>
template <typename policy, unsigned>
class valptridx<managed_type>::basic_idx :
	public policy
{
	using containing_type = valptridx<managed_type>;
public:
	using policy::allow_nullptr;
	using policy::check_allowed_invalid_index;
	using index_type = typename containing_type::index_type;
	using integral_type = typename containing_type::integral_type;
	using typename policy::array_managed_type;
	basic_idx() = delete;
	basic_idx(const basic_idx &) = default;
	basic_idx(basic_idx &&) = default;
	basic_idx &operator=(const basic_idx &) = default;
	basic_idx &operator=(basic_idx &&) = default;

	index_type get_unchecked_index() const { return m_idx; }

	template <typename rpolicy, unsigned ru>
		basic_idx(const basic_idx<rpolicy, ru> &rhs, array_managed_type &a = get_array()) :
			m_idx(rhs.get_unchecked_index())
	{
		if (!(allow_nullptr || !rhs.allow_nullptr))
			check_index_range(m_idx, a);
	}
	template <typename rpolicy, unsigned ru>
		basic_idx(basic_idx<rpolicy, ru> &&rhs) :
			m_idx(rhs.get_unchecked_index())
	{
		/* Prevent move from allow_invalid into require_valid.  The
		 * right hand side must be saved and checked for validity before
		 * being used to initialize a require_valid type.
		 */
		static_assert(allow_nullptr || !rhs.allow_nullptr, "cannot move from allow_invalid to require_valid");
	}
	basic_idx(index_type i, array_managed_type &a = get_array()) :	// default argument deprecated
		m_idx(check_allowed_invalid_index(i) ? i : check_index_range(i, a))
	{
	}
	template <integral_type v>
		basic_idx(const magic_constant<v> &) :
			m_idx(v)
	{
		static_assert(allow_nullptr || static_cast<std::size_t>(v) < get_array_size(), "invalid magic index not allowed for this policy");
	}
	template <typename rpolicy, unsigned ru>
		bool operator==(const basic_idx<rpolicy, ru> &rhs) const
		{
			return m_idx == rhs.get_unchecked_index();
		}
	bool operator==(const index_type &i) const
	{
		return m_idx == i;
	}
	template <integral_type v>
		bool operator==(const magic_constant<v> &) const
		{
			static_assert(allow_nullptr || static_cast<std::size_t>(v) < get_array_size(), "invalid magic index not allowed for this policy");
			return m_idx == v;
		}
	template <typename R>
		bool operator!=(const R &rhs) const
		{
			return !(*this == rhs);
		}
	operator index_type() const
	{
		return m_idx;
	}
protected:
	index_type m_idx;
};

template <typename managed_type>
template <typename policy, unsigned>
class valptridx<managed_type>::basic_ptr :
	public policy
{
	using containing_type = valptridx<managed_type>;
public:
	using policy::allow_nullptr;
	using policy::check_allowed_invalid_index;
	using const_pointer_type = typename containing_type::const_pointer_type;
	using mutable_pointer_type = typename containing_type::mutable_pointer_type;
	using typename policy::array_managed_type;
	using typename policy::pointer_type;
	using typename policy::reference_type;

	basic_ptr() = delete;
	/* Override template matches to make same-type copy/move trivial */
	basic_ptr(const basic_ptr &) = default;
	basic_ptr(basic_ptr &&) = default;
	basic_ptr &operator=(const basic_ptr &) = default;
	basic_ptr &operator=(basic_ptr &&) = default;

	pointer_type get_unchecked_pointer() const { return m_ptr; }

	basic_ptr(std::nullptr_t) :
		m_ptr(nullptr)
	{
		static_assert(allow_nullptr, "nullptr construction not allowed for this policy");
	}
	template <integral_type v>
		basic_ptr(const magic_constant<v> &) :
			m_ptr(nullptr)
	{
		static_assert(static_cast<std::size_t>(v) >= get_array_size(), "valid magic index requires an array");
		static_assert(allow_nullptr || static_cast<std::size_t>(v) < get_array_size(), "invalid magic index not allowed for this policy");
	}
	template <integral_type v>
		basic_ptr(const magic_constant<v> &, array_managed_type &a) :
			m_ptr(static_cast<std::size_t>(v) < get_array_size() ? &(a[v]) : nullptr)
	{
		static_assert(allow_nullptr || static_cast<std::size_t>(v) < get_array_size(), "invalid magic index not allowed for this policy");
	}
	template <typename rpolicy, unsigned ru>
		basic_ptr(const basic_ptr<rpolicy, ru> &rhs) :
			m_ptr(rhs.get_unchecked_pointer())
	{
		if (!(allow_nullptr || !rhs.allow_nullptr))
			check_null_pointer_conversion(m_ptr);
	}
	template <typename rpolicy, unsigned ru>
		basic_ptr(basic_ptr<rpolicy, ru> &&rhs) :
			m_ptr(rhs.get_unchecked_pointer())
	{
		/* Prevent move from allow_invalid into require_valid.  The
		 * right hand side must be saved and checked for validity before
		 * being used to initialize a require_valid type.
		 */
		static_assert(allow_nullptr || !rhs.allow_nullptr, "cannot move from allow_invalid to require_valid");
	}
	basic_ptr(index_type i, array_managed_type &a = get_array()) :
		m_ptr(check_allowed_invalid_index(i) ? nullptr : &a[check_index_range(i, a)])
	{
	}
	basic_ptr(pointer_type p, array_managed_type &a = get_array()) :
		m_ptr(p)
	{
		if (!allow_nullptr)
			check_null_pointer(p, a);
	}
	basic_ptr(reference_type r, array_managed_type &a) :
		m_ptr((check_implicit_index_range_ref(r, a), &r))
	{
	}
	basic_ptr(reference_type r, index_type i, array_managed_type &a) :
		m_ptr((check_explicit_index_range_ref(r, i, a), &r))
	{
	}

	operator mutable_pointer_type() const { return m_ptr; }	// implicit pointer conversion deprecated
	operator const_pointer_type() const { return m_ptr; }	// implicit pointer conversion deprecated
	pointer_type operator->() const DXX_VALPTRIDX_REF_QUALIFIER_LVALUE
	{
		return m_ptr;
	}
	operator reference_type() const DXX_VALPTRIDX_REF_QUALIFIER_LVALUE
	{
		return *m_ptr;
	}
	reference_type operator*() const DXX_VALPTRIDX_REF_QUALIFIER_LVALUE
	{
		return *this;
	}
	explicit operator bool() const DXX_VALPTRIDX_REF_QUALIFIER_LVALUE
	{
		return !(*this == nullptr);
	}
#ifdef DXX_HAVE_CXX11_REF_QUALIFIER
	pointer_type operator->() const &&
	{
		static_assert(!allow_nullptr, "operator-> not allowed with allow_invalid policy");
		return operator->();
	}
	operator reference_type() const &&
	{
		static_assert(!allow_nullptr, "implicit reference not allowed with allow_invalid policy");
		return *this;
	}
	reference_type operator*() const &&
	{
		static_assert(!allow_nullptr, "operator* not allowed with allow_invalid policy");
		return *this;
	}
	explicit operator bool() const && = delete;
#endif
	bool operator==(std::nullptr_t) const
	{
		static_assert(allow_nullptr, "nullptr comparison not allowed: value is never null");
		return m_ptr == nullptr;
	}
	bool operator==(const_pointer_type p) const
	{
		return m_ptr == p;
	}
	bool operator==(mutable_pointer_type p) const
	{
		return m_ptr == p;
	}
	template <typename rpolicy, unsigned ru>
		bool operator==(const basic_ptr<rpolicy, ru> &rhs) const
		{
			return *this == rhs.get_unchecked_pointer();
		}
	template <typename R>
		bool operator!=(const R &rhs) const
		{
			return !(*this == rhs);
		}
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
	pointer_type m_ptr;
};

template <typename managed_type>
template <typename policy>
class valptridx<managed_type>::basic_ptridx :
	public prohibit_void_ptr<basic_ptridx<policy>>,
	public basic_ptr<policy, 1>,
	public basic_idx<policy, 1>
{
public:
	typedef basic_ptr<policy, 1> vptr_type;
	typedef basic_idx<policy, 1> vidx_type;
	using typename vidx_type::array_managed_type;
	using typename vidx_type::index_type;
	using typename vidx_type::integral_type;
	using typename vptr_type::pointer_type;
	using vidx_type::operator==;
	using vptr_type::operator==;
	basic_ptridx(const basic_ptridx &) = default;
	basic_ptridx(basic_ptridx &&) = default;
	basic_ptridx &operator=(const basic_ptridx &) = default;
	basic_ptridx &operator=(basic_ptridx &&) = default;
	basic_ptridx(std::nullptr_t) = delete;
	/* Prevent implicit conversion.  Require use of the factory function.
	 */
	basic_ptridx(pointer_type p) = delete;
	template <typename rpolicy>
		basic_ptridx(const basic_ptridx<rpolicy> &rhs) :
			vptr_type(static_cast<const typename basic_ptridx<rpolicy>::vptr_type &>(rhs)),
			vidx_type(static_cast<const typename basic_ptridx<rpolicy>::vidx_type &>(rhs))
	{
	}
	template <typename rpolicy>
		basic_ptridx(basic_ptridx<rpolicy> &&rhs) :
			vptr_type(static_cast<typename basic_ptridx<rpolicy>::vptr_type &&>(rhs)),
			vidx_type(static_cast<typename basic_ptridx<rpolicy>::vidx_type &&>(rhs))
	{
	}
	template <integral_type v>
		basic_ptridx(const magic_constant<v> &m) :
			vptr_type(m),
			vidx_type(m)
	{
	}
	template <integral_type v>
		basic_ptridx(const magic_constant<v> &m, array_managed_type &a) :
			vptr_type(m, a),
			vidx_type(m)
	{
	}
	basic_ptridx(index_type i, array_managed_type &a = get_array()) :	// default argument deprecated
		vptr_type(i, a),
		vidx_type(i, a)
	{
	}
	basic_ptridx(pointer_type p, array_managed_type &a) :
		/* Null pointer is never allowed when an index must be computed.
		 * Check for null, then use the reference constructor for
		 * vptr_type to avoid checking again.
		 */
		vptr_type((check_null_pointer(p, a), *p), a),
		vidx_type(p - a, a)
	{
	}
	basic_ptridx(pointer_type p, index_type i, array_managed_type &a) :
		vptr_type((check_null_pointer(p, a), *p), i, a),
		vidx_type(i, a)
	{
	}
	template <typename rpolicy>
		bool operator==(const basic_ptridx<rpolicy> &rhs) const
		{
			return vptr_type::operator==(static_cast<const typename basic_ptridx<rpolicy>::vptr_type &>(rhs));
		}
	template <typename R>
		bool operator!=(const R &rhs) const
		{
			return !(*this == rhs);
		}
};

template <typename managed_type>
class valptridx<managed_type>::array_managed_type : public array<managed_type, get_array_size()>
{
	using containing_type = valptridx<managed_type>;
	using array_type = array<managed_type, get_array_size()>;
public:
	using typename array_type::reference;
	using typename array_type::const_reference;
	using index_type = typename containing_type::index_type;
	unsigned highest;
	template <typename T>
		typename tt::enable_if<tt::is_integral<T>::value, reference>::type operator[](T n)
		{
			return array_type::operator[](n);
		}
	template <typename T>
		typename tt::enable_if<tt::is_integral<T>::value, const_reference>::type operator[](T n) const
		{
			return array_type::operator[](n);
		}
	template <typename T>
		typename tt::enable_if<!tt::is_integral<T>::value, reference>::type operator[](T) const = delete;
#if DXX_HAVE_POISON_UNDEFINED
	array_managed_type();
#else
	array_managed_type() = default;
#endif
	array_managed_type(const array_managed_type &) = delete;
	array_managed_type &operator=(const array_managed_type &) = delete;
};

template <typename managed_type>
template <typename P>
class valptridx<managed_type>::basic_vptr_global_factory
{
	using containing_type = valptridx<managed_type>;
public:
	__attribute_warn_unused_result
	P operator()(typename P::const_pointer_type p) const
	{
		return P(p, get_array(p));
	}
	__attribute_warn_unused_result
	P operator()(typename P::mutable_pointer_type p) const
	{
		return P(p, get_array(p));
	}
	__attribute_warn_unused_result
	P operator()(typename containing_type::index_type i) const
	{
		return P(i, get_array());
	}
	template <containing_type::integral_type v>
		__attribute_warn_unused_result
		P operator()(const containing_type::magic_constant<v> &m) const
		{
			return P(m, get_array());
		}
	template <typename T>
		P operator()(T &&) const = delete;
	void *operator &() const = delete;
};

template <typename managed_type>
template <typename PI>
class valptridx<managed_type>::basic_ptridx_global_factory
{
	using containing_type = valptridx<managed_type>;
public:
	__attribute_warn_unused_result
	PI operator()(typename PI::index_type i) const
	{
		return PI(i, get_array());
	}
	template <containing_type::integral_type v>
		__attribute_warn_unused_result
		PI operator()(const containing_type::magic_constant<v> &m) const
		{
			return PI(m, get_array());
		}
	template <typename T>
		PI operator()(T &&) const = delete;
	void *operator &() const = delete;
};

#define _DEFINE_VALPTRIDX_SUBTYPE_USERTYPE(N,P,I,A,prefix,Pconst)	\
	constexpr valptridx<P>::basic_vptr_global_factory<v##prefix##ptr_t> v##prefix##ptr{};	\
	constexpr valptridx<P>::basic_ptridx_global_factory<prefix##ptridx_t> prefix##ptridx{};	\
	constexpr valptridx<P>::basic_vptr_global_factory<v##prefix##ptridx_t> v##prefix##ptridx{};	\
	static inline v##prefix##ptridx_t operator-(P Pconst *o, decltype(A) Pconst &O)	\
	{	\
		return {o, static_cast<v##prefix##ptridx_t::integral_type>(const_cast<const P *>(o) - &(const_cast<const decltype(A) &>(O).front())), A};	\
	}	\

#define DEFINE_VALPTRIDX_SUBTYPE(N,P,I,A)	\
	static inline constexpr decltype(A) &get_global_array(P *) { return A; }	\
	/* "decltype(A) const &" parses correctly, but fails to match */	\
	static inline constexpr typename tt::add_const<decltype(A)>::type &get_global_array(P const *) { return A; }	\
	\
	_DEFINE_VALPTRIDX_SUBTYPE_USERTYPE(N,P,I,A,N,);	\
	_DEFINE_VALPTRIDX_SUBTYPE_USERTYPE(N,P,I,A,c##N,const)	\

