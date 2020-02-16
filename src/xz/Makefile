#
# Makefile
#
# Author: Lasse Collin <lasse.collin@tukaani.org>
#
# This file has been put into the public domain.
# You can do whatever you want with this file.
#

CC = gcc -std=gnu89
BCJ_CPPFLAGS = -DXZ_DEC_X86 -DXZ_DEC_POWERPC -DXZ_DEC_IA64 \
		-DXZ_DEC_ARM -DXZ_DEC_ARMTHUMB -DXZ_DEC_SPARC
CPPFLAGS = -DXZ_USE_CRC64 -DXZ_DEC_ANY_CHECK
CFLAGS = -O2 -pedantic -Wall -Wextra -Wno-implicit-fallthrough
RM = rm -f
COMMON_SRCS = xz_crc32.c xz_crc64.c xz_dec_stream.c xz_dec_lzma2.c xz_dec_bcj.c
COMMON_OBJS = $(COMMON_SRCS:.c=.o)
XZ_HEADERS = xz.h xz_private.h xz_stream.h xz_lzma2.h xz_config.h

ALL_CPPFLAGS = -I. $(BCJ_CPPFLAGS) $(CPPFLAGS)

all: libxz.a

%.o: %.c $(XZ_HEADERS)
	$(CC) $(ALL_CPPFLAGS) $(CFLAGS) -c -o $@ $<

libxz.a: $(COMMON_OBJS)
	$(AR) cq libxz.a $(COMMON_OBJS)

.PHONY: clean
clean:
	-$(RM) $(COMMON_OBJS) libxz.a
