/* $Id: modem.h,v 1.2 2003-10-10 09:36:35 btb Exp $ */
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

/*
 *
 * Header file for modem support
 *
 * Old Log:
 * Revision 1.2  1995/09/05  14:06:40  allender
 * checkpoint again
 *
 * Revision 1.1  1995/05/16  15:59:29  allender
 * Initial revision
 *
 * Revision 2.1  1995/03/21  14:40:38  john
 * Ifdef'd out the NETWORK code.
 *
 * Revision 2.0  1995/02/27  11:31:34  john
 * New version 2.0, which has no anonymous unions, builds with
 * Watcom 10.0, and doesn't require parsing BITMAPS.TBL.
 *
 * Revision 1.18  1994/11/22  17:12:05  rob
 * Starting working on secret level fix for modem games.
 *
 * Revision 1.17  1994/11/17  16:43:17  rob
 * Added prototype for com_level_sync function.
 *
 * Revision 1.16  1994/11/15  21:30:32  rob
 * Added prototype for new menu hook.
 *
 * Revision 1.15  1994/10/07  16:15:33  rob
 * Changed calls to multi_send_position.
 *
 * Revision 1.14  1994/10/07  12:52:24  rob
 * Fixed some problems.
 *
 * Revision 1.13  1994/10/07  11:25:47  rob
 * Tried to fix modem to work with new multi stuff.  Still a bit jacked up, tho.
 *
 * Revision 1.12  1994/10/05  19:14:50  rob
 * Exported macros and arrays to support network object mapping.
 *
 * Revision 1.11  1994/10/05  17:48:59  rob
 * Several changes, most to end_of_level sequencing.
 *
 * Revision 1.10  1994/10/05  14:22:54  rob
 * Added com_end_level.
 *
 * Revision 1.9  1994/09/30  18:37:22  rob
 * Another day's worth of work.  Mostly menus, error checking.  Added
 * level checksums during sync, carrier detect during all com calls,
 * generic com_abort() procedure for bailing out, and nm_messagebox
 * handling of QUIT instead of HUD message (which never gets seen).
 *
 * Revision 1.8  1994/09/29  20:55:16  rob
 * Lots of changes.
 *
 * Revision 1.7  1994/09/29  16:09:20  rob
 * Added explode stuff.
 *
 * Revision 1.6  1994/09/28  14:31:08  rob
 * Added serial setup menu.
 *
 * Revision 1.5  1994/09/27  15:02:49  rob
 * Null modem basic routines working.  Sending DEAD messages and
 * missiles still need to be done.
 *
 * Revision 1.4  1994/09/24  16:52:33  rob
 * Added stubbed funcs for startup and stop of serial games.
 *
 * Revision 1.3  1994/09/24  14:47:31  rob
 * New function protos.
 *
 * Revision 1.2  1994/09/22  17:53:29  rob
 * First revision, not yet functional.
 *
 * Revision 1.1  1994/09/22  12:39:25  rob
 * Initial revision
 *
 *
 */


#ifndef _MODEM_H
#define _MODEM_H

int com_enable(void);
void com_disable(void);
void com_do_frame(void);
void com_process_input(void);
void serial_leave_game(void);
void modem_start_game(void);
void com_main_menu(void);
void com_endlevel(int *secret);
void com_abort(void);
void com_send_data(char *buf, int len, int repeat);
int com_level_sync(void);


extern int com_port_num;
extern int serial_active;
extern int com_speed;
extern int com_baud_rate;

#endif /* _MODEM_H */
