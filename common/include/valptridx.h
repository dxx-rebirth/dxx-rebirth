/*
 * This file is part of the DXX-Rebirth project <https://www.dxx-rebirth.com/>.
 * It is copyright by its individual contributors, as recorded in the
 * project's Git history.  See COPYING.txt at the top level for license
 * terms and a link to the Git history.
 */
#pragma once

#include <stdexcept>
#include <string>
#include <type_traits>
#include "fwd-valptridx.h"
#include "pack.h"
#include "compiler-poison.h"
#include "selfiter.h"

#ifdef DXX_CONSTANT_TRUE
#ifdef DXX_HAVE_ATTRIBUTE_WARNING
/* This causes many warnings because some conversions are not checked for
 * safety.  Eliminating the warnings by changing the call sites to check first
 * would be a useful improvement.
 */
//#define DXX_VALPTRIDX_WARN_CALL_NOT_OPTIMIZED_OUT __attribute__((__warning__("call not eliminated")))
#endif
#endif

#define DXX_VALPTRIDX_CHECK(SUCCESS_CONDITION,ERROR,...)	\
	static_cast<void>(	\
		static_cast<bool>(SUCCESS_CONDITION) || (ERROR::report(DXX_VALPTRIDX_REPORT_STANDARD_LEADER_COMMA_R_PASS_VA(__VA_ARGS__)), 0)	\
	)	\

#ifndef DXX_VALPTRIDX_WARN_CALL_NOT_OPTIMIZED_OUT
#define DXX_VALPTRIDX_WARN_CALL_NOT_OPTIMIZED_OUT
#endif

template <typename managed_type>
class valptridx<managed_type>::array_base_count_type
{
protected:
	/* Use the smallest size that will store all legal values.  Currently,
	 * no user has a maximum size that needs more than uint16_t, so reject
	 * attempts to use that size.  This protects against silently getting
	 * the wrong result if a future change raises the maximum size above
	 * uint16_t.
	 */
	using count_type = typename std::conditional<(array_size <= UINT8_MAX), uint8_t, typename std::conditional<(array_size <= UINT16_MAX), uint16_t, void>::type>::type;
	union {
		count_type count;
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
	count_type get_count() const
	{
		return count;
	}
	void set_count(const count_type c)
	{
		count = c;
	}
};

template <
	typename INTEGRAL_TYPE,
	std::size_t array_size_value,
	valptridx_detail::untyped_utilities::report_error_style report_const_error_value,
	valptridx_detail::untyped_utilities::report_error_style report_mutable_error_value
	>
constexpr std::integral_constant<std::size_t, array_size_value> valptridx_detail::specialized_type_parameters<INTEGRAL_TYPE, array_size_value, report_const_error_value, report_mutable_error_value>::array_size;

class valptridx_detail::untyped_utilities::report_error_undefined
{
public:
	__attribute_cold
	DXX_VALPTRIDX_WARN_CALL_NOT_OPTIMIZED_OUT
	static void report(...)
	{
	}
};

class valptridx_detail::untyped_utilities::report_error_trap_terse
{
public:
	/* Accept and discard any arguments, to encourage the compiler to discard
	 * as dead any values that exist only as arguments to `report()`.
	 */
	[[noreturn]]
	__attribute_cold
	DXX_VALPTRIDX_WARN_CALL_NOT_OPTIMIZED_OUT
	static void report(...)
	{
		__builtin_trap();
	}
};

class valptridx_detail::untyped_utilities::index_mismatch_trap_verbose
{
public:
	[[noreturn]]
	__attribute_cold
	DXX_VALPTRIDX_WARN_CALL_NOT_OPTIMIZED_OUT
	static void report(DXX_VALPTRIDX_REPORT_STANDARD_LEADER_COMMA_R_DEFN_VARS const void *const array, const unsigned long supplied_index, const void *const expected_pointer, const void *const actual_pointer)
	{
		/* Load each of the arguments into storage before executing the trap,
		 * so that inspection of those locations in the core dump can readily
		 * retrieve these values, even if the values would otherwise be unused
		 * and optimized out.
		 */
		__asm__ __volatile__("" :: DXX_VALPTRIDX_REPORT_STANDARD_ASM_LOAD_COMMA_R_VARS "rm" (array), "rm" (supplied_index), "rm" (expected_pointer), "rm" (actual_pointer));
		__builtin_trap();
	}
};

class valptridx_detail::untyped_utilities::index_range_trap_verbose
{
public:
	[[noreturn]]
	__attribute_cold
	DXX_VALPTRIDX_WARN_CALL_NOT_OPTIMIZED_OUT
	static void report(DXX_VALPTRIDX_REPORT_STANDARD_LEADER_COMMA_R_DEFN_VARS const void *const array, const unsigned long supplied_index)
	{
		__asm__ __volatile__("" :: DXX_VALPTRIDX_REPORT_STANDARD_ASM_LOAD_COMMA_R_VARS "rm" (array), "rm" (supplied_index));
		__builtin_trap();
	}
};

class valptridx_detail::untyped_utilities::null_pointer_trap_verbose
{
public:
	[[noreturn]]
	__attribute_cold
	DXX_VALPTRIDX_WARN_CALL_NOT_OPTIMIZED_OUT
	static void report(DXX_VALPTRIDX_REPORT_STANDARD_LEADER_COMMA_N_DEFN_VARS)
	{
		__asm__ __volatile__("" :: DXX_VALPTRIDX_REPORT_STANDARD_ASM_LOAD_COMMA_N_VARS);
		__builtin_trap();
	}
	[[noreturn]]
	__attribute_cold
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
	using std::logic_error::logic_error;
public:
	[[noreturn]]
	__attribute_cold
	DXX_VALPTRIDX_WARN_CALL_NOT_OPTIMIZED_OUT
	static void report(DXX_VALPTRIDX_REPORT_STANDARD_LEADER_COMMA_R_DEFN_VARS const array_managed_type *, index_type, const_pointer_type, const_pointer_type);
};

template <typename P>
class valptridx<P>::index_range_exception :
	public std::out_of_range
{
	using std::out_of_range::out_of_range;
public:
	[[noreturn]]
	__attribute_cold
	DXX_VALPTRIDX_WARN_CALL_NOT_OPTIMIZED_OUT
	static void report(DXX_VALPTRIDX_REPORT_STANDARD_LEADER_COMMA_R_DEFN_VARS const array_managed_type *, long);
};

template <typename P>
class valptridx<P>::null_pointer_exception :
	public std::logic_error
{
	using std::logic_error::logic_error;
public:
	[[noreturn]]
	__attribute_cold
	DXX_VALPTRIDX_WARN_CALL_NOT_OPTIMIZED_OUT
	static void report(DXX_VALPTRIDX_REPORT_STANDARD_LEADER_COMMA_N_DEFN_VARS);
	[[noreturn]]
	__attribute_cold
	DXX_VALPTRIDX_WARN_CALL_NOT_OPTIMIZED_OUT
	static void report(DXX_VALPTRIDX_REPORT_STANDARD_LEADER_COMMA_R_DEFN_VARS const array_managed_type *);
};

template <typename managed_type>
template <typename handle_index_mismatch>
void valptridx<managed_type>::check_index_match(DXX_VALPTRIDX_REPORT_STANDARD_LEADER_COMMA_R_DEFN_VARS const_reference_type r, index_type i, const array_managed_type &a __attribute_unused)
{
	const auto pi = &a[i];
	DXX_VALPTRIDX_CHECK(pi == &r, handle_index_mismatch, &a, i, pi, &r);
}

template <typename managed_type>
template <typename handle_index_range_error, template <typename> class Compare>
typename valptridx<managed_type>::index_type valptridx<managed_type>::check_index_range(DXX_VALPTRIDX_REPORT_STANDARD_LEADER_COMMA_R_DEFN_VARS const index_type i, const array_managed_type *const a)
{
	std::size_t si;
	if constexpr (std::is_enum<index_type>::value)
		si = static_cast<std::size_t>(i);
	else
		si = i;
	check_index_range_size<handle_index_range_error, Compare>(DXX_VALPTRIDX_REPORT_STANDARD_LEADER_COMMA_R_PASS_VARS si, a);
	return i;
}

template <typename managed_type>
template <typename handle_index_range_error, template <typename> class Compare>
typename valptridx<managed_type>::index_type valptridx<managed_type>::check_index_range_size(DXX_VALPTRIDX_REPORT_STANDARD_LEADER_COMMA_R_DEFN_VARS const std::size_t s, const array_managed_type *const a)
{
	DXX_VALPTRIDX_CHECK(Compare<std::size_t>()(s, array_size), handle_index_range_error, a, s);
	/* This cast will truncate the value to fit in index_type, which is
	 * normally smaller than std::size_t.  However, truncation is legal
	 * here, because (1) DXX_VALPTRIDX_CHECK would trap on an index that
	 * was outside the array size and (2) index_type can represent any
	 * valid size in the array.  Thus, if DXX_VALPTRIDX_CHECK did not
	 * trap[1], the truncation cannot change the value of the index.
	 *
	 * [1] If valptridx was built without index validation, then no trap
	 * would be issued even for invalid data.  Validation-disabled builds
	 * are permitted to exhibit undefined behavior in cases where the
	 * validation-enabled build would have trapped.
	 */
	return static_cast<index_type>(s);
}

template <typename managed_type>
template <typename handle_null_pointer>
void valptridx<managed_type>::check_null_pointer_conversion(DXX_VALPTRIDX_REPORT_STANDARD_LEADER_COMMA_R_DEFN_VARS const_pointer_type p)
{
	DXX_VALPTRIDX_CHECK(p, handle_null_pointer);
}

template <typename managed_type>
template <typename handle_null_pointer>
void valptridx<managed_type>::check_null_pointer(DXX_VALPTRIDX_REPORT_STANDARD_LEADER_COMMA_R_DEFN_VARS const_pointer_type p, const array_managed_type &a __attribute_unused)
{
	DXX_VALPTRIDX_CHECK(p, handle_null_pointer, &a);
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
	const auto ii = check_index_range_size<handle_index_range_error>(DXX_VALPTRIDX_REPORT_STANDARD_LEADER_COMMA_R_PASS_VARS i, &a);
	check_index_match<handle_index_mismatch>(DXX_VALPTRIDX_REPORT_STANDARD_LEADER_COMMA_R_PASS_VARS r, ii, a);
}

template <typename managed_type>
class valptridx<managed_type>::partial_policy::require_valid
{
public:
	static constexpr bool allow_nullptr{false};
	[[nodiscard]]
	static constexpr std::false_type check_allowed_invalid_index(index_type) { return {}; }
	[[nodiscard]]
	static constexpr bool check_nothrow_index(index_type i)
	{
		return std::less<std::size_t>()(static_cast<std::size_t>(i), array_size);
	}
};

template <typename managed_type>
class valptridx<managed_type>::partial_policy::allow_invalid
{
public:
	static constexpr bool allow_nullptr{true};
	[[nodiscard]]
	static constexpr bool check_allowed_invalid_index(index_type i)
	{
		return i == static_cast<index_type>(~0);
	}
	[[nodiscard]]
	static constexpr bool check_nothrow_index(index_type i)
	{
		return check_allowed_invalid_index(i) || require_valid::check_nothrow_index(i);
	}
};

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
template <typename policy>
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

	/* If moving from allow_invalid to allow_invalid, no check is
	 * needed.
	 * If moving from require_valid to anything, no check is needed.
	 */
	template <typename rpolicy>
		requires(
			allow_nullptr || !idx<rpolicy>::allow_nullptr
		)
		idx(const idx<rpolicy> &rhs) :
			m_idx{rhs.get_unchecked_index()}
	{
	}
	template <typename rpolicy>
		requires(
			!(allow_nullptr || !idx<rpolicy>::allow_nullptr)
		)
		idx(const idx<rpolicy> &rhs DXX_VALPTRIDX_REPORT_STANDARD_LEADER_COMMA_L_DECL_VARS) :
		/* If moving from allow_invalid to require_valid, check range.
		 */
			m_idx{check_index_range<index_range_error_type<array_managed_type>>(DXX_VALPTRIDX_REPORT_STANDARD_LEADER_COMMA_R_PASS_VARS rhs.get_unchecked_index(), nullptr)}
	{
	}
	template <typename rpolicy>
		idx(idx<rpolicy> &&rhs) :
			m_idx{rhs.get_unchecked_index()}
	{
		/* Prevent move from allow_invalid into require_valid.  The
		 * right hand side must be saved and checked for validity before
		 * being used to initialize a require_valid type.
		 */
		static_assert(allow_nullptr || !idx<rpolicy>::allow_nullptr, "cannot move from allow_invalid to require_valid");
	}
	idx(index_type i DXX_VALPTRIDX_REPORT_STANDARD_LEADER_COMMA_L_DECL_VARS) :
		m_idx{check_allowed_invalid_index(i) ? i : check_index_range<index_range_error_type<array_managed_type>>(DXX_VALPTRIDX_REPORT_STANDARD_LEADER_COMMA_R_PASS_VARS i, nullptr)}
	{
	}
	idx(DXX_VALPTRIDX_REPORT_STANDARD_LEADER_COMMA_R_DEFN_VARS index_type i, array_managed_type &a) :
		m_idx{check_allowed_invalid_index(i) ? i : check_index_range<index_range_error_type<array_managed_type>>(DXX_VALPTRIDX_REPORT_STANDARD_LEADER_COMMA_R_PASS_VARS i, &a)}
	{
	}
protected:
	template <integral_type v>
		idx(const magic_constant<v> &, allow_none_construction) :
			m_idx{v}
	{
		/* Prevent using `allow_none_construction` when the type permits
		 * use of an invalid index.  `allow_none_construction` is intended
		 * to override the validity requirement on types that require a
		 * valid index, for the limited purpose of constructing a sentinel.
		 */
		static_assert(!allow_nullptr, "allow_none_construction used where nullptr was already legal");
		/* Prevent using `allow_none_construction` with a valid index.
		 * Valid indexes should not use the `allow_none_construction`
		 * override.
		 */
		static_assert(static_cast<std::size_t>(v) >= array_size, "allow_none_construction used with valid index");
	}
	idx(DXX_VALPTRIDX_REPORT_STANDARD_LEADER_COMMA_R_DEFN_VARS index_type i, array_managed_type &a, allow_end_construction) :
		m_idx{check_index_range<index_range_error_type<array_managed_type>, std::less_equal>(DXX_VALPTRIDX_REPORT_STANDARD_LEADER_COMMA_R_PASS_VARS i, &a)}
	{
	}
	idx(index_type i, assume_nothrow_index) :
		m_idx{i}
	{
	}
	idx(DXX_VALPTRIDX_REPORT_STANDARD_LEADER_COMMA_R_DEFN_VARS typename policy::pointer_type p, array_managed_type &a) :
		m_idx{check_index_range_size<index_range_error_type<array_managed_type>>(DXX_VALPTRIDX_REPORT_STANDARD_LEADER_COMMA_R_PASS_VARS p - &a.front(), &a)}
	{
	}
public:
	template <integral_type v>
		constexpr idx(const magic_constant<v> &) :
			m_idx(v)
	{
		/* If the policy requires a valid index (indicated by
		 * `allow_nullptr == false`), then require that the magic index be
		 * valid.
		 */
		static_assert(allow_nullptr || static_cast<std::size_t>(v) < array_size, "invalid magic index not allowed for this policy");
	}
	template <typename rpolicy>
		constexpr bool operator==(const idx<rpolicy> &rhs) const
		{
			return m_idx == rhs.get_unchecked_index();
		}
	constexpr bool operator==(const index_type &i) const
	{
		return m_idx == i;
	}
	template <integral_type v>
		constexpr bool operator==(const magic_constant<v> &) const
		{
			static_assert(allow_nullptr || static_cast<std::size_t>(v) < array_size, "invalid magic index not allowed for this policy");
			return m_idx == v;
		}
	operator index_type() const
	{
		return m_idx;
	}
protected:
	index_type m_idx;
	idx &operator++()
	{
		if constexpr (std::is_enum<index_type>::value)
			m_idx = static_cast<index_type>(1u + static_cast<typename std::underlying_type<index_type>::type>(m_idx));
		else
			++ m_idx;
		return *this;
	}
	idx &operator--()
	{
		if constexpr (std::is_enum<index_type>::value)
			m_idx = static_cast<index_type>(static_cast<typename std::underlying_type<index_type>::type>(m_idx) - 1u);
		else
			-- m_idx;
		return *this;
	}
};

template <typename managed_type>
template <typename policy>
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
		const auto p = m_ptr;
		if constexpr (allow_nullptr)
			check_null_pointer_conversion<null_pointer_error_type<array_managed_type>>(DXX_VALPTRIDX_REPORT_STANDARD_LEADER_COMMA_R_PASS_VARS p);
		else
			DXX_VALPTRIDX_REPORT_STANDARD_LEADER_COMMA_N_VOID_VARS();
		return p;
	}
	reference_type get_checked_reference(DXX_VALPTRIDX_REPORT_STANDARD_LEADER_COMMA_N_DECL_VARS) const
	{
		return *get_nonnull_pointer(DXX_VALPTRIDX_REPORT_STANDARD_LEADER_COMMA_R_PASS_VA());
	}

	ptr(std::nullptr_t)
		requires(policy::allow_nullptr)	// This constructor always stores nullptr, so require a policy with `allow_nullptr == true`.
		:
		m_ptr(nullptr)
	{
	}
	template <integral_type v>
		requires(
			static_cast<std::size_t>(v) >= array_size	// Require that the index be invalid, since a valid index should compute a non-nullptr here, but with no array, no pointer can be computed.
		)
		ptr(const magic_constant<v> &) :
			ptr(nullptr)
	{
	}
	template <integral_type v>
		requires(
			static_cast<std::size_t>(v) < array_size	// valid magic index required when using array
		)
		ptr(const magic_constant<v> &, array_managed_type &a) :
			m_ptr{&a[v]}
	{
	}
	template <typename rpolicy>
		requires(
			(allow_nullptr || !ptr<rpolicy>::allow_nullptr)
		)
		ptr(const ptr<rpolicy> &rhs) :
			m_ptr{rhs.get_unchecked_pointer()}
	{
	}
	template <typename rpolicy>
		requires(
			!(allow_nullptr || !ptr<rpolicy>::allow_nullptr)
		)
		ptr(const ptr<rpolicy> &rhs DXX_VALPTRIDX_REPORT_STANDARD_LEADER_COMMA_L_DECL_VARS) :
			m_ptr{rhs.get_unchecked_pointer()}
	{
		check_null_pointer_conversion<null_pointer_error_type<array_managed_type>>(DXX_VALPTRIDX_REPORT_STANDARD_LEADER_COMMA_R_PASS_VARS m_ptr);
	}
	template <typename rpolicy>
		requires(
			(allow_nullptr || !ptr<rpolicy>::allow_nullptr)	// cannot move from allow_invalid to require_valid
		)
		ptr(ptr<rpolicy> &&rhs) :
			m_ptr{rhs.get_unchecked_pointer()}
	{
		/* Prevent move from allow_invalid into require_valid.  The
		 * right hand side must be saved and checked for validity before
		 * being used to initialize a require_valid type.
		 */
	}
	ptr(index_type i) = delete;
	ptr(DXX_VALPTRIDX_REPORT_STANDARD_LEADER_COMMA_R_DEFN_VARS index_type i, array_managed_type &a) :
		m_ptr{check_allowed_invalid_index(i) ? nullptr : &a[check_index_range<index_range_error_type<array_managed_type>>(DXX_VALPTRIDX_REPORT_STANDARD_LEADER_COMMA_R_PASS_VARS i, &a)]}
	{
	}
	ptr(DXX_VALPTRIDX_REPORT_STANDARD_LEADER_COMMA_R_DEFN_VARS index_type i, array_managed_type &a, allow_end_construction) :
		m_ptr{std::next(a.begin(), static_cast<std::size_t>(check_index_range<index_range_error_type<array_managed_type>, std::less_equal>(DXX_VALPTRIDX_REPORT_STANDARD_LEADER_COMMA_R_PASS_VARS i, &a)))}
	{
	}
	ptr(index_type i, array_managed_type &a, assume_nothrow_index) :
		m_ptr{&a[i]}
	{
	}
	ptr(pointer_type p) = delete;
	ptr(DXX_VALPTRIDX_REPORT_STANDARD_LEADER_COMMA_R_DEFN_VARS pointer_type p, array_managed_type &a) :
		/* No array consistency check here, since some code incorrectly
		 * defines instances of `object` outside the Objects array, then
		 * passes pointers to those instances to this function.
		 */
		m_ptr{p}
	{
		if constexpr (!allow_nullptr)
			check_null_pointer<null_pointer_error_type<array_managed_type>>(DXX_VALPTRIDX_REPORT_STANDARD_LEADER_COMMA_R_PASS_VARS p, a);
	}
	ptr(DXX_VALPTRIDX_REPORT_STANDARD_LEADER_COMMA_R_DEFN_VARS reference_type r, array_managed_type &a) :
		m_ptr{(check_implicit_index_range_ref<index_mismatch_error_type<array_managed_type>, index_range_error_type<array_managed_type>>(DXX_VALPTRIDX_REPORT_STANDARD_LEADER_COMMA_R_PASS_VARS r, a), &r)}
	{
	}
	ptr(DXX_VALPTRIDX_REPORT_STANDARD_LEADER_COMMA_R_DEFN_VARS reference_type r, index_type i, array_managed_type &a) :
		m_ptr{(check_explicit_index_range_ref<index_mismatch_error_type<array_managed_type>, index_range_error_type<array_managed_type>>(DXX_VALPTRIDX_REPORT_STANDARD_LEADER_COMMA_R_PASS_VARS r, i, a), &r)}
	{
	}

	operator mutable_pointer_type() const { return m_ptr; }	// implicit pointer conversion deprecated
	operator const_pointer_type() const { return m_ptr; }	// implicit pointer conversion deprecated
	template <typename rpolicy>
		requires(!std::is_same<policy, rpolicy>::value)
		ptr rebind_policy(ptr<rpolicy> &&rhs) const
	{
		/* This method could be marked as `static`, but is non-static so
		 * that callers must possess an instance of the target type.
		 * This serves as a basic check against casts that could remove
		 * `const` incorrectly.
		 */
		return ptr(std::move(rhs), typename containing_type::rebind_policy{});
	}
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
	constexpr bool operator==(std::nullptr_t) const
	{
		static_assert(allow_nullptr, "nullptr comparison not allowed: value is never null");
		return m_ptr == nullptr;
	}
	constexpr bool operator==(const_pointer_type p) const
	{
		return m_ptr == p;
	}
	constexpr bool operator==(mutable_pointer_type p) const
	{
		return m_ptr == p;
	}
	template <typename rpolicy>
		constexpr bool operator==(const ptr<rpolicy> &rhs) const
		{
			return *this == rhs.get_unchecked_pointer();
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
	ptr &operator--()
	{
		-- m_ptr;
		return *this;
	}
	ptr(allow_none_construction)
		requires(
			!policy::allow_nullptr	// allow_none_construction is only needed where nullptr is not already legal
		)
		:
		m_ptr(nullptr)
	{
	}
	template <typename rpolicy>
		requires(
			policy::allow_nullptr || !ptr<rpolicy>::allow_nullptr	// cannot rebind from allow_invalid to require_valid
		)
		ptr(ptr<rpolicy> &&rhs, typename containing_type::rebind_policy) :
			m_ptr{const_cast<managed_type *>(rhs.get_unchecked_pointer())}
	{
	}
};

#if DXX_VALPTRIDX_ENFORCE_STRICT_PI_SEPARATION
template <typename T>
struct strong_typedef : T
{
	using T::T;
	template <typename O>
		requires(std::is_constructible<T, O &&>::value)
		strong_typedef(O &&o) :
			T(std::forward<O>(o))
	{
	}
	strong_typedef() = default;
	strong_typedef(const strong_typedef &) = default;
	strong_typedef(strong_typedef &&) = default;
	strong_typedef &operator=(const strong_typedef &) & = default;
	strong_typedef &operator=(strong_typedef &&) & = default;
	strong_typedef &operator=(const strong_typedef &) && = delete;
	strong_typedef &operator=(strong_typedef &&) && = delete;
};
#endif

template <typename managed_type>
template <typename policy>
class valptridx<managed_type>::ptridx :
	public prohibit_void_ptr<ptridx<policy>>,
	public ptr<policy>,
	public idx<policy>
{
	using containing_type = valptridx<managed_type>;
public:
	using vptr_type = ptr<policy>;
	using vidx_type = idx<policy>;
	using typename vidx_type::array_managed_type;
	using index_type = typename vidx_type::index_type;
	using typename vidx_type::integral_type;
	using typename vptr_type::pointer_type;
	using vptr_type::allow_nullptr;
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
		requires(
			allow_nullptr || !ptridx<rpolicy>::allow_nullptr
		)
		ptridx(const ptridx<rpolicy> &rhs) :
			vptr_type{static_cast<const typename ptridx<rpolicy>::vptr_type &>(rhs)},
			vidx_type{static_cast<const typename ptridx<rpolicy>::vidx_type &>(rhs)}
	{
	}
	template <typename rpolicy>
		requires(
			!(allow_nullptr || !ptridx<rpolicy>::allow_nullptr)
		)
		ptridx(const ptridx<rpolicy> &rhs DXX_VALPTRIDX_REPORT_STANDARD_LEADER_COMMA_L_DECL_VARS) :
			vptr_type{static_cast<const typename ptridx<rpolicy>::vptr_type &>(rhs) DXX_VALPTRIDX_REPORT_STANDARD_LEADER_COMMA_L_PASS_VARS},
			vidx_type{static_cast<const typename ptridx<rpolicy>::vidx_type &>(rhs) DXX_VALPTRIDX_REPORT_STANDARD_LEADER_COMMA_L_PASS_VARS}
	{
	}
	template <typename rpolicy>
		ptridx(ptridx<rpolicy> &&rhs) :
			vptr_type{static_cast<typename ptridx<rpolicy>::vptr_type &&>(rhs)},
			vidx_type{static_cast<typename ptridx<rpolicy>::vidx_type &&>(rhs)}
	{
	}
	template <integral_type v>
		ptridx(const magic_constant<v> &m) :
			vptr_type{m},
			vidx_type{m}
	{
	}
	template <integral_type v>
		ptridx(const magic_constant<v> &m, array_managed_type &a) :
			vptr_type{m, a},
			vidx_type{m}
	{
	}
	template <integral_type v>
		ptridx(const magic_constant<v> &m, const allow_none_construction n) :
			vptr_type{n},
			vidx_type{m, n}
	{
	}
	ptridx(index_type i) = delete;
	ptridx(DXX_VALPTRIDX_REPORT_STANDARD_LEADER_COMMA_R_DEFN_VARS index_type i, array_managed_type &a) :
		vptr_type{DXX_VALPTRIDX_REPORT_STANDARD_LEADER_COMMA_R_PASS_VARS i, a},
		vidx_type{DXX_VALPTRIDX_REPORT_STANDARD_LEADER_COMMA_R_PASS_VARS i, a}
	{
	}
	ptridx(DXX_VALPTRIDX_REPORT_STANDARD_LEADER_COMMA_R_DEFN_VARS index_type i, array_managed_type &a, const allow_end_construction e) :
		vptr_type{DXX_VALPTRIDX_REPORT_STANDARD_LEADER_COMMA_R_PASS_VARS i, a, e},
		vidx_type{DXX_VALPTRIDX_REPORT_STANDARD_LEADER_COMMA_R_PASS_VARS i, a, e}
	{
	}
	ptridx(index_type i, array_managed_type &a, const assume_nothrow_index e) :
		vptr_type{i, a, e},
		vidx_type{i, e}
	{
	}
	ptridx(DXX_VALPTRIDX_REPORT_STANDARD_LEADER_COMMA_R_DEFN_VARS pointer_type p, array_managed_type &a) :
		/* Null pointer is never allowed when an index must be computed.
		 * Check for null, then use the reference constructor for
		 * vptr_type to avoid checking again.
		 */
		vptr_type{DXX_VALPTRIDX_REPORT_STANDARD_LEADER_COMMA_R_PASS_VARS (check_null_pointer<null_pointer_error_type<array_managed_type>>(DXX_VALPTRIDX_REPORT_STANDARD_LEADER_COMMA_R_PASS_VARS p, a), *p), a},
		vidx_type{DXX_VALPTRIDX_REPORT_STANDARD_LEADER_COMMA_R_PASS_VARS p, a}
	{
	}
	ptridx(DXX_VALPTRIDX_REPORT_STANDARD_LEADER_COMMA_R_DEFN_VARS pointer_type p, index_type i, array_managed_type &a) :
		vptr_type{DXX_VALPTRIDX_REPORT_STANDARD_LEADER_COMMA_R_PASS_VARS (check_null_pointer<null_pointer_error_type<array_managed_type>>(DXX_VALPTRIDX_REPORT_STANDARD_LEADER_COMMA_R_PASS_VARS p, a), *p), i, a},
		vidx_type{DXX_VALPTRIDX_REPORT_STANDARD_LEADER_COMMA_R_PASS_VARS i, a}
	{
	}
	template <typename rpolicy>
		requires(!std::is_same<policy, rpolicy>::value)
		ptridx rebind_policy(ptridx<rpolicy> &&rhs) const
	{
		return ptridx(std::move(rhs), typename containing_type::rebind_policy{});
	}
	ptridx absolute_sibling(const index_type i DXX_VALPTRIDX_REPORT_STANDARD_LEADER_COMMA_L_DECL_VARS) const
	{
		static_assert(!allow_nullptr, "absolute_sibling not allowed with invalid ptridx");
		ptridx r(*this);
		r.m_ptr += check_index_range<index_range_error_type<array_managed_type>>(DXX_VALPTRIDX_REPORT_STANDARD_LEADER_COMMA_R_PASS_VARS i, nullptr) - this->m_idx;
		r.m_idx = i;
		return r;
	}
	template <typename rpolicy>
		constexpr bool operator==(const ptridx<rpolicy> &rhs) const
		{
			return vptr_type::operator==(static_cast<const typename ptridx<rpolicy>::vptr_type &>(rhs));
		}
protected:
	ptridx &operator++()
	{
		vptr_type::operator++();
		vidx_type::operator++();
		return *this;
	}
	ptridx &operator--()
	{
		vptr_type::operator--();
		vidx_type::operator--();
		return *this;
	}
	template <typename rpolicy>
		ptridx(ptridx<rpolicy> &&rhs, const typename containing_type::rebind_policy rebind) :
			vptr_type{static_cast<typename ptridx<rpolicy>::vptr_type &&>(rhs), rebind},
			vidx_type{static_cast<typename ptridx<rpolicy>::vidx_type &&>(rhs)}
	{
		/* No static_assert for policy compatibility.  Incompatible
		 * policy conversions will be trapped by the static_assert in
		 * `vptr_type`.
		 */
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
	__attribute_cold
	guarded(std::nullptr_t) :
		m_dummy(), m_state(empty)
	{
	}
	guarded(guarded_type &&v) :
		m_value{std::move(v)}, m_state{initialized}
	{
	}
	[[nodiscard]]
	explicit operator bool() const
	{
		/*
		 * If no contained guarded_type exists, return false.
		 * Otherwise, record that the result has been tested and then
		 * return true.  operator*() uses m_state to enforce that the
		 * result is tested.
		 */
		if (unlikely(m_state == empty))
			return false;
		m_state = checked;
		return true;
	}
	[[nodiscard]]
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
				DXX_ALWAYS_ERROR_FUNCTION(DXX_VALPTRIDX_GUARDED_OBJECT_NO);
			/* If the contained object might not exist: */
			if (DXX_CONSTANT_TRUE(m_state == initialized))
				DXX_ALWAYS_ERROR_FUNCTION(DXX_VALPTRIDX_GUARDED_OBJECT_MAYBE);
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
				return P{i, a, assume_nothrow_index{}};
			else
				return nullptr;
		}
	template <typename P, typename T, typename A>
		static guarded<P> check_untrusted_internal(T &&, A &) = delete;
	template <typename P, typename A>
		[[nodiscard]]
		/* C++ does not allow `static operator()()`, so name it
		 * `call_operator` instead.
		 */
		static P call_operator(DXX_VALPTRIDX_REPORT_STANDARD_LEADER_COMMA_R_DEFN_VARS const typename P::index_type i, A &a)
		{
			return P(DXX_VALPTRIDX_REPORT_STANDARD_LEADER_COMMA_R_PASS_VARS i, a);
		}
	template <typename P, containing_type::integral_type v, typename A>
		[[nodiscard]]
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
		[[nodiscard]]
		guarded<Pc> check_untrusted(T &&t) const
		{
			return this->template check_untrusted_internal<Pc>(static_cast<T &&>(t), get_array());
		}
	template <typename T>
		[[nodiscard]]
		guarded<Pm> check_untrusted(T &&t)
		{
			return this->template check_untrusted_internal<Pm>(static_cast<T &&>(t), get_array());
		}
	template <typename T>
		[[nodiscard]]
		Pc operator()(T &&t DXX_VALPTRIDX_REPORT_STANDARD_LEADER_COMMA_L_DECL_VARS) const
		{
			return this->template call_operator<Pc>(DXX_VALPTRIDX_REPORT_STANDARD_LEADER_COMMA_R_PASS_VARS static_cast<T &&>(t), get_array());
		}
	template <typename T>
		[[nodiscard]]
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
		static P call_operator(DXX_VALPTRIDX_REPORT_STANDARD_LEADER_COMMA_R_DEFN_VARS const typename valptridx<managed_type>::template wrapper<valptridx<managed_type>::idx<policy>> i, A &a)
		{
			return P(DXX_VALPTRIDX_REPORT_STANDARD_LEADER_COMMA_R_PASS_VARS i, a);
		}
	template <typename P>
		[[nodiscard]]
		static P call_operator(DXX_VALPTRIDX_REPORT_STANDARD_LEADER_COMMA_R_DEFN_VARS const typename P::mutable_pointer_type p, typename P::array_managed_type &a)
		{
			return P(DXX_VALPTRIDX_REPORT_STANDARD_LEADER_COMMA_R_PASS_VARS p, a);
		}
	/*
	 * When P is a const type, require a const array, so that attempts to
	 * provide a mutable array fail.  This provides a slightly clearer error
	 * when trying to pass a const pointer to a mutable factory.
	 */
	template <typename P>
		requires(std::is_same<const array_managed_type, typename P::array_managed_type>::value)
		[[nodiscard]]
		static P call_operator(DXX_VALPTRIDX_REPORT_STANDARD_LEADER_COMMA_R_DEFN_VARS const typename P::const_pointer_type p, const array_managed_type &a)
		{
			return P(DXX_VALPTRIDX_REPORT_STANDARD_LEADER_COMMA_R_PASS_VARS p, a);
		}
	template <typename P, typename A>
		static iterator<P> end_internal(DXX_VALPTRIDX_REPORT_STANDARD_LEADER_COMMA_R_DEFN_VARS A &a)
		{
			return P(DXX_VALPTRIDX_REPORT_STANDARD_LEADER_COMMA_R_PASS_VARS static_cast<index_type>(a.get_count()), a, allow_end_construction{});
		}
public:
	[[nodiscard]]
	typename array_base_storage_type::size_type count() const
	{
		return get_array().get_count();
	}
	[[nodiscard]]
	typename array_base_storage_type::size_type size() const
	{
		return get_array().size();
	}
	template <typename T>
		[[nodiscard]]
		Pc operator()(T &&t DXX_VALPTRIDX_REPORT_STANDARD_LEADER_COMMA_L_DECL_VARS) const
		{
			return this->template call_operator<Pc>(DXX_VALPTRIDX_REPORT_STANDARD_LEADER_COMMA_R_PASS_VARS static_cast<T &&>(t), get_array());
		}
	template <typename T>
		[[nodiscard]]
		Pm operator()(T &&t DXX_VALPTRIDX_REPORT_STANDARD_LEADER_COMMA_L_DECL_VARS)
		{
			return this->template call_operator<Pm>(DXX_VALPTRIDX_REPORT_STANDARD_LEADER_COMMA_R_PASS_VARS static_cast<T &&>(t), get_array());
		}
	[[nodiscard]]
	iterator<Pc> begin() const
	{
		return Pc(valptridx<managed_type>::magic_constant<index_type{}>(), get_array());
	}
	[[nodiscard]]
	iterator<Pm> begin()
	{
		return Pm(valptridx<managed_type>::magic_constant<index_type{}>(), get_array());
	}
	[[nodiscard]]
	iterator<Pc> end(DXX_VALPTRIDX_REPORT_STANDARD_LEADER_COMMA_N_DECL_VARS) const
	{
		return this->template end_internal<Pc>(DXX_VALPTRIDX_REPORT_STANDARD_LEADER_COMMA_R_PASS_VARS get_array());
	}
	[[nodiscard]]
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

