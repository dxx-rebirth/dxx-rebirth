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
#include <key.h>
#include <config.h>
#include <arpa/inet.h>
#include "dl_list.h"
#include "error.h"
#include <errno.h>
#include <multi.h>
#include <netdb.h>
#include "netdrv.h"
#include "newmenu.h"
#include <SDL/SDL.h>
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

    // Remove the game if we have it locally tracked...
    static void TrackerRemoveGame(char const *pszDescription);

    // Transmit a message to tracker, returning non-zero if ok. If unSize is
    //  zero, assumes null terminated string in pszMessage...
    static int TrackerSend(
        int Socket, char const *pszMessage, unsigned int unSize);

    // Convert ASCII address or hostname into a socket address...
    static struct in_addr *TrackerStringToAddress(
        char const *pszAddress, struct in_addr *pAddressBuffer);

    // Toggle the error flag and error message. Needed for showing errors from
    //  secondary threads...
    static void TrackerThreadSetError(char const *pszError);

    // Callback to update menu GUI...
    static void TrackerUpdateBrowseMenuCallback(
        int nItems, newmenu_item *pMenuItems, int *pnLastKey, int nCurrentItem);

// Current status states...
typedef enum
{
    Null,           /* Nothing happening yet */
    Initializing,   /* Initializing stuff */
    Connecting,     /* Connecting to the tracker server */
    Refreshing,     /* Refreshing */

}TrackerState;

// TrackerNetGame structure...
typedef struct
{
    // Name / description...
    char            szDescription[NETGAME_NAME_LEN + 1];
    
    // Hostname / IP address...
    char            szAddress[128];
    
    // Port...
    unsigned short  usPort;

}TrackerNetGame;

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

    // An error was detected...
    unsigned char   ErrorDetected;
    char            szError[256];

}TrackerData;


// Callback to update menu GUI...
static void TrackerUpdateBrowseMenuCallback(
    int nItems, newmenu_item *pMenuItems, int *pnLastKey, int nCurrentItem)
{
    // Variables...
    TrackerNetGame *pCurrentGame        = NULL;
    unsigned int    unCurrentMenuItem   = 0;

    // Lock the tracker mutex...
    SDL_LockMutex(TrackerData.pMutex);

    // An error was detected, display it...
    if(TrackerData.ErrorDetected)
    {
        // Display the error message...
        nm_messagebox(NULL, 1, TXT_OK, TrackerData.szError);

        // Unlock the tracker mutex...
        SDL_UnlockMutex(TrackerData.pMutex);

        // Menu should disappear now...
       *pnLastKey = KEY_ESC;

        // Done...
        return;
    }

    // Abort requested...
    if(TrackerData.AbortRequested)
    {
        // Update the status...
        nItems = 1;
        pMenuItems[0].type  = NM_TYPE_TEXT;
        pMenuItems[0].text  = "Aborting, please wait...";

        // Unlock the tracker mutex...
        SDL_UnlockMutex(TrackerData.pMutex);

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
            // Add the status item and separator...
            nItems = 1;
	        pMenuItems[0].type  = NM_TYPE_TEXT;
	        pMenuItems[0].text  = "Refreshing...";

            // We have games received, use them instead...
            if(dl_size(TrackerData.GameList) > 0)
            {
                // We'll use that many menu items to list each tracked game...
                nItems = dl_size(TrackerData.GameList);

                // Populate GUI with game list...

                    // Seek to beginning of game list...
                    TrackerData.GameList->current = TrackerData.GameList->first;
                    
                    // Start filling in menu items from beginning...
                    unCurrentMenuItem = 0;

                    // Add each game...
                    do
                    {
                        // Extract entry...
                        pCurrentGame = (TrackerNetGame *) TrackerData.GameList->current->data;
                        
                        // Fill in menu item...
                        pMenuItems[unCurrentMenuItem].type  = NM_TYPE_MENU;
                        pMenuItems[unCurrentMenuItem].text  = pCurrentGame->szDescription;

                        // Move to next menu item slot...
                        unCurrentMenuItem++;
                    }
                    while(dl_forward(TrackerData.GameList));
            }

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
    int                 Socket                              = 0;
    struct in_addr      ServerAddress;
    struct sockaddr_in  ServerSocketAddress;
    int                 nStatus                             = 0;
    char                szBuffer[1024]                      = {0};
    TrackerNetGame     *pCurrentGame                        = NULL;
    char                szDescription[NETGAME_NAME_LEN + 1] = {0};

    // Initializing...
        
        // Update GUI...
        SDL_LockMutex(TrackerData.pMutex);
        TrackerData.State = Initializing;
        SDL_UnlockMutex(TrackerData.pMutex);
        
        // Allocate socket...
        Socket = socket(PF_INET, SOCK_STREAM, 0);
        
            // Failed...
            if(Socket < 0)
            {
                // Set error...
                TrackerThreadSetError("Unable to allocate socket.");
                
                // Abort...
                return 0;
            }

        // Switch to non-blocking...
        if(fcntl(Socket, F_SETFL, O_NONBLOCK) != 0)
        {
            // Cleanup...
            close(Socket);
            
            // Set error and abort...
            TrackerThreadSetError("Unable to switch to non-blocking mode.");
            
            // Abort...
            return 0;
        }

    // Connecting...
        
        // Update GUI...
        SDL_LockMutex(TrackerData.pMutex);
        TrackerData.State = Connecting;
        SDL_UnlockMutex(TrackerData.pMutex);
        
        // Clear out server address...
        memset(&ServerAddress, 0, sizeof(struct in_addr));
        
        // Resolve address and check for error...
        if(!TrackerStringToAddress(GameCfg.TrackerServer, &ServerAddress))
        {
            // Cleanup...
            close(Socket);
            
            // Set error...
            TrackerThreadSetError("Unable to contact game tracker.");
            
            // Abort...
            return 0;
        }
    
        // Initialize socket address...
        memset(&ServerSocketAddress, 0, sizeof(struct sockaddr_in));
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
            if(nStatus == 0)
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
                
                // Connection refused...
                case ECONNREFUSED:
                {
                    // Cleanup...
                    close(Socket);

                    // Alert user...
                    TrackerThreadSetError("Tracker refused connection.");
                    
                    // Done...
                    return 0;
                }
                
                // Some other unknown error...
                default:
                {
                    // Cleanup...
                    close(Socket);

                    // Alert user...
                    TrackerThreadSetError("Unknown tracker error.");
                    
                    // Done...
                    return 0;
                }
            }
        }

    // Handshake...
    if(!TrackerSend(Socket, "MATERIAL\n", 0) ||
       !TrackerReceive(Socket, szBuffer, sizeof(szBuffer)) ||
       strcmp(szBuffer, "DEFENDER\n") ||
       !TrackerSend(Socket, 
            "USERAGENT " PROGRAM_NAME " " D1XMAJOR " " D1XMINOR "\n", 0) ||
       !TrackerReceive(Socket, szBuffer, sizeof(szBuffer)))
    {
        // Cleanup...
        close(Socket);
        
        // Alert user...
        TrackerThreadSetError("Tracker handshake failed.");
        
        // Done...
        return 0;
    }
    
    // Check if user agent was not accepted...
    if(strncmp(szBuffer, "FAIL", strlen("FAIL")) == 0)
    {
        // Cleanup...
        close(Socket);
        
        // Alert user with server message, if any...
        if(strlen(szBuffer) > strlen("FAIL\n"))
            TrackerThreadSetError(&szBuffer[strlen("FAIL") + 1]);
        
        // Otherwise, use default...
        else
            TrackerThreadSetError("Your client was not accepted.");

        // Done...
        return 0;
    }

    // Chomp user agent accepted and send ready...
    if(strcmp(szBuffer, "OK\n") != 0)
    {
        // Cleanup...
        close(Socket);
        
        // Alert user...
        TrackerThreadSetError("Tracker sent garbage.");

        // Done...
        return 0;
    }

    // Update GUI...
    SDL_LockMutex(TrackerData.pMutex);
    TrackerData.State = Refreshing;
    SDL_UnlockMutex(TrackerData.pMutex);

    // Keep refreshing until we can't anymore...
    while(!TrackerData.AbortRequested)
    {
        // Receive a line and check for error...
        if(!TrackerReceive(Socket, szBuffer, sizeof(szBuffer)))
        {
            // Alert user...
            TrackerThreadSetError("Lost connection with tracker.");
            
            // Abort...
            break;
        }

        // Process notification...

            // Server wants us to add a game to our list...
            if(strncmp(szBuffer, "GAME_ADD ", strlen("GAME_ADD ")) == 0)
            {
                // Allocate a new game structure...
                pCurrentGame = d_malloc(sizeof(TrackerNetGame));

                // Clear it...
                memset(pCurrentGame, '\x0', sizeof(TrackerNetGame));

                // Parse it and check for error...
                /* GAME_ADD <address>:<port> "<description>" */
                if(sscanf(szBuffer, "GAME_ADD %s %hu \"%15[^\"]s", 
                    pCurrentGame->szAddress,
                    &pCurrentGame->usPort,
                    pCurrentGame->szDescription) < 3)
                {
                    // Alert user...
                    TrackerThreadSetError("Tracker list contained garbage.");
                    
                    // Abort...
                    break;
                }

                // Store in game list...
                SDL_LockMutex(TrackerData.pMutex);
                dl_add(TrackerData.GameList, pCurrentGame);
                SDL_UnlockMutex(TrackerData.pMutex);
            }
            
            // Server wants us to remove a game from our list...
            else if(strncmp(szBuffer, "GAME_REM ", strlen("GAME_REM ")) == 0)
            {
                // Parse it and check for error...
                /* GAME_REM "<description>" */
                if(sscanf(szBuffer, "GAME_REM \"%[^\"]s", szDescription) != 1)
                {
                    // Alert user...
                    TrackerThreadSetError("Tracker sent garbage.");
                    
                    // Abort...
                    break;
                }

                // Remove the game if we have it locally tracked...
                TrackerRemoveGame(szDescription);
            }
            
            // Server wants us to display an alert...
            else if(strncmp(szBuffer, "ALERT ", strlen("ALERT ")) == 0)
            {
puts("Alert...");
            }
            
            // Unknown...
            else
            {
                // Alert user...
                TrackerThreadSetError("Received unknown notification.");

                // Abort...
                break;
            }

        // Lock tracker data...        
        SDL_LockMutex(TrackerData.pMutex);
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
        
        // Error stuff...
        TrackerData.ErrorDetected = 0;
        strcpy(TrackerData.szError, "");

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
	    {
		    // Free the node's data first...
		    d_free(TrackerData.GameList->first->data);
		    TrackerData.GameList->first->data = NULL;
		    
		    // Now the node...
		    dl_remove(TrackerData.GameList, TrackerData.GameList->first);
        }

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

                        // Try again in a bit...
                        SDL_Delay(100);
                        continue;
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
                // This is the end of a server message...
                if(pszBuffer[unReceived] == '\n')
                    return TRUE;

                // Update counter...
                unReceived += nStatus;
            }
    }
    
    // Ran out of space...
    return FALSE;
}

// Remove the game if we have it locally tracked...
static void TrackerRemoveGame(char const *pszDescription)
{
    // Variables...
    TrackerNetGame *pCurrentGame    = NULL;

    // Lock resources...
    SDL_LockMutex(TrackerData.pMutex);
    
    // List is empty, so nothing to remove...
    if(dl_is_empty(TrackerData.GameList))
        return;

    // Seek to beginning of game list...
    TrackerData.GameList->current = TrackerData.GameList->first;

    // Search for the game...
    do
    {
        // Extract entry...
        pCurrentGame = (TrackerNetGame *) TrackerData.GameList->current->data;
        
        // Match...
        if(strcmp(pCurrentGame->szDescription, pszDescription) == 0)
        {
            // Remove game...
            dl_remove(TrackerData.GameList, TrackerData.GameList->current);
            
            // Unlock resources...
            SDL_UnlockMutex(TrackerData.pMutex);

            // Done...
            return;
        }
    }
    while(dl_forward(TrackerData.GameList));

    // Unlock resources...
    SDL_UnlockMutex(TrackerData.pMutex);
}

// Transmit a message to tracker, returning non-zero if ok. If unSize is zero, 
//  assumes null terminated string in pszMessage...
static int TrackerSend(
    int Socket, char const *pszMessage, unsigned int unSize)
{
    // Variables...
    unsigned int    unSent      = 0;
    unsigned int    unRemaining = 0;
    int             nStatus     = 0;

    // Size not specified, so must be null terminated string...
    if(!unSize)
        unRemaining = unSize = strlen(pszMessage);
    
    // Size already specified...
    else
        unRemaining = unSize;

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

// Toggle the error flag and error message. Needed for showing errors from
//  secondary threads...
static void TrackerThreadSetError(char const *pszError)
{
    // Lock resources...
    SDL_LockMutex(TrackerData.pMutex);

    // Toggle error and abort flag...
    TrackerData.AbortRequested  = TRUE;
    TrackerData.ErrorDetected   = TRUE;
    
    // Store error message...
    strcpy(TrackerData.szError, pszError);

    // Unlock resources...
    SDL_UnlockMutex(TrackerData.pMutex);
}

