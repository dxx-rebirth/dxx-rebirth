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
;*****                     T I M E R . A S M                           *****
;*****                                                                 *****
;*****                                                                 *****
;***** PROCEDURES                                                      *****
;*****                                                                 *****
;*****                                                                 *****
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

rcsid	db	"$Id: timer.asm,v 1.1.1.1 2001-01-19 03:30:14 bradleyb Exp $"

TDATA       	EQU 	40h
TCOMMAND    	EQU 	43h
PIC         	EQU 	020h
STACK_SIZE	EQU	4096	; A 4K stack

TIMER_DATA STRUCT   
	in_timer	db  0

	nested_counter	dd	0

	_timer_cnt      dd  65536
	dos_timer       dd  ?

	joystick_poller	dd  0

	user_function   df  0
	
	org_interrupt	df  0	

	saved_stack	df  0

	new_stack	df  0

	tick_count	dd  0

	Installed db 	0

	TimerStack	db  STACK_SIZE dup (?)		
TIMER_DATA ENDS

	TimerData	TIMER_DATA <>	

_DATA   ENDS

DGROUP  GROUP _DATA


;************************************************************************
;**************** FLAT MODEL CODE SEGMENT STUFF *************************
;************************************************************************

_TEXT   SEGMENT BYTE PUBLIC USE32 'CODE'

		ASSUME  ds:_DATA
		ASSUME  cs:_TEXT

INCLUDE PSMACROS.INC
INCLUDE FIX.INC

TIMER_LOCKED_CODE_START:

		extn atexit_

PUBLIC timer_get_stamp64

timer_get_stamp64:
		; Return a 64-bit stamp that is the number of 1.19Mhz pulses
		; since the time was initialized.  Returns in EDX:EAX.
		; Also, interrupts must be disabled.

		xor     eax, eax            ; Clear all of EAX
		out     TCOMMAND, al        ; Tell timer to latch

		mov     al, 0ah             ; Find if interrupt pending
		out     PIC, al
		jmp	@f
@@:		in      al, PIC
		and     eax, 01b
		jz      NoInterrupt

		in      al, TDATA           ; Read in lo byte
		mov     dl, al
	        in      al, TDATA           ; Read in hi byte
		mov	dh, al
		and	edx, 0ffffh
		mov	eax, TimerData._timer_cnt
		shl	eax, 1		
		sub	eax, edx
			
		push	ebx
		mov	ebx, eax
		mov	eax, TimerData.tick_count
		imul	TimerData._timer_cnt	; edx:eax = Number of 1.19 MHz ticks elapsed...
		add	eax, ebx
		adc	edx, 0				
		pop	ebx

		ret

NoInterrupt:
		in      al, TDATA           ; Read in lo byte
		mov     ah, al
		in      al, TDATA           ; Read in hi byte
		xchg    ah, al              ; arrange em correctly
		mov     edx, TimerData._timer_cnt
		sub     dx, ax              ; BX = timer ticks
		mov     ax, dx

		push	ebx
		mov	ebx, eax
		mov	eax, TimerData.tick_count
		imul	TimerData._timer_cnt	; edx:eax = Number of 1.19 MHz ticks elapsed...
		add	eax, ebx
		adc	edx, 0				
		pop	ebx

		ret


PUBLIC  timer_get_fixed_seconds_

timer_get_fixed_seconds_:
		push    ebx
		push    edx

		cli
		call    timer_get_stamp64
		sti

		; Timing in fixed point (16.16) seconds.
		; Can be used for up to 1000 hours
		shld    edx, eax, 16            ; Keep 32+11 bits
		shl     eax, 16			
		; edx:eax = number of 1.19Mhz pulses elapsed.
		mov     ebx, 1193180

; Make sure we won't divide overflow.  Make time wrap at about 9 hours
sub_again:		sub     edx, ebx	; subtract until negative...
		jns     sub_again	; ...to prevent divide overflow...
		add     edx, ebx	; ...then add in to get correct value.
		div     ebx
		; eax = fixed point seconds elapsed...

		pop     edx
		pop     ebx

		ret

PUBLIC  timer_get_fixed_secondsX_

timer_get_fixed_secondsX_:
		push    ebx
		push    edx

		call    timer_get_stamp64

		; Timing in fixed point (16.16) seconds.
		; Can be used for up to 1000 hours
		shld    edx, eax, 16            ; Keep 32+11 bits
		shl     eax, 16			
		; edx:eax = number of 1.19Mhz pulses elapsed.
		mov     ebx, 1193180

xsub_again:		sub     edx, ebx	; subtract until negative...
		jns     xsub_again	; ...to prevent divide overflow...
		add     edx, ebx	; ...then add in to get correct value.

		div     ebx
		; eax = fixed point seconds elapsed...

		pop     edx
		pop     ebx

		ret

PUBLIC timer_get_approx_seconds_
timer_get_approx_seconds_:
	push	ebx
	push	edx

	mov	eax, TimerData.tick_count
	imul	TimerData._timer_cnt	; edx:eax = Number of 1.19 MHz ticks elapsed...
	shld    edx, eax, 16            ; Keep 32+16 bits, for conversion to fixed point
	shl     eax, 16			
	; edx:eax = number of 1.19Mhz pulses elapsed.
	mov     ebx, 1193180

approx_sub_again:	sub     edx, ebx	; subtract until negative...
	jns     approx_sub_again	; ...to prevent divide overflow...
	add     edx, ebx	; ...then add in to get correct value.

	div     ebx
	; eax = fixed point seconds elapsed...

	pop	edx
	pop	ebx
	ret



;extern void timer_set_rate(int count_val);
;extern void timer_set_function( void _far * function );

PUBLIC  timer_set_rate_
timer_set_rate_:
	; eax = rate
	pushad

	; Make sure eax below or equal to 65535 and above 0
	; if its not, make it be 65536 which is normal dos
	; timing.
	cmp	eax, 65535
	jbe	@f
	mov	eax, 65536
@@:	cmp	eax, 0
	jne	@f
	mov	eax, 65536
@@:	; Set the timer rate to eax
	cli
	mov	TimerData.tick_count, 0
	mov     TimerData._timer_cnt, eax		
	mov     al, 34h		; count in binary, mode 2, load low byte then hi byte, counter 0:  00 11 010 0
	out     TCOMMAND, al    ; Reset PIT channel 0
	mov     eax, TimerData._timer_cnt
	out     TDATA, al
	mov     al, ah
	out     TDATA, al
	sti
	popad
	ret

PUBLIC  timer_set_function_
timer_set_function_:
	; dx:eax = far pointer to user function
	pushad
	cli
	mov	dword ptr TimerData.user_function[0], eax	; offset
	mov     word ptr TimerData.user_function[4], dx		; selector
	sti
	popad
	ret

PUBLIC timer_set_joyhandler_
timer_set_joyhandler_:
	cli
	mov	TimerData.joystick_poller, eax
	sti
	ret

PUBLIC timer_delay_
timer_delay_:
	pushad
	mov	edx, 18
	mov	ecx, [ds:046Ch]		; Get Current DOS Ticks
	fixmul	edx	
	add	ecx, eax
timeloop:
	cmp	ecx, [ds:046ch]
	jg	timeloop
	popad
	ret



;************************************************************************
;************************************************************************
;*****                                                              *****
;*****                T I M E R _ H A N D L E R                     *****
;*****                                                              *****
;************************************************************************
;************************************************************************

timer_handler:
		push	ds
		push	es
		push	eax

		mov     ax, DGROUP       ; Interrupt, so point to our data segment
		mov     ds, ax
		mov     es, ax

		; Increment time counter...
		inc	TimerData.tick_count

		mov	eax, TimerData._timer_cnt
		add     TimerData.dos_timer, eax		; Increment DOS timer
		cmp	TimerData.dos_timer, 65536
		jb	NoChainToOld        	; See if we need to chain to DOS 18.2
		and	TimerData.dos_timer, 0ffffh

		;
		; Call the original DOS handler....
		;
		pushfd
		call	fword ptr [TimerData.org_interrupt]

		jmp	NoReset		;old handler has reset, so we don't

NoChainToOld:
		; Reset controller
		mov     al, 20h         ; Reset interrupt controller
		out     20h, al

NoReset:
		cmp	TimerData.in_timer, 0
		jne	ExitInterrupt

		mov	TimerData.in_timer, 1		; Mark that we're in a timer interrupt...

		; Reenable interrupts
		sti			; Reenable interrupts

		cmp     word ptr TimerData.user_function[4], 0		; Check the selector...
		je      NoUserFunction

		; Switch stacks while calling the user-definable function...
		pushad
		push	fs
		push	gs
		mov	dword ptr TimerData.saved_stack[0], esp
		mov	word ptr TimerData.saved_stack[4], ss
		lss	esp, TimerData.new_stack	; Switch to new stack
		call    fword ptr [TimerData.user_function]	; Call new function
		lss	esp,  TimerData.saved_stack	; Switch back to original stack
		pop	gs
		pop	fs
		popad

NoUserFunction:	
		cmp	dword ptr TimerData.joystick_poller, 0
		je	NoJoyPolling
		mov	eax, TimerData._timer_cnt
		mov	dword ptr TimerData.saved_stack[0], esp
		mov	word ptr TimerData.saved_stack[4], ss
		lss	esp, TimerData.new_stack	; Switch to new stack
		call	dword ptr TimerData.joystick_poller
		lss	esp,  TimerData.saved_stack	; Switch back to original stack

NoJoyPolling:
		cli
		mov	TimerData.in_timer, 0

ExitInterrupt:
		;;mov     al, 20h         ; Reset interrupt controller
		;;out     20h, al
		pop	eax
		pop	es
		pop	ds
		iretd				; Return from timer interrupt


TIMER_LOCKED_CODE_STOP:

;************************************************************************
;************************************************************************
;*****                                                              *****
;*****                   T I M E R _ I N I T                        *****
;*****                                                              *****
;************************************************************************
;************************************************************************


PUBLIC  timer_init_

timer_init_:
		pushad
		push    ds
		push    es

		cmp     TimerData.Installed, 1
		je      AlreadyInstalled

		mov     TimerData._timer_cnt, 65536	; Set to BIOS's normal 18.2 Hz
		mov     TimerData.dos_timer, 0		; clear DOS Interrupt counter
		mov     dword ptr TimerData.user_function[0], 0	; offset of user function
		mov     word ptr TimerData.user_function[4], 0	; selector of user function

		lea	eax, ds:[TimerData.TimerStack]	; Use EAX as temp stack pointer
		add	eax, STACK_SIZE			; Top of stack minus space for saving old ss:esp
		mov	dword ptr TimerData.new_stack[0], eax
		mov	word ptr TimerData.new_stack[4], ds

		;--------------- lock data used in interrupt
		mov	eax, SIZEOF TIMER_DATA
		mov	esi, eax
		shr	esi, 16			
		mov	edi, eax
		and	edi, 0ffffh	; si:di = length of region to lock in bytes
		lea	ebx, ds:TimerData
		lea	ecx, ds:TimerData
		shr	ebx, 16
		and	ecx, 0ffffh	; bx:cx = start of linear address to lock
		mov	eax, 0600h	; DPMI lock address function
		int	31h		; call dpmi
		jnc	@f
		int	3		; LOCK FAILED!!
@@:
		;--------------- lock code used in interrupt
		lea	eax, cs:TIMER_LOCKED_CODE_STOP
		lea	ecx, cs:TIMER_LOCKED_CODE_START
		sub	eax, ecx
		inc	eax		; EAX = size of timer interrupt handler
		mov	esi, eax
		shr	esi, 16			
		mov	edi, eax
		and	edi, 0ffffh	; si:di = length of region to lock in bytes
		lea	ebx, cs:TIMER_LOCKED_CODE_START
		lea	ecx, cs:TIMER_LOCKED_CODE_START
		shr	ebx, 16
		and	ecx, 0ffffh	; bx:cx = start of linear address to lock
		mov	eax, 0600h	; DPMI lock address function
		int	31h		; call dpmi
		jnc	@f
		int	3		; LOCK FAILED!!
@@:

		;**************************************************************
		;******************* SAVE OLD INT8 HANDLER ********************
		;**************************************************************
		mov     eax, 03508h             ; DOS Get Vector 08h
		int     21h                     ; Call DOS

		mov     dword ptr TimerData.org_interrupt[0], ebx	; offset of user function
		mov	word ptr TimerData.org_interrupt[4], es		; selector of user function

		;**************************************************************
		;***************** INSTALL NEW INT8 HANDLER *******************
		;**************************************************************

		cli

		mov     al, 34h		; count in binary, mode 2, load low byte then hi byte, counter 0:  00 11 010 0
		out     TCOMMAND, al    ; Reset PIT channel 0
		mov     eax, TimerData._timer_cnt
		out     TDATA, al
		mov     al, ah
		out     TDATA, al

		mov     TimerData.tick_count, 0
		mov     TimerData.Installed,1

		mov     eax, 02508h             ; DOS Set Vector 08h
		mov     edx, offset timer_handler  ; Point DS:EDX to new handler
		mov     bx, cs
		push    ds
		mov     ds, bx
		int     21h
		pop     ds
		sti
		lea     eax, cs:timer_close_
		call    atexit_

AlreadyInstalled:

		pop     es
		pop     ds
		popad

		ret


;************************************************************************
;************************************************************************
;*****                                                              *****
;*****                  T I M E R _ C L O S E _                     *****
;*****                                                              *****
;************************************************************************
;************************************************************************

PUBLIC  timer_close_

timer_close_:
		push    eax
		push    edx
		push    ds


		cmp     TimerData.Installed, 0
		je      NotInstalled
		mov     TimerData.Installed, 0

		;**************************************************************
		;***************** RESTORE OLD INT9 HANDLER *******************
		;**************************************************************

		cli
		mov     al, 36h		; count in binary, mode 3, load low byte then hi byte, counter 0:  00 11 011 0
		out     TCOMMAND, al    ; Reser PIT channel 0
		mov     ax, 0h
		out     TDATA, al
		mov     al, ah
		out     TDATA, al

		push	ds
		mov     eax, 02508h         ; DOS Set Vector 08h
		mov     edx, dword ptr TimerData.org_interrupt[0]
		mov     ds, word ptr TimerData.org_interrupt[4]
		int     21h
		pop	ds

		sti
		
		cmp	TimerData.nested_counter, 0
		je	NoNestedInterrupts
		mov	eax, TimerData.nested_counter
		;int 	3		; Get John!!
	
NoNestedInterrupts:
		

NotInstalled:
		pop     ds
		pop     edx
		pop     eax

		ret


_TEXT           ENDS


		END
