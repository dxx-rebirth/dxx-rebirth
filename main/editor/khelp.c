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
 * $Source: /cvs/cvsroot/d2x/main/editor/khelp.c,v $
 * $Revision: 1.1 $
 * $Author: btb $
 * $Date: 2004-12-19 13:54:27 $
 *
 * Functions for showing help.
 *
 * $Log: not supported by cvs2svn $
 * Revision 1.1.1.1  1999/06/14 22:03:26  donut
 * Import of d1x 1.37 source.
 *
 * Revision 2.0  1995/02/27  11:34:27  john
 * Version 2.0! No anonymous unions, Watcom 10.0, with no need
 * for bitmaps.tbl.
 * 
 * Revision 1.5  1993/12/02  12:39:30  matt
 * Removed extra includes
 * 
 * Revision 1.4  1993/11/05  17:32:57  john
 * added funcs
 * .,
 * 
 * Revision 1.3  1993/11/03  13:42:41  yuan
 * Updated help commands
 * 
 * Revision 1.2  1993/10/22  19:48:07  yuan
 * added ctrl-shift-keypad comment in help.
 * 
 * Revision 1.1  1993/10/13  18:53:16  john
 * Initial revision
 * 
 *
 */

#ifdef RCS
static char rcsid[] = "$Id: khelp.c,v 1.1 2004-12-19 13:54:27 btb Exp $";
#endif

#include "inferno.h"
#include "editor.h"

#include "ui.h"

static char MainHelpText[] = "\nMED General Functions\n\n" \
"SPACEBAR         Full Redraw\n" \
"BACKSPACE        Drop into debugger\n\n" \
"A                Attach a segment\n" \
"D                Delete current segment\n\n" \
"F                Toggle to game\n" \
"G or S           Go to game (Toggle screen)\n\n" \
"ALT-C            Create new mine\n" \
"ALT-S            Save mine\n" \
"ALT-L            Load mine\n"\
"ESC              Exit editor\n"\
"ALT-Q/CTRL-Q/SHIFT-Q also Exit editor\n";

static char SegmentHelpText[] = "MED Segment Functions\n\n" \
"ALT-B            Create Bridge from current to marked segment\n" \
"ALT-E            Exchange current and marked segments\n" \
"ALT-J            Create Joint between current and marked segments\n" \
"ALT-SHIFT-J      Create Joint on current side and adjacent segments\n" \
"ALT-CTRL-J       Create Joint on current segment and adjacent segments\n" \
"ALT-CTRL-SHIFT-J Create Joints on all adjacent segments\n" \
"ALT-M            Mark current segment and side\n" \
"ALT-N            Create default segment for New_segment\n" \
"CTRL-A           Toggle - Draw all segments/Draw connected segments\n" \
"CTRL-C           Clear selected list\n" \
"CTRL-D           Toggle display of coordinate axes\n" \
"CTRL-S           Advance to segment through Curside\n" \
"ALT-T            Assign Texture to current side\n" \
"CTRL-P           Propogate Textures\n" \
"CTRL-SHIFT-P     Propogate Textures on Selected segments\n" \
"CTRL-SHIFT-S     Advance to segment opposite Curside\n" \
"CTRL-F           Select next side\n" \
"CTRL-SHIFT-F     Select previous side\n";

static char KeyPadHelpText[] = "MED KeyPad Functions\n\n" \

"SHIFT-KEYPAD FUNCTIONS (Change direction vector of segment)\n" \
"----------------------\n" \
"(7)                  8 Decrease Pitch   (9)\n" \
" 4 Decrease Heading (5)                  6 Increase Heading\n" \
" 1 Decrease Bank     2 Increase Pitch    3 Increase Bank\n\n" \

"CTRL-KEYPAD FUNCTIONS (Change size/shape of segment)\n" \
"---------------------\n" \
"(7)                  8 Increase Length   9 Increase Height\n" \
" 4 Decrease Width   (5)                  6 Increase Width\n" \
"(1)                  2 Decrease Length   3 Decrease Height\n"\
"\nIn addition, CTRL-SHIFT-KEYPAD Changes size at x5 rate as above\n";


char ViewHelpText[] = "MED View Changing Functions\n\n" \
"ALT-V            Change to orthogonal view (1,2,3)\n" \
"CTRL-V           Toggle view to current segment\n" \
"MINUS (-)        Zoom in\n" \
"EQUAL (=)        Zoom out\n" \
"SHIFT-MINUS      Decrease viewer distance\n" \
"SHIFT-EQUAL      Increase viewer distance\n" \
"\n* Holding the Ctrl key and moving the mouse will change\n" \
"the viewer's orientation in the main window.";

//"CTRL-MINUS       Decreases drawing depth\n" 
//"CTRL-EQUAL       Increases drawing depth\n" 

static char GameHelpText[] = "MED Game Screen Functions\n\n" \
"KEYPAD FUNCTIONS (Moves in game screen)\n" \
"----------------\n" \
"(7)                  8 Move Forward     (9)\n" \
" 4 Decrease Heading  5 Complete Stop     6 Increase Heading\n" \
" 1 Decrease Bank     2 Move Backward     3 Increase Bank\n\n" \
"[                Decreases Pitch\n" \
"]                Increases Pitch\n" \
"C                Set Player from Current segment\n" \
"L                Toggle Lock Step\n" \
"O                Toggle Outline Mode\n" \
"SHIFT-C          Set PLayer from Current segment-1\n" \
"SHIFT-L          Toggle Lighting effect\n" \
"NUMLOCK          Reset orientation\n" \
"PAD DIVIDE (/)   Game Zoom out\n" \
"PAD MULTIPLY (*) Game Zoom In\n";

static char CurveHelpText[] = "MED Curve Generation Functions\n\n" \
"ALT-F10          Generate curve\n" \
"F8               Delete curve\n" \
"F11              'Set' curve\n" \
"F9               Decrease r1 vector\n" \
"SHIFT-F9         Increase r1 vector\n" \
"F10              Decrease r4 vector\n" \
"SHIFT-F10        Increase r4 vector\n";

static char MacrosHelpText[] = "MED Macros Functions\n\n" \
"CTRL-INSERT      Play fast\n" \
"CTRL-DELETE      Play normal\n" \
"CTRL-HOME        Record all\n" \
"CTRL-END         Record keys\n" \
"CTRL-PAGEUP      Save Macro\n" \
"CTRL-PAGEDOWN    Load Macro\n";

int DoHelp()
{
	int help_key = 2;
    int more_key = 2;
    while (help_key > 1)
	{
        help_key = MessageBox( -2, -2, 5, MainHelpText, "Ok", "Segment", "Keypad", "View", "More");
		if (help_key == 2)
			MessageBox( -2, -2, 1, SegmentHelpText, "Ok" );
		if (help_key == 3)
			MessageBox( -2, -2, 1, KeyPadHelpText, "Ok" );
		if (help_key == 4)
			MessageBox( -2, -2, 1, ViewHelpText, "Ok" );
        if (help_key == 5) {
            more_key = MessageBox( -2, -2, 4, MainHelpText, "Back", "Curve", "Macro", "Game");
                if (more_key == 2)
                    MessageBox( -2, -2, 1, CurveHelpText, "Ok" );
                if (help_key == 3)
                    MessageBox( -2, -2, 1, MacrosHelpText, "Ok" );
                if (help_key == 4)
                    MessageBox( -2, -2, 1, GameHelpText, "Ok" );
        }
	}
	return 1;
}
