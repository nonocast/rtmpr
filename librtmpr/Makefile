VERSION=v2.4

# change default make data-base
CC=clang
CXX=clang++
COMPILE.c = $(CC) $(CFLAGS) $(TARGET_ARCH) -c 
ARCH=-arch x86_64

DEF_=-DNO_CRYPTO
CRYPTO_DEF=$(DEF_$(CRYPTO))
DEF=-DRTMPDUMP_VERSION=\"$(VERSION)\" $(CRYPTO_DEF) $(XDEF)
OPT=-O2
CPPFLAGS=$(ARCH) -g -std=c++11
CFLAGS=$(ARCH) -g -Wall -Wno-unused -arch arm64 -arch x86_64 $(DEF) $(OPT)
OBJECTS=rtmp.o log.o amf.o hashswf.o parseurl.o rtmpx.o flv.o flvhttpd.o
TEST_OBJECTS=rtmpx_test.o flv_test.o flvhttpd_test.o

all: librtmpr.a test app-flv app-annexb app-flvhttpd

run: app-flvhttpd
	@./$<

debug: app-flvhttpd
	@arch -x86_64 lldb $<

run.test: test
	@./test

app-flvhttpd: $(OBJECTS) app-flvhttpd.o
	clang $(ARCH) -g -o $@ $^

app-annexb: $(OBJECTS) app-annexb.o
	clang $(ARCH) -g -o $@ $^

app-flv: $(OBJECTS) app-flv.o
	clang  $(ARCH) -g -o $@ $^

test: librtmpr.a $(TEST_OBJECTS)
	clang++ $(ARCH) -g `pkg-config --cflags --libs gtest` -lrtmpr -L. -lgtest_main -o $@ $(TEST_OBJECTS)

librtmpr.a: $(OBJECTS)
	libtool -static -o $@ $^

# make -p show implicit rules
log.o: log.c log.h Makefile
rtmp.o: rtmp.c rtmp.h rtmp_sys.h handshake.h dh.h log.h amf.h Makefile
amf.o: amf.c amf.h bytes.h log.h Makefile
hashswf.o: hashswf.c http.h rtmp.h rtmp_sys.h Makefile
parseurl.o: parseurl.c rtmp.h rtmp_sys.h log.h Makefile
flv.o: flv.h Makefile
rtmpx.o: rtmpx.c rtmpx.h Makefile
rtmpx_test.o: rtmpx_test.cc rtmpx.h Makefile
flv_test.o: flv_test.cc flv.h Makefile

clean:
	rm -f *.o *.out librtmpr.a app-annexb app-flv test

.PHONY: run run.test