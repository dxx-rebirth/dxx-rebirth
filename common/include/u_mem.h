/*
 * Portions of this file are copyright Rebirth contributors and licensed as
 * described in COPYING.txt.
 * Portions of this file are copyright Parallax Software and licensed
 * according to the Parallax license below.
 * See COPYING.txt for license details.

THE COMPUTER CODE CONTAINED HEREIN IS THE SOLE PROPERTY OF PARALLAX
SOFTWARE CORPORATION ("PARALLAX").  PARALLAX, IN DISTRIBUTING THE CODE TO
END-USERS, AND SUBJECT TO ALL OF THE TERMS AND CONDITIONS HEREIN, GRANTS A
ROYALTY-FREE, PERPETUAL LICENSE TO SUCH END-USERS FOR USE BY SUCH END-USERS
IN USING, DISPLAYING,  AND CREATING DERIVATIVE WORKS THEREOF, SO LONG AS
SUCH USE, DISPLAY OR CREATION IS FOR NON-COMMERCIAL, ROYALTY OR REVENUE
FREE PURPOSES.  IN NO EVENT SHALL THE END-USER USE THE COMPUTER CODE
CONTAINED HEREIN FOR REVENUE-BEARING PURPOSES.  THE END-USER UNDERSTANDS
AND AGREES TO THE TERMS HEREIN AND ACCEPTS THE SAME BY USE OF THIS FILE.
COPYRIGHT 1993-1999 PARALLAX SOFTWARE CORPORATION.  ALL RIGHTS RESERVED.
*/

#pragma once

#include <stdlib.h>

#ifdef __cplusplus
#include <memory>
#include "dxxsconf.h"
#include "compiler-exchange.h"
#include "compiler-type_traits.h"

#define MEM_K 1.5	// Dynamic array growth factor

#ifdef DEBUG_MEMORY_ALLOCATIONS
void mem_init(void);

void mem_display_blocks();
void * mem_malloc( unsigned int size, const char * var, const char * file, unsigned line) __attribute_alloc_size(1) __attribute_malloc();
void * mem_calloc( size_t nmemb, size_t size, const char * var, const char * filename, unsigned line) __attribute_alloc_size(1,2) __attribute_malloc();
void * mem_realloc( void * buffer, unsigned int size, const char * var, const char * file, int line ) __attribute_alloc_size(2);
extern void mem_free( void * buffer );

/* DPH: Changed malloc, etc. to d_malloc. Overloading system calls is very evil and error prone */

// Checks to see if any blocks are overwritten
void mem_validate_heap();

#define mem_calloc(nmemb,size,var,file,line)	((mem_calloc)((size) ? (nmemb) : 0, (size), (var), (file), (line)))

#else

#define mem_malloc(size,var,file,line)	((void)var,(void)file,(void)line,malloc((size)))
#define mem_calloc(nmemb,size,var,file,line)	((void)var,(void)file,(void)line,calloc((nmemb),(size)))
#define mem_realloc(ptr,size,var,file,line)	((void)var,(void)file,(void)line,realloc((ptr),(size)))
#define mem_free	free

static inline void mem_init(void)
{
}
#endif

template <typename T>
T *MALLOC(T *&r, std::size_t count, const char *var, const char *file, unsigned line)
{
	static_assert(tt::is_pod<T>::value, "MALLOC cannot allocate non-POD");
	return r = reinterpret_cast<T *>(mem_malloc(count, var, file, line));
}

template <typename T>
T *CALLOC(T *&r, std::size_t count, const char *var, const char *file, unsigned line)
{
	static_assert(tt::is_pod<T>::value, "CALLOC cannot allocate non-POD");
	return r = reinterpret_cast<T *>(mem_calloc(count, sizeof(T), var, file, line));
}

#define d_malloc(size)      mem_malloc((size),"Unknown", __FILE__,__LINE__ )
#define d_calloc(nmemb,size)    mem_calloc((nmemb),(size),"Unknown", __FILE__,__LINE__ )
#define d_realloc(ptr,size) mem_realloc((ptr),(size),"Unknown", __FILE__,__LINE__ )
template <typename T>
static inline void d_free(T *&ptr)
{
	static_assert(tt::is_same<T, void>::value || tt::is_pod<T>::value, "d_free cannot free non-POD");
	mem_free(exchange(ptr, nullptr));
}

template <typename T>
class RAIIdmem_deleter
{
public:
	void operator()(T *v) const
	{
		d_free(v);
	}
};

template <typename T>
class RAIIdmem : public std::unique_ptr<T, RAIIdmem_deleter<T>>
{
	static_assert(tt::is_pod<T>::value, "RAIIdmem cannot manage non-POD");
public:
	DXX_INHERIT_CONSTRUCTORS(RAIIdmem, std::unique_ptr<T, RAIIdmem_deleter<T>>);
	operator T*() const
#ifdef DXX_HAVE_CXX11_REF_QUALIFIER
		&
#endif
	{
		return this->get();
	}
};

template <typename T>
T *MALLOC(RAIIdmem<T> &r, std::size_t count, const char *var, const char *file, unsigned line)
{
	T *p;
	return r = RAIIdmem<T>(MALLOC<T>(p, count, var, file, line));
}

template <typename T>
T *CALLOC(RAIIdmem<T> &r, std::size_t count, const char *var, const char *file, unsigned line)
{
	T *p;
	return r = RAIIdmem<T>(CALLOC<T>(p, count, var, file, line));
}

#define MALLOC( var, type, count )	(MALLOC<type>(var, (count)*sizeof(type),#var, __FILE__,__LINE__ ))
#define CALLOC( var, type, count )	(CALLOC<type>(var, (count),#var, __FILE__,__LINE__ ))

typedef RAIIdmem<unsigned char> RAIIdubyte;
typedef RAIIdmem<char> RAIIdchar;
#endif
