/* DPH: This is the file where all the stub functions go. The aim is to have nothing in here ,eventually */
#include <conf.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "gr.h"
#include "pstypes.h"
#include "maths.h"
#include "findfile.h"

int gr_renderstats = 0;
int gr_badtexture = 0;

extern int VGA_current_mode;
int Dont_start_sound_objects = 1;

int Window_clip_left,Window_clip_top,Window_clip_right,Window_clip_bot;

#if 0
char CDROM_dir[40] = ".";
#else
char CDROM_dir[40] = "/cdrom/d2data/";
#endif

#ifndef __DJGPP__
int gr_check_mode(u_int32_t a)
{
  return 0;
} 
#endif

extern int Num_computed_colors;
void gr_copy_palette(ubyte *gr_palette, ubyte *pal, int size)
{
	        memcpy(gr_palette, pal, size);

	        Num_computed_colors = 0;
}

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

int digi_link_sound_to_object2( int org_soundnum, short objnum, int forever, fix max_volume, fix  max_distance );
int digi_link_sound_to_object3( int org_soundnum, short objnum, int forever, fix max_volume, fix  max_distance, int loop_start, int loop_end )
{
	//FIXME: Total hack
	return digi_link_sound_to_object2(org_soundnum, objnum, forever, max_volume, max_distance);
}

void digi_stop_sound(int channel)
{

}

void digi_stop_digi_sounds(void)
{

}

void digi_stop_current_song(void)
{


}

void digi_set_midi_volume(int a)
{

}

void digi_play_midi_song(void)
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
