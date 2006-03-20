#ifndef _BITMAP_H
#define _BITMAP_H

#ifndef NO_ASM
#ifdef __WATCOMC__
void decode_data_asm(ubyte *data, int num_pixels, ubyte * colormap, int * count );
#pragma aux decode_data_asm parm [esi] [ecx] [edi] [ebx] modify exact [esi edi eax ebx ecx] = \
"again_ddn:"                            \
    "xor    eax,eax"                \
    "mov    al,[esi]"           \
    "inc    dword ptr [ebx+eax*4]"      \
    "mov    al,[edi+eax]"       \
    "mov    [esi],al"           \
    "inc    esi"                    \
    "dec    ecx"                    \
    "jne    again_ddn"
#elif defined __GNUC__
static inline void decode_data_asm(ubyte *data, int num_pixels, ubyte * colormap, int * count ) {
	int dummy[4];
   __asm__ __volatile__ (
    "xorl   %%eax,%%eax;"
"0:;"
    "movb   (%%esi), %%al;"
    "incl   (%%ebx, %%eax, 4);"
    "movb   (%%edi, %%eax), %%al;"
    "movb   %%al, (%%esi);"
    "incl   %%esi;"
    "decl   %%ecx;"
    "jne    0b"
    : "=S" (dummy[0]), "=c" (dummy[1]), "=D" (dummy[2]), "=b" (dummy[3])
	: "0" (data), "1" (num_pixels), "2" (colormap), "3" (count)
	: "%eax");
}
#elif defined _MSC_VER
__inline void decode_data_asm(ubyte *data, int num_pixels, ubyte * colormap, int * count )
{
  __asm {
	mov esi,[data]
	mov ecx,[num_pixels]
	mov edi,[colormap]
	mov ebx,[count]
again_ddn:
	xor eax,eax
	mov al,[esi]
	inc dword ptr [ebx+eax*4]
	mov al,[edi+eax]
	mov [esi],al
	inc esi
	dec ecx
	jne again_ddn
  }
}
#else
#define NO_ASM 1 // We really do want no assembler...
#endif
#endif

#ifdef NO_ASM
static void decode_data_asm(ubyte *data, int num_pixels, ubyte *colormap, int *count)
{
	int i;
	
	for (i = 0; i < num_pixels; i++) {
		count[*data]++;
		*data = colormap[*data];
		data++;
	}
}
#endif
#endif
