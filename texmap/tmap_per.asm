; $Id: tmap_per.asm,v 1.1.1.1 2006/03/17 20:00:04 zicodxx Exp $
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
; Perspective texture mapper inner loop.
;
;

[BITS 32]

global	_asm_tmap_scanline_per
global	asm_tmap_scanline_per

%include        "tmap_inc.asm"

[SECTION .data]
align 4
    ;extern _per2_flag;:dword
%ifdef __ELF__
; Cater for ELF compilers...
global x
%define _loop_count loop_count
%define _new_end new_end
%define _scan_doubling_flag scan_doubling_flag
%define _linear_if_far_flag linear_if_far_flag
%endif

	global _x
	global _loop_count
        global _new_end
	global _scan_doubling_flag
	global _linear_if_far_flag

;	 global _max_ecx
;	 global _min_ecx

    mem_edx dd 0
    x:
    _x		dd	0
    _loop_count dd	0

;    _max_ecx	 dd	 0
;    _min_ecx	 dd	 55555555h
    _new_end     dd      1       ; if set, use new, but slower, way of finishing off extra pixels on scanline, 01/10/95 --MK

    _scan_doubling_flag dd 0
    _linear_if_far_flag dd 0

;---------- local variables
align 4
    req_base    dd	0
    req_size    dd	0
    U0          dd	0
    U1          dd	0
    V0          dd	0
    V1          dd	0
    num_left_over   dd	0
    DU1         dd	0
    DV1         dd	0
    DZ1         dd	0

[SECTION .text]

; --------------------------------------------------------------------------------------------------
; Enter:
;	_xleft	fixed point left x coordinate
;	_xright	fixed point right x coordinate
;	_y	fixed point y coordinate
;	_pixptr	address of source pixel map
;	_u	fixed point initial u coordinate
;	_v	fixed point initial v coordinate
;	_z	fixed point initial z coordinate
;	_du_dx	fixed point du/dx
;	_dv_dx	fixed point dv/dx
;	_dz_dx	fixed point dz/dx

;   for (x = (int) xleft; x <= (int) xright; x++) {
;      _setcolor(read_pixel_from_tmap(srcb,((int) (u/z)) & 63,((int) (v/z)) & 63));
;      _setpixel(x,y);
;
;      u += du_dx;
;      v += dv_dx;
;      z += dz_dx;
;   }


align	16
_asm_tmap_scanline_per:
asm_tmap_scanline_per:
;        push    es
	pusha

;---------------------------- setup for loop ---------------------------------
; Setup for loop:	_loop_count  iterations = (int) xright - (int) xleft
;	esi	source pixel pointer = pixptr
;	edi	initial row pointer = y*320+x
; NOTE: fx_xright and fx_xleft changed from fix to int by mk on 12/01/94.

; set esi = pointer to start of texture map data

; set edi = address of first pixel to modify
	mov	edi,[_fx_y]
;        mov     es,[_pixel_data_selector]       ; selector[0*2]

	mov	edi,[_y_pointers+edi*4]

	mov	ebx,[_fx_xleft]
	test	ebx, ebx
	jns	ebx_ok
	xor	ebx, ebx
ebx_ok:	add	edi,[_write_buffer]
	add	edi,ebx

; set _loop_count = # of iterations
	mov	eax,[_fx_xright]
	sub	eax,ebx
	js	near _none_to_do
	mov	[_loop_count],eax

; lighting values are passed in fixed point, but need to be in 8 bit integer, 8 bit fraction so we can easily
; get the integer by reading %bh
	sar	dword [_fx_l], 8
	sar	dword [_fx_dl_dx],8
	jns	dl_dx_ok
	inc	dword [_fx_dl_dx]	; round towards 0 for negative deltas
dl_dx_ok:

; set initial values
	mov	ebx,[_fx_u]
	mov	ebp,[_fx_v]
	mov	ecx,[_fx_z]

	test	dword [_per2_flag],-1
	je	tmap_loop

	test	dword [_Lighting_on], -1
        je     near _tmap_loop_fast_nolight
        jmp     _tmap_loop_fast
;tmap_loop_fast_nolight_jumper:
;    jmp tmap_loop_fast_nolight

;================ PERSPECTIVE TEXTURE MAP INNER LOOPS ========================
;
; Usage in loop:	eax	division, pixel value
;	ebx	u
;	ecx	z
;	edx	division
;	ebp	v
;	esi	source pixel pointer
;	edi	destination pixel pointer

;-------------------- NORMAL PERSPECTIVE TEXTURE MAP LOOP -----------------
tmap_loop:
	mov	esi, ebx	; esi becomes u coordinate

	align	4
tmap_loop0:

; compute v coordinate
	mov	eax, ebp	; get v
	mov	edx, eax
	sar	edx, 31
	idiv	ecx	; eax = (v/z)

	and	eax,3fh	; mask with height-1
	mov	ebx,eax

; compute u coordinate
	mov	eax, esi	; get u
	mov	edx, eax
	sar	edx, 31
	idiv	ecx	; eax = (u/z)

	shl 	eax,26
	shld 	ebx,eax,6	; esi = v*64+u

; read 1 pixel
        add     ebx, [_pixptr]
	xor	eax, eax
	test	dword [_Lighting_on], -1
        mov     al, [ebx]    ; get pixel from source bitmap
	je	NoLight1

; LIGHTING CODE
	mov	ebx, [_fx_l]	; get temp copy of lighting value
	mov	ah, bh	; get lighting level
	add	ebx, [_fx_dl_dx]	; update lighting value
	mov	al, [_gr_fade_table+eax]	; xlat pixel thru lighting tables
	mov	[_fx_l], ebx	; save temp copy of lighting value

; transparency check
NoLight1:	cmp	al,255
	je	skip1

	mov	[edi],al
skip1:	inc	edi

; update deltas
	add	ebp,[_fx_dv_dx]
	add	esi,[_fx_du_dx]
	add	ecx,[_fx_dz_dx]
	je	_div_0_abort	; would be dividing by 0, so abort

	dec	dword [_loop_count]
	jns	tmap_loop0

_none_to_do:
	popa
;        pop     es
	ret

; We detected a z=0 condition, which seems pretty bogus, don't you think?
; So, we abort, but maybe we want to know about it.
_div_0_abort:
	jmp	_none_to_do

;-------------------------- PER/4 TMAPPER ----------------
;
;	x = x1
;	U0 = u/w; V0 = v/w;
;	while ( 1 )
;		u += du_dx*4; v+= dv_dx*4
;		U1 = u/w; V1 = v/w;
;		DUDX = (U1-U0)/4; DVDX = (V1-V0)/4;
;
;	; Pixel 0
;		pixels = texmap[V0*64+U0];
;		U0 += DUDX; V0 += DVDX
;	; Pixel 1
;		pixels = (pixels<<8)+texmap[V0*64+U0];
;		U0 += DUDX; V0 += DVDX
;	; Pixel 2
;		pixels = (pixels<<8)+texmap[V0*64+U0];
;		U0 += DUDX; V0 += DVDX
;	; Pixel 3
;		pixels = (pixels<<8)+texmap[V0*64+U0];
;
;		screen[x] = pixel
;		x += 4;
;		U0 = U1; V0 = V1

NBITS equ 4	; 2^NBITS pixels plotted per divide
ZSHIFT equ 4	; precision used in PDIV macro


;PDIV MACRO
; Returns EAX/ECX in 16.16 format in EAX. Trashes EDX
;          sig bits   6.3
;	mov	edx,eax
;	shl	eax,ZSHIFT
;	sar	edx,32-ZSHIFT
;	idiv	ecx	; eax = (v/z)
;   shl	eax, 16-ZSHIFT
;ENDM

global _tmap_loop_fast

; -------------------------------------- Start of Getting Dword Aligned ----------------------------------------------
;	ebx	fx_u

_tmap_loop_fast:
	mov	esi,ebx

	align	4
NotDwordAligned1:
	test	edi, 11b
	jz	DwordAligned1

; compute v coordinate
	mov	eax, ebp	; get v
	mov	edx, eax
	sar	edx, 31
	idiv	ecx	; eax = (v/z)

	and	eax,3fh	; mask with height-1
	mov	ebx,eax

; compute u coordinate
	mov	eax, esi	; get u
	mov	edx, eax
	sar	edx, 31
	idiv	ecx	; eax = (u/z)

	shl 	eax,26
	shld 	ebx,eax,6	; esi = v*64+u

; read 1  pixel
        add     ebx,[_pixptr]
	xor	eax, eax
        mov     al, [ebx]    ; get pixel from source bitmap

; lighting code
	mov	ebx, [_fx_l]	; get temp copy of lighting value
	mov	ah, bh	; get lighting level
	add	ebx, [_fx_dl_dx]	; update lighting value
	mov	[_fx_l], ebx	; save temp copy of lighting value

; transparency check
	cmp	al,255
	je	skip2	; this pixel is transparent, so don't write it (or light it)

	mov	al, [_gr_fade_table+eax]	; xlat pixel thru lighting tables

; write 1 pixel
	mov	[edi],al
skip2:	inc	edi

; update deltas
	add	ebp,[_fx_dv_dx]
	add	esi,[_fx_du_dx]
	add	ecx,[_fx_dz_dx]
	je	_div_0_abort	; would be dividing by 0, so abort

	dec	dword [_loop_count]
	jns	NotDwordAligned1

	jmp	_none_to_do

; -------------------------------------- End of Getting Dword Aligned ----------------------------------------------

DwordAligned1:

	mov	eax, [_loop_count]
	mov	ebx, esi	; get fx_u [pentium pipelining]
	inc	eax
	mov	esi, eax
	and	esi, (1 << NBITS) - 1
	sar	eax, NBITS
	mov	[num_left_over], esi
	je	near tmap_loop	; there are no 2^NBITS chunks, do divide/pixel for whole scanline
	mov	[_loop_count], eax	; _loop_count = pixels / NPIXS

; compute initial v coordinate
	mov	eax,ebp	; get v
	mov	edx,ebp
	shl	eax,ZSHIFT
	sar	edx,32-ZSHIFT
	idiv	ecx	; eax = (v/z)
	shl	eax, 16-ZSHIFT
	mov	[V0], eax

; compute initial u coordinate
	mov	eax,ebx	; get u
	mov	edx,ebx
	shl	eax,ZSHIFT
	sar	edx,32-ZSHIFT
	idiv	ecx	; eax = (v/z)
	shl	eax, 16-ZSHIFT
	mov	[U0], eax

; Set deltas to NPIXS pixel increments
	mov	eax, [_fx_du_dx]
	shl	eax, NBITS
	mov	[DU1], eax
	mov	eax, [_fx_dv_dx]
	shl	eax, NBITS
	mov	[DV1], eax
	mov	eax, [_fx_dz_dx]
	shl	eax, NBITS
	mov	[DZ1], eax

	align	4
TopOfLoop4:
	add	ebx, [DU1]
	add	ebp, [DV1]
	add	ecx, [DZ1]
	je	near _div_0_abort	; would be dividing by 0, so abort

; Done with ebx, ebp, ecx until next iteration
	push	ebx
	push	ecx
	push	ebp
	push	edi

; Find fixed U1
	mov	eax, ebx
	mov	edx,ebx
	shl	eax,ZSHIFT
	sar	edx,32-ZSHIFT
	idiv	ecx	; eax = (v/z)
	shl	eax, 16-ZSHIFT
	mov	ebx, eax	; ebx = U1 until pop's

; Find fixed V1
	mov	eax, ebp
	mov	edx, ebp
	shl	eax,ZSHIFT
	sar	edx,32-ZSHIFT
	idiv	ecx	; eax = (v/z)

	mov	ecx, [U0]	; ecx = U0 until pop's
	mov	edi, [V0]	; edi = V0 until pop's

	shl	eax, 16-ZSHIFT
	mov	ebp, eax	; ebp = V1 until pop's

; Make ESI =  V0:U0 in 6:10,6:10 format
	mov	eax, ecx
	shr	eax, 6
	mov	esi, edi
	shl	esi, 10
	mov	si, ax

; Make EDX = DV:DU in 6:10,6:10 format
	mov	eax, ebx
	sub	eax, ecx
	sar	eax, NBITS+6
	mov	edx, ebp
	sub	edx, edi
	shl	edx, 10-NBITS	; EDX = V1-V0/ 4 in 6:10 int:frac
	mov	dx, ax	; put delta u in low word

; Save the U1 and V1 so we don't have to divide on the next iteration
	mov	[U0], ebx
	mov	[V0], ebp

	pop	edi	; Restore EDI before using it

; LIGHTING CODE
	mov	ebx, [_fx_l]
	mov	ebp, [_fx_dl_dx]

	test	dword [_Transparency_on],-1
	je	near no_trans1

%macro repproc1 0
	mov	eax, esi	; get u,v
	shr	eax, 26	; shift out all but int(v)
	shld	ax,si,6	; shift in u, shifting up v
	add	esi, edx	; inc u,v
        add     eax, [_pixptr]
        movzx   eax, byte [eax]    ; get pixel from source bitmap
	cmp	al,255
	je	%%skipa1
	mov	ah, bh	; form lighting table lookup value
	add	ebx, ebp	; update lighting value
	mov	al, [_gr_fade_table+eax]	; xlat thru lighting table into dest buffer
	mov	[edi],al
%%skipa1:
	inc	edi

; Do odd pixel
	mov	eax, esi	; get u,v
	shr	eax, 26	; shift out all but int(v)
	shld	ax,si,6	; shift in u, shifting up v
	add	esi, edx	; inc u,v
        add     eax,[_pixptr]
        movzx   eax, byte [eax]    ; get pixel from source bitmap
	cmp	al,255
	je	%%skipa2
	mov	ah, bh	; form lighting table lookup value
	add	ebx, ebp	; update lighting value
	mov	al, [_gr_fade_table+eax]	; xlat thru lighting table into dest buffer
	mov	[edi],al
%%skipa2:
	inc	edi
%endmacro


%rep (2 << (NBITS-2))
;	local	skip3,no_trans1
;	local	skipa1,skipa2
    repproc1
%endrep

jmp	cont1

; -------------------------------------------------------
no_trans1:

%macro repproc2 0
	mov	eax, esi	; get u,v
	shr	eax, 26	; shift out all but int(v)
	shld	ax,si,6	; shift in u, shifting up v
	add	esi, edx	; inc u,v
        add     eax,[_pixptr]
        movzx   eax, byte [eax]    ; get pixel from source bitmap
	mov	ah, bh	; form lighting table lookup value
	add	ebx, ebp	; update lighting value
	mov	cl, [_gr_fade_table+eax]	; xlat thru lighting table into dest buffer

; Do odd pixel
	mov	eax, esi	; get u,v
	shr	eax, 26	; shift out all but int(v)
	shld	ax,si,6	; shift in u, shifting up v
	add	esi, edx	; inc u,v
        add     eax,[_pixptr]
        movzx   eax, byte [eax]    ; get pixel from source bitmap
	mov	ah, bh	; form lighting table lookup value
	add	ebx, ebp	; update lighting value
	mov	ch, [_gr_fade_table+eax]	; xlat thru lighting table into dest buffer

; ----- This is about 1% faster than the above, and could probably be optimized more.
; ----- Problem is, it gets the u,v coordinates backwards.  What you would need to do
; ----- is switch the packing of the u,v coordinates above (about 95 lines up).
;----------;	mov	eax, esi
;----------;	shr	ax, 10
;----------;	rol	eax, 6
;----------;	mov	dx, ax
;----------;	add	esi, mem_edx
;----------;	mov	dl, es:[edx]
;----------;	mov	dh, bh
;----------;	add	ebx, ebp
;----------;	mov	cl, _gr_fade_table[edx]
;----------;
;----------;	mov	eax, esi
;----------;	shr	ax, 10
;----------;	rol	eax, 6
;----------;	mov	dx, ax
;----------;	add	esi, mem_edx
;----------;	mov	dl, es:[edx]
;----------;	mov	dh, bh
;----------;	add	ebx, ebp
;----------;	mov	ch, _gr_fade_table[edx]

	ror	ecx, 16	; move to next double dest pixel position
%endmacro

%rep (1 << (NBITS-2))

    repproc2
    repproc2

	mov 	[edi],ecx	; Draw 4 pixels to display
	add 	edi,4
%endrep
;; pop edx
cont1:

; -------------------------------------------------------

; LIGHTING CODE
	mov	[_fx_l], ebx
	pop	ebp
	pop	ecx
	pop	ebx
	dec	dword [_loop_count]
	jnz	near TopOfLoop4

EndOfLoop4:
	test	dword [num_left_over], -1
	je	near _none_to_do

; ----------------------------------------- Start of LeftOver Pixels ------------------------------------------
DoEndPixels:
	push	ecx

	mov	eax, ecx
	lea	eax, [eax*2+eax]

	add	ecx, [DZ1]
	js	notokhere
	shl	ecx,2
	cmp	eax, ecx
	pop	ecx
	jl	okhere
	jmp	bah_bah
notokhere:
	pop	ecx
bah_bah:
        test    dword [_new_end],-1
	jne	near NewDoEndPixels
okhere:

	add	ebx, [DU1]
	add	ebp, [DV1]
	add	ecx, [DZ1]
	je	near _div_0_abort
	jns	dep_cont

; z went negative.
; this can happen because we added DZ1 to the current z, but dz1 represents dz for perhaps 16 pixels
; though we might only plot one more pixel.
	mov	cl, 1

dep_loop:	mov	eax, [DU1]
	sar	eax, cl
	sub	ebx, eax

	mov	eax, [DV1]
	sar	eax, cl
	sub	ebp, eax

	mov	eax, [DZ1]
	sar	eax, cl
	sub	ecx, eax
	je	near _div_0_abort
	jns	dep_cont

	inc	cl
	cmp	cl, NBITS
	jne	dep_loop

dep_cont:
	push	edi	; use edi as a temporary variable

	cmp	ecx,1 << (ZSHIFT+1)
	jg	ecx_ok
	mov	ecx, 1 << (ZSHIFT+1)
ecx_ok:

; Find fixed U1
	mov	eax, ebx
	;PDIV
	mov	edx,eax
	shl	eax,ZSHIFT
	sar	edx,32-ZSHIFT
	idiv	ecx	; eax = (v/z)
	shl	eax, 16-ZSHIFT

	mov	ebx, eax	; ebx = U1 until pop's

; Find fixed V1
	mov	eax, ebp
	;PDIV
	mov	edx,eax
	shl	eax,ZSHIFT
	sar	edx,32-ZSHIFT
	idiv	ecx	; eax = (v/z)
	shl	eax, 16-ZSHIFT

	mov	ebp, eax	; ebp = V1 until pop's

	mov	ecx, [U0]	; ecx = U0 until pop's
	mov	edi, [V0]	; edi = V0 until pop's

; Make ESI =  V0:U0 in 6:10,6:10 format
	mov	eax, ecx
	shr	eax, 6
	mov	esi, edi
	shl	esi, 10
	mov	si, ax

; Make EDX = DV:DU in 6:10,6:10 format
	mov	eax, ebx
	sub	eax, ecx
	sar	eax, NBITS+6
	mov	edx, ebp
	sub	edx, edi
	shl	edx, 10-NBITS	; EDX = V1-V0/ 4 in 6:10 int:frac
	mov	dx, ax	; put delta u in low word

	pop	edi	; Restore EDI before using it

	mov	ecx, [num_left_over]

; LIGHTING CODE
	mov	ebx, [_fx_l]
	mov	ebp, [_fx_dl_dx]

    ITERATION equ 0

%macro repproc3 0
; Do even pixel
	mov	eax, esi	; get u,v
	shr	eax, 26	; shift out all but int(v)
	shld	ax,si,6	; shift in u, shifting up v
        add     eax,[_pixptr]
        movzx   eax, byte [eax]    ; get pixel from source bitmap
	add	esi, edx	; inc u,v
	mov	ah, bh	; form lighting table lookup value
	add	ebx, ebp	; update lighting value
	cmp	al,255
	je	%%skip4
	mov	al, [_gr_fade_table+eax]	; xlat thru lighting table into dest buffer
	mov	[edi+ITERATION], al	; write pixel
%%skip4:	dec	ecx
	jz	near _none_to_do

; Do odd pixel
	mov	eax, esi	; get u,v
	shr	eax, 26	; shift out all but int(v)
        shld    ax,si,6 ; shift in u, shifting up v
        add     eax,[_pixptr]
        movzx   eax, byte [eax]    ; get pixel from source bitmap
	add	esi, edx	; inc u,v
	mov	ah, bh	; form lighting table lookup value
	add	ebx, [_fx_dl_dx]	; update lighting value
	cmp	al,255
	je	%%skip5
	mov	al, [_gr_fade_table+eax]	; xlat thru lighting table into dest buffer
	mov	[edi+ITERATION+1], al	; write pixel
%%skip5:	dec	ecx
	jz	near _none_to_do
%endmacro

%rep (1 << (NBITS-1))
	;local	skip4, skip5
    repproc3
%assign ITERATION  ITERATION + 2

%endrep

; Should never get here!!!!
	int	3
	jmp	_none_to_do

; ----------------------------------------- End of LeftOver Pixels ------------------------------------------

; --BUGGY NEW--NewDoEndPixels:
; --BUGGY NEW--	mov	eax, num_left_over
; --BUGGY NEW--	and	num_left_over, 3
; --BUGGY NEW--	shr	eax, 2
; --BUGGY NEW--	je	NDEP_1
; --BUGGY NEW-- mov	_loop_count, eax
; --BUGGY NEW--
; --BUGGY NEW--; do 4 pixels per hunk, not 16, so div deltas by 4 (16/4=4)
; --BUGGY NEW-- shr DU1,2
; --BUGGY NEW-- shr DV1,2
; --BUGGY NEW-- shr DZ1,2
; --BUGGY NEW--
; --BUGGY NEW--NDEP_TopOfLoop4:
; --BUGGY NEW--	add	ebx, DU1
; --BUGGY NEW--	add	ebp, DV1
; --BUGGY NEW--	add	ecx, DZ1
; --BUGGY NEW--	je	_div_0_abort	; would be dividing by 0, so abort
; --BUGGY NEW--
; --BUGGY NEW--; Done with ebx, ebp, ecx until next iteration
; --BUGGY NEW--	push	ebx
; --BUGGY NEW--	push	ecx
; --BUGGY NEW--	push	ebp
; --BUGGY NEW--	push	edi
; --BUGGY NEW--
; --BUGGY NEW--; Find fixed U1
; --BUGGY NEW--	mov	eax, ebx
; --BUGGY NEW--	mov	edx,ebx
; --BUGGY NEW--	shl	eax,(ZSHIFT-2)
; --BUGGY NEW--	sar	edx,32-(ZSHIFT-2)
; --BUGGY NEW--	idiv	ecx	; eax = (v/z)
; --BUGGY NEW--	shl	eax, 16-(ZSHIFT-2)
; --BUGGY NEW--	mov	ebx, eax	; ebx = U1 until pop's
; --BUGGY NEW--
; --BUGGY NEW--; Find fixed V1
; --BUGGY NEW--	mov	eax, ebp
; --BUGGY NEW--	mov	edx, ebp
; --BUGGY NEW--	shl	eax,(ZSHIFT-2)
; --BUGGY NEW--	sar	edx,32-(ZSHIFT-2)
; --BUGGY NEW--	idiv	ecx	; eax = (v/z)
; --BUGGY NEW--
; --BUGGY NEW--	mov	ecx, U0	; ecx = U0 until pop's
; --BUGGY NEW--	mov	edi, V0	; edi = V0 until pop's
; --BUGGY NEW--
; --BUGGY NEW--	shl	eax, 16-(ZSHIFT-2)
; --BUGGY NEW--	mov	ebp, eax	; ebp = V1 until pop's
; --BUGGY NEW--
; --BUGGY NEW--; Make ESI =  V0:U0 in 6:10,6:10 format
; --BUGGY NEW--	mov	eax, ecx
; --BUGGY NEW--	shr	eax, 6
; --BUGGY NEW--	mov	esi, edi
; --BUGGY NEW--	shl	esi, 10
; --BUGGY NEW--	mov	si, ax
; --BUGGY NEW--
; --BUGGY NEW--; Make EDX = DV:DU in 6:10,6:10 format
; --BUGGY NEW--	mov	eax, ebx
; --BUGGY NEW--	sub	eax, ecx
; --BUGGY NEW--	sar	eax, (NBITS-2)+6
; --BUGGY NEW--	mov	edx, ebp
; --BUGGY NEW--	sub	edx, edi
; --BUGGY NEW--	shl	edx, 10-(NBITS-2)	; EDX = V1-V0/ 4 in 6:10 int:frac
; --BUGGY NEW--	mov	dx, ax	; put delta u in low word
; --BUGGY NEW--
; --BUGGY NEW--; Save the U1 and V1 so we don't have to divide on the next iteration
; --BUGGY NEW--	mov	U0, ebx
; --BUGGY NEW--	mov	V0, ebp
; --BUGGY NEW--
; --BUGGY NEW--	pop	edi	; Restore EDI before using it
; --BUGGY NEW--
; --BUGGY NEW--; LIGHTING CODE
; --BUGGY NEW--	mov	ebx, _fx_l
; --BUGGY NEW--	mov	ebp, _fx_dl_dx
; --BUGGY NEW--
; --BUGGY NEW--;**	test	_Transparency_on,-1
; --BUGGY NEW--;**	je	NDEP_no_trans1
; --BUGGY NEW--
; --BUGGY NEW--        REPT 2
; --BUGGY NEW--	local	NDEP_skipa1, NDEP_skipa2
; --BUGGY NEW--
; --BUGGY NEW--	mov	eax, esi	; get u,v
; --BUGGY NEW--	shr	eax, 26	; shift out all but int(v)
; --BUGGY NEW--	shld	ax,si,6	; shift in u, shifting up v
; --BUGGY NEW--	add	esi, edx	; inc u,v
; --BUGGY NEW--	mov 	al, es:[eax]	; get pixel from source bitmap
; --BUGGY NEW--	cmp	al,255
; --BUGGY NEW--	je	NDEP_skipa1
; --BUGGY NEW--	mov	ah, bh	; form lighting table lookup value
; --BUGGY NEW--	add	ebx, ebp	; update lighting value
; --BUGGY NEW--	mov	al, _gr_fade_table[eax]	; xlat thru lighting table into dest buffer
; --BUGGY NEW--	mov	[edi],al
; --BUGGY NEW--NDEP_skipa1:
; --BUGGY NEW--	inc	edi
; --BUGGY NEW--
; --BUGGY NEW--; Do odd pixel
; --BUGGY NEW--	mov	eax, esi	; get u,v
; --BUGGY NEW--	shr	eax, 26	; shift out all but int(v)
; --BUGGY NEW--	shld	ax,si,6	; shift in u, shifting up v
; --BUGGY NEW--	add	esi, edx	; inc u,v
; --BUGGY NEW--	mov 	al, es:[eax]	; get pixel from source bitmap
; --BUGGY NEW--	cmp	al,255
; --BUGGY NEW--	je	NDEP_skipa2
; --BUGGY NEW--	mov	ah, bh	; form lighting table lookup value
; --BUGGY NEW--	add	ebx, ebp	; update lighting value
; --BUGGY NEW--	mov	al, _gr_fade_table[eax]	; xlat thru lighting table into dest buffer
; --BUGGY NEW--	mov	[edi],al
; --BUGGY NEW--NDEP_skipa2:
; --BUGGY NEW--	inc	edi
; --BUGGY NEW--
; --BUGGY NEW--        ENDM
; --BUGGY NEW--
; --BUGGY NEW--	mov	_fx_l, ebx
; --BUGGY NEW--	pop	ebp
; --BUGGY NEW--	pop	ecx
; --BUGGY NEW--	pop	ebx
; --BUGGY NEW-- dec	_loop_count
; --BUGGY NEW--	jnz	NDEP_TopOfLoop4
; --BUGGY NEW--
; --BUGGY NEW--	test	num_left_over, -1
; --BUGGY NEW--	je	_none_to_do
; --BUGGY NEW--
; --BUGGY NEW--NDEP_1:
; --BUGGY NEW--	mov	esi,ebx
; --BUGGY NEW--
; --BUGGY NEW--	align	4
; --BUGGY NEW--NDEP_loop:
; --BUGGY NEW--
; --BUGGY NEW--; compute v coordinate
; --BUGGY NEW--	mov	eax, ebp	; get v
; --BUGGY NEW--	mov	edx, eax
; --BUGGY NEW--	sar	edx, 31
; --BUGGY NEW--	idiv	ecx	; eax = (v/z)
; --BUGGY NEW--
; --BUGGY NEW--	and	eax,3fh	; mask with height-1
; --BUGGY NEW--	mov	ebx,eax
; --BUGGY NEW--
; --BUGGY NEW--; compute u coordinate
; --BUGGY NEW--	mov	eax, 	esi	; get u
; --BUGGY NEW--	mov	edx, eax
; --BUGGY NEW--	sar	edx, 31
; --BUGGY NEW--	idiv	ecx	; eax = (u/z)
; --BUGGY NEW--
; --BUGGY NEW--	shl 	eax,26
; --BUGGY NEW--	shld 	ebx,eax,6	; esi = v*64+u
; --BUGGY NEW--
; --BUGGY NEW--; read 1  pixel
; --BUGGY NEW--	xor	eax, eax
; --BUGGY NEW--	mov	al, es:[ebx]	; get pixel from source bitmap
; --BUGGY NEW--
; --BUGGY NEW--; lighting code
; --BUGGY NEW--	mov	ebx, _fx_l	; get temp copy of lighting value
; --BUGGY NEW--	mov	ah, bh	; get lighting level
; --BUGGY NEW--	add	ebx, _fx_dl_dx	; update lighting value
; --BUGGY NEW--	mov	_fx_l, ebx	; save temp copy of lighting value
; --BUGGY NEW--
; --BUGGY NEW--; transparency check
; --BUGGY NEW--	cmp	al,255
; --BUGGY NEW--	je	NDEP_skip2	; this pixel is transparent, so don't write it (or light it)
; --BUGGY NEW--
; --BUGGY NEW--	mov	al, _gr_fade_table[eax]	; xlat pixel thru lighting tables
; --BUGGY NEW--
; --BUGGY NEW--; write 1 pixel
; --BUGGY NEW--	mov	[edi],al
; --BUGGY NEW--NDEP_skip2:	inc	edi
; --BUGGY NEW--
; --BUGGY NEW--; update deltas
; --BUGGY NEW--	add	ebp,_fx_dv_dx
; --BUGGY NEW--	add	esi,_fx_du_dx
; --BUGGY NEW--	add	ecx,_fx_dz_dx
; --BUGGY NEW--	je	_div_0_abort	; would be dividing by 0, so abort
; --BUGGY NEW--
; --BUGGY NEW--	dec	num_left_over
; --BUGGY NEW--	jne	NDEP_loop
; --BUGGY NEW--
; --BUGGY NEW--	jmp	_none_to_do

NewDoEndPixels:
	mov	esi,ebx

	align	4
NDEP_loop:

; compute v coordinate
	mov	eax, ebp	; get v
	mov	edx, eax
	sar	edx, 31
	idiv	ecx	; eax = (v/z)

	and	eax,3fh	; mask with height-1
	mov	ebx,eax

; compute u coordinate
	mov	eax, 	esi	; get u
	mov	edx, eax
	sar	edx, 31
	idiv	ecx	; eax = (u/z)

	shl 	eax,26
	shld 	ebx,eax,6	; esi = v*64+u

; read 1  pixel
        add     ebx,[_pixptr]
	xor	eax, eax
        mov     al, [ebx]    ; get pixel from source bitmap

; lighting code
	mov	ebx, [_fx_l]	; get temp copy of lighting value
	mov	ah, bh	; get lighting level
	add	ebx, [_fx_dl_dx]	; update lighting value
	mov	[_fx_l], ebx	; save temp copy of lighting value

; transparency check
	cmp	al,255
	je	NDEP_skip2	; this pixel is transparent, so don't write it (or light it)

	mov	al, [_gr_fade_table+eax]	; xlat pixel thru lighting tables

; write 1 pixel
	mov	[edi],al
NDEP_skip2:	inc	edi

; update deltas
	add	ebp,[_fx_dv_dx]
	add	esi,[_fx_du_dx]
	add	ecx,[_fx_dz_dx]
        je      near _div_0_abort    ; would be dividing by 0, so abort

	dec	dword [num_left_over]
	jne	NDEP_loop

	jmp	_none_to_do

; ==================================================== No Lighting Code ======================================================
global _tmap_loop_fast_nolight
_tmap_loop_fast_nolight:
	mov	esi,ebx

	align	4
NotDwordAligned1_nolight:
        test    edi, 11b
        jz      DwordAligned1_nolight

; compute v coordinate
	mov	eax,ebp	; get v
	mov	edx, eax
	sar	edx, 31
	idiv	ecx	; eax = (v/z)

	and	eax,3fh	; mask with height-1
	mov	ebx,eax

; compute u coordinate
	mov	eax, esi	; get u
	mov	edx, eax
	sar	edx, 31
	idiv	ecx	; eax = (u/z)

	shl 	eax,26
	shld 	ebx,eax,6	; esi = v*64+u

; read 1  pixel
        add     ebx,[_pixptr]
        mov     al,[ebx]     ; get pixel from source bitmap

; write 1 pixel
	cmp	al,255
	je	skip6
	mov	[edi],al
skip6:	inc	edi

; update deltas
	add	ebp,[_fx_dv_dx]
	add	esi,[_fx_du_dx]
	add	ecx,[_fx_dz_dx]
        je      near _div_0_abort    ; would be dividing by 0, so abort

	dec	dword [_loop_count]
        jns     NotDwordAligned1_nolight
	jmp	_none_to_do

DwordAligned1_nolight:
	mov	ebx,esi

	mov	eax, [_loop_count]
	inc	eax
	mov	[num_left_over], eax
	shr	eax, NBITS

	test	eax, -1
        je      near tmap_loop       ; no 2^NBITS chunks, do divide/pixel for whole scanline

	mov	[_loop_count], eax	; _loop_count = pixels / NPIXS
	shl	eax, NBITS
	sub	[num_left_over], eax	; num_left_over = obvious

; compute initial v coordinate
	mov	eax,ebp	; get v
	;PDIV
	mov	edx,eax
	shl	eax,ZSHIFT
	sar	edx,32-ZSHIFT
	idiv	ecx	; eax = (v/z)
	shl	eax, 16-ZSHIFT

	mov	[V0], eax

; compute initial u coordinate
	mov	eax,ebx	; get u
	;PDIV
	mov	edx,eax
	shl	eax,ZSHIFT
	sar	edx,32-ZSHIFT
	idiv	ecx	; eax = (v/z)
	shl	eax, 16-ZSHIFT

	mov	[U0], eax

; Set deltas to NPIXS pixel increments
	mov	eax, [_fx_du_dx]
	shl	eax, NBITS
	mov	[DU1], eax
	mov	eax, [_fx_dv_dx]
	shl	eax, NBITS
	mov	[DV1], eax
	mov	eax, [_fx_dz_dx]
	shl	eax, NBITS
	mov	[DZ1], eax

	align	4
TopOfLoop4_nolight:
	add	ebx, [DU1]
	add	ebp, [DV1]
	add	ecx, [DZ1]
        je      near _div_0_abort

; Done with ebx, ebp, ecx until next iteration
	push	ebx
	push	ecx
	push	ebp
	push	edi

; Find fixed U1
	mov	eax, ebx
	;PDIV
	mov	edx,eax
	shl	eax,ZSHIFT
	sar	edx,32-ZSHIFT
	idiv	ecx	; eax = (v/z)
	shl	eax, 16-ZSHIFT

	mov	ebx, eax	; ebx = U1 until pop's

; Find fixed V1
	mov	eax, ebp
	;PDIV
	mov	edx,eax
	shl	eax,ZSHIFT
	sar	edx,32-ZSHIFT
	idiv	ecx	; eax = (v/z)
	shl	eax, 16-ZSHIFT

	mov	ebp, eax	; ebp = V1 until pop's

	mov	ecx, [U0]	; ecx = U0 until pop's
	mov	edi, [V0]	; edi = V0 until pop's

; Make ESI =  V0:U0 in 6:10,6:10 format
	mov	eax, ecx
	shr	eax, 6
	mov	esi, edi
	shl	esi, 10
	mov	si, ax

; Make EDX = DV:DU in 6:10,6:10 format
	mov	eax, ebx
	sub	eax, ecx
	sar	eax, NBITS+6
	mov	edx, ebp
	sub	edx, edi
	shl	edx, 10-NBITS	; EDX = V1-V0/ 4 in 6:10 int:frac
	mov	dx, ax	; put delta u in low word

; Save the U1 and V1 so we don't have to divide on the next iteration
	mov	[U0], ebx
	mov	[V0], ebp

	pop	edi	; Restore EDI before using it

%macro repproc4 0
; Do 1 pixel
	mov	eax, esi	; get u,v
	shr	eax, 26	; shift out all but int(v)
	shld	ax,si,6	; shift in u, shifting up v
	add	esi, edx	; inc u,v
        add     eax,[_pixptr]
        mov     cl, [eax]    ; load into buffer register

	mov	eax, esi	; get u,v
	shr	eax, 26	; shift out all but int(v)
	shld	ax,si,6	; shift in u, shifting up v
        add     eax,[_pixptr]
        mov     ch, [eax]    ; load into buffer register
	add	esi, edx	; inc u,v
	ror	ecx, 16	; move to next dest pixel

	mov	eax, esi	; get u,v
	shr	eax, 26	; shift out all but int(v)
	shld	ax,si,6	; shift in u, shifting up v
        add     eax,[_pixptr]
        mov     cl, [eax]    ; load into buffer register
	add	esi, edx	; inc u,v

	mov	eax, esi	; get u,v
	shr	eax, 26	; shift out all but int(v)
	shld	ax,si,6	; shift in u, shifting up v
        add     eax,[_pixptr]
        mov     ch, [eax]    ; load into buffer register
	add	esi, edx	; inc u,v
	ror	ecx, 16 ;-- can get rid of this, just write in different order below -- 	; move to next dest pixel

	test	dword [_Transparency_on],-1
	je	%%no_trans2
	cmp	ecx,-1
	je	%%skip7

	cmp	cl,255
	je	%%skip1q
	mov	[edi],cl
%%skip1q:

	cmp	ch,255
	je	%%skip2q
	mov	[edi+1],ch
%%skip2q:
	ror	ecx,16

	cmp	cl,255
        je      %%skip3q
	mov	[edi+2],cl
%%skip3q:


	cmp	ch,255
	je	%%skip4q
	mov	[edi+3],ch
%%skip4q:

	jmp	%%skip7
%%no_trans2:
	mov 	[edi],ecx	; Draw 4 pixels to display
%%skip7:	add 	edi,4
%endmacro

%rep (1 << (NBITS-2))
	;local	skip7, no_trans2, skip1q, skip2q, skip3q, skip4q
    repproc4

%endrep

	pop	ebp
	pop	ecx
	pop	ebx
	dec	dword [_loop_count]
        jnz     near TopOfLoop4_nolight

EndOfLoop4_nolight:

	test	dword [num_left_over], -1
        je      near _none_to_do

DoEndPixels_nolight:
	add	ebx, [DU1]
	add	ebp, [DV1]
	add	ecx, [DZ1]
        je      near _div_0_abort
	push	edi	; use edi as a temporary variable

; Find fixed U1
	mov	eax, ebx
	mov	edx,eax
	shl	eax,ZSHIFT
	sar	edx,32-ZSHIFT
	idiv	ecx	; eax = (v/z)
	shl	eax, 16-ZSHIFT
	mov	ebx, eax	; ebx = U1 until pop's

; Find fixed V1
	mov	eax, ebp
	mov	edx,eax
	shl	eax,ZSHIFT
	sar	edx,32-ZSHIFT
	idiv	ecx	; eax = (v/z)
	shl	eax, 16-ZSHIFT
	mov	ebp, eax	; ebp = V1 until pop's

	mov	ecx, [U0]	; ecx = U0 until pop's
	mov	edi, [V0]	; edi = V0 until pop's

; Make ESI =  V0:U0 in 6:10,6:10 format
	mov	eax, ecx
	shr	eax, 6
	mov	esi, edi
	shl	esi, 10
	mov	si, ax

; Make EDX = DV:DU in 6:10,6:10 format
	mov	eax, ebx
	sub	eax, ecx
	sar	eax, NBITS+6
	mov	edx, ebp
	sub	edx, edi
	shl	edx, 10-NBITS	; EDX = V1-V0/ 4 in 6:10 int:frac
	mov	dx, ax	; put delta u in low word

	pop	edi	; Restore EDI before using it

	mov	ecx, [num_left_over]

%assign ITERATION 0
%macro repproc5 0
; Do 1 pixel
	mov	eax, esi	; get u,v
	shr	eax, 26	; shift out all but int(v)
	shld	ax,si,6	; shift in u, shifting up v
        add     eax,[_pixptr]
        movzx   eax, byte [eax]    ; load into buffer register
	add	esi, edx	; inc u,v
	cmp	al,255
	je	%%skip8
	mov	[edi+ITERATION], al	; write pixel
%%skip8:	dec	ecx
        jz      near _none_to_do
%endmacro

%rep (1 << NBITS)
	;local	skip8
	repproc5
%assign ITERATION  ITERATION + 1
%endrep

; Should never get here!!!!!
	int	3
	jmp	_none_to_do

