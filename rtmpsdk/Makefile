
CC=$(CROSS_COMPILE)gcc
LD=$(CROSS_COMPILE)gcc
AR=$(CROSS_COMPILE)ar

CXX_OBJ=$(patsubst %.c,%.o,$(wildcard *.c))
SOURCES=$(shell find . -maxdepth 1 -name "*.o") 
BIN=main
LIB=librtmp_sdk.a

LIB_A=../fdk-aac/.libs/libfdk-aac.a ../rtmpdump/librtmp/librtmp.a -lm
USE_AAC_ENCODER=__USE_FDKAAC__
INC=-I../fdk-aac/libAACenc/include/ -I../fdk-aac/libSYS/include/

INC+=-I../rtmpdump/librtmp/
CFLAGS=$(INC) -Wall -Wno-narrowing -Wno-unused -g
#LDFLAGS=-L/home/liuqian/qiniu/cross/arm-2011.09/arm-none-linux-gnueabi/libc/usr/lib -L/home/liuqian/qiniu/ipc365/xlib/rtmpdump/librtmp/ -L../ -lrtmp -lm -lpthread  -ldl 
ARFLAGS=rcs


all: $(CXX_OBJ) $(LIB) 
#all: $(CXX_OBJ) $(LIB) $(BIN)

build: clean all

clean:
	-rm *.o librtmp_sdk.a $(BIN) *~
.c.o:
	@echo "CC " $<
	@$(CC) $(CFLAGS) -c $<
$(LIB):
	@echo "AR " $(LIB)
	$(AR) $(ARFLAGS) $(LIB) $(CXX_OBJ) $(LIBA)

$(BIN): $(SOURCES)
	@echo "LD " $(BIN) 
	@$(LD) $(SOURCES) $(LIB_A) -o $(BIN) $(LDFLAGS)

