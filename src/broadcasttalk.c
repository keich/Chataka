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



#ifdef __ANDROID__
#include <jni.h>
#endif

#include "broadcasttalk.h"
#include "threadsocket.h"
#include "threadoutaudio.h"
#include "threadinaudio.h"
#include "maindata.h"
#include "udpsocket.h"



// Main thread for BroadCastTalk
static mainData_t *StaticmainData = NULL;

//For callback java
#ifdef __ANDROID__
static JavaVM *java_vm;
jclass *clasSingletonBroadCastTalk;
#endif


#define THREADNOTRUN 0
#define THREAD_OK 1
#define MAINDATANOTINIT -1
#define MUTEXERROR -2
#define THREADCREATEERROR -3

/* Return THREAD_OK if Thread for play sound is running */
int GetStateSoundThread(){
	LOGI("Enter: GetStateSoundThread\n");
	if(StaticmainData!=NULL){
		if(pthread_mutex_trylock(&StaticmainData->threadOutaudioQuit)==EBUSY ){
			LOGI("Exit: GetStateSoundThread: THREAD_OK\n");
			return THREAD_OK;
		}else{
			pthread_mutex_unlock(&StaticmainData->threadOutaudioQuit);
			LOGE("Exit: GetStateSoundThread: THREADNOTRUN\n");
			return THREADNOTRUN;
		}
	}
	LOGE("Exit: GetStateSoundThread: MAINDATANOTINIT\n");
	return MAINDATANOTINIT;
}

/* Return THREAD_OK if Thread for recording sound is running */
int GetStateMicThread(){
	LOGI("Enter: GetStateMicThread");
	if(StaticmainData!=NULL){
		if(pthread_mutex_trylock(&StaticmainData->threadInaudioQuit)==EBUSY ){
			LOGI("Exit: GetStateMicThread: THREAD_OK\n");
			return THREAD_OK;
		}else{
			pthread_mutex_unlock(&StaticmainData->threadInaudioQuit);
			LOGE("Exit: GetStateMicThread: THREADNOTRUN\n");
			return THREADNOTRUN;
		}
	}
	LOGE("Exit: GetStateMicThread: MAINDATANOTINIT\n");
	return MAINDATANOTINIT;
}

/* Start play sound thread */
int StartSoundThread(int usespeaker){
	LOGI("Enter: StartSoundThread");
	if(StaticmainData!=NULL){
		StaticmainData->usespeaker = usespeaker;
		if(pthread_mutex_trylock(&StaticmainData->threadOutaudioQuit)==EBUSY ){
			LOGE("Exit: StartSoundThread: THREADNOTRUN\n");
			return THREADNOTRUN;
		}
		int ret = pthread_create( &StaticmainData->threadoutaudio, NULL, startThreadOutAudio, (void *)StaticmainData);
		if(ret){
			LOGE("Exit: StartSoundThread: THREADCREATEERROR\n");
	       	return THREADCREATEERROR;
		}
		LOGI("Exit: StartSoundThread: THREAD_OK\n");
		return THREAD_OK;
	}
	LOGE("Exit: StartSoundThread: MAINDATANOTINIT\n");
	return MAINDATANOTINIT;
}

/* Stop play sound thread */
int StopSoundThread(){
	LOGI("Enter: StopSoundThread");
	if(StaticmainData!=NULL){
		if(pthread_mutex_trylock(&StaticmainData->threadOutaudioQuit)==EBUSY ){
			int rc= pthread_mutex_unlock(&StaticmainData->threadOutaudioQuit);
			if(rc!=0){
				LOGE("Exit: StopSoundThread: MUTEXERROR\n");
				return MUTEXERROR;
			}
			pthread_join(StaticmainData->threadoutaudio,NULL);
			LOGI("Exit: StopSoundThread: THREAD_OK\n");
			return THREAD_OK;
		}else{
			pthread_mutex_unlock(&StaticmainData->threadOutaudioQuit);
			LOGE("Exit: StopSoundThread: THREADNOTRUN\n");
			return THREADNOTRUN;
		}
	}
	LOGE("Exit: StopSoundThread: MAINDATANOTINIT\n");
	return MAINDATANOTINIT;
}

/* Start recording sound thread */
int StartMicThread(){
	LOGI("Enter: StartMicThread");
	if(StaticmainData!=NULL){
		if(pthread_mutex_trylock(&StaticmainData->threadInaudioQuit)==EBUSY ){
			LOGE("Exit: StartMicThread: THREADNOTRUN\n");
			return THREADNOTRUN;
		}
		int ret = pthread_create( &StaticmainData->threadinaudio, NULL, startThreadInAudio, (void *)StaticmainData);
		if(ret){
			LOGE("Exit: StartMicThread: THREADCREATEERROR\n");
	       	return THREADCREATEERROR;
		}
		LOGI("Exit: StartMicThread: THREAD_OK\n");
		return THREAD_OK;
	}
	LOGE("Exit: StartMicThread: MAINDATANOTINIT\n");
	return MAINDATANOTINIT;
}

/* Stop recording sound thread */
int StopMicThread(){
	LOGI("Enter: StopMicThread");
	if(StaticmainData!=NULL){
		if(pthread_mutex_trylock(&StaticmainData->threadInaudioQuit)==EBUSY ){
			int rc= pthread_mutex_unlock(&StaticmainData->threadInaudioQuit);
			if(rc!=0){
				LOGE("Exit: StopMicThread: MUTEXERROR\n");
				return MUTEXERROR;
			}
			pthread_join(StaticmainData->threadinaudio,NULL);
			LOGI("Exit: StopMicThread: THREAD_OK\n");
			return THREAD_OK;
		}else{
			pthread_mutex_unlock(&StaticmainData->threadInaudioQuit);
			LOGE("Exit: StopMicThread: THREADNOTRUN\n");
			return THREADNOTRUN;
		}
	}
	LOGE("Exit: StopMicThread: MAINDATANOTINIT\n");
	return MAINDATANOTINIT;
}

/* Thread for main process */
void thread_main(void * arg) {
	LOGI("Enter: thread_main\n");
#ifdef __ANDROID__
	jniAttachCurrentThread();
#endif

	mainData_t *mainData = (mainData_t *)arg;

    int ret = pthread_create( &mainData->threadsocket, NULL, startThreadSocket, (void *)mainData);
    if(ret){
    	LOGE("Exit: thread_main: ThreadSocket is die\n");
    	return;
	}
    LOGI("thread_main: ThreadSocket is running\n");

#ifdef __ANDROID__
#else
    StartSoundThread(0);
    StartMicThread();
#endif
    struct sockaddr_in brdddr;
    getAllLocalIP(mainData->selfIP);
    int checkLoclIP = 0;
    LOGI("thread_main: ThreadMain is started\n");
	while(pthread_mutex_trylock(&mainData->threadMainQuit)==EBUSY ){
    	sleep(1);
    	getBrdAddr(mainData->selfIP,&brdddr);
    	sendPingPacket(mainData->udpSocket,&brdddr);
    	incrTimeOutAllOrDeleteUsers(mainData->usersArray);
    	registerNewUser(mainData->usersArray,mainData->selfIP);
    	printUseBuffForAll(mainData->usersArray);
    	if(checkLoclIP>9){
    		getAllLocalIP(mainData->selfIP);
    		checkLoclIP = 0;
    	}else{
    		checkLoclIP++;
    	}
    }
#ifdef __ANDROID__
	jniDetachCurrentThread();
#endif
	LOGI("Exit: thread_main: OK");
    return;
}

#ifdef __ANDROID__


/* Run main thread for android application */
JNIEXPORT jint JNICALL Java_ru_zazhigin_keich_broadcasttalk_SingletonBroadCastTalk_CreateBroadCastTalk (JNIEnv * env, jclass jc){
	LOGI("Enter: CreateBroadCastTalk\n");
	if(StaticmainData==NULL){
		StaticmainData = createMainData();
		if(StaticmainData != NULL){
			int ret = pthread_create( &StaticmainData->threadmain, NULL, (void *)thread_main,  StaticmainData);
			if(ret){
				StaticmainData = NULL;
				LOGE("Exit: CreateBroadCastTalk: ThreadMain is die\n");
				return -1;
			}
		}else{
			LOGE("Exit: CreateBroadCastTalk: Can't init internal data structure\n");
			return -2;
		}
	}else{
		LOGE("Exit: CreateBroadCastTalk: Main process already loaded\n");
		return -3;
	}
	LOGI("Exit: CreateBroadCastTalk: OK\n");
	return 0;
}

/* Stop main thread and free memory for android application */
JNIEXPORT jint JNICALL Java_ru_zazhigin_keich_broadcasttalk_SingletonBroadCastTalk_DestoryBroadCastTalk (JNIEnv * env, jclass jc){
	LOGI("Enter: DestoryBroadCastTalk\n");
	if(StaticmainData!=NULL){
		destroyMainData(StaticmainData);
		StaticmainData = NULL;
		LOGI("Exit: DestoryBroadCastTalk: OK \n");
		return 0;
	}
	LOGE("Exit: DestoryBroadCastTalk:  Internal structure data is not init. Nothing do.\n");
	return 1;
}

/* Return array of java class  for android application */
JNIEXPORT jobjectArray JNICALL Java_ru_zazhigin_keich_broadcasttalk_SingletonBroadCastTalk_GetUsersBCTUser (JNIEnv * env, jclass jc){
	LOGI("Enter: GetUsersBCTUser\n");
	if(StaticmainData!=NULL){
		  usersArray_t * usersArray = StaticmainData->usersArray;
		  sem_wait(&usersArray->LockUsersArray);
		  LOGI("GetUsersBCTUser: Create array for %i users\n",usersArray->lenghtArray);
		  if(usersArray->lenghtArray>0){
			  jclass userclass = (*env)->FindClass(env,"ru/zazhigin/keich/broadcasttalk/BCTUser");
			  jmethodID methodIduser = (*env)->GetMethodID(env,userclass, "<init>", "(Ljava/lang/String;Ljava/lang/String;I)V");
			  jobject obj = (*env)->NewObject(env,userclass, methodIduser,(*env)->NewStringUTF(env,""),(*env)->NewStringUTF(env,""),0);
			  jobjectArray ret;
			  ret= (jobjectArray)(*env)->NewObjectArray(env,usersArray->lenghtArray,userclass,obj);
			  int i;
			  for(i=0;i<usersArray->lenghtArray;i++) {
				  if(usersArray->users[i]!=NULL){
					  usersArray->users[i]->name[P_DataInPocket-1]=0;
						(*env)->SetObjectArrayElement(env,
								ret,
								i,
								(*env)->NewObject(env,
								userclass,
								methodIduser,
								(*env)->NewStringUTF(env,usersArray->users[i]->name),
								(*env)->NewStringUTF(env,inet_ntoa(usersArray->users[i]->addr.sin_addr)),
								htons(usersArray->users[i]->addr.sin_port))
								);
						 }
			  }
			  sem_post(&usersArray->LockUsersArray);
			  LOGI("Exit: GetUsersBCTUser: OK");
			  return ret;
		  }
		  sem_post(&usersArray->LockUsersArray);
	}
	LOGE("Exit: GetUsersBCTUser:  Internal structure data is not init\n");
	return NULL;
}

/* Set user name from android application */
JNIEXPORT jint JNICALL Java_ru_zazhigin_keich_broadcasttalk_SingletonBroadCastTalk_SetUserName(JNIEnv * env, jclass jc, jstring name){
	LOGI("Enter: SetUserName\n");
	if(StaticmainData!=NULL){
		const char *nname = (*env)->GetStringUTFChars(env,name, JNI_FALSE);
		setUserNamePacket(StaticmainData->udpSocket,nname);
		(*env)->ReleaseStringUTFChars(env,name, nname);
	}
	LOGI("Exit: SetUserName: OK\n");
	return 0;
}

/* Send message to android application. Callback. */
void sendMessage(int nummsg){
	LOGI("Enter: sendMessage\n");
    JNIEnv *env;
    if ((*java_vm)->GetEnv(java_vm,(void**) &env, JNI_VERSION_1_4) != JNI_OK) {
    	LOGE("Exit: sendMessage: Failed to get the environment using GetEnv()\n");
        return;
    }
    if(clasSingletonBroadCastTalk==NULL){
    	LOGE("Exit: sendMessage: Can't sendMessage to java. No class object\n");
        return;
    }
	jmethodID methodID = (*env)->GetStaticMethodID(env, clasSingletonBroadCastTalk, "onMessage", "(I)V" );
    if(methodID==NULL){
    	LOGE("Exit: sendMessage: Can't sendMessage to java. Can't get java method\n");
        return;
    }
	(*env)->CallStaticVoidMethod(env, clasSingletonBroadCastTalk,methodID,nummsg);
	LOGI("Exit: sendMessage: OK\n");
}

/* For load library from java */
jint JNI_OnLoad(JavaVM* vm, void* reserved){
	LOGI("Enter: JNI_OnLoad\n");
    java_vm = vm;
    JNIEnv *env;
    if ((*java_vm)->GetEnv(java_vm,(void**) &env, JNI_VERSION_1_4) != JNI_OK) {
    	LOGE("Exit: JNI_OnLoad: Failed to get the environment using GetEnv()\n");
        return -1;
    }
    jclass c = (*env)->FindClass(env,"ru/zazhigin/keich/broadcasttalk/SingletonBroadCastTalk");
    if(c==NULL){
    	LOGE("Exit: JNI_OnLoad: Failed to call FindClass\n");
    	return -1;
    }
    clasSingletonBroadCastTalk = (*env)->NewGlobalRef(env,c);
    if(clasSingletonBroadCastTalk==NULL){
    	LOGE("Exit: JNI_OnLoad: Failed to call NewGlobalRef\n");
    	return -1;
    }
    LOGI("Exit: JNI_OnLoad: OK\n");
    return JNI_VERSION_1_4;
}

/* For work with java vm*/
void jniAttachCurrentThread(){
	LOGI("Enter: jniAttachCurrentThread\n");
	JNIEnv *env;
	jint result = (*java_vm)->AttachCurrentThread(java_vm,&env, NULL);
	LOGI("Exit: jniAttachCurrentThread: Res: %i\n",result);
}

/* For work with java vm*/
void jniDetachCurrentThread(){
	LOGI("Enter: jniDetachCurrentThread\n");
	JNIEnv *env;
	jint result = (*java_vm)->DetachCurrentThread(java_vm);
	LOGI("Exit: jniDetachCurrentThread: Res: %i\n",result);
}


/* For android application */
JNIEXPORT jint JNICALL Java_ru_zazhigin_keich_broadcasttalk_SingletonBroadCastTalk_GetStateSoundThread(JNIEnv * env, jclass jc){
	return GetStateSoundThread();
}

/* For android application */
JNIEXPORT jint JNICALL Java_ru_zazhigin_keich_broadcasttalk_SingletonBroadCastTalk_GetStateMicThread(JNIEnv * env, jclass jc){
	return GetStateMicThread();
}

/* For android application */
JNIEXPORT jint JNICALL Java_ru_zazhigin_keich_broadcasttalk_SingletonBroadCastTalk_StartSoundThread(JNIEnv * env, jclass jc, jint usespeaker){
	return StartSoundThread(usespeaker);
}

/* For android application */
JNIEXPORT jint JNICALL Java_ru_zazhigin_keich_broadcasttalk_SingletonBroadCastTalk_StopSoundThread(JNIEnv * env, jclass jc){
	return StopSoundThread();
}

/* For android application */
JNIEXPORT jint JNICALL Java_ru_zazhigin_keich_broadcasttalk_SingletonBroadCastTalk_StartMicThread(JNIEnv * env, jclass jc){
	return StartMicThread();
}

/* For android application */
JNIEXPORT jint JNICALL Java_ru_zazhigin_keich_broadcasttalk_SingletonBroadCastTalk_StopMicThread(JNIEnv * env, jclass jc){
	return StopMicThread();
}


#else

/* Main application */
void main(void * arg){
	LOGI("Enter: main\n");
	StaticmainData = createMainData();
	if(StaticmainData != NULL){
		thread_main(StaticmainData);
	}else{
		LOGE("Exit: main: Can't create main structure data\n");
	}
	LOGI("Exit: main\n");
}
#endif
