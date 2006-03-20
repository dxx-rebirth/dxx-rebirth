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
COPYRIGHT 1993-1998 PARALLAX SOFTWARE CORPORATION.  ALL RIGHTS RESERVED.
*/
/*
 * $Source: /cvsroot/dxx-rebirth/d1x-rebirth/main/automap.h,v $
 * $Revision: 1.1.1.1 $
 * $Author: zicodxx $
 * $Date: 2006/03/17 19:44:09 $
 * 
 * Prototypes for auto-map stuff.
 * 
 * $Log: automap.h,v $
 * Revision 1.1.1.1  2006/03/17 19:44:09  zicodxx
 * initial import
 *
 * Revision 1.3  1999/11/21 13:00:08  donut
 * Changed screen_mode format.  Now directly encodes res into a 32bit int, rather than using arbitrary values.
 *
 * Revision 1.2  1999/10/08 04:26:01  donut
 * automap fix for high fps(including -niceautomap), -automap_gameres, and better -font* help
 *
 * Revision 1.1.1.1  1999/06/14 22:12:08  donut
 * Import of d1x 1.37 source.
 *
 * Revision 2.0  1995/02/27  11:29:35  john
 * New version 2.0, which has no anonymous unions, builds with
 * Watcom 10.0, and doesn't require parsing BITMAPS.TBL.
 * 
 * Revision 1.5  1994/12/09  00:41:21  mike
 * fix hang in automap print screen
 * 
 * Revision 1.4  1994/07/14  11:25:29  john
 * Made control centers destroy better; made automap use Tab key.
 * 
 * Revision 1.3  1994/07/12  15:45:51  john
 * Made paritial map.
 * 
 * Revision 1.2  1994/07/07  18:35:05  john
 * First version of automap
 * 
 * Revision 1.1  1994/07/07  15:12:13  john
 * Initial revision
 * 
 * 
 */



#ifndef _AUTOMAP_H
#define _AUTOMAP_H

extern void do_automap(int key_code);
extern void automap_clear_visited();
extern ubyte Automap_visited[MAX_SEGMENTS];
extern void modex_print_message(int x, int y, char *str);

//added on 9/30/98 by Matt Mueller for selectable automap modes
extern u_int32_t automap_mode ; 
#define AUTOMAP_MODE (automap_use_game_res?Game_screen_mode:automap_mode)
//extern int automap_width ;
//extern int automap_height ;
//end addition -MM
extern int automap_use_game_res;
extern int nice_automap;

#endif
