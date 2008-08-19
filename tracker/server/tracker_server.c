/* 
   This is code needed to host the DXX-Rebirth tracker server. Contact
   me at kip@thevertigo.com for comments. Yes, I thought about GGZ, but it wasn't
   fully portable at the time of writing.

   This file and the acompanying tracker_server.h file are free software;
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
#include <sys/socket.h>       /*  socket definitions        */
#include <sys/types.h>        /*  socket types              */
#include <arpa/inet.h>        /*  inet (3) funtions         */
#include <unistd.h>           /*  misc. UNIX functions      */
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>
#include <errno.h>

/* 
    Note: This is all just TEST driver for protocol. Consider nothing here to 
    even remotely resemble the actual server code.
*/

#define TRACKER_PORT        (7988)
#define MAX_LINE            (1000)

#ifndef PG_SOCK_HELP
#define PG_SOCK_HELP

#define LISTENQ        (1024)   /*  Backlog for listen()   */


/*  Read a line from a socket  */

ssize_t Readline(int sockd, void *vptr, size_t maxlen) {
    ssize_t n, rc;
    char    c, *buffer;

    buffer = vptr;

    for ( n = 1; n < maxlen; n++ ) {
	
	if ( (rc = read(sockd, &c, 1)) == 1 ) {
	    *buffer++ = c;
	    if ( c == '\n' )
		break;
	}
	else if ( rc == 0 ) {
	    if ( n == 1 )
		return 0;
	    else
		break;
	}
	else {
	    if ( errno == EINTR )
		continue;
	    return -1;
	}
    }

    *buffer = 0;
    return n;
}


/*  Write a line to a socket  */

ssize_t Writeline(int sockd, const void *vptr, size_t n) {
    size_t      nleft;
    ssize_t     nwritten;
    const char *buffer;

    buffer = vptr;
    nleft  = n;

    while ( nleft > 0 ) {
	if ( (nwritten = write(sockd, buffer, nleft)) <= 0 ) {
	    if ( errno == EINTR )
		nwritten = 0;
	    else
		return -1;
	}
	nleft  -= nwritten;
	buffer += nwritten;
    }

    return n;
}




#endif  /*  PG_SOCK_HELP  */



int main(int argc, char *argv[]) {
    int       list_s;                /*  listening socket          */
    int       conn_s;                /*  connection socket         */
    short int port;                  /*  port number               */
    struct    sockaddr_in servaddr;  /*  socket address structure  */
    char      buffer[MAX_LINE];      /*  character buffer          */
    char     *endptr;                /*  for strtol()              */


    /*  Get port number from the command line, and
        set to default port if no arguments were supplied  */

    if ( argc == 2 ) {
	port = strtol(argv[1], &endptr, 0);
	if ( *endptr ) {
	    fprintf(stderr, "TRACKER: Invalid port number.\n");
	    exit(EXIT_FAILURE);
	}
    }
    else if ( argc < 2 ) {
	port = TRACKER_PORT;
    }
    else {
	fprintf(stderr, "TRACKER: Invalid arguments.\n");
	exit(EXIT_FAILURE);
    }

	
    /*  Create the listening socket  */

    if ( (list_s = socket(AF_INET, SOCK_STREAM, 0)) < 0 ) {
	fprintf(stderr, "TRACKER: Error creating listening socket.\n");
	exit(EXIT_FAILURE);
    }


    /*  Set all bytes in socket address structure to
        zero, and fill in the relevant data members   */

    memset(&servaddr, 0, sizeof(servaddr));
    servaddr.sin_family      = AF_INET;
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    servaddr.sin_port        = htons(port);


    /*  Bind our socket addresss to the 
	listening socket, and call listen()  */

    if ( bind(list_s, (struct sockaddr *) &servaddr, sizeof(servaddr)) < 0 ) {
	fprintf(stderr, "Error calling bind()\n");
	exit(EXIT_FAILURE);
    }

    if ( listen(list_s, LISTENQ) < 0 ) {
	fprintf(stderr, "Error calling listen()\n");
	exit(EXIT_FAILURE);
    }

    printf("Test server ready...\n");

	if ( (conn_s = accept(list_s, NULL, NULL) ) < 0 ) {
	    fprintf(stderr, "Error calling accept()\n");
	    exit(EXIT_FAILURE);
	}

    if(!Readline(conn_s, buffer, MAX_LINE-1) ||
       strcmp(buffer, "MATERIAL\n") ||
       !Writeline(conn_s, "DEFENDER\n", strlen("DEFENDER\n")) || 
       !Readline(conn_s, buffer, MAX_LINE-1) ||
       !Writeline(conn_s, "OK\n", strlen("OK\n")))
        exit(EXIT_FAILURE);

    printf("Uploading game list to client...");

    strcpy(buffer, "GAME_ADD 12.34.56.78 7988 \"Dicky Chow.\"\n");
    if(!Writeline(conn_s, buffer, strlen(buffer)))
        exit(EXIT_FAILURE);

    strcpy(buffer, "GAME_ADD 23.45.67.89 7943 \"Kip's game.\"\n");
    if(!Writeline(conn_s, buffer, strlen(buffer)))
        exit(EXIT_FAILURE);
        
    strcpy(buffer, "GAME_ADD 21.54.16.29 7943 \"Christian's.\"\n");
    if(!Writeline(conn_s, buffer, strlen(buffer)))
        exit(EXIT_FAILURE);

    printf("done.\nHit enter to close connection and quit.");
    getchar();

	/*  Close the connected socket  */

	if ( close(conn_s) < 0 )
	{
	    fprintf(stderr, "TRACKER: Error calling close()\n");
	    exit(EXIT_FAILURE);
    }
    
    return 0;
}


