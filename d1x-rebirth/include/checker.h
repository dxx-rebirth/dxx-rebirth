//checker.h added 05/17/99 Matt Mueller
//FD_* on linux use asm, but checker doesn't like it.  Borrowed these non-asm versions outta <selectbits.h>
#include <setjmp.h>

#ifdef __CHECKER__

#undef FD_ZERO(set)
#undef FD_SET(d, set)
#undef FD_CLR(d, set)
#undef FD_ISSET(d, set)

# define FD_ZERO(set)  \
  do {                                                                        \
	      unsigned int __i;                                                         \
			      for (__i = 0; __i < sizeof (__fd_set) / sizeof (__fd_mask); ++__i)        \
					        ((__fd_mask *) set)[__i] = 0;                                           \
								  } while (0)
# define FD_SET(d, set)       ((set)->fds_bits[__FDELT(d)] |= __FDMASK(d))
# define FD_CLR(d, set)       ((set)->fds_bits[__FDELT(d)] &= ~__FDMASK(d))
# define FD_ISSET(d, set)     ((set)->fds_bits[__FDELT(d)] & __FDMASK(d))

//checker doesn't seem to handle jmp's correctly...
#undef setjmp(env)
#define setjmp(env) __chcksetjmp(env)
#undef longjmp(env,val)
#define longjmp(env,val) __chcklongjmp(env,val)

int __chcklongjmp(jmp_buf buf,int val);
int __chcksetjmp(jmp_buf buf);
	
void chcksetwritable(char * p, int size);
void chcksetunwritable(char * p, int size);

#endif	
