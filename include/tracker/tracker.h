/* 
   This is code needed to contact and interact with the tracker server. Contact
   me at kip@thevertigo.com for comments. Yes, I thought about GGZ, but it wasn't
   fully portable at the time of writing.

   This file and the acompanying tracker_client.c file are free software;
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

// Multiple include protection...
#ifndef _TRACKER_H
#define _TRACKER_H

// Don't bother with any of this if networking support isn't enabled...
#ifdef NETWORK

// The default game tracker server address...
#define TRACKER_DEFAULT_SERVER "localhost"

// Tracker port...
#define TRACKER_PORT 7988

// Invoke the browse UDP/IP network game GUI...
void TrackerBrowseMenu();

#endif // NETWORK

#endif // _TRACKER_CLIENT_H

