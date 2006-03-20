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

[BITS 32]

section .data

row_count dd	 0

section .text

; C-calling:
; void gr_merge_textures( ubyte * lower, ubyte * upper, ubyte * dest )

global	_gr_merge_textures, _gr_merge_textures_1, _gr_merge_textures_2, _gr_merge_textures_3
global gr_merge_textures, gr_merge_textures_1, gr_merge_textures_2, gr_merge_textures_3

; case 1:
;    // 
;    for (y=0; y<64; y++ )
;       for (x=0; x<64; x++ )   {
;          c = top_data[ 64*x+(63-y) ];      
;          if (c==255)
;             c = bottom_data[ 64*y+x ];
;          *dest_data++ = c;
;       }
;    break;
; case 2:
;    // Normal
;    for (y=0; y<64; y++ )
;       for (x=0; x<64; x++ )   {
;          c = top_data[ 64*(63-y)+(63-x) ];
;          if (c==255)
;             c = bottom_data[ 64*y+x ];
;          *dest_data++ = c;
;       }
;    break;
; case 3:
;    // Normal
;    for (y=0; y<64; y++ )
;       for (x=0; x<64; x++ )   {
;          c = top_data[ 64*(63-x)+y  ];
;          if (c==255)
;             c = bottom_data[ 64*y+x ];
;          *dest_data++ = c;
;       }
;    break;
_gr_merge_textures:
gr_merge_textures:

	; EAX = lower data
	; EDX = upper data
	; EBX = dest data

	push	ebx
	push	esi
        push    edi
	push	ebp

	mov	eax, [esp+20]
	mov	edx, [esp+24]
	mov	ebx, [esp+28]

	mov	ebp, edx
	mov	edi, ebx
	mov	bl, 255
	mov	bh, bl
	and	ebx, 0ffffh
	and	edx, 0ffffh
	mov	esi, eax
	mov	ecx, (64*64)/2

	jmp	gmt1

second_must_be_not_trans:
	mov	ah, dh
	mov	[edi],ax
	add	edi,2
	dec	ecx
	je	donegmt

gmt1:	mov	dx, [ebp]
	add 	ebp, 2
	cmp	edx, ebx
	jne	not_transparent

; both transparent
	movsw
	dec	ecx
	jne	gmt1
	jmp	donegmt

; DH OR DL ARE INVISIBLE
not_transparent:
	mov	ax,[esi]
	add	esi,2
		
	cmp	dl, bl
	je	second_must_be_not_trans
	mov	al, dl
	cmp	dh, bh
	je	@@1
	mov	ah, dh
@@1:	mov	[edi],ax
	add	edi,2
	dec	ecx
	jne	gmt1

donegmt:

	pop	ebp
	pop	edi
	pop	esi
	pop	ebx
	ret

; -----------------------------------------------------------------------------------------
; case 1, normal in x, flip in y
_gr_merge_textures_1:
gr_merge_textures_1:

; eax = background data
; edx = foreground data
; ebx = destination address

	push	ebp
	push	edi
	push	esi
	push	ebx

	mov	eax, [esp+20]
	mov	edx, [esp+24]
        mov     ebx, [esp+28]

	mov	ch, 255 ; transparent color, stick in a register, is this faster?

	mov	esi, 63	; esi will be the offset to the current pixel
	mov	[row_count], esi

	mov	ebp, 64	; do for 64 pixels

	align	4
gmt1_1:	mov	cl, [edx + esi]	; get foreground pixel
	add 	esi, 64	; point at next row
	cmp	cl, ch	; see if transparent
	jne	not_transparent_1	; dl contains a solid color, just write it

	mov	cl,[eax]	; get background pixel

not_transparent_1:	mov	[ebx], cl	; write pixel
	inc	ebx	; point at next destination address
	inc	eax

	dec	ebp	; see if done a whole row
	jne	gmt1_1	; no, so do next pixel

	mov	ebp, 64	; do for 64 pixels

	dec	dword [row_count] ; advance to next row
	mov	esi, [row_count]  ; doing next row, get offset, DANGER: DOESN'T SET FLAGS, BEWARE ON 68000, POWERPC!!
	jns	gmt1_1	; no (going down to 0)

	pop	ebx
	pop	esi
	pop	edi
	pop	ebp
	ret

; -----------------------------------------------------------------------------------------
; case 1, normal in x, flip in y
_gr_merge_textures_2:
gr_merge_textures_2:

; eax = background data
; edx = foreground data
; ebx = destination address

	push	ebp
	push	edi
	push	esi
	push	ebx

	mov	eax, [esp+20]
	mov	edx, [esp+24]
        mov     ebx, [esp+28]

	mov	ch, 255	; transparent color, stick in a register, is this faster?

	mov	esi, 63 + 64*63	; esi will be the offset to the current pixel

	align	4
gmt1_2:	mov	cl, [edx + esi]	; get foreground pixel
	cmp	cl, ch	; see if transparent
	jne	not_transparent_2	; dl contains a solid color, just write it

	mov	cl,[eax]	; get background pixel

not_transparent_2:	mov	[ebx], cl	; write pixel
	inc	ebx	; point at next destination address
	inc	eax

	dec	esi	; advance to next row
	jns	gmt1_2	; no (going down to 0)

	pop	ebx
	pop	esi
	pop	edi
	pop	ebp
	ret

; -----------------------------------------------------------------------------------------
; case 1, normal in x, flip in y
_gr_merge_textures_3:
gr_merge_textures_3:

; eax = background data
; edx = foreground data
; ebx = destination address

	push	ebp
	push	edi
	push	esi
	push	ebx

	mov	eax, [esp+20]
	mov	edx, [esp+24]
        mov     ebx, [esp+28]

	mov	ch, 255	; transparent color, stick in a register, is this faster?

	mov	esi, 64*63	; esi will be the offset to the current pixel
	mov	dword [row_count], 64

	mov	ebp, 32	; do for 64 pixels (2x loop)

; first copy of loop
	align	4
gmt1_3:	mov	cl, [edx + esi]	; get foreground pixel
	sub 	esi, 64	; point at next row
	cmp	cl, ch	; see if transparent
	jne	not_transparent_3	; dl contains a solid color, just write it

	mov	cl,[eax]	; get background pixel

not_transparent_3:	inc	eax
	mov	[ebx], cl	; write pixel
	inc	ebx	; point at next destination address

; second copy of loop
	mov	cl, [edx + esi]	; get foreground pixel
	sub 	esi, 64	; point at next row
	cmp	cl, ch	; see if transparent
	jne	nt_3a	; dl contains a solid color, just write it

	mov	cl,[eax]	; get background pixel

nt_3a:	inc	eax
	mov	[ebx], cl	; write pixel
	inc	ebx	; point at next destination address

	dec	ebp	; see if done a whole row
	jne	gmt1_3	; no, so do next pixel

	mov	ebp, 32	; do for 64 pixels

	add	esi, 64*64+1	; doing next row, get offset, DANGER: DOESN'T SET FLAGS, BEWARE ON 68000, POWERPC!!

	dec	dword [row_count] ; advance to next row
	jne	gmt1_3	; no (going down to 0)

	pop	ebx
	pop	esi
	pop	edi
	pop	ebp
	ret
