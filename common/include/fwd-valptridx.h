#pragma once

#include <cstddef>
#include "dxxsconf.h"
#include "objnum.h"
#include "segnum.h"

template <typename managed_type>
using valptridx_specialized_types = decltype(valptridx_specialized_type(static_cast<managed_type *>(nullptr)));

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
public:
	class array_managed_type;

protected:
	using const_pointer_type = const managed_type *;
	using const_reference_type = const managed_type &;
	using mutable_pointer_type = managed_type *;
	using typename specialized_types::integral_type;
	using index_type = integral_type;	// deprecated; should be dedicated UDT
	using specialized_types::array_size;

	template <typename policy, unsigned>
		class basic_idx;
	template <typename policy, unsigned>
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
	static inline void check_null_pointer_conversion(const_pointer_type);
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
	typedef basic_ptridx<vc>	vcptridx;
	typedef basic_ptridx<ic>	cptridx;
	typedef basic_ptridx<vm>	vptridx;
	typedef basic_ptridx<im>	ptridx;
	typedef basic_idx<vc, 0>	vcidx;
	typedef basic_idx<ic, 0>	cidx;
	typedef basic_idx<vm, 0>	vidx;
	typedef basic_idx<im, 0>	idx;
	typedef basic_ptr<vc, 0>	vcptr;
	typedef basic_ptr<ic, 0>	cptr;
	typedef basic_ptr<vm, 0>	vptr;
	typedef basic_ptr<im, 0>	ptr;
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

template <typename INTEGRAL_TYPE, std::size_t array_size_value>
class valptridx_specialized_type_parameters
{
public:
	using integral_type = INTEGRAL_TYPE;
	static constexpr std::size_t array_size = array_size_value;
};

#define DXX_VALPTRIDX_DEFINE_SUBTYPE_TYPEDEF(managed_type,derived_type_prefix,vcprefix,suffix)	\
	typedef valptridx<managed_type>::vcprefix##suffix vcprefix##derived_type_prefix##suffix##_t
#define DXX_VALPTRIDX_DECLARE_GLOBAL_SUBTYPE(managed_type,derived_type_prefix,global_array,array_size_value)	\
	struct managed_type;	\
	valptridx_specialized_type_parameters<derived_type_prefix##num_t, array_size_value> valptridx_specialized_type(managed_type *);	\
	extern valptridx<managed_type>::array_managed_type global_array;	\
	DXX_VALPTRIDX_SUBTYPE(DXX_VALPTRIDX_DEFINE_SUBTYPE_TYPEDEF, managed_type, derived_type_prefix)
