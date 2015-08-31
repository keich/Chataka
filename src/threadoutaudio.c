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
#include <SLES/OpenSLES.h>
#include <SLES/OpenSLES_Android.h>
#else
#include <alsa/asoundlib.h>
#include <alsa/control.h>
#include <sys/time.h>
#endif
#include "broadcasttalk.h"
#include "maindata.h"


#ifdef __ANDROID__

#define NUMBER_BUFFERS 3








typedef struct {
	mainData_t *mainData;
	P_audioBufRaw Buffers[NUMBER_BUFFERS];
	int read;
	//int write;
} callbackDataOut_t;

void initCallbackDataOut(mainData_t *mainData,callbackDataOut_t *Data){
	memset(Data,0,sizeof(callbackDataOut_t));
	Data->mainData = mainData;

}

inline int nextBuffOut(int now){// макросом сделать?
	now++;
	if(now==NUMBER_BUFFERS){
		now=0;
	}
	return now;
}
inline int beforeBuffOut(int now){
	if(now==0){
		now=NUMBER_BUFFERS-1;
	}else{
		now--;
	}
	return now;
}



/* Callback for Buffer Queue events */
void BufferQueueCallback(SLAndroidSimpleBufferQueueItf queueItf,	void *pContext)	{
	SLresult res;
	callbackDataOut_t *Data = (callbackDataOut_t*)pContext;
	res = (*queueItf)->Enqueue(queueItf, (void*) Data->Buffers[Data->read],	2 * P_RawDataBuf); // Size given in bytes.
	//DEBUG("res %i \n",res);
	Data->read = nextBuffOut(Data->read);
	//memset(Data->Buffers[Data->read],0,2 * P_RawDataBuf);
	getBufForAllUsers(Data->mainData->usersArray,Data->Buffers[Data->read]);

}





/* Play some music from a buffer queue */
void openSLPlayOpen(mainData_t * mainData ,SLObjectItf sl )	{
	LOGI("Enter: openSLPlayOpen\n");

	//Structure for application data.
	callbackDataOut_t Data;
	initCallbackDataOut(mainData,&Data);

	SLresult result;

	/* Get the SL Engine Interface which is implicit */
	SLEngineItf EngineItf;
	result = (*sl)->GetInterface(sl, SL_IID_ENGINE, (void*)&EngineItf);
	if (SL_RESULT_SUCCESS != result){
		LOGE("openSLPlayOpen: Can't get interface OpenSL engine\n");
	}else{
		// Set arrays required[] and iidArray[] for VOLUME interface
		const SLInterfaceID iidArray[1] = {SL_IID_VOLUME};
		const SLboolean required[1] = {SL_BOOLEAN_FALSE};

		// Create Output Mix object to be used by player
		SLObjectItf OutputMix;
		result = (*EngineItf)->CreateOutputMix(EngineItf, &OutputMix, 1, iidArray, required);
		if (SL_RESULT_SUCCESS != result){
			LOGE("openSLPlayOpen: Can't create OpenSL OutputMix\n");
		}else{

			// Realizing the Output Mix object in synchronous mode.
			result = (*OutputMix)->Realize(OutputMix, SL_BOOLEAN_FALSE);
			if (SL_RESULT_SUCCESS != result){
				LOGE("openSLPlayOpen: Can't Realize OpenSL OutputMix\n");
			}else{

				/* Setup the data source structure for the buffer queue */
				SLDataLocator_AndroidSimpleBufferQueue	bufferQueue;
				bufferQueue.locatorType = SL_DATALOCATOR_ANDROIDSIMPLEBUFFERQUEUE;
				bufferQueue.numBuffers = NUMBER_BUFFERS; /* Four buffers in our buffer queue */

				/* Setup the format of the content in the buffer queue */
				SLDataFormat_PCM	pcm;
				pcm.formatType = SL_DATAFORMAT_PCM;
				pcm.numChannels = 1;
				pcm.samplesPerSec = SL_SAMPLINGRATE_8;
				pcm.bitsPerSample = SL_PCMSAMPLEFORMAT_FIXED_16;
				pcm.containerSize = 16;
				pcm.channelMask = SL_SPEAKER_FRONT_CENTER;
				pcm.endianness = SL_BYTEORDER_LITTLEENDIAN;

				SLDataSource	audioSource;
				audioSource.pFormat = (void *)&pcm;
				audioSource.pLocator = (void *)&bufferQueue;
				/* Setup the data sink structure */
				SLDataLocator_OutputMix locator_outputmix;
				locator_outputmix.locatorType = SL_DATALOCATOR_OUTPUTMIX;
				locator_outputmix.outputMix = OutputMix;

				SLDataSink audioSink;
				audioSink.pLocator = (void *)&locator_outputmix;
				audioSink.pFormat = NULL;

				const SLInterfaceID iidArray1[2] = {SL_IID_ANDROIDCONFIGURATION,SL_IID_ANDROIDSIMPLEBUFFERQUEUE};
				const SLboolean required1[2] = {SL_BOOLEAN_TRUE,SL_BOOLEAN_TRUE};
				/* Create the music player */
				SLObjectItf player;
				result = (*EngineItf)->CreateAudioPlayer(EngineItf, &player, &audioSource, &audioSink, 2, iidArray1, required1);
				if (SL_RESULT_SUCCESS != result){
					LOGE("openSLPlayOpen: Can't create OpenSL AudioPlayer\n");
				}else{
					SLAndroidConfigurationItf playerConfig;
					result = (*player)->GetInterface(player, SL_IID_ANDROIDCONFIGURATION, &playerConfig);

					if (SL_RESULT_SUCCESS != result){
						LOGE("openSLPlayOpen: Can't get OpenSL Interface for android audio configuration\n");
					}

					SLint32 streamType;

					if(mainData->usespeaker==0){
						streamType = SL_ANDROID_STREAM_VOICE;
					}else{
						streamType = SL_ANDROID_STREAM_MEDIA;
					}

					result = (*playerConfig)->SetConfiguration(playerConfig, SL_ANDROID_KEY_STREAM_TYPE, &streamType, sizeof(SLint32));
					if (SL_RESULT_SUCCESS != result){
						LOGE("openSLPlayOpen: Can't set stream voice for audio\n");
					}


					/* Realizing the player in synchronous mode. */
					result = (*player)->Realize(player, SL_BOOLEAN_FALSE);
					if (SL_RESULT_SUCCESS != result){
						LOGE("openSLPlayOpen: Can't Realize OpenSL AudioPlayer\n");
					}else{
						/* Get seek and play interfaces */
						SLPlayItf playItf;
						result = (*player)->GetInterface(player, SL_IID_PLAY, (void*)&playItf);
						if (SL_RESULT_SUCCESS != result){
							LOGE("openSLPlayOpen: Can't get OpenSL Player interface\n");
						}else{
							SLAndroidSimpleBufferQueueItf bufferQueueItf;
							result = (*player)->GetInterface(player, SL_IID_ANDROIDSIMPLEBUFFERQUEUE, (void*)&bufferQueueItf);
							if (SL_RESULT_SUCCESS != result){
								LOGE("openSLPlayOpen: Can't get OpenSL Player interface\n");
							}else{
								/* Setup to receive buffer queue event callbacks */
								result = (*bufferQueueItf)->RegisterCallback(bufferQueueItf, BufferQueueCallback, &Data);
								if (SL_RESULT_SUCCESS != result){
									LOGE("openSLPlayOpen: Can't register OpenSL callback function\n");
								}else{
									int i,buffok = 1;
									for(i=0;i<NUMBER_BUFFERS-1;i++){
										result = (*bufferQueueItf)->Enqueue(bufferQueueItf, (void*) Data.Buffers[Data.read],	2 * P_RawDataBuf); // Size given in bytes.
										if (SL_RESULT_SUCCESS != result){
											LOGE("openSLPlayOpen: Can't add buffers to OpenSL queue\n");
											buffok = 0;
										}
										Data.read = nextBuffOut(Data.read);
									}
									if(buffok){
										result = (*playItf)->SetPlayState( playItf, SL_PLAYSTATE_PLAYING );
										if (SL_RESULT_SUCCESS != result){
											LOGE("openSLPlayOpen: Can't set options PlAY to OpenSL Player\n");
										}

										while(pthread_mutex_trylock(&Data.mainData->threadOutaudioQuit)==EBUSY ){
											sleep(1);
										}
										pthread_mutex_unlock(&mainData->threadOutaudioQuit);
										/* Do player is stopped */
										result =  (*playItf)->SetPlayState(playItf, SL_PLAYSTATE_STOPPED);
										LOGI("openSLPlayOpen: Stop playing. Res: %d\n",result);
									}
								}
							}
						}
					}
					/* Destroy the player */
					(*player)->Destroy(player);
					LOGI("openSLPlayOpen: Destroy player\n");
				}
			}

			/* Destroy Output Mix object */
			(*OutputMix)->Destroy(OutputMix);
			LOGI("openSLPlayOpen: Destroy OutputMix");
		}
	}
	LOGI("Exit: openSLPlayOpen: OK\n");
}


void * startThreadOutAudio(void * arg){
	LOGI("Enter: startThreadOutAudio\n");
	//return NULL;
	mainData_t* mainData = (mainData_t*)arg;
	SLresult res;
	SLObjectItf sl;
	SLEngineOption EngineOption[] = {
									(SLuint32) SL_ENGINEOPTION_THREADSAFE,
									(SLuint32) SL_BOOLEAN_TRUE};
	res = slCreateEngine( &sl, 1, EngineOption, 0, NULL, NULL);
	if(res != SL_RESULT_SUCCESS){
		LOGE("Exit: startThreadOutAudio: Can't create OpenSL engine\n");
		return 0;
	}
	/* Realizing the SL Engine in synchronous mode. */
	res = (*sl)->Realize(sl, SL_BOOLEAN_FALSE);
	if(res != SL_RESULT_SUCCESS){
		LOGE("Exit: startThreadOutAudio: Can't Realize OpenSL engine\n");
		/* Shutdown OpenSL ES */
		(*sl)->Destroy(sl);
		return 0;
	}

	openSLPlayOpen(mainData,sl);

	/* Shutdown OpenSL ES */
	(*sl)->Destroy(sl);
	LOGI("Exit: startThreadOutAudio: OK\n");
	return 0;
}

#else

void * startThreadOutAudio(void * arg){
	mainData_t* mainData = (mainData_t*)arg;
	int err;

	snd_pcm_t *playback_handle;
	snd_pcm_hw_params_t *hw_params;

	if ((err = snd_pcm_open (&playback_handle, "default", SND_PCM_STREAM_PLAYBACK, 0)) < 0) {
		fprintf (stderr, "cannot open audio device %s (%s)\n",
				"default",
			 snd_strerror (err));
		return NULL;
	}

	if ((err = snd_pcm_hw_params_malloc (&hw_params)) < 0) {
		fprintf (stderr, "cannot allocate hardware parameter structure (%s)\n",
			 snd_strerror (err));
		snd_pcm_close (playback_handle);
		return NULL;
	}

	if ((err = snd_pcm_hw_params_any (playback_handle, hw_params)) < 0) {
		fprintf (stderr, "cannot initialize hardware parameter structure (%s)\n",
			 snd_strerror (err));
		snd_pcm_hw_params_free (hw_params);
		snd_pcm_close (playback_handle);
		return NULL;
	}

	if ((err = snd_pcm_hw_params_set_access (playback_handle, hw_params, SND_PCM_ACCESS_RW_INTERLEAVED)) < 0) {
		fprintf (stderr, "cannot set access type (%s)\n",
			 snd_strerror (err));
		snd_pcm_hw_params_free (hw_params);
		snd_pcm_close (playback_handle);
		return NULL;
	}

	if ((err = snd_pcm_hw_params_set_format (playback_handle, hw_params, SND_PCM_FORMAT_S16_LE)) < 0) {
		fprintf (stderr, "cannot set sample format (%s)\n",
			 snd_strerror (err));
		snd_pcm_hw_params_free (hw_params);
		snd_pcm_close (playback_handle);
		return NULL;
	}
	unsigned int rate = 8000;
	if ((err = snd_pcm_hw_params_set_rate_near (playback_handle, hw_params, &rate, 0)) < 0) {
		fprintf (stderr, "cannot set sample rate (%s)\n",
			 snd_strerror (err));
		snd_pcm_hw_params_free (hw_params);
		snd_pcm_close (playback_handle);
		return NULL;
	}

	if ((err = snd_pcm_hw_params_set_channels (playback_handle, hw_params, 1)) < 0) {
		fprintf (stderr, "cannot set channel count (%s)\n",
			 snd_strerror (err));
		snd_pcm_hw_params_free (hw_params);
		snd_pcm_close (playback_handle);
		return NULL;
	}
	snd_pcm_uframes_t buffer_size = 160*10;
	if((err=snd_pcm_hw_params_set_buffer_size_near (playback_handle, hw_params, &buffer_size))<0){
		fprintf (stderr, "cannot set parameters (%s)\n",
			 snd_strerror (err));
		snd_pcm_hw_params_free (hw_params);
		snd_pcm_close (playback_handle);
		return NULL;
	}
	if ((err = snd_pcm_hw_params (playback_handle, hw_params)) < 0) {
		fprintf (stderr, "cannot set parameters (%s)\n",
			 snd_strerror (err));
		snd_pcm_hw_params_free (hw_params);
		snd_pcm_close (playback_handle);
		return NULL;
	}

	snd_pcm_hw_params_free (hw_params);

	if ((err = snd_pcm_prepare (playback_handle)) < 0) {
		fprintf (stderr, "cannot prepare audio interface for use (%s)\n",
			 snd_strerror (err));
		snd_pcm_close (playback_handle);
		return NULL;
	}

	struct timeval te;
	long long nexttime,now,delta;
	int delay = DELAY-10;
	gettimeofday(&te, NULL);
	nexttime= te.tv_sec*1000LL + te.tv_usec/1000;
	unsigned short buf[P_RawDataBuf];
	memset(buf,0,P_RawDataBuf*2);
	while(pthread_mutex_trylock(&mainData->threadOutaudioQuit)==EBUSY ){
		getBufForAllUsers(mainData->usersArray,buf);
		err = snd_pcm_writei (playback_handle, buf, P_RawDataBuf);
		if(err==-32){
			printf(" LATE \n");
			snd_pcm_prepare(playback_handle);
			if((err=snd_pcm_writei (playback_handle, buf, P_RawDataBuf))<0){
				fprintf (stderr, "write to audio interface failed (%s)\n",
					 snd_strerror (err));
			}
		}else
		{
			if(err!=P_RawDataBuf){
				fprintf (stderr, "write to audio interface failed (%s)\n",
					 snd_strerror (err));
				return NULL;
				snd_pcm_close (playback_handle);
			}
		}
		gettimeofday(&te, NULL);
		nexttime+=delay;
		now = te.tv_sec*1000LL + te.tv_usec/1000;
		delta = nexttime-now;

		if(delta>0){
			usleep(delta*1000LL);
		}
	}
	pthread_mutex_unlock(&mainData->threadOutaudioQuit);
	snd_pcm_close (playback_handle);
}
#endif
