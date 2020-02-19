/*
 * usbimager/input.h
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
 * @brief Input file functions
 *
 */

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <time.h>
#include <inttypes.h>   /* for PRI* defines */
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
#define PRId64 "llu"
#define PRIu64 "lld"
#endif
#endif

extern int verbose;

/* the overall buffer size. Will write in BUFFER_SIZE chunks */
#define BUFFER_SIZE (1024 * 1024)

/* decompression filters */
enum {
    TYPE_PLAIN = 0,
    TYPE_DEFLATE,
    TYPE_BZIP2,
    TYPE_XZ
};

/* input context */
typedef struct {
    FILE *f;
    uint64_t fileSize;
    uint64_t compSize;
    uint64_t readSize;
    uint64_t cmrdSize;
    unsigned char compBuf[BUFFER_SIZE];
    z_stream zstrm;
    bz_stream bstrm;
    struct xz_buf xstrm;
    struct xz_dec *xz;
    char type;
    time_t start;
} input_t;

/**
 * Returns progress percentage and the status string in str
 */
int input_status(input_t *ctx, char *str);

/**
 * Open file and determine the source's format
 */
int input_open(input_t *ctx, char *fn);

/**
 * Read no more than BUFFER_SIZE uncompressed bytes of source data
 */
int input_read(input_t *ctx, char *buffer);

/**
 * Close source descriptors
 */
void input_close(input_t *ctx);
