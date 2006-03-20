/*
THE COMPUTER CODE CONTAINED HEREIN IS THE SOLE PROPERTY OF PARALLAX
SOFTWARE CORPORATION ("PARALLAX").  PARALLAX, IN DISTRIBUTING THE CODE TO
END-USERS, AND SUBJECT TO ALL OF THE TERMS AND CONDITIONS HEREIN, GRANTS A
ROYALTY-FREE, PERPETUAL LICENSE TO SUCH END-USERS FOR USE BY SUCH END-USERS
IN USING, DISPLAYING,  AND CREATING DERIVATIVE WORKS THEREOF, SO LONG AS
SUCH USE, DISPLAY OR CREATION IS FOR NON-COMMERCIAL, ROYALTY OR REVENUE
FREE PURPOSES.  IN NO EVENT SHALL THE END-USER USE THE COMPUTER CODE
CONTAINED HEREIN FOR REVENUE-BEARING PURPOSES.  THE END-USER UNDERSTANDS
AND AGREES TO THE TERMS HEREIN AND ACCEPTS THE SAME BY USE OF THIS FILE.  
COPYRIGHT 1993-1999 PARALLAX SOFTWARE CORPORATION.  ALL RIGHTS RESERVED.
*/



#ifndef _COINDEV_H
#define _COINDEV_H

/* Header file to define the JoyStick access functions */
/* and bit-masks.                                      */
/*                                                     */
/* Updates:                                            */
/*                                                     */
/*    + Reworked the conio.h check to work with both   */
/*      Borland and Watcom compilers.                  */
/*    + Added UPDATE_JOYSTICK_VALUE and global value   */
/*      JoyStickValue to allow for a stable state of   */
/*      joystick condition.  This means that once at   */
/*      the begining of each game loop (frame) you     */
/*      should execute UPDATE_JOYSTICK_VALUE, then     */
/*      use JoyStickValue for the remainder of that    */
/*      entire loop to determine joystick settings.    */
/*    + Added values needed by the CoinMech section    */
/*      of the digital I/O card.                       */

#define IOCARD_BASEPORT         0x2A0

//#define JOYSTICK_PORT           (IOCARD_BASEPORT + 1)
//#define JS_TRIGGER              0x0001
//#define JS_BUTTONLEFT           0x0002
//#define JS_BUTTONRIGHT          0x0004
//#define JS_BUTTONCENTER         0x0008
//#define JS_UP                   0x0010
//#define JS_DOWN                 0x0020
//#define JS_LEFT                 0x0040
//#define JS_RIGHT                0x0080

#define COINMECH_CMDPORT        (IOCARD_BASEPORT + 7)
#define COINMECH_ADJPORT        (IOCARD_BASEPORT + 3)

#define COINMECH1_PORT          (IOCARD_BASEPORT + 4)
#define COINMECH2_PORT          (IOCARD_BASEPORT + 5)
#define COINMECH3_PORT          (IOCARD_BASEPORT + 6)

#define COINMECH1_ADJMASK       0x10
#define COINMECH2_ADJMASK       0x20
#define COINMECH3_ADJMASK       0x40

#define COINMECH1_CTRLMASK      0x30
#define COINMECH2_CTRLMASK      0x70
#define COINMECH3_CTRLMASK      0xB0


// Include file for use with gameport coin mech interface and parallel
// port joystick interface

//#define GAMEPORT_ADDR 0x201

//#define PARALLEL_ADDR 0x378

#define MECH1   16
#define MECH2   32
#define MECH3   64
// #define MECH4          // Not supported yet.

//#define FIRE    1
//#define LBUTTON 2
//#define RBUTTON 4
//#define CBUTTON 8

//#define UP      16
//#define DOWN    32
//#define LEFT    64
//#define RIGHT   128

//#define JOY_GAMEPORT  0
//#define JOY_PARALLEL  1
//#define JOY_IODEV     2

int coindev_init(int CoinMechNumber);
unsigned int coindev_read(int CoinMechNumber);
unsigned int coindev_count(int CoinMechNumber);

#define ARCADE_FIRST_SECONDS 		120
#define ARCADE_CONTINUE_SECONDS	60

#endif
