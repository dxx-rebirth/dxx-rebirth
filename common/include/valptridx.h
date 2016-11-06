/*
 * This file is part of the DXX-Rebirth project <http://www.dxx-rebirth.com/>.
 * It is copyright by its individual contributors, as recorded in the
 * project's Git history.  See COPYING.txt at the top level for license
 * terms and a link to the Git history.
 */
#pragma once

#include <stdexcept>
#include <string>
#include "fwd-valptridx.h"
#include "compiler-array.h"
#include "compiler-static_assert.h"
#include "compiler-type_traits.h"
#include "pack.h"
#include "poison.h"

#ifdef DXX_CONSTANT_TRUE
#define DXX_VALPTRIDX_STATIC_CHECK(SUCCESS_CONDITION,FAILURE_FUNCTION,FAILURE_STRING)	\
	(	\
		static_cast<void>(DXX_CONSTANT_TRUE(!(SUCCESS_CONDITION)) &&	\
			(DXX_ALWAYS_ERROR_FUNCTION(FAILURE_FUNCTION, FAILURE_STRING), 0)	\
		)	\
	)

#ifdef DXX_HAVE_ATTRIBUTE_WARNING
/* This causes many warnings because some conversions are not checked for
 * safety.  Eliminating the warnings by changing the call sites to check first
 * would be a useful improvement.
 */
//#define DXX_VALPTRIDX_WARN_CALL_NOT_OPTIMIZED_OUT __attribute__((__warning__("call not eliminated")))
#endif
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
			(SUCCESS_CONDITION) || (EXCEPTION::report(DXX_VALPTRIDX_REPORT_STANDARD_LEADER_COMMA_R_PASS_VA(__VA_ARGS__)), 0)	\
		)	\
	)

#ifndef DXX_VALPTRIDX_WARN_CALL_NOT_OPTIMIZED_OUT
#define DXX_VALPTRIDX_WARN_CALL_NOT_OPTIMIZED_OUT
#endif

namespace detail {

class valptridx_array_type_count
{
	unsigned count;
public:
	unsigned get_count() const
	{
		return count;
	}
	void set_count(const unsigned c)
	{
		count = c;
	}
};

}

template <typename INTEGRAL_TYPE, std::size_t array_size_value>
constexpr std::size_t valptridx_specialized_type_parameters<INTEGRAL_TYPE, array_size_value>::array_size;

template <typename P>
class valptridx<P>::index_mismatch_exception :
	public std::logic_error
{
	DXX_INHERIT_CONSTRUCTORS(index_mismatch_exception, logic_error);
public:
	__attribute_cold
	__attribute_noreturn
	DXX_VALPTRIDX_WARN_CALL_NOT_OPTIMIZED_OUT
	static void report(DXX_VALPTRIDX_REPORT_STANDARD_LEADER_COMMA_R_DEFN_VARS const array_managed_type &, index_type, const_pointer_type, const_pointer_type);
};

template <typename P>
class valptridx<P>::index_range_exception :
	public std::out_of_range
{
	DXX_INHERIT_CONSTRUCTORS(index_range_exception, out_of_range);
public:
	__attribute_cold
	__attribute_noreturn
	DXX_VALPTRIDX_WARN_CALL_NOT_OPTIMIZED_OUT
	static void report(DXX_VALPTRIDX_REPORT_STANDARD_LEADER_COMMA_R_DEFN_VARS const array_managed_type *, long);
};

template <typename P>
class valptridx<P>::null_pointer_exception :
	public std::logic_error
{
	DXX_INHERIT_CONSTRUCTORS(null_pointer_exception, logic_error);
public:
	__attribute_cold
	__attribute_noreturn
	DXX_VALPTRIDX_WARN_CALL_NOT_OPTIMIZED_OUT
	static void report(DXX_VALPTRIDX_REPORT_STANDARD_LEADER_COMMA_N_DEFN_VARS);
	__attribute_cold
	__attribute_noreturn
	DXX_VALPTRIDX_WARN_CALL_NOT_OPTIMIZED_OUT
	static void report(DXX_VALPTRIDX_REPORT_STANDARD_LEADER_COMMA_R_DEFN_VARS const array_managed_type &);
};

template <typename managed_type>
void valptridx<managed_type>::check_index_match(DXX_VALPTRIDX_REPORT_STANDARD_LEADER_COMMA_R_DEFN_VARS const managed_type &r, index_type i, const array_managed_type &a)
{
	const auto pi = &a[i];
	DXX_VALPTRIDX_CHECK(pi == &r, index_mismatch_exception, "pointer/index mismatch", a, i, pi, &r);
}

template <typename managed_type>
template <template <typename> class Compare>
typename valptridx<managed_type>::index_type valptridx<managed_type>::check_index_range(DXX_VALPTRIDX_REPORT_STANDARD_LEADER_COMMA_R_DEFN_VARS const index_type i, const array_managed_type *a)
{
	const std::size_t ss = i;
	DXX_VALPTRIDX_CHECK(Compare<std::size_t>()(ss, array_size), index_range_exception, "invalid index used in array subscript", a, ss);
	return i;
}

template <typename managed_type>
void valptridx<managed_type>::check_null_pointer_conversion(DXX_VALPTRIDX_REPORT_STANDARD_LEADER_COMMA_R_DEFN_VARS const_pointer_type p)
{
	DXX_VALPTRIDX_CHECK(p, null_pointer_exception, "NULL pointer converted");
}

template <typename managed_type>
void valptridx<managed_type>::check_null_pointer(DXX_VALPTRIDX_REPORT_STANDARD_LEADER_COMMA_R_DEFN_VARS const_pointer_type p, const array_managed_type &a)
{
	DXX_VALPTRIDX_CHECK(p, null_pointer_exception, "NULL pointer used", a);
}

template <typename managed_type>
void valptridx<managed_type>::check_implicit_index_range_ref(DXX_VALPTRIDX_REPORT_STANDARD_LEADER_COMMA_R_DEFN_VARS const managed_type &r, const array_managed_type &a)
{
	check_explicit_index_range_ref(DXX_VALPTRIDX_REPORT_STANDARD_LEADER_COMMA_R_PASS_VARS r, static_cast<const_pointer_type>(&r) - static_cast<const_pointer_type>(&a.front()), a);
}

template <typename managed_type>
void valptridx<managed_type>::check_explicit_index_range_ref(DXX_VALPTRIDX_REPORT_STANDARD_LEADER_COMMA_R_DEFN_VARS const managed_type &r, std::size_t i, const array_managed_type &a)
{
	check_index_match(DXX_VALPTRIDX_REPORT_STANDARD_LEADER_COMMA_R_PASS_VARS r, i, a);
	check_index_range(DXX_VALPTRIDX_REPORT_STANDARD_LEADER_COMMA_R_PASS_VARS i, &a);
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
template <template <typename> class policy>
class valptridx<managed_type>::partial_policy::apply_cv_policy
{
	template <typename T>
		using apply_cv_qualifier = typename policy<T>::type;
public:
	using array_managed_type = apply_cv_qualifier<valptridx<managed_type>::array_managed_type>;
	using pointer_type = apply_cv_qualifier<managed_type> *;
	using reference_type = apply_cv_qualifier<managed_type> &;
};

template <typename managed_type>
class valptridx<managed_type>::vc :
	public partial_policy::require_valid,
	public partial_policy::template apply_cv_policy<tt::add_const>
{
};

template <typename managed_type>
class valptridx<managed_type>::vm :
	public partial_policy::require_valid,
	public partial_policy::template apply_cv_policy<tt::remove_const>
{
};

template <typename managed_type>
class valptridx<managed_type>::ic :
	public partial_policy::allow_invalid,
	public partial_policy::template apply_cv_policy<tt::add_const>
{
};

template <typename managed_type>
class valptridx<managed_type>::im :
	public partial_policy::allow_invalid,
	public partial_policy::template apply_cv_policy<tt::remove_const>
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
		basic_idx(const basic_idx<rpolicy, ru> &rhs DXX_VALPTRIDX_REPORT_STANDARD_LEADER_COMMA_L_DECL_VARS) :
			m_idx(rhs.get_unchecked_index())
	{
		/* If moving from allow_invalid to require_valid, check range.
		 * If moving from allow_invalid to allow_invalid, no check is
		 * needed.
		 * If moving from require_valid to anything, no check is needed.
		 */
		if (!(allow_nullptr || !rhs.allow_nullptr))
			check_index_range(DXX_VALPTRIDX_REPORT_STANDARD_LEADER_COMMA_R_PASS_VARS m_idx, nullptr);
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
	basic_idx(index_type i DXX_VALPTRIDX_REPORT_STANDARD_LEADER_COMMA_L_DECL_VARS) :
		m_idx(check_allowed_invalid_index(i) ? i : check_index_range(DXX_VALPTRIDX_REPORT_STANDARD_LEADER_COMMA_R_PASS_VARS i, nullptr))
	{
	}
	basic_idx(DXX_VALPTRIDX_REPORT_STANDARD_LEADER_COMMA_R_DEFN_VARS index_type i, array_managed_type &a) :
		m_idx(check_allowed_invalid_index(i) ? i : check_index_range(DXX_VALPTRIDX_REPORT_STANDARD_LEADER_COMMA_R_PASS_VARS i, &a))
	{
	}
protected:
	template <integral_type v>
		basic_idx(const magic_constant<v> &, const allow_none_construction *) :
			m_idx(v)
	{
		static_assert(!allow_nullptr, "allow_none_construction used where nullptr was already legal");
		static_assert(static_cast<std::size_t>(v) >= array_size, "allow_none_construction used with valid index");
	}
	basic_idx(DXX_VALPTRIDX_REPORT_STANDARD_LEADER_COMMA_R_DEFN_VARS index_type i, array_managed_type &a, const allow_end_construction *) :
		m_idx(check_index_range<std::less_equal>(DXX_VALPTRIDX_REPORT_STANDARD_LEADER_COMMA_R_PASS_VARS i, &a))
	{
	}
public:
	template <integral_type v>
		basic_idx(const magic_constant<v> &) :
			m_idx(v)
	{
		static_assert(allow_nullptr || static_cast<std::size_t>(v) < array_size, "invalid magic index not allowed for this policy");
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
			static_assert(allow_nullptr || static_cast<std::size_t>(v) < array_size, "invalid magic index not allowed for this policy");
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
	basic_idx &operator++()
	{
		++ m_idx;
		return *this;
	}
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
	using index_type = typename containing_type::index_type;
	using const_pointer_type = typename containing_type::const_pointer_type;
	using mutable_pointer_type = typename containing_type::mutable_pointer_type;
	using allow_none_construction = typename containing_type::allow_none_construction;
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
		static_assert(static_cast<std::size_t>(v) >= array_size, "valid magic index requires an array");
		static_assert(allow_nullptr || static_cast<std::size_t>(v) < array_size, "invalid magic index not allowed for this policy");
	}
	template <integral_type v>
		basic_ptr(const magic_constant<v> &, array_managed_type &a) :
			m_ptr(static_cast<std::size_t>(v) < array_size ? &(a[v]) : nullptr)
	{
		static_assert(allow_nullptr || static_cast<std::size_t>(v) < array_size, "invalid magic index not allowed for this policy");
	}
	template <typename rpolicy, unsigned ru>
		basic_ptr(const basic_ptr<rpolicy, ru> &rhs DXX_VALPTRIDX_REPORT_STANDARD_LEADER_COMMA_L_DECL_VARS) :
			m_ptr(rhs.get_unchecked_pointer())
	{
		if (!(allow_nullptr || !rhs.allow_nullptr))
			check_null_pointer_conversion(DXX_VALPTRIDX_REPORT_STANDARD_LEADER_COMMA_R_PASS_VARS m_ptr);
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
	basic_ptr(index_type i) = delete;
	basic_ptr(DXX_VALPTRIDX_REPORT_STANDARD_LEADER_COMMA_R_DEFN_VARS index_type i, array_managed_type &a) :
		m_ptr(check_allowed_invalid_index(i) ? nullptr : &a[check_index_range(DXX_VALPTRIDX_REPORT_STANDARD_LEADER_COMMA_R_PASS_VARS i, &a)])
	{
	}
	basic_ptr(DXX_VALPTRIDX_REPORT_STANDARD_LEADER_COMMA_R_DEFN_VARS index_type i, array_managed_type &a, const allow_end_construction *) :
		m_ptr(&a[check_index_range<std::less_equal>(DXX_VALPTRIDX_REPORT_STANDARD_LEADER_COMMA_R_PASS_VARS i, &a)])
	{
	}
	basic_ptr(pointer_type p) = delete;
	basic_ptr(DXX_VALPTRIDX_REPORT_STANDARD_LEADER_COMMA_R_DEFN_VARS pointer_type p, array_managed_type &a) :
		/* No array consistency check here, since some code incorrectly
		 * defines instances of `object` outside the Objects array, then
		 * passes pointers to those instances to this function.
		 */
		m_ptr(p)
	{
		if (!allow_nullptr)
			check_null_pointer(DXX_VALPTRIDX_REPORT_STANDARD_LEADER_COMMA_R_PASS_VARS p, a);
	}
	basic_ptr(DXX_VALPTRIDX_REPORT_STANDARD_LEADER_COMMA_R_DEFN_VARS reference_type r, array_managed_type &a) :
		m_ptr((check_implicit_index_range_ref(DXX_VALPTRIDX_REPORT_STANDARD_LEADER_COMMA_R_PASS_VARS r, a), &r))
	{
	}
	basic_ptr(DXX_VALPTRIDX_REPORT_STANDARD_LEADER_COMMA_R_DEFN_VARS reference_type r, index_type i, array_managed_type &a) :
		m_ptr((check_explicit_index_range_ref(DXX_VALPTRIDX_REPORT_STANDARD_LEADER_COMMA_R_PASS_VARS r, i, a), &r))
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
	basic_ptr &operator++()
	{
		++ m_ptr;
		return *this;
	}
	basic_ptr(const allow_none_construction *) :
		m_ptr(nullptr)
	{
		static_assert(!allow_nullptr, "allow_none_construction used where nullptr was already legal");
	}
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
	using index_type = typename vidx_type::index_type;
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
		basic_ptridx(const basic_ptridx<rpolicy> &rhs DXX_VALPTRIDX_REPORT_STANDARD_LEADER_COMMA_L_DECL_VARS) :
			vptr_type(static_cast<const typename basic_ptridx<rpolicy>::vptr_type &>(rhs) DXX_VALPTRIDX_REPORT_STANDARD_LEADER_COMMA_L_PASS_VARS),
			vidx_type(static_cast<const typename basic_ptridx<rpolicy>::vidx_type &>(rhs) DXX_VALPTRIDX_REPORT_STANDARD_LEADER_COMMA_L_PASS_VARS)
	{
	}
	template <typename rpolicy>
		basic_ptridx(basic_ptridx<rpolicy> &&rhs DXX_VALPTRIDX_REPORT_STANDARD_LEADER_COMMA_L_DECL_VARS) :
			vptr_type(static_cast<typename basic_ptridx<rpolicy>::vptr_type &&>(rhs) DXX_VALPTRIDX_REPORT_STANDARD_LEADER_COMMA_L_PASS_VARS),
			vidx_type(static_cast<typename basic_ptridx<rpolicy>::vidx_type &&>(rhs) DXX_VALPTRIDX_REPORT_STANDARD_LEADER_COMMA_L_PASS_VARS)
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
	template <integral_type v>
		basic_ptridx(const magic_constant<v> &m, const allow_none_construction *const n) :
			vptr_type(n),
			vidx_type(m, n)
	{
	}
	basic_ptridx(index_type i) = delete;
	basic_ptridx(DXX_VALPTRIDX_REPORT_STANDARD_LEADER_COMMA_R_DEFN_VARS index_type i, array_managed_type &a) :
		vptr_type(DXX_VALPTRIDX_REPORT_STANDARD_LEADER_COMMA_R_PASS_VARS i, a),
		vidx_type(DXX_VALPTRIDX_REPORT_STANDARD_LEADER_COMMA_R_PASS_VARS i, a)
	{
	}
	basic_ptridx(DXX_VALPTRIDX_REPORT_STANDARD_LEADER_COMMA_R_DEFN_VARS index_type i, array_managed_type &a, const allow_end_construction *e) :
		vptr_type(DXX_VALPTRIDX_REPORT_STANDARD_LEADER_COMMA_R_PASS_VARS i, a, e),
		vidx_type(DXX_VALPTRIDX_REPORT_STANDARD_LEADER_COMMA_R_PASS_VARS i, a, e)
	{
	}
	basic_ptridx(DXX_VALPTRIDX_REPORT_STANDARD_LEADER_COMMA_R_DEFN_VARS pointer_type p, array_managed_type &a) :
		/* Null pointer is never allowed when an index must be computed.
		 * Check for null, then use the reference constructor for
		 * vptr_type to avoid checking again.
		 */
		vptr_type(DXX_VALPTRIDX_REPORT_STANDARD_LEADER_COMMA_R_PASS_VARS (check_null_pointer(DXX_VALPTRIDX_REPORT_STANDARD_LEADER_COMMA_R_PASS_VARS p, a), *p), a),
		vidx_type(DXX_VALPTRIDX_REPORT_STANDARD_LEADER_COMMA_R_PASS_VARS p - static_cast<pointer_type>(&a.front()), a)
	{
	}
	basic_ptridx(DXX_VALPTRIDX_REPORT_STANDARD_LEADER_COMMA_R_DEFN_VARS pointer_type p, index_type i, array_managed_type &a) :
		vptr_type(DXX_VALPTRIDX_REPORT_STANDARD_LEADER_COMMA_R_PASS_VARS (check_null_pointer(DXX_VALPTRIDX_REPORT_STANDARD_LEADER_COMMA_R_PASS_VARS p, a), *p), i, a),
		vidx_type(DXX_VALPTRIDX_REPORT_STANDARD_LEADER_COMMA_R_PASS_VARS i, a)
	{
	}
	basic_ptridx absolute_sibling(const index_type i DXX_VALPTRIDX_REPORT_STANDARD_LEADER_COMMA_L_DECL_VARS) const
	{
		static_assert(!policy::allow_nullptr, "absolute_sibling not allowed with invalid ptridx");
		basic_ptridx r(*this);
		r.m_ptr += check_index_range(DXX_VALPTRIDX_REPORT_STANDARD_LEADER_COMMA_R_PASS_VARS i, nullptr) - this->m_idx;
		r.m_idx = i;
		return r;
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
protected:
	basic_ptridx &operator++()
	{
		vptr_type::operator++();
		vidx_type::operator++();
		return *this;
	}
};

template <typename managed_type>
class valptridx<managed_type>::array_managed_type :
	public detail::valptridx_array_type_count,
	public array<managed_type, array_size>
{
	using containing_type = valptridx<managed_type>;
	using array_type = array<managed_type, array_size>;
public:
	using typename array_type::reference;
	using typename array_type::const_reference;
	using index_type = typename containing_type::index_type;
	reference operator[](const integral_type &n)
		{
			return array_type::operator[](n);
		}
	const_reference operator[](const integral_type &n) const
		{
			return array_type::operator[](n);
		}
	template <typename T>
		reference operator[](const T &) const = delete;
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
	struct iterator :
		std::iterator<std::forward_iterator_tag, P>,
		P
	{
		using P::operator++;
		iterator(P &&i) :
			P(static_cast<P &&>(i))
		{
		}
		P operator*() const
		{
			return *this;
		}
	};
public:
	using index_type = typename containing_type::index_type;
	using result_type = P;
	basic_vptr_global_factory() = default;
	basic_vptr_global_factory(const basic_vptr_global_factory &) = delete;
	basic_vptr_global_factory &operator=(const basic_vptr_global_factory &) = delete;
	__attribute_warn_unused_result
	P operator()(typename P::const_pointer_type p DXX_VALPTRIDX_REPORT_STANDARD_LEADER_COMMA_L_DECL_VARS) const
	{
		return P(DXX_VALPTRIDX_REPORT_STANDARD_LEADER_COMMA_R_PASS_VARS p, get_array(p));
	}
	__attribute_warn_unused_result
	P operator()(typename P::mutable_pointer_type p DXX_VALPTRIDX_REPORT_STANDARD_LEADER_COMMA_L_DECL_VARS) const
	{
		return P(DXX_VALPTRIDX_REPORT_STANDARD_LEADER_COMMA_R_PASS_VARS p, get_array(p));
	}
	__attribute_warn_unused_result
	P operator()(index_type i DXX_VALPTRIDX_REPORT_STANDARD_LEADER_COMMA_L_DECL_VARS) const
	{
		return P(DXX_VALPTRIDX_REPORT_STANDARD_LEADER_COMMA_R_PASS_VARS i, get_array());
	}
	__attribute_warn_unused_result
	typename array_managed_type::size_type count() const
	{
		return get_array().get_count();
	}
	__attribute_warn_unused_result
	typename array_managed_type::size_type size() const
	{
		return get_array().size();
	}
	__attribute_warn_unused_result
	iterator begin() const
	{
		return P(containing_type::magic_constant<0>(), get_array());
	}
	__attribute_warn_unused_result
	iterator end(DXX_VALPTRIDX_REPORT_STANDARD_LEADER_COMMA_N_DECL_VARS) const
	{
		auto &a = get_array();
		return P(DXX_VALPTRIDX_REPORT_STANDARD_LEADER_COMMA_R_PASS_VARS static_cast<index_type>(a.get_count()), a, static_cast<const allow_end_construction *>(nullptr));
	}
	template <containing_type::integral_type v>
		__attribute_warn_unused_result
		P operator()(const containing_type::magic_constant<v> &m) const
		{
			return P(m, get_array());
		}
	template <typename policy>
		P operator()(containing_type::basic_idx<policy, 0> i DXX_VALPTRIDX_REPORT_STANDARD_LEADER_COMMA_L_DECL_VARS) const
		{
			return P(DXX_VALPTRIDX_REPORT_STANDARD_LEADER_COMMA_R_PASS_VARS i, get_array());
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
	using result_type = PI;
	basic_ptridx_global_factory() = default;
	basic_ptridx_global_factory(const basic_ptridx_global_factory &) = delete;
	basic_ptridx_global_factory &operator=(const basic_ptridx_global_factory &) = delete;
	__attribute_warn_unused_result
	PI operator()(typename PI::index_type i DXX_VALPTRIDX_REPORT_STANDARD_LEADER_COMMA_L_DECL_VARS) const
	{
		return PI(DXX_VALPTRIDX_REPORT_STANDARD_LEADER_COMMA_R_PASS_VARS i, get_array());
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

#define DXX_VALPTRIDX_DEFINE_GLOBAL_FACTORIES3(MANAGED_TYPE,FACTORY)	\
	constexpr valptridx<MANAGED_TYPE>::basic_vptr_global_factory<v##FACTORY##_t> v##FACTORY{};	\
	constexpr valptridx<MANAGED_TYPE>::basic_ptridx_global_factory<FACTORY##_t> FACTORY{}	\

#define DXX_VALPTRIDX_DEFINE_GLOBAL_FACTORIES2(MANAGED_TYPE,PREFIX)	\
	DXX_VALPTRIDX_DEFINE_GLOBAL_FACTORIES3(MANAGED_TYPE,PREFIX##ptr);	\
	DXX_VALPTRIDX_DEFINE_GLOBAL_FACTORIES3(MANAGED_TYPE,PREFIX##ptridx)	\

#define DXX_VALPTRIDX_DEFINE_GLOBAL_FACTORIES(P,N)	\
	DXX_VALPTRIDX_DEFINE_GLOBAL_FACTORIES2(P,N);	\
	DXX_VALPTRIDX_DEFINE_GLOBAL_FACTORIES2(P,c##N)	\

