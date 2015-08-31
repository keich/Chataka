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

#include "users.h"
#include "broadcasttalk.h"
#include "gsmcodec.h"
#include "tools.h"
#include "interfaceip.h"
//#include <sys/types.h>
//#include <sys/socket.h>
#include <arpa/inet.h>
#include <string.h>

/* Set new user name for user in index number*/
void UpdateUsersName(usersArray_t * usersArray,netPacket_t *inPacket ,int index){
	LOGI("Enter: UpdateUsersName\n");
	if(usersArray->users[index]->updateUserName<UPDATEUSERNAME){
		usersArray->users[index]->updateUserName++;
	}else{
		usersArray->users[index]->updateUserName = 0;
		memcpy(&usersArray->users[index]->name,&inPacket->Data,P_DataInPocket);
		usersArray->users[index]->name[P_DataInPocket-1]=0;
#ifdef __ANDROID__
		sendMessage(USERSUPDATED);
#endif
	}
	LOGI("Exit: UpdateUsersName: OK\n");
}

/* Free memory oof user*/
void destroytUser(userFormat_t * user){
	LOGI("Enter: destroytUser\n");
	if(user!=NULL){
		gsm_destroy(user->gsm);
		free(user);
	}
	LOGI("Exit: destroytUser: OK\n");
}

/* Init data structure for user*/
userFormat_t * createUser(newUserFormat_t * newUser){
	LOGI("Enter: createUser\n");
	userFormat_t * user = (userFormat_t *)malloc(sizeof(userFormat_t));
	if(user == NULL){
		LOGE("Exit: createUser: Can't get allocate memory for User\n");
		return NULL;
	}
	memset(user, 0, sizeof(userFormat_t));
	memcpy(&user->addr,&newUser->addr,sizeof(struct sockaddr_in));
	memcpy(&user->name,newUser->name,sizeof(user->name));
	user->gsm = gsm_create();
	user->blockBuf = 1;
	user->updateUserName = 0;
	LOGI("Exit: createUser: OK\n");
	return user;
}

/* free memory of users array*/
void destroyUserArray(usersArray_t * usersArray){
	LOGI("Enter: destroyUserArray\n");
	if(usersArray!=NULL){
		sem_wait(&usersArray->LockUsersArray);

		if(usersArray->lenghtArray!=0){
			int i;
			for(i=0;i<usersArray->lenghtArray;i++){
				destroytUser(usersArray->users[i]);
			}
			free(usersArray->users);
			usersArray->lenghtArray = 0;
			usersArray->users = NULL;
		}

		sem_post(&usersArray->LockUsersArray);

		sem_destroy (&usersArray->LockUsersArray);
		sem_destroy (&usersArray->LockNewUsersArray);

		free(usersArray);
	}else{
		LOGE("destroyUserArray: Array is NULL\n");
	}
	LOGI("Exit: destroyUserArray: OK\n");
}

/* Create user array and allocate memory */
usersArray_t * createUserArray(){
	LOGI("Enter: createUserArray\n");
	usersArray_t * usersArray = (usersArray_t *)malloc(sizeof(usersArray_t));
	if(usersArray == NULL){
		LOGE("Exit: createUserArray: Can't get allocate memory for usersArray\n");
		return NULL;
	}

	memset(&usersArray->newUsers,0,sizeof(usersArray->newUsers));
	usersArray->lenghtArray = 0;
	usersArray->users = NULL;

	sem_init(&usersArray->LockUsersArray, 0, 1);
	sem_init(&usersArray->LockNewUsersArray, 0, 1);
	LOGI("Exit: createUserArray: OK\n");
	return usersArray;
}

/* Find user in array by inet address
 * */
int getIndexOfUser(usersArray_t * UA,struct sockaddr_in * addr){
	int i;
	for(i=0;i<UA->lenghtArray;i++){
		if(UA->users[i]!=NULL){
			if(UA->users[i]->addr.sin_addr.s_addr==addr->sin_addr.s_addr){
				if(UA->users[i]->addr.sin_port==addr->sin_port){
					return i;
				}
			}
		}
	}
	return -1;
}

/*wchar_t * createUserName(char *buf){
	int zero = P_DataInPocket-1,i;
	wchar_t * str = (wchar_t*) buf;
	//for(i=0;i<P_DataInPocket;i++){
	//	if(buf[i]==0){
	//		zero = i;
	//		break;
	//	}
	//}
	char * ret = malloc(zero+1);
	if(ret==NULL){
		printf("Can't get user name from buffer. No memory. \n");
		return NULL;
	}
	memcpy(ret,buf,zero);
	//for(i=0;i<zero;i++){
	//	ret[i]=buf[i];
	//}
	ret[zero] = 0;
	return ret;
}*/

/* Delete user from array by index. Dynamic array */
void delUser(usersArray_t * usersArray, int index){
	LOGI("Enter: delUser\n");
	if((index>=0)&&(index<usersArray->lenghtArray)){
		LOGI("delUser: Delete User %s:%d\n", inet_ntoa(usersArray->users[index]->addr.sin_addr), ntohs(usersArray->users[index]->addr.sin_port));
		if(usersArray->lenghtArray==1){
			destroytUser(usersArray->users[0]);
			free(usersArray->users);
			usersArray->lenghtArray = 0;
			usersArray->users = NULL;
		}else{
			usersArray->lenghtArray--;
			userFormat_t ** tmpUF = (userFormat_t **)malloc(sizeof(userFormat_t *)*(usersArray->lenghtArray));
			if(tmpUF==NULL){
				LOGE("delUser: Can't change size users array. User don't deleted\n");
			}else{
				int s=0,i;
				for(i=0;i<usersArray->lenghtArray+1;i++){
					if(i!=index){
						tmpUF[i-s]=usersArray->users[i];
					}else{
						destroytUser(usersArray->users[i]);
						s=1;
					}
				}
				free(usersArray->users);
				usersArray->users = tmpUF;
			}
		}
	}else{
		LOGE("delUser: Can't delete user. Out of range index");
	}
	LOGI("Exit: delUser: OK\n");
}

/* Add user to users array */
void addUser(usersArray_t * usersArray,newUserFormat_t * newUser){
	LOGI("Enter: addUser\n");
	if(getIndexOfUser(usersArray,&newUser->addr)!=-1){
				LOGE("addUser: Can't add new user. He is in users array. User %s:%d\n", inet_ntoa(newUser->addr.sin_addr), ntohs(newUser->addr.sin_port));
	}else{
		if(usersArray->lenghtArray<A_arrayUsersThreshold){
			userFormat_t ** newarrayusers = (userFormat_t **)realloc(usersArray->users,sizeof(userFormat_t *)*(usersArray->lenghtArray+1));
			if(newarrayusers == NULL){
				LOGE("addUser: Can't add new user. No memory. User %s:%d\n", inet_ntoa(newUser->addr.sin_addr), ntohs(newUser->addr.sin_port));
				//free(Name);
			}else{
				usersArray->users = newarrayusers;
				usersArray->users[usersArray->lenghtArray] = createUser(newUser);
				usersArray->lenghtArray++;
				LOGI("addUser: Add new user. User %s:%d\n", inet_ntoa(newUser->addr.sin_addr), ntohs(newUser->addr.sin_port));
#ifdef __ANDROID__
				sendMessage(USERSUPDATED);
#endif
			}
		}else{
			LOGE("addUser: Can't add new user. User array is over. User %s:%d\n", inet_ntoa(newUser->addr.sin_addr), ntohs(newUser->addr.sin_port));
		}
	}
	LOGI("Exit: addUser: OK\n");
}


/* Update time out user to zero. User is alive*/
int updateTimeOutUser(usersArray_t * usersArray,netPacket_t *inPacket ,struct sockaddr_in * addr) {
	sem_wait(&usersArray->LockUsersArray);
	int i = getIndexOfUser(usersArray,addr);
	if(i!=-1){
		usersArray->users[i]->pingTimOut = 0;
		UpdateUsersName(usersArray,inPacket,i);
	}
	sem_post(&usersArray->LockUsersArray);
	return i;
}

/* Update time out user. If time out is big than remove user from array */
void incrTimeOutAllOrDeleteUsers(usersArray_t * usersArray) {
	sem_wait(&usersArray->LockUsersArray);
	int i;
	for(i=0;i<usersArray->lenghtArray;i++){
		if(usersArray->users[i]!=NULL){
			usersArray->users[i]->pingTimOut++;
		}
	}
	i=0;
	int t =0,upd = 0;
	while(i<usersArray->lenghtArray){
		if(usersArray->users[i]!=NULL){
			if(usersArray->users[i]->pingTimOut>A_TimeOutUser){
				delUser(usersArray,i);
				i=0;
				upd = 1;
			}else{
				i++;
			}
		}
		t++;
		if(t>usersArray->lenghtArray*2){
			break;
		}

	}
	sem_post(&usersArray->LockUsersArray);
	if(upd){
#ifdef __ANDROID__
		// Send message to java application
				sendMessage(USERSUPDATED);
#endif
	}
}


/* Packet is in buffer */
int isPacketInBuffers(userFormat_t *user,unsigned long long * count){
	int i;
	for(i=0;i<P_CountAudioBuf;i++){
		if(user->audioBuf[i].use==1){
			if(user->audioBuf[i].count == *count){
				return 1;
			}
		}
	}
	return 0;
}

/* Put audio data to the users buffer */
void setAudioBuf(usersArray_t * usersArray,netPacket_t *inPacket, struct sockaddr_in * addr){
	sem_wait(&usersArray->LockUsersArray);
	int index = getIndexOfUser(usersArray,addr);
	int i;
	if(index!=-1){
		if(inPacket->count>usersArray->users[index]->lastBufCount){
			if(!isPacketInBuffers(usersArray->users[index],&inPacket->count)){
				for(i=0;i<P_CountAudioBuf;i++){
					if(usersArray->users[index]->audioBuf[i].use==0){
						decode(usersArray->users[index]->gsm,(unsigned char *)&inPacket->Data,usersArray->users[index]->audioBuf[i].bufData);
						usersArray->users[index]->audioBuf[i].count = inPacket->count;
						usersArray->users[index]->audioBuf[i].use = 1;
						usersArray->users[index]->useBuf++;
						break;
					}
				}
			}
		}else{
			if((usersArray->users[index]->lastBufCount-inPacket->count)>10){
				usersArray->users[index]->lastBufCount = inPacket->count;
				int i;
				for(i=0;i<P_CountAudioBuf;i++){
					usersArray->users[index]->audioBuf[i].use =0;
				}
			}
		}
	}
	sem_post(&usersArray->LockUsersArray);
}

/* Return min count of audio data in user buffer*/
int getIndexOfMinCountBuf(userFormat_t * user){
	int i, minindex =-1;
	for(i=0;i<P_CountAudioBuf;i++){
		if(user->audioBuf[i].use!=0){
			if(minindex==-1){
				minindex = i;
			}else{
				if(user->audioBuf[i].count<user->audioBuf[minindex].count){
					minindex = i;
				}
			}
		}
	}
	return minindex;
}

/* Sum audio data to one for play*/
void sumAudio(unsigned short * in,unsigned short * out){
	int i;
	int len = P_RawDataBuf;
	for(i=0;i<len;i++){
		out[i]+=in[i];
	}
}

/* Return one buffer for play. It is sum all buffers of all users */
void getBufForAllUsers(usersArray_t * usersArray,unsigned short * out){
	//memset(out,0,P_RawDataBuf*2);
	//return;
	sem_wait(&usersArray->LockUsersArray);
	memset(out,0,P_RawDataBuf*2);
	int i,indexbuf;
	for(i=0;i<usersArray->lenghtArray;i++){
		if(usersArray->users[i]!=NULL){
			if(!usersArray->users[i]->blockBuf){
			//if(1){
				indexbuf = getIndexOfMinCountBuf(usersArray->users[i]);
				if(indexbuf!=-1){
					sumAudio(usersArray->users[i]->audioBuf[indexbuf].bufData,out);
					usersArray->users[i]->audioBuf[indexbuf].use = 0;
					usersArray->users[i]->lastBufCount = usersArray->users[i]->audioBuf[indexbuf].count;
					usersArray->users[i]->useBuf--;
				}else{
					usersArray->users[i]->blockBuf =1;
					LOGI("getBufForAllUsers: User %s:%d. Waiting for fill all buffers\n", inet_ntoa(usersArray->users[i]->addr.sin_addr), ntohs(usersArray->users[i]->addr.sin_port));
				}
			}else{
				if(usersArray->users[i]->useBuf>=P_CountAudioBuf-1){
					usersArray->users[i]->blockBuf = 0;
					LOGI("getBufForAllUsers: Waiting for fill is over\n");
				}
			}
		}
	}
	sem_post(&usersArray->LockUsersArray);
}

/* Debug print os state buffers for all users*/
void printUseBuffForAll(usersArray_t * usersArray){
	sem_wait(&usersArray->LockUsersArray);
	int i;
	for(i=0;i<usersArray->lenghtArray;i++){
		LOGI("printUseBuffForAll: User %s@%s:%d UseBuf: %i LastCount: %lli \n",
				usersArray->users[i]->name,
				inet_ntoa(usersArray->users[i]->addr.sin_addr),
				ntohs(usersArray->users[i]->addr.sin_port),
				usersArray->users[i]->useBuf,
				usersArray->users[i]->lastBufCount);
	}
	sem_post(&usersArray->LockUsersArray);
}



/* Return index user in new users array, if exist*/
int getIndexOfNewUsers(usersArray_t * UA,struct sockaddr_in * addr){
	LOGI("Enter: getIndexOfNewUsers\n");
	int i;
	for(i=0;i<A_arrayNewUsersThreshold;i++){
		if(UA->newUsers[i].Waiting){
			if(UA->newUsers[i].addr.sin_addr.s_addr==addr->sin_addr.s_addr){
				if(UA->newUsers[i].addr.sin_port==addr->sin_port){
					LOGI("Exit: getIndexOfNewUsers: index %i\n",i);
					return i;
				}
			}
		}
	}
	LOGI("Exit: getIndexOfNewUsers: Not found user in new users array\n");
	return -1;
}
void registerNewUser(usersArray_t * userArray,selfIP_t * selfIP){
	sem_wait(&userArray->LockNewUsersArray);
	int i,loadLocalIP = 1;
	for(i=0;i<A_arrayNewUsersThreshold;i++){
		if(userArray->newUsers[i].Waiting){
			if(loadLocalIP){
				loadLocalIP = 0;
				getAllLocalIP(selfIP);
			}
			if(!isMyAddr(selfIP,&userArray->newUsers[i].addr)){
				addUser(userArray,&userArray->newUsers[i]);
			}
			userArray->newUsers[i].Waiting = 0;
		}
	}
	sem_post(&userArray->LockNewUsersArray);
}

/* Add user from new users array to main users array */
void addNewUser(usersArray_t * usersArray,netPacket_t *inPacket ,struct sockaddr_in * addr){
	LOGI("Enter: addNewUser\n");
	sem_wait(&usersArray->LockNewUsersArray);
	if(getIndexOfNewUsers(usersArray,addr)!=-1){
		LOGI("addNewUser: User is in new users array. User %s:%d\n", inet_ntoa(addr->sin_addr), ntohs(addr->sin_port));
	}else{
		int i;
		for(i=0;i<A_arrayNewUsersThreshold;i++){
			if(!usersArray->newUsers[i].Waiting){
				memcpy(&usersArray->newUsers[i].addr ,addr,sizeof(struct sockaddr_in));
				usersArray->newUsers[i].Waiting = 1;
				memcpy(&usersArray->newUsers[i].name,&inPacket->Data,P_DataInPocket);
				usersArray->newUsers[i].name[P_DataInPocket-1]=0;
				LOGI("addNewUser: Add new user to new users array. User %s:%d\n", inet_ntoa(addr->sin_addr), ntohs(addr->sin_port));
				break;
			}
		}
	}
	sem_post(&usersArray->LockNewUsersArray);
	LOGI("Exit: addNewUser: OK\n");
}

