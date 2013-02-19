; $Id: tmap_ll.asm,v 1.1.1.1 2006/03/17 20:00:04 zicodxx Exp $
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
; Linear, lighted texture mapper inner loop.
;
;

[BITS 32]

global  _asm_tmap_scanline_lin_lighted
global  asm_tmap_scanline_lin_lighted

[SECTION .data]

%include        "tmap_inc.asm"

ALLOW_DITHERING	equ	1

_fx_dl_dx1	dd 0
_fx_dl_dx2	dd 0

_loop_count	dd 0


[SECTION .text]

; --------------------------------------------------------------------------------------------------
; Enter:
;	_xleft	fixed point left x coordinate
;	_xright	fixed point right x coordinate
;	_y	fixed point y coordinate
;	_pixptr	address of source pixel map
;	_u	fixed point initial u coordinate
;	_v	fixed point initial v coordinate
;	_du_dx	fixed point du/dx
;	_dv_dx	fixed point dv/dx

;   for (x = (int) xleft; x <= (int) xright; x++) {
;      _setcolor(read_pixel_from_tmap(srcb,((int) (u/z)) & 63,((int) (v/z)) & 63));
;      _setpixel(x,y);
;
;      u += du_dx;
;      v += dv_dx;
;      z += dz_dx;
;   }

	align	4
_asm_tmap_scanline_lin_lighted:
asm_tmap_scanline_lin_lighted:
;        push    es  ; DPH: No selectors in windows :-(
;        push    fs
	pusha

;        mov     es,[_pixel_data_selector]       ; selector[0*2] ; DPH: No... :-)
;        mov     fs,[_gr_fade_table_selector]    ; selector[1*2] ; fs = bmd_fade_table

; Setup for loop:	_loop_count  iterations = (int) xright - (int) xleft
;   esi	source pixel pointer = pixptr
;   edi	initial row pointer = y*320+x

; set esi = pointer to start of texture map data
	mov	esi,[_pixptr]

; set edi = address of first pixel to modify
	mov	edi,[_fx_y]	; this is actually an int
	cmp	edi,[_window_bottom]
	ja	near all_done

	imul	edi,[_bytes_per_row]
	mov	ebx,[_fx_xleft]
	test	ebx,ebx
	jns	ebx_ok
	sub	ebx,ebx
ebx_ok:
	add	edi,ebx
	add	edi,[_write_buffer]

; set _loop_count = # of iterations
	mov	eax,[_fx_xright]
	cmp	eax,[_window_right]
	jl	eax_ok1
	mov	eax,[_window_right]
eax_ok1:	cmp	eax,[_window_left]
	jg	eax_ok2
	mov	eax,[_window_left]
eax_ok2:

	sub	eax,ebx
	js	near all_done
	cmp	eax,[_window_width]
	jbe	_ok_to_do
	mov	eax,[_window_width]
_ok_to_do:
	mov	[_loop_count],eax

;	edi	destination pixel pointer


	mov	ecx,_gr_fade_table  ;originally offset _lighting_tables
	mov	eax,[_fx_u]	; get 32 bit u coordinate
	shr	eax,6	; get 6:10 int:frac u coordinate into low word
	mov	ebp,[_fx_v]	; get 32 bit v coordinate
	shl	ebp,10	; put 6:10 int:frac into high word
	mov	bp,ax	; put u coordinate in low word

	mov	eax,[_fx_du_dx]	; get 32 bit delta u
	shr	eax,6	; get 6:10 int:frac delta u into low word
	mov	edx,[_fx_dv_dx]	; get 32 bit delta v
	shl	edx,10	; put 6:10 int:frac into high word
	mov	dx,ax	; put delta u in low word

	sar	dword [_fx_dl_dx],8
	jns	dl_dx_ok
	inc	dword [_fx_dl_dx]	; round towards 0 for negative deltas
dl_dx_ok:

%if ALLOW_DITHERING
; do dithering, use lighting values which are .5 less than and .5 more than actual value
	mov	ebx,80h	; assume dithering on
	test	dword [_dither_intensity_lighting],-1
	jne	do_dither
	sub	ebx,ebx	; no dithering
do_dither:
	mov	eax,[_fx_dl_dx]
	add	eax,ebx	; add 1/2
	mov	[_fx_dl_dx1],eax
	sub	eax,ebx
	sub	eax,ebx
	mov	[_fx_dl_dx2],eax
	mov	ebx,[_fx_xleft]
	xor	ebx,[_fx_y]
	and	ebx,1
	jne	dith_1
	xchg	eax,[_fx_dl_dx1]
dith_1:	mov	[_fx_dl_dx2],eax
%endif

	mov	ebx,[_fx_l]

; lighting values are passed in fixed point, but need to be in 8 bit integer, 8 bit fraction so we can easily
; get the integer by reading %bh
	sar	ebx,8

	test	dword [_Transparency_on],-1
	jne	do_transparency_ll

;; esi, ecx should be free
loop_test equ 1	; set to 1 to run as loop for better profiling
%if loop_test
	mov	esi,[_fx_dl_dx]

	mov	ecx,[_loop_count]
	inc	ecx
	shr	ecx,1
	je	one_more_pix
	pushf

	align	4

loop1:
	mov	eax,ebp	; get u, v
	shr	eax,26	; shift out all but int(v)
	shld	ax,bp,6	; shift in u, shifting up v

	add	ebp,edx	; u += du, v += dv

        add eax,[_pixptr] ; DPH: No selectors in windows
        movzx   eax,byte [eax]     ; get pixel from source bitmap
	mov	ah,bh	; get lighting table
	add	ebx,esi	; _fx_dl_dx	; update lighting value
        mov     al,[_gr_fade_table + eax]     ; xlat pixel through lighting tables
	mov	[edi],al	; write pixel...
	inc	edi	; ...and advance

; --- ---

	mov	eax,ebp	; get u, v
	shr	eax,26	; shift out all but int(v)
	shld	ax,bp,6	; shift in u, shifting up v

	add	ebp,edx	; u += du, v += dv

        add eax,[_pixptr] ; DPH:
        movzx   eax,byte [eax]     ; get pixel from source bitmap
	mov	ah,bh	; get lighting table
	add	ebx,esi	; _fx_dl_dx	; update lighting value
        mov     al,[_gr_fade_table + eax]     ; xlat pixel through lighting tables
	mov	[edi],al	; write pixel...
	inc	edi	; ...and advance

	dec	ecx	; _loop_count
	jne	loop1

	popf
	jnc	all_done

one_more_pix:	mov	eax,ebp	; get u, v
	shr	eax,26	; shift out all but int(v)
	shld	ax,bp,6	; shift in u, shifting up v

        add eax,[_pixptr] ; DPH:
        movzx   eax,byte [eax]     ; get pixel from source bitmap
	mov	ah,bh	; get lighting table
        mov     al,[_gr_fade_table + eax]     ; xlat pixel through lighting tables
	mov	[edi],al	; write pixel...

all_done:	popa
;        pop     fs
;        pop     es
	ret


%else

; usage:
;	eax	work
;	ebx	lighting value
;	ecx	_lighting_tables
;	edx	du, dv 6:10:6:10
;	ebp	u, v coordinates 6:10:6:10
;	esi	pointer to source bitmap
;	edi	write address

;_size = (_end1 - _start1)/num_iters
;  note: if eax is odd, you will not be writing the last pixel, you must clean up at the end
%if ALLOW_DITHERING
    mov eax,[_loop_count]
    shr eax,1
    neg eax
    add eax,num_iters
%else
	mov	eax,num_iters
	sub	eax,[_loop_count]

%endif
	imul	eax,eax,(_end1 - _start1)/num_iters
	add	eax,_start1     ;originally offset _start1
	jmp	eax

	align	4
_start1:

; usage:
;	eax	work
;	ebx	lighting value
;	ecx	_lighting_tables
;	edx	du, dv 6:10:6:10
;	ebp	u, v coordinates 6:10:6:10
;	esi	pointer to source bitmap
;	edi	write address

; do all but the last pixel in the unwound loop, last pixel done separately because less work is needed
%rep num_iters
%if 1

;**; inner loop if lighting value is constant
;**; can be optimized if source bitmap pixels are stored as words, then the mov ah,bh is not necessary
;**	mov	eax,ebp	; get u, v
;**	shr	eax,26	; shift out all but int(v)
;**	shld	ax,bp,6	; shift in u, shifting up v
;**
;**	add	ebp,edx	; u += du, v += dv
;**
;**	mov	al,[esi+eax]	; get pixel from source bitmap
;**	mov	ah,bh	; get lighting table
;**	mov	al,[ecx+eax]	; xlat pixel through lighting tables
;**	mov	[edi],al	; write pixel...
;**	inc	edi	; ...and advance


%if ALLOW_DITHERING

; dithering
; loop contains two iterations which must be the same length
	mov	eax,ebp	; get u, v
	shr	eax,26	; shift out all but int(v)
	shld	ax,bp,6	; shift in u, shifting up v

	add	ebp,edx	; u += du, v += dv

	mov	al,[esi+eax]	; get pixel from source bitmap
	mov	ah,bh	; get lighting table
	add	ebx,[_fx_dl_dx1]	; update lighting value
	mov	al,[ecx+eax]	; xlat pixel through lighting tables
	mov	[edi],al	; write pixel...
	inc	edi	; ...and advance

; second iteration
	mov	eax,ebp	; get u, v
	shr	eax,26	; shift out all but int(v)
	shld	ax,bp,6	; shift in u, shifting up v

	add	ebp,edx	; u += du, v += dv

	mov	al,[esi+eax]	; get pixel from source bitmap
	mov	ah,bh	; get lighting table
	add	ebx,[_fx_dl_dx2]	; update lighting value
	mov	al,[ecx+eax]	; xlat pixel through lighting tables
	mov	[edi],al	; write pixel...
	inc	edi	; ...and advance
%else
	mov	eax,ebp	; get u, v
	shr	eax,26	; shift out all but int(v)
	shld	ax,bp,6	; shift in u, shifting up v

	add	ebp,edx	; u += du, v += dv

	mov	al,[esi+eax]	; get pixel from source bitmap
	mov	ah,bh	; get lighting table
	add	ebx,[_fx_dl_dx]	; update lighting value
	mov	al,[ecx+eax]	; xlat pixel through lighting tables
	mov	[edi],al	; write pixel...
	inc	edi	; ...and advance
%endif


%else

;obsolete: ; version which assumes segment overrides are in place (which they are obviously not)
;obsolete: 	mov	eax,ebp	; get u, v
;obsolete: 	shr	eax,26	; shift out all but int(v)
;obsolete: 	shld	ax,bp,6	; shift in u, shifting up v
;obsolete:
;obsolete: 	add	ebp,edx	; u += du, v += dv
;obsolete:
;obsolete: 	mov	al,[eax]	; get pixel from source bitmap
;obsolete: 	mov	ah,bh	; get lighting table
;obsolete: 	add	ebx,esi	; update lighting value
;obsolete: 	mov	al,[eax]	; xlat pixel through lighting tables
;obsolete: 	mov	[edi],al	; write pixel...
;obsolete: 	inc	edi	; ...and advance
%endif
%endrep

_end1:

; now do the leftover pixel
	mov	eax,ebp
	shr	eax,26	; shift in v coordinate
	shld	ax,bp,6	; shift in u coordinate while shifting up v coordinate
	mov	al,[esi+eax]	; get pixel from source bitmap
	mov	ah,bh	; get lighting table
 or ah,ah
 jns ok1
 sub ah,ah
ok1:
	mov	al,[_gr_fade_table+eax]	; xlat pixel through lighting tables
	mov	[edi],al	; write pixel...

_none_to_do:	popa
;        pop     fs
;        pop     es
	ret

%endif
; -- Code to get rgb 5 bits integer, 5 bits fraction value into 5 bits integer (for each gun)
; suitable for inverse color lookup
;**__test:
;** int 3
;**;	              rrrrrfffffrrrrrfffffxxbbbbbfffff
;**	mov	eax,11111001001010101110101101110111b
;**	and	eax,11111000001111100000001111100000b
;**	shld	ebx,eax,15
;**	or	bx,ax


;; esi, ecx should be free
do_transparency_ll:
	mov	esi,[_fx_dl_dx]

	mov	ecx,[_loop_count]
	inc	ecx
	shr	ecx,1
	je	one_more_pix2
	pushf

	align	4

loop1a:
	mov	eax,ebp	; get u, v
	shr	eax,26	; shift out all but int(v)
	shld	ax,bp,6	; shift in u, shifting up v

	add	ebp,edx	; u += du, v += dv

        add eax,[_pixptr] ; DPH:
        movzx   eax,byte [eax]     ; get pixel from source bitmap
	cmp	al,255
	je	skip1a
	mov	ah,bh	; get lighting table
	add	ebx,esi	; _fx_dl_dx	; update lighting value
        mov     al,[_gr_fade_table + eax]     ; xlat pixel through lighting tables
	mov	[edi],al	; write pixel...
skip1a:	inc	edi	; ...and advance

; --- ---

	mov	eax,ebp	; get u, v
	shr	eax,26	; shift out all but int(v)
	shld	ax,bp,6	; shift in u, shifting up v

	add	ebp,edx	; u += du, v += dv

        add eax,[_pixptr] ; DPH:
        movzx   eax,byte [eax]     ; get pixel from source bitmap
	cmp	al,255
	je	skip2a
	mov	ah,bh	; get lighting table
	add	ebx,esi	; _fx_dl_dx	; update lighting value
        mov     al,[_gr_fade_table + eax]     ; xlat pixel through lighting tables
	mov	[edi],al	; write pixel...
skip2a:	inc	edi	; ...and advance

	dec	ecx	; _loop_count
	jne	loop1a

	popf
	jnc	all_done_1

one_more_pix2:	mov	eax,ebp	; get u, v
	shr	eax,26	; shift out all but int(v)
	shld	ax,bp,6	; shift in u, shifting up v

        add eax,[_pixptr] ; DPH:
        movzx   eax,byte [eax]     ; get pixel from source bitmap
	cmp	al,255
	je	skip3a
	mov	ah,bh	; get lighting table
        mov     al,[_gr_fade_table + eax]     ; xlat pixel through lighting tables
	mov	[edi],al	; write pixel...

skip3a:
all_done_1:	popa
;        pop     fs ; DPH:
;        pop     es
	ret

