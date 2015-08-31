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


#include "broadcasttalk.h"
#include "maindata.h"

/*process data from socket*/
void onReceive(mainData_t* mainData,netPacket_t *inPacket, struct sockaddr_in * addr) {
	switch(inPacket->type)
	{
		case PACKET_PING:	{
			if(inPacket->count==PING_COUNT){
				if(updateTimeOutUser(mainData->usersArray,inPacket,addr)==-1){
					if(!isMyAddr(mainData->selfIP,addr)){
						addNewUser(mainData->usersArray,inPacket,addr);
					}
				}
			}
			break;
		}
		case PACKET_DATA:	{
			setAudioBuf(mainData->usersArray,inPacket,addr);
			break;
		}
	}
}

/* Start thread for receive data from UDP socket */
void * startThreadSocket(void * arg){
	LOGI("Enter: startThreadSocket\n");
	mainData_t* mainData = (mainData_t*)arg;
	struct sockaddr_in si;
	int recv_len;

	netPacket_t inPacket;

#ifdef __ANDROID__
	jniAttachCurrentThread();
#endif


	while(pthread_mutex_trylock(&mainData->threadSocketQuit)==EBUSY ){
		recv_len = recvFromUdpSocket(mainData->udpSocket,&inPacket,&si);
		if (recv_len >0){
			onReceive(mainData,&inPacket, &si);
		}

	}
#ifdef __ANDROID__
	jniDetachCurrentThread();
#endif
	LOGI("Exit: startThreadSocket: OK\n");
	return 0;
}




