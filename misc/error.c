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
#include "dxxerror.h"
#include "inferno.h"

#define MAX_MSG_LEN 256

static void (*ErrorPrintFunc)(const char *);

char warn_message[MAX_MSG_LEN];

//takes string in register, calls printf with string on stack
void warn_printf(char *s)
{
	con_printf(CON_URGENT, "%s\n",s);
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

void print_exit_message(const char *exit_message)
{
		if (ErrorPrintFunc)
		{
			(*ErrorPrintFunc)(exit_message);
		}
		con_printf(CON_CRITICAL, "\n%s\n",exit_message);
}

//terminates with error code 1, printing message
void Error(const char *fmt,...)
{
	char exit_message[MAX_MSG_LEN]="Error: "; // don't put the new line in for dialog output
	va_list arglist;

	va_start(arglist,fmt);
	vsprintf(exit_message+strlen(exit_message),fmt,arglist);
	va_end(arglist);

	Int3();

	print_exit_message(exit_message);

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
int error_init(void (*func)(const char *))
{
	ErrorPrintFunc = func;          // Set Error Print Functions
	return 0;
}
