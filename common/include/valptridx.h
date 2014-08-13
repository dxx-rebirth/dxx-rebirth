/*
 * This file is part of the DXX-Rebirth project <http://www.dxx-rebirth.com/>.
 * It is copyright by its individual contributors, as recorded in the
 * project's Git history.  See COPYING.txt at the top level for license
 * terms and a link to the Git history.
 */
#pragma once

#include <stdexcept>
#include "dxxsconf.h"
#include "compiler-type_traits.h"

#ifdef DXX_HAVE_BUILTIN_CONSTANT_P
#define DXX_VALPTRIDX_STATIC_CHECK(E,F,S)	\
	((void)(dxx_builtin_constant_p((E)) && !(E) &&	\
		(DXX_ALWAYS_ERROR_FUNCTION(F,S), 0)))
#else
#define DXX_VALPTRIDX_STATIC_CHECK(E,F,S)	\
	((void)0)
#endif

#define DXX_VALPTRIDX_CHECK(E,S,success,failure)	\
	(	\
		DXX_VALPTRIDX_STATIC_CHECK(E,dxx_trap_##failure,S),	\
		(E) ? success : throw failure(S)	\
	)

template <typename T>
void get_global_array(T *);

/*
 * A data type for passing both a pointer and its offset in an
 * agreed-upon array.  Useful for Segments, Objects.
 */
template <typename T, typename I, template <I> class magic_constant>
class valptridx_t
{
	typedef valptridx_t<T, I, magic_constant> this_type;
public:
	struct null_pointer_exception : std::logic_error {
		null_pointer_exception(const char *s) : std::logic_error(s) {}
	};
	struct index_mismatch_exception : std::logic_error {
		index_mismatch_exception(const char *s) : std::logic_error(s) {}
	};
	struct index_range_exception : std::out_of_range {
		index_range_exception(const char *s) : std::out_of_range(s) {}
	};
	typedef T *pointer_type;
	typedef I index_type;
	pointer_type operator->() const { return p; }
	operator pointer_type() const { return p; }
	operator const index_type&() const { return i; }
	template <index_type v>
		bool operator==(const magic_constant<v> &) const
		{
			return i == v;
		}
	bool operator==(const pointer_type &rhs) const { return p == rhs; }
	bool operator==(const index_type &rhs) const { return i == rhs; }
	// Compatibility hack since some object numbers are in int, not
	// short
	bool operator==(const int &rhs) const { return i == rhs; }
	template <typename R>
		bool operator!=(const R &rhs) const
		{
			return !(*this == rhs);
		}
	bool operator==(const valptridx_t &rhs) const { return p == rhs.p && i == rhs.i; }
	template <typename U>
		typename tt::enable_if<!tt::is_base_of<this_type, U>::value, bool>::type operator==(U) const = delete;
	valptridx_t() = delete;
	valptridx_t(const valptridx_t<typename tt::remove_const<T>::type, I, magic_constant> &t) :
		p(t.p), i(t.i)
	{
	}
	template <index_type v>
		valptridx_t(const magic_constant<v> &) :
			p(static_cast<std::size_t>(v) < get_array().size() ? &get_array()[v] : NULL), i(v)
	{
	}
	template <typename A>
	valptridx_t(A &a, pointer_type t, index_type s) :
		p(check_null_pointer(t)), i(check_index_match(a, t, check_index_range(a, s)))
	{
	}
	valptridx_t(pointer_type t) :
		p(check_null_pointer(t)), i(check_index_match(get_array(), t, check_index_range(get_array(), t-get_array())))
	{
	}
	valptridx_t(index_type s) :
		p(s != ~static_cast<index_type>(0) ? &get_array()[s] : NULL), i(s != ~static_cast<index_type>(0) ? check_index_range(get_array(), s) : s)
	{
	}
protected:
	static pointer_type check_null_pointer(pointer_type p) __attribute_warn_unused_result
	{
		return DXX_VALPTRIDX_CHECK(p, "NULL pointer used", p, null_pointer_exception);
	}
	template <typename A>
		static index_type check_index_match(const A &a, pointer_type t, index_type s) __attribute_warn_unused_result;
	template <typename A>
		static index_type check_index_range(const A &a, index_type s) __attribute_warn_unused_result;
	pointer_type p;
	index_type i;
	static constexpr decltype(get_global_array(pointer_type())) get_array()
	{
		return get_global_array(pointer_type());
	}
};

/* Out of line since gcc chokes on template + inline + attribute */
template <typename T, typename I, template <I> class magic_constant>
template <typename A>
typename valptridx_t<T, I, magic_constant>::index_type valptridx_t<T, I, magic_constant>::check_index_match(const A &a, pointer_type t, index_type s)
{
	return DXX_VALPTRIDX_CHECK(&a[s] == t, "pointer/index mismatch", s, index_mismatch_exception);
}

template <typename T, typename I, template <I> class magic_constant>
template <typename A>
typename valptridx_t<T, I, magic_constant>::index_type valptridx_t<T, I, magic_constant>::check_index_range(const A &a, index_type s)
{
	return DXX_VALPTRIDX_CHECK(static_cast<std::size_t>(s) < a.size(), "invalid index used in array subscript", s, index_range_exception);
}

template <typename T, typename I, template <I> class magic_constant>
class vvalptridx_t : public valptridx_t<T, I, magic_constant>
{
	typedef valptridx_t<T, I, magic_constant> base_t;
protected:
	using base_t::get_array;
	using base_t::check_index_range;
	using base_t::index_range_exception;
public:
	typedef typename base_t::pointer_type pointer_type;
	typedef typename base_t::index_type index_type;
	template <index_type v>
		static constexpr const magic_constant<v> &check_constant_index(const magic_constant<v> &m)
		{
#ifndef constexpr
			static_assert(static_cast<std::size_t>(v) < get_array().size(), "invalid index used");
#endif
			return m;
		}
	vvalptridx_t() = delete;
	template <typename A>
	vvalptridx_t(A &a, pointer_type t, index_type s) :
		base_t(a, t, s)
	{
	}
	template <index_type v>
		vvalptridx_t(const magic_constant<v> &m) :
			base_t(check_constant_index(m))
	{
	}
	vvalptridx_t(const vvalptridx_t<typename tt::remove_const<T>::type, I, magic_constant> &t) :
		base_t(t)
	{
	}
	vvalptridx_t(pointer_type p) :
		base_t(p)
	{
	}
	vvalptridx_t(const base_t &t) :
		base_t(get_array(), t, t)
	{
	}
	template <typename A>
	vvalptridx_t(A &a, index_type s) :
		base_t(check_index_range(a, s))
	{
	}
	vvalptridx_t(index_type s) :
		base_t(check_index_range(get_array(), s))
	{
	}
	using base_t::operator==;
	template <index_type v>
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

#define _DEFINE_VALPTRIDX_SUBTYPE_USERTYPE(N,P,I,A,name,Pconst)	\
	static inline constexpr decltype(A) Pconst &get_global_array(P Pconst*) { return A; }	\
	\
	struct name : valptridx_t<P Pconst, I, P##_magic_constant_t> {	\
		name() = delete;	\
		DXX_INHERIT_CONSTRUCTORS(name, valptridx_t<P Pconst, I, P##_magic_constant_t>);	\
	};	\
	\
	struct v##name : vvalptridx_t<P Pconst, I, P##_magic_constant_t> {	\
		v##name() = delete;	\
		DXX_INHERIT_CONSTRUCTORS(v##name, vvalptridx_t<P Pconst, I, P##_magic_constant_t>);	\
	};	\
	\
	static inline v##name N(name::pointer_type o, name::index_type i) {	\
		return {A, o, i};	\
	}	\
	\
	static inline v##name operator-(P Pconst *o, decltype(A) &O)	\
	{	\
		return N(o, const_cast<const P *>(o) - &(const_cast<const decltype(A) &>(O).front()));	\
	}	\
	\
	name operator-(name, decltype(A) &) = delete;	\

#define DEFINE_VALPTRIDX_SUBTYPE(N,P,I,A)	\
	_DEFINE_VALPTRIDX_SUBTYPE_USERTYPE(N,P,I,A,N##_t,);	\
	_DEFINE_VALPTRIDX_SUBTYPE_USERTYPE(N,P,I,A,c##N##_t,const);	\
	\
	static inline v##N##_t N(N##_t::index_type i) {	\
		return {A, i};	\
	}	\

