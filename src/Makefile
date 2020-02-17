# 
#  usbimager/Makefile
# 
#  Copyright (C) 2020 bzt (bztsrc@gitlab)
# 
#  Permission is hereby granted, free of charge, to any person
#  obtaining a copy of this software and associated documentation
#  files (the "Software"), to deal in the Software without
#  restriction, including without limitation the rights to use, copy,
#  modify, merge, publish, distribute, sublicense, and/or sell copies
#  of the Software, and to permit persons to whom the Software is
#  furnished to do so, subject to the following conditions:
# 
#  The above copyright notice and this permission notice shall be
#  included in all copies or substantial portions of the Software.
# 
#  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
#  EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
#  MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
#  NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
#  HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
#  WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
#  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
#  DEALINGS IN THE SOFTWARE.
# 
#  @brief Project makefile
# 

####### overall configuration #######

TARGET = usbimager
CC = gcc
LD = gcc
STRIP = strip
# setting DISKS_TEST to 1 will add a test.bin "device" to the target disks list
CFLAGS = -DDISKS_TEST=0 -Wall -Wextra -pedantic --std=c99 -O3 -fvisibility=hidden -I./zlib -I./bzip2 -I./xz
LDFLAGS =
LIBS =
DECOMPRESSORS = zlib/libz.a bzip2/libbz2.a xz/libxz.a

SRC = $(filter-out main_win.c main_unix.c disks_linux.c disks_darwin.c disks_win.c,$(wildcard *.c))
OBJ =

####### detect operating system and platform #######

ifneq ("$(wildcard /bin/*.exe)","")
# Windows (mingw)
WIN = 1
SRC += main_win.c disks_win.c
CFLAGS += -DNDEBUG -DWINVER=0x0500
LDFLAGS += -mwindows -static -static-libgcc
TARGET = usbimager.exe
OBJ += resource.o
WINDRES = windres
else
ifneq ("$(wildcard /Library/Frameworks)","")
# MacOSX
MACOSX = 1
SRC += main_unix.c disks_darwin.c
CFLAGS += -DMACOSX=1 -pthread -x objective-c -fobjc-exceptions
LDFLAGS += -macosx_version_min 10.14 -framework CoreFoundation -framework Foundation -framework IOKit -framework DiskArbitration -framework Cocoa
LIBS += libui/darwin.a -lc -lobjc -lpthread
LD = ld
GRP = operator
else
# Linux
LINUX = 1
SRC += main_unix.c disks_linux.c
CFLAGS += -pthread
LDFLAGS += -pthread
LIBS += libui/linux.a
ifneq ("$(wildchard /usr/bin/pkg-config)","")
LIBS += $(shell pkg-config --libs gtk+-3.0)
else
LIBS += -lgtk-3 -lgdk-3 -lgobject-2.0 -lglib-2.0
endif
# don't use. GTK will refuse to start...
#GRP = disk
endif
endif

OBJ += $(SRC:.c=.o)

####### rules to compile #######

all: $(TARGET)

zlib/Makefile:
	@cd zlib && chmod +x ./configure && ./configure && cd ..

zlib/libz.a: zlib/Makefile
	@make -C zlib libz.a

bzip2/libbz2.a:
	@make -C bzip2 libbz2.a

xz/libxz.a:
	@make -C xz libxz.a

resource.o: misc/resource.rc
	$(WINDRES) misc/resource.rc -o resource.o

%.o: %.c
	$(CC) $(CFLAGS) -o $@ -c $<

$(TARGET): $(DECOMPRESSORS) $(OBJ)
	$(LD) $(LDFLAGS) -o $@ $(OBJ) $(DECOMPRESSORS) $(LIBS)
	$(STRIP) $(TARGET)
ifneq ($(GRP),)
	@chgrp $(GRP) $(TARGET)
	@chmod g+s $(TARGET)
endif

####### install and package creation #######

install: $(TARGET)
	install -m 755 -g bin $(TARGET) /usr/bin
	cp misc/usbimager.desktop /usr/share/applications
	cp misc/icon128.png /usr/share/icons/hicolor/128x128/apps/usbimager.png
	cp misc/icon64.png /usr/share/icons/hicolor/64x64/apps/usbimager.png
	cp misc/icon32.png /usr/share/icons/hicolor/32x32/apps/usbimager.png
	cp misc/icon16.png /usr/share/icons/hicolor/16x16/apps/usbimager.png

package: $(TARGET)
ifneq ($(WIN),)
	@mkdir USBImager
	@cp $(TARGET) USBImager
	@rm ../usbimager-i686-win.zip 2>/dev/null || true
	zip -r -9 ../usbimager-i686-win.zip USBImager
	@rm -rf USBImager
else
ifneq ($(MACOSX),)
	@mkdir USBImager.app USBImager.app/Contents USBImager.app/Contents/MacOS USBImager.app/Contents/Resources
	@cp $(TARGET) USBImager.app/Contents/MacOS
	@cp misc/Info.plist USBImager.app/Contents
	@cp misc/usbimager.icns USBImager.app/Contents/Resources
	@rm ../usbimager-intel-macosx.zip 2>/dev/null || true
	zip -r -9 ../usbimager-intel-macosx.zip USBImager.app
	@rm -rf USBImager.app
else
	@mkdir bin share share/applications share/icons share/icons/hicolor share/icons/hicolor/16x16 share/icons/hicolor/16x16/apps share/icons/hicolor/32x32 share/icons/hicolor/32x32/apps share/icons/hicolor/64x64 share/icons/hicolor/64x64/apps share/icons/hicolor/128x128 share/icons/hicolor/128x128/apps
	@cp $(TARGET) bin
	@cp misc/usbimager.desktop share/applications
	@cp misc/icon128.png share/icons/hicolor/128x128/apps/usbimager.png
	@cp misc/icon64.png share/icons/hicolor/64x64/apps/usbimager.png
	@cp misc/icon32.png share/icons/hicolor/32x32/apps/usbimager.png
	@cp misc/icon16.png share/icons/hicolor/16x16/apps/usbimager.png
	@rm ../usbimager-x86_64-linux.zip 2>/dev/null || true
	zip -r -9 ../usbimager-x86_64-linux.zip bin share
	@rm -rf bin share
endif
endif

####### cleanup #######

clean:
	rm $(TARGET) *.o zlib/*.o zlib/*.exe bzip2/*.o xz/*.o 2>/dev/null || true

cleanall: clean
	@make -C zlib clean || true
	@make -C bzip2 clean || true
	@make -C xz clean || true
	@rm zlib/Makefile zlib/*.log zlib/zlib.pc $(DECOMPRESSORS) 2>/dev/null || true
