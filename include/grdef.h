/* $Id: grdef.h,v 1.5 2002-09-04 22:21:25 btb Exp $ */
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
 * Internal definitions for graphics lib.
 *
 * Old Log:
 * Revision 1.5  1995/09/14  15:36:33  allender
 * added stuff for 68k version
 *
 * Revision 1.4  1995/07/05  16:10:57  allender
 * gr_linear_movsd prototype changes
 *
 * Revision 1.3  1995/04/19  14:39:28  allender
 * changed function prototype
 *
 * Revision 1.2  1995/04/18  09:49:53  allender
 * *** empty log message ***
 *
 * Revision 1.1  1995/03/09  09:04:56  allender
 * Initial revision
 *
 *
 * --- PC RCS information ---
 * Revision 1.8  1994/05/06  12:50:09  john
 * Added supertransparency; neatend things up; took out warnings.
 *
 * Revision 1.7  1994/01/25  11:40:29  john
 * Added gr_check_mode function.
 *
 * Revision 1.6  1993/10/15  16:22:53  john
 * y
 *
 * Revision 1.5  1993/09/29  17:31:00  john
 * added gr_vesa_pixel
 *
 * Revision 1.4  1993/09/29  16:14:43  john
 * added global canvas descriptors.
 *
 * Revision 1.3  1993/09/08  17:38:02  john
 * Looking for errors
 *
 * Revision 1.2  1993/09/08  15:54:29  john
 * *** empty log message ***
 *
 * Revision 1.1  1993/09/08  11:37:57  john
 * Initial revision
 *
 */

void gr_init_bitmap_alloc( grs_bitmap *bm, int mode, int x, int y, int w, int h, int bytesperline);
void gr_init_sub_bitmap (grs_bitmap *bm, grs_bitmap *bmParent, int x, int y, int w, int h );
void gr_init_bitmap( grs_bitmap *bm, int mode, int x, int y, int w, int h, int bytesperline, unsigned char * data );

extern void gr_set_buffer(int w, int h, int r, int (*buffer_func)());

extern void gr_pal_setblock( int start, int n, unsigned char * palette );
extern void gr_pal_getblock( int start, int n, unsigned char * palette );
extern void gr_pal_setone( int index, unsigned char red, unsigned char green, unsigned char blue );

void gr_linear_movsb( ubyte * source, ubyte * dest, int nbytes);
void gr_linear_movsw( ubyte * source, ubyte * dest, int nbytes);
void gr_linear_stosd( ubyte * dest, unsigned char color, unsigned int nbytes);

extern unsigned int gr_var_color;
extern unsigned int gr_var_bwidth;
extern unsigned char * gr_var_bitmap;

static void gr_linear_movsd( ubyte * source, ubyte * dest, int nbytes);

#ifndef NO_ASM
// This code aligns edi so that the destination is aligned to a dword boundry before rep movsd

#ifdef __WATCOMC__

#pragma aux gr_linear_movsd parm [esi] [edi] [ecx] modify exact [ecx esi edi eax ebx] = \
" cld "					\
" mov		ebx, ecx	"	\
" mov		eax, edi"	\
" and		eax, 011b"	\
" jz		d_aligned"	\
" mov		ecx, 4"		\
" sub		ecx, eax"	\
" sub		ebx, ecx"	\
" rep		movsb"		\
" d_aligned: "			\
" mov		ecx, ebx"	\
" shr		ecx, 2"		\
" rep 	movsd"		\
" mov		ecx, ebx"	\
" and 	ecx, 11b"	\
" rep 	movsb";

#elif defined __GNUC__

static inline void gr_linear_movsd(ubyte *src, ubyte *dest, int num_pixels) {
	int dummy[3];
 __asm__ __volatile__ (
" cld;"
" movl      %%ecx, %%ebx;"
" movl      %%edi, %%eax;"
" andl      $3, %%eax;"
" jz        0f;"
" movl      $4, %%ecx;"
" subl      %%eax,%%ecx;"
" subl      %%ecx,%%ebx;"
" rep;      movsb;"
"0: ;"
" movl      %%ebx, %%ecx;"
" shrl      $2, %%ecx;"
" rep;      movsl;"
" movl      %%ebx, %%ecx;"
" andl      $3, %%ecx;"
" rep;      movsb"
 : "=S" (dummy[0]), "=D" (dummy[1]), "=c" (dummy[2])
 : "0" (src), "1" (dest), "2" (num_pixels)
 :	"%eax", "%ebx");
}

#elif defined _MSC_VER

__inline void gr_linear_movsd(ubyte *src, ubyte *dest, int num_pixels)
{
 __asm {
   mov esi, [src]
   mov edi, [dest]
   mov ecx, [num_pixels]
   cld
   mov ebx, ecx
   mov eax, edi
   and eax, 011b
   jz d_aligned
   mov ecx, 4
   sub ecx, eax
   sub ebx, ecx
   rep movsb
d_aligned:
   mov ecx, ebx
   shr ecx, 2
   rep movsd
   mov ecx, ebx
   and ecx, 11b
   rep movsb
 }
}

#endif

#endif 		// ifdef NO_ASM

void gr_linear_line( int x0, int y0, int x1, int y1);

extern unsigned int Table8to32[256];

#ifdef __MSDOS__
extern unsigned char * gr_video_memory;
#endif


#define MINX    0
#define MINY    0
#define MAXX    (GWIDTH-1)
#define MAXY    (GHEIGHT-1)
#define TYPE    grd_curcanv->cv_bitmap.bm_type
#define DATA    grd_curcanv->cv_bitmap.bm_data
#define XOFFSET grd_curcanv->cv_bitmap.bm_x
#define YOFFSET grd_curcanv->cv_bitmap.bm_y
#define ROWSIZE grd_curcanv->cv_bitmap.bm_rowsize
#define COLOR   grd_curcanv->cv_color

void order( int *x1, int *x2 );
