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
static char rcsid[] = "$Id: parsarg.c,v 1.1.1.1 2001-01-19 03:30:14 bradleyb Exp $";
#pragma on (unreferenced)

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dos.h>

#include "parsarg.h"

#define ARGBUF_SIZE 500
#define MAX_ARGS 100

char arg_buf[ARGBUF_SIZE];
char *ab_ptr=arg_buf;

void perror_exit(int n,char *s)
{
	perror(s);
	exit(n);
}

void verror(char *s,void *first_arg_ptr)
{
	fprintf(stderr,"Error: ");
	vfprintf(stderr,s,first_arg_ptr);
	fprintf(stderr,"\n");
}

void error_exit(int ret_code,char *s,...)
{
	verror(s,((char *) &s)+sizeof(s));	/* addr of next parm on stack */
	exit(ret_code);
}

void parse_args(int argc,char **argv,void (*handler_func)(char *arg),int flags)
{

	for (;argc--;argv++) {
		if (**argv=='@') {			/* read args from file */
			char *arg_ptrs[MAX_ARGS];
			int arg_count;
			FILE *argfile;
			int len;
			char *p=ab_ptr,c;

			if ((argfile=fopen(*argv+1,"rt"))==0) perror_exit(10,*argv+1);
			if ((len=fread(ab_ptr,1,ARGBUF_SIZE-((int) (ab_ptr-arg_buf)),argfile))==ARGBUF_SIZE-(ab_ptr-arg_buf)) error_exit(20,"Argument buffer not big enough\n");
			fclose(argfile);
			ab_ptr[len++]=0;		/* write terminating null */

			/* remove comments */

			while ((p=strchr(ab_ptr,';'))!=NULL) {
				char *p2=strchr(p,'\n');

				if (p2) { 	/* found cr */
					strcpy(p,p2);	/* copy over comment */
					len = strlen(ab_ptr);
				}
				else {		/* no cr, end of string */
					*p=0;
					len = (int) (p-ab_ptr);
				}
			}
			ab_ptr[len]=0;		/* write terminating null */

			while (!ab_ptr[len-1]) len--;	/* back up over terminating nulls */
			p=ab_ptr;


			for (arg_count=0;p<ab_ptr+len;) {
				while (p<ab_ptr+len && ((c=*p)==' ' || c=='\t' || c=='\n')) p++;
				if (p<ab_ptr+len) {	/* found parm? */
					arg_ptrs[arg_count++]=p;
					if (arg_count>=MAX_ARGS) error_exit(10,"Too many args");
					while (p<ab_ptr+len && !((c=*p)==' ' || c=='\t' || c=='\n')) p++;
					*p++=0;
				}
			}
			ab_ptr+=len;
			parse_args(arg_count,arg_ptrs,handler_func,flags);
		}
		else
			if (flags&PA_EXPAND && (**argv != '-')) {
				struct find_t ffblk;
				char drive[_MAX_DRIVE],dir[_MAX_DIR];
				char filename[_MAX_DRIVE+_MAX_DIR+13],*nptr;
				int done;

				_splitpath(*argv,drive,dir,NULL,NULL);		//get path
				strcpy(filename,drive);
				strcat(filename,dir);
				nptr = filename + strlen(filename);			//point at name part

				done = _dos_findfirst(*argv,0,&ffblk);

				if (done) handler_func(*argv);
			
				else while (!done) {

					strcpy(nptr,ffblk.name);	//copy name after path

					handler_func(filename);
			
					done = _dos_findnext(&ffblk);
			
				}
	
			}
			else
				handler_func(*argv);

	}

}


