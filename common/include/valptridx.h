#pragma once
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

#define _DEFINE_VALPTRIDX_SUBTYPE_CTOR2(N)	\
	N##_template_t(pointer_type t, index_type s) :	\
		base_type(t, s)	\
	{	\
	}	\

#define _DEFINE_VALPTRIDX_SUBTYPE_USERTYPES(N,P)	\
	struct N##_t : public N##_template_t<P> {	\
		N##_t() DXX_CXX11_EXPLICIT_DELETE;	\
		DXX_INHERIT_CONSTRUCTORS(N##_t, N##_template_t<P>)	\
	};	\
	struct c##N##_t : public N##_template_t<P const> {	\
		c##N##_t() DXX_CXX11_EXPLICIT_DELETE;	\
		DXX_INHERIT_CONSTRUCTORS(c##N##_t, N##_template_t<P const>)	\
	}	\

#define DEFINE_VALPTRIDX_SUBTYPE(N,P,I,A)	\
	_DEFINE_VALPTRIDX_SUBTYPE_HEADER(N,I)	\
	{	\
		_DEFINE_VALPTRIDX_SUBTYPE_TYPEDEFS(I)	\
		_DEFINE_VALPTRIDX_SUBTYPE_CTOR2(N)	\
		N##_template_t(pointer_type t) :	\
			base_type(t, t-A)	\
		{	\
		}	\
		N##_template_t(index_type s) :	\
			base_type(&A[s], s)	\
		{	\
		}	\
	};	\
	\
	_DEFINE_VALPTRIDX_SUBTYPE_USERTYPES(N,P);	\

