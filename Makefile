all: librtmpr/librtmpr.a app

app: main.swift Makefile
	swiftc -g -target arm64-apple-macos12.3 -import-objc-header bridge.h -I. -lrtmpr -Llibrtmpr -o $@ $<

librtmpr/librtmpr.a: FORCE
	@cd librtmpr; make all

clean:
	rm -f app
	@cd librtmpr; make clean

run: app
	@./app

test: FORCE
	@cd librtmpr; make run

FORCE: