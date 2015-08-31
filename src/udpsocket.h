/*
 
The MIT License (MIT)

Copyright (c) 2015 Alexander Zazhigin mykeich@yandex.ru

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.

*/

#ifndef UDPSOCKET_H_
#define UDPSOCKET_H_

#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <semaphore.h>



#define VERSION 1
#define PACKET_PING 0
#define PACKET_DATA 1
#define PING_COUNT 2107483647


typedef struct {
	char type;
	char version;
	unsigned long long count;
	P_audioBuf Data;

} netPacket_t;

typedef struct {
	int s;
	sem_t LockSocket;
	int addrlen;
	int packetLen;
	//struct sockaddr_in destaddr;
	netPacket_t pingPacket;
} udpSocket_t;

extern udpSocket_t * createUdpSocket();
extern void destroyUdpSocket(udpSocket_t *);
extern int sendFromUdpSocket(udpSocket_t * ,netPacket_t *,struct sockaddr_in *);
extern int sendPingPacket(udpSocket_t *,struct sockaddr_in* addr );
extern int recvFromUdpSocket(udpSocket_t * ,netPacket_t *,struct sockaddr_in * );

extern void initPingPacket(netPacket_t * );
extern void initDataPacket(netPacket_t * );

extern void setUserNamePacket(udpSocket_t *,const char *);

#endif /* UDPSOCKET_H_ */
