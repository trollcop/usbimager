/*
 * usbimager/stream.h
 *
 * Copyright (C) 2020 bzt (bztsrc@gitlab)
 *
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software and associated documentation
 * files (the "Software"), to deal in the Software without
 * restriction, including without limitation the rights to use, copy,
 * modify, merge, publish, distribute, sublicense, and/or sell copies
 * of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
 * HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 *
 * @brief Stream Input/Output file functions
 *
 */

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <time.h>
#include <sys/stat.h>
#include <inttypes.h>   /* for PRI* defines */
#include "main.h"
#include "zlib.h"
#include "bzlib.h"
#define XZ_USE_CRC64
#define XZ_DEC_ANY_CHECK
#include "xz.h"

#ifndef PRIu64
#if __WORDSIZE == 64
#define PRIu64 "lu"
#define PRId64 "ld"
#else
#define PRIu64 "llu"
#define PRId64 "lld"
#endif
#endif

/* stream context */
typedef struct {
    FILE *f;
    uint64_t fileSize;
    uint64_t compSize;
    uint64_t readSize;
    uint64_t cmrdSize;
    uint64_t avgSpeedBytes;
    uint64_t avgSpeedNum;
    unsigned char *compBuf;
    char *buffer;
    char *verifyBuf;
    z_stream zstrm;
    bz_stream bstrm;
    struct xz_buf xstrm;
    struct xz_dec *xz;
    BZFILE *b;
    char type;
    time_t start;
} stream_t;

/**
 * Returns progress percentage and the status string in str
 */
int stream_status(stream_t *ctx, char *str, int done);

/**
 * Open file and determine the source's format
 * if uncompr is set, then file will be assumed uncompressed
 */
int stream_open(stream_t *ctx, char *fn, int uncompr);

/**
 * Read no more than buffer_size uncompressed bytes of source data
 */
int stream_read(stream_t *ctx);

/**
 * Open file for writing
 */
int stream_create(stream_t *ctx, char *fn, int comp, uint64_t size);

/**
 * Compress and write out data
 */
int stream_write(stream_t *ctx, char *buffer, int size);


/**
 * Close stream descriptors
 */
void stream_close(stream_t *ctx);
