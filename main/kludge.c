/* $Id: kludge.c,v 1.16 2004-11-28 05:16:38 btb Exp $ */

/*
 *
 * DPH: This is the file where all the stub functions go. The aim is
 * to have nothing in here, eventually
 *
 */

#ifdef HAVE_CONFIG_H
#include <conf.h>
#endif

#include <stdio.h>
#include <stdlib.h>

#include "gr.h"
#include "pstypes.h"
#include "maths.h"
#include "findfile.h"

extern int VGA_current_mode;

int Window_clip_left,Window_clip_top,Window_clip_right,Window_clip_bot;

char CDROM_dir[40] = ".";

#ifndef __DJGPP__
void joy_set_btn_values( int btn, int state, int time_down, int downcount, int upcount )
{

}
#endif

void key_putkey(char i)
{

}

void g3_remap_interp_colors()
{

}

/*
extern short interp_color_table
void g3_remap_interp_colors()
{
 int eax, ebx;
 
 ebx = 0;
 if (ebx != n_interp_colors) {
   eax = 0;
   eax = interp_color_table
 }

}
*/

int com_init(void)
{
	return 0;
}

void com_level_sync(void)
{

}

void com_main_menu()
{

}

void com_do_frame()
{

}

void com_send_data()
{

}

void com_endlevel()
{

}

void serial_leave_game()
{

}

void network_dump_appletalk_player(ubyte node, ushort net, ubyte socket, int why)
{

}

void digi_stop_digi_sounds(void)
{

}

void digi_pause_digi_sounds()
{

}

void digi_resume_digi_sounds()
{

}
void digi_play_sample_looping( int soundno, fix max_volume,int loop_start, int loop_end )
{

}

void digi_change_looping_volume( fix volume )
{

}

void digi_stop_looping_sound()
{

}

// Plays a queued voice sound.
void digi_start_sound_queued( short soundnum, fix volume )
{

}
