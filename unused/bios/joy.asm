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

_DATA   SEGMENT BYTE PUBLIC USE32 'DATA'

rcsid	db	"$Id: joy.asm,v 1.1.1.2 2001-01-19 03:33:50 bradleyb Exp $"

	LastTick 	dd	0
	TotalTicks	dd	0
	
_DATA   ENDS

DGROUP  GROUP _DATA


_TEXT   SEGMENT BYTE PUBLIC USE32 'CODE'

	ASSUME  ds:_DATA
	ASSUME  cs:_TEXT

	JOY_PORT        EQU     0201h
	TDATA       	EQU 	40h
	TCOMMAND    	EQU 	43h

joy_get_timer:
	xor	al, al		    	; Latch timer 0 command
	out	TCOMMAND, al		; Latch timer
	in	al, TDATA		; Read lo byte
	mov	ah, al
	in	al, TDATA		; Read hi byte
	xchg	ah, al
	and	eax, 0ffffh
	ret	

PUBLIC joy_read_stick_asm_

joy_read_stick_asm_:	
		; ebx = read mask
		; edi = pointer to event buffer
		; ecx = timeout value
		; returns in eax the number of events
		and	ebx, 01111b		; Make sure we only check the right values										
						; number of events we found will be in bh, so this also clears it to zero.

		mov     dx, JOY_PORT

		cli				; disable interrupts while reading time...
		call	joy_get_timer		; Returns counter in EAX
		sti				; enable interrupts after reading time...
		mov	LastTick, eax

waitforstable:	in	al, dx
		and	al, bl
		jz	ready 			; Wait for the port in question to be done reading...
		
		cli				; disable interrupts while reading time...
		call	joy_get_timer		; Returns counter in EAX
		sti				; enable interrupts after reading time...
		xchg	eax, LastTick
		cmp	eax, LastTick
		jb	@f
		sub	eax, LastTick
@@:		; Higher...
		add	TotalTicks, eax
		cmp	TotalTicks, ecx		; Timeout at 1/200'th of a second
		jae	ready
		jmp	waitforstable
		
ready:
		cli
		mov     al, 0ffh
		out     dx, al			; Start joystick a readin'

		call	joy_get_timer		; Returns counter in EAX
		mov	LastTick, eax
		mov	TotalTicks, 0
		
		mov	[edi], eax		; Store initial count
		add	edi, 4		

	again:	in	al, dx			; Read Joystick port
		not	al
		and	al, bl			; Mask off channels we don't want to read
		jnz	flip			; See if any of the channels flipped

		call	joy_get_timer		; Returns counter in EAX
		
		xchg	eax, LastTick
		cmp	eax, LastTick
		jb	@f
		sub	eax, LastTick
@@:		; Higher...
		add	TotalTicks, eax
		cmp	TotalTicks, ecx		; Timeout at 1/200'th of a second
		jae	timedout
		jmp	again

	flip:	and	eax, 01111b		; Only care about axis values
		mov	[edi], eax		; Record what channel(s) flipped
		add	edi, 4	
		xor	bl, al			; Unmark the channels that just tripped

		call	joy_get_timer		; Returns counter in EAX
		mov	[edi], eax		; Record the time this channel flipped
		add	edi, 4		
		inc	bh			; Increment number of events

		cmp	bl, 0	
		jne	again			; If there are more channels to read, keep looping

	timedout:
		movzx	eax, bh			; Return number of events

		sti
		ret


PUBLIC joy_read_stick_polled_

joy_read_stick_polled_:	
		; ebx = read mask
		; edi = pointer to event buffer
		; ecx = timeout value
		; returns in eax the number of events
		and	ebx, 01111b		; Make sure we only check the right values										
						; number of events we found will be in bh, so this also clears it to zero.

		mov     dx, JOY_PORT

		mov	TotalTicks, 0

waitforstable1:	in	al, dx
		and	al, bl
		jz	ready1 			; Wait for the port in question to be done reading...
		
		inc	TotalTicks
		cmp	TotalTicks, ecx		; Timeout at 1/200'th of a second
		jae	ready1
		jmp	waitforstable1
ready1:
		cli
		mov     al, 0ffh
		out     dx, al			; Start joystick a readin'

		mov	TotalTicks, 0

		mov	dword ptr [edi], 0		; Store initial count
		add	edi, 4		

	again1:	in	al, dx			; Read Joystick port
		not	al
		and	al, bl			; Mask off channels we don't want to read
		jnz	flip1			; See if any of the channels flipped

		inc	TotalTicks
		cmp	TotalTicks, ecx		; Timeout at 1/200'th of a second
		jae	timedout1
		jmp	again1

	flip1:	and	eax, 01111b		; Only care about axis values
		mov	[edi], eax		; Record what channel(s) flipped
		add	edi, 4	
		xor	bl, al			; Unmark the channels that just tripped

		mov	eax, TotalTicks
		mov	[edi], eax		; Record the time this channel flipped
		add	edi, 4		
		inc	bh			; Increment number of events

		cmp	bl, 0	
		jne	again1			; If there are more channels to read, keep looping

	timedout1:
		movzx	eax, bh			; Return number of events

		sti
		ret

PUBLIC joy_read_stick_bios_

joy_read_stick_bios_:	
		; ebx = read mask
		; edi = pointer to event buffer
		; ecx = timeout value
		; returns in eax the number of events
		
		pusha

		mov	dword ptr [edi], 0
			
		mov	eax, 08400h
		mov	edx, 1
		cli
		int	15h
		sti
	
		mov	dword ptr [edi+4], 1	;	Axis 1		
		and	eax, 0ffffh
		mov	[edi+8], eax		; 	Axis 1 value

		mov	dword ptr [edi+12], 2	;	Axis 2
		and	ebx, 0ffffh
		mov	[edi+16], ebx		; 	Axis 2 value

		mov	dword ptr [edi+20], 4	;	Axis 3
		and	ecx, 0ffffh
		mov	[edi+24], ecx		; 	Axis 3 value

		mov	dword ptr [edi+28], 8	;	Axis 3
		and	edx, 0ffffh
		mov	[edi+32], edx		; 	Axis 3 value

		popa
		mov	eax, 4			; 4 events

		ret


PUBLIC joy_read_buttons_bios_

joy_read_buttons_bios_:	
		; returns in eax the button settings
		
		push	ebx
		push	ecx
		push	edx
		mov	eax, 08400h
		mov	edx, 0	; Read switches
		int	15h
		pop	edx
		pop	ecx
		pop	ebx
		
		shr	eax, 4
		not	eax
		and	eax, 01111b
		ret


_TEXT   ENDS


		END
