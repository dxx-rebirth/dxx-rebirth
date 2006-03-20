#include <stdio.h>
#include <stdarg.h>
void mopen() {}
void mclose() {}
void _mprintf(short n, char * format, ...) {
#ifndef NMONO
	va_list args;
	va_start(args, format );
	vprintf(format, args);
	va_end(args);
#endif
}

