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
; $Source: /cvs/cvsroot/d2x/arch/dos_tweak.inc,v $
; $Revision: 1.1.1.1 $
; $Author: bradleyb $
; $Date: 2001-01-19 03:33:51 $
;
; Parameters used by modex.asm to set various modex resolutions.
;
; $Log: not supported by cvs2svn $
; Revision 1.1.1.1  1999/06/14 21:58:45  donut
; Import of d1x 1.37 source.
;
; Revision 1.3  1993/11/16  11:28:09  john
; *** empty log message ***
; 
; Revision 1.2  1993/10/15  16:23:18  john
; y
; 
; Revision 1.1  1993/09/08  11:41:09  john
; Initial revision
; 
;
;


; Mode X CRTC register tweaks for various resolutions

X320Y200    db      00      ; 0e3h    ; dot clock
	    db	    02	    ; Number of CRTC Registers to update
	    dw	    00014h  ; turn off dword mode
	    dw	    0e317h  ; turn on byte mode
	    dw	    320     ; width
	    dw	    200     ; height

X320Y240    db      0e3h    ; dot clock
	    db	    10	    ; Number of CRTC Registers to update
	    dw	    00d06h  ; vertical total
	    dw	    03e07h  ; overflow (bit 8 of vertical counts)
	    dw	    04109h  ; cell height (2 to double-scan)
	    dw	    0ea10h  ; v sync start
	    dw	    0ac11h  ; v sync end and protect cr0-cr7
	    dw	    0df12h  ; vertical displayed
	    dw	    00014h  ; turn off dword mode
	    dw	    0e715h  ; v blank start
	    dw	    00616h  ; v blank end
	    dw	    0e317h  ; turn on byte mode
	    dw	    320     ; width
	    dw	    240     ; height

X360Y200    db      0e7h    ; dot clock
	    db	    08	    ; Number of CRTC Registers to update
	    dw	    06b00h  ; horz total
	    dw	    05901h  ; horz displayed
	    dw	    05a02h  ; start horz blanking
	    dw	    08e03h  ; end horz blanking
	    dw	    05e04h  ; start h sync
	    dw	    08a05h  ; end h sync
	    dw	    00014h  ; turn off dword mode
	    dw	    0e317h  ; turn on byte mode
	    dw	    360     ; width
	    dw	    200     ; height


X360Y240    db      0e7h    ; dot clock
	    db	    17	    ; Number of CRTC Registers to update
	    dw	    06b00h  ; horz total
	    dw	    05901h  ; horz displayed
	    dw	    05a02h  ; start horz blanking
	    dw	    08e03h  ; end horz blanking
	    dw	    05e04h  ; start h sync
	    dw	    08a05h  ; end h sync
	    dw	    00d06h  ; vertical total
	    dw	    03e07h  ; overflow (bit 8 of vertical counts)
	    dw	    04109h  ; cell height (2 to double-scan)
	    dw	    0ea10h  ; v sync start
	    dw	    0ac11h  ; v sync end and protect cr0-cr7
	    dw	    0df12h  ; vertical displayed
	    dw	    02d13h  ; offset;
	    dw	    00014h  ; turn off dword mode
	    dw	    0e715h  ; v blank start
	    dw	    00616h  ; v blank end
	    dw	    0e317h  ; turn on byte mode
	    dw	    360
	    dw	    240

X376Y282    db      0e7h
	    db	    18
	    dw	    06e00h  ; horz total
	    dw	    05d01h  ; horz displayed
	    dw	    05e02h  ; start horz blanking
	    dw	    09103h  ; end horz blanking
	    dw	    06204h  ; start h sync
	    dw	    08f05h  ; end h sync
	    dw	    06206h  ; vertical total
	    dw	    0f007h  ; overflow
	    dw	    06109h  ; cell height
	    dw	    0310fh  ;
	    dw	    03710h  ; v sync start
	    dw	    08911h  ; v sync end and protect cr0-cr7
	    dw	    03312h  ; vertical displayed
	    dw	    02f13h  ; offset
	    dw	    00014h  ; turn off dword mode
	    dw	    03c15h  ; v blank start
	    dw	    05c16h  ; v blank end
	    dw	    0eb17h  ; turn on byte mode
	    dw	    376
	    dw	    282


X320Y400    db      0h    ; dot clock
	    db	    03	    ; Number of CRTC Registers to update
	    dw	    04009h  ; cell height
	    dw	    00014h  ; turn off dword mode
	    dw	    0e317h  ; turn on byte mode
	    dw	    320     ; width
	    dw	    400     ; height

X320Y480    db      0e3h    ; dotclock
	    db	    10	    ; Number of CRTC Registers to update
	    dw	    00d06h  ; vertical total
	    dw	    03e07h  ; overflow (bit 8 of vertical counts)
	    dw	    04009h  ; cell height (2 to double-scan)
	    dw	    0ea10h  ; v sync start
	    dw	    0ac11h  ; v sync end and protect cr0-cr7
	    dw	    0df12h  ; vertical displayed
	    dw	    00014h  ; turn off dword mode
	    dw	    0e715h  ; v blank start
	    dw	    00616h  ; v blank end
	    dw	    0e317h  ; turn on byte mode
	    dw	    320     ; width
	    dw	    480     ; height

X360Y400    db      0e7h    ; dot clock
	    db	    09	    ; Number of CRTC Registers to update
	    dw	    06b00h  ; horz total
	    dw	    05901h  ; horz displayed
	    dw	    05a02h  ; start horz blanking
	    dw	    08e03h  ; end horz blanking
	    dw	    05e04h  ; start h sync
	    dw	    08a05h  ; end h sync
	    dw	    04009h  ; cell height
	    dw	    00014h  ; turn off dword mode
	    dw	    0e317h  ; turn on byte mode
	    dw	    360     ; width
	    dw	    400     ; height

X360Y480    db      0e7h
	    db	    17
	    dw	    06b00h  ; horz total
	    dw	    05901h  ; horz displayed
	    dw	    05a02h  ; start horz blanking
	    dw	    08e03h  ; end horz blanking
	    dw	    05e04h  ; start h sync
	    dw	    08a05h  ; end h sync
	    dw	    00d06h  ; vertical total
	    dw	    03e07h  ; overflow
	    dw	    04009h  ; cell height
	    dw	    0ea10h  ; v sync start
	    dw	    0ac11h  ; v sync end and protect cr0-cr7
	    dw	    0df12h  ; vertical displayed
	    dw	    02d13h  ; offset
	    dw	    00014h  ; turn off dword mode
	    dw	    0e715h  ; v blank start
	    dw	    00616h  ; v blank end
	    dw	    0e317h  ; turn on byte mode
	    dw	    360
	    dw	    480

X360Y360    db      0e7h
	    db	    15
	    dw	    06b00h  ; horz total
	    dw	    05901h  ; horz displayed
	    dw	    05a02h  ; start horz blanking
	    dw	    08e03h  ; end horz blanking
	    dw	    05e04h  ; start h sync
	    dw	    08a05h  ; end h sync
	    dw	    04009h  ; cell height
	    dw	    08810h  ; v sync start
	    dw	    08511h  ; v sync end and protect cr0-cr7
	    dw	    06712h  ; vertical displayed
	    dw	    02d13h  ; offset
	    dw	    00014h  ; turn off dword mode
	    dw	    06d15h  ; v blank start
	    dw	    0ba16h  ; v blank end
	    dw	    0e317h  ; turn on byte mode
	    dw	    360
	    dw	    360

X376Y308    db      0e7h
	    db	    18
	    dw	    06e00h  ; horz total
	    dw	    05d01h  ; horz displayed
	    dw	    05e02h  ; start horz blanking
	    dw	    09103h  ; end horz blanking
	    dw	    06204h  ; start h sync
	    dw	    08f05h  ; end h sync
	    dw	    06206h  ; vertical total
	    dw	    00f07h  ; overflow
	    dw	    04009h  ;
	    dw	    0310fh  ;
	    dw	    03710h  ; v sync start
	    dw	    08911h  ; v sync end and protect cr0-cr7
	    dw	    03312h  ; vertical displayed
	    dw	    02f13h  ; offset
	    dw	    00014h  ; turn off dword mode
	    dw	    03c15h  ; v blank start
	    dw	    05c16h  ; v blank end
	    dw	    0e317h  ; turn on byte mode
	    dw	    376
	    dw	    308

X376Y564    db      0e7h
	    db	    18
	    dw	    06e00h  ; horz total
	    dw	    05d01h  ; horz displayed
	    dw	    05e02h  ; start horz blanking
	    dw	    09103h  ; end horz blanking
	    dw	    06204h  ; start h sync
	    dw	    08f05h  ; end h sync
	    dw	    06206h  ; vertical total
	    dw	    0f007h  ; overflow
	    dw	    06009h  ;
	    dw	    0310fh  ;
	    dw	    03710h  ; v sync start
	    dw	    08911h  ; v sync end and protect cr0-cr7
	    dw	    03312h  ; vertical displayed
	    dw	    02f13h  ; offset
	    dw	    00014h  ; turn off dword mode
	    dw	    03c15h  ; v blank start
	    dw	    05c16h  ; v blank end
	    dw	    0e317h  ; turn on byte mode
	    dw	    376
	    dw	    564

%define LAST_X_MODE 11

ModeTable   dd	    X320Y200
	    dd	    X320Y240
	    dd	    X360Y200
	    dd	    X360Y240
	    dd	    X376Y282
	    dd	    X320Y400
	    dd	    X320Y480
	    dd	    X360Y400
	    dd	    X360Y480
	    dd	    X360Y360
	    dd	    X376Y308
	    dd	    X376Y564
