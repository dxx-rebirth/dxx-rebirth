; $Id: vecmata.asm,v 1.5 2003-12-08 21:21:16 btb Exp $
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
; Source for vector/matrix library
;
; Old Log:
; Revision 1.54  1995/01/31  00:14:50  matt
; Took out int3 from dotprod overflow, since it saturates now
;
; Revision 1.53  1994/12/14  18:29:33  matt
; Made dotprod overflow check stay in, and return saturated value
;
; Revision 1.52  1994/12/14  12:34:13  matt
; Disabled now-unused vector_2_matrix_norm()
;
; Revision 1.51  1994/12/13  16:55:13  matt
; Ripped out optimization from last version, which was bogus
;
; Revision 1.50  1994/12/13  14:55:18  matt
; Use quick normalize in a couple of places where it was safe to do so
;
; Revision 1.49  1994/12/13  14:44:12  matt
; Added vm_vector_2_matrix_norm()
;
; Revision 1.48  1994/12/13  13:26:49  matt
; Fixed overflow check
;
; Revision 1.47  1994/12/03  15:39:54  matt
; Gracefully handle some vector_2_matrix problems
;
; Revision 1.46  1994/11/19  17:15:05  matt
; Assemble out some code not used in DESCENT
;
; Revision 1.45  1994/11/17  11:41:05  matt
; Put handling in extract_angles_from_matrix to deal with bad matrices
;
; Revision 1.44  1994/11/16  11:48:10  matt
; Added error checking to vm_extract_angles_matrix()
;
; Revision 1.43  1994/09/19  22:00:10  matt
; Fixed register trash
;
; Revision 1.42  1994/09/11  19:23:05  matt
; Added vm_vec_normalized_dir_quick()
;
; Revision 1.41  1994/08/14  13:28:38  matt
; Put in check for zero-length vector in extract angles
;
; Revision 1.40  1994/07/19  18:52:53  matt
; Added vm_vec_normalize_quick() and vm_vec_copy_normalize_quick()
;
; Revision 1.39  1994/06/16  18:24:22  matt
; Added vm_vec_mag_quick()
;
; Revision 1.38  1994/06/10  23:18:38  matt
; Added new code for vm_vec_ang_2_matrix() which may be better, but may
; not be.
;
; Revision 1.37  1994/05/22  18:17:29  mike
; Optimize vm_vec_dist_quick, using jns in place of abs_eax.
;
; Revision 1.36  1994/05/19  12:07:04  matt
; Fixed globals and macros and added a constant
;
; Revision 1.35  1994/05/19  09:19:00  matt
; Made vm_vec_normalized_dir() return mag of vector
;
; Revision 1.34  1994/05/18  22:28:01  matt
; Added function vm_vec_normalized_dir()
; Added C macros IS_ZERO_VEC(), vm_vec_zero(), and vm_set_identity()
; Added C global static vars vmd_zero_vector & vmd_identity_matrix
;
; Revision 1.33  1994/05/18  21:44:16  matt
; Added functions:
;   vm_extract_angles_vector()
;   vm_extract_angles_vector_normalized()
;   vm_vec_copy_normalize()
;
; Revision 1.32  1994/05/13  12:41:51  matt
; Added new function, vm_vec_dist_quick(), which does an approximation.
;
; Revision 1.31  1994/05/04  17:41:31  mike
; Comment out debug_brk on null vector.
;
; Revision 1.30  1994/04/15  21:41:31  matt
; Check for foward vector straigt up in extract angles routine
;
; Revision 1.29  1994/03/30  15:45:05  matt
; Added two functions, vm_vec_scale_add() & vm_vec_scale_add2()
;
; Revision 1.28  1994/02/26  19:23:35  matt
; Do an int3 when we get a null vector when computing surface normal
;
; Revision 1.27  1994/02/10  18:29:45  matt
; Changed 'if DEBUG_ON' to 'ifndef NDEBUG'
;
; Revision 1.26  1994/02/10  18:28:55  matt
; Fixed bugs in extract angles function
;
; Revision 1.25  1994/01/31  22:46:07  matt
; Added vm_extract_angles_matrix() function
;
; Revision 1.24  1994/01/30  19:29:55  matt
; Put in debug_brk when vm_vec_2_matrix got zero-length vector
;
; Revision 1.23  1994/01/25  15:27:59  matt
; Added debugging check for dotprod overflow
;
; Revision 1.22  1994/01/24  11:52:59  matt
; Added checking for dest==src for several functions where this is not allowed
;
; Revision 1.21  1994/01/19  23:13:02  matt
; Fixed bug in vm_vec_ang_2_matrix()
;
; Revision 1.20  1994/01/04  12:33:43  mike
; Prevent divide overflow in vm_vec_scale2
;
; Revision 1.19  1993/12/21  19:46:26  matt
; Added function vm_dist_to_plane()
;
; Revision 1.18  1993/12/13  17:26:23  matt
; Added vm_vec_dist()
;
; Revision 1.17  1993/12/02  12:43:39  matt
; New functions: vm_vec_copy_scale(), vm_vec_scale2()
;
; Revision 1.16  1993/10/29  22:39:29  matt
; Changed matrix order, making direction vectors the rows
;
; Revision 1.15  1993/10/29  18:06:01  matt
; Fixed vm_vector_2_matrix() bug when forward vector was straight down
;
; Revision 1.14  1993/10/26  18:51:26  matt
; Fixed some register trashes in vm_vec_ang_2_matrix()
;
; Revision 1.13  1993/10/25  11:49:37  matt
; Made vm_vec_delta_ang() take optional forward vector to return signed delta
;
; Revision 1.12  1993/10/20  01:09:42  matt
; Added vm_vec_delta_ang(), vm_vec_delta_ang_norm(), and vm_vec_ang_2_matrix()
;
; Revision 1.11  1993/10/17  17:03:08  matt
; vm_vector_2_matrix() now takes optional right vector
;
; Revision 1.10  1993/10/10  18:11:42  matt
; Changed angles_2_matrix so that heading & bank rotate in the
; correct directions.
;
; Revision 1.9  1993/09/30  16:17:59  matt
; Fixed bug in vector_2_matrix() by adding another normalize
;
; Revision 1.8  1993/09/29  10:51:58  matt
; Fixed bad register trashes in crossprod, perp, & normal
;
; Revision 1.7  1993/09/28  12:16:46  matt
; Fixed bugs in cross product
; Added func vm_vector_2_matrix()
;
; Revision 1.6  1993/09/24  21:19:37  matt
; Added vm_vec_avg() and vm_vec_avg4()
;
; Revision 1.5  1993/09/20  18:15:07  matt
; Trap zero-length vectors in vm_vec_normalize(), vm_vec_perp(), and vm_vec_normal()
;
; Revision 1.4  1993/09/20  14:56:43  matt
; Fixed bug in vm_vec_normal(), made that routine normalize the results,
; and added new function vm_vec_perp().
;
; Revision 1.3  1993/09/20  10:12:06  mike
; no changes
;
; Revision 1.2  1993/09/17  11:10:33  matt
; Added vm_vec_add2() and vm_vec_sub2(), which take 2 args (dest==src0)
;
; Revision 1.1  1993/09/16  20:10:24  matt
; Initial revision
;
;
;

%define NDEBUG

[BITS 32]

%ifdef __ELF__
; Cater for ELF compilers which don't prefix underscores...
; Variables:
%define _vmd_zero_vector              vmd_zero_vector
%define _vmd_identity_matrix          vmd_identity_matrix
; Functions:
%define _vm_vec_add     vm_vec_add
%define _vm_vec_sub     vm_vec_sub
%define _vm_vec_add2    vm_vec_add2
%define _vm_vec_sub2    vm_vec_sub2
%define _vm_vec_avg     vm_vec_avg
%define _vm_vec_scale   vm_vec_scale
%define _vm_vec_copy_scale      vm_vec_copy_scale
%define _vm_vec_scale2  vm_vec_scale2
%define _vm_vec_scale_add       vm_vec_scale_add
%define _vm_vec_scale_add2      vm_vec_scale_add2
%define _vm_vec_mag     vm_vec_mag
%define _vm_vec_dist    vm_vec_dist
%define _vm_vec_mag_quick       vm_vec_mag_quick
%define _vm_vec_dist_quick      vm_vec_dist_quick
%define _vm_vec_normalize       vm_vec_normalize
%define _vm_vec_normalize_quick vm_vec_normalize_quick
%define _vm_vec_normalized_dir  vm_vec_normalized_dir
%define _vm_vec_normalized_dir_quick    vm_vec_normalized_dir_quick
%define _vm_vec_copy_normalize  vm_vec_copy_normalize
%define _vm_vec_copy_normalize_quick    vm_vec_copy_normalize_quick
%define _vm_vec_dotprod         vm_vec_dotprod
%define _vm_vec_crossprod       vm_vec_crossprod
%define _vm_vec_perp            vm_vec_perp
%define _vm_vec_normal          vm_vec_normal
%define _vm_vec_rotate          vm_vec_rotate
%define _vm_vec_delta_ang       vm_vec_delta_ang
%define _vm_vec_delta_ang_norm  vm_vec_delta_ang_norm
%define _vm_vector_2_matrix     vm_vector_2_matrix
%define _vm_vec_ang_2_matrix    vm_vec_ang_2_matrix
%define _vm_angles_2_matrix             vm_angles_2_matrix
%define _vm_transpose_matrix            vm_transpose_matrix
%define _vm_copy_transpose_matrix       vm_copy_transpose_matrix
%define _vm_matrix_x_matrix             vm_matrix_x_matrix
%endif

[SECTION .data]

;temporary vectors for surface normal calculation
tempv0	dd 0,0,0
tempv1	dd 0,0,0

xvec	dd 0,0,0
yvec	dd 0,0,0
zvec	dd 0,0,0

tempav	dw 0,0,0

;sine & cosine values for angles_2_matrix
sinp	dd	0
cosp	dd	0
sinb	dd	0
cosb	dd	0
sinh	dd	0
cosh	dd	0

	global _vmd_zero_vector,_vmd_identity_matrix
%define f1_0 10000h
;These should never be changed!
_vmd_zero_vector:
	dd	0,0,0

_vmd_identity_matrix:
	dd	f1_0,0,0
	dd	0,f1_0,0
	dd	0,0,f1_0

[SECTION .text]
	extern quad_sqrt_asm
	extern fix_sincos_asm
	extern fix_acos_asm
	extern long_sqrt_asm
; calling convention is arguments on stack from right to left,
; eax,ecx,edx possibly destroyed, ebx,esi,edi,ebp preserved,
; caller removes arguments.
	global _vm_vec_add
	global _vm_vec_sub
	global _vm_vec_add2
	global _vm_vec_sub2
	global _vm_vec_avg
	global _vm_vec_scale
	global _vm_vec_copy_scale
	global _vm_vec_scale2
	global _vm_vec_scale_add
	global _vm_vec_scale_add2
	global _vm_vec_mag
	global _vm_vec_dist
	global _vm_vec_mag_quick
	global _vm_vec_dist_quick
	global _vm_vec_normalize
	global _vm_vec_normalize_quick
	global _vm_vec_normalized_dir
	global _vm_vec_normalized_dir_quick
	global _vm_vec_copy_normalize
	global _vm_vec_copy_normalize_quick
	global _vm_vec_dotprod
	global _vm_vec_crossprod
	global _vm_vec_perp
	global _vm_vec_normal
	global _vm_angles_2_matrix
	global _vm_vec_rotate
	global _vm_vec_delta_ang
	global _vm_vec_delta_ang_norm
	global _vm_transpose_matrix
	global _vm_copy_transpose_matrix
	global _vm_matrix_x_matrix
	global _vm_vector_2_matrix
	global _vm_vec_ang_2_matrix

%macro debug_brk 1-*
;int 3
%endmacro

%macro abs_eax 0
	cdq
        xor     eax,edx
        sub     eax,edx
%endmacro

%macro fixmul 1
	imul %1
	shrd eax,edx,16
%endmacro

%macro fixdiv 1
	mov	edx,eax
	sar	edx,16
	shl	eax,16
	idiv	%1
%endmacro

%macro vm_copy 2
%assign i 0
%rep 3
	mov	eax,[%2+i]
	mov	[%1+i],eax
%assign i i+4
%endrep
%endmacro

; offsets in fixang struct (packed)
%define pitch 0
%define bank 2
%define head 4
; offsets in matrix
%define m1 0*4
%define m4 1*4
%define m7 2*4
%define m2 3*4
%define m5 4*4
%define m8 5*4
%define m3 6*4
%define m6 7*4
%define m9 8*4
;vector offsets in matrix
%define rvec 0*12
%define uvec 1*12
%define fvec 2*12

; vec *vm_vec_add(vec *dest, vec *src1, vec *src2);
; returns dest
; dest=src1+src2
_vm_vec_add:
	push ebx
	mov eax,[esp+8]
	mov ecx,[esp+12]
	mov edx,[esp+16]
%assign i 0
%rep 3
	mov ebx,[ecx+i]
	add ebx,[edx+i]
	mov [eax+i],ebx
%assign i i+4
%endrep
	pop ebx
	ret

; vec *vm_vec_sub(vec *dest, vec *src1, vec *src2);
; returns dest
; dest=src1-src2
_vm_vec_sub:
	push ebx
	mov eax,[esp+8]
	mov ecx,[esp+12]
	mov edx,[esp+16]
%assign i 0
%rep 3
	mov ebx,[ecx+i]
	sub ebx,[edx+i]
	mov [eax+i],ebx
%assign i i+4
%endrep
	pop ebx
        ret

; vec *vm_vec_add2(vec *dest, vec *src);
; returns dest
; dest+=src
_vm_vec_add2:
	mov eax,[esp+4]
	mov ecx,[esp+8]
%assign i 0
%rep 3
	mov edx,[ecx+i]
	add [eax+i],edx
%assign i i+4
%endrep
        ret

; vec *vm_vec_sub2(vec *dest, vec *src);
; returns dest
; dest-=src
_vm_vec_sub2:
	mov eax,[esp+4]
	mov ecx,[esp+8]
%assign i 0
%rep 3
	mov edx,[ecx+i]
	sub [eax+i],edx
%assign i i+4
%endrep
        ret

; vec *vm_vec_avg(vec *dest, vec *src1, vec *src2);
; returns dest
; dest=(src1+src2)/2
_vm_vec_avg:
	push ebx
	mov eax,[esp+8]
	mov ecx,[esp+12]
	mov edx,[esp+16]
%assign i 0
%rep 3
	mov ebx,[ecx+i]
	add ebx,[edx+i]
	sar ebx,1
	mov [eax+i],ebx
%assign i i+4
%endrep
	pop ebx
        ret

; vec *vm_vec_scale(vec *dest, fix scale);
; returns dest
; dest*=scale
_vm_vec_scale:
	push ebx
	mov ebx,[esp+8]
	mov ecx,[esp+12]
%assign i 0
%rep 3
	mov eax,[ebx+i]
	fixmul ecx
	mov [ebx+i],eax
%assign i i+4
%endrep
	mov eax,ebx
	pop ebx
        ret

; vec *vm_vec_copy_scale(vec *dest, vec *src, fix scale);
; returns dest
; dest=src*scale
_vm_vec_copy_scale:
	push ebx
	push edi
	mov edi,[esp+12]
	mov ebx,[esp+16]
	mov ecx,[esp+20]
%assign i 0
%rep 3
	mov eax,[ebx+i]
	fixmul ecx
	mov [edi+i],eax
%assign i i+4
%endrep
	mov eax,edi
	pop edi
	pop ebx
        ret

; vec *vm_vec_scale_add(vec *dest, vec *src1, vec *src2, fix scale);
; returns dest
; dest=src1+src2*scale
_vm_vec_scale_add:
	push ebx
	push esi
        push edi
	mov edi,[esp+16]
	mov ebx,[esp+20]
	mov esi,[esp+24]
	mov ecx,[esp+28]
%assign i 0
%rep 3
	mov eax,[esi+i]
	fixmul ecx
	add eax,[ebx+i]
	mov [edi+i],eax
%assign i i+4
%endrep
	mov eax,edi
	pop edi
	pop esi
	pop ebx
        ret

; vec *vm_vec_scale_add2(vec *dest, vec *src, fix scale);
; returns dest
; dest+=src*scale
_vm_vec_scale_add2:
	push ebx
	push edi
	mov edi,[esp+12]
	mov ebx,[esp+16]
	mov ecx,[esp+20]
%assign i 0
%rep 3
	mov eax,[ebx+i]
	fixmul ecx
	add [edi+i],eax
%assign i i+4
%endrep
	mov eax,edi
	pop edi
	pop ebx
        ret

; vec *vm_vec_scale2(vec *dest, fix n, fix d);
; returns dest
; dest*=n/d
_vm_vec_scale2:
	push ebx
	push edi
	mov edi,[esp+12]
	mov ebx,[esp+16]
	mov ecx,[esp+20]
	or ecx,ecx
	je no_scale2
%assign i 0
%rep 3
	mov eax,[edi+i]
	imul ebx
	idiv ecx
	mov [edi+i],eax
%assign i i+4
%endrep
no_scale2:
	mov eax,edi
	pop edi
	pop ebx
        ret

;compute magnitude of vector. takes esi=vector, returns eax=mag
_vm_vec_mag:
        push    ebx
        push    esi
        mov     esi,[esp+12]

        mov     eax,[esi]
        imul    eax
        mov     ebx,eax
        mov     ecx,edx

        mov     eax,[esi+4]
        imul    eax
        add     ebx,eax
        adc     ecx,edx

        mov     eax,[esi+8]
        imul    eax
        add     eax,ebx
        adc     edx,ecx

        call    quad_sqrt_asm

        pop     esi
        pop     ebx
        ret

;compute the distance between two points. (does sub and mag)
_vm_vec_dist:
	push ebx
	push esi
	push edi
	mov esi,[esp+16]
	mov edi,[esp+20]
	mov eax,[esi]
	sub eax,[edi]
	imul eax
	mov ebx,eax
	mov ecx,edx
	mov eax,[esi+4]
	sub eax,[edi+4]
	imul eax
	add ebx,eax
	adc ecx,edx
	mov eax,[esi+8]
	sub eax,[edi+8]
	imul eax
	add eax,ebx
	adc edx,ecx
	call quad_sqrt_asm ; asm version, takes eax,edx
	pop edi
	pop esi
	pop ebx
	ret


;computes an approximation of the magnitude of a vector
;uses dist = largest + next_largest*3/8 + smallest*3/16
	align 4
_vm_vec_mag_quick:
	push	ebx
	push	esi
	mov	esi,[esp+12]
	mov	eax,[esi]
	or	eax,eax
	jns	eax_ok2
	neg	eax
eax_ok2:

	mov	ebx,[esi+4]
	or	ebx,ebx
	jns	ebx_ok2
	neg	ebx
ebx_ok2:

	mov	ecx,[esi+8]
	or	ecx,ecx
	jns	ecx_ok2
	neg	ecx
ecx_ok2:

mag_quick_eax_ebx_ecx:

	cmp	eax,ebx
	jg	no_swap_ab
	xchg	eax,ebx
no_swap_ab:	cmp	ebx,ecx
	jg	do_add
	xchg	ebx,ecx
	cmp	eax,ebx
	jg	do_add
	xchg	eax,ebx

do_add:	sar	ebx,2	;    b*1/4
	sar	ecx,3	;            c*1/8
	add	ebx,ecx	;    b*1/4 + c*1/8
	add	eax,ebx	;a + b*1/4 + c*1/8
	sar	ebx,1	;    b*1/8 + c*1/16
	add	eax,ebx	;a + b*3/4 + c*3/16

	pop	esi
	pop	ebx
	ret


;computes an approximation of the distance between two points.
;uses dist = largest + next_largest*3/8 + smallest*3/16
	align	4
_vm_vec_dist_quick:
	push	ebx
        push    esi
	push	edi

	mov	esi,[esp+16]
	mov	edi,[esp+20]

	mov	ebx,[esi]
	sub	ebx,[edi]
	jns	ebx_ok
	neg	ebx
ebx_ok:

	mov	ecx,[esi+4]
	sub	ecx,[edi+4]
	jns	ecx_ok
	neg	ecx
ecx_ok:
	mov	eax,[esi+8]
	sub	eax,[edi+8]
	jns	eax_ok
	neg	eax
eax_ok:
	pop	edi
	jmp	mag_quick_eax_ebx_ecx


;return the normalized direction vector between two points
;dest = normalized(end - start).
;takes edi=dest, esi=endpoint, ebx=startpoint.  Returns mag of dir vec
;NOTE: the order of the parameters matches the vector subtraction
_vm_vec_normalized_dir:
	push	ebx
	push	esi
	push	edi
	push	ebp
	mov	edi,[esp+20]
	mov	esi,[esp+24]
	mov	ebp,[esp+28]

	mov	eax,[esi]
	sub	eax,[ebp]
	mov	[edi],eax
	imul	eax
	mov	ebx,eax
	mov	ecx,edx

	mov	eax,[esi+4]
	sub	eax,[ebp+4]
	mov	[edi+4],eax
	imul	eax
	add	ebx,eax
	adc	ecx,edx

	mov	eax,[esi+8]
	sub	eax,[ebp+8]
	mov	[edi+8],eax
	imul	eax
	add	eax,ebx
	adc	edx,ecx

	call	quad_sqrt_asm

	mov	ecx,eax	;mag in ecx
	jecxz	no_div2

%assign i 0
%rep 3
	mov eax,[edi+i]
	fixdiv ecx
	mov [edi+i],eax
%assign i i+4
%endrep
no_div2:
	mov	eax,ecx
	pop	ebp
	pop	edi
	pop	esi
	pop	ebx
	ret


;normalize a vector in place.
;returns mag(!)
_vm_vec_normalize:
	push	ebx
	push	esi
        push    edi
	mov	edi,[esp+16]
	mov	esi,edi
	jmp	vm_vec_copy_normalize_nopar

;normalize a vector.  takes edi=dest, esi=vector
;returns ecx=mag of source vec
_vm_vec_copy_normalize:
	push	ebx
	push	esi
	push	edi
	mov	edi,[esp+16]
	mov	esi,[esp+20]
vm_vec_copy_normalize_nopar:

	mov	eax,[esi]
	imul	eax
	mov	ebx,eax
	mov	ecx,edx

	mov	eax,[esi+4]
	imul	eax
	add	ebx,eax
	adc	ecx,edx

	mov	eax,[esi+8]
	imul	eax
	add	eax,ebx
	adc	edx,ecx

	call	quad_sqrt_asm

	mov	ecx,eax	;mag in ecx
	jecxz	no_div

%assign i 0
%rep 3
	mov eax,[esi+i]
	fixdiv ecx
	mov [edi+i],eax
%assign i i+4
%endrep
no_div:
	mov	eax,ecx
	pop	edi
	pop	esi
	pop	ebx
	ret


;normalize a vector in place.
;uses approx. dist
_vm_vec_normalize_quick:
	push	esi
	push	edi
	mov	edi,[esp+12]
	mov	esi,edi
	jmp	vm_vec_copy_normalize_quick_nopar

;save as vm_vec_normalized_dir, but with quick sqrt
;takes dest, endpoint, startpoint.  Returns mag of dir vec
_vm_vec_normalized_dir_quick:
        push    esi
        push    edi
        mov     edi,[esp+12]
        mov     esi,[esp+16]
        mov     edx,[esp+20]
%assign i 0
%rep 3
	mov eax,[esi+i]
	sub eax,[edx+i]
	mov [edi+i],eax
%assign i i+4
%endrep
	mov	esi,edi
        jmp     vm_vec_copy_normalize_quick_nopar

;normalize a vector.
;uses approx. dist
_vm_vec_copy_normalize_quick:
	push	esi
	push	edi

	mov	edi,[esp+12]
	mov	esi,[esp+16]
vm_vec_copy_normalize_quick_nopar:
	push	esi
	call	_vm_vec_mag_quick
	pop	ecx	;remove par

	mov	ecx,eax	;mag in ecx
	jecxz	no_div_q

%assign i 0
%rep 3
	mov eax,[esi+i]
	fixdiv ecx
	mov [edi+i],eax
%assign i i+4
%endrep

no_div_q:
	mov eax,ecx
	pop edi
	pop esi
	ret


;compute dot product of two vectors. takes esi,edi=vectors, returns eax=dotprod
_vm_vec_dotprod:
	push	ebx
	push	esi
	push	edi
	mov	esi,[esp+16]
	mov	edi,[esp+20]

	mov	eax,[esi]
	imul	dword [edi]
	mov	ebx,eax
	mov	ecx,edx

	mov	eax,[esi+4]
	imul	dword [edi+4]
	add	ebx,eax
	adc	ecx,edx

	mov	eax,[esi+8]
	imul	dword [edi+8]
	add	eax,ebx
	adc	edx,ecx

	shrd	eax,edx,16

	;ifndef	NDEBUG	;check for overflow
	;always do overflow check, and return saturated value
	sar	edx,16	;get real sign from high word
	mov	ebx,edx
	cdq		;get sign of our result 
	cmp	bx,dx	;same sign?
	je	no_oflow
	;;debug_brk	'overflow in vm_vec_dotprod'
	mov	eax,7fffffffh
	or	ebx,ebx	;check desired sign
	jns	no_oflow
	neg	eax
no_oflow:
	;endif
	pop	edi
	pop	esi
	pop	ebx
	ret


;computes cross product of two vectors.
;Note: this magnitude of the resultant vector is the
;product of the magnitudes of the two source vectors.  This means it is
;quite easy for this routine to overflow and underflow.  Be careful that
;your inputs are ok.
; takes dest, src vectors
; returns dest
_vm_vec_crossprod:
	push	ebx
	push	esi
	push	edi
	push	ebp
	mov	ebp,[esp+20]
	mov	esi,[esp+24]
	mov	edi,[esp+28]
	%ifndef  NDEBUG
	 cmp	ebp,esi
         break_if       e,'crossprod: dest==src0'
	 cmp	ebp,edi
         break_if       e,'crossprod: dest==src1'
        %endif

	mov	eax,[edi+4]
	imul	dword [esi+8]
	mov	ebx,eax
	mov	ecx,edx
	mov	eax,[edi+8]
	imul	dword [esi+4]
	sub	eax,ebx
	sbb	edx,ecx
	shrd	eax,edx,16
	%ifndef  NDEBUG  ;check for overflow
	 mov	ebx,edx	;save
	 cdq		;get sign of result
	 shr	ebx,16	;get high 16 of quad result
	 cmp	dx,bx	;sign extension the same?
	 break_if	ne,'overflow in crossprod'
	%endif
	mov	[ebp],eax

	mov	eax,[edi+8]
	imul	dword [esi]
	mov	ebx,eax
	mov	ecx,edx
	mov	eax,[edi]
	imul	dword [esi+8]
	sub	eax,ebx
	sbb	edx,ecx
	shrd	eax,edx,16
	%ifndef  NDEBUG  ;check for overflow
	 mov	ebx,edx	;save
	 cdq		;get sign of result
	 shr	ebx,16	;get high 16 of quad result
	 cmp	dx,bx	;sign extension the same?
	 break_if	ne,'overflow in crossprod'
	%endif
	mov	[ebp+4],eax

	mov	eax,[edi]
	imul	dword [esi+4]
	mov	ebx,eax
	mov	ecx,edx
	mov	eax,[edi+4]
	imul	dword [esi]
	sub	eax,ebx
	sbb	edx,ecx
	shrd	eax,edx,16
	%ifndef  NDEBUG  ;check for overflow
	 mov	ebx,edx	;save
	 cdq		;get sign of result
	 shr	ebx,16	;get high 16 of quad result
	 cmp	dx,bx	;sign extension the same?
	 break_if	ne,'overflow in crossprod'
	%endif
	mov	[ebp+8],eax

	mov	eax,ebp	;return dest in eax

	pop	ebp
	pop	edi
	pop	esi
	pop	ebx
	ret


;computes surface normal from three points. takes ebx=dest, eax,esi,edi=vecs
;returns eax=dest. Result vector is normalized.
_vm_vec_normal:
	push	dword [esp+16+00];src2
	push	dword [esp+12+04];src1
	push	dword [esp+08+08];src0
	push	dword [esp+04+12];dest
	call	_vm_vec_perp	 ;get unnormalized
	add	esp,16
	push	eax		 ;dest
	call	_vm_vec_normalize
	pop	eax
	ret


;make sure a vector is reasonably sized to go into a cross product
;takes vector in esi
;trashes eax,ebx,cl,edx
check_vec:
	mov	eax,[esi]
	abs_eax
	mov	ebx,eax
	mov	eax,[esi+4]
	abs_eax
	or	ebx,eax
	mov	eax,[esi+8]
	abs_eax
	or	ebx,eax
	jz	null_vector

	xor	cl,cl	;init shift count

	test	ebx,0fffc0000h	;too big
	jz	not_too_big
check_4_down:	test	ebx,000f00000h
	jz	check_2_down
	add	cl,4
	sar	ebx,4
	jmp	check_4_down
check_2_down:	test	ebx,0fffc0000h
	jz	not_2_down
	add	cl,2
	sar	ebx,2
	jmp	check_2_down
not_2_down:
	sar	dword [esi],cl
	sar	dword [esi+4],cl
	sar	dword [esi+8],cl
	ret

;maybe too small...
not_too_big:	test	ebx,0ffff8000h
	jnz	not_too_small
check_4_up:	test	ebx,0fffff000h
	jnz	check_2_up
	add	cl,4
	sal	ebx,4
	jmp	check_4_up
check_2_up:	test	ebx,0ffff8000h
	jnz	not_2_up
	add	cl,2
	sal	ebx,2
	jmp	check_2_up
not_2_up:
	sal	dword [esi],cl
	sal	dword [esi+4],cl
	sal	dword [esi+8],cl

not_too_small:	ret

null_vector:
; debug_brk commented out by mk on 05/04/94
;**	debug_brk	"null vector in check_vec"
	ret


;computes surface normal from three points. takes ebx=dest, eax,esi,edi=vecs
;returns eax=dest. Result vector is NOT normalized, but this routine does
;make an effort that cross product does not overflow or underflow  
_vm_vec_perp:
	push	ebx
	push	esi
	push	edi
	; skip dest
	mov	ebx,[esp+20] ;src0
	mov	ecx,[esp+24] ;src1
	mov	edx,[esp+28] ;src2

	mov	esi,tempv0
	mov	edi,tempv1
%assign i 0 ;tempv1=src2-src1
%rep 3
	mov	eax,[edx+i]
	sub	eax,[ecx+i]
	mov	[edi+i],eax
%assign i i+4
%endrep
%assign i 0 ;tempv0=src1-src0
%rep 3
        mov     eax,[ecx+i]
        sub     eax,[ebx+i]
	mov	[esi+i],eax
%assign i i+4
%endrep
        ; esi=tempv0, edi=tempv1
	call	check_vec	;make sure reasonable value
	xchg	esi,edi
	call	check_vec	;make sure reasonable value
	; edi=tempv0, esi=tempv1
	push	esi		 ;tempv1
	push	edi		 ;tempv0
	push	dword [esp+16+8] ;dest
	call	_vm_vec_crossprod
	add	esp,12
	; eax=dest
	pop	edi
	pop	esi
	pop	ebx
	ret


;compute a rotation matrix from three angles. takes edi=dest matrix,
;esi=angles vector.  returns dest matrix.
_vm_angles_2_matrix:
	push	ebx
	push	esi
	push	edi
	mov	edi,[esp+16];dest
	mov	esi,[esp+20];angles

;get sines & cosines
	mov	ax,[esi+pitch]
	call	fix_sincos_asm
	mov	[sinp],eax
	mov	[cosp],ebx

	mov	ax,[esi+bank]
	call	fix_sincos_asm
	mov	[sinb],eax
	mov	[cosb],ebx

	mov	ax,[esi+head]
	call	fix_sincos_asm
	mov	[sinh],eax
	mov	[cosh],ebx

;alternate entry point with sines & cosines already computed.  
;Note all the registers already pushed.
sincos_2_matrix:

;now calculate the 9 elements

	mov	eax,[sinb]
	fixmul	dword [sinh]
	mov	ecx,eax	;save sbsh
	fixmul	dword [sinp]
	mov	ebx,eax
	mov	eax,[cosb]
	fixmul	dword [cosh]
	mov	esi,eax	;save cbch
	add	eax,ebx
	mov	[edi+m1],eax	;m1=cbch+sbspsh

	mov	eax,esi	;get cbch
	fixmul	dword [sinp]
	add	eax,ecx	;add sbsh
	mov	[edi+m8],eax	;m8=sbsh+cbchsp


	mov	eax,[cosb]
	fixmul	dword [sinh]
	mov	ecx,eax	;save cbsh
	fixmul	dword [sinp]
	mov	ebx,eax
	mov	eax,[sinb]
	fixmul	dword [cosh]
	mov	esi,eax	;save sbch
	sub	ebx,eax
	mov	[edi+m2],ebx	;m2=cbshsp-sbch

	mov	eax,esi	;get sbch
	fixmul	dword [sinp]
	sub	eax,ecx	;sub from cbsh
	mov	[edi+m7],eax	;m7=sbchsp-cbsh


	mov	eax,[sinh]
	fixmul	dword [cosp]
	mov	[edi+m3],eax	;m3=shcp

	mov	eax,[sinb]
	fixmul	dword [cosp]
	mov	[edi+m4],eax	;m4=sbcp

	mov	eax,[cosb]
	fixmul	dword [cosp]
	mov	[edi+m5],eax	;m5=cbcp

	mov	eax,[sinp]
	neg	eax
	mov	[edi+m6],eax	;m6=-sp

	mov	eax,[cosh]
	fixmul	dword [cosp]
	mov	[edi+m9],eax	;m9=chcp

	mov	eax,edi
	pop	edi
	pop	esi
	pop	ebx
        ret

%macro m2m 2
	mov eax,%2
	mov %1,eax
%endmacro

%macro m2m_neg 2
	mov eax,%2
	neg eax
        mov %1,eax
%endmacro

;create a rotation matrix from one or two vectors. 
;requires forward vec, and assumes zero bank if up & right vecs==NULL
;up/right vector need not be exactly perpendicular to forward vec
;takes edi=matrix, esi=forward vec, eax=up vec, ebx=right vec. 
;returns edi=matrix.  trashes eax,ebx,esi
;Note: this routine loses precision as the forward vector approaches 
;straigt up or down (I think)
_vm_vector_2_matrix:
	push	ebx
	push	esi
	push	edi
	mov	edi,[esp+16]
	mov	esi,[esp+20];fvec
	mov	eax,[esp+24];uvec
	mov	ebx,[esp+28];rvec

	%ifndef  NDEBUG
	 or	esi,esi
	 break_if	z,"vm_vector_2_matrix: forward vec cannot be NULL!"
	%endif

	or	eax,eax	;up vector present?
	jnz	near use_up_vec      ;..yep

	or	ebx,ebx	;right vector present?
	jz	near just_forward_vec	     ;..nope
;use_right_vec
	push	edi	;save matrix

	push	esi
	push	dword zvec
	call	_vm_vec_copy_normalize
	add	esp,8
	or	eax,eax
	jz	near bad_vector3

	push	ebx
	push	dword xvec
	call	_vm_vec_copy_normalize
	add	esp,8
	or	eax,eax
	jz	bad_vector2

	push	dword xvec;src1
	push	dword zvec;src0
	push	dword yvec;dest
	call	_vm_vec_crossprod	 ;get y = z cross x
	add	esp,12

;normalize new perpendicular vector
	push	eax ;get new vec (up) in esi
	call	_vm_vec_normalize
	pop	ecx
	or	eax,eax
	jz	bad_vector2

;now recompute right vector, in case it wasn't entirely perpendiclar

	push	dword zvec;src1
	push	dword yvec;src0
	push	dword xvec;dest
	call	_vm_vec_crossprod	 ;x = y cross z
	add	esp,12

	pop	edi	;get matrix back

	jmp	copy_into_matrix


;one of the non-forward vectors caused a problem, so ignore them and
;use just the forward vector
bad_vector2:
	pop	edi
	lea	esi,[edi+fvec]
	vm_copy esi,zvec
	jmp	just_forward_vec_norm

;use forward and up vectors
use_up_vec:
	push	edi	;save matrix

	push	eax
	push	dword yvec
	call	_vm_vec_copy_normalize
	add	esp,8
	or	eax,eax
        jz      bad_vector2

	push	esi
	push	dword zvec
	call	_vm_vec_copy_normalize
	add	esp,8
	or	eax,eax
	jz	near bad_vector3

	push	dword zvec;src1
	push	dword yvec;src0
	push	dword xvec;dest
	call	_vm_vec_crossprod	 ;get x = y cross z
	add	esp,12

;normalize new perpendicular vector
	push	eax ;get new vec (up) in esi
	call	_vm_vec_normalize
	pop	ecx
	or	eax,eax
	jz	bad_vector2

;now recompute right vector, in case it wasn't entirely perpendiclar

	push	dword xvec;src1
	push	dword zvec;src0
	push	dword yvec;dest
	call	_vm_vec_crossprod	 ;y = z cross x
	add	esp,12

	pop	edi	;get matrix back

copy_into_matrix:
	vm_copy edi+rvec,xvec
	vm_copy edi+uvec,yvec
	vm_copy edi+fvec,zvec
	mov	eax,edi
	pop	edi
	pop	esi
	pop	ebx
	ret

bad_vector3:
	pop	edi
bad_vector:
	mov	eax,edi
	pop	edi
	pop	esi
        pop     ebx
	debug_brk	'0-len vec in vec_2_mat'
	ret

;only the forward vector is present
just_forward_vec:
	push	esi
	lea	esi,[edi+fvec]
	push	esi
	call	_vm_vec_copy_normalize
	add	esp,8
	or	eax,eax
	jz	bad_vector
just_forward_vec_norm:

	mov	eax,[esi]
	or	eax,[esi+8]	;check both x & z == 0
	jnz	not_up

;forward vector is straight up (or down)

	mov	dword [edi+m1],f1_0
	mov	eax,[esi+4]	;get y componant
	cdq		;get y sign
	mov	eax,-f1_0
	xor	eax,edx
	sub	eax,edx	;make sign correct
	mov	[edi+m8],eax
	xor	eax,eax
	mov	[edi+m4],eax
	mov	[edi+m7],eax
	mov	[edi+m2],eax
	mov	[edi+m5],eax
	jmp	done_v2m

not_up:
	m2m	[edi+0],[esi+8]
	mov	dword [edi+4],0
	m2m_neg [edi+8],[esi+0]
	push	edi
	call	_vm_vec_normalize
	pop	ecx

	lea	eax,[edi+uvec]
	push	edi		;scr1 = x
	push	esi		;src0 = z
	push	eax		;dest = y
	call	_vm_vec_crossprod
	add	esp,12

done_v2m:
	mov	eax,edi
	pop	edi
	pop	esi
	pop	ebx
	ret


;multiply (dot) two vectors. assumes dest ptr in ebp, src pointers in esi,edi.
;trashes ebx,ecx,edx
%macro vv_mul  7
;	 1    2  3  4  5  6  7
;macro	 dest,x0,y0,z0,x1,y1,z1
	mov	eax,[esi+%2]
	imul	dword [edi+%5]
	mov	ebx,eax
	mov	ecx,edx

	mov	eax,[esi+%3]
	imul	dword [edi+%6]
	add	ebx,eax
	adc	ecx,edx

	mov	eax,[esi+%4]
	imul	dword [edi+%7]
	add	ebx,eax
	adc	ecx,edx

	shrd	ebx,ecx,16	;fixup ebx
	mov	[ebp+%1],ebx

%endmacro

;rotate a vector by a rotation matrix
;eax=dest vector, esi=src vector, edi=matrix. returns eax=dest vector
_vm_vec_rotate:
	%ifndef  NDEBUG
	 cmp	eax,esi
	 break_if	e,'vec_rotate: dest==src'
	%endif
	push	ebx
	push	esi
	push	edi
	push	ebp
	mov	ebp,[esp+20];dest vec
	mov	esi,[esp+24];src vec
	mov	edi,[esp+28];matrix

;compute x
	vv_mul	0, 0,4,8, m1,m4,m7
	vv_mul	4, 0,4,8, m2,m5,m8
	vv_mul	8, 0,4,8, m3,m6,m9

	mov	eax,ebp	;return eax=dest
	pop	ebp
	pop	edi
	pop	esi
	pop	ebx
	ret


;transpose a matrix in place.  Takes matrix. returns matrix
_vm_transpose_matrix:
	mov	ecx,[esp+4]

	mov	eax,[ecx+m2]
	xchg	eax,[ecx+m4]
	mov	[ecx+m2],eax

	mov	eax,[ecx+m3]
	xchg	eax,[ecx+m7]
	mov	[ecx+m3],eax

	mov	eax,[ecx+m6]
	xchg	eax,[ecx+m8]
	mov	[ecx+m6],eax
	mov	eax,ecx
	ret



;copy and transpose a matrix.  Takes edi=dest, esi=src. returns edi=dest
_vm_copy_transpose_matrix:
	mov	edx,[esp+4];dest
	mov	ecx,[esp+8];src

	mov	eax,[ecx+m1]
	mov	[edx+m1],eax

	mov	eax,[ecx+m2]
	mov	[edx+m4],eax

	mov	eax,[ecx+m3]
	mov	[edx+m7],eax

	mov	eax,[ecx+m4]
	mov	[edx+m2],eax

	mov	eax,[ecx+m5]
	mov	[edx+m5],eax

	mov	eax,[ecx+m6]
	mov	[edx+m8],eax

	mov	eax,[ecx+m7]
	mov	[edx+m3],eax

	mov	eax,[ecx+m8]
	mov	[edx+m6],eax

	mov	eax,[ecx+m9]
	mov	[edx+m9],eax
	mov	eax,edx
	ret



;mulitply 2 matrices, fill in dest.  returns ptr to dest
;takes dest, src0, scr1
_vm_matrix_x_matrix:
	%ifndef  NDEBUG
	 cmp	eax,esi
	 break_if	e,'matrix_x_matrix: dest==src0'
	 cmp	eax,edi
	 break_if	e,'matrix_x_matrix: dest==src1'
	%endif
	push	ebx
	push	esi
	push	edi
	push	ebp
	mov	ebp,[esp+20] ;ebp=dest
	mov	esi,[esp+24] ;esi=src0
	mov	edi,[esp+28] ;edi=src1

;;This code would do the same as the nine lines below it, but I'm sure
;;Mike would disapprove
;;	for	s0,<<m1,m2,m3>,<m4,m5,m6>,<m7,m8,m9>>
;;	 for	s1,<<m1,m4,m7>,<m2,m5,m8>,<m3,m6,m9>>
;;	  vv_mul	@ArgI(1,s0)+@ArgI(1,s1), s0, s1
;;	 endm
;;	endm

	vv_mul	m1, m1,m2,m3, m1,m4,m7
	vv_mul	m2, m1,m2,m3, m2,m5,m8
	vv_mul	m3, m1,m2,m3, m3,m6,m9

	vv_mul	m4, m4,m5,m6, m1,m4,m7
	vv_mul	m5, m4,m5,m6, m2,m5,m8
	vv_mul	m6, m4,m5,m6, m3,m6,m9

	vv_mul	m7, m7,m8,m9, m1,m4,m7
	vv_mul	m8, m7,m8,m9, m2,m5,m8
	vv_mul	m9, m7,m8,m9, m3,m6,m9

	mov	eax,ebp	;eax=ptr to dest
	pop	ebp
	pop	edi
	pop	esi
	pop	ebx
	ret

;computes the delta angle between two vectors
;two entry points: normalized and non-normalized vectors
;takes esi,edi=vectors, eax=optional forward vector
;returns ax=delta angle
;if the forward vector is NULL, the absolute values of the delta angle
;is returned.  If it is specified, the rotation around that vector from
;esi to edi is returned. 

_vm_vec_delta_ang:
	push	ebx
	push	esi
	push	edi
	mov	esi,[esp+16]
	mov	edi,[esp+20]

	push	esi
	call	_vm_vec_normalize
	pop	ecx
	push	edi
	call	_vm_vec_normalize
	pop	ecx
	jmp	do_vda_dot

_vm_vec_delta_ang_norm:
        push    ebx
        push    esi
        push    edi
        mov     esi,[esp+16]
        mov     edi,[esp+20]
do_vda_dot:
	push	edi
	push	esi
	call	_vm_vec_dotprod
	add	esp,8
	call	fix_acos_asm	;now angle in ax
	mov	ebx,[esp+24]	;get forward vec
	or	ebx,ebx	;null?
	jz	done_vda	;..yes
;do cross product to find sign of angle
	push	eax	;save angle
	;esi,edi still set
	push	edi;src1
	push	esi;src0
	push	dword tempv0	  ;new vec
	call	_vm_vec_crossprod
	add	esp,12
	push	ebx	;forward vec
	push	eax	;new vector
	call	_vm_vec_dotprod  ;eax=dotprod
	add	esp,8

	cdq		;get sign
	pop	eax	;get angle
	xor	eax,edx
	sub	eax,edx	;make sign correct
done_vda:
	pop	edi
	pop	esi
	pop	ebx
	ret


;compute a rotation matrix from the forward vector and a rotation around
;that vector. takes esi=vector, ax=angle, edi=matrix. returns edi=dest matrix. 
;trashes esi,eax
_vm_vec_ang_2_matrix:
	push	ebx
	push	esi
	push	edi
	mov	esi,[esp+16]
	mov	eax,[esp+20]
	mov	edi,[esp+24]

	call	fix_sincos_asm
	mov	[sinb],eax
	mov	[cosb],ebx


;extract heading & pitch from vector

	mov	eax,[esi+4]	;m6=-sp
	neg	eax
	mov	[sinp],eax
	fixmul	eax
	sub	eax,f1_0
	neg	eax
	call	long_sqrt_asm	;eax=cp
	sal	eax,8
	mov	[cosp],eax
	mov	ebx,eax

	mov	eax,[esi+0]	;sh
	fixdiv	ebx
	mov	[sinh],eax

	mov	eax,[esi+8]	;ch
	fixdiv	ebx
	mov	[cosh],eax

	jmp	sincos_2_matrix

%if 0
;compute the distance from a point to a plane.  takes the normalized normal
;of the plane (ebx), a point on the plane (edi), and the point to check (esi).
;returns distance in eax
;distance is signed, so negative dist is on the back of the plane
vm_dist_to_plane:
	pushm	esi,edi

	lea	eax,tempv0
	call	vm_vec_sub	;vecs in esi,edi

	mov	esi,eax	;vector plane -> point
	mov	edi,ebx	;normal
	call	vm_vec_dotprod

	popm	esi,edi
	ret

;extract the angles from a matrix.  takes esi=matrix, fills in edi=angvec
vm_extract_angles_matrix:
	pushm	eax,ebx,edx,ecx

;extract heading & pitch from forward vector

	mov	eax,[esi].fvec.z	;ch
	mov	ebx,[esi].fvec.x	;sh

	mov	ecx,ebx	;check for z==x==0
	or	ecx,eax
	jz	zero_head	;zero, use head=0
	call	fix_atan2
zero_head:	mov	[edi].head,ax	;save heading
	
	call	fix_sincos_asm	;get back sh

	push	eax	;save sine
	abs_eax
	mov	ecx,eax	;save abs(sine)
	mov	eax,ebx
	abs_eax		;abs(cos)
	cmp	eax,ecx	;which is larger?
	pop	eax	;get sine back
	jg	use_cos

;sine is larger, so use it
	mov	ebx,eax	;ebx=sine heading
	mov	eax,[esi].m3	;cp = shcp / sh
	jmp	get_cosp

;cosine is larger, so use it
use_cos:

	mov	eax,[esi].fvec.z	;get chcp
get_cosp:	fixdiv	ebx	;cp = chcp / ch


	push	eax	;save cp

	;eax = x (cos p)
	mov	ebx,[esi].fvec.y	;fvec.y = -sp
	neg	ebx	;ebx = y (sin)
	mov	ecx,ebx	;check for z==x==0
	or	ecx,eax
	jz	zero_pitch	;bogus vec, set p=0
	call	fix_atan2
zero_pitch:	mov	[edi].pitch,ax

	pop	ecx	;get cp
	jecxz	cp_zero

	mov	eax,[esi].m4	;m4 = sbcp
	fixdiv	ecx	;get sb
	mov	ebx,eax	;save sb

	mov	eax,[esi].m5	;get cbcp
	fixdiv	ecx	;get cb
	mov	ecx,ebx	;check for z==x==0
	or	ecx,eax
	jz	zero_bank	;bogus vec, set n=0
	call	fix_atan2
zero_bank:	mov	[edi].bank,ax

m_extract_done:
	popm	eax,ebx,edx,ecx

	ret

;the cosine of pitch is zero.  we're pitched straight up. say no bank
cp_zero:	mov	[edi].bank,0	;no bank

	popm	eax,ebx,edx,ecx
	ret


;extract the angles from a vector, assuming zero bank. 
;takes esi=vec, edi=angvec
;note versions for normalized and not normalized vector
;unnormalized version TRASHES ESI
vm_extract_angles_vector:
	push	edi
	lea	edi,tempv0
	call	vm_vec_copy_normalize	;ecx=mag
	mov	esi,edi
	pop	edi
	jecxz	extract_done

vm_extract_angles_vector_normalized:
	pushm	eax,ebx

	mov	[edi].bank,0	;always zero bank

	mov	eax,[esi].y
	neg	eax
	call	fix_asin
	mov	[edi].pitch,ax	;p = asin(-y)

	mov	eax,[esi].z
	mov	ebx,[esi].x
	or	ebx,eax
	jz	zero_head2	;check for up vector

	mov	ebx,[esi].x	;get x again
	call	fix_atan2
zero_head2:	mov	[edi].head,ax	;h = atan2(x,z) (or zero)


	popm	eax,ebx
extract_done:	ret

%endif
