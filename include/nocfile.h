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
 * $Source: /cvsroot/dxx-rebirth/d1x-rebirth/include/nocfile.h,v $
 * $Revision: 1.1.1.1 $
 * $Author: zicodxx $
 * $Date: 2006/03/17 19:46:39 $
 * 
 * Prototypes to *NOT* use cfile, but use cfile calling parameters.
 * For debugging.
 * 
 * $Log: nocfile.h,v $
 * Revision 1.1.1.1  2006/03/17 19:46:39  zicodxx
 * initial import
 *
 * Revision 1.1.1.1  1999/06/14 22:02:18  donut
 * Import of d1x 1.37 source.
 *
 * Revision 1.4  1994/12/09  17:53:52  john
 * Added error checking to number of hogfiles..
 * 
 * Revision 1.3  1994/02/17  17:36:34  john
 * Added CF_READ_MODE and CF_WRITE_MODE constants.
 * 
 * Revision 1.2  1994/02/15  12:52:08  john
 * Crappy inbetween version
 * 
 * Revision 1.1  1994/02/15  09:51:00  john
 * Initial revision
 * 
 * 
 */



#ifndef _NOCFILE_H
#define _NOCFILE_H

#include <stdio.h>
#ifndef __LINUX__
#include <io.h>
#endif

#define CFILE FILE

#define cfopen(file,mode) fopen(file,mode)
#define cfilelength(f) filelength( fileno( f ))
#define cfwrite(buf,elsize,nelem,fp) fwrite(buf,elsize,nelem,fp)
#define cfread(buf,elsize,nelem,fp ) fread(buf,elsize,nelem,fp )
#define cfclose( cfile ) fclose( cfile )
#define cfputc(c,fp) fputc(c,fp)
#define cfgetc(fp) fgetc(fp)
#define cfseek(fp,offset,where ) fseek(fp,offset,where )
#define cftell(fp) ftell(fp)
#define cfgets(buf,n,fp) fgets(buf,n,fp)

#define CF_READ_MODE "rb"
#define CF_WRITE_MODE "wb"

#endif
 
