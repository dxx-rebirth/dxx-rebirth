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
 *
 * contains routine(s) to read in the configuration file which contains
 * game configuration stuff
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "pstypes.h"
#include "game.h"
#include "digi.h"
#include "kconfig.h"
#include "palette.h"
#include "args.h"
#include "player.h"
#include "mission.h"

char config_last_player[CALLSIGN_LEN+1] = "";
char config_last_mission[MISSION_NAME_LEN+1] = "";
static char *DigiVolumeStr="DigiVolume";
static char *MidiVolumeStr="MidiVolume";
static char *ReverseStereoStr="ReverseStereo";
static char *GammaLevelStr="GammaLevelStr";
static char *DetailLevelStr="DetailLevelStr";
static char *LastPlayerStr="LastPlayerStr";
static char *LastMissionStr="LastMissionStr";
static char *ResolutionXStr="ResolutionX";
static char *ResolutionYStr="ResolutionY";
static int Config_render_width=0, Config_render_height=0;

extern sbyte	Object_complexity, Object_detail, Wall_detail, Wall_render_depth, Debris_amount, SoundChannels;

void set_custom_detail_vars(void);

#ifdef __unix__
#include <unistd.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
int check_and_create_dir(const char *name)
{
	struct stat stat_buffer;

	if (stat(name, &stat_buffer))
	{
		/* error check if it doesn't exist or something else is wrong */
		if (errno == ENOENT)
		{
			/* doesn't exist letts create it ;) */
			if (mkdir(name, 0775))
			{
				fprintf(stderr, "Error creating dir %s", name);
				perror(" ");
				return -1;
			}
		}
		else
		{
			/* something else went wrong yell about it */
			fprintf(stderr, "Error opening %s", name);
			perror(" ");
			return -1;
		}
	}
	else
	{
		/* file exists check it's a dir otherwise yell about it */
		if (!(S_IFDIR & stat_buffer.st_mode))
		{
			fprintf(stderr,"Error %s exists but isn't a dir\n", name);
			return -1;
		}
	}
	return 0;
}
#endif

int ReadConfigFile()
{
	FILE *infile;
	char line[80], *token, *value, *ptr;
	ubyte gamma;

	strcpy( config_last_player, "" );

	Config_digi_volume = 4;
	Config_midi_volume = 4;
	Config_control_type = 0;

#if defined(__unix__)
	/* we abuse the line buf here todo some unix specific stuff */
	ptr = getenv("HOME");
	snprintf(line, sizeof(line), "%s/.d1x-rebirth", ptr? ptr:".");
	/* If we succeed we do a chdir, because patching the file load/save
	   code to use this path is easy, but then we also need to hack the
	   file-selector, so just doing a chdir is easier. */
	if (!check_and_create_dir(line))
		chdir(line);
#endif

        infile = fopen("descent.cfg", "rt");

	if (infile == NULL)
		return 1;

	while (!feof(infile)) {
		memset(line, 0, 80);
		if (!(ptr = fgets(line, 80, infile)))
			break;
		while (isspace(*ptr))
			ptr++;
		if (*ptr != '\0') {
			token = strtok(ptr, "=");
			value = strtok(NULL, "=");
                        if (!strcmp(token, DigiVolumeStr))
				Config_digi_volume = strtol(value, NULL, 10);
			else if (!strcmp(token, MidiVolumeStr))
				Config_midi_volume = strtol(value, NULL, 10);
			else if (!strcmp(token, ReverseStereoStr))
				Config_channels_reversed = strtol(value, NULL, 10);
			else if (!strcmp(token, GammaLevelStr)) {
				gamma = strtol(value, NULL, 10);
				gr_palette_set_gamma( gamma );
			}
			else if (!strcmp(token, DetailLevelStr)) {
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
				}
			}
			else if (!strcmp(token, LastPlayerStr))	{
				char * p;
				strncpy( config_last_player, value, CALLSIGN_LEN );
				p = strchr( config_last_player, '\n');
				if ( p ) *p = 0;
			}
			else if (!strcmp(token, LastMissionStr))	{
				char * p;
				strncpy( config_last_mission, value, MISSION_NAME_LEN );
				p = strchr( config_last_mission, '\n');
				if ( p ) *p = 0;
			}
			else if (!strcmp(token, ResolutionXStr))
				Config_render_width = strtol(value, NULL, 10);
			else if (!strcmp(token, ResolutionYStr))
				Config_render_height = strtol(value, NULL, 10);
		}
	}

	fclose(infile);

        if ( Config_digi_volume > 8 ) Config_digi_volume = 8;

	if ( Config_midi_volume > 8 ) Config_midi_volume = 8;

	digi_set_volume( (Config_digi_volume*32768)/8, (Config_midi_volume*128)/8 );

	if (Config_render_width >= 320 && Config_render_height >= 200)
		Game_screen_mode = SM(Config_render_width,Config_render_height);

	return 0;
}

int WriteConfigFile()
{
	FILE *infile;
	char str[256];
	ubyte gamma = gr_palette_get_gamma();
	
	infile = fopen("descent.cfg", "wt");

	if (infile == NULL) {
		return 1;
	}
	sprintf (str, "%s=%d\n", DigiVolumeStr, Config_digi_volume);
	fputs(str, infile);
	sprintf (str, "%s=%d\n", MidiVolumeStr, Config_midi_volume);
	fputs(str, infile);
	sprintf (str, "%s=%d\n", ReverseStereoStr, Config_channels_reversed);
	fputs(str, infile);
	sprintf (str, "%s=%d\n", GammaLevelStr, gamma);
	fputs(str, infile);
	if (Detail_level == NUM_DETAIL_LEVELS-1)
		sprintf (str, "%s=%d,%d,%d,%d,%d,%d,%d\n", DetailLevelStr, Detail_level,
				Object_complexity,Object_detail,Wall_detail,Wall_render_depth,Debris_amount,SoundChannels);
	else
		sprintf (str, "%s=%d\n", DetailLevelStr, Detail_level);
	fputs(str, infile);
	sprintf (str, "%s=%s\n", LastPlayerStr, Players[Player_num].callsign );
	fputs(str, infile);
	sprintf (str, "%s=%s\n", LastMissionStr, config_last_mission );
	fputs(str, infile);
	sprintf (str, "%s=%i\n", ResolutionXStr, SM_W(Game_screen_mode));
	fputs(str, infile);
	sprintf (str, "%s=%i\n", ResolutionYStr, SM_H(Game_screen_mode));
	fputs(str, infile);

	fclose(infile);

	return 0;
}

