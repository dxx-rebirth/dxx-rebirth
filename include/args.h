/* $Id: args.h,v 1.5 2004-05-19 21:20:06 btb Exp $ */
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
 * Prototypes for accessing arguments.
 *
 * Old Log:
 * Revision 1.1  1995/05/16  15:54:24  allender
 * Initial revision
 *
 * Revision 2.0  1995/02/27  11:33:09  john
 * New version 2.0, which has no anonymous unions, builds with
 * Watcom 10.0, and doesn't require parsing BITMAPS.TBL.
 *
 * Revision 1.4  1994/07/11  16:27:28  matt
 * Took out prototypes for netipx funcs
 *
 * Revision 1.3  1994/05/11  19:45:34  john
 * *** empty log message ***
 *
 * Revision 1.2  1994/05/09  17:02:55  john
 * Split command line parameters into arg.c and arg.h.
 * Also added /dma, /port, /irq to digi.c
 *
 * Revision 1.1  1994/05/09  16:47:49  john
 * Initial revision
 *
 *
 */


#ifndef _ARGS_H
#define _ARGS_H

extern int Num_args;
extern char *Args[];
extern int FindArg(char *s);
extern int FindResArg(char *prefix, int *sw, int *sh);
extern void InitArgs(int argc, char **argv);
extern int Inferno_verbose;

#endif
