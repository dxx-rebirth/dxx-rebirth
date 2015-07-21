#pragma once

#include <cstddef>
#include "dxxsconf.h"
#include "objnum.h"
#include "segnum.h"

template <typename managed_type>
class valptridx_specialized_types;

template <typename managed_type>
class valptridx :
	protected valptridx_specialized_types<managed_type>
{
	using specialized_types = valptridx_specialized_types<managed_type>;
	class partial_policy
	{
	public:
		class require_valid;
		class allow_invalid;
		class const_policy;
		class mutable_policy;
		template <typename policy>
			class apply_cv_policy;
	};
	class vc;	/* require_valid + const_policy */
	class ic;	/* allow_invalid + const_policy */
	class vm;	/* require_valid + mutable_policy */
	class im;	/* allow_invalid + mutable_policy */
protected:
	using const_pointer_type = const managed_type *;
	using const_reference_type = const managed_type &;
	using mutable_pointer_type = managed_type *;
	using typename specialized_types::array_managed_type;
	using typename specialized_types::index_type;
	using typename specialized_types::integral_type;

	template <typename policy>
		class basic_idx;
	template <typename policy>
		class basic_ptr;
	template <typename policy>
		class basic_ptridx;

	static constexpr const array_managed_type &get_array(const_pointer_type p)
	{
		return get_global_array(p);
	}
	static constexpr array_managed_type &get_array(mutable_pointer_type p = mutable_pointer_type())
	{
		return get_global_array(p);
	}
	static inline void check_index_match(const_reference_type, index_type, const array_managed_type &);
	static inline index_type check_index_range(index_type, const array_managed_type &);
	static inline void check_explicit_index_range_ref(const_reference_type, std::size_t, const array_managed_type &);
	static inline void check_implicit_index_range_ref(const_reference_type, const array_managed_type &);
	static inline void check_null_pointer(const_pointer_type, const array_managed_type &);
	static void check_null_pointer(std::nullptr_t, ...) = delete;

#define DXX_VALPTRIDX_SUBTYPE(VERB,managed_type,derived_type_prefix)	\
	DXX_VALPTRIDX_SUBTYPE_C(VERB, managed_type, derived_type_prefix, c);	\
	DXX_VALPTRIDX_SUBTYPE_C(VERB, managed_type, derived_type_prefix, )

#define DXX_VALPTRIDX_SUBTYPE_C(VERB,managed_type,derived_type_prefix,cprefix)	\
	DXX_VALPTRIDX_SUBTYPE_VC(VERB, managed_type, derived_type_prefix, cprefix);	\
	DXX_VALPTRIDX_SUBTYPE_VC(VERB, managed_type, derived_type_prefix, v##cprefix)

#define DXX_VALPTRIDX_SUBTYPE_VC(VERB,managed_type,derived_type_prefix,vcprefix)	\
	VERB(managed_type, derived_type_prefix, vcprefix, idx);	\
	VERB(managed_type, derived_type_prefix, vcprefix, ptr);	\
	VERB(managed_type, derived_type_prefix, vcprefix, ptridx)

public:
#define DXX_VALPTRIDX_FWD_DECLARE_SUBTYPE(managed_type,derived_type_prefix,vcprefix,suffix)	class vcprefix##suffix
	DXX_VALPTRIDX_SUBTYPE(DXX_VALPTRIDX_FWD_DECLARE_SUBTYPE,,);
#undef DXX_VALPTRIDX_FWD_DECLARE_SUBTYPE
	class index_mismatch_exception;
	class index_range_exception;
	class null_pointer_exception;

	template <typename vptr>
		class basic_vptr_global_factory;
	template <typename ptridx>
		class basic_ptridx_global_factory;
	template <integral_type constant>
		class magic_constant
		{
		public:
			constexpr operator integral_type() const { return constant; }	// integral_type conversion deprecated
		};
};

#define DXX_VALPTRIDX_DEFINE_SUBTYPE_TYPEDEF(managed_type,derived_type_prefix,vcprefix,suffix)	\
	typedef valptridx<managed_type>::vcprefix##suffix vcprefix##derived_type_prefix##suffix##_t
#define DXX_VALPTRIDX_DECLARE_GLOBAL_SUBTYPE(managed_type,derived_type_prefix,global_array)	\
	struct managed_type;	\
	struct managed_type##_array_t;	\
	extern managed_type##_array_t global_array;	\
	template <>	\
	class valptridx_specialized_types<managed_type> {	\
	public:	\
		using array_managed_type = managed_type##_array_t;	\
		using index_type = derived_type_prefix##num_t;	\
		using integral_type = derived_type_prefix##num_t;	\
	};	\
	DXX_VALPTRIDX_SUBTYPE(DXX_VALPTRIDX_DEFINE_SUBTYPE_TYPEDEF, managed_type, derived_type_prefix)

DXX_VALPTRIDX_DECLARE_GLOBAL_SUBTYPE(object, obj, Objects);
DXX_VALPTRIDX_DECLARE_GLOBAL_SUBTYPE(segment, seg, Segments);

#undef DXX_VALPTRIDX_DECLARE_GLOBAL_SUBTYPE
#undef DXX_VALPTRIDX_DEFINE_SUBTYPE_TYPEDEF
#undef DXX_VALPTRIDX_SUBTYPE_VC
#undef DXX_VALPTRIDX_SUBTYPE_C
#undef DXX_VALPTRIDX_SUBTYPE
