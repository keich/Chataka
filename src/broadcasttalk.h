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




#ifndef BROADCASTTALK_H_
#define BROADCASTTALK_H_

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <errno.h>

#ifdef __ANDROID__
#include <android/log.h>

#define APPNAME "BroadCastTalk"

#define LOGI( ... ) __android_log_print(ANDROID_LOG_INFO,APPNAME,__VA_ARGS__)
#define LOGE( ... ) __android_log_print(ANDROID_LOG_ERROR,APPNAME,__VA_ARGS__)
#define DEBUG( ... ) __android_log_print(ANDROID_LOG_INFO,APPNAME,__VA_ARGS__)

//#define LOGI( ... )
//#define LOGE( ... )
//#define DEBUG( ... )

#define  printf( ... )  __android_log_print(ANDROID_LOG_INFO,APPNAME,__VA_ARGS__)
//ANDROID_LOG_ERROR



#else
#define LOGI( ... ) printf(__VA_ARGS__)
#define LOGE( ... ) printf(__VA_ARGS__)
#define DEBUG( ... ) printf(__VA_ARGS__)

#endif










#define P_CountAudioBuf 5
#define P_CountFrameDataInPocket 3
#define P_FrameSize 33
#define P_RawFrameSize 160
#define P_DataInPocket P_CountFrameDataInPocket*P_FrameSize
#define P_HeaderSize 10
#define P_RawDataBuf P_RawFrameSize*P_CountFrameDataInPocket
#define P_SizePocket P_DataInPocket+P_HeaderSize


#define PORT 21900   //The port on which to listen

#define A_arrayUsersThreshold 30
#define A_arrayNewUsersThreshold 10
#define A_TimeOutUser 10

#define DELAY 20*P_CountFrameDataInPocket

#define UPDATEUSERNAME  10

typedef unsigned short P_audioBufRaw [P_RawDataBuf];
typedef unsigned short P_audioBuf [P_DataInPocket];


#ifdef __ANDROID__

#define USERSUPDATED 1


extern void sendMessage(int );
extern void jniAttachCurrentThread();
extern void jniDetachCurrentThread();

extern int StopSoundThread();
extern int StopMicThread();
#endif

#endif /* BROADCASTTALK_H_ */
