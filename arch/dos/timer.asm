[BITS 32]

[EXTERN _atexit]
[EXTERN ___djgpp_base_address]

[GLOBAL _timer_get_fixed_seconds]
[GLOBAL _timer_get_fixed_secondsX]
[GLOBAL _timer_get_approx_seconds]
[GLOBAL _timer_set_rate]
[GLOBAL _timer_set_function]
[GLOBAL _timer_set_joyhandler]
[GLOBAL _timer_init]
[GLOBAL _timer_close]


[SECTION .data]
TDATA      EQU 40h
TCOMMAND   EQU 43h
PIC        EQU 020h
STACK_SIZE EQU 4096

%define df times 3 dw

TimerData  db 0
           dd 0
           dd 65536
           dd 0
           dd 0
           df 0
           df 0
           df 0
           df 0
           dd 0
           db 0
times STACK_SIZE db 0

td_in_timer        equ TimerData
td_nested_counter  equ td_in_timer+1
td__timer_cnt      equ td_nested_counter+4
td_dos_timer       equ td__timer_cnt+4
td_joystick_poller equ td_dos_timer+4
td_user_function   equ td_joystick_poller+4
td_org_interrupt   equ td_user_function+6
td_saved_stack     equ td_org_interrupt+6
td_new_stack       equ td_saved_stack+6
td_tick_count      equ td_new_stack+6
td_Installed       equ td_tick_count+4
td_TimerStack      equ td_Installed+1


[SECTION .text]

TIMER_LOCKED_CODE_START:

__my_ds dw 0

timer_get_stamp64:
; Return a 64-bit stamp that is the number of 1.19Mhz pulses
; since the time was initialized.  Returns in EDX:EAX.
; Also, interrupts must be disabled.

 xor     eax, eax            ; Clear all of EAX
 out     TCOMMAND, al        ; Tell timer to latch

 mov     al, 0ah             ; Find if interrupt pending
 out     PIC, al
 jmp     @
 @:
 in      al, PIC
 and     eax, 01b
 jz      NoInterrupt

 in      al, TDATA           ; Read in lo byte
 mov     dl, al
 in      al, TDATA           ; Read in hi byte
 mov     dh, al
 and     edx, 0ffffh
 mov     eax, [td__timer_cnt]
 shl     eax, 1   
 sub     eax, edx
			
 push    ebx
 mov     ebx, eax
 mov     eax, [td_tick_count]
 imul    dword [td__timer_cnt]    ; edx:eax = Number of 1.19 MHz ticks elapsed...
 add     eax, ebx
 adc     edx, 0                   
 pop     ebx

 ret

NoInterrupt:
 in      al, TDATA           ; Read in lo byte
 mov     ah, al
 in      al, TDATA           ; Read in hi byte
 xchg    ah, al              ; arrange em correctly
 mov     edx, [td__timer_cnt]
 sub     dx, ax              ; BX = timer ticks
 mov     ax, dx

 push    ebx
 mov     ebx, eax
 mov     eax, [td_tick_count]
 imul    dword [td__timer_cnt]    ; edx:eax = Number of 1.19 MHz ticks elapsed...
 add     eax, ebx
 adc     edx, 0                   
 pop     ebx

 ret


_timer_get_fixed_seconds:
 push    ebx
 push    edx

 cli
 call	 timer_get_stamp64
 sti

; Timing in fixed point (16.16) seconds.
; Can be used for up to 1000 hours
 shld    edx, eax, 16            ; Keep 32+11 bits
 shl     eax, 16          
; edx:eax = number of 1.19Mhz pulses elapsed.
 mov     ebx, 1193180

; Make sure we won't divide overflow.  Make time wrap at about 9 hours
sub_again:
 sub     edx, ebx        ; subtract until negative...
 jns     sub_again       ; ...to prevent divide overflow...
 add     edx, ebx        ; ...then add in to get correct value.
 div     ebx
; eax = fixed point seconds elapsed...

 pop     edx
 pop     ebx

 ret


_timer_get_fixed_secondsX:
 push    ebx
 push    edx

 call	 timer_get_stamp64

; Timing in fixed point (16.16) seconds.
; Can be used for up to 1000 hours
 shld    edx, eax, 16            ; Keep 32+11 bits
 shl     eax, 16          
; edx:eax = number of 1.19Mhz pulses elapsed.
 mov     ebx, 1193180

xsub_again:
 sub     edx, ebx        ; subtract until negative...
 jns     xsub_again      ; ...to prevent divide overflow...
 add     edx, ebx        ; ...then add in to get correct value.

 div     ebx
; eax = fixed point seconds elapsed...

 pop     edx
 pop     ebx

 ret

_timer_get_approx_seconds:
 push    ebx
 push    edx

 mov     eax, [td_tick_count]
 imul    dword [td__timer_cnt]   ; edx:eax = Number of 1.19 MHz ticks elapsed...
 shld    edx, eax, 16            ; Keep 32+16 bits, for conversion to fixed point
 shl     eax, 16          
; edx:eax = number of 1.19Mhz pulses elapsed.
 mov     ebx, 1193180

approx_sub_again:
 sub     edx, ebx        ; subtract until negative...
 jns     approx_sub_again        ; ...to prevent divide overflow...
 add     edx, ebx        ; ...then add in to get correct value.

 div     ebx
; eax = fixed point seconds elapsed...

 pop     edx
 pop     ebx
 ret

;----------------------------------------------------------------------------
;Prototype: extern void timer_set_rate(int count_val);
;----------------------------------------------------------------------------
_timer_set_rate:
 mov eax,[esp+4]
; eax = rate
 pushad

; Make sure eax below or equal to 65535 and above 0
; if its not, make it be 65536 which is normal dos
; timing.
 cmp     eax, 65535
 jbe     @@
 mov     eax, 65536
@@:
 cmp     eax, 0
 jne     @@@
 mov     eax, 65536
@@@:    ; Set the timer rate to eax
 cli
 mov     dword [td_tick_count], 0
 mov     [td__timer_cnt], eax        
 mov     al, 34h         ; count in binary, mode 2, load low byte then hi byte, counter 0:  00 11 010 0
 out     TCOMMAND, al    ; Reset PIT channel 0
 mov     eax, [td__timer_cnt]
 out     TDATA, al
 mov     al, ah
 out     TDATA, al
 sti
 popad
 ret

;----------------------------------------------------------------------------
;Prototype: extern void timer_set_function( void * function );
;----------------------------------------------------------------------------
_timer_set_function:
; arg1 = near pointer to user function
 push eax
 mov	 eax, [esp+8] ; arg 1
 cli
 mov     dword [td_user_function+0], eax       ; offset
 mov     word  [td_user_function+4], cs         ; selector
 sti
 pop eax
 ret

_timer_set_joyhandler:
 mov	eax, [esp+4]
 cli
 mov    dword [td_joystick_poller], eax
 sti
 ret

;************************************************************************
;************************************************************************
;*****                                                              *****
;*****                T I M E R _ H A N D L E R                     *****
;*****                                                              *****
;************************************************************************
;************************************************************************

timer_handler:
 push    ds
 push    es
 push    eax

 mov     ax, [cs:__my_ds]   ; Interrupt, so point to our data segment
 mov     ds, ax
 mov     es, ax

; Increment time counter...
 inc     dword [td_tick_count]

 mov     eax, [td__timer_cnt]
 add     dword [td_dos_timer], eax                ; Increment DOS timer
 cmp     dword [td_dos_timer], 65536
 jb      NoChainToOld            ; See if we need to chain to DOS 18.2
 and     dword [td_dos_timer], 0ffffh

;
; Call the original DOS handler....
;
 pushfd
 call far dword [td_org_interrupt]

 jmp     NoReset         ;old handler has reset, so we don't

NoChainToOld:
; Reset controller
 mov     al, 20h         ; Reset interrupt controller
 out     20h, al

NoReset:
 cmp     byte [td_in_timer], 0
 jne     ExitInterrupt

 mov     byte [td_in_timer], 1           ; Mark that we're in a timer interrupt...

 ; Reenable interrupts
 sti                     ; Reenable interrupts

 cmp     word [td_user_function+4], 0          ; Check the selector...
 je      NoUserFunction

; Switch stacks while calling the user-definable function...
 pushad
 push    fs
 push    gs
 mov     dword [td_saved_stack+0], esp
 mov     word [td_saved_stack+4], ss
 lss     esp, [td_new_stack]        ; Switch to new stack
 call    far dword [td_user_function]     ; Call new function
 lss     esp, [td_saved_stack]     ; Switch back to original stack
 pop     gs
 pop     fs
 popad

NoUserFunction:	
 cmp     dword [td_joystick_poller], 0
 je      NoJoyPolling
 mov     eax, [td__timer_cnt]
 mov     dword [td_saved_stack+0], esp
 mov     word [td_saved_stack+4], ss
 lss     esp, [td_new_stack]        ; Switch to new stack
 pusha
 push eax
 call    dword [td_joystick_poller]
 pop eax
 popa
 lss     esp,  [td_saved_stack]    ; Switch back to original stack

NoJoyPolling:
 cli
 mov     byte [td_in_timer], 0

ExitInterrupt:
;;mov     al, 20h         ; Reset interrupt controller
;;out     20h, al
 pop     eax
 pop     es
 pop     ds
 iretd                           ; Return from timer interrupt

TIMER_LOCKED_CODE_STOP:

;************************************************************************
;************************************************************************
;*****                                                              *****
;*****                   T I M E R _ I N I T                        *****
;*****                                                              *****
;************************************************************************
;************************************************************************

_timer_init:
 pushad
 push    ds
 push    es

 cmp     byte [td_Installed], 1
 je      NEAR AlreadyInstalled

 mov     [__my_ds],ds

 mov     dword [td__timer_cnt], 65536     ; Set to BIOS's normal 18.2 Hz
 mov     dword [td_dos_timer], 0          ; clear DOS Interrupt counter
 mov     dword [td_user_function+0], 0 ; offset of user function
 mov     word [td_user_function+4], 0  ; selector of user function

 lea     eax, [ds:td_TimerStack]  ; Use EAX as temp stack pointer
 add     eax, STACK_SIZE                 ; Top of stack minus space for saving old ss:esp
 mov     dword [td_new_stack+0], eax
 mov     word [td_new_stack+4], ds

		;--------------- lock data used in interrupt
 mov     eax, (td_TimerStack-TimerData)+STACK_SIZE ;sizeof timer_data
 mov     esi, eax
 shr     esi, 16          
 mov     edi, eax
 and     edi, 0ffffh     ; si:di = length of region to lock in bytes
 mov	 ebx, TimerData
 add	 ebx, [___djgpp_base_address]
 mov	 ecx, ebx
 shr     ebx, 16
 and     ecx, 0ffffh     ; bx:cx = start of linear address to lock
 mov     eax, 0600h      ; DPMI lock address function
 int     31h             ; call dpmi
 jnc     @@@@
 int     3               ; LOCK FAILED!!
@@@@:
 ;--------------- lock code used in interrupt
 mov	 eax, TIMER_LOCKED_CODE_STOP
 sub	 eax, TIMER_LOCKED_CODE_START
 inc     eax             ; EAX = size of timer interrupt handler
 mov     esi, eax
 shr     esi, 16          
 mov     edi, eax
 and     edi, 0ffffh     ; si:di = length of region to lock in bytes
 mov	 ebx, TIMER_LOCKED_CODE_START
 add	 ebx, [___djgpp_base_address]
 mov	 ecx, ebx
 shr     ebx, 16
 and     ecx, 0ffffh     ; bx:cx = start of linear address to lock
 mov     eax, 0600h      ; DPMI lock address function
 int     31h             ; call dpmi
 jnc     @@@@@ ;This is getting fun
 int     3               ; LOCK FAILED!!
@@@@@:

;**************************************************************
;******************* SAVE OLD INT8 HANDLER ********************
;**************************************************************
 mov ax,0204h
 mov bl,8
 int 31h
 mov dword [td_org_interrupt+0],edx
 mov word [td_org_interrupt+4],cx

;**************************************************************
;***************** INSTALL NEW INT8 HANDLER *******************
;**************************************************************

 cli

 mov     al, 34h         ; count in binary, mode 2, load low byte then hi byte, counter 0:  00 11 010 0
 out     TCOMMAND, al    ; Reset PIT channel 0
 mov     eax, [td__timer_cnt]
 out     TDATA, al
 mov     al, ah
 out     TDATA, al

 mov     dword [td_tick_count], 0
 mov     byte [td_Installed],1

 mov ax,0205h
 mov bl,8
 mov edx,timer_handler
 mov cx,cs
 int 31h


 sti
 push	 dword _timer_close
 call    _atexit
 add esp,4

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

_timer_close:
 push    eax
 push    ebx
 push    ecx
 push    edx

 cmp     byte [td_Installed], 0
 je      NotInstalled
 mov     byte [td_Installed], 0

;**************************************************************
;***************** RESTORE OLD INT8 HANDLER *******************
;**************************************************************

 cli
 mov     al, 36h         ; count in binary, mode 3, load low byte then hi byte, counter 0:  00 11 011 0
 out     TCOMMAND, al    ; Reser PIT channel 0
 mov     ax, 0h
 out     TDATA, al
 mov     al, ah
 out     TDATA, al

 mov ax,0205h
 mov bl,8
 mov edx,dword [td_org_interrupt+0]
 mov cx,word [td_org_interrupt+4]
 int 31h

 sti
		
 cmp     dword [td_nested_counter], 0
 je      NoNestedInterrupts
 mov     eax, [td_nested_counter]
 ;int    3               ; Get John!!
	
NoNestedInterrupts:
		

NotInstalled:
 pop     edx
 pop     ecx
 pop     ebx
 pop     eax

 ret

