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
; $Log: not supported by cvs2svn $
; Revision 1.2  1994/02/17  15:55:59  john
; Initial version
; 
; Revision 1.20  1994/01/18  10:58:55  john
; *** empty log message ***
; 
; Revision 1.19  1993/12/22  13:28:40  john
; Added back changes Matt made in r1.14
; 
; Revision 1.18  1993/12/22  13:18:32  john
; *** empty log message ***
; 
; Revision 1.17  1993/12/20  16:48:47  john
; Put cli/sti around clear keybuffer in key_close
; 
; Revision 1.16  1993/12/20  15:39:13  john
; Tried to neaten handler code... also, moved some cli's and sti's around
; trying to find bug.  Made the code call key_get_milliseconds instead
; of timer_get_milliseconds, because we don't want the cli and sti
; stuff in the interrupt handler.  
; 
; Revision 1.15  1993/12/02  10:54:48  john
; Made the Ctrl,Shift,Alt keys buffer like all the other keys.
; 
; Revision 1.14  1993/10/29  11:25:18  matt
; Made key_down_time() not accumulate time if shift,alt,ctrl down
; 
; Revision 1.13  1993/10/29  10:47:00  john
; *** empty log message ***
; 
; Revision 1.12  1993/10/16  19:24:16  matt
; Added new function key_clear_times() & key_clear_counts()
; 
; Revision 1.11  1993/10/15  10:16:49  john
; bunch of stuff, mainly with detecting debugger.
; 
; Revision 1.10  1993/10/04  13:25:57  john
; Changed the way extended keys are processed.
; 
; Revision 1.9  1993/09/28  11:35:32  john
; added key_peekkey
; 
; Revision 1.8  1993/09/23  18:09:23  john
; fixed bug checking for DBG
; 
; Revision 1.7  1993/09/23  17:28:01  john
; made debug check look for DBG> instead of CONTROL
; 
; Revision 1.6  1993/09/20  17:08:19  john
; Made so that keys pressed in debugger don't get passed through to
; the keyboard handler. I also discovered, but didn't fix a error
; (page fault) caused by jumping back and forth between the debugger
; and the program...
; 
; Revision 1.5  1993/09/17  09:58:12  john
; Added checks for already installed, not installed, etc.
; 
; Revision 1.4  1993/09/15  17:28:00  john
; Fixed bug in FlushBuffer that used CX before a REP instead of ECX.
; 
; Revision 1.3  1993/09/08  14:48:00  john
; made getch() return an int instead of a char that has shift states, etc.
; 
; Revision 1.2  1993/07/22  13:12:23  john
; fixed comment
; ,.
; 
; Revision 1.1  1993/07/10  13:10:42  matt
; Initial revision
; 
;
;

;***************************************************************************
;***************************************************************************
;*****                                                                 *****
;*****                                                                 *****
;*****                        K E Y . A S M                            *****
;*****                                                                 *****
;***** Contains routines to get, buffer, and check key presses.        *****
;*****                                                                 *****
;*****                                                                 *****
;***** PROCEDURES                                                      *****
;*****                                                                 *****
;***** key_init()  - Activates the keyboard package.                   *****
;***** key_close() - Deactivates the keyboard package.                 *****
;***** key_check() - Returns 1 if a buffered key is waiting.           *****
;***** key_getch() - Waits for and returns a buffered keypress.        *****
;***** key_flush() - Clears buffers and state array.                   *****
;***** key_time() - Index by scan code. Contains the time key has been *****
;*****             held down. NOT DONE YET.                            *****
;*****                                                                 *****
;*****                                                                 *****
;***** VARIABLES                                                       *****
;*****                                                                 *****
;***** keyd_buffer_type -Set to 0 and key_getch() always returns 0.    *****
;*****                  Set to 1 to so that ASCII codes are returned   *****
;*****                  by key_getch().  Set to 2 and key_getch() returns*****
;*****                  the buffered keyboard scan codes.              *****
;***** keyd_repeat     - Set to 0 to not allow repeated keys in the    *****
;*****                  keyboard buffer.  Set to 1 to allow repeats.   *****
;***** keyd_pressed[] -Index by scan code. Contains 1 if key down else 0*****
;*****                                                                 *****
;*****                                                                 *****
;***** CONSTANTS                                                       *****
;*****                                                                 *****
;***** Setting the DEBUG to 1 at compile time passes SysReq through    *****
;***** to the debugger, and when the debugger is active, it will give  *****
;***** the debugger any keys that are pressed.  Note that this only    *****
;***** works with the Watcom VIDEO debugger at this time.  Setting     *****
;***** DEBUG to 0 takes out all the debugging stuff.                   *****
;*****                                                                 *****
;***************************************************************************
;***************************************************************************

DEBUG EQU 1
.386

;************************************************************************
;**************** FLAT MODEL DATA SEGMENT STUFF *************************
;************************************************************************

_DATA   SEGMENT BYTE PUBLIC USE32 'DATA'

rcsid	db	"$Id: oldkey.asm,v 1.1.1.1 2001-01-19 03:30:14 bradleyb Exp $"

PUBLIC  _keyd_pressed     ; Must start with a _ so C can see the variable.

		_keyd_pressed   db  256 dup (?)

		keybuffer       dw  256 dup (?)     ; Use 256 so an inc wraps around

		TimeKeyWentDown dd  256 dup(0)
		TimeKeyHeldDown dd  256 dup(0)
		NumDowns        dd  256 dup(0)
		NumUps          dd  256 dup(0)

		MyCodeSegment   dw  ?

PUBLIC _keyd_editor_mode

		_keyd_editor_mode db 0

PUBLIC 		_keyd_use_bios

		_keyd_use_bios	db 1

PUBLIC  	_keyd_last_pressed
		_keyd_last_pressed  db 0
PUBLIC  	_keyd_last_released
		_keyd_last_released db 0

PUBLIC 		_keyd_dump_key_array
		_keyd_dump_key_array	db 0
		org_int_sel dw  ?
		org_int_off dd  ?

		interrupted_cs dw  ?
		interrupted_eip dd  ?

		keyhead      db  ?
		keytail      db  ?
PUBLIC  _keyd_buffer_type
PUBLIC  _keyd_repeat
		_keyd_buffer_type db  ?   ; 0=No buffer, 1=buffer ASCII, 2=buffer scans
		_keyd_repeat      db  ?

		E0Flag      db 	0

		Installed   db  0

INCLUDE KEYS.INC


_DATA   ENDS

DGROUP  GROUP _DATA


;************************************************************************
;**************** FLAT MODEL CODE SEGMENT STUFF *************************
;************************************************************************

_TEXT   SEGMENT BYTE PUBLIC USE32 'CODE'

		ASSUME  ds:_DATA
		ASSUME  cs:_TEXT

key_get_milliseconds:
		EXTERNDEF   timer_get_stamp64:NEAR

		push    ebx
		push    edx

		call    timer_get_stamp64

		; Timing in milliseconds
		; Can be used for up to 1000 hours
		shld    edx, eax, 21            ; Keep 32+11 bits
		shl     eax, 21
		mov     ebx, 2502279823         ; 2^21*1193180/1000
		div     ebx

		pop     edx
		pop     ebx

		ret

;************************************************************************
;************************************************************************
;*****                                                              *****
;*****                   K E Y _ T O _ A S C I I _                  *****
;*****                                                              *****
;************************************************************************
;************************************************************************

PUBLIC  key_to_ascii_

key_to_ascii_:

		; EAX = scancode
		push    ebx

		mov     bl, ah
		and     bl, 011111110b
		cmp     bl, 0
		jne     CantDoKey

		cmp     al, 127
		jae     CantDoKey

		and     ah, 01b        ; take away ctrl and alt codes
		shl     al, 1
		shr     eax, 1
		and     eax, 0ffh
		mov     al, byte ptr key1[eax]
		pop     ebx
		ret

CantDoKey:
		pop     ebx
		mov     eax, 255
		ret


public key_clear_times_,key_clear_counts_

;clear the array of key down times.
key_clear_times_:	
	cli
	push	eax
	push	ecx
	push	edi
	xor	eax,eax
	mov	ecx,256
	lea	edi,TimeKeyHeldDown
	rep	stosd	;clear array
	pop	edi
	pop	ecx
	pop	eax
	sti
	ret

;clear the arrays of key down counts
key_clear_counts_:	
	cli
	push	eax
	push	ecx
	push	edi
	xor	eax,eax
	mov	ecx,256
	lea	edi,NumDowns
	rep	stosd	;clear array
	mov	ecx,256
	lea	edi,NumUps
	rep	stosd	;clear array
	pop	edi
	pop	ecx
	pop	eax
	sti
	ret


PUBLIC  key_down_time_

key_down_time_:
		cli

		push    edx
		push    ecx
		push    ebx


		mov     ebx, eax
		xor     eax, eax

		cmp     _keyd_pressed[ebx], 0
		je      NotPressed

		
		cmp	_keyd_editor_mode, 0
		je	read_time

		call    get_modifiers	;shift,alt,ctrl?
		or      ah,ah
		jz      read_time
		xor     eax,eax
		jmp     NotPressed

read_time:	mov     ecx, TimeKeyWentDown[ebx*4]
		call    key_get_milliseconds
		mov     TimeKeyWentDown[ebx*4], eax
		sub     eax, ecx        ; EAX = time held since last

NotPressed:
		add     eax, TimeKeyHeldDown[ebx*4]
		mov     TimeKeyHeldDown[ebx*4], 0

		pop     ebx
		pop     ecx
		pop     edx
		sti
		ret

PUBLIC  key_down_count_
key_down_count_:
		cli
		push    ebx
		mov     ebx, eax
		mov     eax, NumDowns[ebx*4]
		mov     NumDowns[ebx*4], 0
		pop     ebx
		sti
		ret

PUBLIC  key_up_count_
key_up_count_:
		cli
		push    ebx
		mov     ebx, eax
		mov     eax, NumUps[ebx*4]
		mov     NumUps[ebx*4], 0
		pop     ebx
		sti
		ret



;************************************************************************
;************************************************************************
;*****                                                              *****
;*****                   K E Y _ F L U S H                          *****
;*****                                                              *****
;************************************************************************
;************************************************************************

PUBLIC  key_flush_

key_flush_:
		cli

		push    eax
		push    ecx
		push    edi

		mov     keyhead,0
		mov     keytail,255
		mov     E0Flag, 0

		; Clear the keyboard array
		mov     edi, offset _keyd_pressed
		mov     ecx, 32
		mov     eax,0
		rep     stosd

		pop     edi
		pop     ecx
		pop     eax
		sti
		ret

;************************************************************************
;************************************************************************
;*****                                                              *****
;*****                   K E Y _ I N I T                            *****
;*****                                                              *****
;************************************************************************
;************************************************************************

PUBLIC  key_init_

key_init_:
		push    eax
		push    ebx
		push    ds
		push    es

		;**************************************************************
		;******************* INITIALIZE key QUEUE **********************
		;**************************************************************

		mov     _keyd_buffer_type,1
		mov     _keyd_repeat,1
		mov     E0Flag, 0

		; Clear the keyboard array
		call    key_flush_

		cmp     Installed, 0
		jne     AlreadyInstalled

		;**************************************************************
		;******************* SAVE OLD INT9 HANDLER ********************
		;**************************************************************

		mov     Installed, 1

		mov     eax, 03509h             ; DOS Get Vector 09h
		int     21h                     ; Call DOS
		mov     org_int_sel, es         ; Save old interrupt selector
		mov     org_int_off, ebx        ; Save old interrupt offset


		;**************************************************************
		;***************** INSTALL NEW INT9 HANDLER *******************
		;**************************************************************

		mov     eax, 02509h             ; DOS Set Vector 09h
		mov     edx, offset key_handler  ; Point DS:EDX to new handler
		mov     bx, cs
		mov     MyCodeSegment, bx
		mov     ds, bx
		int     21h


AlreadyInstalled:

		pop     es
		pop     ds
		pop     ebx
		pop     eax

		ret


;************************************************************************
;************************************************************************
;*****                                                              *****
;*****                   K E Y _ C L O S E _                          *****
;*****                                                              *****
;************************************************************************
;************************************************************************

PUBLIC  key_close_

key_close_:
		push    eax
		push    ebx
		push    edx
		push    ds


		cmp     Installed, 0
		je      @f

		;**************************************************************
		;***************** RESTORE OLD INT9 HANDLER *******************
		;**************************************************************

		mov     Installed, 0

		; Clear the BIOS buffer
		cli
		mov     ebx, 041ch
		mov     al, byte ptr [ebx]
		mov     ebx, 041ah
		mov     byte ptr [ebx], al
		sti

		mov     eax, 02509h         ; DOS Set Vector 09h
		mov     edx, org_int_off
		mov     ds, org_int_sel
		int     21h

@@:     	pop     ds
		pop     edx
		pop     ebx
		pop     eax

		ret

;************************************************************************
;************************************************************************
;*****                                                              *****
;*****                   K E Y _ C H E C K _                          *****
;*****                                                              *****
;************************************************************************
;************************************************************************

PUBLIC  key_checkch_       ; Must end with a _ so C can see the function.

key_checkch_:
		cli
		push    ebx

		xor     eax, eax
		cmp     Installed, 0
		je      NoKey

		mov     bl, keytail
		inc     bl
		cmp     bl, keyhead
		je      Nokey
		mov     eax, 1
Nokey:
		pop     ebx
		sti
		ret

;************************************************************************
;************************************************************************
;*****                                                              *****
;*****                 K E Y _ D E B U G                              *****
;*****                                                              *****
;************************************************************************
;************************************************************************

PUBLIC  key_debug_
key_debug_:
		int 3h
		ret


;************************************************************************
;************************************************************************
;*****                                                              *****
;*****                   K E Y _ G E T C H _                        *****
;*****                                                              *****
;************************************************************************
;************************************************************************

PUBLIC  key_getch_       ; Must end with a _ so C can see the function.

key_getch_:
		push    ebx

		xor     eax, eax
		xor     ebx, ebx
		cmp     Installed, 0
		jne     StillNoKey
		pop     ebx
		ret

StillNoKey:
		cli             ; Critical section
		mov     bl, keytail
		inc     bl
		cmp     bl, keyhead
		sti
		je      StillNoKey

		cli             ; Critical section
		xor     ebx, ebx
		mov     bl, keyhead
		mov     ax, word ptr keybuffer[ebx*2]
		inc     BYTE PTR keyhead
		sti

		pop     ebx
		ret


;************************************************************************
;************************************************************************
;*****                                                              *****
;*****                   K E Y _ I N K E Y _                        *****
;*****                                                              *****
;************************************************************************
;************************************************************************

PUBLIC  key_inkey_       ; Must end with a _ so C can see the function.

key_inkey_:
		push    ebx

		xor     eax, eax
		xor     ebx, ebx

		cmp     Installed, 0
		je      NoInkey

		cli             ; Critical section
		mov     bl, keytail
		inc     bl
		cmp     bl, keyhead
		sti
		je      NoInkey

		cli             ; Critical section
		mov     bl, keyhead
		mov     ax, word ptr keybuffer[ebx*2]
		inc     BYTE PTR keyhead
		sti
NoInkey:
		pop     ebx
		ret

PUBLIC  key_peekkey_       ; Must end with a _ so C can see the function.

key_peekkey_:
		push    ebx

		xor     eax, eax
		xor     ebx, ebx

		cli             ; Critical section

		cmp     Installed, 0
		je      NoPeek
		mov     bl, keytail
		inc     bl
		cmp     bl, keyhead
		je      NoPeek
		mov     bl, keyhead
		mov     ax, word ptr keybuffer[ebx*2]
		
NoPeek:		sti
		pop     ebx
		ret



;************************************************************************
;************************************************************************
;*****                                                              *****
;*****                   K E Y _ H A N D L E R                      *****
;*****                                                              *****
;************************************************************************
;************************************************************************

PUBLIC  key_handler      ; Must end with a _ so C can see the function.

key_handler:

		pushfd              ; Save flags in case we have to chain to original
		push    eax
		push    ebx
		push    ecx
		push    edx
		push    ds

		mov     ax, DGROUP  ; Point to our data segment, since this is an
		mov     ds, ax      ; interrupt and we don't know where we were.

		mov	eax, (0b0000h+76*2)
		mov 	byte ptr [eax], '1'

IFDEF DEBUG
		call    CheckForDebugger
		jnc     @f
		mov eax, 0b0000h+78*2
		mov byte ptr [eax], 'D'
		jmp      PassToBios      ; If debugger is active, then skip buffer

@@:     	mov eax, 0b0000h+78*2
		mov byte ptr [eax], 'I'

		; Clear the BIOS buffer
		;**mov     ebx, 041ch
		;**mov     al, byte ptr [ebx]
		;**mov     ebx, 041ah
		;**mov     byte ptr [ebx], al
ENDIF

		xor     eax, eax
		xor	ebx, ebx

		in      al, 060h       	 	; Get scan code from keyboard
	
		cmp     al, 0E0h
		jne     NotE0Code

E0Code:		mov     E0Flag, 010000000b
		jmp	LeaveHandler		; If garbage key, then don't buffer it

NotE0Code:	mov	bl, al			; Put break bit into bl	; 0 = pressed, 1=released
		and	al, 01111111b		; AL = scancode
		or	al, E0Flag		; AL = extended scancode
		mov     E0Flag,0		; clear E0 flag
		cmp	al, 029h
		je	pause_execution
		shl	bl, 1			; put upper bit into carry flag
		jc	key_mark_released	; if upper bit of bl was set, then it was a release code

;**************************************************************
;****************** HANDLE A NEWLY PRESSED KEY ****************
;**************************************************************
;Marks the key press in EAX in the scancode array.

key_mark_pressed:
		;cmp	al, 0eh	; backspace
		;je	pause_execution
		
		mov     _keyd_last_pressed, al
		; Check if the key is repeating or if it just got pressed.
		cmp     byte ptr _keyd_pressed[eax], 1
		je      AlreadyDown

;------------------------------- Code for a key pressed for the first time ------------------------
		mov     byte ptr _keyd_pressed[eax], 1	
		; Set the time

		push    edx
		push    eax
		call    key_get_milliseconds
		mov     edx, eax
		pop     eax
		mov     TimeKeyWentDown[eax*4], edx
		pop     edx

		inc     NumDowns[eax*4]

		jmp	BufferAX

;------------------------------- Code for a key that is already pressed ------------------------
AlreadyDown:
		cmp     _keyd_repeat, 0
		je      DoneMarkingPressed

BufferAX:
	cmp     _keyd_buffer_type, 0
	je      SkipBuffer          ; Buffer = 0 means don't buffer anything.

	cmp     al, 0AAh    	; garbage key
	je      SkipBuffer

	call    get_modifiers  	;returns ah
	
	xor     ebx, ebx
	mov     bl, keytail
	inc     bl
	inc     bl

	; If the buffer is full then don't buffer this key
	cmp     bl, keyhead
	je      SkipBuffer
	dec     bl

	mov     word ptr keybuffer[ebx*2], ax
	mov     keytail, bl

SkipBuffer:	
		
;---------------------------------- Exit function -----------------------------
DoneMarkingPressed:
	jmp	LeaveHandler

;**************************************************************
;******************* HANDLE A RELEASED KEY ********************
;**************************************************************
; Unmarks the key press in EAX from the scancode array.
key_mark_released:

		mov     _keyd_last_released, al
		mov     byte ptr _keyd_pressed[eax], 0
		inc     NumUps[eax*4]

		cmp	_keyd_editor_mode, 0
		je	NotInEditorMode
		push    eax
		xor     ah,ah
		call    get_modifiers
		or      ah,ah	;check modifiers
		pop     eax
		jnz     skip_time

NotInEditorMode:	
		push    eax

		call    timer_get_stamp64

		; Timing in milliseconds
		; Can be used for up to 1000 hours
		shld    edx, eax, 21            ; Keep 32+11 bits
		shl     eax, 21
		mov     ebx, 2502279823         ; 2^21*1193180/1000
		div     ebx

		mov     edx, eax
		pop     eax
		sub     edx, TimeKeyWentDown[eax*4]
		add     TimeKeyHeldDown[eax*4], edx

skip_time:	;**jmp	LeaveHandler

;**************************************************************
;*************** FINISH UP THE KEYBOARD INTERRUPT *************
;**************************************************************
LeaveHandler:
		mov	eax, (0b0000h+76*2)
		mov 	byte ptr [eax], '2'

;;		cmp	_keyd_dump_key_array, 0
;;		je	DontPassToBios
		jmp	PassToBios

		mov	ecx, 256
		mov	ebx, 0

showdown:	mov	al, _keyd_pressed[ebx]
		add	al, '0'
		mov	[ebx*2+ 0b0000h], al
		inc	ebx
		loop	showdown

		mov	eax, 0b0000h
		mov     byte ptr [ eax+(036h*2+1) ], 070h
		mov     byte ptr [ eax+(02Ah*2+1) ], 070h
		mov     byte ptr [ eax+(038h*2+1) ], 070h
		mov     byte ptr [ eax+(0B8h*2+1) ], 070h
		mov     byte ptr [ eax+(01Dh*2+1) ], 070h
		mov     byte ptr [ eax+(09dh*2+1) ], 070h

		mov     byte ptr [ eax+(0AAh*2+1) ], 07Fh
		mov     byte ptr [ eax+(0E0h*2+1) ], 07Fh

		jmp	DontPassToBios


; If in debugger, pass control to dos interrupt.

PassToBios:	pop     ds          ; Nothing left on stack but flags
		pop     edx
		pop     ecx
		pop     ebx
		pop     eax

		sub     esp, 8              ; Save space for IRETD frame
		push    ds                  ; Save registers we use.
		push    eax
		mov     ax, DGROUP
		mov     ds, ax              ; Set DS to our data segment
		mov     eax, org_int_off   ; put original handler address
		mov     [esp+8], eax        ;   in the IRETD frame
		movzx   eax, org_int_sel
		mov     [esp+12], eax
		pop     eax                 ; Restore registers
		pop     ds
		iretd                       ; Chain to previous handler

pause_execution:
		in      al, 61h         ; Get current port 61h state
		or      al, 10000000b   ; Turn on bit 7 to signal clear keybrd
		out     61h, al         ; Send to port
		and     al, 01111111b   ; Turn off bit 7 to signal break
		out     61h, al         ; Send to port
		mov     al, 20h         ; Reset interrupt controller
		out     20h, al
		sti                     ; Reenable interrupts
		pop     ds
		pop     edx             ; Restore all of the saved registers.
		pop     ecx
		pop     ebx
		pop     eax

		sub     esp, 8              ; Save space for IRETD frame
		push    ds                  ; Save registers we use.
		push    eax
		mov     ax, DGROUP
		mov     ds, ax              ; Set DS to our data segment
		mov     eax, org_int_off   ; put original handler address
		mov     [esp+8], eax        ;   in the IRETD frame
		movzx   eax, org_int_sel
		mov     [esp+12], eax
		pop     eax                 ; Restore registers
		pop     ds

		iretd               	; Interrupt must return with IRETD

DontPassToBios:	

; Resets the keyboard, PIC, restores stack, returns.
		in      al, 61h         ; Get current port 61h state
		or      al, 10000000b   ; Turn on bit 7 to signal clear keybrd
		out     61h, al         ; Send to port
		and     al, 01111111b   ; Turn off bit 7 to signal break
		out     61h, al         ; Send to port
		mov     al, 20h         ; Reset interrupt controller
		out     20h, al
		sti                     ; Reenable interrupts
		pop     ds
		pop     edx             ; Restore all of the saved registers.
		pop     ecx
		pop     ebx
		pop     eax
		popfd
		iretd               ; Interrupt must return with IRETD

;returns ah=bitmask of shift,ctrl,alt keys
get_modifiers:		push    ecx

		xor     ah,ah

		; Check the shift keys
		mov     cl, _keyd_pressed[ 036h ]
		or      cl, _keyd_pressed[ 02ah ]
		or      ah, cl

		; Check the alt key
		mov     cl, _keyd_pressed[ 038h ]
		or      cl, _keyd_pressed[ 0b8h ]
		shl     cl, 1
		or      ah, cl

		; Check the ctrl key
		mov     cl, _keyd_pressed[ 01dh ]
		or      cl, _keyd_pressed[ 09dh ]
		shl     cl, 2
		or      ah, cl

		pop     ecx
		ret

IFDEF DEBUG
CheckForDebugger:
	; Returns CF=0 if debugger isn't active
	;         CF=1 if debugger is active

		;*************************** DEBUG ******************************
		; When we're in the VIDEO debugger, we want to pass control to
		; the original interrupt.  So, to tell if the debugger is active,
		; I check if video page 1 is the active page since that is what
		; page the debugger uses, and if that works, I check the top of
		; the screen to see if the texxt "Control" is there, which should
		; only be there when we're in the debugger.

	

		push    eax
		;mov     eax, 0462h          ; Address 0462 stores BIOS current page
		;cmp     BYTE PTR [eax], 1
		;jne     NoDebuggerOnColor
		;mov     eax, 0b8000h+4096   ; 4096 = offset to 2nd video mem page
		;cmp     BYTE PTR [eax+2],'C'
		;jne     NoDebuggerOnColor
		;cmp     BYTE PTR [eax+4],'o'
		;jne     NoDebuggerOnColor
		;cmp     BYTE PTR [eax+6],'n'
		;jne     NoDebuggerOnColor
		;cmp     BYTE PTR [eax+8],'t'
		;jne     NoDebuggerOnColor
		;cmp     BYTE PTR [eax+10],'r'
		;jne     NoDebuggerOnColor
		;cmp     BYTE PTR [eax+12],'o'
		;jne     NoDebuggerOnColor
		;cmp     BYTE PTR [eax+14],'l'
		;jne     NoDebuggerOnColor
		;jmp     ActiveDebugger
		;NoDebuggerOnColor:
		; First, see if there is a mono debugger...

		;mov     eax, 0b0000h        ; 4096 = offset to mono video mem
		;cmp     BYTE PTR [eax+2],'C'
		;jne     NoActiveDebugger
		;cmp     BYTE PTR [eax+4],'o'
		;jne     NoActiveDebugger
		;cmp     BYTE PTR [eax+6],'n'
		;jne     NoActiveDebugger
		;cmp     BYTE PTR [eax+8],'t'
		;jne     NoActiveDebugger
		;cmp     BYTE PTR [eax+10],'r'
		;jne     NoActiveDebugger
		;cmp     BYTE PTR [eax+12],'o'
		;jne     NoActiveDebugger
		;cmp     BYTE PTR [eax+14],'l'
		;jne     NoActiveDebugger

		mov     eax, 0b0000h        ; 4096 = offset to mono video mem
		add     eax, 24*80*2


		cmp     BYTE PTR [eax+0],'D'
		jne     NextTest
		cmp     BYTE PTR [eax+2],'B'
		jne     NextTest
		cmp     BYTE PTR [eax+4],'G'
		jne     NextTest
		cmp     BYTE PTR [eax+6],'>'
		jne     NextTest

		;Found DBG>, so consider debugger active:
		jmp     ActiveDebugger

NextTest:
		cmp     BYTE PTR [eax+14],'<'
		jne     NextTest1
		cmp     BYTE PTR [eax+16],'i'
		jne     NextTest1
		cmp     BYTE PTR [eax+18],'>'
		jne     NextTest1
		cmp     BYTE PTR [eax+20],' '
		jne     NextTest1
		cmp     BYTE PTR [eax+22],'-'
		jne     NextTest1

		; Found <i> - , so consider debugger active:
		jmp     ActiveDebugger

NextTest1:
		cmp     BYTE PTR [eax+0], 200
		jne     NextTest2
		cmp     BYTE PTR [eax+2], 27
		jne     NextTest2
		cmp     BYTE PTR [eax+4], 17
		jne     NextTest2

		; Found either the help screen or view screen, so consider
		; debugger active
		jmp     ActiveDebugger

NextTest2:
		; Now we see if its active by looking for the "Executing..."
		; text on the bottom of the mono screen
		;mov     eax, 0b0000h        ; 4096 = offset to mono video mem
		;add     eax, 24*80*2
		;cmp     BYTE PTR [eax+0],'E'
		;je      NoActiveDebugger
		;cmp     BYTE PTR [eax+2],'x'
		;je      NoActiveDebugger
		;cmp     BYTE PTR [eax+4],'e'
		;je      NoActiveDebugger
		;cmp     BYTE PTR [eax+6],'c'
		;je      NoActiveDebugger

NoActiveDebugger:
		pop     eax
		clc
		ret

ActiveDebugger:
		pop     eax
		stc
		ret

ENDIF

_TEXT   ENDS

		END
