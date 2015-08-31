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

#include <sys/types.h>
#include <sys/socket.h>
#include "broadcasttalk.h"
#include "udpsocket.h"
#include "string.h"
#include <wchar.h>




void initPacket(netPacket_t * pingPacket){
	pingPacket->version = VERSION;
}
void setUserNamePacket(udpSocket_t * udpSocket,const char *name){
	LOGI("Enter: setUserNamePacket\n");
	sem_wait(&udpSocket->LockSocket);
	int len = strlen(name);
	if(len>P_DataInPocket-2){
		len = P_DataInPocket-2;
	}
	memcpy((char *)udpSocket->pingPacket.Data,name,len+1);
	udpSocket->pingPacket.Data[len+1] = 0;
	sem_post(&udpSocket->LockSocket);
	LOGI("Exit: setUserNamePacket: OK\n");

}

/* Init structure for ing packet*/
void initPingPacket(netPacket_t * pingPacket){
	LOGI("Enter: initPingPacket\n");
	initPacket(pingPacket);
	pingPacket->type = PACKET_PING;
	pingPacket->count = PING_COUNT;
	memset(&pingPacket->Data,0,sizeof(pingPacket->Data));
	LOGI("Exit: initPingPacket: OK\n");
}

/* Init data structure for packet*/
void initDataPacket(netPacket_t * pingPacket){
	LOGI("Enter: initDataPacket\n");
	initPacket(pingPacket);
	pingPacket->type = PACKET_DATA;
	pingPacket->count = 0;
	LOGI("Exit: initDataPacket: OK\n");
}

/* Create UDP socket and another data*/
udpSocket_t * createUdpSocket(){
	LOGI("Enter: createUdpSocket\n");
	udpSocket_t * udpSocket = malloc(sizeof(udpSocket_t));

	sem_init(&udpSocket->LockSocket, 0, 1);

	if(udpSocket == NULL){
		LOGE("Exit: createUdpSocket: Can't get allocate memory for udpSocket\n");
		return NULL;
	}
	//open socket
	if((udpSocket->s = socket(AF_INET, SOCK_DGRAM, 0)) == -1){
		LOGE("Exit: createUdpSocket: Can't open socket\n");
		free(udpSocket);
		return NULL;
	}

	struct sockaddr_in si_me;
	memset((char *) &si_me, 0, sizeof(si_me));

	si_me.sin_family = AF_INET;
	si_me.sin_port = htons(PORT);
	si_me.sin_addr.s_addr = htonl(INADDR_ANY);
	//bind socket to port
	if( bind(udpSocket->s , (struct sockaddr*)&si_me, sizeof(si_me) ) == -1)
	{
		LOGE("Exit: createUdpSocket: Can't bind port %i  \n",PORT);
		close(udpSocket->s);
		free(udpSocket);
		return NULL;
	}

	int Enable=1;
	if(setsockopt(udpSocket->s , SOL_SOCKET, SO_BROADCAST, &Enable, sizeof(Enable))==-1){
		LOGE("Exit: createUdpSocket: Can't set broadcast option\n");
		close(udpSocket->s);
		free(udpSocket);
		return NULL;
	}
	struct timeval tv;
	tv.tv_sec = 0;
	tv.tv_usec = 100000;
	if (setsockopt(udpSocket->s, SOL_SOCKET, SO_RCVTIMEO,&tv,sizeof(tv)) < 0) {
		LOGE("Exit: createUdpSocket: Can't set socket recv timeout");
		close(udpSocket->s);
		free(udpSocket);
		return NULL;
	}
	memset((char *) &si_me, 0, sizeof(si_me));

	//udpSocket->destaddr.sin_family = AF_INET;
	//udpSocket->destaddr.sin_port = htons(PORT);
	//udpSocket->destaddr.sin_addr.s_addr = INADDR_BROADCAST;

	udpSocket->addrlen = sizeof(struct sockaddr_in);
	udpSocket->packetLen = sizeof(netPacket_t);

	initPingPacket(&udpSocket->pingPacket);
	const  char * str = "NoName";
	setUserNamePacket(udpSocket,str);


	LOGI("Exit: createUdpSocket: OK\n");
	return udpSocket;
}

/* free memory of data structure*/
void destroyUdpSocket(udpSocket_t * udpSocket){
	LOGI("Enter: destroyUdpSocket\n");
	sem_wait(&udpSocket->LockSocket);
	close(udpSocket->s);
	sem_post(&udpSocket->LockSocket);
	sem_destroy (&udpSocket->LockSocket);
	free(udpSocket);
	LOGI("Exit: destroyUdpSocket: OK\n");
}

/* Send packet to addr */
int sendFromUdpSocket(udpSocket_t * udpSocket,netPacket_t *packet,struct sockaddr_in* addr){
	sem_wait(&udpSocket->LockSocket);
	int err = sendto(udpSocket->s,(void *)packet,udpSocket->packetLen ,0,addr, udpSocket->addrlen);
	sem_post(&udpSocket->LockSocket);
	return err;
}

/* Send ping packet to addr*/
int sendPingPacket(udpSocket_t * udpSocket,struct sockaddr_in* addr){
	if(addr->sin_addr.s_addr!=0){
		sem_wait(&udpSocket->LockSocket);
		int err = sendto(udpSocket->s,(void *)&udpSocket->pingPacket,udpSocket->packetLen -1,0, addr, udpSocket->addrlen);
		sem_post(&udpSocket->LockSocket);
		return err;
	}
	return 0;
}

/* recv data ffrom socket */
int recvFromUdpSocket(udpSocket_t * udpSocket,netPacket_t *packet,struct sockaddr_in * si){
	int err = recvfrom(udpSocket->s, packet, udpSocket->packetLen , 0, (struct sockaddr *)si, &udpSocket->addrlen);
	return err;
}
