#ifndef CON_console_H
#define CON_console_H

/*! \mainpage
 
\section intro Introduction
SDL_Console is a console that can be added to any SDL application. It is similar to Quake and other games consoles.
A console is meant to be a very simple way of interacting with a program and executing commands. You can also have 
more than one console at a time. 
 
\section docs Documentation
For a detailed description of all functions see \ref CON_console.h. Remark that functions that have the mark "Internal" 
are only used internally. There's not much use of calling these functions.
 
Have Fun!
 
\author Garett Banuk <mongoose@mongeese.org> (Original Version)
\author Clemens Wacha <reflex-2000@gmx.net> (Version 2.x, Documentation)
\author Boris Lesner <talanthyr@tuxfamily.org> (Package Maintainer)
\author Bradley Bell <btb@icculus.org> (Descent Version)
*/


#include "gr.h"
#include "key.h"

//! Cut the buffer line if it becomes longer than this
#define CON_CHARS_PER_LINE   128
//! Cursor blink frequency in ms
#define CON_BLINK_RATE       500
//! Border in pixels from the most left to the first letter
#define CON_CHAR_BORDER      4
//! Spacing in pixels between lines
#define CON_LINE_SPACE       1
//! Default prompt used at the commandline
#define CON_DEFAULT_PROMPT	"]"
//! Scroll this many lines at a time (when pressing PGUP or PGDOWN)
#define CON_LINE_SCROLL	2
//! Indicator showing that you scrolled up the history
#define CON_SCROLL_INDICATOR "^"
//! Cursor shown if we are in insert mode
#define CON_INS_CURSOR "_"
//! Cursor shown if we are in overwrite mode
#define CON_OVR_CURSOR "|"
//! Defines the default hide key (Hide() the console if pressed)
#define CON_DEFAULT_HIDEKEY	KEY_ESC
//! Defines the opening/closing speed
#define CON_OPENCLOSE_SPEED 25

#ifdef __cplusplus
extern "C" {
#endif

	enum {
	    CON_CLOSED,	//! The console is closed (and not shown)
	    CON_CLOSING,	//! The console is still open and visible but closing
	    CON_OPENING,	//! The console is visible and opening but not yet fully open
	    CON_OPEN	//! The console is open and visible
	};

	/*! This is a struct for each consoles data */
	typedef struct console_information_td {
		int Visible;			//! enum that tells which visible state we are in CON_HIDE, CON_SHOW, CON_RAISE, CON_LOWER
		int RaiseOffset;			//! Offset used when scrolling in the console
		int HideKey;			//! the key that can hide the console
		char **ConsoleLines;		//! List of all the past lines
		char **CommandLines;		//! List of all the past commands
		int TotalConsoleLines;		//! Total number of lines in the console
		int ConsoleScrollBack;		//! How much the user scrolled back in the console
		int TotalCommands;		//! Number of commands in the Back Commands
		int LineBuffer;			//! The number of visible lines in the console (autocalculated)
		int VChars;			//! The number of visible characters in one console line (autocalculated)
		char* Prompt;			//! Prompt displayed in command line
		char Command[CON_CHARS_PER_LINE];	//! current command in command line = lcommand + rcommand
		char RCommand[CON_CHARS_PER_LINE];	//! left hand side of cursor
		char LCommand[CON_CHARS_PER_LINE];	//! right hand side of cursor
		char VCommand[CON_CHARS_PER_LINE];	//! current visible command line
		int CursorPos;			//! Current cursor position in CurrentCommand
		int Offset;			//! CommandOffset (first visible char of command) - if command is too long to fit into console
		int InsMode;			//! Insert or Overwrite characters?
		grs_canvas *ConsoleSurface;	//! Canvas of the console
		grs_screen *OutputScreen;	//! This is the screen to draw the console to
		grs_bitmap *BackgroundImage;	//! Background image for the console
		grs_bitmap *InputBackground;	//! Dirty rectangle to draw over behind the users background
		int DispX, DispY;		//! The top left x and y coords of the console on the display screen
#if 0
		unsigned char ConsoleAlpha;	//! The consoles alpha level
#endif
		int CommandScrollBack;		//! How much the users scrolled back in the command lines
		void(*CmdFunction)(struct console_information_td *console, char* command);	//! The Function that is executed if you press <Return> in the console
		char*(*TabFunction)(char* command);	//! The Function that is executed if you press <Tab> in the console
	}
	ConsoleInformation;

	/*! Takes keys from the keyboard and inputs them to the console if the console isVisible().
		If the event was not handled (i.e. WM events or unknown ctrl- or alt-sequences) 
		the function returns the event for further processing. */
	int CON_Events(int event);
	/*! Makes the console visible */
	void CON_Show(ConsoleInformation *console);
	/*! Hides the console */
	void CON_Hide(ConsoleInformation *console);
	/*! Returns 1 if the console is visible, 0 else */
	int CON_isVisible(ConsoleInformation *console);
	/*! Internal: Updates visible state. Used in CON_DrawConsole() */
	void CON_UpdateOffset(ConsoleInformation* console);
	/*! Draws the console to the screen if it isVisible()*/
	void CON_DrawConsole(ConsoleInformation *console);
	/*! Initializes a new console */
	ConsoleInformation *CON_Init(grs_font *Font, grs_screen *DisplayScreen, int lines, int x, int y, int w, int h);
	/*! Calls CON_Free */
	void CON_Destroy(ConsoleInformation *console);
	/*! Frees all the memory loaded by the console */
	void CON_Free(ConsoleInformation *console);
	/*! printf for the console */
	void CON_Out(ConsoleInformation *console, const char *str, ...);
#if 0
	/*! Sets the alpha channel of an SDL_Surface to the specified value (0 - transparend,
		255 - opaque). Use this function also for OpenGL. */
	void CON_Alpha(ConsoleInformation *console, unsigned char alpha);
	/*! Internal: Sets the alpha channel of an SDL_Surface to the specified value.
		Preconditions: the surface in question is RGBA. 0 <= a <= 255, where 0 is transparent and 255 opaque */
	void CON_AlphaGL(SDL_Surface *s, int alpha);
	/*! Sets a background image for the console */
#endif
	int CON_Background(ConsoleInformation *console, grs_bitmap *image);
	/*! Sets font info for the console */
	void CON_Font(ConsoleInformation *console, grs_font *font, int fg, int bg);
	/*! Changes current position of the console */
	void CON_Position(ConsoleInformation *console, int x, int y);
	/*! Changes the size of the console */
	int CON_Resize(ConsoleInformation *console, int x, int y, int w, int h);
	/*! Beams a console to another screen surface. Needed if you want to make a Video restart in your program. This
		function first changes the OutputScreen Pointer then calls CON_Resize to adjust the new size. */
	int CON_Transfer(ConsoleInformation* console, grs_screen* new_outputscreen, int x, int y, int w, int h);
	/*! Give focus to a console. Make it the "topmost" console. This console will receive events
		sent with CON_Events() */
	void CON_Topmost(ConsoleInformation *console);
	/*! Modify the prompt of the console */
	void CON_SetPrompt(ConsoleInformation *console, char* newprompt);
	/*! Set the key, that invokes a CON_Hide() after press. default is ESCAPE and you can always hide using
		ESCAPE and the HideKey. compared against event->key.keysym.sym !! */
	void CON_SetHideKey(ConsoleInformation *console, int key);
	/*! Internal: executes the command typed in at the console (called if you press ENTER)*/
	void CON_Execute(ConsoleInformation *console, char* command);
	/*! Sets the callback function that is called if a command was typed in. The function could look like this:
		void my_command_handler(ConsoleInformation* console, char* command). @param console: the console the command
		came from. @param command: the command string that was typed in. */
	void CON_SetExecuteFunction(ConsoleInformation *console, void(*CmdFunction)(ConsoleInformation *console2, char* command));
	/*! Sets the callback tabulator completion function. char* my_tabcompletion(char* command). If Tab is
		pressed, the function gets called with the already typed in command. my_tabcompletion then checks if if can
		complete the command or if it should display a list of all matching commands (with CON_Out()). Returns the 
		completed command or NULL if no completion was made. */
	void CON_SetTabCompletion(ConsoleInformation *console, char*(*TabFunction)(char* command));
	/*! Internal: Gets called when TAB was pressed */
	void CON_TabCompletion(ConsoleInformation *console);
	/*! Internal: makes newline (same as printf("\n") or CON_Out(console, "\n") ) */
	void CON_NewLineConsole(ConsoleInformation *console);
	/*! Internal: shift command history (the one you can switch with the up/down keys) */
	void CON_NewLineCommand(ConsoleInformation *console);
	/*! Internal: updates console after resize etc. */
	void CON_UpdateConsole(ConsoleInformation *console);


	/*! Internal: Default Execute callback */
	void Default_CmdFunction(ConsoleInformation *console, char* command);
	/*! Internal: Default TabCompletion callback */
	char* Default_TabFunction(char* command);

	/*! Internal: draws the commandline the user is typing in to the screen. called by update? */
	void DrawCommandLine();

	/*! Internal: Gets called if you press the LEFT key (move cursor left) */
	void Cursor_Left(ConsoleInformation *console);
	/*! Internal: Gets called if you press the RIGHT key (move cursor right) */
	void Cursor_Right(ConsoleInformation *console);
	/*! Internal: Gets called if you press the HOME key (move cursor to the beginning
	of the line */
	void Cursor_Home(ConsoleInformation *console);
	/*! Internal: Gets called if you press the END key (move cursor to the end of the line*/
	void Cursor_End(ConsoleInformation *console);
	/*! Internal: Called if you press DELETE (deletes character under the cursor) */
	void Cursor_Del(ConsoleInformation *console);
	/*! Internal: Called if you press BACKSPACE (deletes character left of cursor) */
	void Cursor_BSpace(ConsoleInformation *console);
	/*! Internal: Called if you type in a character (add the char to the command) */
	void Cursor_Add(ConsoleInformation *console, int event);

	/*! Internal: Called if you press Ctrl-C (deletes the commandline) */
	void Clear_Command(ConsoleInformation *console);
	/*! Internal: Called if you press Ctrl-L (deletes the History) */
	void Clear_History(ConsoleInformation *console);

	/*! Internal: Called if you press UP key (switches through recent typed in commands */
	void Command_Up(ConsoleInformation *console);
	/*! Internal: Called if you press DOWN key (switches through recent typed in commands */
	void Command_Down(ConsoleInformation *console);

#ifdef __cplusplus
};
#endif

#endif


