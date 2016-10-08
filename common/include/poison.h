#pragma once

#ifdef DXX_HAVE_POISON_VALGRIND
#include <valgrind/memcheck.h>
#endif

template <typename T>
static inline void DXX_MAKE_MEM_UNDEFINED(T *b, unsigned long l)
{
	(void)b;(void)l;
#ifdef DXX_HAVE_POISON_VALGRIND
	VALGRIND_MAKE_MEM_UNDEFINED(b, l);
#define DXX_HAVE_POISON_UNDEFINED 1
#endif
}

template <typename T>
static inline void DXX_MAKE_MEM_UNDEFINED(T *b, T *e)
{
	unsigned char *bc = reinterpret_cast<unsigned char *>(b);
	DXX_MAKE_MEM_UNDEFINED(bc, reinterpret_cast<unsigned char *>(e) - bc);
}

template <typename T>
static inline void DXX_MAKE_VAR_UNDEFINED(T &b)
{
	unsigned char *const bc = reinterpret_cast<unsigned char *>(&b);
	DXX_MAKE_MEM_UNDEFINED(bc, sizeof(T));
}

template <typename T, typename V>
static inline void _DXX_POISON_MEMORY_RANGE(T b, T e, const V &v)
{
#if DXX_HAVE_POISON
	int store = DXX_HAVE_POISON_OVERWRITE;
#ifdef DXX_HAVE_POISON_VALGRIND
	if (!store)
		store |= RUNNING_ON_VALGRIND;
#endif
	if (!store)
		return;
	for (; b != e; ++b)
		*b = v;
#else
	(void)b;(void)e;(void)v;
#endif
}

template <typename T, typename V>
static inline void DXX_POISON_MEMORY(T b, unsigned long l, const V &v)
{
	_DXX_POISON_MEMORY_RANGE(b, b + l, v);
	DXX_MAKE_MEM_UNDEFINED(b, l);
}

template <typename T, typename V>
static inline void DXX_POISON_MEMORY(T b, T e, const V &v)
{
	_DXX_POISON_MEMORY_RANGE(b, e, v);
	DXX_MAKE_MEM_UNDEFINED(b, e);
}

#ifndef DXX_HAVE_POISON_UNDEFINED
#define DXX_HAVE_POISON_UNDEFINED	0
#endif
