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

MISC_OUTPUT     = 03c2h             ;Miscellaneous Output register
MAP_MASK        = 02h               ;index in SC of Map Mask register
READ_MAP        = 04h               ;index in GC of the Read Map register
BIT_MASK        = 08h               ;index in GC of Bit Mask register

SC_INDEX        = 3c4h             ;Index register for sequencer ctrl.
SC_MAP_MASK     = 2                ;Number of map mask register

SC_INDEX        = 3c4h             ;Index register for sequencer ctrl.
SC_MAP_MASK     = 2                ;Number of map mask register
SC_MEM_MODE     = 4                ;Number of memory mode register

GC_INDEX        = 3ceh             ;Index register for graphics ctrl.
GC_READ_MAP     = 4                ;Number of read map register
GC_GRAPH_MODE   = 5                ;Number of graphics mode register
GC_MISCELL      = 6                ;Number of miscellaneous register

CRTC_INDEX      = 3d4h             ;Index register for CRT controller
CC_MAX_SCAN     = 9                ;Number of maximum scan line reg.
CC_START_HI     = 0Ch              ;Number of start address high register
CC_START_LO     = 0Dh              ;Number of start address low register
CC_UNDERLINE    = 14h              ;Number of underline register
CC_MODE_CTRL    = 17h              ;Number of mode control register
CRTC_OFFSET     = 13h     ; CRTC offset register index

DAC_WRITE_ADR   = 3C8h             ;DAC write address
DAC_READ_ADR    = 3C7h             ;DAC read address
DAC_DATA        = 3C9h             ;DAC data register

VERT_RESCAN     = 3DAh             ;Input status register #1

