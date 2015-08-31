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

#ifndef USERS_H_
#define USERS_H_

#include "broadcasttalk.h"
#include <semaphore.h>
#include "interfaceip.h"
#include "gsm/gsm.h"
#include "udpsocket.h"
#include <wchar.h>

typedef struct {
	struct sockaddr_in addr;
	char  name[P_DataInPocket];
	int Waiting;
} newUserFormat_t;

typedef struct {
	//P_audioBufRaw bufData;
	unsigned short bufData[P_RawDataBuf];
	unsigned long long count;
	int use;
} audioBufFormat_t;

typedef struct {
	struct sockaddr_in addr;
	char  name[P_DataInPocket];
	//unsigned long long count;
	int pingTimOut;
	audioBufFormat_t audioBuf[P_CountAudioBuf];
	//int useBuf;
	unsigned long long lastBufCount;
	//int audioBufFlag;
	gsm gsm;
	int useBuf;
	int blockBuf;
	int updateUserName;
} userFormat_t;



typedef struct {
	userFormat_t ** users;
	newUserFormat_t newUsers[A_arrayNewUsersThreshold];
	int lenghtArray;
	sem_t LockUsersArray;
	sem_t LockNewUsersArray;
} usersArray_t;

extern usersArray_t* createUserArray();
extern void destroyUserArray(usersArray_t*);
extern int updateTimeOutUser(usersArray_t*, netPacket_t *,struct sockaddr_in *);
extern void incrTimeOutAllOrDeleteUsers(usersArray_t*);
extern void registerNewUser(usersArray_t * ,selfIP_t * );
extern void addNewUser(usersArray_t * ,netPacket_t * ,struct sockaddr_in * );
//extern void addUser(usersArray_t * ,char * , struct sockaddr_in * );
extern void printUseBuffForAll(usersArray_t* );
extern void setAudioBuf(usersArray_t * ,netPacket_t *, struct sockaddr_in * );
extern void getBufForAllUsers(usersArray_t* ,unsigned short * );


#endif /* USERS_H_ */
