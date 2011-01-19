#ifndef _FVI_A_H
#define _FVI_A_H

#ifdef __GNUC__
static inline int oflow_check(fix a,fix b) {
  register int __ret;
  int dummy;
  __asm__ (
    " cdq;"
    " xorl  %%edx,%%eax;"
    " subl  %%edx,%%eax;"
    " xchgl %%ebx,%%eax;"
    " cdq;"
    " xorl  %%edx,%%eax;"
    " subl  %%edx,%%eax;"
    " imull  %%ebx;"
    " sarl  $15,%%edx;"
    " orw   %%dx,%%dx;"
    " setnz %%al;"
    " movzbl %%al,%%eax"
     : "=a" (__ret), "=b" (dummy) : "a" (a), "1" (b) : "%edx");
    return __ret;
}
#else
static int oflow_check(fix a,fix b) {
	return 0; /* hoping the floating point fix-math is used */
}
/*#error unknown compiler*/
#endif
#endif
