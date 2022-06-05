SWIFTFILES=$(shell find . -type f -name \*.swift)


all: librtmpr/librtmpr.a app

app: $(SWIFTFILES) Makefile
	swiftc -g -target arm64-apple-macos12.3 -import-objc-header bridge.h -I. -lrtmpr -Llibrtmpr -module-name rtmpr -o $@ $(SWIFTFILES)

debug: app
	arch -arm64 lldb $<

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

.PHONY: debug