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
 * Error handling/printing/exiting code
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>

#include "pstypes.h"
#include "console.h"
#include "error.h"
#include "inferno.h"

#define MAX_MSG_LEN 256

//edited 05/17/99 Matt Mueller added err_ prefix to prevent conflicts with statically linking SDL
int err_initialized=0;
//end edit -MM

static void (*ErrorPrintFunc)(char *);

char exit_message[MAX_MSG_LEN]="";
char warn_message[MAX_MSG_LEN];

//takes string in register, calls printf with string on stack
void warn_printf(char *s)
{
	con_printf(CON_URGENT,"%s\n",s);
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

void _Assert(int expr,char *expr_text,char *filename,int linenum)
{
	Int3();
	if (!(expr)) Error("Assertion failed: %s, file %s, line %d",expr_text,filename,linenum);
}

void print_exit_message(void)
{
	if (*exit_message)
	{
		if (ErrorPrintFunc)
		{
			(*ErrorPrintFunc)(exit_message);
		}
		con_printf(CON_CRITICAL,"\n%s\n",exit_message);
	}
}

//terminates with error code 1, printing message
void Error(char *fmt,...)
{
	va_list arglist;

	strcpy(exit_message,"Error: "); // don't put the new line in for dialog output
	va_start(arglist,fmt);
	vsprintf(exit_message+strlen(exit_message),fmt,arglist);
	va_end(arglist);

	Int3();

	/*if (!err_initialized)*/ print_exit_message();

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

	(*warn_func)(warn_message);

}

//initialize error handling system, and set default message. returns 0=ok
int error_init(void (*func)(char *), char *fmt, ...)
{
	va_list arglist;
	int len;

	//atexit(print_exit_message);		//last thing at exit is print message CHRIS: Removed to allow newmenu dialog

	ErrorPrintFunc = func;          // Set Error Print Functions

	if (fmt != NULL) {
		va_start(arglist,fmt);
		len = vsprintf(exit_message,fmt,arglist);
		va_end(arglist);
		if (len==-1 || len>MAX_MSG_LEN) Error("Message too long in error_init (len=%d, max=%d)",len,MAX_MSG_LEN);
	}

	err_initialized=1;

	return 0;
}
