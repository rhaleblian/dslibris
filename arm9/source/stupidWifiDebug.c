
#include <nds.h>

#include <dswifi9.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>

#include "stupidWifiDebug.h"


// server port
#define SERVER_PORT 8888

// server name
#define SERVER_NAME "192.168.1.7"


static void stupidWifiDebugSend(u8 *buffer, s32 size) {

	struct sockaddr_in networkServerAddress;
	struct hostent *serverInfo;
	s32 networkServerSocketFD;
	s32 packetSize;

	if ((serverInfo = gethostbyname(SERVER_NAME)) == NULL)
		return;

	// create a socket
	if ((networkServerSocketFD = socket(AF_INET, SOCK_DGRAM, 0)) == -1)
		return;

	networkServerAddress.sin_family = AF_INET;          // host byte order
	networkServerAddress.sin_port = htons(SERVER_PORT); // short, network byte order
	networkServerAddress.sin_addr.s_addr = *((unsigned long *)(serverInfo->h_addr_list[0]));

	if ((packetSize = sendto(networkServerSocketFD, buffer, size, 0,
													 (struct sockaddr *)&networkServerAddress, sizeof(struct sockaddr))) == -1) {
		//fprintf(stderr, "MAIN: Error sending a message.\n");
	}

	shutdown(networkServerSocketFD, 0);
	close(networkServerSocketFD);
}


void stupidWifiDebugSendString(u8 *string, s32 size) {

	u8 buffer[256];
	s32 i;

	// clip the data
	if (size > 255)
		size = 255;

	// construct the message
	buffer[0] = 'S';
	for (i = 0; i < size; i++)
		buffer[i+1] = string[i];

	stupidWifiDebugSend(buffer, size + 1);
}


void stupidWifiDebugSendInt(s32 value) {

	u8 buffer[5];

	// construct the message
	buffer[0] = 'I';
	buffer[1] = (value >> 24) & 0xFF;
	buffer[2] = (value >> 16) & 0xFF;
	buffer[3] = (value >> 8) & 0xFF;
	buffer[4] = (value >> 0) & 0xFF;

	stupidWifiDebugSend(buffer, 5);
}
