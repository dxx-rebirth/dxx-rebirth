; $Id: tmap_lin.asm,v 1.1.1.1 2006/03/17 19:59:56 zicodxx Exp $
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
; Linearly interpolating texture mapper inner loop
;
;

[BITS 32]

global	_asm_tmap_scanline_lin
global	asm_tmap_scanline_lin

[SECTION .data]

%include        "tmap_inc.asm"

_loop_count	dd	0

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
_asm_tmap_scanline_lin:
asm_tmap_scanline_lin:
	pusha

; Setup for loop:	_loop_count  iterations = (int) xright - (int) xleft
;	esi	source pixel pointer = pixptr
;	edi	initial row pointer = y*320+x

; set esi = pointer to start of texture map data
	mov	esi,[_pixptr]

; set edi = address of first pixel to modify
	mov	edi,[_fx_y]
	cmp	edi,[_window_bottom]
	ja	near _none_to_do

	imul	edi,[_bytes_per_row]
	mov	eax,[_fx_xleft]
	test	eax, eax
	jns	eax_ok
	sub	eax,eax
eax_ok:
	add	edi,eax
	add	edi,[_write_buffer]

; set _loop_count = # of iterations
	mov	eax,[_fx_xright]
	cmp	eax,[_window_right]
	jb	eax_ok1
	mov	eax,[_window_right]
eax_ok1:	cmp	eax,[_window_left]
	ja	eax_ok2
	mov	eax,[_window_left]
eax_ok2:

	mov	ebx,[_fx_xleft]
	sub	eax,ebx
	js	near _none_to_do
	cmp	eax,[_window_width]
	jbe	_ok_to_do
	mov	eax,[_window_width]
_ok_to_do:
	mov	[_loop_count],eax

;	edi	destination pixel pointer


	mov	ebx,[_fx_u]
	mov	ecx,[_fx_du_dx]
	mov	edx,[_fx_dv_dx]
	mov	ebp,[_fx_v]

	shl	ebx,10
	shl	ebp,10
	shl	edx,10
	shl	ecx,10

; eax	work
; ebx	u
; ecx	du_dx
; edx	dv_dx
; ebp	v
; esi	read address
; edi	write address

	test	dword [_Transparency_on],-1
	jne	near transparent_texture

%define _size	(_end1 - _start1)/num_iters
	mov	eax,num_iters-1
	sub	eax,[_loop_count]
	jns	j_eax_ok1
	inc	eax	; sort of a hack, but we can get -1 here and want to be graceful
	jns	j_eax_ok1	; if we jump, we had -1, which is kind of ok, if not, we int 3
	int	3	; oops, going to jump behind _start1, very bad...
	sub	eax,eax	; ok to continue
j_eax_ok1:	imul	eax,eax,_size
	add	eax,_start1
	jmp	eax

	align	4
_start1:

; "OPTIMIZATIONS" maybe not worth making
;    Getting rid of the esi from the mov al,[esi+eax] instruction.
;       This would require moving into eax at the top of the loop, rather than doing the sub eax,eax.
;       You would have to align your bitmaps so that the two shlds would create the proper base address.
;       In other words, your bitmap data would have to begin at 4096x (for 64x64 bitmaps).
;       I did timings without converting the sub to a mov eax,esi and setting esi to the proper value.
;       There was a speedup of about 1% to 1.5% without converting the sub to a mov.
;    Getting rid of the edi by doing a mov nnnn[edi],al instead of mov [edi],al.
;       The problem with this is you would have a dword offset for nnnn.  My timings indicate it is slower.  (I think.)
;    Combining u,v and du,dv into single longwords.
;       The problem with this is you then must do a 16 bit operation to extract them, and you don't have enough
;       instructions to separate a destination operand from being used by the next instruction.  It shaves out one
;       register instruction (an add reg,reg), but adds a 16 bit operation, and the setup is more complicated.
; usage:
;	eax	work
;	ebx	u coordinate
;	ecx	delta u
;	edx	delta v
;	ebp	v coordinate
;	esi	pointer to source bitmap
;	edi	write address
%rep num_iters
	mov	eax,ebp	; clear for 
	add	ebp,edx	; update v coordinate
	shr	eax,26	; shift in v coordinate
	shld	eax,ebx,6	; shift in u coordinate while shifting up v coordinate
	add	ebx,ecx	; update u coordinate
	mov	al,[esi+eax]	; get pixel from source bitmap
	mov	[edi],al
	inc	edi		; XPARENT ADDED BY JOHN

; inner loop if bitmaps are 256x256
; your register usage is bogus, and you must clear ecx
; fix your setup
; this is only about 10% faster in the inner loop
; this method would adapt to writing two pixels at a time better than
; the 64x64 method because you wouldn't run out of registers
; Note that this method assumes that both dv_dx and du_dx are in edx.
; edx = vi|vf|ui|uf
; where each field is 8 bits, vi = integer v coordinate, vf = fractional v coordinate, etc.
;** add ebx,edx
;** mov cl,bh
;** shld cx,bx,8
;** mov al,[esi+ecx]
;** mov [edi],al
;** inc edi
%endrep

_end1:

_none_to_do:	popa

	ret

; ----------------------------------------------------------------------------------------
; if texture map has transparency, use this code.
transparent_texture:
	test	dword [_loop_count],-1
	je	_t_none_to_do
loop_transparent:
	mov	eax,ebp	; clear for 
	add	ebp,edx	; update v coordinate
	shr	eax,26	; shift in v coordinate
	shld	eax,ebx,6	; shift in u coordinate while shifting up v coordinate
	add	ebx,ecx	; update u coordinate
	mov	al,[esi+eax]	; get pixel from source bitmap
	cmp	al,255
	je	transp
	mov	[edi],al
transp:	inc	edi		; XPARENT ADDED BY JOHN

	dec	dword [_loop_count]
	jne	loop_transparent

_t_none_to_do:	popa
	ret


; This is the inner loop to write two pixels at a time
; This is about 2.5% faster overall (on Mike's 66 MHz 80486 DX2, VLB)
; You must write code to even align edi and do half as many iterations, and write
; the beginning and ending extra pixels, if necessary.
;	sub	eax,eax	; clear for 
;	shld	eax,ebp,6	; shift in v coordinate
;	add	ebp,_fx_dv_dx	; update v coordinate
;	shld	eax,ebx,6	; shift in u coordinate while shifting up v coordinate
;	add	ebx,ecx	; update u coordinate
;	mov	dl,[esi+eax]	; get pixel from source bitmap
;
;	sub	eax,eax	; clear for 
;	shld	eax,ebp,6	; shift in v coordinate
;	add	ebp,_fx_dv_dx	; update v coordinate
;	shld	eax,ebx,6	; shift in u coordinate while shifting up v coordinate
;	add	ebx,ecx	; update u coordinate
;	mov	dh,[esi+eax]	; get pixel from source bitmap
;
;	mov	[edi],dx
;	add	edi,2

