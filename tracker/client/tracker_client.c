/* 
   This is code needed to contact and interact with the tracker server. Contact
   me at kip@thevertigo.com for comments. Yes, I thought about GGZ, but it wasn't
   fully portable at the time of writing.

   This file and the acompanying tracker.h file are free software;
   you can redistribute them and/or modify them under the terms of the GNU
   Library General Public License as published by the Free Software Foundation;
   either version 2 of the License, or (at your option) any later version.

   These files are distributed in the hope that they will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public
   License along with these files; see the file COPYING.LIB.  If not,
   write to the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
   Boston, MA 02111-1307, USA.
*/

// Includes...
#include "dl_list.h"
#include "error.h"
#include "netdrv.h"
#include "newmenu.h"
#include <SDL.h>
#include <SDL/SDL_thread.h>
#include "text.h"
#include "tracker/tracker.h"
#include "u_mem.h"

// Maximum number of games to list...
#define TRACKER_MAX_GAMES   64

// Current status states...
typedef enum
{
    Null,           /* Nothing happening yet */
    Initializing,   /* Initializing stuff */
    Connecting,     /* Connecting to the tracker server */
    Refreshing,     /* Refreshing */

}TrackerState;

// Tracker data used to communicate with communication thread...
struct
{
    // Mutex...
    SDL_mutex      *pMutex;

    // Status...
    TrackerState    State;

    // Game list...
    dl_list        *GameList;

    // Toggle to tell tracker thread to gracefuly exit...
    unsigned char   AbortRequested;

}TrackerData;


// Callback to update menu GUI...
static void TrackerUpdateBrowseMenuCallback(
    int nItems, newmenu_item *pMenuItems, int *nLastKey, int nCurrentItem)
{
    // Lock the tracker mutex...
    SDL_LockMutex(TrackerData.pMutex);

    // Abort requested...
    if(TrackerData.AbortRequested)
    {
        // Update the status...
        nItems = 1;
        pMenuItems[0].type  = NM_TYPE_TEXT;
        pMenuItems[0].text  = "Aborting, please wait...";

        // Done...
        return;
    }

    // What state is the tracker in?
    switch(TrackerData.State)
    {
        // Nothing...
        default:
        case Null:
        {
            // Update the status...
            nItems = 1;
	        pMenuItems[0].type  = NM_TYPE_TEXT;
	        pMenuItems[0].text  = "";

            // Done...
            break;
        }

        // Initializing...
        case Initializing:
        {
            // Update the status...
            nItems = 1;
	        pMenuItems[0].type  = NM_TYPE_TEXT;
	        pMenuItems[0].text  = "Initializing, please wait...";

            // Done...
            break;
        }

        // Connecting...
        case Connecting:
        {
            // Update the status...
            nItems = 1;
	        pMenuItems[0].type  = NM_TYPE_TEXT;
	        pMenuItems[0].text  = "Connecting to tracker server...";

            // Done...
            break;
        }
        
        // Refreshing...
        case Refreshing:
        {
            // Update the status...
            nItems = 1;
	        pMenuItems[0].type  = NM_TYPE_TEXT;
	        pMenuItems[0].text  = "Refreshing...";

            // Done...
            break;
        }
    }

    // Unlock the tracker mutex...
    SDL_UnlockMutex(TrackerData.pMutex);
}

// Tracker communication thread...
static int TrackerCommunicationThread(void *pThreadData)
{
    /* Test code to make sure GUI is working the way I plan */

    SDL_Delay(1000);

    // Initializing...
    SDL_LockMutex(TrackerData.pMutex);
    TrackerData.State = Initializing;
    SDL_UnlockMutex(TrackerData.pMutex);
    
    SDL_Delay(1000);
    SDL_LockMutex(TrackerData.pMutex);
    TrackerData.State = Connecting;
    SDL_UnlockMutex(TrackerData.pMutex);
    
    while(1)
    {
        SDL_Delay(1000);
        
        SDL_LockMutex(TrackerData.pMutex);

        if(TrackerData.AbortRequested)
        {
            return 0;
        }

        TrackerData.State = Refreshing;
        SDL_UnlockMutex(TrackerData.pMutex);
    }

    // Stubbed...
    return 0;
}


// Show the browse netgames menu...
void TrackerBrowseMenu()
{
	// Variables...
	newmenu_item        MenuItems[TRACKER_MAX_GAMES];
	SDL_Thread         *pCommunicationThread    = NULL;

	// This isn't available in release mode yet...
	#ifdef NDEBUG
	
	    // Alert user...
        nm_messagebox(NULL, 1, TXT_OK, "Sorry, but NetGame browsing is not\n"
            "enabled in release mode yet.");

        // Done...
        return;

    #endif

    // Initialize tracker data...
    memset(&TrackerData, '\x0', sizeof(TrackerData));
    
        // Create mutex...
        TrackerData.pMutex = SDL_CreateMutex();
        
            // Failed...
            if(!TrackerData.pMutex)
    			Error("Failed to create tracker mutex...");

        // State...
        TrackerData.State = Null;
        
        // Game list...
        TrackerData.GameList = dl_init();
        
        // Abort flag...
        TrackerData.AbortRequested = 0;

    // Launch the communication thread...
    pCommunicationThread = SDL_CreateThread(TrackerCommunicationThread, NULL);
    
        // Failed... (not sure if SDL has its own handler already, oh well)
        if(!pCommunicationThread)
            Error("Failed to spawn communication thread.");

    // Initialize the stubbed menu...
    memset(MenuItems, '\x0', sizeof(MenuItems));
	MenuItems[0].type   = NM_TYPE_TEXT;
	MenuItems[0].text   = "                                    ";

    // Display the GUI...
	int const nMenuReturn = newmenu_do(
	    "UDP/IP NetGames", NULL, 1, MenuItems, TrackerUpdateBrowseMenuCallback);

    // Signal to communication thread to terminate gracefully...
    TrackerData.AbortRequested = 1;

    // Wait for thread to die...
    SDL_WaitThread(pCommunicationThread, NULL);
    
    // Destroy the mutex...
    SDL_DestroyMutex(TrackerData.pMutex);
    TrackerData.pMutex = NULL;
    
    // Cleanup the game list if allocated...
    if(TrackerData.GameList)
    {
	    // Free each node...
	    while(TrackerData.GameList->first)
		    dl_remove(TrackerData.GameList, TrackerData.GameList->first);

        // Free the list itself...
	    d_free(TrackerData.GameList);
	    TrackerData.GameList = NULL;
    }

    // User hit escape... (or something else unexpected)
    if(nMenuReturn < 0)
        return;
}

