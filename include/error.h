/* $Id: error.h,v 1.12 2004-08-28 23:17:45 schaffner Exp $ */
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
COPYRIGHT 1993-1998 PARALLAX SOFTWARE CORPORATION.  ALL RIGHTS RESERVED.
*/

/*
 *
 * Header for error handling/printing/exiting code
 *
 */

#ifndef _ERROR_H
#define _ERROR_H

#include <stdio.h>

#ifdef __GNUC__
#define __noreturn __attribute__ ((noreturn))
#define __format __attribute__ ((format (printf, 1, 2)))
#else
#define __noreturn
#define __format
#endif

int error_init(void (*func)(char *), char *fmt,...);    //init error system, set default message, returns 0=ok
void set_exit_message(char *fmt,...);	//specify message to print at exit
void Warning(char *fmt,...);				//print out warning message to user
void set_warn_func(void (*f)(char *s));//specifies the function to call with warning messages
void clear_warn_func(void (*f)(char *s));//say this function no longer valid
void _Assert(int expr,char *expr_text,char *filename,int linenum);	//assert func
void Error(char *fmt,...) __noreturn __format;				//exit with error code=1, print message
void Assert(int expr);
void Int3();
#ifndef NDEBUG		//macros for debugging

#ifdef NO_ASM
# if defined(__APPLE__) || defined(macintosh)
extern void Debugger(void);	// Avoids some name clashes
#  define Int3 Debugger
# else
//# define Int3() Error("int 3 %s:%i\n",__FILE__,__LINE__);
//# define Int3() {volatile int a=0,b=1/a;}
#  define Int3() ((void)0)
# endif // Macintosh

#else // NO_ASM

#ifdef __GNUC__
#ifdef SDL_INPUT
#include <SDL.h>
#endif
#include "args.h"
static inline void _Int3()
{
	if (FindArg("-debug")) {
#ifdef SDL_INPUT
		SDL_WM_GrabInput(SDL_GRAB_OFF);
#endif
		asm("int $3");
	}
}
#define Int3() _Int3()

#elif defined __WATCOMC__
void Int3(void);								      //generate int3
#pragma aux Int3 = "int 3h";

#elif defined _MSC_VER
static __inline void _Int3()
{
	__asm { int 3 }
}
#define Int3() _Int3()

#else
#error Unknown Compiler!
#endif

#endif // NO_ASM

#define Assert(expr) ((expr)?(void)0:(void)_Assert(0,#expr,__FILE__,__LINE__))

#ifdef __GNUC__
//#define Error(format, args...) ({ /*Int3();*/ Error(format , ## args); })
#elif defined __WATCOMC__
//make error do int3, then call func
#pragma aux Error aborts = \
	"int	3"	\
	"jmp Error";

//#pragma aux Error aborts;
#else
// DPH: I'm not going to bother... it's not needed... :-)
#endif

#ifdef __WATCOMC__
//make assert do int3 (if expr false), then call func
#pragma aux _Assert parm [eax] [edx] [ebx] [ecx] = \
	"test eax,eax"		\
	"jnz	no_int3"		\
	"int	3"				\
	"no_int3:"			\
	"call _Assert";
#endif

#else					//macros for real game

#ifdef __WATCOMC__
#pragma aux Error aborts;
#endif
//Changed Assert and Int3 because I couldn't get the macros to compile -KRB
#define Assert(__ignore) ((void)0)
//void Assert(int expr);
#define Int3() ((void)0)
//void Int3();
#endif

#endif /* _ERROR_H */
