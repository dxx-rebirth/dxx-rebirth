/*  CON_console.c
 *  Written By: Garrett Banuk <mongoose@mongeese.org>
 *  Code Cleanup and heavily extended by: Clemens Wacha <reflex-2000@gmx.net>
 *
 *  This is free, just be sure to give us credit when using it
 *  in any of your programs.
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include "SDL.h"
#include "SDL_image.h"
#include "CON_console.h"
#include "DT_drawtext.h"
#include "internal.h"


/* This contains a pointer to the "topmost" console. The console that
 * is currently taking keyboard input. */
static ConsoleInformation *Topmost;

/*  Takes keys from the keyboard and inputs them to the console
    If the event was not handled (i.e. WM events or unknown ctrl-shift 
    sequences) the function returns the event for further processing. */
SDL_Event* CON_Events(SDL_Event *event) {
	if(Topmost == NULL)
		return event;
	if(!CON_isVisible(Topmost))
		return event;

	if(event->type == SDL_KEYDOWN) {
		if(event->key.keysym.mod & KMOD_CTRL) {
			//CTRL pressed
			switch(event->key.keysym.sym) {
			case SDLK_a:
				Cursor_Home(Topmost);
				break;
			case SDLK_e:
				Cursor_End(Topmost);
				break;
			case SDLK_c:
				Clear_Command(Topmost);
				break;
			case SDLK_l:
				Clear_History(Topmost);
				CON_UpdateConsole(Topmost);
				break;
			default:
				return event;
			}
		} else if(event->key.keysym.mod & KMOD_ALT) {
			//the console does not handle ALT combinations!
			return event;
		} else {
			//first of all, check if the console hide key was pressed
			if(event->key.keysym.sym == Topmost->HideKey) {
				CON_Hide(Topmost);
				return NULL;
			}
			switch (event->key.keysym.sym) {
			case SDLK_HOME:
				if(event->key.keysym.mod & KMOD_SHIFT) {
					Topmost->ConsoleScrollBack = Topmost->LineBuffer-1;
					CON_UpdateConsole(Topmost);
				} else {
					Cursor_Home(Topmost);
				}
				break;
			case SDLK_END:
				if(event->key.keysym.mod & KMOD_SHIFT) {
					Topmost->ConsoleScrollBack = 0;
					CON_UpdateConsole(Topmost);
				} else {
					Cursor_End(Topmost);
				}
				break;
			case SDLK_PAGEUP:
				Topmost->ConsoleScrollBack += CON_LINE_SCROLL;
				if(Topmost->ConsoleScrollBack > Topmost->LineBuffer-1)
					Topmost->ConsoleScrollBack = Topmost->LineBuffer-1;

				CON_UpdateConsole(Topmost);
				break;
			case SDLK_PAGEDOWN:
				Topmost->ConsoleScrollBack -= CON_LINE_SCROLL;
				if(Topmost->ConsoleScrollBack < 0)
					Topmost->ConsoleScrollBack = 0;
				CON_UpdateConsole(Topmost);
				break;
			case SDLK_UP:
				Command_Up(Topmost);
				break;
			case SDLK_DOWN:
				Command_Down(Topmost);
				break;
			case SDLK_LEFT:
				Cursor_Left(Topmost);
				break;
			case SDLK_RIGHT:
				Cursor_Right(Topmost);
				break;
			case SDLK_BACKSPACE:
				Cursor_BSpace(Topmost);
				break;
			case SDLK_DELETE:
				Cursor_Del(Topmost);
				break;
			case SDLK_INSERT:
				Topmost->InsMode = 1-Topmost->InsMode;
				break;
			case SDLK_TAB:
				CON_TabCompletion(Topmost);
				break;
			case SDLK_RETURN:
				if(strlen(Topmost->Command) > 0) {
					CON_NewLineCommand(Topmost);

					// copy the input into the past commands strings
					strcpy(Topmost->CommandLines[0], Topmost->Command);

					// display the command including the prompt
					CON_Out(Topmost, "%s%s", Topmost->Prompt, Topmost->Command);
					CON_UpdateConsole(Topmost);

					CON_Execute(Topmost, Topmost->Command);
					//printf("Command: %s\n", Topmost->Command);

					Clear_Command(Topmost);
					Topmost->CommandScrollBack = -1;
				}
				break;
			case SDLK_ESCAPE:
				//deactivate Console
				CON_Hide(Topmost);
				return NULL;
			default:
				if(Topmost->InsMode)
					Cursor_Add(Topmost, event);
				else {
					Cursor_Add(Topmost, event);
					Cursor_Del(Topmost);
				}
			}
		}
		return NULL;
	}
	return event;
}

/* CON_AlphaGL() -- sets the alpha channel of an SDL_Surface to the
 * specified value.  Preconditions: the surface in question is RGBA.
 * 0 <= a <= 255, where 0 is transparent and 255 is opaque. */
void CON_AlphaGL(SDL_Surface *s, int alpha) {
	Uint8 val;
	int x, y, w, h;
	Uint32 pixel;
	Uint8 r, g, b, a;
	SDL_PixelFormat *format;
	static char errorPrinted = 0;


	/* debugging assertions -- these slow you down, but hey, crashing sucks */
	if(!s) {
		PRINT_ERROR("NULL Surface passed to CON_AlphaGL\n");
		return;
	}

	/* clamp alpha value to 0...255 */
	if(alpha < SDL_ALPHA_TRANSPARENT)
		val = SDL_ALPHA_TRANSPARENT;
	else if(alpha > SDL_ALPHA_OPAQUE)
		val = SDL_ALPHA_OPAQUE;
	else
		val = alpha;

	/* loop over alpha channels of each pixel, setting them appropriately. */
	w = s->w;
	h = s->h;
	format = s->format;
	switch (format->BytesPerPixel) {
	case 2:
		/* 16-bit surfaces don't seem to support alpha channels. */
		if(!errorPrinted) {
			errorPrinted = 1;
			PRINT_ERROR("16-bit SDL surfaces do not support alpha-blending under OpenGL.\n");
		}
		break;
	case 4: {
			/* we can do this very quickly in 32-bit mode.  24-bit is more
			 * difficult.  And since 24-bit mode is reall the same as 32-bit,
			 * so it usually ends up taking this route too.  Win!  Unroll loop
			 * and use pointer arithmetic for extra speed. */
			int numpixels = h * (w << 2);
			Uint8 *pix = (Uint8 *) (s->pixels);
			Uint8 *last = pix + numpixels;
			Uint8 *pixel;
			if((numpixels & 0x7) == 0)
				for(pixel = pix + 3; pixel < last; pixel += 32)
					*pixel = *(pixel + 4) = *(pixel + 8) = *(pixel + 12) = *(pixel + 16) = *(pixel + 20) = *(pixel + 24) = *(pixel + 28) = val;
			else
				for(pixel = pix + 3; pixel < last; pixel += 4)
					*pixel = val;
			break;
		}
	default:
		/* we have no choice but to do this slowly.  <sigh> */
		for(y = 0; y < h; ++y)
			for(x = 0; x < w; ++x) {
				char print = 0;
				/* Lock the surface for direct access to the pixels */
				if(SDL_MUSTLOCK(s) && SDL_LockSurface(s) < 0) {
					PRINT_ERROR("Can't lock surface: ");
					fprintf(stderr, "%s\n", SDL_GetError());
					return;
				}
				pixel = DT_GetPixel(s, x, y);
				if(x == 0 && y == 0)
					print = 1;
				SDL_GetRGBA(pixel, format, &r, &g, &b, &a);
				pixel = SDL_MapRGBA(format, r, g, b, val);
				SDL_GetRGBA(pixel, format, &r, &g, &b, &a);
				DT_PutPixel(s, x, y, pixel);

				/* unlock surface again */
				if(SDL_MUSTLOCK(s))
					SDL_UnlockSurface(s);
			}
		break;
	}
}


/* Updates the console buffer */
void CON_UpdateConsole(ConsoleInformation *console) {
	int loop;
	int loop2;
	int Screenlines;
	SDL_Rect DestRect;
	BitFont *CurrentFont = DT_FontPointer(console->FontNumber);

	if(!console)
		return;

	/* Due to the Blits, the update is not very fast: So only update if it's worth it */
	if(!CON_isVisible(console))
		return;

	Screenlines = console->ConsoleSurface->h / console->FontHeight;


	SDL_FillRect(console->ConsoleSurface, NULL, SDL_MapRGBA(console->ConsoleSurface->format, 0, 0, 0, console->ConsoleAlpha));

	if(console->OutputScreen->flags & SDL_OPENGLBLIT)
		SDL_SetAlpha(console->ConsoleSurface, 0, SDL_ALPHA_OPAQUE);

	/* draw the background image if there is one */
	if(console->BackgroundImage) {
		DestRect.x = console->BackX;
		DestRect.y = console->BackY;
		DestRect.w = console->BackgroundImage->w;
		DestRect.h = console->BackgroundImage->h;
		SDL_BlitSurface(console->BackgroundImage, NULL, console->ConsoleSurface, &DestRect);
	}

	/* Draw the text from the back buffers, calculate in the scrollback from the user
	 * this is a normal SDL software-mode blit, so we need to temporarily set the ColorKey
	 * for the font, and then clear it when we're done.
	 */
	if((console->OutputScreen->flags & SDL_OPENGLBLIT) && (console->OutputScreen->format->BytesPerPixel > 2)) {
		Uint32 *pix = (Uint32 *) (CurrentFont->FontSurface->pixels);
		SDL_SetColorKey(CurrentFont->FontSurface, SDL_SRCCOLORKEY, *pix);
	}


	//now draw text from last but second line to top
	for(loop = 0; loop < Screenlines-1 && loop < console->LineBuffer - console->ConsoleScrollBack; loop++) {
		if(console->ConsoleScrollBack != 0 && loop == 0)
			for(loop2 = 0; loop2 < (console->VChars / 5) + 1; loop2++)
				DT_DrawText(CON_SCROLL_INDICATOR, console->ConsoleSurface, console->FontNumber, CON_CHAR_BORDER + (loop2*5*console->FontWidth), (Screenlines - loop - 2) * console->FontHeight);
		else
			DT_DrawText(console->ConsoleLines[console->ConsoleScrollBack + loop], console->ConsoleSurface, console->FontNumber, CON_CHAR_BORDER, (Screenlines - loop - 2) * console->FontHeight);
	}

	if(console->OutputScreen->flags & SDL_OPENGLBLIT)
		SDL_SetColorKey(CurrentFont->FontSurface, 0, 0);
}

void CON_UpdateOffset(ConsoleInformation* console) {
	if(!console)
		return;

	switch(console->Visible) {
	case CON_CLOSING:
		console->RaiseOffset -= CON_OPENCLOSE_SPEED;
		if(console->RaiseOffset <= 0) {
			console->RaiseOffset = 0;
			console->Visible = CON_CLOSED;
		}
		break;
	case CON_OPENING:
		console->RaiseOffset += CON_OPENCLOSE_SPEED;
		if(console->RaiseOffset >= console->ConsoleSurface->h) {
			console->RaiseOffset = console->ConsoleSurface->h;
			console->Visible = CON_OPEN;
		}
		break;
	case CON_OPEN:
	case CON_CLOSED:
		break;
	}
}

/* Draws the console buffer to the screen if the console is "visible" */
void CON_DrawConsole(ConsoleInformation *console) {
	SDL_Rect DestRect;
	SDL_Rect SrcRect;

	if(!console)
		return;

	/* only draw if console is visible: here this means, that the console is not CON_CLOSED */
	if(console->Visible == CON_CLOSED)
		return;

	/* Update the scrolling offset */
	CON_UpdateOffset(console);

	/* Update the command line since it has a blinking cursor */
	DrawCommandLine();

	/* before drawing, make sure the alpha channel of the console surface is set
	 * properly.  (sigh) I wish we didn't have to do this every frame... */
	if(console->OutputScreen->flags & SDL_OPENGLBLIT)
		CON_AlphaGL(console->ConsoleSurface, console->ConsoleAlpha);

	SrcRect.x = 0;
	SrcRect.y = console->ConsoleSurface->h - console->RaiseOffset;
	SrcRect.w = console->ConsoleSurface->w;
	SrcRect.h = console->RaiseOffset;

	/* Setup the rect the console is being blitted into based on the output screen */
	DestRect.x = console->DispX;
	DestRect.y = console->DispY;
	DestRect.w = console->ConsoleSurface->w;
	DestRect.h = console->ConsoleSurface->h;

	SDL_BlitSurface(console->ConsoleSurface, &SrcRect, console->OutputScreen, &DestRect);

	if(console->OutputScreen->flags & SDL_OPENGLBLIT)
		SDL_UpdateRects(console->OutputScreen, 1, &DestRect);
}


/* Initializes the console */
ConsoleInformation *CON_Init(const char *FontName, SDL_Surface *DisplayScreen, int lines, SDL_Rect rect) {
	int loop;
	SDL_Surface *Temp;
	ConsoleInformation *newinfo;


	/* Create a new console struct and init it. */
	if((newinfo = (ConsoleInformation *) malloc(sizeof(ConsoleInformation))) == NULL) {
		PRINT_ERROR("Could not allocate the space for a new console info struct.\n");
		return NULL;
	}
	newinfo->Visible = CON_CLOSED;
	newinfo->WasUnicode = 0;
	newinfo->RaiseOffset = 0;
	newinfo->ConsoleLines = NULL;
	newinfo->CommandLines = NULL;
	newinfo->TotalConsoleLines = 0;
	newinfo->ConsoleScrollBack = 0;
	newinfo->TotalCommands = 0;
	newinfo->BackgroundImage = NULL;
	newinfo->ConsoleAlpha = SDL_ALPHA_OPAQUE;
	newinfo->Offset = 0;
	newinfo->InsMode = 1;
	newinfo->CursorPos = 0;
	newinfo->CommandScrollBack = 0;
	newinfo->OutputScreen = DisplayScreen;
	newinfo->Prompt = CON_DEFAULT_PROMPT;
	newinfo->HideKey = CON_DEFAULT_HIDEKEY;

	CON_SetExecuteFunction(newinfo, Default_CmdFunction);
	CON_SetTabCompletion(newinfo, Default_TabFunction);

	/* Load the consoles font */
	if(-1 == (newinfo->FontNumber = DT_LoadFont(FontName, TRANS_FONT))) {
		PRINT_ERROR("Could not load the font ");
		fprintf(stderr, "\"%s\" for the console!\n", FontName);
		return NULL;
	}

	newinfo->FontHeight = DT_FontHeight(newinfo->FontNumber);
	newinfo->FontWidth = DT_FontWidth(newinfo->FontNumber);

	/* make sure that the size of the console is valid */
	if(rect.w > newinfo->OutputScreen->w || rect.w < newinfo->FontWidth * 32)
		rect.w = newinfo->OutputScreen->w;
	if(rect.h > newinfo->OutputScreen->h || rect.h < newinfo->FontHeight)
		rect.h = newinfo->OutputScreen->h;
	if(rect.x < 0 || rect.x > newinfo->OutputScreen->w - rect.w)
		newinfo->DispX = 0;
	else
		newinfo->DispX = rect.x;
	if(rect.y < 0 || rect.y > newinfo->OutputScreen->h - rect.h)
		newinfo->DispY = 0;
	else
		newinfo->DispY = rect.y;

	/* load the console surface */
	Temp = SDL_CreateRGBSurface(SDL_SWSURFACE, rect.w, rect.h, newinfo->OutputScreen->format->BitsPerPixel, 0, 0, 0, 0);
	if(Temp == NULL) {
		PRINT_ERROR("Couldn't create the ConsoleSurface\n");
		return NULL;
	}
	newinfo->ConsoleSurface = SDL_DisplayFormat(Temp);
	SDL_FreeSurface(Temp);
	SDL_FillRect(newinfo->ConsoleSurface, NULL, SDL_MapRGBA(newinfo->ConsoleSurface->format, 0, 0, 0, newinfo->ConsoleAlpha));

	/* Load the dirty rectangle for user input */
	Temp = SDL_CreateRGBSurface(SDL_SWSURFACE, rect.w, newinfo->FontHeight, newinfo->OutputScreen->format->BitsPerPixel, 0, 0, 0, SDL_ALPHA_OPAQUE);
	if(Temp == NULL) {
		PRINT_ERROR("Couldn't create the InputBackground\n");
		return NULL;
	}
	newinfo->InputBackground = SDL_DisplayFormat(Temp);
	SDL_FreeSurface(Temp);
	SDL_FillRect(newinfo->InputBackground, NULL, SDL_MapRGBA(newinfo->ConsoleSurface->format, 0, 0, 0, SDL_ALPHA_OPAQUE));

	/* calculate the number of visible characters in the command line */
	newinfo->VChars = (rect.w - CON_CHAR_BORDER) / newinfo->FontWidth;
	if(newinfo->VChars > CON_CHARS_PER_LINE)
		newinfo->VChars = CON_CHARS_PER_LINE;

	/* We would like to have a minumum # of lines to guarentee we don't create a memory error */
	if(rect.h / newinfo->FontHeight > lines)
		newinfo->LineBuffer = rect.h / newinfo->FontHeight;
	else
		newinfo->LineBuffer = lines;


	newinfo->ConsoleLines = (char **)malloc(sizeof(char *) * newinfo->LineBuffer);
	newinfo->CommandLines = (char **)malloc(sizeof(char *) * newinfo->LineBuffer);
	for(loop = 0; loop <= newinfo->LineBuffer - 1; loop++) {
		newinfo->ConsoleLines[loop] = (char *)calloc(CON_CHARS_PER_LINE, sizeof(char));
		newinfo->CommandLines[loop] = (char *)calloc(CON_CHARS_PER_LINE, sizeof(char));
	}
	memset(newinfo->Command, 0, CON_CHARS_PER_LINE);
	memset(newinfo->LCommand, 0, CON_CHARS_PER_LINE);
	memset(newinfo->RCommand, 0, CON_CHARS_PER_LINE);
	memset(newinfo->VCommand, 0, CON_CHARS_PER_LINE);


	CON_Out(newinfo, "Console initialised.");
	CON_NewLineConsole(newinfo);
	//CON_ListCommands(newinfo);

	return newinfo;
}

/* Makes the console visible */
void CON_Show(ConsoleInformation *console) {
	if(console) {
		console->Visible = CON_OPENING;
		CON_UpdateConsole(console);

		console->WasUnicode = SDL_EnableUNICODE(-1);
		SDL_EnableUNICODE(1);
	}
}

/* Hides the console (make it invisible) */
void CON_Hide(ConsoleInformation *console) {
	if(console) {
		console->Visible = CON_CLOSING;
		SDL_EnableUNICODE(console->WasUnicode);
	}
}

/* tells wether the console is visible or not */
int CON_isVisible(ConsoleInformation *console) {
	if(!console)
		return CON_CLOSED;
	return((console->Visible == CON_OPEN) || (console->Visible == CON_OPENING));
}

/* Frees all the memory loaded by the console */
void CON_Destroy(ConsoleInformation *console) {
	DT_DestroyDrawText();
	CON_Free(console);
}

/* Frees all the memory loaded by the console */
void CON_Free(ConsoleInformation *console) {
	int i;

	if(!console)
		return;

	//CON_DestroyCommands();
	for(i = 0; i <= console->LineBuffer - 1; i++) {
		free(console->ConsoleLines[i]);
		free(console->CommandLines[i]);
	}
	free(console->ConsoleLines);
	free(console->CommandLines);

	console->ConsoleLines = NULL;
	console->CommandLines = NULL;
	free(console);
}


/* Increments the console lines */
void CON_NewLineConsole(ConsoleInformation *console) {
	int loop;
	char* temp;

	if(!console)
		return;

	temp = console->ConsoleLines[console->LineBuffer - 1];

	for(loop = console->LineBuffer - 1; loop > 0; loop--)
		console->ConsoleLines[loop] = console->ConsoleLines[loop - 1];

	console->ConsoleLines[0] = temp;

	memset(console->ConsoleLines[0], 0, CON_CHARS_PER_LINE);
	if(console->TotalConsoleLines < console->LineBuffer - 1)
		console->TotalConsoleLines++;

	//Now adjust the ConsoleScrollBack
	//dont scroll if not at bottom
	if(console->ConsoleScrollBack != 0)
		console->ConsoleScrollBack++;
	//boundaries
	if(console->ConsoleScrollBack > console->LineBuffer-1)
		console->ConsoleScrollBack = console->LineBuffer-1;

}


/* Increments the command lines */
void CON_NewLineCommand(ConsoleInformation *console) {
	int loop;
	char *temp;

	if(!console)
		return;

	temp  = console->CommandLines[console->LineBuffer - 1];


	for(loop = console->LineBuffer - 1; loop > 0; loop--)
		console->CommandLines[loop] = console->CommandLines[loop - 1];

	console->CommandLines[0] = temp;

	memset(console->CommandLines[0], 0, CON_CHARS_PER_LINE);
	if(console->TotalCommands < console->LineBuffer - 1)
		console->TotalCommands++;
}

/* Draws the command line the user is typing in to the screen */
/* completely rewritten by C.Wacha */
void DrawCommandLine() {
	SDL_Rect rect;
	int x;
	int commandbuffer;
	BitFont* CurrentFont;
	static Uint32 LastBlinkTime = 0;	/* Last time the consoles cursor blinked */
	static int LastCursorPos = 0;		// Last Cursor Position
	static int Blink = 0;			/* Is the cursor currently blinking */

	if(!Topmost)
		return;

	commandbuffer = Topmost->VChars - strlen(Topmost->Prompt)-1; // -1 to make cursor visible

	CurrentFont = DT_FontPointer(Topmost->FontNumber);

	//Concatenate the left and right side to command
	strcpy(Topmost->Command, Topmost->LCommand);
	strncat(Topmost->Command, Topmost->RCommand, strlen(Topmost->RCommand));

	//calculate display offset from current cursor position
	if(Topmost->Offset < Topmost->CursorPos - commandbuffer)
		Topmost->Offset = Topmost->CursorPos - commandbuffer;
	if(Topmost->Offset > Topmost->CursorPos)
		Topmost->Offset = Topmost->CursorPos;

	//first add prompt to visible part
	strcpy(Topmost->VCommand, Topmost->Prompt);

	//then add the visible part of the command
	strncat(Topmost->VCommand, &Topmost->Command[Topmost->Offset], strlen(&Topmost->Command[Topmost->Offset]));

	//now display the result

	//once again we're drawing text, so in OpenGL context we need to temporarily set up
	//software-mode transparency.
	if(Topmost->OutputScreen->flags & SDL_OPENGLBLIT) {
		Uint32 *pix = (Uint32 *) (CurrentFont->FontSurface->pixels);
		SDL_SetColorKey(CurrentFont->FontSurface, SDL_SRCCOLORKEY, *pix);
	}

	//first of all restore InputBackground
	rect.x = 0;
	rect.y = Topmost->ConsoleSurface->h - Topmost->FontHeight;
	rect.w = Topmost->InputBackground->w;
	rect.h = Topmost->InputBackground->h;
	SDL_BlitSurface(Topmost->InputBackground, NULL, Topmost->ConsoleSurface, &rect);

	//now add the text
	DT_DrawText(Topmost->VCommand, Topmost->ConsoleSurface, Topmost->FontNumber, CON_CHAR_BORDER, Topmost->ConsoleSurface->h - Topmost->FontHeight);

	//at last add the cursor
	//check if the blink period is over
	if(SDL_GetTicks() > LastBlinkTime) {
		LastBlinkTime = SDL_GetTicks() + CON_BLINK_RATE;
		if(Blink)
			Blink = 0;
		else
			Blink = 1;
	}

	//check if cursor has moved - if yes display cursor anyway
	if(Topmost->CursorPos != LastCursorPos) {
		LastCursorPos = Topmost->CursorPos;
		LastBlinkTime = SDL_GetTicks() + CON_BLINK_RATE;
		Blink = 1;
	}

	if(Blink) {
		x = CON_CHAR_BORDER + Topmost->FontWidth * (Topmost->CursorPos - Topmost->Offset + strlen(Topmost->Prompt));
		if(Topmost->InsMode)
			DT_DrawText(CON_INS_CURSOR, Topmost->ConsoleSurface, Topmost->FontNumber, x, Topmost->ConsoleSurface->h - Topmost->FontHeight);
		else
			DT_DrawText(CON_OVR_CURSOR, Topmost->ConsoleSurface, Topmost->FontNumber, x, Topmost->ConsoleSurface->h - Topmost->FontHeight);
	}


	if(Topmost->OutputScreen->flags & SDL_OPENGLBLIT) {
		SDL_SetColorKey(CurrentFont->FontSurface, 0, 0);
	}
}

/* Outputs text to the console (in game), up to CON_CHARS_PER_LINE chars can be entered */
void CON_Out(ConsoleInformation *console, const char *str, ...) {
	va_list marker;
	//keep some space free for stuff like CON_Out(console, "blablabla %s", console->Command);
	char temp[CON_CHARS_PER_LINE + 128];
	char* ptemp;

	if(!console)
		return;

	va_start(marker, str);
	vsnprintf(temp, CON_CHARS_PER_LINE + 127, str, marker);
	va_end(marker);

	ptemp = temp;

	//temp now contains the complete string we want to output
	// the only problem is that temp is maybe longer than the console
	// width so we have to cut it into several pieces

	if(console->ConsoleLines) {
		while(strlen(ptemp) > console->VChars) {
			CON_NewLineConsole(console);
			strncpy(console->ConsoleLines[0], ptemp, console->VChars);
			console->ConsoleLines[0][console->VChars] = '\0';
			ptemp = &ptemp[console->VChars];
		}
		CON_NewLineConsole(console);
		strncpy(console->ConsoleLines[0], ptemp, console->VChars);
		console->ConsoleLines[0][console->VChars] = '\0';
		CON_UpdateConsole(console);
	}

	/* And print to stdout */
	//printf("%s\n", temp);
}


/* Sets the alpha level of the console, 0 turns off alpha blending */
void CON_Alpha(ConsoleInformation *console, unsigned char alpha) {
	if(!console)
		return;

	/* store alpha as state! */
	console->ConsoleAlpha = alpha;

	if((console->OutputScreen->flags & SDL_OPENGLBLIT) == 0) {
		if(alpha == 0)
			SDL_SetAlpha(console->ConsoleSurface, 0, alpha);
		else
			SDL_SetAlpha(console->ConsoleSurface, SDL_SRCALPHA, alpha);
	}

	//	CON_UpdateConsole(console);
}


/* Adds  background image to the console, x and y based on consoles x and y */
int CON_Background(ConsoleInformation *console, const char *image, int x, int y) {
	SDL_Surface *temp;
	SDL_Rect backgroundsrc, backgrounddest;

	if(!console)
		return 1;

	/* Free the background from the console */
	if(image == NULL) {
		if(console->BackgroundImage ==NULL)
			SDL_FreeSurface(console->BackgroundImage);
		console->BackgroundImage = NULL;
		SDL_FillRect(console->InputBackground, NULL, SDL_MapRGBA(console->ConsoleSurface->format, 0, 0, 0, SDL_ALPHA_OPAQUE));
		return 0;
	}

	/* Load a new background */
	temp = IMG_Load(image);
	if(!temp) {
		CON_Out(console, "Cannot load background %s.", image);
		return 1;
	}

	console->BackgroundImage = SDL_DisplayFormat(temp);
	SDL_FreeSurface(temp);
	console->BackX = x;
	console->BackY = y;

	backgroundsrc.x = 0;
	backgroundsrc.y = console->ConsoleSurface->h - console->FontHeight - console->BackY;
	backgroundsrc.w = console->BackgroundImage->w;
	backgroundsrc.h = console->InputBackground->h;

	backgrounddest.x = console->BackX;
	backgrounddest.y = 0;
	backgrounddest.w = console->BackgroundImage->w;
	backgrounddest.h = console->FontHeight;

	SDL_FillRect(console->InputBackground, NULL, SDL_MapRGBA(console->ConsoleSurface->format, 0, 0, 0, SDL_ALPHA_OPAQUE));
	SDL_BlitSurface(console->BackgroundImage, &backgroundsrc, console->InputBackground, &backgrounddest);

	return 0;
}

/* takes a new x and y of the top left of the console window */
void CON_Position(ConsoleInformation *console, int x, int y) {
	if(!console)
		return;

	if(x < 0 || x > console->OutputScreen->w - console->ConsoleSurface->w)
		console->DispX = 0;
	else
		console->DispX = x;

	if(y < 0 || y > console->OutputScreen->h - console->ConsoleSurface->h)
		console->DispY = 0;
	else
		console->DispY = y;
}

/* resizes the console, has to reset alot of stuff
 * returns 1 on error */
int CON_Resize(ConsoleInformation *console, SDL_Rect rect) {
	SDL_Surface *Temp;
	SDL_Rect backgroundsrc, backgrounddest;

	if(!console)
		return 1;

	/* make sure that the size of the console is valid */
	if(rect.w > console->OutputScreen->w || rect.w < console->FontWidth * 32)
		rect.w = console->OutputScreen->w;
	if(rect.h > console->OutputScreen->h || rect.h < console->FontHeight)
		rect.h = console->OutputScreen->h;
	if(rect.x < 0 || rect.x > console->OutputScreen->w - rect.w)
		console->DispX = 0;
	else
		console->DispX = rect.x;
	if(rect.y < 0 || rect.y > console->OutputScreen->h - rect.h)
		console->DispY = 0;
	else
		console->DispY = rect.y;

	/* load the console surface */
	SDL_FreeSurface(console->ConsoleSurface);
	Temp = SDL_CreateRGBSurface(SDL_SWSURFACE, rect.w, rect.h, console->OutputScreen->format->BitsPerPixel, 0, 0, 0, 0);
	if(Temp == NULL) {
		PRINT_ERROR("Couldn't create the console->ConsoleSurface\n");
		return 1;
	}
	console->ConsoleSurface = SDL_DisplayFormat(Temp);
	SDL_FreeSurface(Temp);

	/* Load the dirty rectangle for user input */
	SDL_FreeSurface(console->InputBackground);
	Temp = SDL_CreateRGBSurface(SDL_SWSURFACE, rect.w, console->FontHeight, console->OutputScreen->format->BitsPerPixel, 0, 0, 0, 0);
	if(Temp == NULL) {
		PRINT_ERROR("Couldn't create the input background\n");
		return 1;
	}
	console->InputBackground = SDL_DisplayFormat(Temp);
	SDL_FreeSurface(Temp);

	/* Now reset some stuff dependent on the previous size */
	console->ConsoleScrollBack = 0;

	/* Reload the background image (for the input text area) in the console */
	if(console->BackgroundImage) {
		backgroundsrc.x = 0;
		backgroundsrc.y = console->ConsoleSurface->h - console->FontHeight - console->BackY;
		backgroundsrc.w = console->BackgroundImage->w;
		backgroundsrc.h = console->InputBackground->h;

		backgrounddest.x = console->BackX;
		backgrounddest.y = 0;
		backgrounddest.w = console->BackgroundImage->w;
		backgrounddest.h = console->FontHeight;

		SDL_FillRect(console->InputBackground, NULL, SDL_MapRGBA(console->ConsoleSurface->format, 0, 0, 0, SDL_ALPHA_OPAQUE));
		SDL_BlitSurface(console->BackgroundImage, &backgroundsrc, console->InputBackground, &backgrounddest);
	}

	/* restore the alpha level */
	CON_Alpha(console, console->ConsoleAlpha);
	return 0;
}

/* Transfers the console to another screen surface, and adjusts size */
int CON_Transfer(ConsoleInformation* console, SDL_Surface* new_outputscreen, SDL_Rect rect) {
	if(!console)
		return 1;

	console->OutputScreen = new_outputscreen;

	return( CON_Resize(console, rect) );
}

/* Sets the topmost console for input */
void CON_Topmost(ConsoleInformation *console) {
	SDL_Rect rect;

	if(!console)
		return;

	// Make sure the blinking cursor is gone
	if(Topmost) {
		rect.x = 0;
		rect.y = Topmost->ConsoleSurface->h - Topmost->FontHeight;
		rect.w = Topmost->InputBackground->w;
		rect.h = Topmost->InputBackground->h;
		SDL_BlitSurface(Topmost->InputBackground, NULL, Topmost->ConsoleSurface, &rect);
		DT_DrawText(Topmost->VCommand, Topmost->ConsoleSurface, Topmost->FontNumber, CON_CHAR_BORDER, Topmost->ConsoleSurface->h - Topmost->FontHeight);
	}
	Topmost = console;
}

/* Sets the Prompt for console */
void CON_SetPrompt(ConsoleInformation *console, char* newprompt) {
	if(!console)
		return;

	//check length so we can still see at least 1 char :-)
	if(strlen(newprompt) < console->VChars)
		console->Prompt = strdup(newprompt);
	else
		CON_Out(console, "prompt too long. (max. %i chars)", console->VChars - 1);
}

/* Sets the key that deactivates (hides) the console. */
void CON_SetHideKey(ConsoleInformation *console, int key) {
	if(console)
		console->HideKey = key;
}

/* Executes the command entered */
void CON_Execute(ConsoleInformation *console, char* command) {
	if(console)
		console->CmdFunction(console, command);
}

void CON_SetExecuteFunction(ConsoleInformation *console, void(*CmdFunction)(ConsoleInformation *console2, char* command)) {
	if(console)
		console->CmdFunction = CmdFunction;
}

void Default_CmdFunction(ConsoleInformation *console, char* command) {
	CON_Out(console, "     No CommandFunction registered");
	CON_Out(console, "     use 'CON_SetExecuteFunction' to register one");
	CON_Out(console, " ");
	CON_Out(console, "Unknown Command \"%s\"", command);
}

void CON_SetTabCompletion(ConsoleInformation *console, char*(*TabFunction)(char* command)) {
	if(console)
		console->TabFunction = TabFunction;
}

void CON_TabCompletion(ConsoleInformation *console) {
	int i,j;
	char* command;

	if(!console)
		return;

	command = strdup(console->LCommand);
	command = console->TabFunction(command);

	if(!command)
		return;	//no tab completion took place so return silently

	j = strlen(command);
	if(j > CON_CHARS_PER_LINE - 2)
		j = CON_CHARS_PER_LINE-1;

	memset(console->LCommand, 0, CON_CHARS_PER_LINE);
	console->CursorPos = 0;

	for(i = 0; i < j; i++) {
		console->CursorPos++;
		console->LCommand[i] = command[i];
	}
	//add a trailing space
	console->CursorPos++;
	console->LCommand[j] = ' ';
	console->LCommand[j+1] = '\0';
}

char* Default_TabFunction(char* command) {
	CON_Out(Topmost, "     No TabFunction registered");
	CON_Out(Topmost, "     use 'CON_SetTabCompletion' to register one");
	CON_Out(Topmost, " ");
	return NULL;
}

void Cursor_Left(ConsoleInformation *console) {
	char temp[CON_CHARS_PER_LINE];

	if(Topmost->CursorPos > 0) {
		Topmost->CursorPos--;
		strcpy(temp, Topmost->RCommand);
		strcpy(Topmost->RCommand, &Topmost->LCommand[strlen(Topmost->LCommand)-1]);
		strcat(Topmost->RCommand, temp);
		Topmost->LCommand[strlen(Topmost->LCommand)-1] = '\0';
		//CON_Out(Topmost, "L:%s, R:%s", Topmost->LCommand, Topmost->RCommand);
	}
}

void Cursor_Right(ConsoleInformation *console) {
	char temp[CON_CHARS_PER_LINE];

	if(Topmost->CursorPos < strlen(Topmost->Command)) {
		Topmost->CursorPos++;
		strncat(Topmost->LCommand, Topmost->RCommand, 1);
		strcpy(temp, Topmost->RCommand);
		strcpy(Topmost->RCommand, &temp[1]);
		//CON_Out(Topmost, "L:%s, R:%s", Topmost->LCommand, Topmost->RCommand);
	}
}

void Cursor_Home(ConsoleInformation *console) {
	char temp[CON_CHARS_PER_LINE];

	Topmost->CursorPos = 0;
	strcpy(temp, Topmost->RCommand);
	strcpy(Topmost->RCommand, Topmost->LCommand);
	strncat(Topmost->RCommand, temp, strlen(temp));
	memset(Topmost->LCommand, 0, CON_CHARS_PER_LINE);
}

void Cursor_End(ConsoleInformation *console) {
	Topmost->CursorPos = strlen(Topmost->Command);
	strncat(Topmost->LCommand, Topmost->RCommand, strlen(Topmost->RCommand));
	memset(Topmost->RCommand, 0, CON_CHARS_PER_LINE);
}

void Cursor_Del(ConsoleInformation *console) {
	char temp[CON_CHARS_PER_LINE];

	if(strlen(Topmost->RCommand) > 0) {
		strcpy(temp, Topmost->RCommand);
		strcpy(Topmost->RCommand, &temp[1]);
	}
}

void Cursor_BSpace(ConsoleInformation *console) {
	if(Topmost->CursorPos > 0) {
		Topmost->CursorPos--;
		Topmost->Offset--;
		if(Topmost->Offset < 0)
			Topmost->Offset = 0;
		Topmost->LCommand[strlen(Topmost->LCommand)-1] = '\0';
	}
}

void Cursor_Add(ConsoleInformation *console, SDL_Event *event) {
	if(strlen(Topmost->Command) < CON_CHARS_PER_LINE - 1 && event->key.keysym.unicode) {
		Topmost->CursorPos++;
		Topmost->LCommand[strlen(Topmost->LCommand)] = (char)event->key.keysym.unicode;
		Topmost->LCommand[strlen(Topmost->LCommand)] = '\0';
	}
}

void Clear_Command(ConsoleInformation *console) {
	Topmost->CursorPos = 0;
	memset(Topmost->VCommand, 0, CON_CHARS_PER_LINE);
	memset(Topmost->Command, 0, CON_CHARS_PER_LINE);
	memset(Topmost->LCommand, 0, CON_CHARS_PER_LINE);
	memset(Topmost->RCommand, 0, CON_CHARS_PER_LINE);
}

void Clear_History(ConsoleInformation *console) {
	int loop;

	for(loop = 0; loop <= console->LineBuffer - 1; loop++)
		memset(console->ConsoleLines[loop], 0, CON_CHARS_PER_LINE);
}

void Command_Up(ConsoleInformation *console) {
	if(console->CommandScrollBack < console->TotalCommands - 1) {
		/* move back a line in the command strings and copy the command to the current input string */
		console->CommandScrollBack++;
		memset(console->RCommand, 0, CON_CHARS_PER_LINE);
		console->Offset = 0;
		strcpy(console->LCommand, console->CommandLines[console->CommandScrollBack]);
		console->CursorPos = strlen(console->CommandLines[console->CommandScrollBack]);
		CON_UpdateConsole(console);
	}
}

void Command_Down(ConsoleInformation *console) {
	if(console->CommandScrollBack > -1) {
		/* move forward a line in the command strings and copy the command to the current input string */
		console->CommandScrollBack--;
		memset(console->RCommand, 0, CON_CHARS_PER_LINE);
		memset(console->LCommand, 0, CON_CHARS_PER_LINE);
		console->Offset = 0;
		if(console->CommandScrollBack > -1)
			strcpy(console->LCommand, console->CommandLines[console->CommandScrollBack]);
		console->CursorPos = strlen(console->LCommand);
		CON_UpdateConsole(console);
	}
}

