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

OPTION OLDSTRUCTS
INCLUDE VGAREGS.INC
INCLUDE GR.INC

_DATA   SEGMENT BYTE PUBLIC USE32 'DATA'

		PUBLIC      __A0000
		__A0000      dw  ?

		BufferPtr   dd  0
		BufferSeg   dw  0
		GoalMode    dw  ?
		LastPage    db  0FFh

		BPR         dw  ?
		TempReg     dd  ?

		; Information from VESA return SuperVGA Information

		VESAVersion         dw  ?
		OEMStringPtr        dd  ?
		Capabilities        dd  ?
		VideoModePtr        dd  ?
		TotalMemory         dw  ?
		WinGranularity      dw  ?
		WinSize             dw  ?
		WinFuncPtr          dd  ?
		PageSizeShift       db  ?
		WinAttributes       dw	?
		

		VESA_Signature      = 041534556h

REALREGS    STRUCT
		RealEDI     dd      ?
		RealESI     dd      ?
		RealEBP     dd      ?
		Reserved    dd      ?
		RealEBX     dd      ?
		RealEDX     dd      ?
		RealECX     dd      ?
		RealEAX     dd      ?
		RealFlags   dw      ?
		RealES      dw      ?
		RealDS      dw      ?
		RealFS      dw      ?
		RealGS      dw      ?
		RealIP      dw      ?
		RealCS      dw      ?
		RealSP      dw      ?
		RealSS      dw      ?
REALREGS    ENDS

		regs    REALREGS    < >

		vesa_error	dd	?
		SourceInc   dd  ?
		DestInc     dw  ?
		RowWidth    dd  ?

		extern _gr_var_color:dword, _gr_var_bwidth:dword


_DATA   ENDS

DGROUP  GROUP _DATA


_TEXT   SEGMENT BYTE PUBLIC USE32 'CODE'

		ASSUME  DS:_DATA
		ASSUME  CS:_TEXT

MyStosd MACRO Width:REQ
; Assumes:   EDI = Dest Address
;            Width = a 32-bit value, can't be ECX or EDI
; Trashes:   ECX will be zero
;            EDI = Dest Address + Width
;            EDX = ????
;            Width
LOCAL Aligned
			cmp     Width, 16
			jbe     Aligned
			mov     ecx, edi
			and     ecx, 3
			jcxz    Aligned
			neg     ecx
			add     ecx, 4
			sub     Width, ecx
			rep     stosb
Aligned:    		mov     ecx, Width
			shr     ecx, 2
			rep     stosd
			mov     ecx, Width
			and     ecx, 3
			rep     stosb
ENDM


ENTER_PROC  MACRO
			push    esi
			push    edi
			push    ebp
			push    eax
			push    ebx
			push    ecx
			push    edx
ENDM

EXIT_PROC   MACRO

			cmp     [esp-4], edx
			je      @f
			; YOU TRASHED EDX !!!!!!!
			int     3
@@:         pop     edx

			cmp     [esp-4], ecx
			je      @f
			; YOU TRASHED ECX !!!!!!!
			int     3
@@:         pop     ecx

			cmp     [esp-4], ebx
			je      @f
			; YOU TRASHED EBX !!!!!!!
			int     3
@@:         pop     ebx

			cmp     [esp-4], eax
			je      @f
			; YOU TRASHED EAX !!!!!!!
			int     3
@@:         pop     eax

			cmp     [esp-4], ebp
			je      @f
			; YOU TRASHED EBP !!!!!!!
			int     3
@@:         pop     ebp

			cmp     [esp-4], edi
			je      @f
			; YOU TRASHED EDI !!!!!!!
			int     3
@@:         pop     edi

			cmp     [esp-4], esi
			je      @f
			; YOU TRASHED ESI !!!!!!!
			int     3
@@:         pop     esi

ENDM


MyMovsd MACRO Width:REQ
; Assumes:   EDI = Dest Address
;            ESI = Source Address
;            Width = a 32-bit value, can't be ECX or EDI or ESI
;            Assumes that ESI is already aligned
; Trashes:   ECX will be zero
;            EDI = Dest Address + Width
;            ESI = Source Address + Width
;            EDX = ????
LOCAL Aligned
			cmp     Width, 16
			jbe     Aligned
			mov     ecx, edi
			and     ecx, 3
			jcxz    Aligned
			neg     ecx
			add     ecx, 4
			sub     Width, ecx
			rep     movsb
Aligned:    mov     ecx, Width
			shr     ecx, 2
			rep     movsd
			mov     ecx, Width
			and     ecx, 3
			rep     movsb
ENDM


EBXFarTo32:
			push    ecx
			mov     ecx, ebx
			and     ecx, 0FFFF0000h
			shr     ecx, 12
			and     ebx, 0FFFFh
			add     ebx, ecx
			pop     ecx
			ret

PUBLIC  gr_init_A0000_

gr_init_A0000_:

			push    ebx
			mov     ax, 0002h
			mov     bx, 0a000h
			int     31h
			jc      NoGo
			mov     __A0000, ax
			pop     ebx
			xor     eax, eax
			ret
NoGo:       pop     ebx
			mov     eax, 1
			ret

PUBLIC  gr_vesa_checkmode_

gr_vesa_checkmode_:
			pushad

			mov     GoalMode, ax
			cmp 	BufferSeg, 0
			jne	GotDosMemory

			; Allocate a 256 byte block of DOS memory using DPMI
			mov     ax, 0100h
			mov     bx, 16
			int     31h
			jc      NoMemory

			; AX = real mode segment of allocated block
			and     eax, 0FFFFh
			mov     BufferSeg, ax
			shl     eax, 4      ; EAX = 32-bit pointer to DOS memory
			mov     BufferPtr, eax
GotDosMemory:


			; Get SuperVGA information
			mov     ax, BufferSeg
			mov     regs.RealEDI, 0
			mov     regs.RealESI, 0
			mov     regs.RealEBP, 0
			mov     regs.Reserved, 0
			mov     regs.RealEBX, 0
			mov     regs.RealEDX, 0
			mov     regs.RealECX, 0
			mov     regs.RealEAX, 04f00h
			mov     regs.RealFlags, 0
			mov     regs.RealES, ax
			mov     regs.RealDS, 0
			mov     regs.RealFS, 0
			mov     regs.RealGS, 0
			mov     regs.RealIP, 0
			mov     regs.RealCS, 0
			mov     regs.RealSP, 0
			mov     regs.RealSS, 0

			mov     bl, 10h
			xor     bh, bh
			xor     ecx, ecx
			mov     edi, offset regs
			mov     ax, 0300h
			int     31h

			mov     eax, regs.RealEAX
			cmp     ax, 04fh
			jne     NoVESADriver

			; Make sure there is a VESA signature
			mov     eax, BufferPtr
			cmp     dword ptr[eax+0], VESA_Signature
			jne     NoVESADriver

			; We now have a valid VESA driver loaded

			mov     bx, word ptr [eax+4]
			mov     VESAVersion, bx

			mov     ebx, dword ptr [eax+6]
			call    EBXFarTo32
			mov     OEMStringPtr, ebx

			mov     ebx, dword ptr [eax+10]
			mov     Capabilities, ebx

			mov     bx, word ptr [eax+18]
			mov     TotalMemory, bx

			mov     ebx, dword ptr [eax+14]
			call    EBXFarTo32
			mov     VideoModePtr, ebx

TryAnotherMode:
			mov     ax, word ptr [ebx]
			add     ebx, 2
			cmp     ax, GoalMode
			je      ModeSupported
			cmp     ax, -1
			je      ModeNotSupported
			jmp     TryAnotherMode

ModeSupported:

			; Get SuperVGA information
			mov     ax, BufferSeg
			movzx   ecx, GoalMode
			mov     regs.RealEDI, 0
			mov     regs.RealESI, 0
			mov     regs.RealEBP, 0
			mov     regs.Reserved, 0
			mov     regs.RealEBX, 0
			mov     regs.RealEDX, 0
			mov     regs.RealECX, ecx
			mov     regs.RealEAX, 04f01h
			mov     regs.RealFlags, 0
			mov     regs.RealES, ax
			mov     regs.RealDS, 0
			mov     regs.RealFS, 0
			mov     regs.RealGS, 0
			mov     regs.RealIP, 0
			mov     regs.RealCS, 0
			mov     regs.RealSP, 0
			mov     regs.RealSS, 0

			mov     bl, 10h
			xor     bh, bh
			xor     cx, cx
			mov     edi, offset regs
			mov     ax, 0300h
			int     31h

			mov     eax, regs.RealEAX
			cmp     ax, 04fh
			jne     BadStatus

			; Check if this mode supported by hardware.
			mov     eax, BufferPtr
			mov     bx, [eax]
			bt      bx, 0
			jnc     HardwareNotSupported


			mov     bx, [eax+4]
			cmp     bx, 64
			jne     @f
			mov     PageSizeShift, 0
			jmp     GranularityOK
@@:         		cmp     bx, 32
			jne     @f
			mov     PageSizeShift, 1
			jmp     GranularityOK
@@:         		cmp     bx, 16
			jne     @f
			mov     PageSizeShift, 2
			jmp     GranularityOK
@@:         		cmp     bx, 8
			jne     @f
			mov     PageSizeShift, 3
			jmp     GranularityOK
@@:         		cmp     bx, 4
			jne     @f
			mov     PageSizeShift, 4
			jmp     GranularityOK
@@:         		cmp     bx, 2
			jne     @f
			mov     PageSizeShift, 5
			jmp     GranularityOK
@@:         		cmp     bx, 1
			jne     WrongGranularity
			mov     PageSizeShift, 6

GranularityOK:
			shl     bx, 10
			mov     WinGranularity, bx

			mov     bx, [eax+6]
			mov     WinSize, bx

			mov     ebx, [eax+12]
			call    EBXFarTo32
			mov     WinFuncPtr, ebx

			mov	bl, byte ptr [eax+2]
			mov	bh, byte ptr [eax+3]
			mov	word ptr WinAttributes, bx

			movzx   ebx, word ptr [eax+16]

NoError:
			mov     vesa_error, 0
			jmp     Done

WrongGranularity:
			mov     vesa_error, 2
			jmp     Done

HardwareNotSupported:
			mov     vesa_error, 3
			jmp     Done

ModeNotSupported:
			mov     vesa_error, 4
			jmp     Done

NoVESADriver:
			mov     vesa_error, 5
			jmp     Done

BadStatus:
			mov     vesa_error, 6
			jmp     Done

NoMemory:
			mov     vesa_error, 7
			jmp	Done

DPMIError:
			mov     vesa_error, 8

Done:			popad
			mov	eax, vesa_error

			ret

PUBLIC  gr_get_dos_mem_

gr_get_dos_mem_:
			
		; eax = how many bytes

		push	ebx
		
		mov	ebx, eax
		shr	ebx, 4
		mov	eax, 0100h
		int	31h
		jc	nomem
		and	eax, 0ffffh
		shl	eax, 4
		pop	ebx
		ret

nomem:		pop	ebx
		mov	eax,0
		ret
		



PUBLIC gr_vesa_setmodea_

gr_vesa_setmodea_:

		; eax = mode
		pushad
		mov	LastPage,0ffh	;force page reset  
		mov	ebx, eax				
		mov     eax, 04f02h
		int     10h
		cmp     ax, 04fh
		jne     BadStatus
		jmp	NoError

PUBLIC  gr_vesa_setpage_

gr_vesa_setpage_:

			; EAX = 64K Page number
			and	eax, 0ffh
			cmp     al, LastPage
			jne     @f
			ret
@@:         		mov     LastPage, al
			push    edx
			push    ebx
			push    ecx
			mov     edx, eax
			mov     cl, PageSizeShift
			shl     edx, cl         ; Convert from 64K pages to GranUnit pages.
			push	edx
			xor     ebx, ebx        ; BH=Select window, BL=Window A
			mov     eax, 04f05h     ; AX=Super VGA video memory window control
			int     10h

			; New code to fix those ATI Mach64's with separate 
			; read/write pages. 
			mov	bx, word ptr WinAttributes
			and	ebx, 0110b	; Set if PageA can read and write
			cmp	ebx, 0110b	
			jne	WinACantReadAndWrite
			pop	edx
			pop     ecx
			pop     ebx
			pop     edx
			ret

WinACantReadAndWrite:	; Page A can't read and write, so we need to update page B also!
			pop	edx		; DX=Window
			mov 	ebx, 1		; BH=Select window, BL=Window B
			mov     eax, 04f05h     ; AX=Super VGA video memory window control
			int     10h
			pop     ecx
			pop     ebx
			pop     edx
			ret
      
PUBLIC  gr_vesa_incpage_

gr_vesa_incpage_:
			push	eax
			xor	eax, eax
			mov	al, LastPage
			inc	eax
			call	gr_vesa_setpage_
			pop	eax
			ret

PUBLIC  gr_vesa_setstart_

gr_vesa_setstart_:

			; EAX = First column
			; EDX = First row
			push    ebx
			push    ecx
			mov     ecx, eax
			mov     eax, 4f07h
			xor     ebx, ebx
			int     10h
			pop     ecx
			pop     ebx
			ret


PUBLIC  gr_vesa_setlogical_

gr_vesa_setlogical_:

			; EAX = line width
			push    ebx
			push    ecx
			push    edx

			mov     cx, ax
			mov     ax, 04f06h
			mov     bl, 0
			int     10h
			and     ebx, 0FFFFh
			mov     ax, cx

			pop     edx
			pop     ecx
			pop     ebx
			ret



PUBLIC gr_vesa_scanline_

gr_vesa_scanline_:

			; EAX = x1
			; EDX = x2
			; EBX = y
			; ECX = color

			push    edi
			cld
			cmp     edx, eax
			jge     @f
			xchg    edx, eax

@@:         		mov     edi, ebx
			imul    edi, _gr_var_bwidth
			add     edi, eax        ; EDI = y*bpr+x1
			sub     edx, eax        ; ECX = x2-x1
			inc     edx

			mov     eax, edi
			shr     eax, 16

			call	gr_vesa_setpage_

         		and     edi, 00FFFFh
			or      edi, 0A0000h

			;mov     eax, _Table8to32[ecx*4]
			mov	ch, cl
			mov	ax, cx
			shl	eax, 16
			mov	ax, cx

			; edx = width in bytes
			; edi = dest
			mov     bx, dx
			add     bx, di
			jnc     scanonepage

			sub     dx, bx
			movzx   ecx, dx
			
			shr	ecx, 1			
			rep     stosw
			adc	ecx, ecx
			rep	stosb

			movzx   edx, bx
			cmp     edx, 0
			je      scandone

			call    gr_vesa_incpage_
			mov     edi, 0A0000h

scanonepage:
			movzx   ecx, dx

			shr	ecx, 1			
			rep     stosw
			adc	ecx, ecx
			rep	stosb

scandone:

			pop     edi
			ret


PUBLIC gr_vesa_set_logical_

gr_vesa_set_logical_:

			; EAX = logical width in pixels

			push    ebx
			push    ecx

			mov     ecx, eax
			mov     eax, 04f06h
			mov     bl, 0
			int     10h
			and     ebx, 0ffffh

			movzx   eax, cx

			pop     ecx
			pop     ebx

			ret


PUBLIC gr_vesa_pixel_

gr_vesa_pixel_:
			; EAX = color (in AL)
			; EDX = offset from 0A0000
			push	ebx
			mov	bl, al

			mov	eax, edx
			shr	eax, 16
			and     edx, 0ffffh

			call	gr_vesa_setpage_
			mov     [edx+0A0000h], bl
			pop	ebx
			ret

PUBLIC gr_vesa_bitblt_

gr_vesa_bitblt_:

			; EAX = source_ptr
			; EDX = vesa_address
			; EBX = height
			; ECX = width

			push    edi
			push    esi

			mov     esi, eax        ; Point ESI to source bitmap

			; Set the initial page
			mov     eax, edx            ; Move offset into SVGA into eax
			shr     eax, 16             ; Page = offset / 64K
			call    gr_vesa_setpage_

			mov     edi, edx            ; EDI = offset into SVGA
			and     edi,  0FFFFh        ; EDI = offset into 64K page
			add     edi, 0A0000h        ; EDI = ptr to dest


			mov     edx, _gr_var_bwidth
			sub     edx, ecx            ; EDX = amount to step each row


			mov     eax, ecx

NextScanLine:
			push    eax
			MyMovsd eax
			pop     eax

			dec     ebx
			jz      DoneBlt

			add     di, dx
			jnc     NextScanLine

			; Need to increment page!
			call    gr_vesa_incpage_
			jmp     NextScanLine

DoneBlt:    pop     esi
			pop     edi

			ret

PUBLIC gr_vesa_bitmap_

gr_vesa_bitmap_:

			; EAX = Source bitmap       (LINEAR)
			; EDX = Destination bitmap  (SVGA)
			; EBX = x
			; ECX = y

			push    esi
			push    edi
			push    ebp
			push    es

			push    eax
			mov     eax, [edx].bm_data
			imul    ecx, _gr_var_bwidth
			add     eax, ecx
			add     eax, ebx
			mov     edi, eax            ; EDI = offset into SVGA
			shr     eax, 16
			call    gr_vesa_setpage_

			mov     ax, __A0000
			mov     es, ax
			pop     eax

			mov     esi, [eax].bm_data
			and     edi, 0ffffh

			movzx   ecx, [eax].bm_h

NextScanLine1:
				push    ecx
				movzx   ecx, [eax].bm_w
				mov     bx, cx
				add     bx, di
				jnc     OnePage

				sub     cx,bx
				mov     ebp, ecx
				MyMovsd ebp
				and     edi, 00ffffh        ; IN CASE IT WENT OVER 64K
				mov     cx,bx
				call    gr_vesa_incpage_
				jcxz    DoneWithLine
OnePage:
				mov     ebp, ecx
				MyMovsd ebp
				and     edi, 00ffffh        ; IN CASE IT WENT OVER 64K

DoneWithLine:   mov     bx, [eax].bm_rowsize
				sub     bx, [eax].bm_w
				and     ebx, 0ffffh
				add     esi, ebx
				mov     bx, [edx].bm_rowsize
				sub     bx, [eax].bm_w
				add     di, bx
				jnc     NoPageInc
				call    gr_vesa_incpage_
NoPageInc:  pop     ecx
			dec     ecx
			jz      @f
			jmp     NextScanLine1

@@:

			pop es
			pop ebp
			pop edi
			pop esi
			ret




PUBLIC gr_vesa_update_

gr_vesa_update_:

			; EAX = Source bitmap       (LINEAR)
			; EDX = Destination bitmap  (SVGA)
			; EBX = Old source bitmap   (LINEAR)

			push    ecx
			push    esi
			push    edi
			push    ebp
			push    fs

			push    eax
			mov     eax, [edx].bm_data
			mov     ebp, eax            ; EDI = offset into SVGA
			shr     eax, 16
			call    gr_vesa_setpage_

			mov     ax, __A0000
			mov     fs, ax
			pop     eax

			mov     esi, [eax].bm_data
			and     ebp, 0ffffh

			movzx   ecx, [eax].bm_h

			mov     edi, [ebx].bm_data

			movzx   ebx, [eax].bm_rowsize
			sub     bx, [eax].bm_w
			mov     SourceInc, ebx

			movzx   ebx, [edx].bm_rowsize
			sub     bx, [eax].bm_w
			mov     DestInc, bx

			movzx   ebx, [eax].bm_w
			mov     RowWidth, ebx

NextScanLine3:
				push    ecx
				mov     ecx, RowWidth
				mov     dx, cx
				add     dx, bp
				jnc     OnePage3

				sub     cx,dx
				mov     ebx, esi

InnerLoop3:     repe    cmpsb
				mov     al, [esi-1]
				sub     esi, ebx
				mov     fs:[ebp+esi-1], al       ; EDX = dest + size - bytes to end
				add     esi, ebx
				cmp     ecx, 0
				jne     InnerLoop3

				sub     esi, ebx
				add     ebp, esi
				add     esi, ebx
				and     ebp, 00ffffh        ; IN CASE IT WENT OVER 64K

				mov     cx,dx
				call    gr_vesa_incpage_
				jcxz    DoneWithLine3
OnePage3:
				mov     ebx, esi
				mov     edx, ecx
				and     edx, 11b
				shr     ecx, 2

InnerLoop4:     repe    cmpsd
				mov     eax, [esi-4]
				sub     esi, ebx
				mov     fs:[ebp+esi-4], eax       ; EDX = dest + size - bytes to end
				add     esi, ebx
				cmp     ecx, 0
				jne     InnerLoop4

				mov     ecx, edx
				jecxz   EvenWidth
InnerLoop5:     repe    cmpsb
				mov     al, [esi-1]
				sub     esi, ebx
				mov     fs:[ebp+esi-1], al       ; EDX = dest + size - bytes to end
				add     esi, ebx
				cmp     ecx, 0
				jne     InnerLoop5

EvenWidth:      sub     esi, ebx
				add     ebp, esi
				add     esi, ebx
				and     ebp, 00ffffh        ; IN CASE IT WENT OVER 64K

DoneWithLine3:
				add     esi, SourceInc
				add     edi, SourceInc
				add     bp, DestInc
				jnc     NoPageInc3
				call    gr_vesa_incpage_
NoPageInc3: pop     ecx
			dec     ecx
			jnz     NextScanLine3

			pop     fs
			pop     ebp
			pop     edi
			pop     esi
			pop     ecx
			ret




_TEXT   ENDS


		END


