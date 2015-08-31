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

#include "maindata.h"


mainData_t *createMainData(){
	LOGI("Enter: createMainData\n");
	mainData_t *mainData = malloc(sizeof(mainData_t));
	if(mainData == NULL){
		LOGE("Exit: createMainData: Can't get allocate memory for mainData\n");
		return NULL;
	}
	mainData->usersArray = createUserArray();
	if(mainData->usersArray== NULL){
		free(mainData);
		LOGE("Exit: createMainData: Can't create users array data structure\n");
		return NULL;
	}
	mainData->selfIP = createIPArray();
	if(mainData->selfIP == NULL){
		destroyUserArray(mainData->usersArray);
		free(mainData);
		LOGE("Exit: createMainData: Can't create IP array data structure\n");
		return NULL;
	}
	mainData->udpSocket = createUdpSocket();
	if(mainData->udpSocket == NULL){
		destroyUserArray(mainData->usersArray);
		destroyIPArray(mainData->selfIP);
		free(mainData);
		LOGE("Exit: createMainData: Can't create socket data structure\n");
		return NULL;
	}
	int rc = 0;
	rc+= pthread_mutex_init(&mainData->threadMainQuit,NULL);
	rc+= pthread_mutex_init(&mainData->threadSocketQuit,NULL);
	rc+= pthread_mutex_init(&mainData->threadOutaudioQuit,NULL);
	rc+= pthread_mutex_init(&mainData->threadInaudioQuit,NULL);
	rc+= pthread_mutex_lock(&mainData->threadMainQuit);
	rc+= pthread_mutex_lock(&mainData->threadSocketQuit);
	//rc+= pthread_mutex_lock(&mainData->threadOutaudioQuit);
	//rc+= pthread_mutex_lock(&mainData->threadInaudioQuit);
	if(rc!=0){
		destroyUserArray(mainData->usersArray);
		destroyIPArray(mainData->selfIP);
		destroyUdpSocket(mainData->udpSocket);
		free(mainData);
		LOGE("Exit: createMainData: Can't init pthread mutex structures\n");
		return NULL;
	}
	LOGI("Exit: createMainData: OK\n");
	return mainData;
}

void destroyMainData(mainData_t *mainData){
	LOGI("Enter: destroyMainData\n");
	if(mainData!=NULL){
		pthread_mutex_unlock(&mainData->threadMainQuit);
		pthread_mutex_unlock(&mainData->threadSocketQuit);
		pthread_join(mainData->threadmain,NULL);
		pthread_join(mainData->threadsocket,NULL);
		StopSoundThread();
		StopMicThread();
		destroyUdpSocket(mainData->udpSocket);
		destroyIPArray(mainData->selfIP);
		destroyUserArray(mainData->usersArray);
		mainData->udpSocket = NULL;
		mainData->selfIP = NULL;
		mainData->usersArray = NULL;
		free(mainData);
		mainData = NULL;
	}else{
		LOGE("Exit: destroyMainData: Main data structure not init\n");
	}
	LOGI("Exit: destroyMainData: OK\n");
}

