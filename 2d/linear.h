#ifndef _LINEAR_H
#define _LINEAR_H

#ifndef NO_ASM
#ifdef __WATCOMC__
extern unsigned int gr_var_color;
extern unsigned int gr_var_bwidth;
extern unsigned char * gr_var_bitmap;

void gr_linear_stosd( void * source, unsigned char color, unsigned int nbytes);
void gr_linear_line( int x0, int y0, int x1, int y1);
void gr_update_buffer( void * sbuf1, void * sbuf2, void * dbuf, int size );

// This code aligns edi so that the destination is aligned to a dword boundry before rep movsd
void gr_linear_movsd(ubyte * src, ubyte * dest, unsigned int num_pixels );
#pragma aux gr_linear_movsd parm [esi] [edi] [ecx] modify exact [ecx esi edi eax ebx] = \
" cld "                 \
" mov       ebx, ecx    "   \
" mov       eax, edi"   \
" and       eax, 011b"  \
" jz        d_aligned"  \
" mov       ecx, 4"     \
" sub       ecx, eax"   \
" sub       ebx, ecx"   \
" rep       movsb"      \
" d_aligned: "          \
" mov       ecx, ebx"   \
" shr       ecx, 2"     \
" rep   movsd"      \
" mov       ecx, ebx"   \
" and   ecx, 11b"   \
" rep   movsb";

void gr_linear_rep_movsdm(void * src, void * dest, unsigned int num_pixels );
#pragma aux gr_linear_rep_movsdm parm [esi] [edi] [ecx] modify exact [ecx esi edi eax] = \
"nextpixel:"                    \
    "mov    al,[esi]"           \
    "inc    esi"                    \
    "cmp    al, 255"                \
    "je skip_it"                \
    "mov    [edi], al"          \
"skip_it:"                      \
    "inc    edi"                    \
    "dec    ecx"                    \
    "jne    nextpixel";

void gr_linear_rep_movsdm_faded(void * src, void * dest, unsigned int num_pixels, ubyte fade_value );
#pragma aux gr_linear_rep_movsdm_faded parm [esi] [edi] [ecx] [ebx] modify exact [ecx esi edi eax ebx] = \
"  xor eax, eax"    \
"  mov ah, bl"  \
"nextpixel:"                    \
    "mov    al,[esi]"           \
    "inc    esi"                    \
    "cmp    al, 255"                \
    "je skip_it"                \
    "movb  al, gr_fade_table[eax]"   \
    "movb  [edi], al"          \
"skip_it:"                      \
    "inc    edi"                    \
    "dec    ecx"                    \
    "jne    nextpixel";


void gr_linear_rep_movsd_2x(ubyte * src, ubyte * dest, int num_dest_pixels );
#pragma aux gr_linear_rep_movsd_2x parm [esi] [edi] [ecx] modify exact [ecx esi edi eax ebx] = \
    "shr    ecx, 1"             \
    "jnc    nextpixel"          \
    "mov    al, [esi]"          \
    "mov    [edi], al"          \
    "inc    esi"                    \
    "inc    edi"                    \
    "cmp    ecx, 0"             \
    "je done"                   \
"nextpixel:"                    \
    "mov    al,[esi]"           \
    "mov    ah, al"             \
    "mov    [edi], ax"          \
    "inc    esi"                    \
    "inc    edi"                    \
    "inc    edi"                    \
    "dec    ecx"                    \
    "jne    nextpixel"          \
"done:"



void modex_copy_column(ubyte * src, ubyte * dest, int num_pixels, int src_rowsize, int dest_rowsize );
#pragma aux modex_copy_column parm [esi] [edi] [ecx] [ebx] [edx] modify exact [ecx esi edi] = \
"nextpixel:"                            \
    "mov    al,[esi]"           \
    "add    esi, ebx"   \
    "mov    [edi], al"  \
    "add    edi, edx"   \
    "dec    ecx"            \
    "jne    nextpixel"  

void modex_copy_column_m(ubyte * src, ubyte * dest, int num_pixels, int src_rowsize, int dest_rowsize );
#pragma aux modex_copy_column_m parm [esi] [edi] [ecx] [ebx] [edx] modify exact [ecx esi edi] = \
"nextpixel:"                            \
    "mov    al,[esi]"           \
    "add    esi, ebx"   \
    "cmp    al, 255"        \
    "je skip_itx"       \
    "mov    [edi], al"  \
"skip_itx:"             \
    "add    edi, edx"   \
    "dec    ecx"            \
    "jne    nextpixel"  

void modex_copy_scanline( ubyte * src, ubyte * dest, int npixels );
#pragma aux modex_copy_scanline parm [esi] [edi] [ecx] modify exact [ecx esi edi eax ebx edx] = \
"       mov ebx, ecx                "   \
"       and ebx, 11b                "   \
"       shr ecx, 2              "   \
"       cmp ecx, 0              "   \
"       je      no2group                "   \
"next4pixels:                       "   \
"       mov al, [esi+8]         "   \
"       mov ah, [esi+12]        "   \
"       shl eax, 16             "   \
"       mov al, [esi]           "   \
"       mov ah, [esi+4]         "   \
"       mov [edi], eax          "   \
"       add esi, 16             "   \
"       add edi, 4              "   \
"       dec ecx                 "   \
"       jne next4pixels         "   \
"no2group:                          "   \
"       cmp ebx, 0              "   \
"       je      done2                   "   \
"finishend:                         "   \
"       mov al, [esi]           "   \
"       add esi, 4              "   \
"       mov [edi], al           "   \
"       inc edi                 "   \
"       dec ebx                 "   \
"       jne finishend           "   \
"done2:                             ";

void modex_copy_scanline_2x( ubyte * src, ubyte * dest, int npixels );
#pragma aux modex_copy_scanline_2x parm [esi] [edi] [ecx] modify exact [ecx esi edi eax ebx edx] = \
"       mov ebx, ecx                "   \
"       and ebx, 11b                "   \
"       shr ecx, 2              "   \
"       cmp ecx, 0              "   \
"       je      no2group                "   \
"next4pixels:                       "   \
"       mov al, [esi+4]         "   \
"       mov ah, [esi+6]         "   \
"       shl eax, 16             "   \
"       mov al, [esi]           "   \
"       mov ah, [esi+2]         "   \
"       mov [edi], eax          "   \
"       add esi, 8              "   \
"       add edi, 4              "   \
"       dec ecx                 "   \
"       jne next4pixels         "   \
"no2group:                          "   \
"       cmp ebx, 0              "   \
"       je      done2                   "   \
"finishend:                         "   \
"       mov al, [esi]           "   \
"       add esi, 2              "   \
"       mov [edi], al           "   \
"       inc edi                 "   \
"       dec ebx                 "   \
"       jne finishend           "   \
"done2:                             ";

#elif defined __GNUC__
// This code aligns edi so that the destination is aligned to a dword boundry before rep movsd
inline void gr_linear_movsd(ubyte * src, ubyte * dest, unsigned int num_pixels ) {
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

static inline void gr_linear_rep_movsdm(void * src, void * dest, unsigned int num_pixels ) {
	int dummy[3];
 __asm__ __volatile__ (
"0: ;"
" movb  (%%esi), %%al;"
" incl  %%esi;"
" cmpb  $255, %%al;"
" je    1f;"
" movb  %%al,(%%edi);"
"1: ;"
" incl  %%edi;"
" decl  %%ecx;"
" jne   0b"
 : "=S" (dummy[0]), "=D" (dummy[1]), "=c" (dummy[2])
 : "0" (src), "1" (dest), "2" (num_pixels)
 :	"%eax");
}

/* #pragma aux gr_linear_rep_movsdm_faded parm [esi] [edi] [ecx] [ebx] modify exact [ecx esi edi eax ebx] */
static inline void gr_linear_rep_movsdm_faded(void * src, void * dest, unsigned int num_pixels, ubyte fade_value ) {
	int dummy[4];
 __asm__ __volatile__ (
"  xorl   %%eax, %%eax;"
"  movb   %%bl, %%ah;"
"0:;"
"  movb   (%%esi), %%al;"
"  incl   %%esi;"
"  cmpb   $255, %%al;"
"  je 1f;"
#ifdef __LINUX__
"  movb   gr_fade_table(%%eax), %%al;"
#else
"  movb   _gr_fade_table(%%eax), %%al;"
#endif
"  movb   %%al, (%%edi);"
"1:"
"  incl   %%edi;"
"  decl   %%ecx;"
"  jne    0b"
 : "=S" (dummy[0]), "=D" (dummy[1]), "=c" (dummy[2]), "=b" (dummy[3])
 : "0" (src), "1" (dest), "2" (num_pixels), "3" (fade_value)
 :	"%eax");
}

inline void gr_linear_rep_movsd_2x(ubyte * src, ubyte * dest, unsigned int num_dest_pixels ) {
/* #pragma aux gr_linear_rep_movsd_2x parm [esi] [edi] [ecx] modify exact [ecx esi edi eax ebx] */
	int dummy[3];
 __asm__ __volatile__ (
    "shrl   $1, %%ecx;"
    "jnc    0f;"
    "movb   (%%esi), %%al;"
    "movb   %%al, (%%edi);"
    "incl   %%esi;"
    "incl   %%edi;"
    "cmpl   $0, %%ecx;"
    "je 1f;"
"0: ;"
    "movb   (%%esi), %%al;"
    "movb   %%al, %%ah;"
    "movw   %%ax, (%%edi);"
    "incl   %%esi;"
    "incl   %%edi;"
    "incl   %%edi;"
    "decl   %%ecx;"
    "jne    0b;"
"1:"
 : "=S" (dummy[0]), "=D" (dummy[1]), "=c" (dummy[2])
 : "0" (src), "1" (dest), "2" (num_dest_pixels)
 :      "%eax");
}

static inline void modex_copy_column(ubyte * src, ubyte * dest, int num_pixels, int src_rowsize, int dest_rowsize ) {
/*#pragma aux modex_copy_column parm [esi] [edi] [ecx] [ebx] [edx] modify exact [ecx esi edi] = */
 __asm__ __volatile__ (
"0: ;"
    "movb   (%%esi), %%al;"
    "addl   %%ebx, %%esi;"
    "movb   %%al, (%%edi);"
    "addl   %%edx, %%edi;"
    "decl   %%ecx;"
    "jne    0b"
 : : "S" (src), "D" (dest), "c" (num_pixels), "b" (src_rowsize), "d" (dest_rowsize)
 :	"%eax", "%ecx", "%esi", "%edi");
}

static inline void modex_copy_column_m(ubyte * src, ubyte * dest, int num_pixels, int src_rowsize, int dest_rowsize ) {
/* #pragma aux modex_copy_column_m parm [esi] [edi] [ecx] [ebx] [edx] modify exact [ecx esi edi] = */
 int dummy[3];
 __asm__ __volatile__ (
"0: ;"
    "movb    (%%esi), %%al;"
    "addl    %%ebx, %%esi;"
    "cmpb    $255, %%al;"
    "je 1f;"
    "movb   %%al, (%%edi);"
"1: ;"
    "addl   %%edx, %%edi;"
    "decl   %%ecx;"
    "jne    0b"
 : "=c" (dummy[0]), "=S" (dummy[1]), "=D" (dummy[2])
 : "1" (src), "2" (dest), "0" (num_pixels), "b" (src_rowsize), "d" (dest_rowsize)
 :      "%eax" );
}

static inline void modex_copy_scanline( ubyte * src, ubyte * dest, int npixels ) {
/* #pragma aux modex_copy_scanline parm [esi] [edi] [ecx] modify exact [ecx esi edi eax ebx edx] */
int dummy[3];
 __asm__ __volatile__ (
"       movl %%ecx, %%ebx;"
"       andl $3, %%ebx;"
"       shrl $2, %%ecx;"
"       cmpl $0, %%ecx;"
"       je   1f;"
"0: ;"
"       movb 8(%%esi), %%al;"
"       movb 12(%%esi), %%ah;"
"       shll $16, %%eax;"
"       movb (%%esi), %%al;"
"       movb 4(%%esi), %%ah;"
"       movl %%eax, (%%edi);"
"       addl $16, %%esi;"
"       addl $4, %%edi;"
"       decl %%ecx;"
"       jne 0b;"
"1: ;"
"       cmpl $0, %%ebx;"
"       je      3f;"
"2: ;"
"       movb (%%esi), %%al;"
"       addl $4, %%esi;"
"       movb %%al, (%%edi);"
"       incl %%edi;"
"       decl %%ebx;"
"       jne 2b;"
"3:"
 : "=c" (dummy[0]), "=S" (dummy[1]), "=D" (dummy[2])
 : "1" (src), "2" (dest), "0" (npixels)
 :      "%eax", "%ebx", "%edx" );
}

static inline void modex_copy_scanline_2x( ubyte * src, ubyte * dest, int npixels ) {
/* #pragma aux modex_copy_scanline_2x parm [esi] [edi] [ecx] modify exact [ecx esi edi eax ebx edx] = */
int dummy[3];
 __asm__ __volatile__ (
"       movl %%ecx, %%ebx;"
"       andl $3, %%ebx;"
"       shrl $2, %%ecx;"
"       cmpl $0, %%ecx;"
"       je 1f;"
"0: ;"
"       movb 4(%%esi), %%al;"
"       movb 6(%%esi), %%ah;"
"       shll $16, %%eax;"
"       movb (%%esi), %%al;"
"       movb 2(%%esi), %%ah;"
"       movl %%eax, (%%edi);"
"       addl $8, %%esi;"
"       addl $4, %%edi;"
"       decl %%ecx;"
"       jne 0b;"
"1: ;"
"       cmp $0, %%ebx;"
"       je  3f;"
"2:"
"       movb (%%esi),%%al;"
"       addl $2, %%esi;"
"       movb %%al, (%%edi);"
"       incl %%edi;"
"       decl %%ebx;"
"       jne 2b;"
"3:"
 : "=c" (dummy[0]), "=S" (dummy[1]), "=D" (dummy[2])
 : "1" (src), "2" (dest), "0" (npixels)
 :      "%eax", "%ebx", "%edx" );
}
#elif defined _MSC_VER

extern unsigned int gr_var_color;
extern unsigned int gr_var_bwidth;
extern unsigned char * gr_var_bitmap;

void gr_linear_stosd( ubyte * source, unsigned char color, unsigned int nbytes);
void gr_linear_line( int x0, int y0, int x1, int y1);
void gr_update_buffer( void * sbuf1, void * sbuf2, void * dbuf, int size );

// This code aligns edi so that the destination is aligned to a dword boundry before rep movsd
__inline void gr_linear_movsd(ubyte * src, ubyte * dest, unsigned int num_pixels )
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

__inline void gr_linear_rep_movsdm(void * src, void * dest, unsigned int num_pixels )
{
 __asm {
  nextpixel:
  mov esi, [src]
  mov edi, [dest]
  mov ecx, [num_pixels]
  mov al,  [esi]
  inc esi
  cmp al,  255
  je skip_it
  mov [edi], al
  skip_it:
  inc edi
  dec ecx
  jne nextpixel
 }
}

__inline void gr_linear_rep_movsdm_faded(void * src, void * dest, unsigned int num_pixels, ubyte fade_value )
{
 __asm {
  mov esi, [src]
  mov edi, [dest]
  mov ecx, [num_pixels]
  movzx ebx, byte ptr [fade_value]
  xor eax, eax
  mov ah, bl
  nextpixel:
  mov al, [esi]
  inc esi
  cmp al, 255
  je skip_it
  mov al, gr_fade_table[eax]
  mov [edi], al
  skip_it:
  inc edi
  dec ecx
  jne nextpixel
 }
}

__inline void gr_linear_rep_movsd_2x(ubyte * src, ubyte * dest, uint num_dest_pixels )
{
 __asm {
  mov esi, [src]
  mov edi, [dest]
  mov ecx, [num_dest_pixels]
  shr ecx, 1
  jnc nextpixel
  mov al, [esi]
  mov [edi], al
  inc esi
  inc edi
  cmp ecx, 0
  je done
nextpixel:
  mov al, [esi]
  mov ah, al
  mov [edi], ax
  inc esi
  inc edi
  inc edi
  dec ecx
  jne nextpixel
done:
 }
}

#else
#define NO_ASM 1 // We really do want no assembler...

#endif
#endif // NO_ASM

#endif
