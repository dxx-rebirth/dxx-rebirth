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

#pragma off (unreferenced)
static char rcsid[] = "$Id: error.c,v 1.1.1.1 2001-01-19 03:30:15 bradleyb Exp $";
#pragma on (unreferenced)

#if defined(__NT__)
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>

#include "mono.h"
#include "error.h"
#include "args.h"

#ifdef MACINTOSH
	#include <Types.h>
	#include <Strings.h>
	#include <LowMem.h>
	
	#include "resource.h"
#endif

#define MAX_MSG_LEN 256

int initialized=0;

static void (*ErrorPrintFunc)(char *);

char exit_message[MAX_MSG_LEN]="";
char warn_message[MAX_MSG_LEN];

//takes string in register, calls printf with string on stack
void warn_printf(char *s)
{
	printf("%s\n",s);
}

void (*warn_func)(char *s)=warn_printf;

//provides a function to call with warning messages
void set_warn_func(void (*f)(char *s))
{
	warn_func = f;
}

//uninstall warning function - install default printf
void clear_warn_func(void (*f)(char *s))
{
	warn_func = warn_printf;
}

void set_exit_message(char *fmt,...)
{
	va_list arglist;
	int len;

	va_start(arglist,fmt);
	len = vsprintf(exit_message,fmt,arglist);
	va_end(arglist);

	if (len==-1 || len>MAX_MSG_LEN) Error("Message too long in set_exit_message (len=%d, max=%d)",len,MAX_MSG_LEN);

}

#ifdef __NT__
void _Assert(int expr, char *expr_text, char *filename, int linenum)
{
	if (!(expr)) {
	#ifndef NDEBUG
		if (FindArg("-debugmode")) DebugBreak();
	#endif
		Error("Assertion failed: %s, file %s, line %d", expr_text, filename, linenum);
	}
}
#else
void _Assert(int expr,char *expr_text,char *filename,int linenum)
{
	if (!(expr)) Error("Assertion failed: %s, file %s, line %d",expr_text,filename,linenum);

}
#endif

void print_exit_message()
{
	if (*exit_message)
	{
		if (ErrorPrintFunc)
		{
			(*ErrorPrintFunc)(exit_message);
		}
		else
		{
			#if (defined(MACINTOSH) && defined(NDEBUG) && defined(RELEASE))
				
				c2pstr(exit_message);
				ShowCursor();
				ParamText(exit_message, "\p", "\p", "\p");
				StopAlert(ERROR_ALERT, nil);
				
			#else
				
				printf("%s\n",exit_message);
			
			#endif
		}
	}
}

//terminates with error code 1, printing message
void Error(char *fmt,...)
{
	va_list arglist;

	#if (defined(MACINTOSH) && defined(NDEBUG) && defined(RELEASE))
		strcpy(exit_message,"Error: ");		// don't put the new line in for dialog output
	#else
		strcpy(exit_message,"\nError: ");
	#endif
	va_start(arglist,fmt);
	vsprintf(exit_message+strlen(exit_message),fmt,arglist);
	va_end(arglist);

	Int3();

	if (!initialized) print_exit_message();

	exit(1);
}

//print out warning message to user
void Warning(char *fmt,...)
{
	va_list arglist;

	if (warn_func == NULL)
		return;

	strcpy(warn_message,"Warning: ");

	va_start(arglist,fmt);
	vsprintf(warn_message+strlen(warn_message),fmt,arglist);
	va_end(arglist);

	mprintf((0, "%s\n", warn_message));
	(*warn_func)(warn_message);

}

//initialize error handling system, and set default message. returns 0=ok
int error_init(void (*func)(char *), char *fmt,...)
{
	va_list arglist;
	int len;

	atexit(print_exit_message);		//last thing at exit is print message

	ErrorPrintFunc = func;				// Set Error Print Functions

	if (fmt != NULL) {
		va_start(arglist,fmt);
		len = vsprintf(exit_message,fmt,arglist);
		va_end(arglist);
		if (len==-1 || len>MAX_MSG_LEN) Error("Message too long in error_init (len=%d, max=%d)",len,MAX_MSG_LEN);
	}

	initialized=1;

	return 0;
}

#ifdef MACINTOSH

int MacEnableInt3 = 1;

void MacAssert(int expr, char *expr_text, char *filename, int linenum)
{
	if (!(expr)) {
		Int3();
//		Error("Assertion failed: %s, file %s, line %d", expr_text, filename, linenum);
	}
}

#endif

#if defined(__NT__)

int WinEnableInt3 = 1;

void WinInt3()
{
	if (WinEnableInt3) 
		DebugBreak();
}
#endif
