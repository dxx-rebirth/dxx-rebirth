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
;***************************************************************************
;***************************************************************************
;*****                                                                 *****
;*****                      M O U S E . A S M                          *****
;*****                                                                 *****
;***** Contains routines for a mouse interface.                        *****
;*****                                                                 *****
;*****                                                                 *****
;***** PROCEDURES                                                      *****
;*****                                                                 *****
;***** VARIABLES                                                       *****
;*****                                                                 *****
;*****                                                                 *****
;***** CONSTANTS                                                       *****
;*****                                                                 *****
;*****                                                                 *****
;***************************************************************************
;***************************************************************************

.386

;************************************************************************
;**************** FLAT MODEL DATA SEGMENT STUFF *************************
;************************************************************************

_DATA   SEGMENT BYTE PUBLIC USE32 'DATA'

rcsid   db  "$Id: mouse.asm,v 1.1.1.1 2001-01-19 03:30:14 bradleyb Exp $"

		MOUSE_EVENT STRUCT 2
			MouseDX         dw  ?
			MouseDY         dw  ?
			MouseButtons    dw  ?
			MouseFlags      dw  ?
		MOUSE_EVENT ENDS

		NumberOfButtons dw ?

		MyEvent             MOUSE_EVENT < >
		MyEventHandler      dd  0



_DATA   ENDS

DGROUP  GROUP _DATA


;************************************************************************
;**************** FLAT MODEL CODE SEGMENT STUFF *************************
;************************************************************************

_TEXT   SEGMENT BYTE PUBLIC USE32 'CODE'

		ASSUME  ds:_DATA
		ASSUME  cs:_TEXT

PUBLIC  mouse_init_

mouse_init_:

		push    bx
		xor     ax, ax      ; Reset mouse
		int     33h
		mov     NumberOfButtons, bx
		pop     bx
		or      ax, ax
		jnz     present
		jmp     absent


present:
		mov     ax, 0020h   ; Enable driver
		int     33h

		mov     ax, 1
		ret

absent:
		mov     ax, 0
		ret



PUBLIC  mouse_get_pos_

mouse_get_pos_:

		push    eax
		push    ebx
		push    ecx
		push    edx

		push    eax

		mov     ax, 03h     ; Get Mouse Position and Button Status
		int     33h
		; bx = buttons, cx = x, dx = y

		pop     eax
		mov     [eax], cx
		pop     eax
		mov     [eax], dx
		mov     edx, eax

		pop     ecx
		pop     ebx
		pop     eax

		ret

PUBLIC  mouse_get_delta_

mouse_get_delta_:

		push    eax
		push    ebx
		push    ecx
		push    edx


		push    eax

		mov     eax, 0bh    ; Read Mouse Motion Counters
		int     33h

		pop     eax
		mov     [eax], cx
		pop     eax
		mov     [eax], dx
		mov     edx, eax

		pop     ecx
		pop     ebx
		pop     eax
		ret


PUBLIC  mouse_get_btns_

mouse_get_btns_:

		push    ebx
		push    ecx
		push    edx

		mov     ax, 03h     ; Get Mouse Position and Button Status
		int     33h
		; bx = buttons, cx = x, dx = y

		movzx   eax, bx

		pop     edx
		pop     ecx
		pop     ebx

		ret


PUBLIC  mouse_close_

mouse_close_:

		push    eax
		push    ebx
		push    es

		mov     ax, 01fh    ; Disable mouse driver
		int     33h

		pop     es
		pop     ebx
		pop     eax

		ret

MyHandler:

		pushad
		push    ds

		push    ax
		mov     ax, DGROUP
		mov     ds, ax
		pop     ax

		mov     MyEvent.MouseDX, cx
		mov     MyEvent.MouseDY, dx
		mov     MyEvent.MouseButtons, bx
		mov     MyEvent.MouseFlags, ax

		mov     eax, offset MyEvent

		call    MyEventHandler

		pop     ds
		popad

		retf

PUBLIC  mouse_set_handler_

mouse_set_handler_:

		push    eax
		push    ebx
		push    ecx
		push    edx
		push    es

		mov     MyEventHandler, edx

		mov     ecx, eax    ; Event flags
		mov     edx, MyHandler
		mov     ax, cs
		mov     es, ax
		mov     ax, 0Ch     ; Set User-defined Mouse Event Handler
		int     33h

		pop     es
		pop     edx
		pop     ecx
		pop     edx
		pop     eax

		ret

PUBLIC  mouse_clear_handler_

mouse_clear_handler_:

		push    eax
		push    ebx
		push    ecx
		push    edx
		push    es

		mov     MyEventHandler, 0

		mov     ecx, 0    ; Event flags
		mov     edx, MyHandler
		mov     ax, cs
		mov     es, ax
		mov     ax, 0Ch     ; Set User-defined Mouse Event Handler
		int     33h

		pop     es
		pop     edx
		pop     ecx
		pop     edx
		pop     eax

		ret


PUBLIC  mouse_set_limits_

mouse_set_limits_:

		; EAX = minx
		; EDX = miny
		; EBX = maxx
		; ECX = maxy

		push    edx     ; Save Vertical stuff
		push    ecx

		mov     cx, ax
		mov     dx, bx
		mov     ax, 7
		int     33h

		pop     edx
		pop     ecx
		mov     ax, 8
		int     33h

		ret


;extern void mouse_set_pos( short x, short y);


PUBLIC  mouse_set_pos_

mouse_set_pos_:

		; EAX = x
		; EDX = y

		push    ecx

		mov     ecx, eax
		mov     eax, 04h
		int     33h
		pop     ecx


		ret



_TEXT   ENDS


		END
