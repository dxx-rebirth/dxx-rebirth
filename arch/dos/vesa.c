#include <string.h>
#include "gr.h"
#include "grdef.h"
#include "u_dpmi.h"
#include "vesa.h"

#define SC_INDEX   0x3c4
#define CRTC_INDEX 0x3d4
#define CRTC_DATA  0x3d5

static int bankshift;
static int lastbank;

/* this comes from Allegro */
typedef struct vesa_mode_info
{
   unsigned short ModeAttributes;
   unsigned char  WinAAttributes;
   unsigned char  WinBAttributes;
   unsigned short WinGranularity;
   unsigned short WinSize;
   unsigned short WinASegment;
   unsigned short WinBSegment;
   unsigned long  WinFuncPtr;
   unsigned short BytesPerScanLine;
   unsigned short XResolution;
   unsigned short YResolution;
   unsigned char  XCharSize;
   unsigned char  YCharSize;
   unsigned char  NumberOfPlanes;
   unsigned char  BitsPerPixel;
   unsigned char  NumberOfBanks;
   unsigned char  MemoryModel;
   unsigned char  BankSize;
   unsigned char  NumberOfImagePages;
   unsigned char  Reserved_page;
   unsigned char  RedMaskSize;
   unsigned char  RedMaskPos;
   unsigned char  GreenMaskSize;
   unsigned char  GreenMaskPos;
   unsigned char  BlueMaskSize;
   unsigned char  BlueMaskPos;
   unsigned char  ReservedMaskSize;
   unsigned char  ReservedMaskPos;
   unsigned char  DirectColorModeInfo;
   unsigned long  PhysBasePtr;
   unsigned long  OffScreenMemOffset;
   unsigned short OffScreenMemSize;
   unsigned char  Reserved[206];
} __attribute__ ((packed)) vesa_mode_info;


int gr_vesa_checkmode(int mode) {
	int i;
	dpmi_real_regs rregs;
	vesa_mode_info *mode_info;

	if (!(mode_info = dpmi_get_temp_low_buffer( 1024 )))
	    return 7;
	rregs.eax = 0x4f01;
	rregs.ecx = mode;
	rregs.edi = DPMI_real_offset(mode_info);
	rregs.es = DPMI_real_segment(mode_info);
	dpmi_real_int386x( 0x10, &rregs );
	if (rregs.eax != 0x4f)
		return 5; /* no vesa */
	if (!(mode_info->ModeAttributes & 1))
		return 4; /* unsupported mode */

	bankshift = 0;
	i = mode_info->WinGranularity;
	while (i < 64) {
		i <<= 1;
		bankshift++;
	}
	if (i != 64)
		return 2; /* incompatible window granularity */
	return 0;
}

int gr_vesa_setmode(int mode) {
    int ret;
    lastbank = -1;
    if ((ret = gr_vesa_checkmode(mode)))
	return ret;
    asm volatile("int $0x10" : "=a" (ret) : "a" (0x4f02), "b" (mode));
    return (ret == 0x4f) ? 0 : 4;
}

inline void gr_vesa_setpage(int bank) {
    if (bank != lastbank)
	asm volatile("int $0x10"
	 : : "a" (0x4f05), "b" (0), "d" ((lastbank = bank) << bankshift)
	 : "%eax");
}

void gr_vesa_incpage() {
    gr_vesa_setpage(lastbank + 1);
}

void gr_vesa_setstart(int col, int row) {
    asm volatile("int $0x10"
     : : "a" (0x4f07), "b" (0), "c" (col), "d" (row) : "%eax");
}

int gr_vesa_setlogical(int line_width) {
    int ret;
    asm volatile("int $0x10"
     : "=c" (ret) : "a" (0x4f07), "b" (0), "c" (line_width) : "%eax");
    return ret;
}

void gr_vesa_scanline(int x1, int x2, int y, unsigned char color) {
    int addr = (y * gr_var_bwidth) + x1;
    int left = x2 - x1 + 1;
    gr_vesa_setpage(addr >> 16);
    addr &= 0xffff;
    while (left) {
	if (left > (65536 - addr)) {
	    left -= (65536 - addr);
	    memset(gr_video_memory + addr, color, 65536 - addr);
	    addr = 0;
	    gr_vesa_incpage();
	} else {
	    memset(gr_video_memory + addr, color, left);
	    left = 0;
	}
    }
}

void gr_vesa_pixel(unsigned char color, unsigned int addr) {
    gr_vesa_setpage(addr >> 16);
    gr_video_memory[addr & 0xffff] = color;
}
