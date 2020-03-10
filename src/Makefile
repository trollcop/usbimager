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
CFLAGS = -DDISKS_TEST=1 -D_FILE_OFFSET_BITS=64 -D__USE_FILE_OFFSET64 -D__USE_LARGEFILE -Wall -Wextra -pedantic --std=c99 -O3 -fvisibility=hidden -I./zlib -I./bzip2 -I./xz
LDFLAGS =
LIBS =
DECOMPRESSORS = zlib/libz.a bzip2/libbz2.a xz/libxz.a

SRC = $(filter-out $(wildcard main_*.c) $(wildcard disks_*.c),$(wildcard *.c))
OBJ =

####### detect operating system and platform #######

ifneq ("$(wildcard /bin/*.exe)","")
# Windows (mingw)
WIN = 1
SRC += main_win.c disks_win.c
CFLAGS += -DNDEBUG -DWINVER=0x0500 -DUNICODE=1
LDFLAGS += -mwindows -static -static-libgcc
TARGET = usbimager.exe
OBJ += resource.o
WINDRES = windres
ARCH = i686
FRM = win-gdi
else
ifneq ("$(wildcard /Library/Frameworks)","")
# MacOSX
MACOSX = 1
CFLAGS += -DMACOSX=1 -x objective-c -fobjc-exceptions
LDFLAGS += -macosx_version_min 10.14 -framework CoreFoundation -framework IOKit -framework DiskArbitration
ifneq ($(USE_X11),)
SRC += main_x11.c
CFLAGS += -I/usr/include/X11 -I/opt/X11/include
LIBS += -lX11
FRM = macosx-x11
else
SRC += main_libui.c
CFLAGS += -pthread
LDFLAGS += -framework Foundation -framework Cocoa
LIBS += libui/darwin.a -lc -lobjc -lpthread
FRM = macosx-cocoa
endif
SRC += disks_darwin.c
LD = ld
GRP = operator
ARCH = intel
else
# Linux
LINUX = 1
ifeq ($(USE_LIBUI),)
SRC += main_x11.c
CFLAGS += -I/usr/include/X11
LIBS += -lX11
FRM = linux-x11
GRP = disk
ifneq ($(USE_UNITY),)
CFLAGS += -DUSE_UNITY=1
endif
else
SRC += main_libui.c
CFLAGS += -pthread
LDFLAGS += -pthread
LIBS += libui/linux.a
ifneq ("$(wildchard /usr/bin/pkg-config)","")
LIBS += $(shell pkg-config --libs gtk+-3.0)
else
LIBS += -lgtk-3 -lgdk-3 -lgobject-2.0 -lglib-2.0
endif
FRM = linux-gtk
endif
SRC += disks_linux.c
ARCH = $(shell uname -m)
DEBARCH = $(ARCH:x86_64=amd64)
endif
endif

OBJ += $(SRC:.c=.o)
ifneq ($(DEBUG),)
CFLAGS += -g
GRP =
endif

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
ifeq ($(DEBUG),)
	$(STRIP) $(TARGET)
endif
ifneq ($(GRP),)
	chgrp $(GRP) $(TARGET) || true
	chmod g+s $(TARGET) || true
	@(ls -la $(TARGET)|grep $(GRP)|grep sr) || printf "\n\nWARNING - Your user is not member of the '$(GRP)' group, can't grant access. Run the following two commands manually:\n\n  sudo chgrp $(GRP) $(TARGET)\n  sudo chmod g+s $(TARGET)\n\n"
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
	@rm ../usbimager-$(ARCH)-$(FRM).zip 2>/dev/null || true
	zip -r -9 ../usbimager-$(ARCH)-$(FRM).zip USBImager
	@rm -rf USBImager
else
ifneq ($(MACOSX),)
	@mkdir USBImager.app USBImager.app/Contents USBImager.app/Contents/MacOS USBImager.app/Contents/Resources
	@cp $(TARGET) USBImager.app/Contents/MacOS
	@cp misc/Info.plist USBImager.app/Contents
	@cp misc/usbimager.icns USBImager.app/Contents/Resources
	@rm ../usbimager-$(ARCH)-$(FRM).zip 2>/dev/null || true
	zip -r -9 ../usbimager-$(ARCH)-$(FRM).zip USBImager.app
	@rm -rf USBImager.app
else
	@mkdir bin share share/applications share/icons share/icons/hicolor share/icons/hicolor/16x16 share/icons/hicolor/16x16/apps share/icons/hicolor/32x32 share/icons/hicolor/32x32/apps share/icons/hicolor/64x64 share/icons/hicolor/64x64/apps share/icons/hicolor/128x128 share/icons/hicolor/128x128/apps
	@cp $(TARGET) bin
	@cp misc/usbimager.desktop share/applications
	@cp misc/icon128.png share/icons/hicolor/128x128/apps/usbimager.png
	@cp misc/icon64.png share/icons/hicolor/64x64/apps/usbimager.png
	@cp misc/icon32.png share/icons/hicolor/32x32/apps/usbimager.png
	@cp misc/icon16.png share/icons/hicolor/16x16/apps/usbimager.png
	@rm ../usbimager-$(ARCH)-$(FRM).zip 2>/dev/null || true
	zip -r -9 ../usbimager-$(ARCH)-$(FRM).zip bin share
	@rm -rf bin share
endif
endif

deb: $(TARGET)
	@mkdir DEBIAN usr usr/bin usr/share usr/share/applications usr/share/icons usr/share/icons/hicolor usr/share/icons/hicolor/16x16 usr/share/icons/hicolor/16x16/apps usr/share/icons/hicolor/32x32 usr/share/icons/hicolor/32x32/apps usr/share/icons/hicolor/64x64 usr/share/icons/hicolor/64x64/apps usr/share/icons/hicolor/128x128 usr/share/icons/hicolor/128x128/apps
	@cp $(TARGET) usr/bin
	@cp misc/usbimager.desktop usr/share/applications
	@cp misc/icon128.png usr/share/icons/hicolor/128x128/apps/usbimager.png
	@cp misc/icon64.png usr/share/icons/hicolor/64x64/apps/usbimager.png
	@cp misc/icon32.png usr/share/icons/hicolor/32x32/apps/usbimager.png
	@cp misc/icon16.png usr/share/icons/hicolor/16x16/apps/usbimager.png
	@cat misc/deb_control | sed s/ARCH/$(DEBARCH)/g | sed s/SIZE/`du -s usr|cut -f 1`/g >DEBIAN/control
	@md5sum `find usr -type f` >DEBIAN/md5sums
	@cp misc/deb_postinst DEBIAN/postinst
	@cp ../LICENSE DEBIAN/copyright
	@chmod +x DEBIAN/postinst
	@echo "2.0" >debian-binary
	@tar -czvf data.tar.gz usr
	@tar -C DEBIAN -czvf control.tar.gz control copyright md5sums postinst
	@ar r ../usbimager_$(shell cat main.h|grep USBIMAGER_VERSION|cut -d '"' -f 2)_$(DEBARCH).deb debian-binary control.tar.gz data.tar.gz
	@rm -rf debian-binary control.tar.gz data.tar.gz DEBIAN usr

####### cleanup #######

clean:
	rm $(TARGET) *.o *.bin zlib/*.o zlib/*.exe zlib/ztest* bzip2/*.o xz/*.o 2>/dev/null || true

cleanall: clean
	@make -C zlib clean || true
	@make -C bzip2 clean || true
	@make -C xz clean || true
	@rm zlib/Makefile zlib/*.log zlib/zlib.pc $(DECOMPRESSORS) 2>/dev/null || true
