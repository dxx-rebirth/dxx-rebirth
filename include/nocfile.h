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



#ifndef _CFILE_H
#define _CFILE_H

#include <stdio.h>
//#include <io.h>

#include "physfsx.h"	// cfile.h includes this, so nocfile.h does too

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
