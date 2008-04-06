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
 * Routines to manage the songs in Descent.
 *
 */


#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "error.h"
#include "pstypes.h"
#include "songs.h"
#include "cfile.h"
#include "digi.h"

song_info Songs[MAX_SONGS];
int Songs_initialized = 0;
int cGameSongsAvailable = 0;

void songs_init()
{
	int i;
	char inputline[80+1];
	CFILE * fp;

	if ( Songs_initialized ) return;

	fp = cfopen( "descent.sng", "rb" );
	if ( fp == NULL )	{
		int i;
		
		for (i = 0; i < SONG_LEVEL_MUSIC + NUM_GAME_SONGS; i++) {
			strcpy(Songs[i].melodic_bank_file, "melodic.bnk");
			strcpy(Songs[i].drum_bank_file, "drum.bnk");
			if (i >= SONG_LEVEL_MUSIC)
			{
				sprintf(Songs[i].filename, "game%02d.hmp", i - SONG_LEVEL_MUSIC + 1);
				if (!digi_music_exists(Songs[i].filename))
					sprintf(Songs[i].filename, "game%d.hmp", i - SONG_LEVEL_MUSIC);
				if (!digi_music_exists(Songs[i].filename))
				{
					Songs[i].filename[0] = '\0';	// music not available
					break;
				}
			}
		}
		strcpy(Songs[SONG_TITLE].filename, "descent.hmp");
		strcpy(Songs[SONG_BRIEFING].filename, "briefing.hmp");
		strcpy(Songs[SONG_CREDITS].filename, "credits.hmp");
		strcpy(Songs[SONG_ENDLEVEL].filename, "endlevel.hmp");	// can't find it? give a warning
		strcpy(Songs[SONG_ENDGAME].filename, "endgame.hmp");	// ditto
		cGameSongsAvailable = i - SONG_LEVEL_MUSIC;
		return;
	}

	i = 0;
	while (cfgets(inputline, 80, fp )) {
		char *p = strchr(inputline,'\n');
		if (p) *p = '\0';
		if ( strlen( inputline ) )	{
			Assert( i < MAX_SONGS );
			sscanf( inputline, "%15s %15s %15s", Songs[i].filename, Songs[i].melodic_bank_file, Songs[i].drum_bank_file );
			i++;
		}
	}

	// HACK: If Descent.hog is patched from 1.0 to 1.5, descent.sng is broken and will not exceed 12 songs. So let's HACK it here.
	if (i==12)
	{
		sprintf(Songs[i].filename,"game08.hmp"); sprintf(Songs[i].melodic_bank_file,"rickmelo.bnk"); sprintf(Songs[i].drum_bank_file,"rickdrum.bnk");
		i++;
		sprintf(Songs[i].filename,"game09.hmp"); sprintf(Songs[i].melodic_bank_file,"melodic.bnk"); sprintf(Songs[i].drum_bank_file,"drum.bnk");
		i++;
		sprintf(Songs[i].filename,"game10.hmp"); sprintf(Songs[i].melodic_bank_file,"melodic.bnk"); sprintf(Songs[i].drum_bank_file,"drum.bnk");
		i++;
		sprintf(Songs[i].filename,"game11.hmp"); sprintf(Songs[i].melodic_bank_file,"intmelo.bnk"); sprintf(Songs[i].drum_bank_file,"intdrum.bnk");
		i++;
		sprintf(Songs[i].filename,"game12.hmp"); sprintf(Songs[i].melodic_bank_file,"melodic.bnk"); sprintf(Songs[i].drum_bank_file,"drum.bnk");
		i++;
		sprintf(Songs[i].filename,"game13.hmp"); sprintf(Songs[i].melodic_bank_file,"intmelo.bnk"); sprintf(Songs[i].drum_bank_file,"intdrum.bnk");
		i++;
		sprintf(Songs[i].filename,"game14.hmp"); sprintf(Songs[i].melodic_bank_file,"intmelo.bnk"); sprintf(Songs[i].drum_bank_file,"intdrum.bnk");
		i++;
		sprintf(Songs[i].filename,"game15.hmp"); sprintf(Songs[i].melodic_bank_file,"melodic.bnk"); sprintf(Songs[i].drum_bank_file,"drum.bnk");
		i++;
		sprintf(Songs[i].filename,"game16.hmp"); sprintf(Songs[i].melodic_bank_file,"melodic.bnk"); sprintf(Songs[i].drum_bank_file,"drum.bnk");
		i++;
		sprintf(Songs[i].filename,"game17.hmp"); sprintf(Songs[i].melodic_bank_file,"melodic.bnk"); sprintf(Songs[i].drum_bank_file,"drum.bnk");
		i++;
		sprintf(Songs[i].filename,"game18.hmp"); sprintf(Songs[i].melodic_bank_file,"intmelo.bnk"); sprintf(Songs[i].drum_bank_file,"intdrum.bnk");
		i++;
		sprintf(Songs[i].filename,"game19.hmp"); sprintf(Songs[i].melodic_bank_file,"melodic.bnk"); sprintf(Songs[i].drum_bank_file,"drum.bnk");
		i++;
		sprintf(Songs[i].filename,"game20.hmp"); sprintf(Songs[i].melodic_bank_file,"melodic.bnk"); sprintf(Songs[i].drum_bank_file,"drum.bnk");
		i++;
		sprintf(Songs[i].filename,"game21.hmp"); sprintf(Songs[i].melodic_bank_file,"intmelo.bnk"); sprintf(Songs[i].drum_bank_file,"intdrum.bnk");
		i++;
		sprintf(Songs[i].filename,"game22.hmp"); sprintf(Songs[i].melodic_bank_file,"hammelo.bnk"); sprintf(Songs[i].drum_bank_file,"hamdrum.bnk");
		i++;
	}

	cGameSongsAvailable = i - SONG_LEVEL_MUSIC;
	Songs_initialized = 1;
	cfclose(fp);
}

int loop;

void songs_play_song( int songnum, int repeat )
{
	if ( !Songs_initialized ) songs_init();

	loop=repeat;

	digi_play_midi_song( Songs[songnum].filename, Songs[songnum].melodic_bank_file, Songs[songnum].drum_bank_file, repeat );
}

void songs_play_level_song( int levelnum )
{
	int songnum;

	Assert( levelnum != 0 );

	if ( !Songs_initialized ) songs_init();

	if (cGameSongsAvailable < 1)
		return;

	if (levelnum < 0)
		songnum = (-levelnum) % cGameSongsAvailable;
	else
		songnum = (levelnum-1) % cGameSongsAvailable;

	songnum += SONG_LEVEL_MUSIC;
	digi_play_midi_song( Songs[songnum].filename, Songs[songnum].melodic_bank_file, Songs[songnum].drum_bank_file, 1 );
}
