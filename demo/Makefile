#PROJECT_PATH=${shell pwd}
#CC=arm-linux-gnueabihf-gcc

DEMO_LIBA = ../lib/librtmp_sdk.a ../lib/librtmp.a ../lib/libfdk-aac.a


DEMO_LIBS =-lm  -lstdc++  -lpthread
INCLUDE_PATH = -I../rtmpsdk -I../rtmpdump/librtmp/


CC = $(CROSS_COMPILE)gcc
OBJS= main.o QnRtmp.o QnCommon.o  mock_anjia.o adts.o

.c.o:
	$(CC) $(INCLUDE_PATH) -g -c $<


demo_rtmp:$(OBJS)
	#$(CC) -o $@ $^ $(LDFLAGS)
	$(CC) -o $@ $^ $(DEMO_LIBA) $(DEMO_LIBS)

clean:
	rm *.o demo_rtmp

