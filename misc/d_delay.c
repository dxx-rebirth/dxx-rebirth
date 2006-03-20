//added on 9/2/98 by Matt Mueller
#include "d_delay.h"

#ifndef d_delay

#ifdef __WINDOWS__

#include <windows.h>

void d_delay (int ms) {
 Sleep(ms);
}

#else

#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>
#include <time.h>

void d_delay (int ms) {
#if 0
	struct timeval tv;
	tv.tv_sec=ms/1000;
//edited 02/06/99 Matt Mueller - microseconds, not milliseconds
        tv.tv_usec=(ms%1000)*1000;
//end edit -MM
	select(0,NULL,NULL,NULL,&tv);
#elif 0
	struct timespec tv;
	tv.tv_sec=ms/1000;
	tv.tv_nsec=(ms%1000)*1000000;//nanoseconds
	nanosleep(&tv,NULL);
#else
	usleep(ms*1000);
#endif
}

#endif
#endif
