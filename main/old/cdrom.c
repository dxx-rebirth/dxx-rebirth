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

#pragma off (unreferenced)
static char rcsid[] = "$Id: cdrom.c,v 1.1.1.1 2001-01-19 03:30:14 bradleyb Exp $";
#pragma on (unreferenced)

#include <dos.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "dpmi.h"

typedef struct {
	char unit;
	ushort dev_offset;
	ushort dev_segment;
} dev_list;

typedef struct _Dev_Hdr {
	unsigned int dev_next;
	unsigned short dev_att;
	ushort dev_stat;
	ushort dev_int;
	char dev_name[8];
	short dev_resv;
	char dev_letr;
	char dev_units;
} dev_header;

int find_descent_cd()
{
	dpmi_real_regs rregs;
		
	// Get dos memory for call...
	dev_list * buf;
	dev_header *device;
	int num_drives, i;
	unsigned cdrive, cur_drive, cdrom_drive;

	memset(&rregs,0,sizeof(dpmi_real_regs));
	rregs.eax = 0x1500;
	rregs.ebx = 0;
	dpmi_real_int386x( 0x2f, &rregs );
	if ((rregs.ebx & 0xffff) == 0) {
		return -1;			// No cdrom
	}
	num_drives = rregs.ebx;

	buf = (dev_list *)dpmi_get_temp_low_buffer( sizeof(dev_list)*26 );	
	if (buf==NULL) {
		return -2;			// Error getting memory!
	}

	memset(&rregs,0,sizeof(dpmi_real_regs));
	rregs.es = DPMI_real_segment(buf);
	rregs.ebx = DPMI_real_offset(buf);
	rregs.eax = 0x1501;
	dpmi_real_int386x( 0x2f, &rregs );
	cdrom_drive = 0;
	_dos_getdrive(&cdrive);
	for (i = 0; i < num_drives; i++) {
		device = (dev_header *)((buf[i].dev_segment<<4)+ buf[i].dev_offset);
		_dos_setdrive(device->dev_letr,&cur_drive);
		_dos_getdrive(&cur_drive);
		if (cur_drive == device->dev_letr) {
			if (!chdir("\\descent")) {
				FILE * fp;
				fp = fopen( "saturn.hog", "rb" );	
				if ( fp )	{
					cdrom_drive = device->dev_letr;
					fclose(fp);
					break;
				}
			}
		}				
	}
	_dos_setdrive(cdrive,&cur_drive);
	return cdrom_drive;
}

