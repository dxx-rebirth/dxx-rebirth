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

#ifdef HAVE_CONFIG_H
#include <conf.h>
#endif

#ifndef MACINTOSH			// I'm going to totally seperate these routines -- yeeech!!!!
							// see end of file for macintosh equivs

#ifdef WINDOWS
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include "winapp.h"
#else
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "pstypes.h"
#include "game.h"
#include "menu.h"
#include "movie.h"
#include "digi.h"
#include "kconfig.h"
#include "palette.h"
#include "joy.h"
#include "songs.h"
#include "args.h"
#include "player.h"
#include "mission.h"
#include "mono.h"
#include "pa_enabl.h"



#ifdef RCS
static char rcsid[] = "$Id: config.c,v 1.7 2003-06-16 06:57:34 btb Exp $";
#endif

ubyte Config_digi_volume = 8;
ubyte Config_midi_volume = 8;
ubyte Config_redbook_volume = 8;
ubyte Config_control_type = 0;
ubyte Config_channels_reversed = 0;
ubyte Config_joystick_sensitivity = 8;

#ifdef __MSDOS__
static char *digi_dev8_str = "DigiDeviceID8";
static char *digi_dev16_str = "DigiDeviceID16";
static char *digi_port_str = "DigiPort";
static char *digi_irq_str = "DigiIrq";
static char *digi_dma8_str = "DigiDma8";
static char *digi_dma16_str = "DigiDma16";
static char *midi_dev_str = "MidiDeviceID";
static char *midi_port_str = "MidiPort";

#define _CRYSTAL_LAKE_8_ST		0xe201
#define _CRYSTAL_LAKE_16_ST	0xe202
#define _AWE32_8_ST				0xe208
#define _AWE32_16_ST				0xe209
#endif
static char *digi_volume_str = "DigiVolume";
static char *midi_volume_str = "MidiVolume";
static char *redbook_enabled_str = "RedbookEnabled";
static char *redbook_volume_str = "RedbookVolume";
static char *detail_level_str = "DetailLevel";
static char *gamma_level_str = "GammaLevel";
static char *stereo_rev_str = "StereoReverse";
static char *joystick_min_str = "JoystickMin";
static char *joystick_max_str = "JoystickMax";
static char *joystick_cen_str = "JoystickCen";
static char *last_player_str = "LastPlayer";
static char *last_mission_str = "LastMission";
static char *config_vr_type_str = "VR_type";
static char *config_vr_resolution_str = "VR_resolution";
static char *config_vr_tracking_str = "VR_tracking";
static char *movie_hires_str = "MovieHires";

char config_last_player[CALLSIGN_LEN+1] = "";
char config_last_mission[MISSION_NAME_LEN+1] = "";

int Config_digi_type = 0;
int Config_digi_dma = 0;
int Config_midi_type = 0;

#ifdef WINDOWS
int	 DOSJoySaveMin[4];
int	 DOSJoySaveCen[4];
int	 DOSJoySaveMax[4];

char win95_current_joyname[256];
#endif



int Config_vr_type = 0;
int Config_vr_resolution = 0;
int Config_vr_tracking = 0;

int digi_driver_board_16;
int digi_driver_dma_16;

extern byte	Object_complexity, Object_detail, Wall_detail, Wall_render_depth, Debris_amount, SoundChannels;

void set_custom_detail_vars(void);


#define CL_MC0 0xF8F
#define CL_MC1 0xF8D
/*
void CrystalLakeWriteMCP( ushort mc_addr, ubyte mc_data )
{
	_disable();
	outp( CL_MC0, 0xE2 );				// Write password
	outp( mc_addr, mc_data );		// Write data
	_enable();
}

ubyte CrystalLakeReadMCP( ushort mc_addr )
{
	ubyte value;
	_disable();
	outp( CL_MC0, 0xE2 );		// Write password
	value = inp( mc_addr );		// Read data
	_enable();
	return value;
}

void CrystalLakeSetSB()
{
	ubyte tmp;
	tmp = CrystalLakeReadMCP( CL_MC1 );
	tmp &= 0x7F;
	CrystalLakeWriteMCP( CL_MC1, tmp );
}

void CrystalLakeSetWSS()
{
	ubyte tmp;
	tmp = CrystalLakeReadMCP( CL_MC1 );
	tmp |= 0x80;
	CrystalLakeWriteMCP( CL_MC1, tmp );
}
*/
//MovieHires might be changed by -nohighres, so save a "real" copy of it
int SaveMovieHires;
int save_redbook_enabled;

#ifdef WINDOWS
void CheckMovieAttributes()
{
		HKEY hKey;
		DWORD len, type, val;
		long lres;
  
		lres = RegOpenKeyEx(HKEY_LOCAL_MACHINE, "SOFTWARE\\Parallax\\Descent II\\1.1\\INSTALL",
							0, KEY_READ, &hKey);
		if (lres == ERROR_SUCCESS) {
			len = sizeof(val);
			lres = RegQueryValueEx(hKey, "HIRES", NULL, &type, &val, &len);
			if (lres == ERROR_SUCCESS) {
				MovieHires = val;
				logentry("HIRES=%d\n", val);
			}
			RegCloseKey(hKey);
		}
}
#endif



int ReadConfigFile()
{
	CFILE *infile;
	char line[80], *token, *value, *ptr;
	ubyte gamma;
	int joy_axis_min[7];
	int joy_axis_center[7];
	int joy_axis_max[7];
	int i;

	strcpy( config_last_player, "" );

	joy_axis_min[0] = joy_axis_min[1] = joy_axis_min[2] = joy_axis_min[3] = 0;
	joy_axis_max[0] = joy_axis_max[1] = joy_axis_max[2] = joy_axis_max[3] = 0;
	joy_axis_center[0] = joy_axis_center[1] = joy_axis_center[2] = joy_axis_center[3] = 0;

#ifdef WINDOWS
	memset(&joy_axis_min[0], 0, sizeof(int)*7);
	memset(&joy_axis_max[0], 0, sizeof(int)*7);
	memset(&joy_axis_center[0], 0, sizeof(int)*7);
//@@	joy_set_cal_vals(joy_axis_min, joy_axis_center, joy_axis_max);
#else
	joy_set_cal_vals(joy_axis_min, joy_axis_center, joy_axis_max);
#endif

	/*digi_driver_board = 0;
	digi_driver_port = 0;
	digi_driver_irq = 0;
	digi_driver_dma = 0;

	digi_midi_type = 0;
	digi_midi_port = 0;*/

	Config_digi_volume = 8;
	Config_midi_volume = 8;
	Config_redbook_volume = 8;
	Config_control_type = 0;
	Config_channels_reversed = 0;

	//set these here in case no cfg file
	SaveMovieHires = MovieHires;
	save_redbook_enabled = Redbook_enabled;

	infile = cfopen("descent.cfg", "rt");
	if (infile == NULL) {
		WIN(CheckMovieAttributes());
		return 1;
	}
	while (!cfeof(infile))
	{
		memset(line, 0, 80);
		cfgets(line, 80, infile);
		ptr = &(line[0]);
		while (isspace(*ptr))
			ptr++;
		if (*ptr != '\0') {
			token = strtok(ptr, "=");
			value = strtok(NULL, "=");
			if (value[strlen(value)-1] == '\n')
				value[strlen(value)-1] = 0;
/*			if (!strcmp(token, digi_dev8_str))
				digi_driver_board = strtol(value, NULL, 16);
			else if (!strcmp(token, digi_dev16_str))
				digi_driver_board_16 = strtol(value, NULL, 16);
			else if (!strcmp(token, digi_port_str))
				digi_driver_port = strtol(value, NULL, 16);
			else if (!strcmp(token, digi_irq_str))
				digi_driver_irq = strtol(value, NULL, 10);
			else if (!strcmp(token, digi_dma8_str))
				digi_driver_dma = strtol(value, NULL, 10);
			else if (!strcmp(token, digi_dma16_str))
				digi_driver_dma_16 = strtol(value, NULL, 10);
			else*/ if (!strcmp(token, digi_volume_str))
				Config_digi_volume = strtol(value, NULL, 10);
			else/* if (!strcmp(token, midi_dev_str))
				digi_midi_type = strtol(value, NULL, 16);
			else if (!strcmp(token, midi_port_str))
				digi_midi_port = strtol(value, NULL, 16);
			else*/ if (!strcmp(token, midi_volume_str))
				Config_midi_volume = strtol(value, NULL, 10);
			else if (!strcmp(token, redbook_enabled_str))
				Redbook_enabled = save_redbook_enabled = strtol(value, NULL, 10);
			else if (!strcmp(token, redbook_volume_str))
				Config_redbook_volume = strtol(value, NULL, 10);
			else if (!strcmp(token, stereo_rev_str))
				Config_channels_reversed = strtol(value, NULL, 10);
			else if (!strcmp(token, gamma_level_str)) {
				gamma = strtol(value, NULL, 10);
				gr_palette_set_gamma( gamma );
			}
			else if (!strcmp(token, detail_level_str)) {
				Detail_level = strtol(value, NULL, 10);
				if (Detail_level == NUM_DETAIL_LEVELS-1) {
					int count,dummy,oc,od,wd,wrd,da,sc;

					count = sscanf (value, "%d,%d,%d,%d,%d,%d,%d\n",&dummy,&oc,&od,&wd,&wrd,&da,&sc);

					if (count == 7) {
						Object_complexity = oc;
						Object_detail = od;
						Wall_detail = wd;
						Wall_render_depth = wrd;
						Debris_amount = da;
						SoundChannels = sc;
						set_custom_detail_vars();
					}
				  #ifdef PA_3DFX_VOODOO   // Set to highest detail because you can't change em	
					   Object_complexity=Object_detail=Wall_detail=
						Wall_render_depth=Debris_amount=SoundChannels = NUM_DETAIL_LEVELS-1;
						Detail_level=NUM_DETAIL_LEVELS-1;
						set_custom_detail_vars();
					#endif
				}
			}
			else if (!strcmp(token, joystick_min_str))	{
				sscanf( value, "%d,%d,%d,%d", &joy_axis_min[0], &joy_axis_min[1], &joy_axis_min[2], &joy_axis_min[3] );
			} 
			else if (!strcmp(token, joystick_max_str))	{
				sscanf( value, "%d,%d,%d,%d", &joy_axis_max[0], &joy_axis_max[1], &joy_axis_max[2], &joy_axis_max[3] );
			}
			else if (!strcmp(token, joystick_cen_str))	{
				sscanf( value, "%d,%d,%d,%d", &joy_axis_center[0], &joy_axis_center[1], &joy_axis_center[2], &joy_axis_center[3] );
			}
			else if (!strcmp(token, last_player_str))	{
				char * p;
				strncpy( config_last_player, value, CALLSIGN_LEN );
				p = strchr( config_last_player, '\n');
				if ( p ) *p = 0;
			}
			else if (!strcmp(token, last_mission_str))	{
				char * p;
				strncpy( config_last_mission, value, MISSION_NAME_LEN );
				p = strchr( config_last_mission, '\n');
				if ( p ) *p = 0;
			} else if (!strcmp(token, config_vr_type_str)) {
				Config_vr_type = strtol(value, NULL, 10);
			} else if (!strcmp(token, config_vr_resolution_str)) {
				Config_vr_resolution = strtol(value, NULL, 10);
			} else if (!strcmp(token, config_vr_tracking_str)) {
				Config_vr_tracking = strtol(value, NULL, 10);
			} else if (!strcmp(token, movie_hires_str)) {
				SaveMovieHires = MovieHires = strtol(value, NULL, 10);
			}
		}
	}

	cfclose(infile);

#ifdef WINDOWS
	for (i=0;i<4;i++)
	{
	 DOSJoySaveMin[i]=joy_axis_min[i];
	 DOSJoySaveCen[i]=joy_axis_center[i];
	 DOSJoySaveMax[i]=joy_axis_max[i];
   	}
#else
	joy_set_cal_vals(joy_axis_min, joy_axis_center, joy_axis_max);
#endif

	i = FindArg( "-volume" );
	
	if ( i > 0 )	{
		i = atoi( Args[i+1] );
		if ( i < 0 ) i = 0;
		if ( i > 100 ) i = 100;
		Config_digi_volume = (i*8)/100;
		Config_midi_volume = (i*8)/100;
		Config_redbook_volume = (i*8)/100;
	}

	if ( Config_digi_volume > 8 ) Config_digi_volume = 8;
	if ( Config_midi_volume > 8 ) Config_midi_volume = 8;
	if ( Config_redbook_volume > 8 ) Config_redbook_volume = 8;

	digi_set_volume( (Config_digi_volume*32768)/8, (Config_midi_volume*128)/8 );
/*
	printf( "DigiDeviceID: 0x%x\n", digi_driver_board );
	printf( "DigiPort: 0x%x\n", digi_driver_port		);
	printf( "DigiIrq: 0x%x\n",  digi_driver_irq		);
	printf( "DigiDma: 0x%x\n",	digi_driver_dma	);
	printf( "MidiDeviceID: 0x%x\n", digi_midi_type	);
	printf( "MidiPort: 0x%x\n", digi_midi_port		);
  	key_getch();
*/

	/*Config_midi_type = digi_midi_type;
	Config_digi_type = digi_driver_board;
	Config_digi_dma = digi_driver_dma;*/

#if 0
	if (digi_driver_board_16 > 0 && !FindArg("-no16bit") && digi_driver_board_16 != _GUS_16_ST) {
		digi_driver_board = digi_driver_board_16;
		digi_driver_dma = digi_driver_dma_16;
	}

	// HACK!!!
	//Hack to make some cards look like others, such as
	//the Crytal Lake look like Microsoft Sound System
	if ( digi_driver_board == _CRYSTAL_LAKE_8_ST )	{
		ubyte tmp;
		tmp = CrystalLakeReadMCP( CL_MC1 );
		if ( !(tmp & 0x80) )
			atexit( CrystalLakeSetSB );		// Restore to SB when done.
	 	CrystalLakeSetWSS();
		digi_driver_board = _MICROSOFT_8_ST;
	} else if ( digi_driver_board == _CRYSTAL_LAKE_16_ST )	{
		ubyte tmp;
		tmp = CrystalLakeReadMCP( CL_MC1 );
		if ( !(tmp & 0x80) )
			atexit( CrystalLakeSetSB );		// Restore to SB when done.
	 	CrystalLakeSetWSS();
		digi_driver_board = _MICROSOFT_16_ST;
	} else if ( digi_driver_board == _AWE32_8_ST )	{
		digi_driver_board = _SB16_8_ST;
	} else if ( digi_driver_board == _AWE32_16_ST )	{
		digi_driver_board = _SB16_16_ST;
	} else
		digi_driver_board		= digi_driver_board;
#else
	infile = cfopen("descentw.cfg", "rt");
	if (infile) {
		while (!cfeof(infile))
		{
			memset(line, 0, 80);
			cfgets(line, 80, infile);
			ptr = &(line[0]);
			while (isspace(*ptr))
				ptr++;
			if (*ptr != '\0') {
				token = strtok(ptr, "=");
				value = strtok(NULL, "=");
				if (value[strlen(value)-1] == '\n')
					value[strlen(value)-1] = 0;
				if (!strcmp(token, joystick_min_str))	{
					sscanf( value, "%d,%d,%d,%d,%d,%d,%d", &joy_axis_min[0], &joy_axis_min[1], &joy_axis_min[2], &joy_axis_min[3], &joy_axis_min[4], &joy_axis_min[5], &joy_axis_min[6] );
				} 
				else if (!strcmp(token, joystick_max_str))	{
					sscanf( value, "%d,%d,%d,%d,%d,%d,%d", &joy_axis_max[0], &joy_axis_max[1], &joy_axis_max[2], &joy_axis_max[3], &joy_axis_max[4], &joy_axis_max[5], &joy_axis_max[6] );
				}
				else if (!strcmp(token, joystick_cen_str))	{
					sscanf( value, "%d,%d,%d,%d,%d,%d,%d", &joy_axis_center[0], &joy_axis_center[1], &joy_axis_center[2], &joy_axis_center[3], &joy_axis_center[4], &joy_axis_center[5], &joy_axis_center[6] );
				}
			}
		}
		cfclose(infile);
	}
#endif

	return 0;
}

int WriteConfigFile()
{
	CFILE *infile;
	char str[256];
	int joy_axis_min[7];
	int joy_axis_center[7];
	int joy_axis_max[7];
	ubyte gamma = gr_palette_get_gamma();
	
	joy_get_cal_vals(joy_axis_min, joy_axis_center, joy_axis_max);

#ifdef WINDOWS
	for (i=0;i<4;i++)
   {
	 joy_axis_min[i]=DOSJoySaveMin[i];
	 joy_axis_center[i]=DOSJoySaveCen[i];
	 joy_axis_max[i]=DOSJoySaveMax[i];
   }
#endif

	infile = cfopen("descent.cfg", "wt");
	if (infile == NULL) {
		return 1;
	}
	/*sprintf (str, "%s=0x%x\n", digi_dev8_str, Config_digi_type);
	cfputs(str, infile);
	sprintf (str, "%s=0x%x\n", digi_dev16_str, digi_driver_board_16);
	cfputs(str, infile);
	sprintf (str, "%s=0x%x\n", digi_port_str, digi_driver_port);
	cfputs(str, infile);
	sprintf (str, "%s=%d\n", digi_irq_str, digi_driver_irq);
	cfputs(str, infile);
	sprintf (str, "%s=%d\n", digi_dma8_str, Config_digi_dma);
	cfputs(str, infile);
	sprintf (str, "%s=%d\n", digi_dma16_str, digi_driver_dma_16);
	cfputs(str, infile);*/
	sprintf (str, "%s=%d\n", digi_volume_str, Config_digi_volume);
	cfputs(str, infile);
	/*sprintf (str, "%s=0x%x\n", midi_dev_str, Config_midi_type);
	cfputs(str, infile);
	sprintf (str, "%s=0x%x\n", midi_port_str, digi_midi_port);
	cfputs(str, infile);*/
	sprintf (str, "%s=%d\n", midi_volume_str, Config_midi_volume);
	cfputs(str, infile);
	sprintf (str, "%s=%d\n", redbook_enabled_str, FindArg("-noredbook")?save_redbook_enabled:Redbook_enabled);
	cfputs(str, infile);
	sprintf (str, "%s=%d\n", redbook_volume_str, Config_redbook_volume);
	cfputs(str, infile);
	sprintf (str, "%s=%d\n", stereo_rev_str, Config_channels_reversed);
	cfputs(str, infile);
	sprintf (str, "%s=%d\n", gamma_level_str, gamma);
	cfputs(str, infile);
	if (Detail_level == NUM_DETAIL_LEVELS-1)
		sprintf (str, "%s=%d,%d,%d,%d,%d,%d,%d\n", detail_level_str, Detail_level,
				Object_complexity,Object_detail,Wall_detail,Wall_render_depth,Debris_amount,SoundChannels);
	else
		sprintf (str, "%s=%d\n", detail_level_str, Detail_level);
	cfputs(str, infile);

	sprintf (str, "%s=%d,%d,%d,%d\n", joystick_min_str, joy_axis_min[0], joy_axis_min[1], joy_axis_min[2], joy_axis_min[3] );
	cfputs(str, infile);
	sprintf (str, "%s=%d,%d,%d,%d\n", joystick_cen_str, joy_axis_center[0], joy_axis_center[1], joy_axis_center[2], joy_axis_center[3] );
	cfputs(str, infile);
	sprintf (str, "%s=%d,%d,%d,%d\n", joystick_max_str, joy_axis_max[0], joy_axis_max[1], joy_axis_max[2], joy_axis_max[3] );
	cfputs(str, infile);

	sprintf (str, "%s=%s\n", last_player_str, Players[Player_num].callsign );
	cfputs(str, infile);
	sprintf (str, "%s=%s\n", last_mission_str, config_last_mission );
	cfputs(str, infile);
	sprintf (str, "%s=%d\n", config_vr_type_str, Config_vr_type );
	cfputs(str, infile);
	sprintf (str, "%s=%d\n", config_vr_resolution_str, Config_vr_resolution );
	cfputs(str, infile);
	sprintf (str, "%s=%d\n", config_vr_tracking_str, Config_vr_tracking );
	cfputs(str, infile);
	sprintf (str, "%s=%d\n", movie_hires_str, (FindArg("-nohires") || FindArg("-nohighres") || FindArg("-lowresmovies"))?SaveMovieHires:MovieHires);
	cfputs(str, infile);

	cfclose(infile);

#ifdef WINDOWS
{
//	Save Windows Config File
	char joyname[256];
						

	joy_get_cal_vals(joy_axis_min, joy_axis_center, joy_axis_max);
	
	infile = cfopen("descentw.cfg", "wt");
	if (infile == NULL) return 1;

	sprintf(str, "%s=%d,%d,%d,%d,%d,%d,%d\n", joystick_min_str, 
			joy_axis_min[0], joy_axis_min[1], joy_axis_min[2], joy_axis_min[3],
			joy_axis_min[4], joy_axis_min[5], joy_axis_min[6]);
	cfputs(str, infile);
	sprintf(str, "%s=%d,%d,%d,%d,%d,%d,%d\n", joystick_cen_str, 
			joy_axis_center[0], joy_axis_center[1], joy_axis_center[2], joy_axis_center[3],
			joy_axis_center[4], joy_axis_center[5], joy_axis_center[6]);
	cfputs(str, infile);
	sprintf(str, "%s=%d,%d,%d,%d,%d,%d,%d\n", joystick_max_str, 
			joy_axis_max[0], joy_axis_max[1], joy_axis_max[2], joy_axis_max[3],
			joy_axis_max[4], joy_axis_max[5], joy_axis_max[6]);
	cfputs(str, infile);

	cfclose(infile);
}
	CheckMovieAttributes();
#endif

	return 0;
}		

#else		// !defined(MACINTOSH)

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include <Memory.h>
#include <Folders.h>
#include <GestaltEqu.h>
#include <Errors.h>
#include <Processes.h>
#include <Resources.h>
#include <LowMem.h>

#include "pa_enabl.h"		// because some prefs rely on this fact
#include "error.h"
#include "pstypes.h"
#include "game.h"
#include "digi.h"
#include "kconfig.h"
#include "palette.h"
#include "joy.h"
#include "args.h"
#include "player.h"
#include "mission.h"
#include "prefs.h"			// prefs file for configuration stuff -- from DeSalvo

#if defined(POLY_ACC)
#include "poly_acc.h"
#endif

#ifdef RCS
static char rcsid[] = "$Id: config.c,v 1.7 2003-06-16 06:57:34 btb Exp $";
#endif

#define MAX_CTB_LEN	512

typedef struct preferences {
	ubyte	digi_volume;
	ubyte	midi_volume;
	ubyte	stereo_reverse;
	ubyte	detail_level;
	ubyte	oc;					// object complexity
	ubyte	od;					// object detail
	ubyte	wd;					// wall detail
	ubyte	wrd;				// wall render depth
	ubyte	da;					// debris amount
	ubyte	sc;					// sound channels
	ubyte	gamma_level;
	ubyte	pixel_double;
	int		joy_axis_min[4];
	int		joy_axis_max[4];
	int		joy_axis_center[4];
	char	lastplayer[CALLSIGN_LEN+1];
	char	lastmission[MISSION_NAME_LEN+1];
	char	ctb_config[MAX_CTB_LEN];
	int		ctb_tool;
	ubyte	master_volume;
	ubyte	display_dialog;
	ubyte	change_resolution;
	ubyte	nosound;
	ubyte	nomidi;
	ubyte	sound_11k;
	ubyte	no_movies;
	ubyte	game_monitor;
	ubyte	redbook_volume;
	ubyte	enable_rave;
	ubyte	enable_input_sprockets;
} Preferences;

char config_last_player[CALLSIGN_LEN+1] = "";
char config_last_mission[MISSION_NAME_LEN+1] = "";
char config_last_ctb_cfg[MAX_CTB_LEN] = "";
int config_last_ctb_tool;
ubyte Config_master_volume = 4;
ubyte Config_digi_volume = 8;
ubyte Config_midi_volume = 8;
ubyte Config_redbook_volume = 8;
ubyte Config_control_type = 0;
ubyte Config_channels_reversed = 0;
ubyte Config_joystick_sensitivity = 8;

int Config_vr_type = 0;
int Config_vr_resolution = 0;
int Config_vr_tracking = 0;

extern byte	Object_complexity, Object_detail, Wall_detail, Wall_render_depth, Debris_amount, SoundChannels;
extern void digi_set_master_volume( int volume );

void set_custom_detail_vars(void);

static ubyte have_prefs = 0;

//¥	------------------------------	Private Definitions
//¥	------------------------------	Private Types

typedef struct
{
	Str31	fileName;
	OSType	creator;
	OSType	fileType;
	OSType	resType;
	short	resID;
} PrefsInfo, *PrefsInfoPtr, **PrefsInfoHandle;

//¥	------------------------------	Private Variables

static PrefsInfo		prefsInfo;
static Boolean		prefsInited = 0;

//¥	------------------------------	Private Functions

static void Pstrcpy(StringPtr dst, StringPtr src);
static void Pstrcat(StringPtr dst, StringPtr src);
static Boolean FindPrefsFile(short *prefVRefNum, long *prefDirID);

//¥	--------------------	Pstrcpy

static void
Pstrcpy(StringPtr dst, StringPtr src)
{
	BlockMove(src, dst, (*src) + 1);
}

//¥	--------------------	Pstrcat

static void
Pstrcat(StringPtr dst, StringPtr src)
{
	BlockMove(src + 1, dst + (*dst) + 1, *src);
	*dst += *src;
}

//¥	--------------------	FindPrefsFile

static Boolean
FindPrefsFile(short *prefVRefNum, long *prefDirID)
{
OSErr		theErr;
long			response;
CInfoPBRec	infoPB;

	if (! prefsInited)
		return (0);
		
	theErr = Gestalt(gestaltFindFolderAttr, &response);
	if (theErr == noErr && ((response >> gestaltFindFolderPresent) & 1))
	{
		//¥	Find (or make) it the easy way...
		theErr = FindFolder(kOnSystemDisk, kPreferencesFolderType, kCreateFolder, prefVRefNum, prefDirID);
	}
	else
	{
	SysEnvRec	theSysEnv;
	StringPtr		prefFolderName = "\pPreferences";

		//¥	yeachh -- we have to do it all by hand!
		theErr = SysEnvirons(1, &theSysEnv);
		if (theErr != noErr)
			return (0);
			
		*prefVRefNum = theSysEnv.sysVRefNum;
		
		//¥	Check whether Preferences folder already exists
		infoPB.hFileInfo.ioCompletion	= 0;
		infoPB.hFileInfo.ioNamePtr	= prefFolderName;
		infoPB.hFileInfo.ioVRefNum	= *prefVRefNum;
		infoPB.hFileInfo.ioFDirIndex	= 0;
		infoPB.hFileInfo.ioDirID		= 0;

		theErr = PBGetCatInfo(&infoPB, 0);
		if (theErr == noErr)
		{
			*prefDirID = infoPB.hFileInfo.ioDirID;
		}
		else if (theErr == fnfErr)		//¥	Preferences doesn't already exist
		{
		HParamBlockRec	dirPB;
		
			//¥	Create "Preferences" folder
			dirPB.fileParam.ioCompletion	= 0;
			dirPB.fileParam.ioVRefNum	= *prefVRefNum;
			dirPB.fileParam.ioNamePtr	= prefFolderName;
			dirPB.fileParam.ioDirID		= 0;

			theErr = PBDirCreate(&dirPB, 0);
			if (theErr == noErr)
				*prefDirID = dirPB.fileParam.ioDirID;
		}
	}
	
	//¥	If we make it here OK, create Preferences file if necessary
	if (theErr == noErr)
	{
		infoPB.hFileInfo.ioCompletion	= 0;
		infoPB.hFileInfo.ioNamePtr	= prefsInfo.fileName;
		infoPB.hFileInfo.ioVRefNum	= *prefVRefNum;
		infoPB.hFileInfo.ioFDirIndex	= 0;
		infoPB.hFileInfo.ioDirID		= *prefDirID;

		theErr = PBGetCatInfo(&infoPB, 0);
		if (theErr == fnfErr)
		{
			theErr = HCreate(*prefVRefNum, *prefDirID, prefsInfo.fileName, prefsInfo.creator, prefsInfo.fileType);
			if (theErr == noErr)
			{
				HCreateResFile(*prefVRefNum, *prefDirID, prefsInfo.fileName);
				theErr = ResError();
			}
		}
	}
	
	return (theErr == noErr);
}

//¥	--------------------	InitPrefsFile

#define UNKNOWN_TYPE 0x3f3f3f3f

void
InitPrefsFile(OSType creator)
{
	OSErr err;
PrefsInfoHandle		piHdl;
	
	if ((piHdl = (PrefsInfoHandle) GetResource('PRFI', 0)) == nil)
	{
	ProcessSerialNumber	thePSN;
	ProcessInfoRec			thePIR;
	FSSpec				appSpec;
	StringPtr			app_string;

#if 0	
		GetCurrentProcess(&thePSN);
		thePIR.processName = nil;
		thePIR.processAppSpec = &appSpec;
		
		//¥	Set default to 'ÇApplicationÈ Prefs', PREF 0
		err = GetProcessInformation(&thePSN, &thePIR);
		if (err)
			Int3();
#endif
		app_string = LMGetCurApName();
//		Pstrcpy(prefsInfo.fileName, appSpec.name);
		Pstrcpy(prefsInfo.fileName, app_string);
		Pstrcat(prefsInfo.fileName, "\p Preferences");
		
		//¥	Set creator to calling application's signature (should be able to
		//¥	Determine this automatically, but unable to for some reason)
		prefsInfo.creator = creator;
		prefsInfo.fileType = 'pref';
		prefsInfo.resType = 'pref';
		prefsInfo.resID = 0;
	}
	else
	{
		//¥	Get Preferences file setup from PRFI 0
		BlockMove(*piHdl, &prefsInfo, sizeof (prefsInfo));
		ReleaseResource((Handle) piHdl);
		
		if (prefsInfo.creator == UNKNOWN_TYPE)
			prefsInfo.creator = creator;
	}
	
	prefsInited = 1;
}

//¥	--------------------	LoadPrefsFile

OSErr
LoadPrefsFile(Handle prefsHdl)
{
short	prefVRefNum, prefRefNum;
long		prefDirID;
OSErr	theErr = noErr;
Handle	origHdl;
Size		prefSize, origSize;

	if (prefsHdl == nil)
		return (nilHandleErr);

	prefSize = GetHandleSize(prefsHdl);
		
	if (! FindPrefsFile(&prefVRefNum, &prefDirID))
		return (fnfErr);

	prefRefNum = HOpenResFile(prefVRefNum, prefDirID, prefsInfo.fileName, fsRdWrPerm);
	if (prefRefNum == -1)
		return (ResError());
	
	//¥	Not finding the resource is not an error -- caller will use default data
	if ((origHdl = Get1Resource(prefsInfo.resType, prefsInfo.resID)) != nil)
	{
		origSize = GetHandleSize(origHdl);
		if (origSize > prefSize)			//¥	Extend handle for extra stored data
			SetHandleSize(prefsHdl, origSize);

		BlockMove(*origHdl, *prefsHdl, origSize);
		ReleaseResource(origHdl);
	}
	
	CloseResFile(prefRefNum);

	if (theErr == noErr)
		theErr = ResError();
	
	return (theErr);
}

//¥	--------------------	SavePrefsFile

OSErr
SavePrefsFile(Handle prefHdl)
{
short	prefVRefNum, prefRefNum;
long		prefDirID;
Handle	origHdl = nil;
Size		origSize, prefSize;
OSErr	theErr = noErr;
	
	if (! FindPrefsFile(&prefVRefNum, &prefDirID))
		return (fnfErr);
	
	if (prefHdl == nil)
		return (nilHandleErr);

	prefSize = GetHandleSize(prefHdl);

	prefRefNum = HOpenResFile(prefVRefNum, prefDirID, prefsInfo.fileName, fsRdWrPerm);
	if (prefRefNum == -1)
		return (ResError());
		
	if ((origHdl = Get1Resource(prefsInfo.resType, prefsInfo.resID)) != nil)
	{
		//¥	Overwrite existing preferences
		origSize = GetHandleSize(origHdl);
		if (prefSize > origSize)
			SetHandleSize(origHdl, prefSize);
			
		BlockMove(*prefHdl, *origHdl, prefSize);
		ChangedResource(origHdl);
		WriteResource(origHdl);
		ReleaseResource(origHdl);
	}
	else
	{
		//¥	Store specified preferences for the first time
		AddResource(prefHdl, prefsInfo.resType, prefsInfo.resID, "\p");
		WriteResource(prefHdl);
		DetachResource(prefHdl);
	}
	
	CloseResFile(prefRefNum);

	if (theErr == noErr)
		theErr = ResError();
	
	return (theErr);
}

//¥	-------------------------------------------------------------------------------------------

/*

	This module provides the ability to save and load a preferences file in the
	Preferences folder in the System Folder.  An optional resource, PRFI 0,
	is used to provide specifications for the preferences file (creator, etc.).

	Three functions are provided:

		void InitPrefsFile(OSType creator)

	This function will initialize the preferences file, that is, it will create
	it in the appropriate place if it doesn't currently exist.  It should be
	called with the creator code for the application.  Note that the creator
	code specified in PRFI 0 (if any) will be used only if the creator code
	passed to this function is '????'.  Without the PRFI 0 resource, the default
	specifications are:

	File Name: "{Application} Prefs" (i.e., the name of the app plus " Prefs"
	Creator: the creator passed to InitPrefsFile
	Type: 'PREF'
	Pref Resource Type: 'PREF'
 	Pref Resource ID: 0

	The PRFI 0 resource allows you to specify overrides for each of the above
	values.  This is useful for development, since the application name might
	go through changes, but the preferences file name is held constant.
 
	 	OSErr LoadPrefsFile(Handle prefsHndl)

	This function will attempt to copy the data stored in the preferences
	file to the given handle (which must be pre-allocated).  If the handle is too
	small, then it will be enlarged.  If it is too large, it will not be resized.
	The data in the preferences file (normally in PREF 0) will then be copied
	into the handle.  If the preferences file did not exist, the original data
	in the handle will not change.

		OSErr SavePrefsFile(Handle prefsHndl)

	This function will attempt to save the given handle to the preferences
	file.  Its contents will completely replace the previous data (normally
	the PREF 0 resource).

	In typical use, you will use InitPrefsFile once, then allocate a handle large
	enough to contain your preferences data, and initialize it with default values.
	Throughout the course of your program, the handle will undergo modification as
	user preferences change.  You can use SavePrefsFile anytime to update the
	preferences file, or wait until program exit to do so.

*/

int ReadConfigFile()
{
	int i;
	OSErr err;
	Handle prefs_handle;
	Preferences *prefs;
	char *p;
	
	if (!have_prefs) {			// not initialized....get a handle to the preferences file
		InitPrefsFile('DCT2');
		have_prefs = 1;
	}
	
	prefs_handle = NewHandleClear(sizeof(Preferences));		// new prefs handle
	if (prefs_handle == NULL)
		return;
		
	prefs = (Preferences *)(*prefs_handle);
	err = LoadPrefsFile(prefs_handle);
	if (err) {
		DisposeHandle(prefs_handle);
		return -1;
	}

	p = (char *)prefs;
	for (i = 0; i < sizeof(Preferences); i++) {
		if (*p != 0)
			break;
		p++;
	}
	if ( i == sizeof(Preferences) )
		return -1;
	
	Config_digi_volume = prefs->digi_volume;
	Config_midi_volume = prefs->midi_volume;
	Config_master_volume = prefs->master_volume;
	Config_redbook_volume = prefs->redbook_volume;
	Config_channels_reversed = prefs->stereo_reverse;
	gr_palette_set_gamma( (int)(prefs->gamma_level) );

	Scanline_double = (int)prefs->pixel_double;
	if ( PAEnabled )
		Scanline_double = 0;		// can't double with hardware acceleration
		
	Detail_level = prefs->detail_level;
	if (Detail_level == NUM_DETAIL_LEVELS-1) {
		Object_complexity = prefs->oc;
		Object_detail = prefs->od;
		Wall_detail = prefs->wd;
		Wall_render_depth = prefs->wrd;
		Debris_amount = prefs->da;
		SoundChannels = prefs->sc;
		set_custom_detail_vars();
	}
  #ifdef PA_3DFX_VOODOO   // Set to highest detail because you can't change em	
		   Object_complexity=Object_detail=Wall_detail=
			Wall_render_depth=Debris_amount=SoundChannels = NUM_DETAIL_LEVELS-1;
			Detail_level=NUM_DETAIL_LEVELS-1;
			set_custom_detail_vars();
  #endif

	strncpy( config_last_player, prefs->lastplayer, CALLSIGN_LEN );
	p = strchr(config_last_player, '\n' );
	if (p) *p = 0;
	
	strncpy(config_last_mission, prefs->lastmission, MISSION_NAME_LEN);
	p = strchr(config_last_mission, '\n' );
	if (p) *p = 0;

	strcpy(config_last_ctb_cfg, prefs->ctb_config);
	
	if ( Config_digi_volume > 8 ) Config_digi_volume = 8;

	if ( Config_midi_volume > 8 ) Config_midi_volume = 8;

	joy_set_cal_vals( prefs->joy_axis_min, prefs->joy_axis_center, prefs->joy_axis_max);
	digi_set_volume( (Config_digi_volume*256)/8, (Config_midi_volume*256)/8 );
	digi_set_master_volume(Config_master_volume);
	
	gConfigInfo.mDoNotDisplayOptions = prefs->display_dialog;
	gConfigInfo.mUse11kSounds = prefs->sound_11k;
	gConfigInfo.mDisableSound = prefs->nosound;
	gConfigInfo.mDisableMIDIMusic = prefs->nomidi;
	gConfigInfo.mChangeResolution = prefs->change_resolution;
	gConfigInfo.mDoNotPlayMovies = prefs->no_movies;
	gConfigInfo.mGameMonitor = prefs->game_monitor;
	gConfigInfo.mAcceleration = prefs->enable_rave;
	gConfigInfo.mInputSprockets = prefs->enable_input_sprockets;
	
	DisposeHandle(prefs_handle);
	return 0;
}

int WriteConfigFile()
{
	OSErr err;
	Handle prefs_handle;
	Preferences *prefs;
	
	prefs_handle = NewHandleClear(sizeof(Preferences));		// new prefs handle
	if (prefs_handle == NULL)
		return;
		
	prefs = (Preferences *)(*prefs_handle);
	
	joy_get_cal_vals(prefs->joy_axis_min, prefs->joy_axis_center, prefs->joy_axis_max);
	prefs->digi_volume = Config_digi_volume;
	prefs->midi_volume = Config_midi_volume;
	prefs->stereo_reverse = Config_channels_reversed;
	prefs->detail_level = Detail_level;
	if (Detail_level == NUM_DETAIL_LEVELS-1) {
		prefs->oc = Object_complexity;
		prefs->od = Object_detail;
		prefs->wd = Wall_detail;
		prefs->wrd = Wall_render_depth;
		prefs->da = Debris_amount;
		prefs->sc = SoundChannels;
	}
	prefs->gamma_level = (ubyte)gr_palette_get_gamma();

	if ( !PAEnabled )
		prefs->pixel_double = (ubyte)Scanline_double;		// hmm..don't write this out if doing hardware accel.
		
	strncpy( prefs->lastplayer, Players[Player_num].callsign, CALLSIGN_LEN );
	strncpy( prefs->lastmission, config_last_mission, MISSION_NAME_LEN );
	strcpy( prefs->ctb_config, config_last_ctb_cfg);
	prefs->ctb_tool = config_last_ctb_tool;
	prefs->master_volume = Config_master_volume;
	prefs->display_dialog = gConfigInfo.mDoNotDisplayOptions;
	prefs->change_resolution = gConfigInfo.mChangeResolution;
	prefs->nosound = gConfigInfo.mDisableSound;
	prefs->nomidi = gConfigInfo.mDisableMIDIMusic;
	prefs->sound_11k = gConfigInfo.mUse11kSounds;
	prefs->no_movies = gConfigInfo.mDoNotPlayMovies;
	prefs->game_monitor = gConfigInfo.mGameMonitor;
	prefs->redbook_volume = Config_redbook_volume;
	prefs->enable_rave = gConfigInfo.mAcceleration;
	prefs->enable_input_sprockets = gConfigInfo.mInputSprockets;

	err = SavePrefsFile(prefs_handle);
	DisposeHandle(prefs_handle);
	return (int)err;
}

#endif

