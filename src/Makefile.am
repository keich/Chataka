bin_PROGRAMS = broadcasttalk
AM_LDFLAGS = -pthread
LIBS = -lasound
broadcasttalk_SOURCES = broadcasttalk.c maindata.c udpsocket.c interfaceip.c threadsocket.c threadinaudio.c tools.c gsmcodec.c threadoutaudio.c users.c
SUBDIRS = gsm
LDADD = gsm/libgsm.a
