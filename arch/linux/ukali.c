#include <sys/socket.h>
#include <sys/types.h>
#include <string.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <stdlib.h>
#include <sys/time.h>
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>
#include "ukali.h"
//added 05/17/99 Matt Mueller - needed to redefine FD_* so that no asm is used
#include "checker.h"
//end addition -MM
int g_sockfd = -1;
struct sockaddr_in kalinix_addr;
char g_mynodenum[6];

int knix_newSock(void) {

	int tempsock;
	struct sockaddr_in taddr;

	taddr.sin_family = AF_INET;
	taddr.sin_addr.s_addr = inet_addr("127.0.0.1");
	taddr.sin_port = 0;

	if ((tempsock = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
		return -1;

	fcntl(tempsock, F_SETFL, fcntl(tempsock, F_GETFL) | O_NONBLOCK);

	if ((bind(tempsock, (struct sockaddr *)&taddr, sizeof(taddr))) < 0) {
		close(tempsock);
		return -1;
	}
	return tempsock;
}

int knix_Send(int hand, char *data, int len) {
	int i = 0, t;

	while ((t = sendto(hand, data, len, 0, (struct sockaddr *)&kalinix_addr, 
			sizeof(kalinix_addr))) < 0) {
		i++;
		if (i > 10)
			return 0;
	}

	return t;
}

int knix_Recv(int hand, char *data, int len) {
	struct sockaddr_in taddr;
	int tlen;

	tlen = sizeof(taddr);

	return recvfrom(hand, data, len, 0, (struct sockaddr *)&taddr, &tlen);

}

int knix_WaitforSocket(int hand, int timems) {
	fd_set set;
	struct timeval tv;

	FD_ZERO(&set);
	FD_SET(hand, &set);
	tv.tv_sec = 0;
	// 100ms
	tv.tv_usec = timems * 1000;
	return select(hand + 1, &set, NULL, NULL, &tv);
}

int knix_ReceivePacket(int hand, char *outdata, int *outlen, kaliaddr_ipx *from) {
	static char data[MAX_PACKET_SIZE];
	int len;

	len = knix_Recv(hand, data, sizeof(data));

	if (len <= 0) return 0;

	switch (data[0]) {
//		case 1: // open socket
//			break;
//		case 2: // close socket
//			break;
		case 3: // received data packet
			if (len < 11) break;
			if (!outdata)
				break;
			from->sa_family = AF_IPX;
			memcpy(from->sa_nodenum, &data[1], sizeof(from->sa_nodenum));
			memset(from->sa_netnum, 0, sizeof(from->sa_netnum));
			memcpy(&from->sa_socket, &data[9], sizeof(unsigned short));
			memcpy(outdata, &data[11], len-11 > *outlen ? *outlen : len-11);
			*outlen = len-11;
			break;
		case 4: // myipxaddress
			if (len < 7) break;
			memcpy(g_mynodenum, &data[1], 6);
			break;
//		case 5: // Init KaliNix connection
//			break;
		case 6: // open response
		case 7: // close response
			if (len < 3) break;
//			memcpy(g_LastPort, &data[1], sizeof(g_LastPort))
			break;
	}

	return data[0];

}

int knix_GetMyAddress(void) {
	char initdata[1];

	if (g_sockfd < 0) {
		kalinix_addr.sin_family = AF_INET;
		kalinix_addr.sin_addr.s_addr = inet_addr("127.0.0.1");
		kalinix_addr.sin_port = htons(4213);

		g_sockfd = knix_newSock();
	}

	initdata[0] = 5; // Get Address
	knix_Send(g_sockfd, initdata, sizeof(initdata));

	return 0;

}

int KaliSendPacket(int hand, char *data, int len, kaliaddr_ipx *to) {
	static char sendbuf[MAX_PACKET_SIZE+11];
//		char	code; == 3
//		char	sa_nodenum[6];
//		char	dport[2];
//		char	sport[2];
//		char data[];

	sendbuf[0] = 3;
	memcpy(&sendbuf[1], to->sa_nodenum, sizeof(to->sa_nodenum));
	memcpy(&sendbuf[7], &to->sa_socket, sizeof(to->sa_socket));
	memset(&sendbuf[9], 0, sizeof(to->sa_socket));
	len = len > MAX_PACKET_SIZE ? MAX_PACKET_SIZE : len;
	memcpy(&sendbuf[11], data, len);

	if (!knix_Send(hand, sendbuf, len+11)){
		return -1;
	}
	return len;

}

int KaliReceivePacket(int hand, char *data, int len, kaliaddr_ipx *from) {
	int newlen;
	int t;

	newlen = len;

	t = knix_ReceivePacket(hand, data, &newlen, from);

	while (t != 0 && t != 3)
		t = knix_ReceivePacket(hand, data, &newlen, from);

	if (t == 3)
		return newlen;
	else return -1;

}

int KaliGetNodeNum(kaliaddr_ipx *myaddr) {
	int tcount = 0;

	if (g_sockfd < 0 && knix_GetMyAddress())
		return -1;

	while (tcount < 5) {
		if (knix_WaitforSocket(g_sockfd, 100) > 0) {
			if (knix_ReceivePacket(g_sockfd, NULL, 0, NULL) == 4)
				break;
			continue;
		}
		knix_GetMyAddress();
		tcount++;
	}
	if (tcount == 5)
		return -1;

	close(g_sockfd);
	g_sockfd = -1;
	memcpy(myaddr->sa_nodenum, g_mynodenum, sizeof(g_mynodenum));

	return 0;

}

int KaliCloseSocket(int hand) {
	char opendata[3] = {2, 0, 0};
	int tcount = 0;

	knix_Send(hand, opendata, sizeof(opendata));

	while (tcount < 5) {

		if (knix_WaitforSocket(hand, 100) > 0) {
			if (knix_ReceivePacket(hand, NULL, 0, NULL) == 7)
				break;
			continue;
		}
		knix_Send(hand, opendata, sizeof(opendata));
		tcount++;
	}
	close(hand);
	return 0;

}

int KaliOpenSocket(unsigned short port) {
	char opendata[16];
	int hand;
	int tcount = 0;
	long pid;

	opendata[0] = 1; // open socket
	memcpy(&opendata[1], &port, sizeof(port));
	pid = (int)htonl(getpid());
	memcpy(&opendata[3], &pid, sizeof(pid));
	strncpy(&opendata[7], KALI_PROCESS_NAME, sizeof(KALI_PROCESS_NAME));
	opendata[15] = 0;

	if ((hand = knix_newSock()) < 0)
		return -1;

	knix_Send(hand, opendata, sizeof(opendata));

	while (tcount < 5) {

		if (knix_WaitforSocket(hand, 100) > 0) {
			if (knix_ReceivePacket(hand, NULL, 0, NULL) == 6)
				break;
			continue;
		}
		knix_Send(hand, opendata, sizeof(opendata));
		tcount++;
	}

	if (tcount == 5) {
		close(hand);
		return -1;
	}

	return hand;
}

