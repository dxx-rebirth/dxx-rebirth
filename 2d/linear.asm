; $Id: linear.asm,v 1.4 2003-12-08 21:21:16 btb Exp $
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
;
; Routines to access linear VGA memory
;
; Old Log:
;
; Revision 1.20  1994/11/28  17:08:30  john
; Took out some unused functions in linear.asm, moved
; gr_linear_movsd from linear.asm to bitblt.c, made sure that
; the code in ibiblt.c sets the direction flags before rep movsing.
; 
; Revision 1.19  1994/11/27  22:57:56  john
; Took out some code that was never called.
; 
; Revision 1.18  1994/09/12  14:40:16  john
; Neatend.
; 
; Revision 1.17  1994/07/27  18:30:30  john
; Took away the blending table.
; 
; Revision 1.16  1994/04/08  16:59:28  john
; Add fading poly's; Made palette fade 32 instead of 16.
; 
; Revision 1.15  1993/12/21  20:10:03  john
; *** empty log message ***
; 
; Revision 1.14  1993/12/21  19:58:31  john
; added selector stuff
; 
; Revision 1.13  1993/12/21  11:40:51  john
; *** empty log message ***
; 
; Revision 1.12  1993/12/09  15:01:52  john
; Changed palette stuff majorly
; 
; Revision 1.11  1993/12/08  16:41:02  john
; *** empty log message ***
; 
; Revision 1.10  1993/12/08  11:50:17  john
; Fixed bug with gr_init
; 
; Revision 1.9  1993/12/07  12:32:12  john
; moved bmd_palette to gr_palette
; 
; Revision 1.8  1993/12/03  12:11:25  john
; *** empty log message ***
; 
; Revision 1.7  1993/11/16  11:29:08  john
; *** empty log message ***
; 
; Revision 1.6  1993/10/15  16:22:13  john
; *** empty log message ***
; 
; Revision 1.5  1993/09/29  16:15:28  john
; added assembler linear_line
; 
; Revision 1.4  1993/09/26  18:59:27  john
; fade stuff
; 
; Revision 1.3  1993/09/21  14:00:59  john
; added code to save 43/50 line text modes.
; 
; Revision 1.2  1993/09/16  17:28:06  john
; added code to save/restore video mode
; 
; Revision 1.1  1993/09/08  11:41:30  john
; Initial revision
; 
;
;


[BITS 32]

section .data
		; Put data here
%ifdef __ELF__
%define _gr_var_color gr_var_color
%define _gr_var_bitmap gr_var_bitmap
%define _gr_var_bwidth gr_var_bwidth
%endif
		
		global _gr_var_color
		global _gr_var_bitmap
		global _gr_var_bwidth
		_gr_var_color	dd  0
		_gr_var_bitmap	dd  0
		_gr_var_bwidth	dd  0

		; Local variables for gr_linear_line_
		AdjUp		dd  0	;error term adjust up on each advance
		AdjDown 	dd  0	;error term adjust down when error term turns over
		WholeStep	dd  0	;minimum run length
		XAdvance	dd  0	;1 or -1, for direction in which X advances
		XStart		dd  0
		YStart		dd  0
		XEnd		dd  0
		YEnd		dd  0

section .text
; Fast run length slice line drawing implementation for mode 0x13, the VGA's
; 320x200 256-color mode.
; Draws a line between the specified endpoints in color Color.


global _gr_linear_line
global gr_linear_line
_gr_linear_line:
gr_linear_line:

	cld

	push	ebx  ;preserve C register variables
	push	esi
	push    edi
	push    ebp

	mov	eax,[esp+20]
	mov	edx,[esp+24]
	mov	ebx,[esp+28]
	mov	ecx,[esp+32]
	mov	[XStart], eax
	mov	[YStart], edx
	mov	[XEnd], ebx
	mov	[YEnd], ecx

	mov	ebp, [_gr_var_bwidth]

; We'll draw top to bottom, to reduce the number of cases we have to handle,
; and to make lines between the same endpoints always draw the same pixels.

	mov	eax,[YStart]
	cmp	eax,[YEnd]
	jle     LineIsTopToBottom
	xchg	[YEnd], eax    ;swap endpoints
	mov	[YStart], eax
	mov	ebx, [XStart]
	xchg	[XEnd], ebx
	mov	[XStart], ebx

LineIsTopToBottom:

; Point EDI to the first pixel to draw.
	mov     edx,ebp
	mul	edx		;[YStart] * ebp
	mov	esi, [XStart]
	mov     edi, esi
	add	edi, [_gr_var_bitmap]
	add	edi, eax	;EDI = [YStart] * ebp + [XStart]
							; = offset of initial pixel

; Figure out how far we're going vertically (guaranteed to be positive).
	mov	ecx, [YEnd]
	sub	ecx, [YStart]	  ;ECX = YDelta

; Figure out whether we're going left or right, and how far we're going
; horizontally. In the process, special-case vertical lines, for speed and
; to avoid nasty boundary conditions and division by 0.

	mov	edx, [XEnd]
	sub     edx, esi        ;XDelta
	jnz     NotVerticalLine ;XDelta == 0 means vertical line
							;it is a vertical line
							;yes, special case vertical line

	mov	eax, [_gr_var_color]
VLoop:
	mov     [edi],al
	add     edi,ebp
	dec     ecx
	jns     VLoop
	jmp     Done

; Special-case code for horizontal lines.

IsHorizontalLine:
	mov	eax, [_gr_var_color]
	mov     ah,al           ;duplicate in high byte for word access
	and     ebx,ebx         ;left to right?
	jns     DirSet          ;yes
	sub     edi, edx        ;currently right to left, point to left end so we
							; can go left to right (avoids unpleasantness with
							; right to left REP STOSW)
DirSet:
	mov     ecx, edx
	inc     ecx             ;# of pixels to draw

	shr     ecx, 1          ;# of words to draw
	rep     stosw           ;do as many words as possible
	adc     ecx, ecx
	rep     stosb           ;do the odd byte, if there is one

	jmp     Done

; Special-case code for diagonal lines.

IsDiagonalLine:
	mov	eax, [_gr_var_color]
	add     ebx, ebp    ;advance distance from one pixel to next

DLoop:
	mov     [edi],al
	add     edi, ebx
	dec     ecx
	jns     DLoop
	jmp     Done

NotVerticalLine:
	mov	ebx,1		    ;assume left to right, so [XAdvance] = 1
								;***leaves flags unchanged***
	jns     LeftToRight         ;left to right, all set
	neg	ebx		    ;right to left, so [XAdvance] = -1
	neg     edx                 ;|XDelta|

LeftToRight:
; Special-case horizontal lines.

	and     ecx, ecx            ;YDelta == 0?
	jz      IsHorizontalLine    ;yes

; Special-case diagonal lines.
	cmp     ecx, edx            ;YDelta == XDelta?
	jz      IsDiagonalLine      ;yes

; Determine whether the line is X or Y major, and handle accordingly.
	cmp     edx, ecx
	jae     XMajor
	jmp     YMajor

; X-major (more horizontal than vertical) line.

XMajor:
		and     ebx,ebx         ;left to right?
		jns     DFSet           ;yes, CLD is already set
	std                     ;right to left, so draw backwards
DFSet:
		mov     eax,edx         ;XDelta
		sub     edx,edx         ;prepare for division
		div     ecx             ;EAX = XDelta/YDelta
				; (minimum # of pixels in a run in this line)
								;EDX = XDelta % YDelta
		mov     ebx,edx         ;error term adjust each time Y steps by 1;
		add     ebx,ebx         ; used to tell when one extra pixel should be
		mov	[AdjUp], ebx	  ; drawn as part of a run, to account for
				; fractional steps along the X axis per
				; 1-pixel steps along Y
		mov     esi, ecx        ;error term adjust when the error term turns
		add     esi, esi        ; over, used to factor out the X step made at
		mov	[AdjDown], esi	  ; that time

; Initial error term; reflects an initial step of 0.5 along the Y axis.
		sub     edx, esi        ;(XDelta % YDelta) - (YDelta * 2)
				;DX = initial error term
; The initial and last runs are partial, because Y advances only 0.5 for
; these runs, rather than 1. Divide one full run, plus the initial pixel,
; between the initial and last runs.
		mov     esi, ecx        ;SI = YDelta
		mov     ecx, eax        ;whole step (minimum run length)
		shr     ecx,1
		inc     ecx             ;initial pixel count = (whole step / 2) + 1;
				; (may be adjusted later). This is also the
		; final run pixel count
		push    ecx             ;remember final run pixel count for later
; If the basic run length is even and there's no fractional advance, we have
; one pixel that could go to either the initial or last partial run, which
; we'll arbitrarily allocate to the last run.
; If there is an odd number of pixels per run, we have one pixel that can't
; be allocated to either the initial or last partial run, so we'll add 0.5 to
; the error term so this pixel will be handled by the normal full-run loop.
		add     edx,esi         ;assume odd length, add YDelta to error term
		; (add 0.5 of a pixel to the error term)
	test    al,1            ;is run length even?
	jnz     XMajorAdjustDone ;no, already did work for odd case, all set
		sub     edx,esi         ;length is even, undo odd stuff we just did
		and     ebx,ebx         ;is the adjust up equal to 0?
	jnz     XMajorAdjustDone ;no (don't need to check for odd length,
								; because of the above test)
		dec     ecx             ;both conditions met; make initial run 1
				; shorter

XMajorAdjustDone:
		mov	[WholeStep],eax   ;whole step (minimum run length)
		mov	eax, [_gr_var_color]	   ;AL = drawing color
; Draw the first, partial run of pixels.
	rep     stosb           ;draw the final run
		add     edi,ebp ;advance along the minor axis (Y)
; Draw all full runs.
		cmp     esi,1           ;are there more than 2 scans, so there are
								; some full runs? (SI = # scans - 1)
	jna     XMajorDrawLast  ;no, no full runs
		dec     edx             ;adjust error term by -1 so we can use
				; carry test
		shr     esi,1           ;convert from scan to scan-pair count
	jnc     XMajorFullRunsOddEntry  ;if there is an odd number of scans,
					; do the odd scan now
XMajorFullRunsLoop:
		mov	ecx, [WholeStep]  ;run is at least this long
		add     edx,ebx         ;advance the error term and add an extra
	jnc     XMajorNoExtra   ; pixel if the error term so indicates
		inc     ecx             ;one extra pixel in run
		sub	edx,[AdjDown]	  ;reset the error term
XMajorNoExtra:
		rep     stosb           ;draw this scan line's run
		add     edi,ebp ;advance along the minor axis (Y)
XMajorFullRunsOddEntry:         ;enter loop here if there is an odd number
				; of full runs
		mov	ecx,[WholeStep]   ;run is at least this long
		add     edx,ebx         ;advance the error term and add an extra
	jnc     XMajorNoExtra2  ; pixel if the error term so indicates
		inc     ecx             ;one extra pixel in run
		sub	edx,[AdjDown]	  ;reset the error term
XMajorNoExtra2:
		rep     stosb           ;draw this scan line's run
		add     edi,ebp ;advance along the minor axis (Y)

		dec     esi
	jnz     XMajorFullRunsLoop
; Draw the final run of pixels.
XMajorDrawLast:
		pop     ecx             ;get back the final run pixel length
	rep     stosb           ;draw the final run

	cld                     ;restore normal direction flag
	jmp     Done
; Y-major (more vertical than horizontal) line.
YMajor:
		mov	[XAdvance],ebx	  ;remember which way X advances
		mov     eax,ecx         ;YDelta
		mov     ecx,edx         ;XDelta
		sub     edx,edx         ;prepare for division
		div     ecx             ;EAX = YDelta/XDelta
				; (minimum # of pixels in a run in this line)
								;EDX = YDelta % XDelta
		mov     ebx,edx         ;error term adjust each time X steps by 1;
		add     ebx,ebx         ; used to tell when one extra pixel should be
		mov	[AdjUp],ebx	  ; drawn as part of a run, to account for
				; fractional steps along the Y axis per
				; 1-pixel steps along X
		mov     esi,ecx         ;error term adjust when the error term turns
		add     esi,esi         ; over, used to factor out the Y step made at
		mov	[AdjDown], esi	  ; that time

; Initial error term; reflects an initial step of 0.5 along the X axis.
		sub     edx,esi         ;(YDelta % XDelta) - (XDelta * 2)
				;DX = initial error term
; The initial and last runs are partial, because X advances only 0.5 for
; these runs, rather than 1. Divide one full run, plus the initial pixel,
; between the initial and last runs.
		mov     esi,ecx         ;SI = XDelta
		mov     ecx,eax         ;whole step (minimum run length)
		shr     ecx,1
		inc     ecx             ;initial pixel count = (whole step / 2) + 1;
				; (may be adjusted later)
		push    ecx             ;remember final run pixel count for later

; If the basic run length is even and there's no fractional advance, we have
; one pixel that could go to either the initial or last partial run, which
; we'll arbitrarily allocate to the last run.
; If there is an odd number of pixels per run, we have one pixel that can't
; be allocated to either the initial or last partial run, so we'll add 0.5 to
; the error term so this pixel will be handled by the normal full-run loop.
		add     edx,esi         ;assume odd length, add XDelta to error term
	test    al,1            ;is run length even?
	jnz     YMajorAdjustDone ;no, already did work for odd case, all set
		sub     edx,esi         ;length is even, undo odd stuff we just did
		and     ebx,ebx         ;is the adjust up equal to 0?
	jnz     YMajorAdjustDone ;no (don't need to check for odd length,
		 ; because of the above test)
		dec     ecx             ;both conditions met; make initial run 1
				; shorter
YMajorAdjustDone:
		mov	[WholeStep],eax   ;whole step (minimum run length)
		mov	eax,[_gr_var_color]	   ;AL = drawing color
		mov	ebx, [XAdvance]   ;which way X advances

; Draw the first, partial run of pixels.
YMajorFirstLoop:
		mov     [edi],al        ;draw the pixel
		add     edi,ebp ;advance along the major axis (Y)
		dec     ecx
	jnz     YMajorFirstLoop
		add     edi,ebx           ;advance along the minor axis (X)
; Draw all full runs.
		cmp     esi,1            ;# of full runs. Are there more than 2
		; columns, so there are some full runs?
		; (SI = # columns - 1)
	jna     YMajorDrawLast  ;no, no full runs
		dec     edx              ;adjust error term by -1 so we can use
				; carry test
		shr     esi,1            ;convert from column to column-pair count
	jnc     YMajorFullRunsOddEntry  ;if there is an odd number of
					; columns, do the odd column now
YMajorFullRunsLoop:
		mov	ecx,[WholeStep]   ;run is at least this long
		add	edx,[AdjUp]	  ;advance the error term and add an extra
	jnc     YMajorNoExtra   ; pixel if the error term so indicates
		inc     ecx              ;one extra pixel in run
		sub	edx,[AdjDown]	  ;reset the error term
YMajorNoExtra:
				;draw the run
YMajorRunLoop:
		mov     [edi],al        ;draw the pixel
		add     edi,ebp ;advance along the major axis (Y)
		dec     ecx
	jnz     YMajorRunLoop
		add     edi,ebx         ;advance along the minor axis (X)
YMajorFullRunsOddEntry:         ;enter loop here if there is an odd number
				; of full runs
		mov	ecx,[WholeStep]   ;run is at least this long
		add	edx,[AdjUp]	  ;advance the error term and add an extra
	jnc     YMajorNoExtra2  ; pixel if the error term so indicates
		inc     ecx              ;one extra pixel in run
		sub	edx, [AdjDown]	  ;reset the error term
YMajorNoExtra2:
				;draw the run
YMajorRunLoop2:
		mov     [edi],al         ;draw the pixel
		add     edi,ebp ;advance along the major axis (Y)
		dec     ecx
	jnz     YMajorRunLoop2
		add     edi,ebx           ;advance along the minor axis (X)

		dec     esi
	jnz     YMajorFullRunsLoop
; Draw the final run of pixels.
YMajorDrawLast:
		pop     ecx              ;get back the final run pixel length
YMajorLastLoop:
		mov     [edi],al         ;draw the pixel
		add     edi,ebp ;advance along the major axis (Y)
		dec     ecx
	jnz     YMajorLastLoop
Done:
	pop ebp
	pop edi
	pop esi
	pop ebx;restore C register variables
    ret

global _gr_linear_stosd
global gr_linear_stosd

_gr_linear_stosd:
gr_linear_stosd:

			; EAX -> Destination buffer
			; EDX -> Byte to store
			; EBX -> Number of bytes to move

			push	ebx
			push    edi
			mov	eax,[esp+12]
			mov	edx,[esp+16]
			mov	ebx,[esp+20]
			mov     edi, eax
			mov     dh, dl
			mov     ax, dx
			shl     eax, 16
			mov     ax, dx
			cld
			mov     ecx, ebx
			shr     ecx, 2
			rep     stosd
			mov     ecx, ebx
			and     ecx, 011b
			rep     stosb
			pop     edi
			pop	ebx
			ret
