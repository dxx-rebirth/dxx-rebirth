#include <fcntl.h>
#include <sys/mman.h>
#include <linux/fb.h>
#include <unistd.h>
#include <stropts.h>
#include <stdio.h>
#define SYS_CLK_FREQ 7372800

volatile unsigned long *memregs32;
volatile unsigned short *memregs16;
int memfd;

unsigned char initphys (void)
{
	memfd = open("/dev/mem", O_RDWR);
	if (memfd == -1)
	{
		printf ("Open failed for /dev/mem\n");
		return 0;
	}
	
	memregs32 = (unsigned long*) mmap(0, 0x10000, PROT_READ|PROT_WRITE, MAP_SHARED, memfd, 0xc0000000);
	
	if (memregs32 == (unsigned long *)0xFFFFFFFF) return 0;
	memregs16 = (unsigned short *)memregs32;
	
	return 1;
}

void SetClock(unsigned int MHZ)
{
	unsigned int v;
	unsigned int mdiv,pdiv = 3,scale = 0;
	MHZ*=1000000;
	mdiv=(MHZ*pdiv)/SYS_CLK_FREQ;

	if (!initphys()) return;
	
	mdiv=((mdiv-8)<<8) & 0xff00;
	pdiv=((pdiv-2)<<2) & 0xfc;
	scale&=3;
	v = mdiv | pdiv | scale;
	
	unsigned int l = memregs32[0x808>>2]; // Get interupt flags
	memregs32[0x808>>2] = 0xFF8FFFE7;     //Turn off interrupts
	memregs16[0x910>>1] = v;              //Set frequentie
	while(memregs16[0x0902>>1] & 1);      //Wait for the frequentie to be ajused
	memregs32[0x808>>2] = l;              //Turn on interrupts
}
