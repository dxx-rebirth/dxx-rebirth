extern char scale_trans_color;
extern int scale_error_term;
extern int scale_initial_pixel_count;
extern int scale_adj_up;
extern int scale_adj_down;
extern int scale_final_pixel_count;
extern int scale_ydelta_minus_1;
extern int scale_whole_step;
extern ubyte * scale_source_ptr;
extern ubyte * scale_dest_ptr;
extern void rls_stretch_scanline_asm();
extern void scale_do_cc_scanline();
extern void rls_do_cc_setup_asm();

#ifdef __WATCOMC__
void rep_stosb(char *ScreenPtr, int RunLength, int Color);
#pragma aux rep_stosb = \
"               rep stosb"  \
parm [EDI] [ECX] [EAX]\
modify [];

// esi, edi = source, dest
// ecx = width
// ebx = u
// edx = du

void scale_row_asm_transparent( ubyte * sbits, ubyte * dbits, int width, fix u, fix du );
#pragma aux scale_row_asm_transparent parm [esi] [edi] [ecx] [ebx] [edx] modify exact [edi eax ebx ecx] = \
"newpixel:  mov eax, ebx            " \
"               shr eax, 16         " \
"               mov al, [esi+eax]   " \
"               cmp al, 255         " \
"               je      skip_it         " \
"               mov [edi], al       " \
"skip_it:   add ebx, edx            " \
"               inc edi             " \
"               dec ecx             " \
"               jne newpixel            "

void scale_row_asm( ubyte * sbits, ubyte * dbits, int width, fix u, fix du );
#pragma aux scale_row_asm parm [esi] [edi] [ecx] [ebx] [edx] modify exact [edi eax ebx ecx] = \
"newpixel1: mov eax, ebx            " \
"               shr eax, 16         " \
"               mov al, [esi+eax]   " \
"               add ebx, edx            " \
"               mov [edi], al       " \
"               inc edi             " \
"               dec ecx             " \
"               jne newpixel1       "


void rep_movsb( ubyte * sbits, ubyte * dbits, int width );
#pragma aux rep_movsb parm [esi] [edi] [ecx] modify exact [esi edi ecx] = \
"rep movsb"

#else
static inline void rep_stosb(char *ScreenPtr, int RunLength, int Color) {
   int dummy[2];
   __asm__ __volatile__ ("cld; rep; stosb"
    : "=c" (dummy[0]), "=D" (dummy[1]) : "1" (ScreenPtr), "0" (RunLength), "a" (Color) );
}
static inline void scale_row_asm_transparent( ubyte * sbits, ubyte * dbits, int width, fix u, fix du ) {
   int dummy[3];
   __asm__ __volatile__ (
"0:           movl %%ebx, %%eax;"
"             shrl $16, %%eax;"
"             movb (%%esi, %%eax), %%al;"
"             cmpb $255, %%al;"
"             je  1f;"
"             movb %%al, (%%edi);"
"1:           addl %%edx, %%ebx;"
"             incl %%edi;"
"             decl %%ecx;"
"             jne 0b"
 : "=c" (dummy[0]), "=b" (dummy[1]), "=D" (dummy[2])
 : "S" (sbits), "2" (dbits), "0" (width), "1" (u), "d" (du)
 : "%eax");
}

static inline void scale_row_asm( ubyte * sbits, ubyte * dbits, int width, fix u, fix du ) {
     __asm__ __volatile__ (
"0:         movl %%ebx,%%eax;"
"           shrl $16, %%eax;"
"           movb (%%esi, %%eax), %%al;"
"           addl %%edx, %%ebx;"
"           movb %%al, (%%edi);"
"           incl %%edi;"
"           decl %%ecx;"
"           jne 0b"
	: : "S" (sbits), "D" (dbits), "c" (width), "b" (u), "d" (du)
	: "%eax", "%ebx", "%ecx", "%edi");
}

static inline void rep_movsb( ubyte * sbits, ubyte * dbits, int width ) {
   __asm__ __volatile__ ("cld; rep; movsb"
    : : "S" (sbits), "D" (dbits), "c" (width) : "%ecx", "%esi", "%edi");
}
#endif
