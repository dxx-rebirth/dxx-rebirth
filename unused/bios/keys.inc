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
;**************************************************************
;**************** SCAN CODE TO ASCII TABLES *******************
;**************************************************************
; This are two tables of ASCII codes for a given scan code.
; The character л is used to denote an invalid keystroke that won't
; be put into the keyboard buffer.

;0                         1                    2               3               4               5               6               7
;0   1  23456789ABCD  E F  0123456789AB   C  DEF0123456789ABCDEF0123456789ABCDEF0123456789ABCDEF0123456789ABCDEF0123456789ABCDEF0123456789ABCDEF
;db "л",27,"1234567890-=",8,9,"qwertyuiop[]",13,"лasdfghjkl;'`л\zxcvbnm,./л*л ллллллллллллл789-456+1230.лллллллллллллллллллллллллллллллллллллллллллл"

;0                  1               2                    3               4               5               6               7
;0123456789ABCDE   F0123456789ABCDEF01234567   8  9ABCDEF0123456789ABCDEF0123456789ABCDEF0123456789ABCDEF0123456789ABCDEF0123456789ABCDEF
;Key2 db "лл!@#$%^&*()_+л",11,"QWERTYUIOP{}ллASDFGHJKL:",34,"лл|ZXCVBNM<>?ллл лллллллллллллллл-ллл+ллллллллллллллллллллллллллллллллллллллллллллллллл"

Key1    db  255     ; No key for 0
		db  255     ; Esc
		db  '1'
		db  '2'
		db  '3'
		db  '4'
		db  '5'
		db  '6'
		db  '7'
		db  '8'
		db  '9'
		db  '0'
		db  '-'
		db  '='
		db  255     ; Backspace
		db  255     ; Tab
		db  'q'
		db  'w'
		db  'e'
		db  'r'
		db  't'
		db  'y'
		db  'u'
		db  'i'
		db  'o'
		db  'p'
		db  '['
		db  ']'
		db  255     ; Enter
		db  255     ; Ctrl
		db  'a'
		db  's'
		db  'd'
		db  'f'
		db  'g'
		db  'h'
		db  'j'
		db  'k'
		db  'l'
		db  ';'
		db  39      ; '
		db  '`'
		db  255     ; Left Shift
		db  '\'
		db  'z'
		db  'x'
		db  'c'
		db  'v'
		db  'b'
		db  'n'
		db  'm'
		db  ','
		db  '.'
		db  '/'
		db  255     ; Right Shift
		db  '*'     ; Prtsc or *
		db  255     ; Alt
		db  ' '     ; Spacebar
		db  255     ; Capslock
		db  255     ; F1
		db  255     ; F2
		db  255     ; F3
		db  255     ; F4
		db  255     ; F5
		db  255     ; F6
		db  255     ; F7
		db  255     ; F8
		db  255     ; F9
		db  255     ; F10
		db  255     ; Numlock
		db  255     ; ScrollLock
		db  255     ; Home or 7
		db  255     ; Up or 8
		db  255     ;
		db  255
		db  255
		db  255
		db  255
		db  255
		db  255
		db  255
		db  255
		db  255
		db  255
		db  255
		db  255
		db  255
		db  255
		db  255
		db  255
		db  255
		db  255
		db  255
		db  255
		db  255
		db  255
		db  255
		db  255
		db  255
		db  255
		db  255
		db  255
		db  255
		db  255
		db  255
		db  255
		db  255
		db  255
		db  255
		db  255
		db  255
		db  255
		db  255
		db  255
		db  255
		db  255
		db  255
		db  255
		db  255
		db  255
		db  255
		db  255
		db  255
		db  255
		db  255
		db  255
		db  255
		db  255
Key2    db  255     ; No key for 0
		db  255     ; Esc
		db  '!'
		db  '@'
		db  '#'
		db  '$'
		db  '%'
		db  '^'
		db  '&'
		db  '*'
		db  '('
		db  ')'
		db  '_'
		db  '+'
		db  255     ; Backspace
		db  255     ; Tab
		db  'Q'
		db  'W'
		db  'E'
		db  'R'
		db  'T'
		db  'Y'
		db  'U'
		db  'I'
		db  'O'
		db  'P'
		db  '{'
		db  '}'
		db  255     ; Enter
		db  255     ; Ctrl
		db  'A'
		db  'S'
		db  'D'
		db  'F'
		db  'G'
		db  'H'
		db  'J'
		db  'K'
		db  'L'
		db  ':'
		db  '"'
		db  '~'
		db  255     ; Left Shift
		db  '|'
		db  'Z'
		db  'X'
		db  'C'
		db  'V'
		db  'B'
		db  'N'
		db  'M'
		db  '<'
		db  '>'
		db  '?'
		db  255     ; Right Shift
		db  255     ; Prtsc or *
		db  255     ; Alt
		db  ' '     ; Spacebar
		db  255     ; Capslock
		db  255     ; F1
		db  255     ; F2
		db  255     ; F3
		db  255     ; F4
		db  255     ; F5
		db  255     ; F6
		db  255     ; F7
		db  255     ; F8
		db  255     ; F9
		db  255     ; F10
		db  255     ; Numlock
		db  255     ; ScrollLock
		db  255     ; Home or 7
		db  255     ; Up or 8
		db  255     ;
		db  255
		db  255
		db  255
		db  255
		db  255
		db  255
		db  255
		db  255
		db  255
		db  255
		db  255
		db  255
		db  255
		db  255
		db  255
		db  255
		db  255
		db  255
		db  255
		db  255
		db  255
		db  255
		db  255
		db  255
		db  255
		db  255
		db  255
		db  255
		db  255
		db  255
		db  255
		db  255
		db  255
		db  255
		db  255
		db  255
		db  255
		db  255
		db  255
		db  255
		db  255
		db  255
		db  255
		db  255
		db  255
		db  255
		db  255
		db  255
		db  255
		db  255
		db  255
		db  255
		db  255
		db  255



