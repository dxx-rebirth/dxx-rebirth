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
; $Source: /cvs/cvsroot/d2x/arch/dos/modex.asm,v $
; $Revision: 1.3 $
; $Author: bradleyb $
; $Date: 2001-10-19 09:01:56 $
;
; Routines to access ModeX VGA memory
;
; $Log: not supported by cvs2svn $
; Revision 1.3  2001/01/31 14:04:44  bradleyb
; Fix compiler warnings
;
; Revision 1.2  2001/01/29 13:35:08  bradleyb
; Fixed build system, minor fixes
;
; Revision 1.1.1.1  2001/01/19 03:30:15  bradleyb
; Import of d2x-0.0.8
;
; Revision 1.1.1.1  1999/06/14 21:57:56  donut
; Import of d1x 1.37 source.
;
; Revision 1.19  1995/03/01  15:37:46  john
; Better ModeX support.
; 
; Revision 1.18  1994/11/24  13:24:20  john
; Made sure that some rep movs had the cld set first.
; Took some unused functions out.
; 
; Revision 1.17  1994/09/22  18:15:02  john
; Made flip page wait for retrace.
; 
; Revision 1.16  1994/09/22  16:08:27  john
; Fixed some palette stuff.
; 
; Revision 1.15  1994/07/13  12:03:05  john
; Added assembly modex line-drawer.
; 
; Revision 1.14  1994/05/06  12:50:34  john
; Added supertransparency; neatend things up; took out warnings.
; 
; Revision 1.13  1994/05/03  19:39:04  john
; *** empty log message ***
; 
; Revision 1.12  1994/02/18  15:32:32  john
; *** empty log message ***
; 
; Revision 1.11  1993/12/21  11:40:36  john
; *** empty log message ***
; 
; Revision 1.10  1993/12/09  15:02:26  john
; Changed palette stuff majorly
; 
; Revision 1.9  1993/12/03  12:11:32  john
; fixed cx/ecx loop bugs.
; 
; Revision 1.8  1993/11/16  11:28:18  john
; *** empty log message ***
; 
; Revision 1.7  1993/10/15  16:23:23  john
; y
; 
; Revision 1.6  1993/09/28  19:07:19  john
; stripped the waitforretrace out of fade to speed things up.
; 
; Revision 1.5  1993/09/26  18:58:58  john
; fade stuff
; 
; Revision 1.4  1993/09/21  14:01:15  john
; turned off video before mode set to reduce flicker.
; 
; Revision 1.3  1993/09/08  11:38:36  john
; changed rcs stuff at beginning.
; 
;
;


[BITS 32]

section .data
%include "dos_vgaregs.inc"
%include "dos_tweak.inc"

extern _gr_var_bwidth
extern _gr_video_memory

global _modex_line_vertincr
global _modex_line_incr1
global _modex_line_incr2
global _modex_line_x1
global _modex_line_x2
global _modex_line_y1
global _modex_line_y2
global _modex_line_Color

_modex_line_vertincr dd 0
_modex_line_incr1    dd 0
_modex_line_incr2    dd 0
_modex_line_x1       dd 0
_modex_line_y1       dd 0
_modex_line_x2       dd 0
_modex_line_y2       dd 0
_modex_line_Color    db 0


_modex_line_routine	dd 0
SavedColor		db 0

%define LEFT_MASK1	1000b
%define LEFT_MASK2	1100b
%define LEFT_MASK3	1110b
%define RIGHT_MASK1	0001b
%define RIGHT_MASK2	0011b
%define RIGHT_MASK3	0111b
%define ALL_MASK	1111b

tmppal		times 768 db 0
fb_pal		dd 0
fb_add		dw 0
fb_count	dd 0

MaskTable1  db	ALL_MASK & RIGHT_MASK1
	    db			  ALL_MASK & RIGHT_MASK2
	    db			  ALL_MASK & RIGHT_MASK3
	    db			  ALL_MASK
	    db			  LEFT_MASK3 & RIGHT_MASK1
	    db			  LEFT_MASK3 & RIGHT_MASK2
	    db			  LEFT_MASK3 & RIGHT_MASK3
	    db			  LEFT_MASK3
	    db			  LEFT_MASK2 & RIGHT_MASK1
	    db			  LEFT_MASK2 & RIGHT_MASK2
	    db			  LEFT_MASK2 & RIGHT_MASK3
	    db			  LEFT_MASK2
	    db			  LEFT_MASK1 & RIGHT_MASK1
	    db			  LEFT_MASK1 & RIGHT_MASK2
	    db			  LEFT_MASK1 & RIGHT_MASK3
	    db			  LEFT_MASK1

MaskTable2  db	ALL_MASK,RIGHT_MASK1
	    db			  ALL_MASK,RIGHT_MASK2
	    db			  ALL_MASK,RIGHT_MASK3
	    db			  ALL_MASK,ALL_MASK
	    db			  LEFT_MASK3,RIGHT_MASK1
	    db			  LEFT_MASK3,RIGHT_MASK2
	    db			  LEFT_MASK3,RIGHT_MASK3
	    db			  LEFT_MASK3,ALL_MASK
	    db			  LEFT_MASK2,RIGHT_MASK1
	    db			  LEFT_MASK2,RIGHT_MASK2
	    db			  LEFT_MASK2,RIGHT_MASK3
	    db			  LEFT_MASK2,ALL_MASK
	    db			  LEFT_MASK1,RIGHT_MASK1
	    db			  LEFT_MASK1,RIGHT_MASK2
	    db			  LEFT_MASK1,RIGHT_MASK3
	    db			  LEFT_MASK1,ALL_MASK

DrawTable   dd	DrawMR
	    dd			  DrawMR
	    dd			  DrawMR
	    dd			  DrawM
	    dd			  DrawLMR
	    dd			  DrawLMR
	    dd			  DrawLMR
	    dd			  DrawLM
	    dd			  DrawLMR
	    dd			  DrawLMR
	    dd			  DrawLMR
	    dd			  DrawLM
	    dd			  DrawLMR
	    dd			  DrawLMR
	    dd			  DrawLMR
	    dd			  DrawLM

section .text

global	_gr_sync_display

_gr_sync_display:
		mov     dx, VERT_RESCAN
VS2A:		in	al, dx
		and     al, 08h
		jnz     VS2A        ; Loop until not in vertical retrace
VS2B:		in	al, dx
		and     al, 08h
		jz      VS2B        ; Loop until in vertical retrace
		ret


; gr_modex_uscanline(int x1,int x2,int y,int color);
global	_gr_modex_uscanline

_gr_modex_uscanline:
		push	ebx
		mov	eax,[esp+8];x1
		mov	edx,[esp+12];x2
		mov	ebx,[esp+16];y
		mov	ecx,[esp+20];color
		push    edi

		; EAX = X1  (X1 and X2 don't need to be sorted)
		; EDX = X2
		; EBX = Y
		; ECX = Color

		mov	[SavedColor], cl

		;mov     ebx, _RowOffset[ebx*4]
		mov	edi, [_gr_var_bwidth]
		imul	edi, ebx
		add	edi, [_gr_video_memory]
		cmp     eax, edx
		jle	@@f1
		xchg    eax, edx
@@f1:	  mov	  bl, al
		shl     bl, 2
		mov     bh, dl
		and     bh, 011b
		or      bl, bh
		and     ebx, 0fh
		; BX = LeftRight switch command. (4bit)
		shr     eax, 2
		shr     edx, 2
		add     edi, eax
		; EDI = Offset into video memory
		mov     ecx, edx
		sub     ecx, eax
		; ECX = X2/4 - X1/4 - 1
		jnz     LargerThan4

;======================= ONE GROUP OF 4 OR LESS TO DRAW ====================

		mov     dx, SC_INDEX
		mov     al, SC_MAP_MASK
		out     dx, al
		inc     dx
		mov	al, [MaskTable1+ebx]
		out     dx, al
		mov	al, [SavedColor]
		mov     [edi], al           ; Write the one pixel
		pop     edi
		pop	ebx
		ret

LargerThan4:
		dec     ecx
		jnz     LargerThan8

;===================== TWO GROUPS OF 4 OR LESS TO DRAW ====================

		mov	cx, word [MaskTable2+ebx*2]
		mov	bl, [SavedColor]
		mov     dx, SC_INDEX
		mov     al, SC_MAP_MASK
		out     dx, al
		inc     dx
		mov     al, cl
		out     dx, al
		mov     [edi], bl           ; Write the left pixel
		mov     al, ch
		out     dx, al
		mov     [edi+1], bl         ; Write the right pixel
		pop     edi
		pop	ebx
		ret

;========================= MANY GROUPS OF 4 TO DRAW ======================
LargerThan8:
		mov     dx, SC_INDEX
		mov     al, SC_MAP_MASK
		out     dx, al
		inc     dx
		; DX = SC_INDEX
		mov	al, [SavedColor]
		mov     ah, al
		; AL,AH = color of pixel
		jmp	dword [DrawTable+ebx*4]


DrawM:  ;AH=COLOR, EDI=DEST, CX=X2/4-X1/4-1, BX=Table Index
		mov     al, ALL_MASK
		out     dx, al
		mov     al, ah
		cld
		add     ecx, 2
		shr     ecx, 1
		rep     stosw       ; Write the middle pixels
		jnc	@@f2
		stosb               ; Write the middle odd pixel
@@f2:		pop	edi
		pop	ebx
		ret

DrawLM:
		;AH=COLOR, EDI=DEST, CX=X2/4-X1/4-1, BX=Table Index
		mov	al, byte [MaskTable2+ebx*2]
		out     dx, al
		mov     [edi], ah       ; Write leftmost pixels
		inc     edi
		mov     al, ALL_MASK
		out     dx, al
		mov     al, ah
		cld
		inc     ecx
		shr     ecx, 1
		rep     stosw       ; Write the middle pixels
		jnc	@@f3
		stosb               ; Write the middle odd pixel
@@f3:		pop	edi
		pop	ebx
		ret


DrawLMR: ;AH=COLOR, EDI=DEST, CX=X2/4-X1/4-1, BX=Table Index
		mov	bx, word [MaskTable2+ebx*2]
		mov     al, bl
		out     dx, al
		mov     [edi], ah       ; Write leftmost pixels
		inc     edi
		mov     al, ALL_MASK
		out     dx, al
		mov     al, ah
		cld
		shr     ecx, 1
		rep     stosw       ; Write the middle pixels
		jnc	@@f4
		stosb               ; Write the middle odd pixel
@@f4:		mov	al, bh
		out     dx, al
		mov     [edi], ah   ; Write the rightmost pixels
		pop     edi
		pop	ebx
		ret

DrawMR: ;AH=COLOR, EDI=DEST, CX=X2/4-X1/4-1, BX=Table Index
		mov	bx, word [MaskTable2+ebx*2]
		mov     al, ALL_MASK
		out     dx, al
		mov     al, ah
		cld
		inc     ecx
		shr     ecx, 1
		rep     stosw       ; Write the middle pixels
		jnc	@@f5
		stosb               ; Write the middle odd pixel
@@f5:	  mov	  al, bh
		out     dx, al
		mov     [edi], ah   ; Write the rightmost pixels
		pop     edi
		pop	ebx
		ret


global _gr_modex_setmode

_gr_modex_setmode:
	mov   eax,[esp+4]
	push  ecx
	push  edx
	push  esi
	push  edi

	mov   ecx, eax
	dec   ecx

	cmp   ecx, LAST_X_MODE
	jbe   @@f6
	mov   ecx, 0
@@f6:

	;call  turn_screen_off

	push  ecx                   ; some bios's dont preserve cx

	;mov ax, 1201h
	;mov bl, 31h     ; disable palette loading at mode switch
	;int 10h
	mov   ax,13h                ; let the BIOS set standard 256-color
	int   10h                   ;  mode (320x200 linear)
	;mov ax, 1200h
	;mov bl, 31h     ; enable palette loading at mode switch
	;int 10h

	pop   ecx

	mov   dx,SC_INDEX
	mov   ax,0604h
	out   dx,ax                 ; disable chain4 mode

	mov   dx,SC_INDEX
	mov   ax,0100h
	out   dx,ax                 ; synchronous reset while setting Misc
								;  Output for safety, even though clock
								;  unchanged

	mov   esi, dword [ModeTable+ecx*4]
	lodsb

	or    al,al
	jz    DontSetDot

	mov   dx,MISC_OUTPUT
	out   dx,al               ; select the dot clock and Horiz
							  ;  scanning rate

	;mov   dx,SC_INDEX
	;mov   al,01h
	;out   dx,al
	;inc   dx
	;in    al, dx
	;or    al, 1000b
	;out   dx, al

DontSetDot:
	mov   dx,SC_INDEX
	mov   ax,0300h
	out   dx,ax        ; undo reset (restart sequencer)

	mov   dx,CRTC_INDEX       ; reprogram the CRT Controller
	mov   al,11h              ; VSync End reg contains register write
	out   dx,al               ; protect bit
	inc   dx                  ; CRT Controller Data register
	in    al,dx               ; get current VSync End register setting
	and   al,07fh             ; remove write protect on various
	out   dx,al               ; CRTC registers
	dec   dx                  ; CRT Controller Index
	cld
	xor   ecx,ecx
	lodsb
	mov   cl,al

SetCRTParmsLoop:
	lodsw                     ; get the next CRT Index/Data pair
	out   dx,ax               ; set the next CRT Index/Data pair
	loop  SetCRTParmsLoop

	mov   dx,SC_INDEX
	mov   ax,0f02h
	out   dx,ax       ; enable writes to all four planes
	mov   edi, [_gr_video_memory]  ; point ES:DI to display memory
	xor   eax,eax		  ; clear to zero-value pixels
	mov   ecx,4000h 	  ; # of dwords in display memory
	rep   stosd		  ; clear all of display memory

	;  Set pysical screen dimensions

	xor   eax, eax
	lodsw                               ; Load scrn pixel width
	mov   cx, ax
	shl   eax, 16

	mov   dx,CRTC_INDEX
	mov   al,CRTC_OFFSET
	out   dx,al
	inc   dx
	mov   ax,cx
	shr   ax,3
	out   dx,al

	;mov     si, 360
	;@@:
	;mov     ax, 04f06h
	;mov     bl, 0
	;mov     cx, si
	;int     10h
	;cmp     cx, si
	;je      @f
	;inc     si
	;jmp     @B
	;@@:
	;movzx     eax, si

	lodsw                               ; Load Screen Phys. Height

	;call  turn_screen_on

	pop  edi
	pop  esi
	pop  edx
	pop  ecx

	ret

global _gr_modex_setplane

_gr_modex_setplane:
	mov	eax,[esp+4]

	mov	cl, al

	; SELECT WRITE PLANE
	and	cl,011b 	    ;CL = plane
	mov	ax,0100h + MAP_MASK ;AL = index in SC of Map Mask reg
	shl	ah,cl		    ;set only the bit for the required
									; plane to 1
	mov	dx,SC_INDEX	    ;set the Map Mask to enable only the
	out	dx,ax		    ; pixel's plane

	; SELECT READ PLANE
	mov	ah,cl		    ;AH = plane
	mov	al,READ_MAP	    ;AL = index in GC of the Read Map reg
	mov	dx,GC_INDEX	    ;set the Read Map to read the pixel's
	out	dx,ax		    ; plane
	ret

global	_gr_modex_setstart

_gr_modex_setstart:
	push	ebx
	mov	eax,[esp+8]
	mov	edx,[esp+12]
	mov	ebx,[esp+16]
	push	ebx

	; EAX = X
	; EDX = Y
	; EBX = Wait for retrace

	mov	ecx, [_gr_var_bwidth]
	imul	ecx, edx

	shr	eax, 2
	add	eax, ecx

	mov	ch, ah
	mov	bh, al

	mov	bl, CC_START_LO
	mov	cl, CC_START_HI

	cli
	mov	dx, VERT_RESCAN
WaitDE: in      al, dx
	test	al, 01h
	jnz	WaitDE

	mov	dx, CRTC_INDEX
	mov	ax, bx
	out	dx, ax	    ; Start address low
	mov	ax, cx
	out	dx, ax	    ; Start address high
	sti

	pop	ebx
	cmp	ebx, 0
	je	NoWaitVS
	mov	dx, VERT_RESCAN
WaitVS: in      al, dx
	test	al, 08h
	jz	WaitVS	    ; Loop until in vertical retrace
NoWaitVS:

	pop	ebx

	ret




%macro ModeXAddr 0
; given: ebx=x, eax=y, return cl=plane mask, ebx=address, trash eax
	mov	cl, bl
	and	cl, 3
	shr	ebx, 2
	imul	eax, [_gr_var_bwidth]
	add	ebx, eax
	add	ebx, [_gr_video_memory]
%endmacro


;-----------------------------------------------------------------------
;
; Line drawing function for all MODE X 256 Color resolutions
; Based on code from "PC and PS/2 Video Systems" by Richard Wilton.

global _gr_modex_line

_gr_modex_line:
	pusha

	mov	dx,SC_INDEX	; setup for plane mask access

; check for vertical line

	mov	esi,[_gr_var_bwidth]
	mov	ecx,[_modex_line_x2]
	sub	ecx,[_modex_line_x1]
	jz	near VertLine

; force x1 < x2

	jns	L01

	neg	ecx

	mov	ebx,[_modex_line_x2]
	xchg	ebx,[_modex_line_x1]
	mov	[_modex_line_x2],ebx

	mov	ebx,[_modex_line_y2]
	xchg	ebx,[_modex_line_y1]
	mov	[_modex_line_y2],ebx

; calc dy = abs(y2 - y1)

L01:
	mov	ebx,[_modex_line_y2]
	sub	ebx,[_modex_line_y1]
	jnz	short skip
	jmp     HorizLine
skip:		jns	L03

	neg	ebx
	neg	esi

; select appropriate routine for slope of line

L03:
	mov	[_modex_line_vertincr],esi
	mov	dword [_modex_line_routine],LoSlopeLine
	cmp	ebx,ecx
	jle	L04
	mov	dword [_modex_line_routine],HiSlopeLine
	xchg	ebx,ecx

; calc initial decision variable and increments

L04:
	shl	ebx,1
	mov	[_modex_line_incr1],ebx
	sub	ebx,ecx
	mov	esi,ebx
	sub	ebx,ecx
	mov	[_modex_line_incr2],ebx

; calc first pixel address

	push	ecx
	mov	eax,[_modex_line_y1]
	mov	ebx,[_modex_line_x1]
	ModeXAddr
	mov	edi,ebx
	mov	al,1
	shl	al,cl
	mov	ah,al		; duplicate nybble
	shl	al,4
	add	ah,al
	mov	bl,ah
	pop	ecx
	inc	ecx
	jmp	near dword [_modex_line_routine]

; routine for verticle lines

VertLine:
	mov	eax,[_modex_line_y1]
	mov	ebx,[_modex_line_y2]
	mov	ecx,ebx
	sub	ecx,eax
	jge	L31
	neg	ecx
	mov	eax,ebx

L31:
	inc	ecx
	mov	ebx,[_modex_line_x1]
	push	ecx
	ModeXAddr

	mov	ah,1
	shl	ah,cl
	mov	al,MAP_MASK
	out	dx,ax
	pop	ecx
	mov	ax, word [_modex_line_Color]

; draw the line

L32:
	mov	[ebx],al
	add	ebx,esi
	loop	L32
	jmp	Lexit

; routine for horizontal line

HorizLine:

	mov	eax,[_modex_line_y1]
	mov	ebx,[_modex_line_x1]
	ModeXAddr

	mov	edi,ebx     ; set dl = first byte mask
	mov	dl,00fh
	shl	dl,cl

	mov	ecx,[_modex_line_x2] ; set dh = last byte mask
	and	cl,3
	mov	dh,00eh
	shl	dh,cl
	not	dh

; determine byte offset of first and last pixel in line

	mov	eax,[_modex_line_x2]
	mov	ebx,[_modex_line_x1]

	shr	eax,2     ; set ax = last byte column
	shr	ebx,2    ; set bx = first byte column
	mov	ecx,eax    ; cx = ax - bx
	sub	ecx,ebx

	mov    	eax,edx    ; mov end byte masks to ax
	mov	dx,SC_INDEX ; setup dx for VGA outs
	mov	bl,[_modex_line_Color]

; set pixels in leftmost byte of line

	or	ecx,ecx      ; is start and end pt in same byte
	jnz	L42        ; no !
	and	ah,al      ; combine start and end masks
	jmp	short L44

L42:            push    eax
	mov     ah,al
	mov     al,MAP_MASK
	out     dx,ax
	mov	al,bl
	stosb
	dec	ecx

; draw remainder of the line

L43:
	mov	ah,0Fh
	mov	al,MAP_MASK
	out	dx,ax
	mov	al,bl
	rep	stosb
	pop     eax

; set pixels in rightmost byte of line

L44:	
	mov	al,MAP_MASK
	out	dx, ax
	mov	[edi],bl
	jmp	Lexit


; routine for dy >= dx (slope <= 1)

LoSlopeLine:
	mov	al,MAP_MASK
	mov	bh,byte [_modex_line_Color]
L10:
	mov	ah,bl

L11:
	or	ah,bl
	rol	bl,1
	jc	L14

; bit mask not shifted out

	or	esi,esi
	jns	L12
	add	esi,[_modex_line_incr1]
	loop	L11

	out	dx,ax
	mov	[edi],bh
	jmp	short Lexit

L12:
	add	esi,[_modex_line_incr2]
	out	dx,ax
	mov	[edi],bh
	add	edi,[_modex_line_vertincr]
	loop	L10
	jmp	short Lexit

; bit mask shifted out

L14:            out	dx,ax
	mov	[edi],bh
	inc	edi
	or	esi,esi
	jns	L15
	add	esi,[_modex_line_incr1]
	loop	L10
	jmp	short Lexit

L15:
	add	esi,[_modex_line_incr2]
	add	edi,[_modex_line_vertincr]
	loop	L10
	jmp	short Lexit

; routine for dy > dx (slope > 1)

HiSlopeLine:
	mov	ebx,[_modex_line_vertincr]
	mov	al,MAP_MASK
L21:    out	dx,ax
	push	eax
	mov	al,[_modex_line_Color]
	mov	[edi],al
	pop	eax
	add	edi,ebx

L22:
	or	esi,esi
	jns	L23

	add	esi,[_modex_line_incr1]
	loop	L21
	jmp	short Lexit

L23:
	add	esi,[_modex_line_incr2]
	rol	ah,1
	adc	edi,0
lx21:	loop	L21

; return to caller

Lexit:
	popa
	ret
