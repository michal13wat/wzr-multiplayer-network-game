
// server.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#define _WINSOCK_DEPRECATED_NO_WARNINGS
#include <string.h>
#include <list>

#include <winsock2.h>
#include <ws2tcpip.h>
#include <stdlib.h> 

#ifdef WIN32
WSADATA localWSA; //***************** for Windows
#endif

int sock;                         // Socket 
struct sockaddr_in udpServAddr; // Local address 
struct sockaddr_in udpClntAddr; // Client address 

unsigned long udpIP;             // IP  address 
unsigned short udpPort;     // Server port 

std::list<unsigned long> ips;

short sSize;

void DieWithError(char *errorMessage)
{
	perror(errorMessage);
	//exit(1);
}

int send(char *buffer, char *servIP, unsigned short size)
{
	udpServAddr.sin_addr.s_addr = inet_addr(servIP);

	if (sendto(sock, buffer, size, 0, (struct sockaddr *)
		&udpServAddr, sizeof(udpServAddr)) == size) return size;
	else DieWithError("sendto() sent a different number of bytes than expected");
}

int send(char *buffer, unsigned long servIP, unsigned short size)
{
	udpServAddr.sin_addr.s_addr = servIP;

	if (sendto(sock, buffer, size, 0, (struct sockaddr *)
		&udpServAddr, sizeof(udpServAddr)) == size) return size;
	else DieWithError("sendto() sent a different number of bytes than expected");
}


int reciv(char *buffer, struct sockaddr_in clntAddr, unsigned short size)
{
	int ucleng = sizeof(clntAddr);

	if ((sSize = recvfrom(sock, buffer, size, 0,
		(struct sockaddr *) &clntAddr, (socklen_t*)&ucleng)) < 0)
		DieWithError("recvfrom() failed ");
	else
	{
		bool isIpSaved = false;
		unsigned long clientIp = clntAddr.sin_addr.s_addr;
		//printf("%d\n", clientIp);
		for each (unsigned long ip in ips)
		{
			if (ip == clientIp) {
				isIpSaved = true;
				break;
			}
		}
		if (!isIpSaved)
			ips.push_back(clientIp);
		return sSize;
	}
}

int _tmain(int argc, _TCHAR* argv[])
{
#ifdef WIN32
	if (WSAStartup(MAKEWORD(2, 2), &localWSA) != 0)
	{
		printf("WSAStartup error");
	}
#endif
	if ((sock = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP)) < 0)
		DieWithError("socket() failed");

	memset(&udpServAddr, 0, sizeof(udpServAddr));   /* Zero out structure */
	udpServAddr.sin_family = AF_INET;                /* Internet address family */
	udpServAddr.sin_addr.s_addr = htonl(INADDR_ANY); /* Any incoming interface */
	udpServAddr.sin_port = 10001;      /* Local port */

	if (bind(sock, (struct sockaddr *) &udpServAddr, sizeof(udpServAddr)) < 0)
		DieWithError("bind() failed");
	int i = 0;
	char buffer[1024];
	const int BUFFER_SIZE = 1024;
	while (true) {
		if (i == 10000) {
			for each(unsigned long ip in ips)
				printf("ip: %d\n", ip);
			printf("\n");
			i = 0;
		}
		else
			i++;
		reciv(buffer, udpClntAddr, BUFFER_SIZE);
		for each(unsigned long ip in ips)
			send(buffer, ip, BUFFER_SIZE);

	}
	return 0;
}
