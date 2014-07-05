/*
 * This file is part of the DXX-Rebirth project <http://www.dxx-rebirth.com/>.
 * It is copyright by its individual contributors, as recorded in the
 * project's Git history.  See COPYING.txt at the top level for license
 * terms and a link to the Git history.
 */
#pragma once

#include <stdexcept>
#include "dxxsconf.h"

/*
 * A data type for passing both a pointer and its offset in an
 * agreed-upon array.  Useful for Segments, Objects.
 */
template <typename T, typename I>
class valptridx_t
{
	valptridx_t() DXX_CXX11_EXPLICIT_DELETE;
public:
	typedef T *pointer_type;
	typedef I index_type;
	pointer_type operator->() const { return p; }
	operator pointer_type() const { return p; }
	operator const index_type&() const { return i; }
	bool operator==(const valptridx_t &rhs) const { return p == rhs.p; }
protected:
	valptridx_t(pointer_type t, index_type s) :
		p(t), i(s)
	{
	}
	pointer_type p;
	index_type i;
};

#define _DEFINE_VALPTRIDX_SUBTYPE_HEADER(N,I)	\
	template <typename T>	\
	struct N##_template_t : public valptridx_t<T, I>	\

#define _DEFINE_VALPTRIDX_SUBTYPE_TYPEDEFS(I)	\
	typedef valptridx_t<T, I> base_type;	\
	typedef typename base_type::pointer_type pointer_type;	\
	typedef typename base_type::index_type index_type;	\

#define _DEFINE_VALPTRIDX_SUBTYPE_CTOR2(N,A)	\
	N##_template_t(pointer_type t, index_type s) :	\
		base_type(t, s)	\
	{	\
		if (!t)	\
			throw std::logic_error("NULL pointer explicit constructor");	\
		if (&A[s] != t)	\
			throw std::logic_error("pointer/index mismatch");	\
	}	\

#define _DEFINE_VALPTRIDX_SUBTYPE_USERTYPE(name,base)	\
	struct name : public base {	\
		name() DXX_CXX11_EXPLICIT_DELETE;	\
		DXX_INHERIT_CONSTRUCTORS(name, base);	\
	}	\

#define _DEFINE_VALPTRIDX_SUBTYPE_USERTYPES(N,P)	\
	_DEFINE_VALPTRIDX_SUBTYPE_USERTYPE(N##_t, N##_template_t<P>);	\
	_DEFINE_VALPTRIDX_SUBTYPE_USERTYPE(c##N##_t, N##_template_t<P const>);	\

#ifdef DXX_HAVE_BUILTIN_CONSTANT_P
#define DXX_VALPTRIDX_STATIC_CHECK(I,E,F,S)	\
	if (dxx_builtin_constant_p(I) && !(E)) {	\
		void F() __attribute_error(S);	\
		F();	\
	}
#else
#define DXX_VALPTRIDX_STATIC_CHECK(I,E,F,S)	\

#endif

#define DEFINE_VALPTRIDX_SUBTYPE(N,P,I,A)	\
	_DEFINE_VALPTRIDX_SUBTYPE_HEADER(N,I)	\
	{	\
		_DEFINE_VALPTRIDX_SUBTYPE_TYPEDEFS(I)	\
		_DEFINE_VALPTRIDX_SUBTYPE_CTOR2(N,A)	\
		N##_template_t(pointer_type t) :	\
			base_type(t, t-A)	\
		{	\
			DXX_VALPTRIDX_STATIC_CHECK(t, t, dxx_trap_constant_null_pointer, "NULL pointer used");	\
			if (!t)	\
				throw std::logic_error("NULL pointer explicit constructor");	\
			if (&A[this->i] != t)	\
				throw std::logic_error("unaligned pointer");	\
		}	\
		N##_template_t(index_type s) :	\
			base_type(&A[s], s)	\
		{	\
			DXX_VALPTRIDX_STATIC_CHECK(s, static_cast<std::size_t>(s) < A.size(), dxx_trap_constant_invalid_index, "invalid index used in array subscript");	\
			if (!(static_cast<std::size_t>(s) < A.size()))	\
				throw std::out_of_range("index exceeds " #N " range");	\
		}	\
		template <index_type i>	\
		N##_template_t(const P##_magic_constant_t<i> &) :	\
			base_type(static_cast<std::size_t>(i) < A.size() ? &A[i] : NULL, i)	\
		{	\
		}	\
	};	\
	\
	_DEFINE_VALPTRIDX_SUBTYPE_USERTYPES(N,P);	\
	\
	static inline N##_t N(N##_t::pointer_type o, N##_t::index_type i) { return N##_t(o, i); }	\
	static inline c##N##_t N(c##N##_t::pointer_type o, c##N##_t::index_type i) { return c##N##_t(o, i); }	\

