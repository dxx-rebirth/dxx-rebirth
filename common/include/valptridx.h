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

#endif

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
		p(t), i(s)
	{
		check_null_pointer();
		check_index_match(a);
		check_index_range(a);
	}
	valptridx_t(pointer_type t) :
		p(t), i(t-get_array())
	{
		check_null_pointer();
		auto &a = get_array();
		check_index_match(a);
		check_index_range(a);
	}
	valptridx_t(index_type s) :
		p(s != ~static_cast<index_type>(0) ? &get_array()[s] : NULL), i(s)
	{
		if (s != ~static_cast<index_type>(0))
			check_index_range(get_array());
	}
protected:
	void check_null_pointer() const
	{
		DXX_VALPTRIDX_STATIC_CHECK(p, dxx_trap_constant_null_pointer, "NULL pointer used");
		if (!p)
			throw null_pointer_exception("NULL pointer constructor");
	}
	template <typename A>
		void check_index_match(A &a)
		{
			if (&a[i] != p)
				throw index_mismatch_exception("pointer/index mismatch");
		}
	template <typename A>
		void check_index_range(A &a) const
		{
			DXX_VALPTRIDX_STATIC_CHECK(static_cast<std::size_t>(i) < a.size(), dxx_trap_constant_invalid_index, "invalid index used in array subscript");
			if (!(static_cast<std::size_t>(i) < a.size()))
				throw index_range_exception("index exceeds range");
		}
	pointer_type p;
	index_type i;
	static decltype(get_global_array(pointer_type())) get_array()
	{
		return get_global_array(pointer_type());
	}
};

#define _DEFINE_VALPTRIDX_SUBTYPE_USERTYPE(N,P,I,A,name,Pconst)	\
	static inline decltype(A) Pconst &get_global_array(P Pconst*) { return A; }	\
	\
	struct name : valptridx_t<P Pconst, I, P##_magic_constant_t> {	\
		name() = delete;	\
		DXX_INHERIT_CONSTRUCTORS(name, valptridx_t<P Pconst, I, P##_magic_constant_t>);	\
	};	\
	\
	static inline name N(name::pointer_type o, name::index_type i) {	\
		return {A, o, i};	\
	}	\
	\
	static inline name operator-(P Pconst *o, decltype(A) &O)	\
	{	\
		return N(o, o - (&*O.begin()));	\
	}	\
	\
	name operator-(name, decltype(A) &) = delete;	\

#define DEFINE_VALPTRIDX_SUBTYPE(N,P,I,A)	\
	_DEFINE_VALPTRIDX_SUBTYPE_USERTYPE(N,P,I,A,N##_t,);	\
	_DEFINE_VALPTRIDX_SUBTYPE_USERTYPE(N,P,I,A,c##N##_t,const);	\

