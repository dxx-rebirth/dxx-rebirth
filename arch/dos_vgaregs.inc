;THE COMPUTER CODE CONTAINED HEREIN IS THE SOLE PROPERTY OF PARALLAX
;SOFTWARE CORPORATION ("PARALLAX").  PARALLAX, IN DISTRIBUTING THE CODE TO
;END-USERS, AND SUBJECT TO ALL OF THE TERMS AND CONDITIONS HEREIN, GRANTS A
;ROYALTY-FREE, PERPETUAL LICENSE TO SUCH END-USERS FOR USE BY SUCH END-USERS
;IN USING, DISPLAYING,  AND CREATING DERIVATIVE WORKS THEREOF, SO LONG AS
;SUCH USE, DISPLAY OR CREATION IS FOR NON-COMMERCIAL, ROYALTY OR REVENUE
;FREE PURPOSES.  IN NO EVENT SHALL THE END-USER USE THE COMPUTER CODE
;CONTAINED HEREIN FOR REVENUE-BEARING PURPOSES.  THE END-USER UNDERSTANDS
;AND AGREES TO THE TERMS HEREIN AND ACCEPTS THE SAME BY USE OF THIS FILE.  
;COPYRIGHT 1993-1998 PARALLAX SOFTWARE CORPORATION.  ALL RIGHTS RESERVED.
;
; $Source: /cvs/cvsroot/d2x/arch/dos_vgaregs.inc,v $
; $Revision: 1.2 $
; $Author: bradleyb $
; $Date: 2001-01-29 13:35:08 $
;
; Readable descriptions of VGA ports.
;
; $Log: not supported by cvs2svn $
; Revision 1.1.1.1  2001/01/19 03:30:15  bradleyb
; Import of d2x-0.0.8
;
; Revision 1.1.1.1  1999/06/14 21:58:45  donut
; Import of d1x 1.37 source.
;
; Revision 1.2  1993/10/15  16:22:45  john
; *** empty log message ***
; 
; Revision 1.1  1993/09/08  11:41:00  john
; Initial revision
; 
;
;


%define MISC_OUTPUT	03c2h		  ;Miscellaneous Output register
%define MAP_MASK	02h		  ;index in SC of Map Mask register
%define READ_MAP	04h		  ;index in GC of the Read Map register
%define BIT_MASK	08h		  ;index in GC of Bit Mask register

%define SC_INDEX	3c4h		 ;Index register for sequencer ctrl.
%define SC_MAP_MASK	2		 ;Number of map mask register

%define SC_INDEX        3c4h             ;Index register for sequencer ctrl.
%define SC_MAP_MASK	2		 ;Number of map mask register
%define SC_MEM_MODE	4		 ;Number of memory mode register

%define GC_INDEX	3ceh		 ;Index register for graphics ctrl.
%define GC_READ_MAP	4		 ;Number of read map register
%define GC_GRAPH_MODE	5		 ;Number of graphics mode register
%define GC_MISCELL	6		 ;Number of miscellaneous register

%define CRTC_INDEX	3d4h		 ;Index register for CRT controller
%define CC_MAX_SCAN	9		 ;Number of maximum scan line reg.
%define CC_START_HI	0Ch		 ;Number of start address high register
%define CC_START_LO	0Dh		 ;Number of start address low register
%define CC_UNDERLINE	14h		 ;Number of underline register
%define CC_MODE_CTRL	17h		 ;Number of mode control register
%define CRTC_OFFSET	13h	; CRTC offset register index

%define DAC_WRITE_ADR	3C8h		 ;DAC write address
%define DAC_READ_ADR	3C7h		 ;DAC read address
%define DAC_DATA	3C9h		 ;DAC data register

%define VERT_RESCAN	3DAh		 ;Input status register #1
