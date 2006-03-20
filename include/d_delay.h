//added on 9/2/98 by Matt Mueller

#ifndef _D_DELAY
#define _D_DELAY

#if (defined(__LINUX__) || defined(__WINDOWS__))
#define SUPPORTS_NICEFPS
#endif

//#ifdef __LINUX__
  void d_delay (int ms);
/*#else
#include <dos.h>
#define d_delay delay
#endif
  */
#endif
