/*
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

#ifndef _U_MEM_H
#define _U_MEM_H

#include <stdlib.h>

#ifdef __cplusplus
#include "dxxsconf.h"

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

#define mem_malloc(size,var,file,line)	malloc((size))
#define mem_calloc(nmemb,size,var,file,line)	calloc((nmemb), (size))
#define mem_realloc(ptr,size,var,file,line)	realloc((ptr),(size))
#define mem_free	free

static inline void mem_init(void)
{
}
#endif

#define MALLOC( var, type, count )	(var=(type *)mem_malloc((count)*sizeof(type),#var, __FILE__,__LINE__ ))
#define CALLOC( var, type, count )	(var=(type *)mem_calloc((count),sizeof(type),#var, __FILE__,__LINE__ ))

#define d_malloc(size)      mem_malloc((size),"Unknown", __FILE__,__LINE__ )
#define d_calloc(nmemb,size)    mem_calloc((nmemb),(size),"Unknown", __FILE__,__LINE__ )
#define d_realloc(ptr,size) mem_realloc((ptr),(size),"Unknown", __FILE__,__LINE__ )
#define d_free(ptr)         (mem_free(ptr), ptr=NULL)

class BaseRAIIdmem
{
	BaseRAIIdmem(const BaseRAIIdmem&);
	BaseRAIIdmem& operator=(const BaseRAIIdmem&);
protected:
	void *p;
	BaseRAIIdmem() : p(NULL) {}
	BaseRAIIdmem(void *v) : p(v) {}
	~BaseRAIIdmem() {
		*this = NULL;
	}
	BaseRAIIdmem& operator=(void *v)
	{
		if (p != v)
		{
#ifdef DEBUG_MEMORY_ALLOCATIONS
			if (p)	// Avoid bogus warning about freeing NULL
#endif
				d_free(p);
			p = v;
		}
		return *this;
	}
};

template <typename T>
class RAIIdmem : public BaseRAIIdmem
{
	RAIIdmem(const RAIIdmem&);
	RAIIdmem& operator=(const RAIIdmem&);
public:
	RAIIdmem() {}
	RAIIdmem(T *v) : BaseRAIIdmem(v) {}
	operator T*() const { return static_cast<T*>(p); }
	T *operator->() const { return static_cast<T*>(p); }
	RAIIdmem& operator=(T *v)
	{
		BaseRAIIdmem::operator=(v);
		return *this;
	}
};

typedef RAIIdmem<unsigned char> RAIIdubyte;
typedef RAIIdmem<char> RAIIdchar;
#endif

#endif // _U_MEM_H
