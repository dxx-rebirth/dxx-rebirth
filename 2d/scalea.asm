; $ Id: $
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
; Asm inner loop for scaler.
; 
; Old Log:
;
; Revision 1.2  1994/01/12  18:02:52  john
; Asm code for the scaler... first iteration here
; has compiled code that works!!
; 
; Revision 1.1  1994/01/12  12:20:11  john
; Initial revision
; 
; 



[BITS 32]

[SECTION .data]

	align	4
	

	global _scale_trans_color
	global _scale_error_term
	global _scale_initial_pixel_count
	global _scale_adj_up
	global _scale_adj_down
	global _scale_final_pixel_count
	global _scale_ydelta_minus_1
        global _scale_whole_step
	global _scale_source_ptr
	global _scale_dest_ptr

		
	_scale_trans_color		db	0
	_scale_error_term		dd	0
	_scale_initial_pixel_count	dd	0
	_scale_adj_up			dd	0
	_scale_adj_down 		dd	0
	_scale_final_pixel_count	dd	0
	_scale_ydelta_minus_1		dd	0
	_scale_whole_step		dd	0
	_scale_source_ptr		dd	0
	_scale_dest_ptr 		dd	0
	_scale_cc_jump_spot		dd	0
	_scale_slice_length_1		dd	0
	_scale_slice_length_2		dd	0

[SECTION .text]

global _rls_stretch_scanline_asm

_rls_stretch_scanline_asm:
		cld
		pusha
		mov	esi, [_scale_source_ptr]
		mov	edi, [_scale_dest_ptr]
		mov	edx, [_scale_ydelta_minus_1]
		mov	ebx, [_scale_whole_step]
		mov	ah,  [_scale_trans_color]
		mov	ebp, [_scale_error_term]

		mov	ecx, [_scale_initial_pixel_count]
		lodsb
		cmp	al, ah
		je	first_pixel_transparent
		rep	stosb
		jmp	next_pixel

first_pixel_transparent:
		add	edi, ecx
		jmp	next_pixel

skip_this_pixel:	add	edi, ecx
			dec	edx
			jz	done

next_pixel:		lodsb				; get next source pixel
			mov	ecx, ebx					
			add	ebp, [_scale_adj_up]
			jle	no_inc_error_term
			inc	ecx
			sub	ebp, [_scale_adj_down]
no_inc_error_term:						
			cmp	al, ah
			je	skip_this_pixel
			rep	stosb			; write source pixel to n locations
			dec	edx
			jnz	next_pixel

done:			lodsb
			cmp	al, ah
			je	exit_sub
			mov	ecx, [_scale_final_pixel_count]
			rep	stosb

exit_sub:
			popa
			ret

global _scale_do_cc_scanline
global _rls_do_cc_setup_asm

_scale_do_cc_scanline:
	cld
	pusha
	mov	esi, [_scale_source_ptr]
	mov	edi, [_scale_dest_ptr]
	mov	ah,  [_scale_trans_color]
	mov	ebx, [_scale_slice_length_1]
	mov	edx, [_scale_slice_length_2]

	mov	ecx, [_scale_initial_pixel_count]

	; Do the first texture pixel
	mov	ecx, [_scale_initial_pixel_count]
	lodsb
	cmp	al, ah
	je	@1
	rep	stosb
@1:	add	edi, ecx

	mov	ecx, [_scale_cc_jump_spot]
	jmp	ecx

	; This is the compiled code to do the middle pixels...
scale_cc_start:		mov	al, [esi]
scale_cc_changer:	db	08bh, 0cbh ;mov     ecx, ebx	    ;<==== CODE CHANGES TO EBX OR EDX !!!!!!
			inc	esi
			cmp	al, ah
			je	@2
			rep	stosb
		@2:	add	edi, ecx
scale_cc_end:

%macro repproc 0
                        mov     al, [esi]
			db	08bh, 0cbh ;mov     ecx, ebx	    ;<==== CODE CHANGES TO EBX OR EDX !!!!!!
                        inc     esi
                        cmp     al, ah
                        je      %%skip
                        rep     stosb
%%skip:                 add     edi, ecx
%endmacro

%rep 319
repproc
%endrep
last_cc_instruction:

	mov	ecx, [_scale_final_pixel_count]
	lodsb
	cmp	al, ah
	je	last_one_transparent
	rep	stosb
last_one_transparent:

	popa
	ret

_rls_do_cc_setup_asm:

		pusha

		mov	ebx, [_scale_whole_step]
		mov	[_scale_slice_length_1], ebx
		inc	ebx
		mov	[_scale_slice_length_2], ebx

		mov	ebp, [_scale_error_term]
		mov	edx, [_scale_ydelta_minus_1]

		mov	ebx,  scale_cc_end
		sub	ebx,  scale_cc_start	  ; ebx = distance to next changer inst.

		mov	eax, [_scale_ydelta_minus_1]

		
		imul	eax, ebx		; eax = sizeof 1 iteration * numiterations.
						
		mov	edi,  last_cc_instruction
		sub	edi, eax		; edi = address of first iteration that we need
						;       to jump to
		mov	[_scale_cc_jump_spot], edi

		mov	ecx,  scale_cc_changer
		sub	ecx,  scale_cc_start	  ; ecx = distance from start to to next changer inst.
		
		add	edi, ecx

next_pixel1:		add	ebp, [_scale_adj_up]
			jle	no_inc_error_term1
			; Modify code in scale_do_cc_scanline_ to write 'edx' pixels
		mov	al, byte [edi]
		cmp	al, 08bh
		jne	BigError
			mov	word[edi], 0CA8Bh  ; 0x8BCA = mov ecx, edx
			add	edi, ebx		
			sub	ebp, [_scale_adj_down]
			dec	edx
			jnz	next_pixel1
			jmp	done1

no_inc_error_term1:	; Modify code in scale_do_cc_scanline_ to write 'ebx' pixels
		mov	al, byte [edi]
		cmp	al, 08bh
		jne	BigError
			mov	word [edi], 0CB8Bh  ; 0x8BCB = mov ecx, ebx
			add	edi, ebx		
			dec	edx
			jnz	next_pixel1

done1:		popa
		ret

BigError:	int 3	; Stop, buddy!!
		popa
		ret
