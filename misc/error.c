/* $Id: error.c,v 1.3 2002-08-02 11:05:26 btb Exp $ */
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
 * Old Log:
 * Revision 1.12  1994/12/07  18:49:39  matt
 * error_init() can now take NULL as parm
 * 
 * Revision 1.11  1994/11/29  15:42:07  matt
 * Added newline before error message
 * 
 * Revision 1.10  1994/11/27  23:20:39  matt
 * Made changes for new mprintf calling convention
 * 
 * Revision 1.9  1994/06/20  21:20:56  matt
 * Allow NULL for warn func, to kill warnings
 * 
 * Revision 1.8  1994/05/20  15:11:35  mike
 * mprintf Warning message so you can actually see it.
 * 
 * Revision 1.7  1994/02/10  18:02:38  matt
 * Changed 'if DEBUG_ON' to 'ifndef NDEBUG'
 * 
 * Revision 1.6  1993/10/17  18:19:10  matt
 * If error_init() not called, Error() now prints the error message before
 * calling exit()
 * 
 * Revision 1.5  1993/10/14  15:29:11  matt
 * Added new function clear_warn_func()
 * 
 * Revision 1.4  1993/10/08  16:17:19  matt
 * Made Assert() call function _Assert(), rather to do 'if...' inline.
 * 
 * Revision 1.3  1993/09/28  12:45:25  matt
 * Fixed wrong print call, and made Warning() not append a CR to string
 * 
 * Revision 1.2  1993/09/27  11:46:35  matt
 * Added function set_warn_func()
 * 
 * Revision 1.1  1993/09/23  20:17:33  matt
 * Initial revision
 * 
 *
 */

#ifdef HAVE_CONFIG_H
#include <conf.h>
#endif

#ifdef RCS
static char rcsid[] = "$Id: error.c,v 1.3 2002-08-02 11:05:26 btb Exp $";
#endif

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>

#include "pstypes.h"
#include "console.h"
#include "error.h"

#define MAX_MSG_LEN 256

//edited 05/17/99 Matt Mueller added err_ prefix to prevent conflicts with statically linking SDL
int err_initialized=0;
//end edit -MM

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

void _Assert(int expr,char *expr_text,char *filename,int linenum)
{
	if (!(expr)) Error("Assertion failed: %s, file %s, line %d",expr_text,filename,linenum);

}

void print_exit_message(void)
{
	if (*exit_message)
		con_printf(CON_CRITICAL, "%s\n",exit_message);
}

//terminates with error code 1, printing message
void Error(char *fmt,...)
{
	va_list arglist;

	strcpy(exit_message,"\nError: ");

	va_start(arglist,fmt);
	vsprintf(exit_message+strlen(exit_message),fmt,arglist);
	va_end(arglist);

	con_printf(CON_CRITICAL, exit_message);

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

	con_printf(CON_URGENT, warn_message);

}

//initialize error handling system, and set default message. returns 0=ok
int error_init(char *fmt,...)
{
	va_list arglist;
	int len;

	atexit(print_exit_message);		//last thing at exit is print message

	if (fmt != NULL) {
		va_start(arglist,fmt);
		len = vsprintf(exit_message,fmt,arglist);
		va_end(arglist);
		if (len==-1 || len>MAX_MSG_LEN) Error("Message too long in error_init (len=%d, max=%d)",len,MAX_MSG_LEN);
	}

	err_initialized=1;

	return 0;
}
