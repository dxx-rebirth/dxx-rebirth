/* $Id: error.h,v 1.8 2003-04-08 00:59:17 btb Exp $ */
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
 * Old Log:
 * Revision 1.12  1994/06/17  15:22:46  matt
 * Added pragma for Error() for when NDEBUG
 *
 * Revision 1.11  1994/03/07  13:22:14  matt
 * Since the Error() function has 'aborts' set in pragma, we do a jmp
 * to the function rather than call.
 *
 * Revision 1.10  1994/02/17  12:37:15  matt
 * Combined two pragma's for Error(), since second superseded the first
 *
 * Revision 1.9  1994/02/10  18:02:53  matt
 * Changed 'if DEBUG_ON' to 'ifndef NDEBUG'
 *
 * Revision 1.8  1994/02/09  15:18:29  matt
 * Added pragma saying that Error() never returns
 *
 * Revision 1.7  1993/10/19  12:57:53  matt
 * If DEBUG_ON not defined, define it to be 1
 *
 * Revision 1.6  1993/10/15  21:40:39  matt
 * Made error functions generate int3's if debugging on
 *
 * Revision 1.5  1993/10/14  15:29:22  matt
 * Added new function clear_warn_func()
 *
 * Revision 1.4  1993/10/08  16:16:47  matt
 * Made Assert() call function _Assert(), rather to do 'if...' inline.
 *
 * Revision 1.3  1993/09/29  11:39:07  matt
 * Added Assert() macro, like the system one, but calls Error()
 *
 * Revision 1.2  1993/09/27  11:47:03  matt
 * Added function set_warn_func()
 *
 * Revision 1.1  1993/09/23  20:17:46  matt
 * Initial revision
 *
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

#ifdef __GNUC__
#ifdef NO_ASM
//#define Int3() Error("int 3 %s:%i\n",__FILE__,__LINE__);
//#define Int3() {volatile int a=0,b=1/a;}
#define Int3() ((void)0)
#else
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
#endif

#elif defined __WATCOMC__
void Int3(void);								      //generate int3
#pragma aux Int3 = "int 3h";
#elif defined _MSC_VER
#define Int3() __asm { int 3 }
#else
#error Unknown Compiler!
#endif

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
