/* hmp 2 midi file conversion. The conversion algorithm is based on the
   conversion algorithms found in d1x, d2x-xl and jjffe. The code is mine.
   
   Copyright 2006 Hans de Goede
   
   This file and the acompanying hmp2mid.c file are free software;
   you can redistribute them and/or modify them under the terms of the GNU
   Library General Public License as published by the Free Software Foundation;
   either version 2 of the License, or (at your option) any later version.

   These files are distributed in the hope that they will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public
   License along with these files; see the file COPYING.LIB.  If not,
   write to the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
   Boston, MA 02111-1307, USA.
*/
#ifndef __HMP2MID_H
#define __HMP2MID_H
//#include <stdio.h>
#if !(defined(__APPLE__) && defined(__MACH__))
#include <physfs.h>
#else
#include <physfs/physfs.h>
#endif

//typedef size_t (*hmp2mid_read_func_t)(void *ptr, size_t size, size_t nmemb,
//  void *stream);

/* Returns NULL on success, otherwise a c-string with an error message */
const char *hmp2mid(/*hmp2mid_read_func_t read_func, */PHYSFS_file *hmp_in, PHYSFS_file* mid_out);

#endif
