; THE COMPUTER CODE CONTAINED HEREIN IS THE SOLE PROPERTY OF PARALLAX
; SOFTWARE CORPORATION ("PARALLAX").  PARALLAX, IN DISTRIBUTING THE CODE TO
; END-USERS, AND SUBJECT TO ALL OF THE TERMS AND CONDITIONS HEREIN, GRANTS A
; ROYALTY-FREE, PERPETUAL LICENSE TO SUCH END-USERS FOR USE BY SUCH END-USERS
; IN USING, DISPLAYING,  AND CREATING DERIVATIVE WORKS THEREOF, SO LONG AS
; SUCH USE, DISPLAY OR CREATION IS FOR NON-COMMERCIAL, ROYALTY OR REVENUE
; FREE PURPOSES.  IN NO EVENT SHALL THE END-USER USE THE COMPUTER CODE
; CONTAINED HEREIN FOR REVENUE-BEARING PURPOSES.  THE END-USER UNDERSTANDS
; AND AGREES TO THE TERMS HEREIN AND ACCEPTS THE SAME BY USE OF THIS FILE.  
; COPYRIGHT 1993-1999 PARALLAX SOFTWARE CORPORATION.  ALL RIGHTS RESERVED.


.386
	option	oldstructs

	.nolist
	include	pstypes.inc
	include	psmacros.inc
	include	gr.inc
	include	3d.inc
	.list

	assume	cs:_TEXT, ds:_DATA

_DATA	segment	dword public USE32 'DATA'

rcsid	db	"$Id: interp.asm,v 1.1.1.1 2001-01-19 03:29:58 bradleyb Exp $"
	align	4

;table with address for each opcode
opcode_table	dd	op_eof	;0 = eof
	dd	op_defpoints	;1 = defpoints
	dd	op_flatpoly	;2 = flat-shaded polygon
	dd	op_tmappoly	;3 = texture-mapped polygon
	dd	op_sortnorm	;4 = sort by normal
	dd	op_rodbm	;5 = rod bitmap
	dd	op_subcall	;6 = call a subobject
	dd	op_defp_start	;7 = defpoints with start
	dd	op_glow	;8 = glow value for next poly
n_opcodes = ($-opcode_table)/4

bitmap_ptr	dd	?
anim_angles	dd	?	;pointer to angle data

morph_points	dd	?	;alternate points for morph

;light value for the next tmap
glow_num	dd	-1	;-1 means off
glow_values	dd	?	;ptr to array of values

	public	_highest_texture_num
_highest_texture_num dw	0

zero_angles	fixang	0,0,0	;vms_angvec <0,0,0>	;for if no angles specified

rod_top_p	g3s_point <>
rod_bot_p	g3s_point <>

	public	g3d_interp_outline,_g3d_interp_outline
_g3d_interp_outline label	dword
g3d_interp_outline	dd	0

morph_pointlist	dd	?,?,?

morph_uvls	fix	3 dup (?,?,?)

;the light for the current model
model_light	fix	?

;ptr to array of points
Interp_point_list	dd	?

MAX_POINTS_PER_POLY = 25

point_list	dd	MAX_POINTS_PER_POLY dup (?)

MAX_INTERP_COLORS = 100

;this is a table of mappings from RGB15 to palette colors
interp_color_table	dw	MAX_INTERP_COLORS dup (?,?)

n_interp_colors	dd	0
uninit_flag	dd	0


_DATA	ends



_TEXT	segment	dword public USE32 'CODE'

;get and jump to next opcode
next	macro
	xor	ebx,ebx
	mov	bx,[ebp]
	ifndef	NDEBUG
	 cmp	ebx,n_opcodes
	 break_if	ge,'Invalid opcode'
	endif
	mov	ebx,opcode_table[ebx*4]
	jmp	ebx
	endm

;get and call the next opcode
call_next	macro	
	xor	ebx,ebx
	mov	bx,[ebp]
	ifndef	NDEBUG
	 cmp	ebx,n_opcodes
	 break_if	ge,'Invalid opcode'
	endif
	mov	ebx,opcode_table[ebx*4]
	call	ebx
	endm

;give the interpreter an array of points to use
g3_set_interp_points:
	mov	Interp_point_list,eax
	ret

;interpreter to draw polygon model
;takes esi=ptr to object, edi=ptr to array of bitmap pointers, 
;eax=ptr to anim angles, edx=light value, ebx=ptr to array of glow values
g3_draw_polygon_model:
	pushm	eax,ebx,ecx,edx,esi,edi,ebp

	mov	ebp,esi	;ebp = interp ptr

	mov	bitmap_ptr,edi	;save ptr to bitmap array
	mov	anim_angles,eax
	mov	model_light,edx
	mov	glow_values,ebx

	mov	glow_num,-1

;;@@ mov depth,0

	call_next

	popm	eax,ebx,ecx,edx,esi,edi,ebp
	ret

;handlers for opcodes

;end of a model or sub-rountine
op_eof:	ret

;define a list of points
op_defpoints:	xor	ecx,ecx
	mov	cx,2[ebp]	;num points
	mov	edi,Interp_point_list
	lea	esi,4[ebp]
rotate_loop:	call	g3_rotate_point
	add	edi,size g3s_point
	add	esi,size vms_vector
	dec	ecx
	jnz	rotate_loop
	mov	ebp,esi
	next

;define a list of points, with starting point num specified
op_defp_start:	xor	ecx,ecx
	xor	eax,eax
	mov	ax,w 4[ebp]	;starting point num
	imulc	eax,size g3s_point	;get ofs of point
	add	eax,Interp_point_list
	mov	edi,eax
	mov	cx,2[ebp]	;num points
	lea	esi,8[ebp]
	jmp	rotate_loop
	next

;draw a flat-shaded polygon
op_flatpoly:	xor	ecx,ecx
	mov	cx,2[ebp]	;num verts
	lea	esi,4[ebp]	;point
	lea	edi,16[ebp]	;vector
	call	g3_check_normal_facing
	jng	flat_not_facing

;polygon is facing, so draw it

;here the glow parameter is used for the player's headlight
	test	glow_num,-1	;glow override?
	js	no_glow2
	mov	eax,glow_num
	mov	esi,glow_values
	mov	eax,[esi+eax*4]
	mov	glow_num,-1
	cmp	eax,-1	;-1 means draw normal color
	jne	not_normal_color
;use the color specified, run through darkening table
	xor	ebx,ebx
	mov	eax,32	;32 shades
	imul	model_light
	sar	eax,16
	or	eax,eax
	jns	no_sat1
	xor	eax,eax
no_sat1:	cmp	eax,32
	jl	no_sat2
	mov	eax,32
no_sat2:	mov	bh,al	;get lighting table
	xor	eax,eax
	mov	ax,28[ebp]			;get color index
	mov	ax,interp_color_table[eax*4]
	mov	bl,al
	mov	al,gr_fade_table[ebx]
	jmp	got_color_index
not_normal_color:	cmp	eax,-2	;-2 means use white
	jne	not_white
	mov	eax,255	;hack!
	jmp	got_color_index
not_white:	cmp	eax,-3	;-3 means don't draw polygon
	je	flat_not_facing
no_glow2:

	xor	eax,eax
	mov	ax,28[ebp]			;get color index
	mov	ax,interp_color_table[eax*4]
got_color_index:	call	gr_setcolor_			;set it

	lea	esi,30[ebp]	;point number list

	;make list of point pointers

	ifndef	NDEBUG
	 cmp	ecx,MAX_POINTS_PER_POLY
	 break_if	ge,'Too many points in interp poly'
	endif

	lea	edi,point_list
	xor	ebx,ebx
copy_loop:	xor	eax,eax
	mov	ax,w [esi+ebx*2]	;get point number
	imulc	eax,size g3s_point
	add	eax,Interp_point_list
	mov	[edi+ebx*4],eax
	inc	ebx
	cmp	ebx,ecx
	jne	copy_loop
	mov	esi,edi

	call	g3_draw_poly
	xor	ecx,ecx
	mov	cx,2[ebp]	;restore count

	ifndef	NDEBUG
	test	g3d_interp_outline,-1
	jz	no_outline
	pushm	ecx,esi
	lea	esi,30[ebp]
	call	draw_outline
	popm	ecx,esi
no_outline:
	endif	

;polygon is not facing (or we've plotted it). jump to next opcode
flat_not_facing:	and	ecx,0fffffffeh
	inc	ecx	;adjust for pad
	lea	ebp,30[ebp+ecx*2]
	next

;set the glow value for the next tmap
op_glow:	test	glow_values,-1
	jz	skip_glow
	xor	eax,eax
	mov	ax,2[ebp]
	mov	glow_num,eax
skip_glow:	add	ebp,4
	next


;draw a texture map
op_tmappoly:	xor	ecx,ecx
	mov	cx,2[ebp]	;num verts
	lea	esi,4[ebp]	;point
	lea	edi,16[ebp]	;normal
	call	g3_check_normal_facing
	jng	tmap_not_facing

;polygon is facing, so draw it

	xor	edx,edx
	mov	dx,28[ebp]	;get bitmap number
	mov	eax,bitmap_ptr
	mov	edx,[eax+edx*4]

	lea	esi,30[ebp]	;point number list
	mov	eax,ecx
	and	eax,0fffffffeh
	inc	eax
	lea	ebx,30[ebp+eax*2]	;get uvl list

;calculate light from surface normal
	push	esi

	test	glow_num,-1	;glow override?
	js	no_glow
;special glow lighting, which doesn't care about surface normal
	mov	eax,glow_num
	mov	esi,glow_values
	mov	eax,[esi+eax*4]
	mov	glow_num,-1
	jmp	got_light_value
no_glow:
	lea	esi,View_matrix.fvec
	lea	edi,16[ebp]	;normal
	call	vm_vec_dotprod
	neg	eax
;scale light by model light
	push	edx
	mov	edx,eax
	add	eax,eax
	add	eax,edx	;eax *= 3
	sar	eax,2	;eax *= 3/4
	add	eax,f1_0/4	;eax = 1/4 + eax * 3/4
	fixmul	model_light
	pop	edx
;now poke light into l values
got_light_value:	pushm	ecx,ebx
l_loop:	mov	8[ebx],eax
	add	ebx,12
	dec	ecx
	jnz	l_loop
	popm	ecx,ebx
	pop	esi

;now draw it
	;make list of point pointers

	ifndef	NDEBUG
	 cmp	ecx,MAX_POINTS_PER_POLY
	 break_if	ge,'Too many points in interp poly'
	endif

	push	ebx
	lea	edi,point_list
	xor	ebx,ebx
copy_loop2:	xor	eax,eax
	mov	ax,w [esi+ebx*2]	;get point number
	imulc	eax,size g3s_point
	add	eax,Interp_point_list
	mov	[edi+ebx*4],eax
	inc	ebx
	cmp	ebx,ecx
	jne	copy_loop2
	mov	esi,edi
	pop	ebx

	call	g3_draw_tmap
	xor	ecx,ecx
	mov	cx,2[ebp]	;restore count

	ifndef	NDEBUG
	test	g3d_interp_outline,-1
	jz	no_outline2
	pushm	ecx,esi
	lea	esi,30[ebp]
	call	draw_outline
	popm	ecx,esi
no_outline2:
	endif	

;polygon is not facing (or we've plotted it). jump to next opcode
tmap_not_facing:	mov	ebx,ecx
	and	ebx,0fffffffeh
	inc	ebx	;adjust for pad
	lea	ebp,30[ebp+ebx*2]

	mov	eax,ecx
	sal	ecx,1
	add	ecx,eax
	sal	ecx,2	;ecx=ecx*12
	add	ebp,ecx	;point past uvls

	next

;sort based on surface normal
op_sortnorm:	lea	esi,16[ebp]	;point
	lea	edi,4[ebp]	;vector
	call	g3_check_normal_facing
	jng	sortnorm_not_facing

;is facing.  draw back then front

	push	ebp
	xor	eax,eax
	mov	ax,30[ebp]	;get back offset
	add	ebp,eax
	call_next
	mov	ebp,[esp]	;get ebp
	xor	eax,eax
	mov	ax,28[ebp]	;get front offset
	add	ebp,eax
	call_next
	pop	ebp

	lea	ebp,32[ebp]
	next

;is not facing.  draw front then back

sortnorm_not_facing:
	push	ebp
	xor	eax,eax
	mov	ax,28[ebp]	;get back offset
	add	ebp,eax
	call_next
	mov	ebp,[esp]	;get ebp
	xor	eax,eax
	mov	ax,30[ebp]	;get front offset
	add	ebp,eax
	call_next
	pop	ebp

	lea	ebp,32[ebp]
	next

;draw a rod bitmap 
op_rodbm:	lea	esi,20[ebp]	;bot point
	lea	edi,rod_bot_p
	call	g3_rotate_point

	lea	esi,4[ebp]	;top point
	lea	edi,rod_top_p
	call	g3_rotate_point

	lea	esi,rod_bot_p	;esi=bot, edi=top
	mov	eax,16[ebp]	;bot width
	mov	edx,32[ebp]	;top width

	xor	ebx,ebx
	mov	bx,2[ebp]	;get bitmap number
	mov	ecx,bitmap_ptr
	mov	ebx,[ecx+ebx*4]

	call	g3_draw_rod_tmap

	lea	ebp,36[ebp]
	next


;draw a subobject
op_subcall:	xor	eax,eax
	mov	ax,2[ebp]	;get object number

;get ptr to angles
	mov	edi,anim_angles
	or	edi,edi
	jnz	angles_not_null
;angles not specified.  Use zero angles
	lea	edi,zero_angles
	jmp	got_angles
angles_not_null:
	imulc	eax,size vms_angvec
	add	edi,eax
got_angles:
	;angles in edi

	lea	esi,4[ebp]	;get position
	call	g3_start_instance_angles

	push	ebp
	xor	eax,eax
	mov	ax,16[ebp]
	add	ebp,eax	;offset of subobject
	call_next		;draw the subobject
	pop	ebp

	call	g3_done_instance

	lea	ebp,20[ebp]
	next

;takes ax, returns ax
find_color_index:	push	ebx

	;first, see if color already in table

	xor	ebx,ebx	;counter
look_loop:	cmp	ebx,n_interp_colors
	je	must_add_color
	cmp	ax,interp_color_table+2[ebx*4]
	je	found_color
	inc	ebx
	jmp	look_loop

must_add_color:	mov	interp_color_table+2[ebx*4],ax	;save rgb15
	call	gr_find_closest_color_15bpp_
	mov	interp_color_table[ebx*4],ax		;save pal entry
	inc	n_interp_colors

found_color:	mov	eax,ebx	;return index
	pop	ebx
	ret

;this remaps the 15bpp colors for the models into a new palette.  It should
;be called whenever the palette changes
g3_remap_interp_colors:
	pushm	eax,ebx

	xor	ebx,ebx	;index
remap_loop:	cmp	ebx,n_interp_colors
	je	done_remap

	xor	eax,eax
	mov	ax,interp_color_table+2[ebx*4]	;get rgb15
	call	gr_find_closest_color_15bpp_
	mov	interp_color_table[ebx*4],ax		;store pal entry 

	inc	ebx
	jmp	remap_loop

done_remap:	popm	eax,ebx
	ret


;maps the colors back to RGB15
g3_uninit_polygon_model:
	mov	uninit_flag,1

;initialize a polygon object
;translate colors, scale UV values
;takes esi=ptr to model
g3_init_polygon_model:
	mov	_highest_texture_num,-1

	pushm	eax,ebx,ecx,edx,esi,edi
	call	init_loop
	popm	eax,ebx,ecx,edx,esi,edi

	mov	uninit_flag,0
	ret


init_loop:	mov	ax,[esi]	;get opcode
	cmp	ax,0	;eof
	jne	not_eof
	ret
not_eof:

;defpoints
	cmp	ax,1	;defpoints
	jne	not_defpoints
	xor	eax,eax
	mov	ax,2[esi]	;get count
	sal	eax,1
	add	ax,2[esi]	;*3
	lea	esi,4[esi+eax*4]
	jmp	init_loop
not_defpoints:

;flat polygon
	cmp	ax,2
	jne	not_flatpoly
	ifndef	NDEBUG
	cmp 	w 2[esi],3
	break_if	l,'face must have 3 or more points'
	endif
	; The following 3 lines replace the above
	xor	eax, eax
	mov	ax,28[esi]		;get color
	test	uninit_flag,-1
	jz	not_uninit
;unitialize!
	mov	ax,interp_color_table+2[eax*4]
	jmp	cont1
not_uninit:
	call	find_color_index
cont1:	mov	28[esi],ax		;store new color

	xor	ecx,ecx
	mov	cx,2[esi]	;get nverts
	and	ecx,0fffffffeh
	inc	ecx	;adjust for pad
	lea	esi,30[esi+ecx*2]
	jmp	init_loop
not_flatpoly:

;tmap polygon
	cmp	ax,3
	jne	not_tmappoly
	ifndef	NDEBUG
	cmp 	w 2[esi],3
	break_if	l,'face must have 3 or more points'
	endif
	mov	ax,28[esi]	;get bitmap number
	cmp	ax,_highest_texture_num
	jle	not_new
	mov	_highest_texture_num,ax
not_new:	xor	ecx,ecx
	mov	cx,2[esi]	;get nverts
	mov	ebx,ecx
	and	ebx,0fffffffeh
	inc	ebx	;adjust for pad
	lea	esi,30[esi+ebx*2]	;point at uvls

	imul	ecx,12	;size of uvls
	add	esi,ecx	;skip them
	jmp	init_loop

;;@@init_uv_loop:	mov	eax,[esi]	;get u
;;@@	imul	eax,64	;times bitmap w
;;@@	mov	[esi],eax
;;@@	mov	eax,4[esi]	;get v
;;@@	imul	eax,64	;times bitmap h
;;@@	mov	4[esi],eax
;;@@	add	esi,12	;point at next
;;@@	dec	ecx
;;@@	jnz	init_uv_loop
;;@@	jmp	init_loop
not_tmappoly:

;sort
	cmp	ax,4	;sortnorm
	jne	not_sortnorm

	push	esi
	xor	eax,eax
	mov	ax,28[esi]	;get front offset
	add	esi,eax
	call	init_loop
	mov	esi,[esp]
	xor	eax,eax
	mov	ax,30[esi]	;get front offset
	add	esi,eax
	call	init_loop
	pop	esi
	lea	esi,32[esi]

	jmp	init_loop
not_sortnorm:
	cmp	ax,5
	jne	not_rodbm

	add	esi,36
	jmp	init_loop

not_rodbm:
	cmp	ax,6
	jne	not_subcall


	push	esi
	xor	eax,eax
	mov	ax,16[esi]	;get subobj offset
	add	esi,eax
	call	init_loop
	pop	esi

	add	esi,20
	jmp	init_loop
not_subcall:
	cmp	ax,7	;defpoints
	jne	not_defpoints_st
	xor	eax,eax
	mov	ax,2[esi]	;get count
	sal	eax,1
	add	ax,2[esi]	;*3
	lea	esi,8[esi+eax*4]
	jmp	init_loop
not_defpoints_st:
	cmp	ax,8
	jne	not_glow
	add	esi,4
	jmp	init_loop
not_glow:

	debug_brk	"Invalid opcode"

	jmp	init_loop


;takes ecx=count, esi=ptr to point list
draw_outline:	pushm	eax,ebx,ecx,edx,esi,edi

	;NO_INVERSE_TABLE xor	eax,eax
	;NO_INVERSE_TABLE mov	al,gr_inverse_table[7fffh]	;white 
	mov	eax, 255	; bright white
	call	gr_setcolor_

	mov	ebx,esi

	xor	eax,eax
	mov	ax,[ebx]	;get first point
	push	eax	;save it

outline_loop:	xor	esi,esi
	xor	edi,edi
	mov	si,[ebx]
	dec	ecx
	jz	done_loop
	mov	di,2[ebx]
	push	ebx

	imul	esi,size g3s_point
	add	esi,Interp_point_list
	imul	edi,size g3s_point
	add	edi,Interp_point_list

	call	g3_draw_line
	pop	ebx

	add	ebx,2
	jmp	outline_loop

done_loop:	pop	edi	;get first point back

	imul	esi,size g3s_point
	add	esi,Interp_point_list
	imul	edi,size g3s_point
	add	edi,Interp_point_list

	call	g3_draw_line

	popm	eax,ebx,ecx,edx,esi,edi
	ret

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

;Special code to draw morphing objects

;define a list of points
morph_defpoints:	xor	ecx,ecx
	mov	cx,2[ebp]	;num points
	mov	edi,Interp_point_list
	lea	esi,4[ebp]
morph_rotate_list:	lea	eax,[ecx*2+ecx]	;eax=npoint * 3
	lea	esi,[esi+eax*4]	;point past points
	push	esi
	mov	esi,morph_points	;get alternate points
morph_rotate_loop:	call	g3_rotate_point
	add	edi,size g3s_point
	add	esi,size vms_vector
	dec	ecx
	jnz	morph_rotate_loop
	pop	esi	;restore pointer
	mov	ebp,esi
	next

;define a list of points, with starting point num specified
morph_defp_start:	xor	ecx,ecx
	xor	eax,eax
	mov	ax,w 4[ebp]	;starting point num
	imulc	eax,size g3s_point	;get ofs of point
	add	eax,Interp_point_list
	mov	edi,eax
	mov	cx,2[ebp]	;num points
	lea	esi,8[ebp]
	jmp	morph_rotate_list

;draw a flat-shaded polygon
morph_flatpoly:	xor	ecx,ecx
	mov	cx,2[ebp]	;num verts
	lea	esi,4[ebp]	;point
	lea	edi,16[ebp]	;vector

;set color
	xor	eax,eax
	mov	ax,28[ebp]			;get color
	mov	ax,interp_color_table[eax*4]
	call	gr_setcolor_			;set it

;check and draw
	lea	esi,30[ebp]	;point number list

	xor	eax,eax
	lodsw
	imulc	eax,size g3s_point
	add	eax,Interp_point_list
	mov	morph_pointlist,eax
	xor	eax,eax
	lodsw
	imulc	eax,size g3s_point
	add	eax,Interp_point_list
	mov	morph_pointlist+4,eax
	xor	eax,eax
	lodsw
	imulc	eax,size g3s_point
	add	eax,Interp_point_list
	mov	morph_pointlist+8,eax

	cmp	ecx,3	;3 points is good!
	je	flat_got3
	sub	ecx,2	;tri count

flat_tri_loop:	xor	edi,edi	;no normal, must compute
	pushm	ecx,esi
	mov	ecx,3	;always draw triangle
	lea	esi,morph_pointlist
	call	g3_check_and_draw_poly
	popm	ecx,esi

	mov	eax,morph_pointlist+8
	mov	morph_pointlist+4,eax
	xor	eax,eax
	lodsw
	imulc	eax,size g3s_point
	add	eax,Interp_point_list
	mov	morph_pointlist+8,eax

	dec	ecx
	jnz	flat_tri_loop
	jmp	flat_done_draw
	
flat_got3:
	lea	esi,morph_pointlist
	xor	edi,edi	;no normal, must compute
	call	g3_check_and_draw_poly

flat_done_draw:	xor	ecx,ecx
	mov	cx,2[ebp]	;restore count

	and	ecx,0fffffffeh
	inc	ecx	;adjust for pad
	lea	ebp,30[ebp+ecx*2]
	next

;draw a texture map
morph_tmappoly:	xor	ecx,ecx
	mov	cx,2[ebp]	;num verts
	lea	esi,4[ebp]	;point
	lea	edi,16[ebp]	;normal

;get bitmap
	xor	edx,edx
	mov	dx,28[ebp]	;get bitmap number
	mov	eax,bitmap_ptr
	mov	edx,[eax+edx*4]

	lea	esi,30[ebp]	;point number list
	mov	eax,ecx
	and	eax,0fffffffeh
	inc	eax
	lea	ebx,30[ebp+eax*2]	;get uvl list
;calculate light from surface normal
	push	esi
	lea	esi,View_matrix.fvec
	lea	edi,16[ebp]	;normal
	call	vm_vec_dotprod
	neg	eax
;scale light by model light
	push	edx
	mov	edx,eax
	add	eax,eax
	add	eax,edx	;eax *= 3
	sar	eax,2	;eax *= 3/4
	add	eax,f1_0/4	;eax = 1/4 + eax * 3/4
	fixmul	model_light

	or	eax,eax
	jge	not_zero
	xor	eax,eax
not_zero:
	pop	edx
	pop	esi

;now eax=plane light value

	mov	morph_uvls+8,eax
	mov	morph_uvls+20,eax
	mov	morph_uvls+32,eax

	xor	eax,eax
	lodsw
	imulc	eax,size g3s_point
	add	eax,Interp_point_list
	mov	morph_pointlist,eax
	xor	eax,eax
	lodsw
	imulc	eax,size g3s_point
	add	eax,Interp_point_list
	mov	morph_pointlist+4,eax
	xor	eax,eax
	lodsw
	imulc	eax,size g3s_point
	add	eax,Interp_point_list
	mov	morph_pointlist+8,eax

	cmp	ecx,3	;3 points is good!
	jl	tmap_done_draw	;something is bad, abort
	je	tmap_got3
	sub	ecx,2	;tri count

	push	edx

	mov	edx,[ebx]
	mov	morph_uvls,edx
	mov	edx,4[ebx]
	mov	morph_uvls+4,edx

	mov	edx,12[ebx]
	mov	morph_uvls+12,edx
	mov	edx,16[ebx]
	mov	morph_uvls+16,edx

	mov	edx,24[ebx]
	mov	morph_uvls+24,edx
	mov	edx,28[ebx]
	mov	morph_uvls+28,edx

	add	ebx,3*12

	pop	edx


tmap_tri_loop:	xor	edi,edi	;no normal, must compute
	pushm	ebx,edx,ecx,esi
	mov	ecx,3	;always draw triangle
	lea	esi,morph_pointlist
	lea	ebx,morph_uvls
	call	g3_check_and_draw_tmap
	popm	ebx,edx,ecx,esi

	mov	eax,morph_pointlist+8
	mov	morph_pointlist+4,eax
	xor	eax,eax
	lodsw
	imulc	eax,size g3s_point
	add	eax,Interp_point_list
	mov	morph_pointlist+8,eax

	push	edx
	mov	edx,morph_uvls+24
	mov	morph_uvls+12,edx
	mov	edx,morph_uvls+28
	mov	morph_uvls+16,edx

	mov	edx,[ebx]
	mov	morph_uvls+24,edx
	mov	edx,4[ebx]
	mov	morph_uvls+28,edx
	add	ebx,12
	pop	edx

	dec	ecx
	jnz	tmap_tri_loop
	jmp	tmap_done_draw
	
tmap_got3:
;poke in light values
	pusha
tmap_l_loop:	mov	8[ebx],eax
	add	ebx,12
	dec	ecx
	jnz	tmap_l_loop
	popa

	lea	esi,morph_pointlist

	xor	edi,edi	;no normal
	call	g3_check_and_draw_tmap

tmap_done_draw:	xor	ecx,ecx
	mov	cx,2[ebp]	;restore count

;jump to next opcode
	mov	ebx,ecx
	and	ebx,0fffffffeh
	inc	ebx	;adjust for pad
	lea	ebp,30[ebp+ebx*2]

	mov	eax,ecx
	sal	ecx,1
	add	ecx,eax
	sal	ecx,2	;ecx=ecx*12
	add	ebp,ecx	;point past uvls

	next


;interpreter to draw polygon model
;takes esi=ptr to object, edi=ptr to array of bitmap pointers, 
;eax=ptr to anim angles, ebx=alternate points, edx=light value

g3_draw_morphing_model:
	pushm	eax,ebx,ecx,edx,esi,edi,ebp

	mov	bitmap_ptr,edi	;save ptr to bitmap array
	mov	anim_angles,eax
	mov	morph_points,ebx
	mov	model_light,edx

	mov	ebp,esi	;ebp = interp ptr

	;set alternate opcode pointers

	push	opcode_table[1*4]	;defpoints
	push	opcode_table[2*4]	;flatpoly
	push	opcode_table[3*4]	;tmappoly
	push	opcode_table[7*4]	;defp_start

	mov	opcode_table[1*4],offset morph_defpoints
	mov	opcode_table[2*4],offset morph_flatpoly
	mov	opcode_table[3*4],offset morph_tmappoly
	mov	opcode_table[7*4],offset morph_defp_start

	call_next

	pop	opcode_table[7*4]	;defp_start
	pop	opcode_table[3*4]	;tmappoly
	pop	opcode_table[2*4]	;flatpoly
	pop	opcode_table[1*4]	;defpoints

	popm	eax,ebx,ecx,edx,esi,edi,ebp
	ret

_TEXT	ends

	end

