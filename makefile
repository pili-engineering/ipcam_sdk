all:lib install

install:
	if [ -e lib ]; then \
		rm -rf lib ;\
	fi
	mkdir lib
	cp rtmpdump/librtmp/librtmp.a lib
	cp rtmpsdk/librtmp_sdk.a lib
	cp fdk-aac/.libs/libfdk-aac.a lib
	

lib:librtmp libfdk-aac librtmp_sdk

librtmp:
	cd rtmpdump && make CRYPTO= SHARED=

librtmp_sdk:
	cd rtmpsdk && make  all

libfdk-aac:
	cd fdk-aac && ./autogen.sh && ./configure --enable-shared=no  && make

clean:
	cd rtmpdump && make clean
	cd rtmpsdk  && make clean
	cd fdk-aac  && make clean
	rm -rf lib

help:
	cat readme
