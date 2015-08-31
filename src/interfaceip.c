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

#include <sys/ioctl.h>
#include <net/if.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include "interfaceip.h"
#include <string.h>
#include <unistd.h>
#include "broadcasttalk.h"

//#define  printf(...)  __android_log_print(ANDROID_LOG_INFO,APPNAME,__VA_ARGS__);

#define INT_TO_ADDR(_addr) \
(_addr & 0xFF), \
(_addr >> 8 & 0xFF), \
(_addr >> 16 & 0xFF), \
(_addr >> 24 & 0xFF)


void getBrdAddr(selfIP_t * myIp,struct sockaddr_in * addr){
	sem_wait(&myIp->Lock);
	int i;
	memcpy(addr,&myIp->brdaddr,sizeof(struct sockaddr_in));
	sem_post(&myIp->Lock);
	return;
}

/* If addr is in array than return 1 or return 0  */
int isMyAddr(selfIP_t * myIp,struct sockaddr_in * addr){
	LOGI("Enter: isMyAddr\n");
	sem_wait(&myIp->Lock);
	int i;
	for(i=0;i<myIp->lenghtArray;i++){
		if(myIp->addr[i].sin_addr.s_addr==addr->sin_addr.s_addr){
			sem_post(&myIp->Lock);
			LOGI("Exit: isMyAddr: TRUE\n");
			return 1;
		}
	}
	sem_post(&myIp->Lock);
	LOGI("Exit: isMyAddr: FALSE\n");
	return 0;
}
/* Destroy selfIP struct and free memory */
void destroyIPArray(selfIP_t * myIp){
	LOGI("Enter: destroyIPArray\n");
	free(myIp->addr);
	sem_destroy(&myIp->Lock);
	free(myIp);
	LOGI("Exit: destroyIPArray: OK\n");
}
/* Create selfIP structure and allocate memory */
selfIP_t * createIPArray(){
	LOGI("Enter: createIPArray\n");
	selfIP_t * ret =  ( selfIP_t * )malloc(sizeof(selfIP_t));
	memset(ret,0,sizeof(selfIP_t));
	sem_init(&ret->Lock, 0, 1);
	ret->addr = NULL;
	ret->lenghtArray = 0;
	ret->brdaddr.sin_family = AF_INET;
	ret->brdaddr.sin_port = htons(PORT);
	ret->brdaddr.sin_addr.s_addr = INADDR_BROADCAST;
	LOGI("Exit: createIPArray: OK\n");
	return ret;
}

/* Add ip to the array and alloc more memory */
void addtoIPArray(selfIP_t * myIp,struct sockaddr_in * addr){
	LOGI("Enter: addtoIPArray\n");
	myIp->addr = (struct sockaddr_in *)realloc(myIp->addr,sizeof(struct sockaddr_in)*(myIp->lenghtArray+1));
	memcpy(&myIp->addr[myIp->lenghtArray],addr,sizeof(struct sockaddr_in));
	myIp->lenghtArray++;
	LOGI("Exit: addtoIPArray: OK\n");
}
/* Remove all ip in array and free memory for this array. */
void clearIPArray(selfIP_t * myIp){
	LOGI("Enter: clearIPArray\n");
	if(myIp->addr!=NULL){
		free(myIp->addr);
	}
	myIp->addr = NULL;
	myIp->lenghtArray=0;
	LOGI("Exit: clearIPArray: OK\n");
}

/* Load all local ip to the array */
void getAllLocalIP(selfIP_t * myIp){
	LOGI("Enter: getAllLocalIP\n");
	sem_wait(&myIp->Lock);
    struct ifconf ifc;
    struct ifreq ifr[10];
    int sd, ifc_num, addr, bcast, mask, network, i;
    myIp->brdaddr.sin_addr.s_addr = INADDR_BROADCAST;

    /* Create a socket so we can use ioctl on the file
     * descriptor to retrieve the interface info.
     */
    sd = socket(PF_INET, SOCK_DGRAM, 0);
    if (sd > 0)
    {
        ifc.ifc_len = sizeof(ifr);
        ifc.ifc_ifcu.ifcu_buf = (caddr_t)ifr;

        if (ioctl(sd, SIOCGIFCONF, &ifc) == 0)
        {
            ifc_num = ifc.ifc_len / sizeof(struct ifreq);
            LOGI("getAllLocalIP: %d interfaces found\n", ifc_num);

            for (i = 0; i < ifc_num; ++i)
            {
#ifdef __ANDROID__
#else
                if (ifr[i].ifr_addr.sa_family != AF_INET)
                {
                    continue;
                }
#endif

                /* display the interface name */
                LOGI("getAllLocalIP: %d) interface: %s\n", i+1, ifr[i].ifr_name);

                /* Retrieve the IP address, broadcast address, and subnet mask. */
                if (ioctl(sd, SIOCGIFBRDADDR, &ifr[i]) == 0)
                {
                    bcast = ((struct sockaddr_in *)(&ifr[i].ifr_broadaddr))->sin_addr.s_addr;
                    memcpy(&myIp->brdaddr,(struct sockaddr_in *)(&ifr[i].ifr_broadaddr),sizeof(struct sockaddr_in));
                    myIp->brdaddr.sin_family = AF_INET;
                    myIp->brdaddr.sin_port = htons(PORT);
                    LOGI("getAllLocalIP: %d) broadcast: %d.%d.%d.%d\n", i+1, INT_TO_ADDR(bcast));

                }
				if (ioctl(sd, SIOCGIFADDR, &ifr[i]) == 0)
				{
					addr = ((struct sockaddr_in *)(&ifr[i].ifr_addr))->sin_addr.s_addr;
					addtoIPArray(myIp,(struct sockaddr_in *)&ifr[i].ifr_addr);
					LOGI("getAllLocalIP: %d) address: %d.%d.%d.%d\n",i+1, INT_TO_ADDR(addr));
				}

            }
        }

        close(sd);
    }else{
    	LOGE("Exit: getAllLocalIP: Can't create socket to get ethernet configuration\n");
    }
    sem_post(&myIp->Lock);
    LOGI("Exit: getAllLocalIP: OK\n");
    return;
}
