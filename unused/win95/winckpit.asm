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
	.list

	assume	cs:_TEXT, ds:_DATA

_DATA	segment	dword public USE32 'DATA'

rcsid	db	"$Id: winckpit.asm,v 1.1.1.1 2001-01-19 03:30:15 bradleyb Exp $"
	align	4

_DATA	ends



_TEXT	segment	dword public USE32 'CODE'

; gr_winckpit_blt_span
;	blts a span region from source to dest buffer given a span
;	list
;
;	EBX = xmin
;	ECX = xmax
;	ESI = bm_data source at start of y
;	EDI = bm_data dest

PUBLIC gr_winckpit_blt_span
gr_winckpit_blt_span:

	push	ebp

	inc	ecx			; Better for counting and testing

NewSpanBlt:	
	mov	al, [esi+ebx]		; else blt odd pixel then check right
	mov	[edi+ebx], al		 
	inc	ebx
	cmp	ebx, ecx
	jl	NewSpanBlt

	pop	ebp
	ret



; gr_winckpit_blt_span_long
;	blts a span region from source to dest buffer given a span
;	list
;	This uses word optimization, and should be used for spans longer
;	than 10 pixels, and can't be used for spans of 3 pixels or less.
;
;	EBX = xmin
;	ECX = xmax
;	ESI = bm_data source at start of y
;	EDI = bm_data dest

PUBLIC gr_winckpit_blt_span_long
gr_winckpit_blt_span_long:

;	EDX = right word boundary

	push	ebp

	inc	ecx			; Better for counting and testing
	mov	edx, ecx		; Current right word boundary

	test	ebx, 1			; is left boundary odd?
	jz	TestRightBound		; if even check right boundary.

	mov	al, [esi+ebx]		; else blt odd pixel then check right
	mov	[edi+ebx], al		 
	inc	ebx
				
; 	Assured even left boundary and find right word boundary.

TestRightBound:
	test	ecx, 1			; if even, then we have an even bound
	jz	NewSpanBlt2_0
	dec	edx			; if odd, force even boundary		
	jmp	NewSpanBlt2_1		


;	This is a word only blt.  No odd pixels.

NewSpanBlt2_0:			
	mov	ax, [esi+ebx]
	mov	[edi+ebx], ax		; straight out blt.
	add	ebx, 2
	cmp	ebx, edx   		; do up to right word boundary
	jl	NewSpanBlt2_0
	jmp	EndProc


;	Blts word span and odd right byte is needed.

NewSpanBlt2_1:
	mov	ax, [esi+ebx]
	mov	[edi+ebx], ax		; straight out blt.
	add	ebx, 2
	cmp	ebx, edx   		; do up to right word boundary
	jl	NewSpanBlt2_1
	mov	al, [esi+ebx]		; blt right odd pixel.
	mov	[edi+ebx], al		 

EndProc:
	pop	ebp
	ret

_TEXT	ends

	end

