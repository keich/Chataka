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
#include "string.h"

#define MASK 255

/*unsigned long long getCount(unsigned char * buf){
	//unsigned long long i;
	//i = (MASK&(int)(buf[2]));
	//i =  (i << 8) | (MASK&(int)(buf[3])) ;
	//i =  (i << 8) | (MASK&(int)(buf[4])) ;
	//i =  (i << 8) | (MASK&(int)(buf[5])) ;

	unsigned long long count = 0;
	printf("sizeof %i",sizeof(count));
	memcpy(count,buf[2],8);
	return count;
}


void setHeader(unsigned char *buf,char type,unsigned long long count){
	buf[0] = type;
	buf[1] = VERSION;
	//memcpy(buf[2],&count,8);
	//buf[5] = MASK & count;
	//buf[4] = MASK & (count  >>8);
	//buf[3] = MASK & (count  >>16);
	//buf[2] = MASK & (count  >>24);
}*/
