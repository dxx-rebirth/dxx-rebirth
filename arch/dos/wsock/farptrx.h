#ifndef ___farptrx_h___
#define ___farptrx_h___

extern __inline__ void _farpokex (unsigned short selector, unsigned long offset, void *x, int len)
{
    int dummy1, dummy2, dummy3;

    __asm__ __volatile__ ("pushl %%es\n"
 			"movw %w3, %%es\n"
                         "rep\n"
                         "movsb\n"
                         "popl %%es"

       : "=c" (dummy1), "=S" (dummy2), "=D" (dummy3)
       : "rm" (selector), "1" (x), "2" (offset), "0" (len));
}

extern __inline__ void _farpeekx (unsigned short selector, unsigned long offset, void *x, int len)
{
    int dummy1, dummy2, dummy3;

    __asm__ __volatile__ ("pushl %%ds\n"
 			"movw %w3,%%ds\n"
                         "rep\n"
                         "movsb\n"
                         "popl %%ds"
       : "=c" (dummy1), "=S" (dummy2), "=D" (dummy3)
       : "rm" (selector), "1" (offset), "2" (x), "0" (len));
}
#endif 