/*
 * This file is part of the DXX-Rebirth project <http://www.dxx-rebirth.com/>.
 * It is copyright by its individual contributors, as recorded in the
 * project's Git history.  See COPYING.txt at the top level for license
 * terms and a link to the Git history.
 */
#pragma once

#include <stdexcept>
#include <string>
#include <type_traits>
#include "fwd-valptridx.h"
#include "compiler-array.h"
#include "pack.h"
#include "compiler-poison.h"
#include "selfiter.h"

#ifdef DXX_CONSTANT_TRUE
#define DXX_VALPTRIDX_STATIC_CHECK(SUCCESS_CONDITION,FAILURE_FUNCTION,FAILURE_STRING)	\
		static_cast<void>(DXX_CONSTANT_TRUE(!SUCCESS_CONDITION) &&	\
			(DXX_ALWAYS_ERROR_FUNCTION(FAILURE_FUNCTION, FAILURE_STRING), 0)	\
		)	\

#ifdef DXX_HAVE_ATTRIBUTE_WARNING
/* This causes many warnings because some conversions are not checked for
 * safety.  Eliminating the warnings by changing the call sites to check first
 * would be a useful improvement.
 */
//#define DXX_VALPTRIDX_WARN_CALL_NOT_OPTIMIZED_OUT __attribute__((__warning__("call not eliminated")))
#endif
#else
#define DXX_VALPTRIDX_STATIC_CHECK(E,F,S)
#endif

#define DXX_VALPTRIDX_CHECK(SUCCESS_CONDITION,ERROR,FAILURE_STRING,...)	\
	( DXX_BEGIN_COMPOUND_STATEMENT {	\
		const bool dxx_valptridx_check_success_condition = (SUCCESS_CONDITION);	\
		DXX_VALPTRIDX_STATIC_CHECK(dxx_valptridx_check_success_condition, dxx_trap_##ERROR, FAILURE_STRING);	\
		static_cast<void>(	\
			dxx_valptridx_check_success_condition || (ERROR::report(DXX_VALPTRIDX_REPORT_STANDARD_LEADER_COMMA_R_PASS_VA(__VA_ARGS__)), 0)	\
		);	\
	} DXX_END_COMPOUND_STATEMENT )

#ifndef DXX_VALPTRIDX_WARN_CALL_NOT_OPTIMIZED_OUT
#define DXX_VALPTRIDX_WARN_CALL_NOT_OPTIMIZED_OUT
#endif

template <typename managed_type>
class valptridx<managed_type>::array_base_count_type
{
protected:
	union {
		unsigned count;
		/*
		 * Use DXX_VALPTRIDX_FOR_EACH_PPI_TYPE to generate empty union
		 * members based on basic_{i,v}val_member_factory
		 * specializations.
		 */
#define DXX_VALPTRIDX_DEFINE_MEMBER_FACTORIES(MANAGED_TYPE, DERIVED_TYPE_PREFIX, CONTEXT, PISUFFIX, IVPREFIX, MCPREFIX)	\
		valptridx<MANAGED_TYPE>::f ## IVPREFIX ## MCPREFIX ## PISUFFIX	\
			IVPREFIX ## MCPREFIX ## PISUFFIX
		DXX_VALPTRIDX_FOR_EACH_PPI_TYPE(DXX_VALPTRIDX_DEFINE_MEMBER_FACTORIES, managed_type,,);
#undef DXX_VALPTRIDX_DEFINE_MEMBER_FACTORIES
	};
	constexpr array_base_count_type() :
		count(0)
	{
	}
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

template <
	typename INTEGRAL_TYPE,
	std::size_t array_size_value,
	valptridx_untyped_utilities::report_error_style report_const_error_value,
	valptridx_untyped_utilities::report_error_style report_mutable_error_value
	>
constexpr std::integral_constant<std::size_t, array_size_value> valptridx_specialized_type_parameters<INTEGRAL_TYPE, array_size_value, report_const_error_value, report_mutable_error_value>::array_size;

class valptridx_untyped_utilities::report_error_undefined
{
public:
	__attribute_cold
	DXX_VALPTRIDX_WARN_CALL_NOT_OPTIMIZED_OUT
	static void report(...)
	{
	}
};

class valptridx_untyped_utilities::report_error_trap_terse
{
public:
	__attribute_cold
	__attribute_noreturn
	DXX_VALPTRIDX_WARN_CALL_NOT_OPTIMIZED_OUT
	static void report(...)
	{
		__builtin_trap();
	}
};

class valptridx_untyped_utilities::index_mismatch_trap_verbose
{
public:
	__attribute_cold
	__attribute_noreturn
	DXX_VALPTRIDX_WARN_CALL_NOT_OPTIMIZED_OUT
	static void report(DXX_VALPTRIDX_REPORT_STANDARD_LEADER_COMMA_R_DEFN_VARS const void *const array, const unsigned long supplied_index, const void *const expected_pointer, const void *const actual_pointer)
	{
		__asm__ __volatile__("" :: DXX_VALPTRIDX_REPORT_STANDARD_ASM_LOAD_COMMA_R_VARS "rm" (array), "rm" (supplied_index), "rm" (expected_pointer), "rm" (actual_pointer));
		__builtin_trap();
	}
};

class valptridx_untyped_utilities::index_range_trap_verbose
{
public:
	__attribute_cold
	__attribute_noreturn
	DXX_VALPTRIDX_WARN_CALL_NOT_OPTIMIZED_OUT
	static void report(DXX_VALPTRIDX_REPORT_STANDARD_LEADER_COMMA_R_DEFN_VARS const void *const array, const unsigned long supplied_index)
	{
		__asm__ __volatile__("" :: DXX_VALPTRIDX_REPORT_STANDARD_ASM_LOAD_COMMA_R_VARS "rm" (array), "rm" (supplied_index));
		__builtin_trap();
	}
};

class valptridx_untyped_utilities::null_pointer_trap_verbose
{
public:
	__attribute_cold
	__attribute_noreturn
	DXX_VALPTRIDX_WARN_CALL_NOT_OPTIMIZED_OUT
	static void report(DXX_VALPTRIDX_REPORT_STANDARD_LEADER_COMMA_N_DEFN_VARS)
	{
		__asm__ __volatile__("" :: DXX_VALPTRIDX_REPORT_STANDARD_ASM_LOAD_COMMA_N_VARS);
		__builtin_trap();
	}
	__attribute_cold
	__attribute_noreturn
	DXX_VALPTRIDX_WARN_CALL_NOT_OPTIMIZED_OUT
	static void report(DXX_VALPTRIDX_REPORT_STANDARD_LEADER_COMMA_R_DEFN_VARS const void *const array)
	{
		__asm__ __volatile__("" :: DXX_VALPTRIDX_REPORT_STANDARD_ASM_LOAD_COMMA_R_VARS "rm" (array));
		__builtin_trap();
	}
};

template <typename P>
class valptridx<P>::index_mismatch_exception :
	public std::logic_error
{
	DXX_INHERIT_CONSTRUCTORS(index_mismatch_exception, logic_error);
public:
	__attribute_cold
	__attribute_noreturn
	DXX_VALPTRIDX_WARN_CALL_NOT_OPTIMIZED_OUT
	static void report(DXX_VALPTRIDX_REPORT_STANDARD_LEADER_COMMA_R_DEFN_VARS const array_managed_type *, index_type, const_pointer_type, const_pointer_type);
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
	static void report(DXX_VALPTRIDX_REPORT_STANDARD_LEADER_COMMA_R_DEFN_VARS const array_managed_type *);
};

template <typename managed_type>
template <typename handle_index_mismatch>
void valptridx<managed_type>::check_index_match(DXX_VALPTRIDX_REPORT_STANDARD_LEADER_COMMA_R_DEFN_VARS const_reference_type r, index_type i, const array_managed_type &a __attribute_unused)
{
	const auto pi = &a[i];
	DXX_VALPTRIDX_CHECK(pi == &r, handle_index_mismatch, "pointer/index mismatch", &a, i, pi, &r);
}

template <typename managed_type>
template <typename handle_index_range_error, template <typename> class Compare>
typename valptridx<managed_type>::index_type valptridx<managed_type>::check_index_range(DXX_VALPTRIDX_REPORT_STANDARD_LEADER_COMMA_R_DEFN_VARS const index_type i, const array_managed_type *const a)
{
	const std::size_t ss = i;
	DXX_VALPTRIDX_CHECK(Compare<std::size_t>()(ss, array_size), handle_index_range_error, "invalid index used in array subscript", a, ss);
	return i;
}

template <typename managed_type>
template <typename handle_null_pointer>
void valptridx<managed_type>::check_null_pointer_conversion(DXX_VALPTRIDX_REPORT_STANDARD_LEADER_COMMA_R_DEFN_VARS const_pointer_type p)
{
	DXX_VALPTRIDX_CHECK(p, handle_null_pointer, "NULL pointer converted");
}

template <typename managed_type>
template <typename handle_null_pointer>
void valptridx<managed_type>::check_null_pointer(DXX_VALPTRIDX_REPORT_STANDARD_LEADER_COMMA_R_DEFN_VARS const_pointer_type p, const array_managed_type &a __attribute_unused)
{
	DXX_VALPTRIDX_CHECK(p, handle_null_pointer, "NULL pointer used", &a);
}

template <typename managed_type>
template <typename handle_index_mismatch, typename handle_index_range_error>
void valptridx<managed_type>::check_implicit_index_range_ref(DXX_VALPTRIDX_REPORT_STANDARD_LEADER_COMMA_R_DEFN_VARS const managed_type &r, const array_managed_type &a)
{
	check_explicit_index_range_ref<handle_index_mismatch, handle_index_range_error>(DXX_VALPTRIDX_REPORT_STANDARD_LEADER_COMMA_R_PASS_VARS r, static_cast<const_pointer_type>(&r) - static_cast<const_pointer_type>(&a.front()), a);
}

template <typename managed_type>
template <typename handle_index_mismatch, typename handle_index_range_error>
void valptridx<managed_type>::check_explicit_index_range_ref(DXX_VALPTRIDX_REPORT_STANDARD_LEADER_COMMA_R_DEFN_VARS const_reference_type &r, std::size_t i, const array_managed_type &a)
{
	check_index_match<handle_index_mismatch>(DXX_VALPTRIDX_REPORT_STANDARD_LEADER_COMMA_R_PASS_VARS r, i, a);
	check_index_range<handle_index_range_error>(DXX_VALPTRIDX_REPORT_STANDARD_LEADER_COMMA_R_PASS_VARS i, &a);
}

template <typename managed_type>
class valptridx<managed_type>::partial_policy::require_valid
{
public:
	static constexpr std::false_type allow_nullptr{};
	static constexpr std::false_type check_allowed_invalid_index(index_type) { return {}; }
	static constexpr bool check_nothrow_index(index_type i)
	{
		return std::less<std::size_t>()(i, array_size);
	}
};

template <typename managed_type>
class valptridx<managed_type>::partial_policy::allow_invalid
{
public:
	static constexpr std::true_type allow_nullptr{};
	static constexpr bool check_allowed_invalid_index(index_type i)
	{
		return i == static_cast<index_type>(~0);
	}
	static constexpr bool check_nothrow_index(index_type i)
	{
		return check_allowed_invalid_index(i) || require_valid::check_nothrow_index(i);
	}
};

template <typename managed_type>
constexpr std::false_type valptridx<managed_type>::partial_policy::require_valid::allow_nullptr;

template <typename managed_type>
constexpr std::true_type valptridx<managed_type>::partial_policy::allow_invalid::allow_nullptr;

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
	public partial_policy::template apply_cv_policy<std::add_const>
{
};

template <typename managed_type>
class valptridx<managed_type>::vm :
	public partial_policy::require_valid,
	public partial_policy::template apply_cv_policy<std::remove_const>
{
};

template <typename managed_type>
class valptridx<managed_type>::ic :
	public partial_policy::allow_invalid,
	public partial_policy::template apply_cv_policy<std::add_const>
{
};

template <typename managed_type>
class valptridx<managed_type>::im :
	public partial_policy::allow_invalid,
	public partial_policy::template apply_cv_policy<std::remove_const>
{
};

template <typename managed_type>
template <typename policy, unsigned>
class valptridx<managed_type>::idx :
	public policy
{
	using containing_type = valptridx<managed_type>;
public:
	using policy::allow_nullptr;
	using policy::check_allowed_invalid_index;
	using index_type = typename containing_type::index_type;
	using integral_type = typename containing_type::integral_type;
	using typename policy::array_managed_type;
	idx() = delete;
	idx(const idx &) = default;
	idx(idx &&) = default;
	idx &operator=(const idx &) & = default;
	idx &operator=(idx &&) & = default;
	idx &operator=(const idx &) && = delete;
	idx &operator=(idx &&) && = delete;

	index_type get_unchecked_index() const { return m_idx; }

	template <typename rpolicy, unsigned ru>
		idx(const idx<rpolicy, ru> &rhs DXX_VALPTRIDX_REPORT_STANDARD_LEADER_COMMA_L_DECL_VARS) :
			m_idx(rhs.get_unchecked_index())
	{
		/* If moving from allow_invalid to require_valid, check range.
		 * If moving from allow_invalid to allow_invalid, no check is
		 * needed.
		 * If moving from require_valid to anything, no check is needed.
		 */
		if (!(allow_nullptr || !rhs.allow_nullptr))
			check_index_range<index_range_error_type<array_managed_type>>(DXX_VALPTRIDX_REPORT_STANDARD_LEADER_COMMA_R_PASS_VARS m_idx, nullptr);
	}
	template <typename rpolicy, unsigned ru>
		idx(idx<rpolicy, ru> &&rhs) :
			m_idx(rhs.get_unchecked_index())
	{
		/* Prevent move from allow_invalid into require_valid.  The
		 * right hand side must be saved and checked for validity before
		 * being used to initialize a require_valid type.
		 */
		static_assert(policy::allow_nullptr || !rpolicy::allow_nullptr, "cannot move from allow_invalid to require_valid");
	}
	idx(index_type i DXX_VALPTRIDX_REPORT_STANDARD_LEADER_COMMA_L_DECL_VARS) :
		m_idx(check_allowed_invalid_index(i) ? i : check_index_range<index_range_error_type<array_managed_type>>(DXX_VALPTRIDX_REPORT_STANDARD_LEADER_COMMA_R_PASS_VARS i, nullptr))
	{
	}
	idx(DXX_VALPTRIDX_REPORT_STANDARD_LEADER_COMMA_R_DEFN_VARS index_type i, array_managed_type &a) :
		m_idx(check_allowed_invalid_index(i) ? i : check_index_range<index_range_error_type<array_managed_type>>(DXX_VALPTRIDX_REPORT_STANDARD_LEADER_COMMA_R_PASS_VARS i, &a))
	{
	}
protected:
	template <integral_type v>
		idx(const magic_constant<v> &, const allow_none_construction *) :
			m_idx(v)
	{
		static_assert(!allow_nullptr, "allow_none_construction used where nullptr was already legal");
		static_assert(static_cast<std::size_t>(v) >= array_size, "allow_none_construction used with valid index");
	}
	idx(DXX_VALPTRIDX_REPORT_STANDARD_LEADER_COMMA_R_DEFN_VARS index_type i, array_managed_type &a, const allow_end_construction *) :
		m_idx(check_index_range<index_range_error_type<array_managed_type>, std::less_equal>(DXX_VALPTRIDX_REPORT_STANDARD_LEADER_COMMA_R_PASS_VARS i, &a))
	{
	}
	idx(index_type i, array_managed_type &, const assume_nothrow_index *) :
		m_idx(i)
	{
	}
public:
	template <integral_type v>
		constexpr idx(const magic_constant<v> &) :
			m_idx(v)
	{
		static_assert(allow_nullptr || static_cast<std::size_t>(v) < array_size, "invalid magic index not allowed for this policy");
	}
	template <typename rpolicy, unsigned ru>
		bool operator==(const idx<rpolicy, ru> &rhs) const
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
	idx &operator++()
	{
		++ m_idx;
		return *this;
	}
};

template <typename managed_type>
template <typename policy, unsigned>
class valptridx<managed_type>::ptr :
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

	ptr() = delete;
	/* Override template matches to make same-type copy/move trivial */
	ptr(const ptr &) = default;
	ptr(ptr &&) = default;
	ptr &operator=(const ptr &) & = default;
	ptr &operator=(ptr &&) & = default;
	ptr &operator=(const ptr &) && = delete;
	ptr &operator=(ptr &&) && = delete;

	pointer_type get_unchecked_pointer() const { return m_ptr; }
	pointer_type get_nonnull_pointer(DXX_VALPTRIDX_REPORT_STANDARD_LEADER_COMMA_N_DECL_VARS) const
	{
		/* If !allow_nullptr, assume nullptr was caught at construction.  */
		if (allow_nullptr)
			check_null_pointer_conversion<null_pointer_error_type<array_managed_type>>(DXX_VALPTRIDX_REPORT_STANDARD_LEADER_COMMA_R_PASS_VARS m_ptr);
		return m_ptr;
	}
	reference_type get_checked_reference(DXX_VALPTRIDX_REPORT_STANDARD_LEADER_COMMA_N_DECL_VARS) const
	{
		return *get_nonnull_pointer(DXX_VALPTRIDX_REPORT_STANDARD_LEADER_COMMA_R_PASS_VA());
	}

	ptr(std::nullptr_t) :
		m_ptr(nullptr)
	{
		static_assert(allow_nullptr, "nullptr construction not allowed for this policy");
	}
	template <integral_type v>
		ptr(const magic_constant<v> &) :
			m_ptr(nullptr)
	{
		static_assert(static_cast<std::size_t>(v) >= array_size, "valid magic index requires an array");
		static_assert(allow_nullptr || static_cast<std::size_t>(v) < array_size, "invalid magic index not allowed for this policy");
	}
	template <integral_type v>
		ptr(const magic_constant<v> &, array_managed_type &a) :
			m_ptr(&a[v])
	{
		static_assert(static_cast<std::size_t>(v) < array_size, "valid magic index required when using array");
	}
	template <typename rpolicy, unsigned ru>
		ptr(const ptr<rpolicy, ru> &rhs DXX_VALPTRIDX_REPORT_STANDARD_LEADER_COMMA_L_DECL_VARS) :
			m_ptr(rhs.get_unchecked_pointer())
	{
		if (!(allow_nullptr || !rhs.allow_nullptr))
			check_null_pointer_conversion<null_pointer_error_type<array_managed_type>>(DXX_VALPTRIDX_REPORT_STANDARD_LEADER_COMMA_R_PASS_VARS m_ptr);
	}
	template <typename rpolicy, unsigned ru>
		ptr(ptr<rpolicy, ru> &&rhs) :
			m_ptr(rhs.get_unchecked_pointer())
	{
		/* Prevent move from allow_invalid into require_valid.  The
		 * right hand side must be saved and checked for validity before
		 * being used to initialize a require_valid type.
		 */
		static_assert(policy::allow_nullptr || !rpolicy::allow_nullptr, "cannot move from allow_invalid to require_valid");
	}
	ptr(index_type i) = delete;
	ptr(DXX_VALPTRIDX_REPORT_STANDARD_LEADER_COMMA_R_DEFN_VARS index_type i, array_managed_type &a) :
		m_ptr(check_allowed_invalid_index(i) ? nullptr : &a[check_index_range<index_range_error_type<array_managed_type>>(DXX_VALPTRIDX_REPORT_STANDARD_LEADER_COMMA_R_PASS_VARS i, &a)])
	{
	}
	ptr(DXX_VALPTRIDX_REPORT_STANDARD_LEADER_COMMA_R_DEFN_VARS index_type i, array_managed_type &a, const allow_end_construction *) :
		m_ptr(&a[check_index_range<index_range_error_type<array_managed_type>, std::less_equal>(DXX_VALPTRIDX_REPORT_STANDARD_LEADER_COMMA_R_PASS_VARS i, &a)])
	{
	}
	ptr(index_type i, array_managed_type &a, const assume_nothrow_index *) :
		m_ptr(&a[i])
	{
	}
	ptr(pointer_type p) = delete;
	ptr(DXX_VALPTRIDX_REPORT_STANDARD_LEADER_COMMA_R_DEFN_VARS pointer_type p, array_managed_type &a) :
		/* No array consistency check here, since some code incorrectly
		 * defines instances of `object` outside the Objects array, then
		 * passes pointers to those instances to this function.
		 */
		m_ptr(p)
	{
		if (!allow_nullptr)
			check_null_pointer<null_pointer_error_type<array_managed_type>>(DXX_VALPTRIDX_REPORT_STANDARD_LEADER_COMMA_R_PASS_VARS p, a);
	}
	ptr(DXX_VALPTRIDX_REPORT_STANDARD_LEADER_COMMA_R_DEFN_VARS reference_type r, array_managed_type &a) :
		m_ptr((check_implicit_index_range_ref<index_mismatch_error_type<array_managed_type>, index_range_error_type<array_managed_type>>(DXX_VALPTRIDX_REPORT_STANDARD_LEADER_COMMA_R_PASS_VARS r, a), &r))
	{
	}
	ptr(DXX_VALPTRIDX_REPORT_STANDARD_LEADER_COMMA_R_DEFN_VARS reference_type r, index_type i, array_managed_type &a) :
		m_ptr((check_explicit_index_range_ref<index_mismatch_error_type<array_managed_type>, index_range_error_type<array_managed_type>>(DXX_VALPTRIDX_REPORT_STANDARD_LEADER_COMMA_R_PASS_VARS r, i, a), &r))
	{
	}

	operator mutable_pointer_type() const { return m_ptr; }	// implicit pointer conversion deprecated
	operator const_pointer_type() const { return m_ptr; }	// implicit pointer conversion deprecated
	pointer_type operator->() const &
	{
		return get_nonnull_pointer();
	}
	operator reference_type() const &
	{
		return get_checked_reference();
	}
	reference_type operator*() const &
	{
		return get_checked_reference();
	}
	explicit operator bool() const &
	{
		return !(*this == nullptr);
	}
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
		bool operator==(const ptr<rpolicy, ru> &rhs) const
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
	ptr &operator++()
	{
		++ m_ptr;
		return *this;
	}
	ptr(const allow_none_construction *) :
		m_ptr(nullptr)
	{
		static_assert(!allow_nullptr, "allow_none_construction used where nullptr was already legal");
	}
};

template <typename managed_type>
template <typename policy>
class valptridx<managed_type>::ptridx :
	public prohibit_void_ptr<ptridx<policy>>,
	public ptr<policy, 1>,
	public idx<policy, 1>
{
public:
	typedef ptr<policy, 1> vptr_type;
	typedef idx<policy, 1> vidx_type;
	using typename vidx_type::array_managed_type;
	using index_type = typename vidx_type::index_type;
	using typename vidx_type::integral_type;
	using typename vptr_type::pointer_type;
	using vidx_type::operator==;
	using vptr_type::operator==;
	ptridx(const ptridx &) = default;
	ptridx(ptridx &&) = default;
	ptridx &operator=(const ptridx &) & = default;
	ptridx &operator=(ptridx &&) & = default;
	ptridx &operator=(const ptridx &) && = delete;
	ptridx &operator=(ptridx &&) && = delete;
	ptridx(std::nullptr_t) = delete;
	/* Prevent implicit conversion.  Require use of the factory function.
	 */
	ptridx(pointer_type p) = delete;
	template <typename rpolicy>
		ptridx(const ptridx<rpolicy> &rhs DXX_VALPTRIDX_REPORT_STANDARD_LEADER_COMMA_L_DECL_VARS) :
			vptr_type(static_cast<const typename ptridx<rpolicy>::vptr_type &>(rhs) DXX_VALPTRIDX_REPORT_STANDARD_LEADER_COMMA_L_PASS_VARS),
			vidx_type(static_cast<const typename ptridx<rpolicy>::vidx_type &>(rhs) DXX_VALPTRIDX_REPORT_STANDARD_LEADER_COMMA_L_PASS_VARS)
	{
	}
	template <typename rpolicy>
		ptridx(ptridx<rpolicy> &&rhs DXX_VALPTRIDX_REPORT_STANDARD_LEADER_COMMA_L_DECL_VARS) :
			vptr_type(static_cast<typename ptridx<rpolicy>::vptr_type &&>(rhs) DXX_VALPTRIDX_REPORT_STANDARD_LEADER_COMMA_L_PASS_VARS),
			vidx_type(static_cast<typename ptridx<rpolicy>::vidx_type &&>(rhs) DXX_VALPTRIDX_REPORT_STANDARD_LEADER_COMMA_L_PASS_VARS)
	{
	}
	template <integral_type v>
		ptridx(const magic_constant<v> &m) :
			vptr_type(m),
			vidx_type(m)
	{
	}
	template <integral_type v>
		ptridx(const magic_constant<v> &m, array_managed_type &a) :
			vptr_type(m, a),
			vidx_type(m)
	{
	}
	template <integral_type v>
		ptridx(const magic_constant<v> &m, const allow_none_construction *const n) :
			vptr_type(n),
			vidx_type(m, n)
	{
	}
	ptridx(index_type i) = delete;
	ptridx(DXX_VALPTRIDX_REPORT_STANDARD_LEADER_COMMA_R_DEFN_VARS index_type i, array_managed_type &a) :
		vptr_type(DXX_VALPTRIDX_REPORT_STANDARD_LEADER_COMMA_R_PASS_VARS i, a),
		vidx_type(DXX_VALPTRIDX_REPORT_STANDARD_LEADER_COMMA_R_PASS_VARS i, a)
	{
	}
	ptridx(DXX_VALPTRIDX_REPORT_STANDARD_LEADER_COMMA_R_DEFN_VARS index_type i, array_managed_type &a, const allow_end_construction *e) :
		vptr_type(DXX_VALPTRIDX_REPORT_STANDARD_LEADER_COMMA_R_PASS_VARS i, a, e),
		vidx_type(DXX_VALPTRIDX_REPORT_STANDARD_LEADER_COMMA_R_PASS_VARS i, a, e)
	{
	}
	ptridx(index_type i, array_managed_type &a, const assume_nothrow_index *e) :
		vptr_type(i, a, e),
		vidx_type(i, a, e)
	{
	}
	ptridx(DXX_VALPTRIDX_REPORT_STANDARD_LEADER_COMMA_R_DEFN_VARS pointer_type p, array_managed_type &a) :
		/* Null pointer is never allowed when an index must be computed.
		 * Check for null, then use the reference constructor for
		 * vptr_type to avoid checking again.
		 */
		vptr_type(DXX_VALPTRIDX_REPORT_STANDARD_LEADER_COMMA_R_PASS_VARS (check_null_pointer<null_pointer_error_type<array_managed_type>>(DXX_VALPTRIDX_REPORT_STANDARD_LEADER_COMMA_R_PASS_VARS p, a), *p), a),
		vidx_type(DXX_VALPTRIDX_REPORT_STANDARD_LEADER_COMMA_R_PASS_VARS p - static_cast<pointer_type>(&a.front()), a)
	{
	}
	ptridx(DXX_VALPTRIDX_REPORT_STANDARD_LEADER_COMMA_R_DEFN_VARS pointer_type p, index_type i, array_managed_type &a) :
		vptr_type(DXX_VALPTRIDX_REPORT_STANDARD_LEADER_COMMA_R_PASS_VARS (check_null_pointer<null_pointer_error_type<array_managed_type>>(DXX_VALPTRIDX_REPORT_STANDARD_LEADER_COMMA_R_PASS_VARS p, a), *p), i, a),
		vidx_type(DXX_VALPTRIDX_REPORT_STANDARD_LEADER_COMMA_R_PASS_VARS i, a)
	{
	}
	ptridx absolute_sibling(const index_type i DXX_VALPTRIDX_REPORT_STANDARD_LEADER_COMMA_L_DECL_VARS) const
	{
		static_assert(!policy::allow_nullptr, "absolute_sibling not allowed with invalid ptridx");
		ptridx r(*this);
		r.m_ptr += check_index_range<index_range_error_type<array_managed_type>>(DXX_VALPTRIDX_REPORT_STANDARD_LEADER_COMMA_R_PASS_VARS i, nullptr) - this->m_idx;
		r.m_idx = i;
		return r;
	}
	template <typename rpolicy>
		bool operator==(const ptridx<rpolicy> &rhs) const
		{
			return vptr_type::operator==(static_cast<const typename ptridx<rpolicy>::vptr_type &>(rhs));
		}
	template <typename R>
		bool operator!=(const R &rhs) const
		{
			return !(*this == rhs);
		}
protected:
	ptridx &operator++()
	{
		vptr_type::operator++();
		vidx_type::operator++();
		return *this;
	}
};

template <typename managed_type>
template <typename guarded_type>
class valptridx<managed_type>::guarded
{
	static_assert(std::is_trivially_destructible<guarded_type>::value, "non-trivial destructor found for guarded_type");
	enum state : uint8_t
	{
		/* empty - the untrusted input was invalid, so no guarded_type
		 * exists
		 */
		empty,
		/* initialized - the untrusted input was valid, so a
		 * guarded_type type exists, but the calling code has not yet
		 * tested the state of this guarded<P>
		 */
		initialized,
		/* checked - the untrusted input was valid, and the calling code
		 * has called operator bool()
		 */
		checked,
	};
	union {
		state m_dummy;
		guarded_type m_value;
	};
	mutable state m_state;
public:
	guarded(std::nullptr_t) :
		m_dummy(), m_state(empty)
	{
	}
	guarded(guarded_type &&v) :
		m_value(std::move(v)), m_state(initialized)
	{
	}
	__attribute_warn_unused_result
	explicit operator bool() const
	{
		/*
		 * If no contained guarded_type exists, return false.
		 * Otherwise, record that the result has been tested and then
		 * return true.  operator*() uses m_state to enforce that the
		 * result is tested.
		 */
		if (m_state == empty)
			return false;
		m_state = checked;
		return true;
	}
	__attribute_warn_unused_result
	guarded_type operator*() const &
	{
		/*
		 * Correct code will always execute as if this method was just
		 * the return statement, with none of the sanity checks.  The
		 * checks are present to catch misuse of this type, preferably
		 * at compile-time, but at least at runtime.
		 */
#define DXX_VALPTRIDX_GUARDED_OBJECT_NO	"access to guarded object that does not exist"
#define DXX_VALPTRIDX_GUARDED_OBJECT_MAYBE	"access to guarded object that may not exist"
#ifdef DXX_CONSTANT_TRUE
		{
			/*
			 * If the validity has not been verified by the caller, then
			 * fail.  Choose an error message and function name
			 * based on whether the contained type provably does not
			 * exist.  It provably does not exist if this call is on a
			 * path where operator bool() returned false.  It
			 * conditionally might not exist if this call is on a path
			 * where operator bool() has not been called.
			 */
			if (DXX_CONSTANT_TRUE(m_state == empty))
				DXX_ALWAYS_ERROR_FUNCTION(guarded_accessed_empty, DXX_VALPTRIDX_GUARDED_OBJECT_NO);
			/* If the contained object might not exist: */
			if (DXX_CONSTANT_TRUE(m_state == initialized))
				DXX_ALWAYS_ERROR_FUNCTION(guarded_accessed_unchecked, DXX_VALPTRIDX_GUARDED_OBJECT_MAYBE);
		}
#endif
		/*
		 * If the compiler does not offer constant truth analysis
		 * (perhaps because of insufficient optimization), then emit a
		 * runtime check for whether the guarded_type exists.
		 *
		 * This test can throw even if the contained object is valid, if
		 * the caller did not first validate that the contained object
		 * is valid.  This restriction is necessary since inputs are
		 * usually valid even when untested, so throwing only on state
		 * `empty` would allow incorrect usage to persist in the code
		 * until someone happened to receive an invalid input from an
		 * untrusted source.
		 */
		if (m_state != checked)
			throw std::logic_error(m_state == empty ? DXX_VALPTRIDX_GUARDED_OBJECT_NO : DXX_VALPTRIDX_GUARDED_OBJECT_MAYBE);
#undef DXX_VALPTRIDX_GUARDED_OBJECT_MAYBE
#undef DXX_VALPTRIDX_GUARDED_OBJECT_NO
		return m_value;
	}
	guarded_type operator*() const && = delete;
};

template <typename managed_type>
class valptridx<managed_type>::array_managed_type :
	public array_base_count_type,
	public array_base_storage_type
{
public:
	/*
	 * Expose the union members that act as factory methods.  Leave the
	 * `count` union member protected.
	 */
#define DXX_VALPTRIDX_ACCESS_SUBTYPE_MEMBER_FACTORIES(MANAGED_TYPE, DERIVED_TYPE_PREFIX, CONTEXT, PISUFFIX, IVPREFIX, MCPREFIX)	\
	using array_base_count_type::IVPREFIX ## MCPREFIX ## PISUFFIX
	DXX_VALPTRIDX_FOR_EACH_PPI_TYPE(DXX_VALPTRIDX_ACCESS_SUBTYPE_MEMBER_FACTORIES,,,);
#undef DXX_VALPTRIDX_ACCESS_SUBTYPE_MEMBER_FACTORIES
	using typename array_base_storage_type::reference;
	using typename array_base_storage_type::const_reference;
	reference operator[](const integral_type &n)
		{
			return array_base_storage_type::operator[](n);
		}
	const_reference operator[](const integral_type &n) const
		{
			return array_base_storage_type::operator[](n);
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
template <typename Pc, typename Pm>
class valptridx<managed_type>::basic_ival_member_factory
{
	using containing_type = valptridx<managed_type>;
protected:
	/*
	 * These casts are well-defined:
	 * - The reinterpret_cast is defined because
	 *   `basic_ival_member_factory` is a base of a member of the
	 *   anonymous union in `array_base_count_type`.
	 * - The anonymous union in `array_base_count_type` is the only
	 *   member of that type, so its storage must be aligned to the
	 *   beginning of the object.
	 * - `basic_ival_member_factory` and its derivatives are not used
	 *   anywhere other than `array_base_count_type`, so any call to
	 *   these methods must be on an instance used in
	 *   `array_base_count_type`.
	 *
	 * - The static_cast is defined because `array_base_count_type` is a
	 *   non-virtual base of `array_managed_type`.
	 * - `array_base_count_type` is not used as a base for any class
	 *   other than `array_managed_type`, nor used freestanding, so any
	 *   instance of `array_base_count_type` must be safe to downcast to
	 *   `array_managed_type`.
	 */
	constexpr const array_managed_type &get_array() const
	{
		return static_cast<const array_managed_type &>(reinterpret_cast<const array_base_count_type &>(*this));
	}
	array_managed_type &get_array()
	{
		return static_cast<array_managed_type &>(reinterpret_cast<array_base_count_type &>(*this));
	}
	template <typename P, typename A>
		static guarded<P> check_untrusted_internal(const index_type i, A &a)
		{
			if (P::check_nothrow_index(i))
				return P(i, a, static_cast<const assume_nothrow_index *>(nullptr));
			else
				return nullptr;
		}
	template <typename P, typename T, typename A>
		static guarded<P> check_untrusted_internal(T &&, A &) = delete;
	template <typename P, typename A>
		__attribute_warn_unused_result
		/* C++ does not allow `static operator()()`, so name it
		 * `call_operator` instead.
		 */
		static P call_operator(DXX_VALPTRIDX_REPORT_STANDARD_LEADER_COMMA_R_DEFN_VARS const typename P::index_type i, A &a)
		{
			return P(DXX_VALPTRIDX_REPORT_STANDARD_LEADER_COMMA_R_PASS_VARS i, a);
		}
	template <typename P, containing_type::integral_type v, typename A>
		__attribute_warn_unused_result
		static P call_operator(DXX_VALPTRIDX_REPORT_STANDARD_LEADER_COMMA_R_DEFN_VARS const containing_type::magic_constant<v> &m, A &a)
		{
			/*
			 * All call_operator definitions must have the macro which
			 * defines filename/lineno, but the magic_constant overload
			 * has no need for them.
			 *
			 * Cast them to void to silence the warning about unused
			 * parameters.
			 */
			DXX_VALPTRIDX_REPORT_STANDARD_LEADER_COMMA_N_VOID_VARS();
			return P(m, a);
		}
	template <typename P, typename T, typename A>
		static P call_operator(DXX_VALPTRIDX_REPORT_STANDARD_LEADER_COMMA_R_DEFN_VARS T &&, A &a) = delete;
	basic_ival_member_factory() = default;
public:
	basic_ival_member_factory(const basic_ival_member_factory &) = delete;
	basic_ival_member_factory &operator=(const basic_ival_member_factory &) = delete;
	void *operator &() const = delete;
	template <typename T>
		__attribute_warn_unused_result
		guarded<Pc> check_untrusted(T &&t) const
		{
			return this->template check_untrusted_internal<Pc>(static_cast<T &&>(t), get_array());
		}
	template <typename T>
		__attribute_warn_unused_result
		guarded<Pm> check_untrusted(T &&t)
		{
			return this->template check_untrusted_internal<Pm>(static_cast<T &&>(t), get_array());
		}
	template <typename T>
		__attribute_warn_unused_result
		Pc operator()(T &&t DXX_VALPTRIDX_REPORT_STANDARD_LEADER_COMMA_L_DECL_VARS) const
		{
			return this->template call_operator<Pc>(DXX_VALPTRIDX_REPORT_STANDARD_LEADER_COMMA_R_PASS_VARS static_cast<T &&>(t), get_array());
		}
	template <typename T>
		__attribute_warn_unused_result
		Pm operator()(T &&t DXX_VALPTRIDX_REPORT_STANDARD_LEADER_COMMA_L_DECL_VARS)
		{
			return this->template call_operator<Pm>(DXX_VALPTRIDX_REPORT_STANDARD_LEADER_COMMA_R_PASS_VARS static_cast<T &&>(t), get_array());
		}
};

template <typename managed_type>
template <typename Pc, typename Pm>
class valptridx<managed_type>::basic_vval_member_factory :
	public basic_ival_member_factory<Pc, Pm>
{
protected:
	using basic_ival_member_factory<Pc, Pm>::get_array;
	using basic_ival_member_factory<Pc, Pm>::call_operator;
	template <typename P>
		using iterator = self_return_iterator<P>;
	template <typename P, typename policy, typename A>
		static P call_operator(DXX_VALPTRIDX_REPORT_STANDARD_LEADER_COMMA_R_DEFN_VARS const valptridx<managed_type>::idx<policy, 0> i, A &a)
		{
			return P(DXX_VALPTRIDX_REPORT_STANDARD_LEADER_COMMA_R_PASS_VARS i, a);
		}
	template <typename P>
		__attribute_warn_unused_result
		static P call_operator(DXX_VALPTRIDX_REPORT_STANDARD_LEADER_COMMA_R_DEFN_VARS const typename P::mutable_pointer_type p, typename P::array_managed_type &a)
		{
			return P(DXX_VALPTRIDX_REPORT_STANDARD_LEADER_COMMA_R_PASS_VARS p, a);
		}
	/*
	 * When P is a const type, enable an overload that takes a const
	 * array.  Otherwise, enable a deleted overload that takes a mutable
	 * array.  This provides a slightly clearer error when trying to
	 * pass a const pointer to a mutable factory.
	 */
	template <typename P>
		__attribute_warn_unused_result
		static typename std::enable_if<std::is_same<const array_managed_type, typename P::array_managed_type>::value, P>::type call_operator(DXX_VALPTRIDX_REPORT_STANDARD_LEADER_COMMA_R_DEFN_VARS const typename P::const_pointer_type p, const array_managed_type &a)
		{
			return P(DXX_VALPTRIDX_REPORT_STANDARD_LEADER_COMMA_R_PASS_VARS p, a);
		}
	template <typename P>
		static typename std::enable_if<!std::is_same<const array_managed_type, typename P::array_managed_type>::value, P>::type call_operator(DXX_VALPTRIDX_REPORT_STANDARD_LEADER_COMMA_R_DEFN_VARS const typename P::const_pointer_type p, array_managed_type &a) = delete;
	template <typename P, typename A>
		static iterator<P> end_internal(DXX_VALPTRIDX_REPORT_STANDARD_LEADER_COMMA_R_DEFN_VARS A &a)
		{
			return P(DXX_VALPTRIDX_REPORT_STANDARD_LEADER_COMMA_R_PASS_VARS static_cast<index_type>(a.get_count()), a, static_cast<const allow_end_construction *>(nullptr));
		}
public:
	__attribute_warn_unused_result
	typename array_base_storage_type::size_type count() const
	{
		return get_array().get_count();
	}
	__attribute_warn_unused_result
	typename array_base_storage_type::size_type size() const
	{
		return get_array().size();
	}
	template <typename T>
		__attribute_warn_unused_result
		Pc operator()(T &&t DXX_VALPTRIDX_REPORT_STANDARD_LEADER_COMMA_L_DECL_VARS) const
		{
			return this->template call_operator<Pc>(DXX_VALPTRIDX_REPORT_STANDARD_LEADER_COMMA_R_PASS_VARS static_cast<T &&>(t), get_array());
		}
	template <typename T>
		__attribute_warn_unused_result
		Pm operator()(T &&t DXX_VALPTRIDX_REPORT_STANDARD_LEADER_COMMA_L_DECL_VARS)
		{
			return this->template call_operator<Pm>(DXX_VALPTRIDX_REPORT_STANDARD_LEADER_COMMA_R_PASS_VARS static_cast<T &&>(t), get_array());
		}
	__attribute_warn_unused_result
	iterator<Pc> begin() const
	{
		return Pc(valptridx<managed_type>::magic_constant<0>(), get_array());
	}
	__attribute_warn_unused_result
	iterator<Pm> begin()
	{
		return Pm(valptridx<managed_type>::magic_constant<0>(), get_array());
	}
	__attribute_warn_unused_result
	iterator<Pc> end(DXX_VALPTRIDX_REPORT_STANDARD_LEADER_COMMA_N_DECL_VARS) const
	{
		return this->template end_internal<Pc>(DXX_VALPTRIDX_REPORT_STANDARD_LEADER_COMMA_R_PASS_VARS get_array());
	}
	__attribute_warn_unused_result
	iterator<Pm> end(DXX_VALPTRIDX_REPORT_STANDARD_LEADER_COMMA_N_DECL_VARS)
	{
		return this->template end_internal<Pm>(DXX_VALPTRIDX_REPORT_STANDARD_LEADER_COMMA_R_PASS_VARS get_array());
	}
};

#define DXX_VALPTRIDX_DEFINE_FACTORY(MANAGED_TYPE, GLOBAL_FACTORY, GLOBAL_ARRAY, MEMBER_FACTORY)	\
	__attribute_unused static auto &GLOBAL_FACTORY = GLOBAL_ARRAY.MEMBER_FACTORY

#define DXX_VALPTRIDX_DEFINE_GLOBAL_FACTORY(MANAGED_TYPE, DERIVED_TYPE_PREFIX, GLOBAL_ARRAY, PISUFFIX, IVPREFIX, MCPREFIX)	\
	DXX_VALPTRIDX_DEFINE_FACTORY(MANAGED_TYPE, IVPREFIX ## MCPREFIX ## DERIVED_TYPE_PREFIX ## PISUFFIX, GLOBAL_ARRAY, IVPREFIX ## MCPREFIX ## PISUFFIX)

#define DXX_VALPTRIDX_DEFINE_GLOBAL_FACTORIES(MANAGED_TYPE,DERIVED_TYPE_PREFIX,GLOBAL_ARRAY)	\
	extern MANAGED_TYPE ## _array GLOBAL_ARRAY;	\
	namespace { namespace {	\
	DXX_VALPTRIDX_FOR_EACH_PPI_TYPE(DXX_VALPTRIDX_DEFINE_GLOBAL_FACTORY, MANAGED_TYPE, DERIVED_TYPE_PREFIX, GLOBAL_ARRAY);	\
	} }

