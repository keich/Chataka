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
#ifdef __ANDROID__
#include <SLES/OpenSLES.h>
#include <SLES/OpenSLES_Android.h>
#else
#include <alsa/asoundlib.h>
#include <alsa/control.h>
#include <time.h>
#include "tools.h"
#include "gsmcodec.h"
#endif


#define NUMBER_BUFFERS 3

#ifdef __ANDROID__




typedef struct {
	mainData_t *mainData;
	P_audioBufRaw Buffers[NUMBER_BUFFERS];
	int read;
	int write;
	netPacket_t outPacket;
	struct sockaddr_in destaddr;
	long long int count;
	gsm gsm;
} callbackDataIn_t;


/* Create structure for callback method*/
callbackDataIn_t * cretaeCalbackData(mainData_t *mainData){
	LOGI("Enter: cretaeCalbackData\n");
	callbackDataIn_t * Data = malloc(sizeof(callbackDataIn_t));
	if(Data==NULL){
		LOGE("Enter: cretaeCalbackData: Can't create callbackData. No memory\n");
		return NULL;
	}
	Data->mainData = mainData;
	Data->read = 0;
	Data->write = 0;
	Data->count = 0;
	Data->gsm = gsm_create();
	initDataPacket(&Data->outPacket);
	LOGI("Exit: cretaeCalbackData: OK\n");
	return Data;
}

/* Destroy structure for callback method */
void destroycallbackData(callbackDataIn_t *Data){
	LOGI("Enter: destroycallbackData\n");
	if(Data!=NULL){
		gsm_destroy(Data->gsm);
		free(Data);
	}else{
		LOGE("Exit: destroycallbackData: Callback data structure not init\n");
	}
	LOGI("Exit: destroycallbackData: OK\n");
}

/* Next number of buffer*/
inline int nextBuff(int now){
	now++;
	if(now==NUMBER_BUFFERS){
		now=0;
	}
	return now;
}

/* Before number for buffer*/
inline int beforeBuff(int now){
	if(now==0){
		now=NUMBER_BUFFERS-1;
	}else{
		now--;
	}
	return now;
}

/* Callback method for record audio */
void RecordEventCallback(SLAndroidSimpleBufferQueueItf bq, void *pContext){
	int err;
	callbackDataIn_t * Data = (callbackDataIn_t *)pContext;
	mainData_t * mainData = Data->mainData;
	(*bq)->Enqueue(bq, (void*) &Data->Buffers[Data->write], 2*P_RawDataBuf);
	encode(Data->gsm,&Data->Buffers[Data->read],(unsigned char *)&Data->outPacket.Data);
	Data->read = nextBuff(Data->read);
	int i;
	for(i=0;i<Data->mainData->usersArray->lenghtArray;i++){
		if(Data->mainData->usersArray->users[i]!=NULL){
			sendFromUdpSocket(mainData->udpSocket,&Data->outPacket,&Data->mainData->usersArray->users[i]->addr);
		}
	}
	Data->outPacket.count++;
	Data->write = nextBuff(Data->write);
}

/* Thread for record audio */
SLresult openSLRecOpen(mainData_t* mainData , SLObjectItf sl){
	LOGI("Enter: openSLRecOpen\n");
	SLresult result;
	SLEngineItf EngineItf;
	callbackDataIn_t * Data =  cretaeCalbackData(mainData);
	if(Data!=NULL){
		result = (*sl)->GetInterface(sl, SL_IID_ENGINE, (void*)&EngineItf);
		if (SL_RESULT_SUCCESS != result){
			LOGE("openSLRecOpen: Can't get interface OpenSL engine\n");
		}else{
			SLDataLocator_IODevice locator_mic;
			locator_mic.locatorType = SL_DATALOCATOR_IODEVICE;
			locator_mic.deviceType = SL_IODEVICE_AUDIOINPUT;
			locator_mic.deviceID = SL_DEFAULTDEVICEID_AUDIOINPUT;
			locator_mic.device 	= NULL;

			SLDataSource audioSrc;
			audioSrc.pLocator = (void *)&locator_mic;
			audioSrc.pFormat = NULL;


			// Setup the data source structure for the buffer queue
			SLDataLocator_AndroidSimpleBufferQueue loc_bq;
			loc_bq.locatorType = SL_DATALOCATOR_ANDROIDSIMPLEBUFFERQUEUE;
			loc_bq.numBuffers = NUMBER_BUFFERS;

			// Setup the format of the content in the buffer queue
			SLDataFormat_PCM format_pcm;
			format_pcm.formatType = SL_DATAFORMAT_PCM;
			format_pcm.numChannels = 1;
			format_pcm.samplesPerSec = SL_SAMPLINGRATE_8;
			format_pcm.bitsPerSample = SL_PCMSAMPLEFORMAT_FIXED_16;
			format_pcm.containerSize = 16;
			format_pcm.channelMask = SL_SPEAKER_FRONT_CENTER;
			format_pcm.endianness = SL_BYTEORDER_LITTLEENDIAN;

			SLDataSink audioSnk;
			audioSnk.pLocator = (void *)&loc_bq;
			audioSnk.pFormat = (void *)&format_pcm;

			// create audio recorder
			// (requires the RECORD_AUDIO permission)
			const SLInterfaceID id[1] = {SL_IID_ANDROIDSIMPLEBUFFERQUEUE};
			const SLboolean req[1] = {SL_BOOLEAN_TRUE};
			SLObjectItf recorderObject;
			result = (*EngineItf)->CreateAudioRecorder(EngineItf, &recorderObject, &audioSrc, &audioSnk, 1, id, req);
			if (SL_RESULT_SUCCESS != result) {
				LOGE("openSLRecOpen: Can't create OpenSL AudioRecorder\n");
			}else{
				// realize the audio recorder
				result = (*recorderObject)->Realize(recorderObject, SL_BOOLEAN_FALSE);
				if (SL_RESULT_SUCCESS != result){
					LOGE("openSLRecOpen: Can't Realize OpenSL AudioRecorder\n");
				}else{
					// get the record interface
					SLRecordItf recorderRecord;
					result = (*recorderObject)->GetInterface(recorderObject, SL_IID_RECORD, &recorderRecord);
					if (SL_RESULT_SUCCESS != result){
						LOGE("openSLRecOpen: Can't Create interface OpenSL AudioRecorder\n");
					}else{
						// get the buffer queue interface
						SLAndroidSimpleBufferQueueItf recorderBufferQueue;
						result = (*recorderObject)->GetInterface(recorderObject, SL_IID_ANDROIDSIMPLEBUFFERQUEUE,
											&recorderBufferQueue);
						if (SL_RESULT_SUCCESS != result){
							LOGE("openSLRecOpen: Can't Create interface OpenSL Buffer\n");
						}else{
							// register callback on the buffer queue
							result = (*recorderBufferQueue)->RegisterCallback(recorderBufferQueue, RecordEventCallback, (void *)Data);

							if (SL_RESULT_SUCCESS != result){
								LOGE("openSLRecOpen: Can't register OpenSL Callback function\n");
							}else{
								int i,buffok = 1;
								for(i=0;i<NUMBER_BUFFERS-1;i++){
									result = (*recorderBufferQueue)->Enqueue(recorderBufferQueue, (void*) Data->Buffers[Data->write],	 2*P_RawDataBuf);
									if (SL_RESULT_SUCCESS != result){
										LOGE("openSLRecOpen: Can't add buffers to OpenSL queue\n");
										buffok = 0;
										break;
									}
									Data->write = nextBuff(Data->write);
								}
								if(buffok){
									result = (*recorderRecord)->SetRecordState(recorderRecord, SL_RECORDSTATE_RECORDING);
									if (SL_RESULT_SUCCESS != result){
										LOGE("openSLRecOpen: Can't start record OpenSL\n");
									}else{
										while(pthread_mutex_trylock(&Data->mainData->threadInaudioQuit)==EBUSY )
										{
											sleep(1);

										}
										pthread_mutex_unlock(&mainData->threadInaudioQuit);
										result = (*recorderRecord)->SetRecordState(recorderRecord, SL_RECORDSTATE_STOPPED);
										LOGI("openSLRecOpen: Stop recording audio. Res: %d\n",result);
									}


								}
							}


						}
						(*recorderObject)->Destroy(recorderObject);
						LOGI("openSLRecOpen: Destroy recorderObject\n");
					}


				}


			}


		}
		destroycallbackData(Data);
		LOGI("Exit: openSLRecOpen: Destroy Data structure\n");
	}else{
		LOGE("Exit: openSLRecOpen: Data structure is NULL\n");
	}

	LOGI("Exit: openSLRecOpen: OK\n");
	return result;
}







/* Start thread for capture audio data */
void * startThreadInAudio(void * arg){
	LOGI("Enter: startThreadInAudio\n");
	mainData_t* mainData = (mainData_t*)arg;
	SLresult res;
	SLObjectItf sl;
	/* Create OpenSL ES engine in thread-safe mode */
	SLEngineOption EngineOption[] = {(SLuint32) SL_ENGINEOPTION_THREADSAFE, (SLuint32) SL_BOOLEAN_TRUE};
	res = slCreateEngine( &sl, 1, EngineOption, 0, NULL, NULL);
	if(res != SL_RESULT_SUCCESS){
		LOGE("Exit: startThreadInAudio: Can't create OpenSL engine\n");
		return 0;
	}
	/* Realizing the SL Engine in synchronous mode. */
	res = (*sl)->Realize(sl, SL_BOOLEAN_FALSE);
	if(res != SL_RESULT_SUCCESS){
		LOGE("Exit: startThreadInAudio: Can't Realize OpenSL engine\n");
		/* Shutdown OpenSL ES */
		(*sl)->Destroy(sl);
		return 0;
	}
	openSLRecOpen(mainData,sl);

	/* Shutdown OpenSL ES */
	(*sl)->Destroy(sl);
	LOGI("Exit: startThreadInAudio: OK\n");
	return 0;
}

#else

void * startThreadInAudio(void * arg){
	mainData_t* mainData = (mainData_t*)arg;
	int err;

	snd_pcm_t *capture_handle;
	snd_pcm_hw_params_t *hw_params;

	if ((err = snd_pcm_open (&capture_handle, "default", SND_PCM_STREAM_CAPTURE, 0)) < 0) {
		fprintf (stderr, "cannot open audio device %s (%s)\n",
				"default",
			 snd_strerror (err));
		return NULL;
	}

	if ((err = snd_pcm_hw_params_malloc (&hw_params)) < 0) {
		fprintf (stderr, "cannot allocate hardware parameter structure (%s)\n",
			 snd_strerror (err));
		snd_pcm_close (capture_handle);
		return NULL;
	}

	if ((err = snd_pcm_hw_params_any (capture_handle, hw_params)) < 0) {
		fprintf (stderr, "cannot initialize hardware parameter structure (%s)\n",
			 snd_strerror (err));
		snd_pcm_hw_params_free (hw_params);
		snd_pcm_close (capture_handle);
		return NULL;
	}

	if ((err = snd_pcm_hw_params_set_access (capture_handle, hw_params, SND_PCM_ACCESS_RW_INTERLEAVED)) < 0) {
		fprintf (stderr, "cannot set access type (%s)\n",
			 snd_strerror (err));
		snd_pcm_hw_params_free (hw_params);
		snd_pcm_close (capture_handle);
		return NULL;
	}

	if ((err = snd_pcm_hw_params_set_format (capture_handle, hw_params, SND_PCM_FORMAT_S16_LE)) < 0) {
		fprintf (stderr, "cannot set sample format (%s)\n",
			 snd_strerror (err));
		snd_pcm_hw_params_free (hw_params);
		snd_pcm_close (capture_handle);
		return NULL;
	}
	unsigned int rate = 8000;
	if ((err = snd_pcm_hw_params_set_rate_near (capture_handle, hw_params, &rate, 0)) < 0) {
		fprintf (stderr, "cannot set sample rate (%s)\n",
			 snd_strerror (err));
		snd_pcm_hw_params_free (hw_params);
		snd_pcm_close (capture_handle);
		return NULL;
	}

	if ((err = snd_pcm_hw_params_set_channels (capture_handle, hw_params, 1)) < 0) {
		fprintf (stderr, "cannot set channel count (%s)\n",
			 snd_strerror (err));
		snd_pcm_hw_params_free (hw_params);
		snd_pcm_close (capture_handle);
		return NULL;
	}
	snd_pcm_uframes_t buffer_size = 160*10;
	if((err=snd_pcm_hw_params_set_buffer_size_near (capture_handle, hw_params, &buffer_size))<0){
		fprintf (stderr, "cannot set parameters (%s)\n",
			 snd_strerror (err));
		snd_pcm_hw_params_free (hw_params);
		snd_pcm_close (capture_handle);
		return NULL;
	}
	if ((err = snd_pcm_hw_params (capture_handle, hw_params)) < 0) {
		fprintf (stderr, "cannot set parameters (%s)\n",
			 snd_strerror (err));
		snd_pcm_hw_params_free (hw_params);
		snd_pcm_close (capture_handle);
		return NULL;
	}

	snd_pcm_hw_params_free (hw_params);

	if ((err = snd_pcm_prepare (capture_handle)) < 0) {
		fprintf (stderr, "cannot prepare audio interface for use (%s)\n",
			 snd_strerror (err));
		snd_pcm_close (capture_handle);
		return NULL;
	}

	P_audioBufRaw outBuffer;
	netPacket_t outPacket;
	initDataPacket(&outPacket);
	gsm gsm = gsm_create();
	while(pthread_mutex_trylock(&mainData->threadInaudioQuit)==EBUSY ){
		err = snd_pcm_readi(capture_handle, &outBuffer, P_RawDataBuf);
		if (err < 0){
			printf("snd_pcm_readi error: %s\n", snd_strerror(err));
			err = snd_pcm_recover(capture_handle, err, 0);
		}
		if (err < 0) {
			printf("snd_pcm_readi failed: %s\n", snd_strerror(err));
			break;
		}
		encode(gsm,(unsigned short *)&outBuffer,(unsigned char *)&outPacket.Data);
		int i;
		for(i=0;i<mainData->usersArray->lenghtArray;i++){
			if(mainData->usersArray->users[i]!=NULL){
				sendFromUdpSocket(mainData->udpSocket,&outPacket,&mainData->usersArray->users[i]->addr);
			}
		}
		outPacket.count++;
	}
	pthread_mutex_unlock(&mainData->threadInaudioQuit);
	snd_pcm_close (capture_handle);
	gsm_destroy(gsm);
	return NULL;
}
#endif
