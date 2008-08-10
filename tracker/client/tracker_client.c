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
#include <config.h>
#include "dl_list.h"
#include "error.h"
#include <errno.h>
#include <netdb.h>
#include "netdrv.h"
#include "newmenu.h"
#include <SDL.h>
#include <SDL/SDL_thread.h>
#include "text.h"
#include "tracker/tracker.h"
#include <sys/fcntl.h>
#include <sys/socket.h>
#include <sys/unistd.h>
#include "u_mem.h"

// Maximum number of games to list...
#define TRACKER_MAX_GAMES   64

// Truth...
#ifndef TRUE
#define TRUE 1
#endif

// Falsity...
#ifndef FALSE
#define FALSE !TRUE
#endif

// Private functions...

    // Tracker communication thread...
    static int TrackerCommunicationThread(void *pThreadData);

    // Receive a message from tracker, returning non-zero if ok...
    static int TrackerReceive(
        int Socket, char *pszBuffer, unsigned int const unSize);

    // Transmit a message to tracker, returning non-zero if ok...
    static int TrackerSend(
        int Socket, char const *pszMessage, unsigned int const unSize);

    // Convert ASCII address or hostname into a socket address...
    static struct in_addr *TrackerStringToAddress(
        char const *pszAddress, struct in_addr *pAddressBuffer);

    // Callback to update menu GUI...
    static void TrackerUpdateBrowseMenuCallback(
        int nItems, newmenu_item *pMenuItems, int *nLastKey, int nCurrentItem);

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
    // Variables...
    int                 Socket                  = 0;
    struct in_addr      ServerAddress;
    struct sockaddr_in  ServerSocketAddress;
    int                 nStatus                 = 0;


    // Initializing...
        
        // Update GUI...
        SDL_LockMutex(TrackerData.pMutex);
        TrackerData.State = Initializing;
        SDL_UnlockMutex(TrackerData.pMutex);
        
        // Allocate socket...
        Socket = socket(PF_INET, SOCK_STREAM, 0);
        
            // Failed...
            if(Socket < 0)
                Error("Unable to allocate socket.");

        // Switch to non-blocking...
        if(fcntl(Socket, F_SETFL, O_NONBLOCK) != 0)
        {
            // Cleanup and abort...
            close(Socket);
            Error("Unable to switch to non-blocking mode.");
        }

    // Connecting...
        
        // Update GUI...
        SDL_LockMutex(TrackerData.pMutex);
        TrackerData.State = Connecting;
        SDL_UnlockMutex(TrackerData.pMutex);
        
        // Clear out server address...
        memset(&ServerAddress, 0, sizeof(struct sockaddr_in));
        
        // Resolve address and check for error...
        if(!TrackerStringToAddress(GameCfg.TrackerServer, &ServerAddress))
        {
            // Cleanup...
            close(Socket);
            
            // Alert user...
            Error("Unable to contact game tracker.");
            
            // Done...
            return 0;
        }
    
        // Initialize socket address...
        memset(&ServerSocketAddress, 0, sizeof(struct in_addr));
        ServerSocketAddress.sin_family      = AF_INET;
        ServerSocketAddress.sin_port        = htons(TRACKER_PORT);
        ServerSocketAddress.sin_addr.s_addr = ServerAddress.s_addr;

        // Connect...
        while(TRUE)
        {
            // Try to connect...
            nStatus = connect(Socket, (struct sockaddr *) &ServerSocketAddress, 
                sizeof(ServerSocketAddress));

            // Connect completed...
            if(nStatus >= 0)
                break;
            
            // Something else happened...
            switch(errno)
            {
                // Connection already in progress...
                case EINPROGRESS:
                {
                    // User triggered an abort...
                    if(TrackerData.AbortRequested)
                    {
                        // Cleanup...
                        close(Socket);

                        // Abort...
                        return 0;
                    }

                    // Don't spin the clock...
                    SDL_Delay(50);
                    
                    // Give it some more time...
                    continue;
                }
                
                // Some other unknown error...
                default:
                {
                    // Cleanup...
                    close(Socket);
                    
                    // Alert user...
                    Error("Unknown error while connecting to tracker.");
                    
                    // Done...
                    return 0;                
                }
            }
        }

    // Refresh...

        // Update GUI...
        SDL_LockMutex(TrackerData.pMutex);
        TrackerData.State = Refreshing;
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

    // Cleanup...
    close(Socket);

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

// Receive a message from tracker, returning non-zero if everything went ok...
static int TrackerReceive(
    int Socket, char *pszBuffer, unsigned int const unSize)
{
    // Variables...
    unsigned int    unReceived  = 0;
    int             nStatus     = 0;

    // Not enough space to retrieve anything...
    if(unSize < 1)
        return FALSE;

    // Clear receive buffer...
    memset(pszBuffer, 0, unSize);

    // Receive loop...
    while(unReceived < unSize)
    {
        // Try to receive some data...
        nStatus = recv(Socket, &pszBuffer[unReceived], 1, 0);

            // An error occurred...
            if(nStatus < 0)
            {
                // What happened?
                switch(errno)
                {
                    // We were asked to try receiving again...
                    case EAGAIN:
                    {
                        // User aborted...
                        if(TrackerData.AbortRequested)
                            return FALSE;
                    }
                    
                    // Some other error...
                    default:
                        return FALSE;
                }
            }

            // Tracker closed connection on us...
            else if(nStatus == 0)
                return FALSE;

            // Received a byte...
            else
            {
                // Update counter...
                unReceived += nStatus;
                
                // This is the end of a server message...
                if(pszBuffer[unReceived] == '\n')
                    return TRUE;
            }
    }
    
    // Ran out of space...
    return FALSE;
}

// Transmit a message to tracker, returning non-zero if everything went ok...
static int TrackerSend(
    int Socket, char const *pszMessage, unsigned int const unSize)
{
    // Variables...
    unsigned int    unSent      = 0;
    unsigned int    unRemaining = unSize;
    int             nStatus     = 0;

    // Keep trying to send data until none left to send...
    while(unSent < unSize)
    {
        // Transmit...
        nStatus = send(Socket, &pszMessage[unSent], unRemaining, 0);
        
        // Something interesting happened...
        if(nStatus < 0)
        {
            // What?
            switch(errno)
            {
                // We were asked to try sending again...
                case EAGAIN:
                {
                    // User aborted...
                    if(TrackerData.AbortRequested)
                        return FALSE;
                }
                
                // Some other error...
                default:
                    return FALSE;
            }
        }

        // Update counters...
        unSent      += nStatus;
        unRemaining -= nStatus;
    }

    // Done...
    return TRUE;
}

// Convert ASCII address or hostname into a socket address...
static struct in_addr *TrackerStringToAddress(
    char const *pszAddress, struct in_addr *pAddressBuffer)
{
    // Variables...
    struct hostent *pHostList = NULL;
    
    // Clear address...
    memset(pAddressBuffer, 0, sizeof(struct in_addr));

    // Is this an IP address?
    pAddressBuffer->s_addr = inet_addr(pszAddress);
        
        // Yes, use the new in_addr format...
        if(pAddressBuffer->s_addr != -1)
             return pAddressBuffer;

    // No, it's probably a host name. Try and resolve...
    pHostList = gethostbyname(pszAddress);
    
        // Lookup successfull...
        if(pHostList != NULL)
        {
            // Store first address...
            memcpy(pAddressBuffer, (struct in_addr *) *pHostList->h_addr_list, 
                   sizeof(struct in_addr));
            
            // Return to caller...
            return pAddressBuffer;
        }
    
    // Unknown or DNS down...
    return NULL;
}

