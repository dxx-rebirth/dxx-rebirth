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

rcsid	db	"$Id: key.asm,v 1.1.1.1 2001-01-19 03:30:14 bradleyb Exp $"

PUBLIC  _keyd_pressed     ; Must start with a _ so C can see the variable.

		_keyd_pressed   db  256 dup (?)

		keybuffer       dw  256 dup (?)     ; Use 256 so an inc wraps around

		TimeKeyWentDown dd  256 dup(0)
		TimeKeyHeldDown dd  256 dup(0)
		NumDowns        dd  256 dup(0)
		NumUps          dd  256 dup(0)

		MyCodeSegment   dw  ?

PUBLIC  _keyd_last_pressed
		_keyd_last_pressed  db 0
PUBLIC  _keyd_last_released
		_keyd_last_released db 0

		org_int_sel dw  ?
		org_int_off dd  ?
		keyhead      db  ?
		keytail      db  ?
PUBLIC  _keyd_buffer_type
PUBLIC  _keyd_repeat
		_keyd_buffer_type db  ?   ; 0=No buffer, 1=buffer ASCII, 2=buffer scans
		_keyd_repeat      db  ?

		E0Flag      db 0

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
key_clear_times_:	push	eax
	push	ecx
	push	edi
	xor	eax,eax
	mov	ecx,256
	lea	edi,TimeKeyHeldDown
	rep	stosd	;clear array
	pop	edi
	pop	ecx
	pop	eax
	ret

;clear the arrays of key down counts
key_clear_counts_:	push	eax
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
	ret


PUBLIC  key_down_time_

key_down_time_:

		EXTERNDEF   timer_get_milliseconds_:NEAR

		push    edx
		push    ecx
		push    ebx

		mov     ebx, eax
		xor     eax, eax

		cmp     _keyd_pressed[ebx], 0
		je      NotPressed

		call    get_modifiers	;shift,alt,ctrl?
		or      ah,ah
		jz      read_time
		xor     eax,eax
		jmp     NotPressed
read_time:

		mov     ecx, TimeKeyWentDown[ebx*4]
		cli
		call    timer_get_milliseconds_
		mov     TimeKeyWentDown[ebx*4], eax
		sub     eax, ecx        ; EAX = time held since last

NotPressed:
		add     eax, TimeKeyHeldDown[ebx*4]
		mov     TimeKeyHeldDown[ebx*4], 0

		sti
		pop     ebx
		pop     ecx
		pop     edx
		ret

PUBLIC  key_down_count_
key_down_count_:
		push    ebx
		mov     ebx, eax
		cli
		mov     eax, NumDowns[ebx*4]
		mov     NumDowns[ebx*4], 0
		sti
		pop     ebx
		ret

PUBLIC  key_up_count_
key_up_count_:
		push    ebx
		mov     ebx, eax
		cli
		mov     eax, NumUps[ebx*4]
		mov     NumUps[ebx*4], 0
		sti
		pop     ebx
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
		push    eax
		push    ecx
		push    edi

		cli
		mov     keyhead,0
		mov     keytail,255
		mov     E0Flag, 0

		; Clear the keyboard array
		mov     edi, offset _keyd_pressed
		mov     ecx, 32
		mov     eax,0
		rep     stosd
		sti

		pop     edi
		pop     ecx
		pop     eax
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
		mov     ebx, 041ch
		mov     al, byte ptr [ebx]
		mov     ebx, 041ah
		mov     byte ptr [ebx], al

		mov     eax, 02509h         ; DOS Set Vector 09h
		mov     edx, org_int_off
		mov     ds, org_int_sel
		int     21h

@@:     pop     ds
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
		push    ebx

		xor     eax, eax
		cmp     Installed, 0
		je      NoKey

		cli
		mov     bl, keytail
		inc     bl
		cmp     bl, keyhead
		je      Nokey
		mov     eax, 1

Nokey:
		sti
		pop     ebx
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
;*****                   K E Y _ G E T C H _                          *****
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

		cmp     Installed, 0
		je      NoPeek

		cli             ; Critical section
		mov     bl, keytail
		inc     bl
		cmp     bl, keyhead
		sti
		je      NoPeek

		cli             ; Critical section
		mov     bl, keyhead
		mov     ax, word ptr keybuffer[ebx*2]
		sti
NoPeek:
		pop     ebx
		ret



;************************************************************************
;************************************************************************
;*****                                                              *****
;*****                   K E Y _ H A N D L E R                        *****
;*****                                                              *****
;************************************************************************
;************************************************************************

PUBLIC  key_handler      ; Must end with a _ so C can see the function.

key_handler:
		EXTERNDEF   timer_get_milliseconds_:NEAR

		pushfd              ; Save flags in case we have to chain to original
		push    eax
		push    ebx
		push    ecx
		push    edx
		push    ds

		mov     ax, DGROUP  ; Point to our data segment, since this is an
		mov     ds, ax      ; interrupt and we don't know where we were.

IFDEF DEBUG
		call    CheckForDebugger
		jnc     @f
		mov eax, 0b0000h+78*2
		mov byte ptr [eax], 'D'
		jmp      SkipBuffer      ; If debugger is active, then skip buffer
@@:     mov eax, 0b0000h+78*2
		mov byte ptr [eax], 'I'

		; Clear the BIOS buffer
		mov     ebx, 041ch
		mov     al, byte ptr [ebx]
		mov     ebx, 041ah
		mov     byte ptr [ebx], al
ENDIF

		;**************************************************************
		;****************** READ IN THE SCAN CODE *********************
		;**************************************************************
		; Reads the scan code from the keyboard and masks off the
		; scan code and puts it in EAX.

		xor     eax, eax
		in      al, 060h        ; Get scan code from keyboard

		cmp     al, 0E0h
		jne     NotE0Code

E0Code:
		mov     E0Flag, 128
		jmp     SkipBuffer

NotE0Code:
		mov     bl,al                   ; Save scan code in BL
		and     bl,01111111b
		add     bl, E0Flag
		mov     E0Flag,0
		xor     bh,bh                   ; clear for index use
		and     al,10000000b            ; keep break bit, if set
		xor     al,10000000b            ; flip bit - 1 means pressed
										;          - 0 means released
		rol     al,1                    ; put it in bit 0
		xchg    ax, bx

		; AX = Key code
		; BX = 1 if pressed, 0 if released.
		cmp     bx,  1
		je      pkeyDown

		;**************************************************************
		;******************* HANDLE A RELEASED KEY ********************
		;**************************************************************
		; Unmarks the key press in EAX from the scancode array.


		mov     _keyd_last_released, al
		mov     byte ptr _keyd_pressed[eax], 0
		inc     NumUps[eax*4]

		push    eax
		xor     ah,ah
		call    get_modifiers
		or      ah,ah	;check modifiers
		pop     eax
		jnz     skip_time

		push    edx
		push    eax
		call    timer_get_milliseconds_
		mov     edx, eax
		pop     eax
		sub     edx, TimeKeyWentDown[eax*4]
		add     TimeKeyHeldDown[eax*4], edx
		pop     edx
skip_time:

		jmp     pdone

pkeyDown:

		;**************************************************************
		;****************** HANDLE A NEWLY PRESSED KEY ****************
		;**************************************************************
		;Marks the key press in EAX in the scancode array.


		mov     _keyd_last_pressed, al
		; Check if the key is repeating or if it just got pressed.
		cmp     byte ptr _keyd_pressed[eax], 1
		je      AlreadyDown

		mov     byte ptr _keyd_pressed[eax], 1
		; Set the time

		push    edx
		push    eax
		call    timer_get_milliseconds_
		mov     edx, eax
		pop     eax
		mov     TimeKeyWentDown[eax*4], edx
		pop     edx

		inc     NumDowns[eax*4]

		jmp     TryBuffer

		;**************************************************************
		;******************** HANDLE A PRESSED KEY ********************
		;**************************************************************
		; Adds key scan code in EAX to the keybuffer array.

AlreadyDown:
		cmp     _keyd_repeat, 0
		je      SkipBuffer

TryBuffer:
		cmp     _keyd_buffer_type, 0
		je      SkipBuffer          ; Buffer = 0 means don't buffer anything.

		; Dont buffer shift, ctrl, or alt keys.
		;cmp     al, 02ah  	; Right Shift
		;je      SkipBuffer
		;cmp     al, 036h  	; Left Shift
		;je      SkipBuffer
		;cmp     al, 038h  	; Left Alt
		;je      SkipBuffer
		;cmp     al, 0b8h	; Right Alt
		;je      SkipBuffer
		;cmp     al, 01dh  	; Left Ctrl
		;je      SkipBuffer
		;cmp     al, 09dh	' Right Ctrl
		;je      SkipBuffer

		cmp     al, 0AAh    	; garbage key
		je      SkipBuffer

		call    get_modifiers  ;returns ah

BufferAX:

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

pdone:


;**************************************************************
;*************** FINISH UP THE KEYBOARD INTERRUPT *************
;**************************************************************

; If in debugger, pass control to dos interrupt.
IFDEF DEBUG
		pop     ds          ; Nothing left on stack but flags
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
ENDIF

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


