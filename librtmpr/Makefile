VERSION=v2.4

# change default make data-base
CC=clang
CXX=clang++
COMPILE.c = $(CC) $(CFLAGS) $(TARGET_ARCH) -c 

DEF_=-DNO_CRYPTO
CRYPTO_DEF=$(DEF_$(CRYPTO))
DEF=-DRTMPDUMP_VERSION=\"$(VERSION)\" $(CRYPTO_DEF) $(XDEF)
OPT=-O2
CPPFLAGS=-std=c++11 -g
CFLAGS=-g -Wall -Wno-unused -arch arm64 -arch x86_64 $(DEF) $(OPT)
OBJECTS=rtmp.o log.o amf.o hashswf.o parseurl.o rtmpwrapper.o flv.o
TEST_OBJECTS=rtmpwrapper_test.o flv_test.o

all: librtmpr.a test app-flv app-annexb

run: app-annexb
	@./$<

run.test: test
	@./test

app-annexb: $(OBJECTS) app-annexb.o
	clang -o $@ $^

app-flv: $(OBJECTS) app-flv.o
	clang -o $@ $^

test: librtmpr.a $(TEST_OBJECTS)
	clang++ `pkg-config --cflags --libs gtest` -lrtmpr -L. -lgtest_main -o $@ $(TEST_OBJECTS)

librtmpr.a: $(OBJECTS)
	libtool -static -o $@ $^

# make -p show implicit rules
log.o: log.c log.h Makefile
rtmp.o: rtmp.c rtmp.h rtmp_sys.h handshake.h dh.h log.h amf.h Makefile
amf.o: amf.c amf.h bytes.h log.h Makefile
hashswf.o: hashswf.c http.h rtmp.h rtmp_sys.h Makefile
parseurl.o: parseurl.c rtmp.h rtmp_sys.h log.h Makefile
flv.o: flv.h Makefile
rtmpwrapper.o: rtmpwrapper.c rtmpwrapper.h Makefile
rtmpwrapper_test.o: rtmpwrapper_test.cc rtmpwrapper.h Makefile
flv_test.o: flv_test.cc flv.h Makefile

clean:
	rm -f *.o *.out librtmpr.a app-annexb app-flv test

.PHONY: run run.test