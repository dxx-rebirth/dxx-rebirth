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

int error_init(void (*func)(char *), char *fmt,...);			//init error system, set default message, returns 0=ok
void set_exit_message(char *fmt,...);	//specify message to print at exit
void Warning(char *fmt,...);				//print out warning message to user
void set_warn_func(void (*f)(char *s));//specifies the function to call with warning messages
void clear_warn_func(void (*f)(char *s));//say this function no longer valid
void _Assert(int expr,char *expr_text,char *filename,int linenum);	//assert func
void Error(char *fmt,...);					//exit with error code=1, print message

#ifndef NDEBUG		//macros for debugging

#ifndef MACINTOSH

	#if defined(__NT__) 
		void WinInt3();
		#define Int3() WinInt3()
		#define Assert(expr) _Assert(expr, #expr, __FILE__, __LINE__)
	#else // ifdef __NT__
		void Int3(void);									//generate int3
		#pragma aux Int3 = "int 3h";

		#define Assert(expr) _Assert(expr,#expr,__FILE__,__LINE__)

	//make error do int3, then call func
		#pragma aux Error aborts = \
			"int	3"	\
		"jmp Error";

	//#pragma aux Error aborts;

	//make assert do int3 (if expr false), then call func
		#pragma aux _Assert parm [eax] [edx] [ebx] [ecx] = \
			"test eax,eax"		\
			"jnz	no_int3"		\
			"int	3"				\
			"no_int3:"			\
			"call _Assert";
	#endif	

#else		// ifndef MACINTOSH

extern int MacEnableInt3;

#define Int3() do { key_close(); if (MacEnableInt3) Debugger(); } while(0)
#define Assert(expr) MacAssert(expr,#expr,__FILE__,__LINE__)

#endif		// ifndef MACINTOSH

#else					//macros for real game

#if !defined(__NT__)
#pragma aux Error aborts;
#endif

#define Assert(__ignore) ((void)0)
#define Int3() ((void)0)

#endif


