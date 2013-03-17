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
 * Code for localizable text
 *
 */


#include <stdlib.h>
#include <string.h>

#include "physfsx.h"
#include "pstypes.h"
#include "u_mem.h"
#include "dxxerror.h"
#include "inferno.h"
#include "text.h"
#include "args.h"

char *text;

char *Text_string[N_TEXT_STRINGS];

void free_text()
{
	d_free(Text_string[330]);
	d_free(text);
}

// rotates a byte left one bit, preserving the bit falling off the right
void
encode_rotate_left(char *c)
{
	int found;

	found = 0;
	if (*c & 0x80)
		found = 1;
	*c = *c << 1;
	if (found)
		*c |= 0x01;
}

#define BITMAP_TBL_XOR 0xD3

//decode an encoded line of text of bitmaps.tbl
void decode_text_line(char *p)
{
	for (;*p;p++) {
		encode_rotate_left(p);
		*p = *p ^ BITMAP_TBL_XOR;
		encode_rotate_left(p);
	}
}

// decode buffer of text, preserves newlines
void decode_text(char *buf, int len)
{
	char *ptr;
	int i;

	for (i = 0, ptr = buf; i < len; i++, ptr++)
	{
		if (*ptr != '\n')
		{
			encode_rotate_left(ptr);
			*ptr = *ptr  ^ BITMAP_TBL_XOR;
			encode_rotate_left(ptr);
		}
	}
}

//load all the text strings for Descent
void load_text()
{
	PHYSFS_file  *tfile;
	PHYSFS_file *ifile;
	int len,i, have_binary = 0;
	char *tptr;
	char *filename="descent.tex";
	char *extra_strings[] = {
		"done",
		"I am a",
		"CHEATER!",
		"Loading Data",
		"ALT-F2\t  Save Game",
		"ALT-F3\t  Load Game",
		"Only in Registered version!",
		"Concussion",
		"Homing",
		"ProxBomb",
		"Smart",
		"Mega",
		"Mission '%s' not found.\nYou must have this mission\nfile in order to playback\nthis demo.",
		"All player callsigns on screen",
		"There is already a game\nin progress with that name",
		"This mission is designated\nAnarchy-only",
		"Force level start",
		"Quitting now means ending the\nentire netgame\n\nAre you sure?",
		"The mission for that netgame\nis not installed on your\nsystem.  Cannot join.",
		"Start Multiplayer Game\n\nSelect mission",
		"Error loading mission file",
		"Custom (return to set)",
		"Base address (in Hex)",
		"IRQ Number",
		"Reset to Default",
		"Valid IRQ values are 2-7",
		"No UART was detected\nat those settings",
		"You will pay dearly for that!",
		"Revenge is mine!!",
		"Man I'm good!",
		"Its almost too easy!",
		"   Mission:",
		"+/- Changes viewing distance",
		"Alternate exit found!\n\nProceeding to Secret Level!",
		"Show all players on automap",
		"Killed by a robot",
		"Baud",
		"A consistency error has been\ndetected in your network connection.\nCheck you hardware and re-join",
		"Press any key to continue (Print Screen to save screenshot)",
		"An error occured while writing\ndemo.  Demo is likely corrupted.\nEnter demo name now or\npress ESC to delete demo.",
		"The main reactor is invulnerable for",
		"The level being loaded is not\navailable in Destination Saturn.\nUnable to continue demo playback.\n\nPress any key to continue.",
		"Reactor life",
		"min",
		"Current IPX Socket is default",
		"This program requires MS-DOS 5.0 or higher.\nYou are using MS-DOS",
		"You can use the -nodoscheck command line\nswitch to override this check, but it\nmay have unpredictable results, namely\nwith DOS file error handling.\n",
		"Not enough file handles!",
		"of the necessary file handles\nthat Descent requires to execute properly.  You will\nneed to increase the FILES=n line in your config.sys.",
		"If you are running with a clean boot, then you will need\nto create a CONFIG.SYS file in your root directory, with\nthe line FILES=15 in it.  If you need help with this,\ncontact Interplay technical support.",
		"You may also run with the -nofilecheck command line option\nthat will disable this check, but you might get errors\nwhen loading saved games or playing demos.",
		"Available memory",
		"more bytes of DOS memory needed!",
		"more bytes of virtual memory needed.  Reconfigure VMM.",
		"more bytes of extended/expanded memory needed!",
		"Or else you you need to use virtual memory (See README.TXT)",
		"more bytes of physical memory needed!",
		"Check to see that your virtual memory settings allow\nyou to use all of your physical memory (See README.TXT)",
		"Initializing DPMI services",
		"Initializing critical error handler",
		"Enables Virtual I/O Iglasses! stereo display",
		"Enables Iglasses! head tracking via COM port",
		"Enables Kasan's 3dMax stereo display in low res.",
		"3DBios must be installed for 3dMax operation.",
		"Enables Kasan's 3dMax stereo display in high res",
		"Press any key for more options...",
		"Enables dynamic socket changing",
		"Disables the file handles check",
		"Getting settings from DESCENT.CFG...",
		"Initializing timer system...",
		"Initializing keyboard handler...",
		"Initializing mouse handler...",
		"Mouse support disabled...",
		"Initializing joystick handler...",
		"Slow joystick reading enabled...",
		"Polled joystick reading enabled...",
		"BIOS joystick reading enabled...",
		"Joystick support disabled...",
		"Initializing divide by zero handler...",
		"Initializing network...",
		"Using IPX network support on channel",
		"No IPX compatible network found.",
		"Error opening socket",
		"Not enough low memory for IPX buffers.",
		"Error initializing IPX.  Error code:",
		"Network support disabled...",
		"Initializing graphics system...",
		"SOUND: Error opening",
		"SOUND: Error locking down instruments",
		"SOUND: (HMI)",
		"SOUND: Error locking down drums",
		"SOUND: Error locking midi track map!",
		"SOUND: Error locking midi callback function!",
		"Using external control:",
		"Invalid serial port parameter for -itrak!",
		"Initializing i-glasses! head tracking on serial port %d",
		"Make sure the glasses are turned on!",
		"Press ESC to abort",
		"Failed to open serial port.  Status =",
		"Message",
		"Macro",
		"Error locking serial interrupt routine!",
		"Error locking serial port data!",
		"Robots are normal",
		"Robots move fast, fire seldom",
		"Robot painting OFF",
		"Robot painting with texture %d"
	};

	if (GameArg.DbgAltTex)
		filename = GameArg.DbgAltTex;

	if ((tfile = PHYSFSX_openReadBuffered(filename)) == NULL) {
		filename="descent.txb";
		if ((ifile = PHYSFSX_openReadBuffered(filename)) == NULL) {
			Error("Cannot open file DESCENT.TEX or DESCENT.TXB");
			return;
		}
		have_binary = 1;

		len = PHYSFS_fileLength(ifile);

//edited 05/17/99 Matt Mueller - malloc an extra byte, and null terminate.
		MALLOC(text,char,len+1);

		PHYSFS_read(ifile,text,1,len);
		text[len]=0;
//end edit -MM
		PHYSFS_close(ifile);

	} else {
		int c;
		char * p;

		len = PHYSFS_fileLength(tfile);

//edited 05/17/99 Matt Mueller - malloc an extra byte, and null terminate.
		MALLOC(text,char,len+1);

		//fread(text,1,len,tfile);
		p = text;
		do {
			c = PHYSFSX_fgetc( tfile );
			if ( c != 13 )
				*p++ = c;
		} while ( c!=EOF );
		*p=0;
//end edit -MM

		PHYSFS_close(tfile);
	}

	for (i=0,tptr=text;i<N_TEXT_STRINGS;i++) {
		char *p;
		char *buf;

		if (!tptr && i >= N_TEXT_STRINGS_MIN)	// account for non-registered 1.4/1.5 text files
		{
			Text_string[i-1] = extra_strings[i - N_TEXT_STRINGS_MIN - 1];
			Text_string[i] = extra_strings[i - N_TEXT_STRINGS_MIN];
			continue;
		}
		else if (!tptr && i < N_TEXT_STRINGS_MIN)
		{
			Error("Not enough strings in text file - expecting %d (or at least %d), found %d",N_TEXT_STRINGS,N_TEXT_STRINGS_MIN,i);
		}
		
		Text_string[i] = tptr;

		tptr = strchr(tptr,'\n');

		if ( tptr ) *tptr++ = 0;

		if (have_binary)
			decode_text_line(Text_string[i]);

		//scan for special chars (like \n)
		for (p=Text_string[i];(p=strchr(p,'\\'));) {
			char newchar;

			if (p[1] == 'n') newchar = '\n';
			else if (p[1] == 't') newchar = '\t';
			else if (p[1] == '\\') newchar = '\\';
			else
				Error("Unsupported key sequence <\\%c> on line %d of file <%s>",p[1],i+1,filename);

			p[0] = newchar;
// 			strcpy(p+1,p+2);
			MALLOC(buf,char,len+1);
			strcpy(buf,p+2);
			strcpy(p+1,buf);
			d_free(buf);
			p++;
		}

          switch(i) {
				  char *extra;
				  char *str;

			case 116:
				if (!d_stricmp(Text_string[i], "SPREADFIRE")) // This string is too long to fit in the cockpit-box
				{
					memset(Text_string[i], '\0', sizeof(char)*(strlen(Text_string[i])+1));
					strncpy(Text_string[i], "SPREAD", sizeof(char)*6);
				}
				break;
				  
			  case 330:
				  extra = "\n<Ctrl-C> converts format\nIntel <-> PowerPC";
				  str = d_malloc(strlen(Text_string[i]) + strlen(extra) + 1);
				  if (!str)
					  break;
				  strcpy(str, Text_string[i]);
				  strcat(str, extra);
				  Text_string[i] = str;
				  break;
          }
	}

	//Assert(tptr==text+len || tptr==text+len-2);
}


