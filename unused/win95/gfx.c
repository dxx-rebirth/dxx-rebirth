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


#pragma off (unreferenced)
static char rcsid[] = "$Id: gfx.c,v 1.1.1.1 2001-01-19 03:30:15 bradleyb Exp $";
#pragma on (unreferenced)




#define _WIN32
#define WIN95
#define WIN32_LEAN_AND_MEAN

#include <windows.h>
#include <windowsx.h>

#include <stdio.h>

#include "ddraw.h"

#include "pstypes.h"
#include "mem.h"
#include "mono.h"
#include "args.h"
#include "error.h"
#include "winapp.h"

#include "dd.h"
#include "direct3d.h"



//	Globals
//	----------------------------------------------------------------------------

static BOOL						gfx_initialized=0;// GFX flag
static BOOL						d3d_enhanced=0;	//   3D enhanced?
static FILE 					*LogFile=NULL;		// Log File!


//	Function prototypes
//	----------------------------------------------------------------------------

#ifdef NDEBUG
#define WRITELOG(t)
#define LOGINIT(n)
#define LOGCLOSE
#else
#define WRITELOG(t) if (LogFile) { fprintf t; fflush(LogFile); }
#define LOGINIT(n) LogFile = fopen(n, "wt"); 
#define LOGCLOSE if (LogFile) fclose(LogFile);
#endif



/* gfx Philosophy
	
	The GFX system is a higher level abstraction that is independent of the
	DD-DDGR interface.   gfx uses the DD-DDGR interface, but not the other way
	around.   You may use GFX calls and DD-DDGR calls interchangably with older
	functions.  Any newer graphic functionality for Descent 2 will use
	GFX calls
*/


//	Initialization
//	----------------------------------------------------------------------------

/* gfxInit
		
		When called at game initialization, this will initialize the DirectDraw
		system.  Then we initialize other graphic components if called for.
	
*/

BOOL gfxInit(int hw_acc)
{
//	Initialize Direct Draw and DDGR system.

	if (gfx_initialized) return TRUE;

	if (!DDInit(DDGR_FULLSCREEN)) 
		return FALSE;

	grd_curscreen = (grs_screen *)malloc(sizeof(grs_screen));
	W95DisplayMode = SM95_640x480x8;

	gr_init();
	dd_gr_init();
	dd_gr_init_screen();

//	Initialize 3D system if available.
	if (hw_acc) 
	{
		if (!d3d_init()) 
			Error("Unable to initialize 3D Hardware.");
		d3d_enhanced = 1;
	}
	else d3d_enhanced = 0;

	gfx_initialized = 1;

	return TRUE;
}


void gfxClose(void)
{
	if (!gfx_initialized) return;
	gfx_initialized = 0;	
}
