#include "clipboard.h"

#include <SDL/SDL.h>
#include <SDL/SDL_syswm.h>
#include <limits.h>
#include <string.h>

//adapted by Matthew Mueller from the Scrap 1.0 example by Sam Lantinga
// ( see http://www.libsdl.org/projects/scrap/ )
int getClipboardText(char *text, int strlength)
{
	int retval = 0;
	SDL_SysWMinfo info;
	SDL_VERSION(&info.version);
	if ( SDL_GetWMInfo(&info) )
	{
		if ( info.subsystem == SDL_SYSWM_X11 )
		{
			Display *SDL_Display = info.info.x11.display;
			Window SDL_Window = info.info.x11.window;
			void (*Lock_Display)(void) = info.info.x11.lock_func;
			void (*Unlock_Display)(void) = info.info.x11.unlock_func;
			
			Window owner;
			Atom selection;
			Atom seln_type;
			int seln_format;
			unsigned long nbytes;
			unsigned long overflow;
			unsigned char *src;

			/* Enable the special window hook events */
			SDL_EventState(SDL_SYSWMEVENT, SDL_ENABLE);

			Lock_Display();
			owner = XGetSelectionOwner(SDL_Display, XA_PRIMARY);
			Unlock_Display();
			if ( (owner == None) || (owner == SDL_Window) )
			{
				owner = DefaultRootWindow(SDL_Display);
				selection = XA_CUT_BUFFER0;
			}
			else
			{
				int selection_response = 0;
				SDL_Event event;

				owner = SDL_Window;
				Lock_Display();
				selection = XInternAtom(SDL_Display, "SDL_SELECTION", False);
				XConvertSelection(SDL_Display, XA_PRIMARY, XA_STRING,
						selection, owner, CurrentTime);
				Unlock_Display();
				while ( ! selection_response )
				{
					SDL_WaitEvent(&event);
					if ( event.type == SDL_SYSWMEVENT )
					{
						XEvent xevent = event.syswm.msg->event.xevent;

						if ( (xevent.type == SelectionNotify) &&
								(xevent.xselection.requestor == owner) )
							selection_response = 1;
					}
				}
			}
			Lock_Display();
			if ( XGetWindowProperty(SDL_Display, owner, selection, 0, INT_MAX/4,
						False, XA_STRING, &seln_type, &seln_format,
						&nbytes, &overflow, &src) == Success )
			{
				if ( seln_type == XA_STRING )
				{
					strncpy(text, src, strlength);
					retval = nbytes;
				}
				XFree(src);
			}
			Unlock_Display();

			SDL_EventState(SDL_SYSWMEVENT, SDL_DISABLE);
		}
		//else
		//	SDL is not running on X11

	}

	return retval;
}
