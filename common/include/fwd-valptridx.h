/*
 * This file is part of the DXX-Rebirth project <https://www.dxx-rebirth.com/>.
 * It is copyright by its individual contributors, as recorded in the
 * project's Git history.  See COPYING.txt at the top level for license
 * terms and a link to the Git history.
 */
#pragma once

#include <functional>
#include <type_traits>
#include <cstddef>
#include "dxxsconf.h"
#include "cpp-valptridx.h"
#include "d_array.h"

#if defined(DXX_HAVE_CXX_BUILTIN_FILE_LINE)
#define DXX_VALPTRIDX_ENABLE_REPORT_FILENAME
#define DXX_VALPTRIDX_REPORT_STANDARD_LEADER_COMMA_N_DECL_VARS	const char *filename = __builtin_FILE(), const unsigned lineno = __builtin_LINE()
#define DXX_VALPTRIDX_REPORT_STANDARD_LEADER_COMMA_L_DECL_VARS	, DXX_VALPTRIDX_REPORT_STANDARD_LEADER_COMMA_N_DECL_VARS
#define DXX_VALPTRIDX_REPORT_STANDARD_LEADER_COMMA_N_DEFN_VARS	const char *const filename, const unsigned lineno
#define DXX_VALPTRIDX_REPORT_STANDARD_LEADER_COMMA_R_DEFN_VARS	DXX_VALPTRIDX_REPORT_STANDARD_LEADER_COMMA_N_DEFN_VARS,
#define DXX_VALPTRIDX_REPORT_STANDARD_LEADER_COMMA_N_PASS_VARS_	filename, lineno
#define DXX_VALPTRIDX_REPORT_STANDARD_LEADER_COMMA_N_VOID_VARS()	static_cast<void>(filename), static_cast<void>(lineno)
#define DXX_VALPTRIDX_REPORT_STANDARD_LEADER_COMMA_L_PASS_VARS	, DXX_VALPTRIDX_REPORT_STANDARD_LEADER_COMMA_N_PASS_VARS_
#define DXX_VALPTRIDX_REPORT_STANDARD_LEADER_COMMA_R_PASS_VARS	DXX_VALPTRIDX_REPORT_STANDARD_LEADER_COMMA_N_PASS_VARS_,
#define DXX_VALPTRIDX_REPORT_STANDARD_LEADER_COMMA_R_PASS_VA(...)	DXX_VALPTRIDX_REPORT_STANDARD_LEADER_COMMA_N_PASS_VARS_, ## __VA_ARGS__
#define DXX_VALPTRIDX_REPORT_STANDARD_ASM_LOAD_COMMA_N_VARS	"rm" (filename), "rm" (lineno)
#define DXX_VALPTRIDX_REPORT_STANDARD_ASM_LOAD_COMMA_R_VARS	DXX_VALPTRIDX_REPORT_STANDARD_ASM_LOAD_COMMA_N_VARS,
#else
#define DXX_VALPTRIDX_REPORT_STANDARD_LEADER_COMMA_N_DECL_VARS
#define DXX_VALPTRIDX_REPORT_STANDARD_LEADER_COMMA_L_DECL_VARS
#define DXX_VALPTRIDX_REPORT_STANDARD_LEADER_COMMA_N_DEFN_VARS
#define DXX_VALPTRIDX_REPORT_STANDARD_LEADER_COMMA_R_DEFN_VARS
#define DXX_VALPTRIDX_REPORT_STANDARD_LEADER_COMMA_N_VOID_VARS()	static_cast<void>(0)
#define DXX_VALPTRIDX_REPORT_STANDARD_LEADER_COMMA_L_PASS_VARS
#define DXX_VALPTRIDX_REPORT_STANDARD_LEADER_COMMA_R_PASS_VARS
#define DXX_VALPTRIDX_REPORT_STANDARD_LEADER_COMMA_R_PASS_VA(...)	__VA_ARGS__
#define DXX_VALPTRIDX_REPORT_STANDARD_ASM_LOAD_COMMA_N_VARS
#define DXX_VALPTRIDX_REPORT_STANDARD_ASM_LOAD_COMMA_R_VARS
#endif

template <typename managed_type>
class valptridx :
	protected valptridx_specialized_types<managed_type>::type
{
	using specialized_types = typename valptridx_specialized_types<managed_type>::type;
	using specialized_types::array_size;
	class partial_policy
	{
	public:
		class require_valid;
		class allow_invalid;
		template <template <typename> class policy>
			class apply_cv_policy;
	};
	class vc;	/* require_valid + const_policy */
	class ic;	/* allow_invalid + const_policy */
	class vm;	/* require_valid + mutable_policy */
	class im;	/* allow_invalid + mutable_policy */
	template <typename>
		class guarded;
	class array_base_count_type;
	using array_base_storage_type = typename std::conditional<
		std::is_enum<typename specialized_types::integral_type>::value,
		enumerated_array<managed_type, array_size, typename specialized_types::integral_type>,
		std::array<managed_type, array_size>
		>::type;
protected:
	using const_pointer_type = const managed_type *;
	using const_reference_type = const managed_type &;
	using mutable_pointer_type = managed_type *;
	/* integral_type must be a primitive integer type capable of holding
	 * all legal values used with managed_type.  Legal values are valid
	 * indexes in array_managed_type and any magic out-of-range values.
	 */
	using typename specialized_types::integral_type;
	using index_type = integral_type;	// deprecated; should be dedicated UDT

public:
	class array_managed_type;
	using typename specialized_types::report_error_uses_exception;
	/* ptridx<policy> publicly inherits from idx<policy> and
	 * ptr<policy>, but should not be implicitly sliced to one of the
	 * base types.  To prevent slicing, define
	 * DXX_VALPTRIDX_ENFORCE_STRICT_PI_SEPARATION to a non-zero value.
	 * When enabled, slicing prevention makes *ptr and *idx derive from
	 * ptr<policy> / idx<policy>.  When disabled, *ptr and *idx are
	 * typedef aliases for ptr<policy> / idx<policy>.
	 */
	template <typename policy>
		class idx;
	template <typename policy>
		class ptr;
	template <typename policy>
		class ptridx;
protected:
	template <typename Pc, typename Pm>
		class basic_ival_member_factory;
	template <typename Pc, typename Pm>
		class basic_vval_member_factory;
	using typename specialized_types::allow_end_construction;
	using typename specialized_types::assume_nothrow_index;

	template <typename handle_index_mismatch>
	static inline void check_index_match(DXX_VALPTRIDX_REPORT_STANDARD_LEADER_COMMA_R_DEFN_VARS const_reference_type, index_type, const array_managed_type &);
	template <
		typename handle_index_range_error,
		template <typename> class Compare = std::less
		>
	static inline index_type check_index_range(DXX_VALPTRIDX_REPORT_STANDARD_LEADER_COMMA_R_DEFN_VARS index_type, const array_managed_type *);
	template <typename handle_index_mismatch, typename handle_index_range_error>
	static inline void check_explicit_index_range_ref(DXX_VALPTRIDX_REPORT_STANDARD_LEADER_COMMA_R_DEFN_VARS const_reference_type, std::size_t, const array_managed_type &);
	template <typename handle_index_mismatch, typename handle_index_range_error>
	static inline void check_implicit_index_range_ref(DXX_VALPTRIDX_REPORT_STANDARD_LEADER_COMMA_R_DEFN_VARS const_reference_type, const array_managed_type &);
	template <typename handle_null_pointer>
	static inline void check_null_pointer_conversion(DXX_VALPTRIDX_REPORT_STANDARD_LEADER_COMMA_R_DEFN_VARS const_pointer_type);
	template <typename handle_null_pointer>
	static inline void check_null_pointer(DXX_VALPTRIDX_REPORT_STANDARD_LEADER_COMMA_R_DEFN_VARS const_pointer_type, const array_managed_type &);

#define DXX_VALPTRIDX_FOR_EACH_VC_TYPE(VERB, MANAGED_TYPE, DERIVED_TYPE_PREFIX, CONTEXT, PISUFFIX, IVPREFIX)	\
	VERB(MANAGED_TYPE, DERIVED_TYPE_PREFIX, CONTEXT, PISUFFIX, IVPREFIX, m);	\
	VERB(MANAGED_TYPE, DERIVED_TYPE_PREFIX, CONTEXT, PISUFFIX, IVPREFIX, c)

#define DXX_VALPTRIDX_FOR_EACH_IV_TYPE(VERB, MANAGED_TYPE, DERIVED_TYPE_PREFIX, CONTEXT, PISUFFIX)	\
	DXX_VALPTRIDX_FOR_EACH_VC_TYPE(VERB, MANAGED_TYPE, DERIVED_TYPE_PREFIX, CONTEXT, PISUFFIX, i);	\
	DXX_VALPTRIDX_FOR_EACH_VC_TYPE(VERB, MANAGED_TYPE, DERIVED_TYPE_PREFIX, CONTEXT, PISUFFIX, v)

#define DXX_VALPTRIDX_FOR_EACH_IDX_TYPE(VERB, MANAGED_TYPE, DERIVED_TYPE_PREFIX, CONTEXT)	\
	DXX_VALPTRIDX_FOR_EACH_IV_TYPE(VERB, MANAGED_TYPE, DERIVED_TYPE_PREFIX, CONTEXT, idx)

#define DXX_VALPTRIDX_FOR_EACH_PTR_TYPE(VERB, MANAGED_TYPE, DERIVED_TYPE_PREFIX, CONTEXT)	\
	DXX_VALPTRIDX_FOR_EACH_IV_TYPE(VERB, MANAGED_TYPE, DERIVED_TYPE_PREFIX, CONTEXT, ptr)

#define DXX_VALPTRIDX_FOR_EACH_PTRIDX_TYPE(VERB, MANAGED_TYPE, DERIVED_TYPE_PREFIX, CONTEXT)	\
	DXX_VALPTRIDX_FOR_EACH_IV_TYPE(VERB, MANAGED_TYPE, DERIVED_TYPE_PREFIX, CONTEXT, ptridx)

#define DXX_VALPTRIDX_FOR_EACH_PPI_TYPE(VERB, MANAGED_TYPE, DERIVED_TYPE_PREFIX, CONTEXT)	\
	DXX_VALPTRIDX_FOR_EACH_PTR_TYPE(VERB, MANAGED_TYPE, DERIVED_TYPE_PREFIX, CONTEXT);	\
	DXX_VALPTRIDX_FOR_EACH_PTRIDX_TYPE(VERB, MANAGED_TYPE, DERIVED_TYPE_PREFIX, CONTEXT)

#define DXX_VALPTRIDX_FOR_EACH_IPPI_TYPE(VERB, MANAGED_TYPE, DERIVED_TYPE_PREFIX, CONTEXT)	\
	DXX_VALPTRIDX_FOR_EACH_IDX_TYPE(VERB, MANAGED_TYPE, DERIVED_TYPE_PREFIX, CONTEXT);	\
	DXX_VALPTRIDX_FOR_EACH_PPI_TYPE(VERB, MANAGED_TYPE, DERIVED_TYPE_PREFIX, CONTEXT)

	class index_mismatch_exception;
	class index_range_exception;
	class null_pointer_exception;
public:
	/* This is a special placeholder that allows segiter to bypass the
	 * normal rules.  The calling code is responsible for providing the
	 * safety that is bypassed.  Use this bypass only if you understand
	 * exactly what you are skipping and why your use is safe despite
	 * the lack of checking.
	 */
	using typename specialized_types::allow_none_construction;
	using typename specialized_types::rebind_policy;
	typedef ptridx<ic>	icptridx;
	typedef ptridx<im>	imptridx;
	typedef ptridx<vc>	vcptridx;
	typedef ptridx<vm>	vmptridx;

#define DXX_VALPTRIDX_DECLARE_PI_TYPE(MANAGED_TYPE, DERIVED_TYPE_PREFIX, CONTEXT, PISUFFIX, IVPREFIX, MCPREFIX)	\
	using IVPREFIX ## MCPREFIX ## PISUFFIX = typename specialized_types::template wrapper< PISUFFIX < IVPREFIX ## MCPREFIX > >
	DXX_VALPTRIDX_FOR_EACH_IDX_TYPE(DXX_VALPTRIDX_DECLARE_PI_TYPE,,,);
	DXX_VALPTRIDX_FOR_EACH_PTR_TYPE(DXX_VALPTRIDX_DECLARE_PI_TYPE,,,);
#undef DXX_VALPTRIDX_DEFINE_PI_TYPE

#define DXX_VALPTRIDX_MC_qualifier_m
#define DXX_VALPTRIDX_MC_qualifier_c	const
#define DXX_VALPTRIDX_DECLARE_MEMBER_FACTORIES(MANAGED_TYPE, DERIVED_TYPE_PREFIX, CONTEXT, PISUFFIX, IVPREFIX, MCPREFIX)	\
	using f ## IVPREFIX ## MCPREFIX ## PISUFFIX =	\
	DXX_VALPTRIDX_MC_qualifier_ ## MCPREFIX	\
	basic_ ## IVPREFIX ## val_member_factory<	\
		IVPREFIX ## c ## PISUFFIX,	\
		IVPREFIX ## m ## PISUFFIX	\
	>
	DXX_VALPTRIDX_FOR_EACH_PPI_TYPE(DXX_VALPTRIDX_DECLARE_MEMBER_FACTORIES,,,);
#undef DXX_VALPTRIDX_DECLARE_MEMBER_FACTORIES
#undef DXX_VALPTRIDX_MC_qualifier_c
#undef DXX_VALPTRIDX_MC_qualifier_m

	/* Commit a DRY violation here so that tools can easily find these
	 * types.  Strict DRY compliance would mean using a macro to
	 * generate the `using` line, since the typedef, the dispatcher type
	 * it references, and an argument to that type are all dependent on
	 * the same string.
	 */
	template <typename T>
		using index_mismatch_error_type = typename specialized_types::template dispatch_index_mismatch_error<T, index_mismatch_exception>;
	template <typename T>
		using index_range_error_type = typename specialized_types::template dispatch_index_range_error<T, index_range_exception>;
	template <typename T>
		using null_pointer_error_type = typename specialized_types::template dispatch_null_pointer_error<T, null_pointer_exception>;

	template <integral_type constant>
		class magic_constant
		{
		public:
			constexpr operator integral_type() const { return constant; }	// integral_type conversion deprecated
		};
};

#define DXX_VALPTRIDX_DEFINE_FACTORY_TYPEDEF(MANAGED_TYPE, DERIVED_TYPE_PREFIX, CONTEXT, PISUFFIX, IVPREFIX, MCPREFIX)	\
	using f ## IVPREFIX ## MCPREFIX ## DERIVED_TYPE_PREFIX ## PISUFFIX = valptridx<MANAGED_TYPE>::f ## IVPREFIX ## MCPREFIX ## PISUFFIX

#define DXX_VALPTRIDX_DEFINE_SUBTYPE_TYPEDEF(MANAGED_TYPE, DERIVED_TYPE_PREFIX, CONTEXT, PISUFFIX, IVPREFIX, MCPREFIX)	\
	using IVPREFIX ## MCPREFIX ## DERIVED_TYPE_PREFIX ## PISUFFIX ## _t = valptridx<MANAGED_TYPE>::IVPREFIX ## MCPREFIX ## PISUFFIX

#define DXX_VALPTRIDX_DEFINE_SUBTYPE_TYPEDEFS(MANAGED_TYPE, DERIVED_TYPE_PREFIX)	\
	using MANAGED_TYPE ## _array = valptridx<MANAGED_TYPE>::array_managed_type;	\
	DXX_VALPTRIDX_FOR_EACH_PPI_TYPE(DXX_VALPTRIDX_DEFINE_FACTORY_TYPEDEF, MANAGED_TYPE, DERIVED_TYPE_PREFIX,);	\
	DXX_VALPTRIDX_FOR_EACH_IPPI_TYPE(DXX_VALPTRIDX_DEFINE_SUBTYPE_TYPEDEF, MANAGED_TYPE, DERIVED_TYPE_PREFIX,)
