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

#ifdef NETWORK

#define DOS4G

#pragma off (unreferenced)
static char rcsid[] = "$Id: modem.c,v 1.1.1.1 2001-01-19 03:30:07 bradleyb Exp $";
#pragma on (unreferenced)

#include <stdio.h>
#include <stdlib.h>
#include <dos.h>
#include <conio.h>
#include <string.h>
#include <time.h>
#include <io.h>

#include "pa_enabl.h"       //$$POLY_ACC
#include "fast.h" // Commlib stuff
#include "game.h"
#include "scores.h"
#include "modem.h"
#include "object.h"
#include "player.h"
#include "laser.h"
#include "error.h"
#include "collide.h"
#include "endlevel.h"
#include "network.h"
#include "mono.h"
#include "gauges.h"
#include "newmenu.h"
#include "menu.h"
#include "gamesave.h"
#include "netmisc.h"
#include "cntrlcen.h"
#include "dpmi.h"
#include "commlib.h"
#include "glfmodem.h"
#include "multi.h"
#include "timer.h"
#include "text.h"
#include "pcx.h"
#include "palette.h"
#include "sounds.h"
#include "digi.h"
#include "multibot.h"
#include "args.h"

#if defined(POLY_ACC)
#include "poly_acc.h"
#endif

#define MIN_COMM_GAP 8000
#define INIT_STRING_LEN 20
#define LEN_PHONE_NUM_OLD 15
#define LEN_PHONE_NUM 	30
#define LEN_PHONE_NAME 12
#define NUM_PHONE_NUM 8

// How many times to repeat 'reliable' messages

#define EOR_MARK 0xaa

#define COM_PROCESS_NORMAL 0
#define COM_PROCESS_ENDLEVEL 1
#define COM_PROCESS_SYNC 2
#define COM_PROCESS_MENU 3

#define SELECTION_STARTGAME		1
#define SELECTION_NO_START			2
#define SELECTION_YES_START		3
#define SELECTION_STARTGAME_ABORT	4
#define SELECTION_CLOSE_LINK		5

int default_base[4] = { 0x3f8, 0x2f8, 0x3e8, 0x2e8 };
int default_irq[4] = { 4, 3, 4, 3 };

//	Code to support modem/null-modem play

// Program determined variables for serial play

typedef struct com_sync_pack {
	char type;
	byte proto_version;
	long sync_time;
	byte level_num;
	char difficulty;
	char game_mode;
	char callsign[CALLSIGN_LEN+1];
	short kills[2];
	ushort seg_checksum;
	byte sync_id;
	char mission_name[9];
	short killed;
	byte game_flags;
   
   short DoMegas:1;
   short DoSmarts:1;
   short DoFusions:1;
   short DoHelix:1;
	short DoPhoenix:1;
	short DoAfterburner:1;
	short DoInvulnerability:1;
	short DoCloak:1;
	short DoGauss:1;
	short DoVulcan:1;
	short DoPlasma:1;
	short DoOmega:1;
	short DoSuperLaser:1;
	short DoProximity:1;
	short DoSpread:1;
	short DoSmartMine:1;
	short DoFlash:1;
	short DoGuided:1;
	short DoEarthShaker:1;
	short DoMercury:1;
	short Allow_marker_view:1;
	short RefusePlayers:1;
	short AlwaysLighting:1; 
	short DoAmmoRack:1;
	short DoConverter:1;
	short DoHeadlight:1;
	
	char	dummy[3]; // Extra space for checksum & sequence number
} com_sync_pack;

PORT *com_port;
int serial_active;
int com_baud_rate = 0;
//--unused-- int sync_time = 0;
int com_open = 0;
int got_sync = 0;
int other_got_sync = 0;
int carrier_on = 0;
long com_type = -1; /* What type of UART is available */
static long synccnt;
static ubyte rx_seqnum = 0xff;
static ubyte tx_seqnum = 0;
int OtherPlayer; // Player num for modem opponent
int com_process_mode = COM_PROCESS_NORMAL;
int master = -1; // Am I the master or is the other guy the master?
com_sync_pack my_sync, other_sync;
int start_level_num;

int com_custom_port = -1;
int com_custom_irq = -1;
int com_custom_base = -1;

int other_menu_choice = 0;

int chars_sent = 0;

// Com buffers

static char syncbuffer[MAX_MULTI_MESSAGE_LEN+4];
static char sendbuf[MAX_MULTI_MESSAGE_LEN+4]; // +4 because of +1 for null and +3 for checksum/sequence

// Serial setup variables

int com_port_num = -1;
int com_speed = -1;
char modem_init_string[INIT_STRING_LEN+1];
char phone_num[NUM_PHONE_NUM+1][LEN_PHONE_NUM+1];
char phone_name[NUM_PHONE_NUM][LEN_PHONE_NAME+1];

#define MAX_MODEMS 		200
#define MODEM_NAME_LEN	22

char ModemNames[MAX_MODEMS][MODEM_NAME_LEN+1],ModemStrings[MAX_MODEMS][INIT_STRING_LEN+1];  // for init strings
char ModemInitString[INIT_STRING_LEN+1];


fix  SerialLastMessage = 0;

/* Function prototypes for functions not exported through modem.h */

int GetModemList(void); 
void com_param_setup(void);
void com_start_game(void);
void modem_dialout(void);
void modem_answer(void);
int com_sync(int id);
void com_sync_poll(int nitem, newmenu_item *menus, int *key, int citem);

#if 0
#define	codex(name_start, name_end)	\
void name_start(void)	\
{	\
	int	a,b,i;	\
	\
	a = 3;	\
	b = a + 4;	\
	\
	for (i=0; i<123; i++)	\
		a += i;	\
	\
	if (a < b)	\
		b += a;	\
	else if (a == b)	\
		a += b;	\
	else	\
		a += a + b;	\
	\
	while (a < b)	\
		a = b;	\
	\
}	\
	\
void name_end(void)	\
{	\
}
#else
#define codex(name_start, name_end)
#endif

codex(code_01s, code_01e)

int detect_UART(unsigned baseaddr, int * loc, int * code )
{
   // this function returns 0 if no UART is installed.
   // 1: 8250, 2: 16450 or 8250 with scratch reg., 3: 16550, 4: 16550A
   int x,olddata,temp;

	*loc = 0; *code = 0;

   // check if a UART is present.  This is code John hacked by looking at the return
	// values from peoples computers.  
   olddata=inp(baseaddr+4);	
   outp(baseaddr+4,0x1f);			// Enable Loopback mode, sets RTS & DTR to 1.
	delay(1);
	_disable();
	temp = inp(baseaddr+6);			// Read the state of RTS and DTR.
	temp = inp(baseaddr+6);			// Do this twice, so that lower 4 bits are clear. OS/2 returns 0xB0 after this,
											// instead of 0xff if no port is there.
	_enable();
   if ((temp&0x3f)!=0x30) {
		*loc = 1; *code = temp;
		return 0;
	}
   outp(baseaddr+4,olddata);		// Restore RTS & DTR
	delay(1);
   // next thing to do is look for the scratch register
   olddata=inp(baseaddr+7);
   outp(baseaddr+7,0x55);
	delay(1);
   if (inp(baseaddr+7)!=0x55) return 1;
   outp(baseaddr+7,0xAA);
	delay(1);
   if (inp(baseaddr+7)!=0xAA) return 1;
   outp(baseaddr+7,olddata); // we don't need to restore it if it's not there
	delay(1);
   // then check if there's a FIFO
   outp(baseaddr+2,1);
	delay(1);
   x=inp(baseaddr+2);
   // some old-fashioned software relies on this!
   outp(baseaddr+2,0x0);
	delay(1);
   if ((x&0x80)==0) return 2;
   if ((x&0x40)==0) return 3;
   return 4;
}
	
codex(code_02s, code_02e)

int 
com_type_detect()
{
//	static long port;
//	short *ptr;
	int loc, code;

	long port_addr[4] = { 0x3f8, 0x2f8, 0x3e8, 0x2e8 };
	long portaddr;

	if (com_type != -1)
		return com_type;

	if ( (com_port_num != 0) && (com_port_num != 1) && (com_port_num != 2) && (com_port_num != 3) )
	{
		Int3();
		return -1; // Error, set com_port_num before calling this!	
	}

	if (com_port_num == com_custom_port)
		portaddr = com_custom_base;
	else
		portaddr = port_addr[com_port_num];

	mprintf((0, "com port %x.\n", portaddr));
	
	switch( detect_UART(portaddr, &loc, &code) )	{											 	
	case 0:  // No UART
		mprintf((0, "No UART detected. (LOC:%d, CODE:0x%x)\n", loc, code));
		return -1;
	case 1:	// 8250
		mprintf((0, "UART type is 8250.\n" ));
		return(16450);
	case 2:	// 16450 or 8250 with scratch reg.
		mprintf((0, "UART is type 16450, or an 8250 with scratch register.\n" ));
		return(16450);
	case 3:	// 16550
		mprintf((0, "UART is type 16550, with FIFO bug.\n" ));
		return(16450);		// 16550's FIFOs don't work. This is not a typo. (detected 16550, returned 16450 )
	case 4:	// 16550A,  which is the only UART the FIFO mode works with.
		mprintf((0, "UART is type 16550A, no FIFO bug.\n" ));
		return(16550);
	}
	return (-1);
}

#if !defined(NDEBUG) && !defined(NMONO)
void
com_dump_string(char *string)
{
	mprintf((0, "%s\n", string));
}
#else
#define com_dump_string()
#endif

codex(code_03s, code_03e)

int
com_enable() 
{
	// Detect and enable the COM port selected by the user

	int rc;

	if (com_open)
		return 0;

	rx_seqnum = 0xff;
	tx_seqnum = 0;

	// com_port_num and com_speed should be set before calling this func
	com_type = com_type_detect();

	if (com_custom_port == com_port_num)
		rc = Change8259Priority(com_custom_irq);
	else if ((com_port_num == COM2) || (com_port_num == COM4))
		rc = Change8259Priority( IRQ3 );
	else
		rc = Change8259Priority( IRQ4 );

	if (rc != ASSUCCESS) 
	{
		nm_messagebox(TXT_ERROR, 1, TXT_OK, TXT_SERIAL_OPEN_ERROR);
		return -1;
	}

// If our detection is wrong, we want them to be able to go ahead and play anyway.
//	if (com_type == -1)
//	{
//		nm_messagebox(TXT_ERROR, 1, TXT_OK, "Error 2\n%s", TXT_SERIAL_OPEN_ERROR);
//		return -1;
//	}

	if (com_port_num == com_custom_port )
	{
		rc = FastSetPortHardware(com_port_num, com_custom_irq, com_custom_base);
		if (rc != ASSUCCESS)
		{
			nm_messagebox(TXT_ERROR, 1, TXT_OK, TXT_SERIAL_OPEN_ERROR);
			return -1;
		}
	}

	if (com_type == 16550)
	{
		FastSet16550TriggerLevel( TRIGGER_04 );
		FastSet16550UseTXFifos( ON );
	}

	FastSavePortParameters(com_port_num);

	com_port = PortOpenGreenleafFast(com_port_num, com_speed, 'N', 8, 1);

#ifndef NDEBUG
	{
		int curr_irq, curr_base;
		FastGetPortHardware(com_port_num, &curr_irq, &curr_base);
		mprintf((0, "Current port settings: base %x, irq %x.\n", curr_base, curr_irq ));
	}
#endif

	if ((com_port == 0) || (com_port->status != 0))
	{
		nm_messagebox(TXT_ERROR, 1, TXT_OK, TXT_SERIAL_OPEN_ERROR);
		return -1;
	}

	#ifndef NDEBUG
	mprintf((0, "Com port opened.  Status = %d\n", com_port->status));
	#endif

	SetDtr(com_port, ON);
	
	if ( FindArg( "-ctsrts" ) || FindArg( "-rtscts" )  )	
		UseRtsCts(com_port, ON); // Now used for null-modem as well, helps IHHD!
	else
		UseRtsCts(com_port, OFF);

	com_open = 1;

 	master = -1;

//	DumpPortStatus(com_port, com_dump_string);

	return 0;
}

void
com_disable()
{
	// close the com port and free associated structures.

	int rc;

	if (!com_open) 
		return;

// SetDtr(com_port, OFF);
//	UseRtsCts(com_port, OFF);

	rc = PortClose(com_port);

	FastRestorePortParameters(com_port_num);

	if (com_custom_port == com_port_num)
	{
		// Custom settings were in effect, roll them back
		rc = FastSetPortHardware(com_port_num, default_irq[com_port_num], default_base[com_port_num]);
	}

	if (rc != ASSUCCESS) {
		#ifndef NDEBUG
		mprintf((1, "PortClose returned %d!\n", rc));
		#endif
	}

	com_port = 0;
	com_open = 0;

	master = -1;

	#ifndef NDEBUG
	mprintf((0, "Com port released.\n"));
	#endif
}

codex(code_04s, code_04e)

void
com_abort(void)
{
	// this is the safest way to get out of some modem/serial negotiation
	// and back to the main menu.  Use this whenever this have gone too far
	// awry to repair.

	com_disable();

	N_players = 0;

	change_playernum_to(0);

	Viewer = ConsoleObject = &Objects[0];
	Game_mode = GM_GAME_OVER; // Force main menu selection
}

void
com_hangup(void)
{
	// Close the serial link

	com_send_choice(SELECTION_CLOSE_LINK);
	com_abort();
}

void
com_carrier_lost(void)
{
	// Carrier lost, inform and abort

	if (multi_in_menu > 0)
	{
		multi_leave_menu = 1;
		return;
	}

	Function_mode = FMODE_MENU;

	multi_quit_game = 1;
	nm_messagebox(NULL, 1, TXT_OK, TXT_CARRIER_LOST);
	com_abort();
}

codex(code_05s, code_05e)

extern void SetAllAllowablesTo (int on);
extern ubyte Cockpit_mode_save; // From object.c
extern void network_set_power (void);

com_reset_game(void)
{
	int i;

	// Reset various parameters before starting a new game
	
	N_players = 2;

	for (i = 0; i < N_players; i++)
	{
		Players[i].connected = 1;
	}

//	master = -1;

	multi_new_game(); // Reset kill list, among other things
	Control_center_destroyed = 0;
	Endlevel_sequence = 0;
	
	// Yes, this really IS as ugly as it gets, kids...

	if (Cockpit_mode == CM_LETTERBOX || Cockpit_mode == CM_REAR_VIEW)
	{
		select_cockpit(Cockpit_mode_save);
	}
}

codex(code_06s, code_06e)

void
com_save_settings(void)
{
	FILE *settings;
	int i;

	if ( (settings = fopen("serial.cfg", "wb")) == NULL)
		goto error;

	if (fwrite(&com_speed, sizeof(int), 1, settings) != 1)
		goto error;
	
	if (fwrite(&com_port_num, sizeof(int), 1, settings) != 1)
		goto error;

	if (fwrite(modem_init_string, 1, INIT_STRING_LEN+1, settings) != INIT_STRING_LEN+1)
		goto error;

	for (i = 0; i < NUM_PHONE_NUM; i++)
	{
		if (fwrite(phone_num[i], 1, LEN_PHONE_NUM+1, settings) != LEN_PHONE_NUM+1)
			goto error;
		if (fwrite(phone_name[i], 1, LEN_PHONE_NAME+1, settings) != LEN_PHONE_NAME+1)
			goto error;
	}

	if (fwrite(&com_custom_port, sizeof(int), 1, settings) != 1)
		goto error;
	if (fwrite(&com_custom_irq, sizeof(int), 1, settings) != 1)
		goto error;
	if (fwrite(&com_custom_base, sizeof(int), 1, settings) != 1)
		goto error;
	// 100 % success!

	fclose(settings);
	return;

error:
	nm_messagebox(NULL, 1, TXT_OK, TXT_ERROR_SERIAL_CFG);

	if (settings) {
		fclose(settings);
		unlink("serial.cfg");
	}
	
	return;
}

codex(code_07s, code_07e)

void
com_load_settings(void)
{
	FILE *settings;
	int i, cfg_size;

	if ((settings = fopen("serial.cfg", "rb")) == NULL)
		goto defaults;

	cfg_size = filelength(fileno(settings));

	// Read the data from the file
	
	if (fread(&com_speed, sizeof(int), 1, settings) != 1)
		goto error;
	if (! ((com_speed == 9600) || (com_speed == 19200) || (com_speed == 38400)) )
		goto error;

	if (fread(&com_port_num, sizeof(int), 1, settings) != 1)
		goto error;
	if ( (com_port_num < COM1) || (com_port_num > COM4) )
		goto error;

	if (fread(modem_init_string, 1, INIT_STRING_LEN+1, settings) != INIT_STRING_LEN+1)
		goto error;
	modem_init_string[INIT_STRING_LEN] = '\0';

	if (cfg_size <= 273 )	{				// Old 15 char LEN_PHONE_NUM's
		mprintf(( 1, "Reading old pre 1.1 phone.cfg\n" ));
		for (i = 0; i < NUM_PHONE_NUM; i++)
		{
			if (fread(phone_num[i], 1, LEN_PHONE_NUM_OLD+1, settings) != LEN_PHONE_NUM_OLD+1)
				goto error;
			phone_num[i][LEN_PHONE_NUM_OLD] = '\0';
			if (fread(phone_name[i], 1, LEN_PHONE_NAME+1, settings) != LEN_PHONE_NAME+1)
				goto error;
			phone_name[i][LEN_PHONE_NAME] = '\0';
		}
	} else {			// Normal Phone nums
		for (i = 0; i < NUM_PHONE_NUM; i++)
		{
			if (fread(phone_num[i], 1, LEN_PHONE_NUM+1, settings) != LEN_PHONE_NUM+1)
				goto error;
			phone_num[i][LEN_PHONE_NUM] = '\0';
			if (fread(phone_name[i], 1, LEN_PHONE_NAME+1, settings) != LEN_PHONE_NAME+1)
				goto error;
			phone_name[i][LEN_PHONE_NAME] = '\0';
		}
	}

	if (fread(&com_custom_port, sizeof(int), 1, settings) != 1) {
		mprintf((0, "Reading old file format for serial.cfg.\n"));
		goto close;
	}

	if ( (com_custom_port < -1) || (com_custom_port > COM4) )
		goto error;

	if (fread(&com_custom_irq, sizeof(int), 1, settings) != 1)
		goto error;
	if ( (com_custom_port < -1) || (com_custom_port > IRQ15) )
		goto error;

	if (fread(&com_custom_base, sizeof(int), 1, settings) != 1)
		goto error;
	if (com_custom_base < -1)
		goto error;

	//Everything was A-Ok!
close:
	fclose(settings);

	return;

error:
	nm_messagebox(NULL, 1, TXT_OK, TXT_ERR_SER_SETTINGS);
		
defaults:
	// Return some defaults
	com_speed = 19200; // UART speed
	strcpy(modem_init_string, "ATZ");
	com_port_num = COM2;
	com_custom_port = -1;
	for (i = 0; i < NUM_PHONE_NUM; i++)
	{
		phone_num[i][0] = '\0';
		strcpy(phone_name[i], TXT_EMPTY);
	}

	if (settings)
		fclose(settings);

	return;
}

void
serial_leave_game(void)
{
	#ifndef NDEBUG
	mprintf((0, "Called serial_leave_game.\n"));
	#endif
//	com_abort();
	serial_sync_abort(0); // Just in case the other guy is in sync mode
	Game_mode |= GM_GAME_OVER;
	Function_mode = FMODE_MENU;
}

codex(code_08s, code_08e)

void
com_send_data(char *ptr, int len, int repeat)
{
	int i;

	// Take the raw packet data specified by ptr and append the sequence
	// number and checksum, and pass it to com_send_ptr

	if (!com_port)
		return;

	if (Game_mode & GM_MODEM)
	{
		i = GetCd(com_port);
		if (i == 0)
			mprintf((0, "CARRIER LOST!\n"));
	}
	
	len += 3; // Checksum data is 3 bytes

	*(ubyte *)(ptr+(len-3)) = (tx_seqnum+1)%256;
	tx_seqnum = (tx_seqnum+1)%256;

	*(ushort *)(ptr+(len-2)) = netmisc_calc_checksum(ptr, len-2);

	com_send_ptr(ptr, len);
	if (repeat>0)
		for (i = 0; i < repeat; i++)
			com_send_ptr(ptr, len);
}

com_send_ptr(char *ptr, int len)
{
	register	int count;
	register char dat;

	for (count = 0, dat=ptr[0]; count < len; dat=ptr[++count])
	{
		WriteChar(com_port, dat);
		if (dat == EOR_MARK)
			WriteChar(com_port, EOR_MARK); // double in-band endmarkers
	}
	WriteChar(com_port, EOR_MARK);  // EOR
	WriteChar(com_port, 0);         // EOR
	chars_sent += len;
}

codex(code_09s, code_09e)

void
com_flush()
{
	// Get rid of all waiting data in the serial buffer

	int i = 0;

	if (!com_open)	
		return;

	mprintf((0, "COM FLUSH:"));

	while (ReadCharTimed(com_port, 100) >= 0)
		i++;
	mprintf((0, "%d characters.\n", i));
}

int
com_getchar()
{
	register int i;
	static int eor_recv = 0;

	// return values:
	//  -1 = Nothing in buffer
 	//  -2 = End of record

	if (!com_open)
		return(-1);

	i = ReadChar(com_port);

//	mprintf((0, "%c", i));

	if (i == ASBUFREMPTY)
		return (-1);

	if ((i == EOR_MARK) || eor_recv)
	{
		if (!eor_recv)
			i = ReadChar(com_port);

		if (i == ASBUFREMPTY)
		{
//			Assert(eor_recv == 0);
			eor_recv = 1;
			return(-1);
		}
		else if (i == EOR_MARK) 
		{
			eor_recv = 0;
			return(EOR_MARK); // Doubled EOR returns the character
		}
		else
		{
#ifndef NDEBUG
			if (i != 0) {
				mprintf((0, "EOR followed by unexpected value %d.\n", i));
			}
#endif
			eor_recv = 0;
			return(-2);							
		}
	}
	return(i);
}

#define SERIAL_IDLE_TIMEOUT F1_0*10

codex(code_10s, code_10e)

void
com_do_frame(void)
{
	static fix last_comm_time = 0;
	static int last_pos_skipped = 0;
	int rval = 0;

	if (Endlevel_sequence || (com_process_mode==COM_PROCESS_ENDLEVEL)) {	// Only recieve during endlevel
		int old_Endlevel_sequence = Endlevel_sequence;
		Endlevel_sequence = 1;
		com_process_input();
		Endlevel_sequence = old_Endlevel_sequence;
		return;
	}

	last_comm_time += FrameTime;

	if ((last_comm_time > MIN_COMM_GAP) || Network_laser_fired)
	{
#ifndef SHAREWARE
		if ((Game_mode & GM_MULTI_ROBOTS) && !last_pos_skipped) {
			rval = multi_send_robot_frame(0);
		}
		if (rval && !Network_laser_fired)
		{
			last_pos_skipped = 1;
			goto skippos;
		}
#endif
		last_pos_skipped = 0;
		multi_send_position(Players[Player_num].objnum);
		multi_send_fire(); // Will return w/o sending if we haven't fired

skippos:
//		mprintf((0, "%d chars sent, %f cps.\n", chars_sent, f2fl(fixdiv((chars_sent*F1_0),last_comm_time)) ));
		chars_sent = 0;
		last_comm_time = 0;
	}
//	else
//		multi_send_robot_frame(1);

	com_process_input();

	if (!Control_center_destroyed && (Function_mode == FMODE_GAME) && (SerialLastMessage+SERIAL_IDLE_TIMEOUT < GameTime))
	{
		SerialLastMessage = 0x7fffffff-SERIAL_IDLE_TIMEOUT; // Give no further warnings until next message arrives!
		nm_messagebox(TXT_WARNING, 1, TXT_OK, TXT_CONNECT_LOST, Players[OtherPlayer].callsign);
	}

	return;
}

int
com_check_message(char *checkbuf, int len)
{
	ushort check;
	int seqnum;
  
   //mprintf ((0,"TYPE=%d!\n",checkbuf[0]));

	if (len < 4)
	{
		mprintf((0, "message type %d too short to be a real message!\n", checkbuf[0]));
		goto error;
	}

	if (checkbuf[0] > MULTI_MAX_TYPE)
	{
		mprintf((0, "message type %d out of range.\n", checkbuf[0]));
		goto error;
	}

	if ( (len-3) != message_length[checkbuf[0]])
	{
		mprintf((0, "id:%d message length %d != %d.\n",checkbuf[0], len-3, message_length[checkbuf[0]]));
		goto error;
	}

	check = netmisc_calc_checksum(checkbuf, len-2);
	if (check != *(ushort *)(checkbuf+(len-2)))
	{
		#ifndef NDEBUG
		mprintf((0, "error in message type %d, length %d, checksum %d != %d\n", checkbuf[0], len, check, *(ushort *)(checkbuf+(len-2))));	
		#endif
		goto error;
	}

	seqnum = checkbuf[(len-3)];

	if (seqnum == rx_seqnum)
	{
		mprintf ((0,"returning due to bad checksum! type=%d seq=%d rx=%d\n",checkbuf[0],seqnum,rx_seqnum));
		return -1;
	}
	
	if (seqnum != (rx_seqnum+1)%256)
	{
		#ifndef NDEBUG
		mprintf((0, "Warning, missed 1 or more messages.\n"));	
		#endif
	}
	rx_seqnum = seqnum;
	//mprintf((0, "message type %d len %d OK!\n", checkbuf[0], len));
	return 0; 

error:
	mprintf((1,"Line status: %d.\n", GetLineStatus(com_port)));
	ClearLineStatus(com_port);
	return -1;
}
	
codex(code_11s, code_11e)

void
com_process_menu(char *buf, int len)
{
	char text[80];

	len = len;

	mprintf((0, "com_process_menu: type %d.\n", buf[0]));

	switch(buf[0])
	{
		case MULTI_MESSAGE:
//#ifndef SHAREWARE
			sprintf(text, "%s %s\n'%s'", Players[OtherPlayer].callsign, TXT_SAYS, buf+2);
//#else
//			sprintf(text, "%s %s\n'%s'", Players[OtherPlayer].callsign, TXT_SAYS, buf+3);
//#endif
			nm_messagebox(NULL, 1, TXT_OK, text);
			break;
		case MULTI_MENU_CHOICE:
			other_menu_choice = buf[1];
			mprintf((0, "Other menu choice = %d.\n", other_menu_choice));
			break;
		case MULTI_BEGIN_SYNC:
			// If we get a sync at the menu, send an abort sync, we're not ready yet!
			serial_sync_abort(0);
			break;
	}
}

void
com_process_input(void)
{
	// Read all complete messages from the serial buffer and process
	// the contents.  Messages are read into global array snycbuffer.

	static int len = 0;
	int entry_com_mode = com_process_mode;
	register	int dat;

	if (!com_port)
		return;

nextmessage:
	if (Game_mode & GM_MODEM)
	{
		if (!GetCd(com_port))
			com_carrier_lost();
	}

	if (!com_port) {
		if (!multi_in_menu) {
			multi_quit_game = 1;
		}
		else {
			multi_leave_menu = 1;
		}
	}

	if (com_process_mode != entry_com_mode)
	{
		mprintf((0, "Exiting com_process_input due to mode switch.\n"));
		return;
	}

	while ( (len <= MAX_MULTI_MESSAGE_LEN) && (dat = com_getchar()) > -1) // Returns -1 when serial pipe empty
	{
		syncbuffer[len++] = dat;
	}

	if ((dat == -2) || (len > MAX_MULTI_MESSAGE_LEN)) // Returns -2 when end of message reached
	{
		// End of message
		SerialLastMessage = GameTime;

		if (!com_check_message(syncbuffer, len))
		{
			switch(com_process_mode)
			{
				case COM_PROCESS_NORMAL:
				case COM_PROCESS_ENDLEVEL:
					multi_process_data(syncbuffer, len); break;
				case COM_PROCESS_MENU:
					if (!Endlevel_sequence) com_process_menu(syncbuffer, len); break;
				case COM_PROCESS_SYNC:
					if (!Endlevel_sequence) com_process_sync(syncbuffer, len); break;
				default:
					Int3(); // Bad com_process_mode switch set!
			}
		}
 	   else mprintf ((0,"Huh?\n"));
   
		len = 0;
		goto nextmessage;
	}
	if (dat == -3) // Returns -3 when carrier lost
	{
		com_abort();
		len = 0;
	}
	return ;
}

int
com_connect()
{

   Network_status=0;
	my_sync.type = MULTI_BEGIN_SYNC;
	my_sync.difficulty = 0;
	memcpy(my_sync.callsign, Players[Player_num].callsign, CALLSIGN_LEN+1);
	my_sync.seg_checksum = 0;
	my_sync.game_mode = Game_mode;
	my_sync.level_num = 0;
								 
	#ifndef NDEBUG
	mprintf((0, "com_connect()\n"));
	#endif

	if(com_sync(-1))
		return(-1); // Failure in sync

	if (master == -1)
	{
		#ifndef NDEBUG
		mprintf((0, "My rand = %d, other rand = %d.\n", my_sync.sync_time, other_sync.sync_time));
		#endif

		// Figure out who is the master
		if (my_sync.sync_time > other_sync.sync_time)
		{
			mprintf((0, "Swtiching player to master.\n"));
			master=1;
			change_playernum_to(0);
		}
		else if (my_sync.sync_time < other_sync.sync_time)
		{
			mprintf((0, "Switching player to slave.\n"));
			master = 0;
			change_playernum_to(1);
		}
		else
			return(-1);  // Didn't sync properly, try again
	}
	
	// Copy the remote sync data into local variables
	
	OtherPlayer = (Player_num+1)%2;
	mprintf((0, "Other player is #%d.\n", OtherPlayer));
	memcpy(Players[OtherPlayer].callsign, other_sync.callsign, CALLSIGN_LEN+1);

	return(0);
}

#define ADD_ITEM(t,value,key)  do { m[num_options].type=NM_TYPE_MENU; menu_choice[num_options]=value; m[num_options].text=t; num_options++; } while (0)

#define MENU_MODEM_CALL				0
#define MENU_MODEM_ANSWER			1
#define MENU_SERIAL_LINK_START 	2
#define MENU_SERIAL_SETUP 			3
#define MENU_MODEM_HANGUP			4
#define MENU_SERIAL_GAME_START	5
#define MENU_SEND_MESSAGE			6

codex(code_12s, code_12e)

void
com_menu_poll(int nitems, newmenu_item *menus, int *key, int citem)
{
	// Watch the serial stream if we are connected and take appropriate actions

	int old_game_mode;

	menus = menus;
	citem = citem;
	nitems = nitems;

	if (! ( (Game_mode & GM_SERIAL) || (Game_mode & GM_MODEM) ) )
		return;

	com_process_mode = COM_PROCESS_MENU;
	old_game_mode = Game_mode;
	other_menu_choice = 0;	

	com_process_input();

	if ((old_game_mode != Game_mode) || other_menu_choice || (com_process_mode != COM_PROCESS_MENU))
		*key = -2;
	if (multi_leave_menu)
		*key = -2;
}

void
com_send_choice(int choice)
{
	sendbuf[0] = (char)MULTI_MENU_CHOICE;
	sendbuf[1] = (char)choice;

	com_send_data(sendbuf, 2, 1);
}

void
com_ready_to_start(void)
{
	newmenu_item m[2];
	int choice;

	m[0].type = m[1].type = NM_TYPE_MENU;
	m[0].text = TXT_YES;
	m[1].text = TXT_NO;

	choice = newmenu_do1(NULL, TXT_READY_DESCENT, 2, m, com_menu_poll, 0 );
	if (choice == 0)
	{
		// Yes
		com_send_choice(SELECTION_YES_START);
		other_menu_choice = SELECTION_STARTGAME;
		com_start_game();
	}		
	else 
	{
		com_send_choice(SELECTION_NO_START);
	}
}

void
com_process_other_menu_choice(void)
{
	if (other_menu_choice == SELECTION_STARTGAME)	
		com_ready_to_start();
	else if (other_menu_choice == SELECTION_CLOSE_LINK) 
	{
		nm_messagebox(NULL, 1, TXT_OK, TXT_CLOSED_LINK);
		com_hangup();
	}
}

extern void nm_draw_background1(char * filename);

#define SUBTITLE_LEN 120

void
com_main_menu(void)
{
	newmenu_item m[10];
	int menu_choice[10];
	int num_options = 0;
	int choice=0;
	int old_game_mode;
	char subtitle[SUBTITLE_LEN];
	//@@int pcx_error;

	if (com_port_num == -1)
		com_load_settings();

	setjmp(LeaveGame);

	//@@gr_set_current_canvas(NULL);
	//@@pcx_error = pcx_read_bitmap(Menu_pcx_name,&grd_curcanv->cv_bitmap,grd_curcanv->cv_bitmap.bm_type,NULL);
	//@@Assert(pcx_error == PCX_ERROR_NONE);

	nm_draw_background1(Menu_pcx_name);

	#ifdef POLY_ACC
    pa_update_clut(gr_palette, 0, 256, 0);
	#endif

	com_process_mode = COM_PROCESS_MENU;

newmenu:
	num_options = 0;

	if (! ((Game_mode & GM_SERIAL) || (Game_mode & GM_MODEM)) )
	{
		// We haven't established any type of link
		ADD_ITEM(TXT_DIAL_MODEM, MENU_MODEM_CALL, KEY_D);
		ADD_ITEM(TXT_ANSWER_MODEM, MENU_MODEM_ANSWER, KEY_A);
		ADD_ITEM(TXT_NULL_MODEM, MENU_SERIAL_LINK_START, KEY_E);
		ADD_ITEM(TXT_COM_SETTINGS, MENU_SERIAL_SETUP, KEY_C);
	}
	else
	{
		ADD_ITEM(TXT_START_GAME, MENU_SERIAL_GAME_START, KEY_S);
		ADD_ITEM(TXT_SEND_MESSAGEP, MENU_SEND_MESSAGE, KEY_S);
	}
	if (Game_mode & GM_MODEM)
		ADD_ITEM(TXT_HANGUP_MODEM, MENU_MODEM_HANGUP, KEY_H);
	
	if (Game_mode & GM_SERIAL)
		ADD_ITEM(TXT_CLOSE_LINK, MENU_MODEM_HANGUP, KEY_C);

	sprintf(subtitle, "Serial/Modem Game\n\n");

	if (Game_mode & GM_SERIAL)
		sprintf(subtitle+strlen(subtitle), "%s %s\n%s", TXT_SERIAL, TXT_LINK_ACTIVE, Players[OtherPlayer].callsign);
	else if (Game_mode & GM_MODEM)
		sprintf(subtitle+strlen(subtitle), "%d %s %s %s\n%s", com_baud_rate, TXT_BAUD, TXT_MODEM, TXT_LINK_ACTIVE, Players[OtherPlayer].callsign);	
	else
		sprintf(subtitle+strlen(subtitle), TXT_NOT_CONNECTED);

	multi_leave_menu = 0;

	Assert(strlen(subtitle) < SUBTITLE_LEN);

	choice = newmenu_do1(NULL, subtitle, num_options, m, com_menu_poll, 0);

	mprintf((0, "main menu choice was %d.\n", choice));

	if (choice == -1)
	{
		if (!((Game_mode & GM_SERIAL) || (Game_mode & GM_MODEM)))
		{
			com_disable();
			return;
		}
		m[0].text = TXT_YES; m[1].text = TXT_NO;
		m[0].type = m[1].type = NM_TYPE_MENU;

		choice = newmenu_do1(NULL, TXT_EXIT_WILL_CLOSE, 2, m, com_menu_poll, 0);
		if (choice == 0)
		{
			com_send_choice(SELECTION_CLOSE_LINK);
			com_hangup();
			return;
		}
		if ((choice == -2) && (other_menu_choice))
			com_process_other_menu_choice();

		goto newmenu;
	}

	if (choice == -2)
	{
		// Menu poll loop caused a re-draw
		if (other_menu_choice == SELECTION_STARTGAME)	
			com_ready_to_start();
		else if (other_menu_choice == SELECTION_CLOSE_LINK) 
		{
			nm_messagebox(NULL, 1, TXT_OK, TXT_CLOSED_LINK);
			com_hangup();
		}
			
		if (Function_mode == FMODE_GAME)
			return;	

		if (!com_port)
			Game_mode = GM_GAME_OVER;

		goto newmenu;
	}		

	if (choice > -1) 
	{
		old_game_mode=Game_mode;
		switch (menu_choice[choice])
		{
			case MENU_SERIAL_SETUP:
			 	com_param_setup();
				goto newmenu;
				break;
			case MENU_SERIAL_GAME_START:
				com_start_game();
				if (Function_mode != FMODE_GAME) 
					goto newmenu;
				break;
			case MENU_MODEM_CALL:
				modem_dialout();
				goto newmenu;
				break;
			case MENU_MODEM_ANSWER:
				modem_answer();
				goto newmenu;
				break;
			case MENU_SEND_MESSAGE:
				multi_send_message_dialog();
				if (Network_message_reciever != -1)
					multi_send_message();
				multi_sending_message = 0;
				goto newmenu;
				break;
			case MENU_SERIAL_LINK_START:
				serial_link_start();
				goto newmenu;
				break;
			case MENU_MODEM_HANGUP:
				com_hangup();
				goto newmenu;
				break;
			default: 
				Int3();
				return;
		}
	}
}

codex(code_13s, code_13e)

void com_custom_param_setup(void)
{
	// User menu for setting up custom IRQ/Base settings for a COM port

	newmenu_item mm[6];
	int loc;

	char base[10]; 
	char irq[3];
	char title[60];
	int new_irq, new_base;
	int menu_save, menu_reset;
	int mmn;

	sprintf(title, "%s%d", TXT_COM_CUSTOM_SETTINGS, com_port_num+1);
	
	if (com_port_num != com_custom_port) 
	{
		new_irq = default_irq[com_port_num];
		new_base = default_base[com_port_num];
	}
	else
	{
		new_irq = com_custom_irq;
		new_base = com_custom_base;
	}

newmenu:
	sprintf(base, "%x", new_base);
	sprintf(irq, "%d", new_irq);

	loc = 0;
	mm[loc].type = NM_TYPE_TEXT; mm[loc].text = TXT_COM_BASE; loc++;
	mm[loc].type = NM_TYPE_INPUT; mm[loc].text = base; mm[loc].text_len = 9; loc++;
	mm[loc].type = NM_TYPE_TEXT; mm[loc].text = TXT_COM_IRQ; loc++;
	mm[loc].type = NM_TYPE_INPUT; mm[loc].text = irq, mm[loc].text_len = 2; loc++;
	menu_reset = loc;
	mm[loc].type = NM_TYPE_MENU; mm[loc].text = TXT_RESET_DEFAULTS; loc++;
	menu_save = loc;
	mm[loc].type = NM_TYPE_MENU; mm[loc].text = TXT_ACCEPT; loc++;	

	mmn = newmenu_do1(NULL, title, loc, mm, NULL, menu_save);

	if (mmn == -1)
		return; // All changes lost

	new_irq = strtol(irq, NULL, 0);
	new_base = strtol(base, NULL, 16);

	if (mmn == menu_reset) 
	{
		new_irq = default_irq[com_port_num];
		new_base = default_base[com_port_num];
	}
	if (mmn == menu_save) 
	{
		if ((new_irq == default_irq[com_port_num]) && (new_base == default_base[com_port_num])) {
			com_custom_port = -1;
			mprintf((0, "Custom com settings not changed.\n"));
			return;
		}
		if ((new_irq < IRQ2) || (new_irq > IRQ7)) {
		   new_irq = default_irq[com_port_num];
			nm_messagebox(NULL, 1, TXT_OK, TXT_VALID_IRQS);
			goto newmenu;
		}

		com_custom_irq = new_irq;
		com_custom_base = new_base;
		com_custom_port = com_port_num;
		return;
	}
	goto newmenu;
}

void com_param_setup_poll(int nitems, newmenu_item *menus, int *key, int citem)
{
	nitems = nitems;
	key = key;
	citem = citem;
	
	if ((com_custom_port == -1) && menus[4].value)
	{
		menus[4].value = 0; menus[4].redraw = 1;
		return;
	}

	if (com_custom_port == -1)
		return;

	if (menus[com_custom_port].value && !menus[4].value) 
	{
		menus[4].value = 1; menus[4].redraw = 1;
	}
	else if (menus[4].value && !menus[com_custom_port].value)
	{
		menus[4].value = 0; menus[4].redraw = 1;
	}
	
}

void com_param_setup(void)
{
	int mmn;
	int was_enabled = 0;
	newmenu_item mm[20];
	int changed = 0;
	int menu_baud, menu_custom, menu_save,menu_choose_string;
	int loc;

	strcpy (ModemInitString, modem_init_string);

	com_type = -1;

	if (com_open)
	{
		was_enabled = 1;
		com_disable();
	}

setupmenu:	
	loc = 0;
	mm[loc].type=NM_TYPE_RADIO; mm[loc].value=(com_port_num == COM1); mm[loc].text="COM1"; mm[loc].group=0; loc++;
	mm[loc].type=NM_TYPE_RADIO; mm[loc].value=(com_port_num == COM2); mm[loc].text="COM2"; mm[loc].group=0; loc++;
	mm[loc].type=NM_TYPE_RADIO; mm[loc].value=(com_port_num == COM3); mm[loc].text="COM3"; mm[loc].group=0; loc++;
	mm[loc].type=NM_TYPE_RADIO; mm[loc].value=(com_port_num == COM4); mm[loc].text="COM4"; mm[loc].group=0; loc++;
	menu_custom = loc;
	mm[loc].type=NM_TYPE_CHECK; mm[loc].value=(com_port_num == com_custom_port); mm[loc].text=TXT_COM_CUSTOM_SETTINGS; loc++;
	mm[loc].type=NM_TYPE_TEXT; mm[loc].text = TXT_BAUD_RATE; loc++;
	menu_baud = loc;
	mm[loc].type=NM_TYPE_RADIO; mm[loc].value=(com_speed == 9600); mm[loc].text="9600"; mm[loc].group=1; loc++;
	mm[loc].type=NM_TYPE_RADIO; mm[loc].value=(com_speed == 19200); mm[loc].text="19200"; mm[loc].group=1; loc++;
	mm[loc].type=NM_TYPE_RADIO; mm[loc].value=(com_speed == 38400); mm[loc].text="38400"; mm[loc].group=1; loc++;

   menu_choose_string=loc;
	mm[loc].type=NM_TYPE_MENU; mm[loc].text = "Choose modem init string"; loc++;
  
	mm[loc].type=NM_TYPE_TEXT; mm[loc].text = TXT_MODEM_INIT_STRING; loc++;
	mm[loc].type=NM_TYPE_INPUT; mm[loc].text_len = INIT_STRING_LEN; mm[loc].text = ModemInitString; loc++;
	menu_save = loc;
	mm[loc].type=NM_TYPE_MENU; mm[loc].text = TXT_ACCEPT_SAVE; loc++;

	mmn = newmenu_do1(NULL, TXT_SERIAL_SETTINGS, loc, mm, com_param_setup_poll, menu_save);

	if (mmn > -1 ) {
		changed = 1;

		if (mm[0].value)
			com_port_num = COM1;
		else if (mm[1].value)
			com_port_num = COM2;
		else if (mm[2].value) 
			com_port_num = COM3;
		else 
			com_port_num = COM4;
		
		if (mmn == menu_custom)
		{
			com_custom_param_setup();
		}
		if (mmn==menu_choose_string)
		 {
			com_choose_string();
			goto setupmenu;  
		 }

		com_type = -1;
		com_type = com_type_detect();

		if (com_type == -1)
		{
			nm_messagebox(NULL, 1, TXT_OK, "%s\n%s", TXT_WARNING, TXT_NO_UART);
		}

		if ((mm[menu_baud].value) || (mmn == menu_baud)) 
			com_speed = 9600;
		else if ((mm[menu_baud+1].value) || (mmn == menu_baud+1))
			com_speed = 19200;  
		else
		{
			if (com_type == 16550)
				com_speed = 38400;
			else
			{
				nm_messagebox(NULL, 1, TXT_OK, TXT_WARNING_16550);
				com_speed = 19200;
			}
		}
				
		//mprintf((0, "%s\n", ModemInitString));

		if ((strnicmp("AT", ModemInitString, 2)) && (strlen(ModemInitString) < (INIT_STRING_LEN-2)))
			sprintf(modem_init_string, "AT%s", ModemInitString);
		else
			strcpy(modem_init_string, ModemInitString);

		if (mmn != menu_save)
			goto setupmenu;
	}
	
	if (was_enabled)
		com_enable();

	if (changed)
		com_save_settings();

}
	
codex(code_14s, code_14e)

// Handshaking to start a serial game, 2 players only

int com_start_game_menu(void)
{
	newmenu_item m[17];
	char level[5];
	int choice = 0;
	int opt, diff_opt, mode_opt, options_opt,allow_opt,extraopts_opt;
	char level_text[32];

#ifndef SHAREWARE
	int new_mission_num, anarchy_only = 0;

	new_mission_num = multi_choose_mission(&anarchy_only);

	if (new_mission_num < 0)
		return 0;

	strcpy(my_sync.mission_name, Mission_list[new_mission_num].filename);
#endif

	sprintf(level, "1");

	Game_mode &= ~GM_MULTI_COOP;
	Game_mode &= ~GM_MULTI_ROBOTS;
   Game_mode &= ~GM_CAPTURE;
	Netgame.game_flags = 0;
	SetAllAllowablesTo(1);

	sprintf(level_text, "%s (1-%d)", TXT_LEVEL_, Last_level);
//	if (Last_secret_level < -1)
//		sprintf(level_text+strlen(level_text)-1, ", S1-S%d)", -Last_secret_level);
//	else if (Last_secret_level == -1)
//		sprintf(level_text+strlen(level_text)-1, ", S1)");

	Assert(strlen(level_text) < 32);

	// Put up menu for user choices controlling gameplay

newmenu:
	opt = 0;
	m[opt].type = NM_TYPE_TEXT; m[opt].text = level_text; opt++;
	m[opt].type = NM_TYPE_INPUT; m[opt].text_len = 4; m[opt].text = level; opt++;
	m[opt].type = NM_TYPE_TEXT; m[opt].text = TXT_MODE;
	mode_opt = opt; 
	m[opt].type = NM_TYPE_RADIO; m[opt].text = TXT_ANARCHY; m[opt].value=!(Game_mode & GM_MULTI_ROBOTS); m[opt].group = 0; opt++;
	m[opt].type = NM_TYPE_RADIO; m[opt].text = TXT_ANARCHY_W_ROBOTS; m[opt].value=(!(Game_mode & GM_MULTI_COOP) && (Game_mode & GM_MULTI_ROBOTS)); m[opt].group = 0; opt++;
	m[opt].type = NM_TYPE_RADIO; m[opt].text = TXT_COOPERATIVE; m[opt].value=(Game_mode & GM_MULTI_COOP);m[opt].group = 0; opt++;
	m[opt].type = NM_TYPE_RADIO; m[opt].text = "Capture the Flag"; m[opt].value=(Game_mode & GM_CAPTURE); m[opt].group = 0; opt++;
	diff_opt = opt;
	m[opt].type = NM_TYPE_SLIDER; m[opt].text = TXT_DIFFICULTY; m[opt].value = Player_default_difficulty; m[opt].min_value = 0; m[opt].max_value = (NDL-1); opt++;
	m[opt].type = NM_TYPE_TEXT; m[opt].text = " "; opt ++;

   extraopts_opt=opt;

	m[opt].type = NM_TYPE_CHECK; m[opt].text = "Marker camera views"; m[opt].value=Netgame.Allow_marker_view; opt++;
   m[opt].type = NM_TYPE_CHECK; m[opt].text = "Indestructable lights"; m[opt].value=Netgame.AlwaysLighting; opt++;
	   
   allow_opt=opt;
	m[opt].type = NM_TYPE_MENU; m[opt].text = "Choose objects allowed"; opt++;
	

#ifndef SHAREWARE
	options_opt = opt;
//	m[opt].type = NM_TYPE_CHECK; m[opt].text = TXT_SHOW_IDS; m[opt].value=0; opt++;
	m[opt].type = NM_TYPE_CHECK; m[opt].text = TXT_SHOW_ON_MAP; m[opt].value=0; opt++;
#endif

	Assert(opt <= 12);

	GetChoice:
	
	choice = newmenu_do1(NULL, TXT_SERIAL_GAME_SETUP, opt, m, NULL, 1);

	
	if (choice > -1) 
	{
	   if (choice==allow_opt)
			{
	   		network_set_power();
				goto GetChoice;
			}
		Netgame.Allow_marker_view=m[extraopts_opt].value;
		Netgame.AlwaysLighting=m[extraopts_opt+1].value;
		
		if (m[mode_opt].value)
			{
			mprintf ((0,"Undoing capture in modem.c\n"));
			Game_mode &= ~(GM_MULTI_COOP | GM_MULTI_ROBOTS | GM_CAPTURE);
			}
#ifdef SHAREWARE	  
		else {
			nm_messagebox(NULL, 1, TXT_OK, TXT_ONLY_ANARCHY);
			goto newmenu;
		}
#else
		else if (anarchy_only) {
			if (m[mode_opt+3].value)
			{
				mprintf ((0,"Setting capture 1\n"));
   			Game_mode |= (GM_CAPTURE);
			}
			else 
			 {
				nm_messagebox(NULL, 1, TXT_OK, TXT_ANARCHY_ONLY_MISSION);
				goto newmenu;
			 }
		}
		else if (m[mode_opt+1].value)
		{
			Game_mode &= ~GM_MULTI_COOP;
			Game_mode |= GM_MULTI_ROBOTS;
		}
		else if (m[mode_opt+2].value)
			Game_mode |= (GM_MULTI_COOP | GM_MULTI_ROBOTS);
		else if (m[mode_opt+3].value)
			{
				mprintf ((0,"Setting capture 2\n"));
   			Game_mode |= (GM_CAPTURE);
			}

//		if (m[options_opt].value)
//			Netgame.game_flags |= NETGAME_FLAG_SHOW_ID;
		if (m[options_opt].value)
			Netgame.game_flags |= NETGAME_FLAG_SHOW_MAP;
#endif
			start_level_num = atoi(level);

		if ((start_level_num < 1) || (start_level_num > Last_level))
		{
			nm_messagebox(TXT_ERROR, 1, TXT_OK, TXT_LEVEL_OUT_RANGE);
			sprintf(level, "1");
			goto newmenu;
		}

		Difficulty_level = m[diff_opt].value;

		return(1); // Go for game!
	}
	return 0; // No game
}

int
com_ask_to_start()
{	
	// Ask the other player if its OK to start now

	newmenu_item m[1];
	int choice;

	com_send_choice(SELECTION_STARTGAME);

	m[0].type = NM_TYPE_TEXT; m[0].text = TXT_ESC_ABORT;
menu:
	choice = newmenu_do(NULL, TXT_WAIT_FOR_OK, 1, m, com_menu_poll);

	if (choice == -1)
	{
		com_send_choice(SELECTION_STARTGAME_ABORT);
		return(0);
	}
	if (choice == -2)
	{
		if (other_menu_choice == SELECTION_YES_START)
			return(1);
		else if (other_menu_choice == SELECTION_STARTGAME)
		{
			com_send_choice(SELECTION_YES_START);
			return(1);
		}
		else 
			return(0);
	}
	goto menu;
}
		
codex(code_15s, code_15e)

void
com_start_game()
{
	// Start a serial game after being linked

	mprintf((0, "Entered com_start_game\n"));

	com_reset_game();

	if (! ( (Game_mode & GM_MODEM) || (Game_mode & GM_SERIAL) ) ) 
		return;
	
	Assert (master != -1);

	if (other_menu_choice != SELECTION_STARTGAME)
	{
		if (!com_ask_to_start())
			return;
	}

	if (master == 1) // Master chooses options
	{
		if (com_start_game_menu())
		{
			OtherPlayer = 1;
			change_playernum_to(0);
			my_sync.level_num = start_level_num;
			my_sync.difficulty = Difficulty_level;
			my_sync.game_mode = Game_mode;
			if (Game_mode & GM_CAPTURE)
				my_sync.game_mode |=GM_UNKNOWN;	
			memcpy(my_sync.callsign, Players[Player_num].callsign, CALLSIGN_LEN+1);
//#ifndef SHAREWARE
			my_sync.sync_time = control_invul_time;
			my_sync.game_flags = Netgame.game_flags;
			Netgame.control_invul_time = control_invul_time;
			memcpy ((&my_sync.game_flags)+1,(&Netgame.team_vector)+1,4);
			
//#endif
			com_sync(0);
			if (Game_mode & GM_CAPTURE)
				my_sync.game_mode &=GM_UNKNOWN;
		}
	}
	else // Slave
	{
		OtherPlayer = 0;
		change_playernum_to(1);
		memcpy(my_sync.callsign, Players[Player_num].callsign, CALLSIGN_LEN+1);
	
		my_sync.level_num = 1;
		
		com_sync(0);
		if (com_process_mode == COM_PROCESS_NORMAL)
		{
			Difficulty_level = other_sync.difficulty;
			start_level_num = other_sync.level_num;
			Game_mode = other_sync.game_mode;
			if (Game_mode & GM_UNKNOWN) // Super HACK! Out of bit fields, doubling up
			{
				Game_mode &=~GM_UNKNOWN;
				Game_mode |=GM_CAPTURE;
			}
				
			memcpy ((&Netgame.team_vector)+1,(&other_sync.game_flags)+1,4);
			
#ifndef SHAREWARE
			Netgame.game_flags = other_sync.game_flags;
			Netgame.control_invul_time = other_sync.sync_time;
			if (!load_mission_by_name(other_sync.mission_name))
			{
				mprintf((0, "Mission not found: %s!\n", other_sync.mission_name));
				nm_messagebox(NULL, 1, TXT_OK, TXT_MISSION_NOT_FOUND);
				my_sync.sync_id = start_level_num;
				serial_sync_abort(0);
				return;
			}
#endif
		}
	}
	if (com_process_mode != COM_PROCESS_NORMAL)
		return;

	memcpy(Players[OtherPlayer].callsign, other_sync.callsign, CALLSIGN_LEN+1);
	Function_mode = FMODE_GAME;
	Game_mode &= ~GM_GAME_OVER;
	Show_kill_list = 1;
	init_player_stats_game();
	init_player_stats_level(0);
//	Assert(start_level_num > 0);
  	Assert((start_level_num >= Last_secret_level) && (start_level_num <= Last_level));
  	StartNewLevel(start_level_num, 0);
}

//
// Modem control functions, dialing, answering, etc.
//

void modem_edit_phonebook(newmenu_item *m)
{
	int choice, choice2;
	newmenu_item menu[5];
	char text[2][25];
	int default_choice = 0;

	m[NUM_PHONE_NUM].text = TXT_SAVE;

	menu[0].text = TXT_NAME; menu[0].type = NM_TYPE_TEXT;
	menu[1].type = NM_TYPE_INPUT; menu[1].text = text[0]; menu[1].text_len = LEN_PHONE_NAME;
	menu[2].text = TXT_PHONE_NUM; menu[2].type = NM_TYPE_TEXT;
	menu[3].type = NM_TYPE_INPUT; menu[3].text = text[1]; menu[3].text_len = LEN_PHONE_NUM;
	menu[4].text = TXT_ACCEPT; menu[4].type = NM_TYPE_MENU;

menu:
	choice = newmenu_do1(NULL, TXT_SEL_NUMBER_EDIT, NUM_PHONE_NUM+1, m, NULL, default_choice);
	if (choice == -1)
	{
		com_load_settings();
		return;
	}
	if (choice == NUM_PHONE_NUM)
	{
		// Finished
		com_save_settings();
		return;
	}
	
	default_choice = 1;
edit:
	// Edit an entry
	strcpy(menu[1].text, phone_name[choice]);
	strcpy(menu[3].text, phone_num[choice]);

	choice2 = newmenu_do1(NULL, TXT_EDIT_PHONE_ENTRY, 5, menu, NULL, default_choice);
	if (choice2 != -1)
	{	
		strcpy(phone_name[choice], menu[1].text);
		strcpy(phone_num[choice], menu[3].text);
		sprintf(m[choice].text, "%d. %s \t", choice+1, phone_name[choice]);
		add_phone_number(m[choice].text, phone_num[choice] );
	}
	if (choice2 != 4)
	{
		default_choice += 2; if (default_choice > 4) default_choice = 4;
		goto edit;
	}

	default_choice = NUM_PHONE_NUM;
	goto menu;
}


codex(code_16s, code_16e)

void add_phone_number( char * src, char * num )
{
	char p;
	int l;
	l = strlen(num);
	if ( l<15)	{
		strcat( src, num );
		return;
	}
	p = num[15];
	num[15] = 0;
	strcat( src, num );
	num[15] = p;
	strcat( src, "..." );
}

int modem_dial_menu(void)
{
	newmenu_item m[NUM_PHONE_NUM+2];
	char menu_text[NUM_PHONE_NUM][80];
	int choice = 0;
	int i;

menu:
	for (i = 0; i < NUM_PHONE_NUM; i++)
	{
		m[i].text = menu_text[i];
		sprintf(m[i].text, "%d. %s \t", i+1, phone_name[i]);
		add_phone_number(m[i].text, phone_num[i] );
		m[i].type = NM_TYPE_MENU;
	}

	strcat(m[i-1].text, "\n");

	m[NUM_PHONE_NUM].type = NM_TYPE_MENU; 
	m[NUM_PHONE_NUM].text = TXT_MANUAL_ENTRY;
	m[NUM_PHONE_NUM+1].text = TXT_EDIT_PHONEBOOK;
	m[NUM_PHONE_NUM+1].type = NM_TYPE_MENU;

	choice = newmenu_do1(NULL, TXT_SEL_NUMBER_DIAL, NUM_PHONE_NUM+2, m, NULL, 0);
	if (choice == -1) 
		return -1; // user abort

	if (choice == NUM_PHONE_NUM+1)
	{
		// Edit phonebook
		modem_edit_phonebook(m);
		goto menu;
	}

	if (choice == NUM_PHONE_NUM)
	{
		// Manual entry
		newmenu_item m2[1];
		m2[0].type = NM_TYPE_INPUT; m2[0].text = phone_num[NUM_PHONE_NUM]; m2[0].text_len = LEN_PHONE_NUM;
		choice = newmenu_do(NULL, TXT_ENTER_NUMBER_DIAL, 1, m2, NULL);
		if (choice == -1)
			goto menu;
		else
			return NUM_PHONE_NUM;
	}

	// A phone number was chosen
	return(choice);
}

void
com_wait_for_connect(int nitems, newmenu_item *menus, int *key, int citem)
{
	int result;
	char input_buffer[81];
	int baud;
	char error_mess[5][15] = 
		{"NO DIAL TONE",
		 "BUSY",
		 "NO ANSWER",
		 "NO CARRIER",
		 "VOICE"};
	char text[100];
	int i;

	int num_error_messages = 5;

	menus = menus;
	nitems = nitems;
	citem = citem;

	
	if (GetCd(com_port))
	{
		carrier_on = 1;
	}
	else
	{
		if (carrier_on)
		{
			*key = -3;
			nm_messagebox(NULL, 1, TXT_OK, TXT_CARRIER_LOST);
			carrier_on = 0;
			return;
		}
	}

	result = HMInputLine(com_port, 500, input_buffer, 80);
	
	if (result == 0) 
		return;		// Timed out

	mprintf((0, "Modem string: '%s'\n", input_buffer));

	for (i = 0; i < num_error_messages; i++)
	{
		if (!strncmp(input_buffer, error_mess[i], strlen(error_mess[i])))
		{
			sprintf(text, "%s %s", TXT_ERR_MODEM_RETURN, input_buffer);
			nm_messagebox(NULL, 1, TXT_OK, text);
			*key = -3;
			return;
		}
	}

	if (strncmp(input_buffer, TXT_CONNECT, 7))
	{
		mprintf((0, "Non-connect message found.\n"));
		return; // some other string.  Not an error, but not a connect
	}

	sscanf(input_buffer, "CONNECT %d", &baud);

	mprintf((0, "Connect at %d baud.\n", baud));

	if (baud < 9600)
	{
		nm_messagebox(NULL, 1, TXT_OK, TXT_BAUD_GREATER_9600);
		*key = -3;
		return;
	}

	com_baud_rate = baud;
	*key = -2;
	
	return;
}

codex(code_17s, code_17e)

void
com_wait_for_ring(int nitems, newmenu_item *menus, int *key, int citem)
{
	int result;
	char input_buffer[81];

	menus = menus;
	nitems = nitems;
	citem = citem;

	result = HMInputLine(com_port, 500, input_buffer, 80);
	
	if ((result <= 0)	|| strncmp(input_buffer, TXT_RING, 4))
		return;
	
	*key = -2;

	return;
	
}

int modem_verify(void)
{
	// Is there a modem on this com port or not?

	int result;

	HMWaitForOK( 5000, NULL);

	HMSendString(com_port, "AT");
	result = HMSendString(com_port, "AT");

	if (result != 0)
		return (0);
	return(1);
}
	
void modem_dialout(void)
{
	newmenu_item m[5];
	char text[50];
	int choice;
	
	if (!serial_active)
	{
		nm_messagebox(TXT_ERROR, 1, TXT_OK, TXT_NO_SERIAL_OPT);
		return;
	}

	com_enable(); // Open COM port as configured

	if (!com_open)
		return;

//	UseRtsCts(com_port, ON); // use hardware handshaking

main:
	if ((choice = modem_dial_menu()) == -1)
		return; // user aborted

	show_boxed_message(TXT_RESET_MODEM);

	// Verify presence of modem
	if (!modem_verify())
	{
		clear_boxed_message();
		nm_messagebox(NULL, 1, TXT_OK, TXT_NO_MODEM);
		com_abort();
		return;
	}

	if (strlen(phone_num[choice]) == 0)
	{
		clear_boxed_message();
		nm_messagebox(NULL, 1, TXT_OK, TXT_NO_PHONENUM);
		goto main;
	}

	clear_boxed_message();

	sprintf(text, "%s\n%s", TXT_DIALING, phone_num[choice]);
	show_boxed_message(text);

	// Proceed with the call

	HMReset( com_port );

	HMSendString( com_port, modem_init_string );
	
	HMDial( com_port, phone_num[choice] );
	
	carrier_on = 0;

	clear_boxed_message();

	m[0].type=NM_TYPE_TEXT; m[0].text=TXT_ESC_ABORT;

//repeat:
	choice = newmenu_do(NULL, TXT_WAITING_FOR_ANS, 1, m, com_wait_for_connect);
	if (choice != -2) {
		HMSendStringNoWait(com_port, "", -2);
		com_abort();
		return;
	}

	// We are now connected to the other modem

	N_players = 2;

   my_sync.sync_time=32000;
	// -- commented out by mk, 2/21/96 -- master = 1;   // The person who dialed is the master of the connection
	change_playernum_to(0);

	if (!com_connect())
	{
		Game_mode |= GM_MODEM;
		digi_play_sample(SOUND_HUD_MESSAGE, F1_0);
	}
	else {
		HMSendStringNoWait(com_port, "", -2);
		com_abort();
	}
}
									 	
codex(code_18s, code_18e)

void modem_answer(void)
{
	int choice;
	newmenu_item m[3];

	if (!serial_active)
	{
		nm_messagebox(TXT_ERROR, 1, TXT_OK, TXT_NO_SERIAL_OPT);
		return;
	}

	com_enable(); // Open COM port as configured

	if (!com_open)
		return;

//	UseRtsCts(com_port, ON); // use hardware handshaking
	
	show_boxed_message(TXT_RESET_MODEM);

	// Verify presence of modem
	if (!modem_verify())
	{
		clear_boxed_message();
		nm_messagebox(NULL, 1, TXT_OK, TXT_NO_MODEM);
		com_abort();
		return;
	}

	HMReset( com_port );
	
	HMSendString( com_port, modem_init_string );

	HMSendString( com_port, "AT"); // To set the DTE rate for RING notification

	clear_boxed_message();

	m[0].type=NM_TYPE_TEXT; m[0].text=TXT_ESC_ABORT;

repeat:
	choice = newmenu_do(NULL, TXT_WAITING_FOR_CALL, 1, m, com_wait_for_ring);
	if (choice == -1) {
		HMSendStringNoWait(com_port, "", -2);
		com_abort();
		return;
	}
	if (choice != -2)	
		goto repeat;

	// Now answer the phone and wait for carrier

	HMAnswer(com_port);

	carrier_on = 0;

	choice = newmenu_do(NULL, TXT_WAITING_FOR_CARR, 1, m, com_wait_for_connect);
	if (choice != -2) {
		HMSendStringNoWait(com_port, "", -2);
		com_abort();
		return;
	}

	// We are now connected to the other modem
	
	N_players = 2;

   my_sync.sync_time=0;
	// -- commented out by mk, 2/21/96 -- master = 0;
	change_playernum_to(1);

	if (!com_connect()) 
	{
		Game_mode |= GM_MODEM;
		digi_play_sample(SOUND_HUD_MESSAGE, F1_0);
	}
	else {
		HMSendStringNoWait(com_port, "", -2);
		com_abort();
	}
}

void serial_link_start(void)
{
	if (!serial_active)
	{
		nm_messagebox(TXT_ERROR, 1, TXT_OK, TXT_NO_SERIAL_OPT);
		return;
	}

	com_enable(); // Open COM port as configured

	if (!com_open)
		return;

	N_players = 2;

	synccnt = 0;

	srand(clock());
	my_sync.sync_time = rand();
	mprintf((0, "My rand set to %d.\n", my_sync.sync_time));

	if (!com_connect()) 
	{
		Game_mode |= GM_SERIAL;
		digi_play_sample(SOUND_HUD_MESSAGE, F1_0);
	} 
	else
	{
		nm_messagebox(NULL, 1, TXT_OK, "%s\n%s", TXT_ERROR, TXT_FAILED_TO_NEGOT);
	}
}

//
// Syncronization functions
//

void
serial_sync_abort(int val)
{
	// Send "I got Sync but it was no good!" packet

	sendbuf[0] = (char)MULTI_END_SYNC;
	sendbuf[1] = Player_num;
	sendbuf[2] = (char)val; // Indicated failure
//#ifndef SHAREWARE
	sendbuf[3] = my_sync.sync_id;
	com_send_data(sendbuf, 4, 1);
//#else
//	com_send_data(sendbuf, 3, 1);
//#endif
}
	
int
com_level_sync(void)
{
	// Send between-level sync stuff

	mprintf((0, "entered com_level_sync()\n"));

	Function_mode = FMODE_MENU; // Prevent the game loop from running during the menus!

	// At this point, the new level is loaded but the extra objects or players have not 
	// been removed

	my_sync.level_num = Current_level_num;
	my_sync.seg_checksum = netmisc_calc_checksum(Segments, (Highest_segment_index+1) * sizeof(segment));
	my_sync.kills[0] = kill_matrix[Player_num][0];
	my_sync.kills[1] = kill_matrix[Player_num][1];
	my_sync.proto_version = MULTI_PROTO_VERSION;
//#ifndef SHAREWARE
	my_sync.killed = Players[Player_num].net_killed_total;
//#endif
	srand(clock());

	if (Game_mode & GM_MULTI_COOP)
		my_sync.difficulty = Player_num;
	else
		my_sync.difficulty = rand()%MAX_NUM_NET_PLAYERS; // My starting position

	if (com_sync(Current_level_num))
	{
		com_process_mode = COM_PROCESS_MENU;
		nm_messagebox(TXT_ERROR, 1, TXT_OK, TXT_NEGOTIATION_FAIL );
		gr_palette_clear ();
  		longjmp(LeaveGame, 0);
	}

	if (my_sync.level_num != other_sync.level_num)
	{
		// Fatal error
		nm_messagebox(TXT_ERROR, 1, TXT_OK, "%s %d\n%s %d", TXT_FATAL_ERROR_LEVEL, my_sync.level_num, TXT_OTHER_LEVEL, other_sync.level_num);
		gr_palette_clear ();
		longjmp(LeaveGame, 0);
	}

	if (my_sync.seg_checksum != other_sync.seg_checksum)
	{
		// Checksum failure
		mprintf((1, "My check %d, other check %d.\n", my_sync.seg_checksum, other_sync.seg_checksum));
		nm_messagebox(TXT_ERROR, 1, TXT_OK, "%s %d %s %s%s", TXT_YOUR_LEVEL, my_sync.level_num, TXT_LVL_NO_MATCH, other_sync.callsign, TXT_CHECK_VERSION);
		gr_palette_clear ();
//      #ifdef RELEASE
			
			longjmp(LeaveGame, 0);
//		#endif	
	}

	if (my_sync.proto_version != other_sync.proto_version)
	{
		// Version mismatch
		nm_messagebox(TXT_ERROR, 1, TXT_OK, TXT_DESCENT_NO_MATCH);
		gr_palette_clear ();
		longjmp(LeaveGame, 0);
	}

	mprintf((0, "My pos = %d, other pos = %d.\n", my_sync.difficulty, other_sync.difficulty));

	if ((other_sync.difficulty == my_sync.difficulty) && !master)
	{
		// If we chose the same position and I am the slave, choose another
		my_sync.difficulty = (my_sync.difficulty+1) % MAX_NUM_NET_PLAYERS;
	}

	Objects[Players[OtherPlayer].objnum].pos = Player_init[other_sync.difficulty].pos;
	Objects[Players[OtherPlayer].objnum].orient = Player_init[other_sync.difficulty].orient;
	obj_relink(Players[OtherPlayer].objnum,Player_init[other_sync.difficulty].segnum);
	Objects[Players[OtherPlayer].objnum].type = OBJ_PLAYER;

	Objects[Players[Player_num].objnum].pos = Player_init[my_sync.difficulty].pos;
	Objects[Players[Player_num].objnum].orient = Player_init[my_sync.difficulty].orient;
	obj_relink(Players[Player_num].objnum, Player_init[my_sync.difficulty].segnum);
	Objects[Players[Player_num].objnum].type = OBJ_PLAYER;

	SerialLastMessage = GameTime;

	kill_matrix[OtherPlayer][0] = other_sync.kills[0];
	kill_matrix[OtherPlayer][1] = other_sync.kills[1];
	Players[Player_num].net_kills_total = kill_matrix[Player_num][OtherPlayer] - kill_matrix[Player_num][Player_num];
	Players[OtherPlayer].net_kills_total = kill_matrix[OtherPlayer][Player_num] - kill_matrix[OtherPlayer][OtherPlayer];
//	Players[Player_num].net_killed_total = kill_matrix[0][Player_num] + kill_matrix[1][Player_num];
//	Players[OtherPlayer].net_killed_total = kill_matrix[0][OtherPlayer] + kill_matrix[1][OtherPlayer];
	Players[OtherPlayer].net_killed_total = other_sync.killed;
	Players[OtherPlayer].connected = Players[Player_num].connected = 1;

	Assert(N_players == 2);
	Assert(Player_num != OtherPlayer);

	gr_palette_fade_out(gr_palette, 32, 0);

	Function_mode = FMODE_GAME;
	com_process_mode = COM_PROCESS_NORMAL;
	multi_sort_kill_list();
	return(0);
}

codex(code_19s, code_19e)
	
void
com_send_end_sync(void)
{
	// Send "I got Sync" packet

	sendbuf[0] = (char)MULTI_END_SYNC;
	sendbuf[1] = Player_num;
	sendbuf[2] = 1; // Indicates success
//#ifndef SHAREWARE
	sendbuf[3] = my_sync.sync_id;
	com_send_data(sendbuf, 4, 2);
//#else
//	com_send_data(sendbuf, 3, 2);
// #endif
}

void
com_send_begin_sync(void)
{
	mprintf((0, "Sending my sync.\n"));
	com_send_data((char *)&my_sync, sizeof(com_sync_pack)-3, 1);
}

void
com_process_end_sync(byte *buf)
{
	// Process incoming end-sync packet

	if (buf[2] != 1) {
		com_process_mode = COM_PROCESS_MENU;
		return;
	}

//#ifndef SHAREWARE
	if (buf[3] == my_sync.sync_id)
//#endif
		other_got_sync = 1;
}

void
com_process_sync(char *buf, int len)
{
   mprintf ((0,"Processing sync!"));

	len = len;
	switch(buf[0])
	{
		case MULTI_END_SYNC:
		{
			com_process_end_sync(buf);
			break;
		}
		case MULTI_BEGIN_SYNC:
		{
			mprintf((0, "Incoming begin_sync message.\n"));
			if (got_sync)
				break;

			memcpy(&other_sync, buf, sizeof(com_sync_pack)-3);
//#ifndef SHAREWARE
			if (other_sync.sync_id != my_sync.sync_id) 
			{
				mprintf((0, "Other sync invalid id, %d != %d.\n", other_sync.sync_id, my_sync.sync_id));
			}
			else
//#endif
			{			
				mprintf((0, "got other sync size %d.\n", sizeof(com_sync_pack)-3));
				got_sync = 1;
				com_send_end_sync();
			}
			break;
		}
	} // switch

	if (got_sync && other_got_sync)
	{
		// OK to start game
		mprintf((1, "Starting game.\n"));
		got_sync = 0;
		other_got_sync = 0;
		com_process_mode = COM_PROCESS_NORMAL;
	}
}
	
void
com_send_sync(void)
{
	// Send sync information, depending on the situation

	if (!other_got_sync)
	{
		com_send_begin_sync();
	}
	if (got_sync)
	{
		com_send_end_sync();
	}
}

codex(code_20s, code_20e)

void com_sync_poll(int nitems, newmenu_item *menus, int *key, int citem)
{
	static fix t1 = 0;

	menus = menus;
	nitems = nitems;
	citem = citem;

	if (!com_open)
	{
		*key = -3;
		return;
	}

	if (timer_get_approx_seconds() > t1+F1_0)
	{
		com_send_sync();
		t1 = timer_get_approx_seconds();
	}

	Assert(com_process_mode == COM_PROCESS_SYNC);
		
	com_process_input();

	if (com_process_mode == COM_PROCESS_NORMAL)
	{
		*key = -2;
		com_send_sync();
		mprintf((0, "Sync finished.\n"));
		return;
	}
	if (com_process_mode == COM_PROCESS_MENU)
	{
		*key = -3;
		mprintf((0, "Sync denied by other side.\n"));
		return;
	}
	return;
}

int
com_sync(int id)
{
	// How to handle the end of the level and start of the next level
	// returns 0 for success or 1 for failure

	int choice;
	newmenu_item m[3];
	//@@int pcx_error;

	mprintf((0, "Entered com_sync\n"));

	//@@gr_set_current_canvas(NULL);
	//@@pcx_error = pcx_read_bitmap(Menu_pcx_name, &grd_curcanv->cv_bitmap,grd_curcanv->cv_bitmap.bm_type,NULL);
	//@@Assert(pcx_error == PCX_ERROR_NONE);

	com_process_mode = COM_PROCESS_SYNC;
	got_sync = other_got_sync = 0;

	com_flush();
	com_flush();

//#ifndef SHAREWARE
 	my_sync.sync_id = id;
//#endif

	m[0].type=NM_TYPE_TEXT; m[0].text=TXT_ESC_ABORT;
	m[1].type = m[2].type = NM_TYPE_MENU;
	m[1].text = TXT_YES; 
	m[2].text = TXT_NO;

repeat:
	choice = newmenu_do(NULL, TXT_WAIT_OPPONENT, 1, m, com_sync_poll);

	if (choice == -1) 
	{
		choice = newmenu_do1(NULL, TXT_SURE_ABORT_SYNC, 2, m+1, com_sync_poll, 1);
		if (choice == -1)
			choice = 1;
		if (choice == 0)
			choice = -1;
	}

	if ((choice == -1) || (choice == -3)) {
		return(-1);
	}
	else if (choice != -2)
		goto repeat;

	return(0);
}

void
com_endlevel(int *secret)
{
	// What do we do between levels?

	Function_mode = FMODE_MENU;

	gr_palette_fade_out(gr_palette, 32, 0);

	my_sync.level_num = multi_goto_secret;

	if (com_sync(-3))
	{
		com_process_mode = COM_PROCESS_MENU;
		nm_messagebox(TXT_ERROR, 1, TXT_OK, TXT_NEGOTIATION_FAIL);
		longjmp(LeaveGame, 0);
	}

	com_process_mode = COM_PROCESS_ENDLEVEL;

	if ((multi_goto_secret == 1) || (other_sync.level_num == 1))
		*secret = 1;
	else
		*secret = 0;

	multi_goto_secret = 0;

	return;
}

codex(code_21s, code_21e)

#endif

int ReadModemList ()
 {
   int num=0,i=0,namemode=1;
 	FILE *MyFile;
	char c;

	if ((MyFile=fopen ("modem.lst","rb"))==NULL)
    return (0);

	ModemNames[num][0] = ModemStrings[num][0] = 0;

   while (!feof(MyFile))
    {
      c=fgetc (MyFile);

      mprintf ((0,"%c",c));

      if (c=='$')
       {
   	  fclose (MyFile);
   	  return (num);
	  	 }
      else if (c==13)
       ;
		else if (c==10) 
		 {
        ModemStrings[num][i]=0;
	     namemode=1;
		  i=0;
			if (strlen(ModemNames[num]) && strlen(ModemStrings[num]))
		     num++;
			if (num >= MAX_MODEMS)
				break;		//too many!  abort!
			ModemNames[num][0] = ModemStrings[num][0] = 0;
	    }
	   else if (c==';')
       {
	     while (fgetc(MyFile)!=10)
	      ;
        namemode=1;
		  i=0;
		 }
	   else if (c=='=' && namemode)
	    {
        ModemNames[num][i]=0;
	     namemode=0;
		  i=0;
		 }
		else
		 {
	     if (namemode)
		   {
          if (i<MODEM_NAME_LEN)
				ModemNames[num][i++]=c;
		   }
		  else
		   {
          if (i<INIT_STRING_LEN)
		   	ModemStrings[num][i++]=c;
			}
		 }		
	 }  // end while

	ModemStrings[num][i]=0;		//terminate in case no final newline
   fclose (MyFile);
   return (num);
  }	     
    
   
void com_choose_string ()
 { 
  int num,i,choice;
  newmenu_item m[MAX_MODEMS];

  if ((num=ReadModemList())==0)   
   {
    nm_messagebox ("Error",1,TXT_OK,"No valid modem file found!");
    return;
   }
  
  mprintf ((0,"Number of strings=%d\n",num));
  

  for (i=0;i<num;i++)
  {
   m[i].type = NM_TYPE_MENU; m[i].text = &ModemNames[i];
   mprintf ((0,"Text(%d)=%s\n",i,ModemNames[i]));
  }
  choice = newmenu_do(NULL, "Choose a modem", num, m, NULL);

  if (choice<0)
   return;

  strcpy (ModemInitString,ModemStrings[choice]);
 }
 
  
   
   



   

	
