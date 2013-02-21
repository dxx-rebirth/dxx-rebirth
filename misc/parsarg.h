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


/*
 *
 * Routine to parse the command line. Will also read and parse args
 * from a file.
 *
 * parse_args() is called with argc & argv from main(), the function
 * to be called with each argument, and flags.  argc & argv are usually
 * adjusted to not pass the first parameter (the program file name).
 * Thus the general method of calling is:
 *
 *  parse_args(argc-1,argv+1,hander_func,flags);
 *
 * handler_func() is then called with each parameter.
 *
 * If the PA_EXPAND flag is passed, all arguments which do not start
 * with '-' are assumed to be filenames and are expanded for wildcards,
 * with the handler function called for each match.  If a spec matches
 * nothing, the spec itself is passed to the handler func.
 *
 * Args that start with '@' are assumed to be argument files.  These
 * files are opened, and arguments are read from them just as if they
 * were specified on the command line.  Arg files can be nested.
 *
 */

//Flags

#define PA_EXPAND	1	//wildcard expand args that don't start with '-'


//Function

void parse_args(int argc,char **argv,void (*handler_func)(char *arg),int flags);

