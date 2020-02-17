/*
 * usbimager/input.c
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

#include <time.h>
#include <errno.h>
#include "input.h"

int verbose = 0;


/**
 * Returns progress percentage and the status string in str
 */
int input_status(input_t *ctx, char *str)
{
    time_t t = time(NULL);
    uint64_t d;
    int h,m,s;
    char rem[64];

    str[0] = 0;
    if(ctx->start < t && ctx->readSize) {
        if(ctx->fileSize)
            d = (t - ctx->start) * (ctx->fileSize - ctx->readSize) / ctx->readSize;
        else
            d = (t - ctx->start) * (ctx->compSize - ctx->cmrdSize) / ctx->cmrdSize;
        h = d / 3600; d %= 3600; m = d / 60; s = d % 60;
        if(h > 0) sprintf(rem, "%d hour%s and %d min%s left",
            h, h>1?"s":"", m, m>1?"s":"");
        else if(m > 0) sprintf(rem, "%d min%s and %d sec%s left",
            m, m>1?"s":"", s, s>1?"s":"");
        else sprintf(rem, "%d sec%s left", s, s>1?"s":"");
        if(ctx->fileSize)
            sprintf(str, "%6" SPFLG "u MiB / %" SPFLG "u MiB, %s",
                (ctx->readSize / 1024 / 1024),
                (ctx->fileSize / 1024 / 1024), rem);
        else
            sprintf(str, "%6" SPFLG "u MiB so far, %s",
                (ctx->readSize / 1024 / 1024), rem);
    }
    return ctx->fileSize ? (ctx->readSize * 100) / ctx->fileSize :
        (ctx->cmrdSize * 100) / (ctx->compSize+1);
}

/**
 * Open file and determine the source's format
 */
int input_open(input_t *ctx, char *fn)
{
    unsigned char hdr[65536], *buff;
    int x, y;

    errno = 0;
    memset(ctx, 0, sizeof(input_t));
    if(!fn || !*fn) return 1;

    if(verbose) printf("input_open(%s)\r\n", fn);

    ctx->f = fopen(fn, "rb");
    if(!ctx->f) return 1;
    memset(hdr, 0, sizeof(hdr));
    fread(hdr, sizeof(hdr), 1, ctx->f);

    /* detect input format */
    if(hdr[0] == 0x1f && hdr[1] == 0x8b) {
        /* gzip */
        if(verbose) printf(" gzip\r\n");
        fseek(ctx->f, -4L, SEEK_END);
        fread(&ctx->fileSize, 4, 1, ctx->f);
        ctx->compSize = (uint64_t)ftell(ctx->f) - 8;
        buff = hdr + 3;
        x = *buff++; buff += 6;
        if(x & 4) { y = *buff++; y += (*buff++ << 8); buff += y; }
        if(x & 8) { while(*buff++ != 0); }
        if(x & 16) { while(*buff++ != 0); }
        if(x & 2) buff += 2;
        ctx->compSize -= (uint64_t)(buff - hdr);
        fseek(ctx->f, (uint64_t)(buff - hdr), SEEK_SET);
        ctx->type = TYPE_DEFLATE;
    } else
    if(hdr[0] == 'B' && hdr[1] == 'Z' && hdr[2] == 'h') {
        /* bzip2 */
        if(verbose) printf(" bzip2\r\n");
        fseek(ctx->f, 0L, SEEK_END);
        ctx->compSize = (uint64_t)ftell(ctx->f);
        fseek(ctx->f, 0L, SEEK_SET);
        ctx->type = TYPE_BZIP2;
    } else
    if(hdr[0] == 0xFD && hdr[1] == '7' && hdr[2] == 'z' && hdr[3] == 'X' && hdr[4] == 'Z') {
        /* xz */
        if(verbose) printf(" xz\r\n");
        fseek(ctx->f, 0L, SEEK_END);
        ctx->compSize = (uint64_t)ftell(ctx->f);
        fseek(ctx->f, 0L, SEEK_SET);
        ctx->type = TYPE_XZ;
    } else
    if(hdr[0] == 'P' && hdr[1] == 'K' && hdr[2] == 3 && hdr[3] == 4) {
        /* pkzip */
        if(verbose) printf(" pkzip\r\n");
        if((hdr[6] & 1) || (hdr[6] & (1<<6))) {
            fclose(ctx->f);
            return 2;
        }
        switch(hdr[8]) {
            case 0: ctx->type = TYPE_PLAIN; break;
            case 8: ctx->type = TYPE_DEFLATE; break;
            case 12: ctx->type = TYPE_BZIP2; break;
            default: fclose(ctx->f); return 3;
        }
        if(memcmp(hdr + 18, "\xff\xff\xff\xff\xff\xff\xff\xff", 8)) {
            memcpy(&ctx->compSize, hdr + 18, 4);
            memcpy(&ctx->fileSize, hdr + 22, 4);
        } else {
            /* zip64 */
            if(verbose) printf("   zip64\r\n");
            for(x = 30 + hdr[26] + (hdr[27]<<8), y = x + hdr[28] + (hdr[29]<<8);
                x < y && x < (int)sizeof(hdr) - 4; x += 4 + hdr[x + 2] + (hdr[x + 3]<<8))
                    if(hdr[x] == 1 && hdr[x + 1] == 0) {
                        memcpy(&ctx->compSize, hdr + x + 12, 8);
                        memcpy(&ctx->fileSize, hdr + x + 4, 8);
                        break;
                    }
            if(!ctx->compSize || !ctx->fileSize) { fclose(ctx->f); return 4; }
        }
        fseek(ctx->f, (uint64_t)(30 + hdr[26] + (hdr[27]<<8) + hdr[28] + (hdr[29]<<8)), SEEK_SET);
    } else {
        /* uncompressed image */
        if(verbose) printf(" raw image\r\n");
        fseek(ctx->f, 0L, SEEK_END);
        ctx->fileSize = (uint64_t)ftell(ctx->f);
        fseek(ctx->f, 0L, SEEK_SET);
        ctx->type = TYPE_PLAIN;
    }
    switch(ctx->type) {
        case TYPE_DEFLATE:
            x = inflateInit2(&ctx->zstrm, -MAX_WBITS);
            if (x != Z_OK) { fclose(ctx->f); return 4; }
        break;
        case TYPE_BZIP2:
            x = BZ2_bzDecompressInit(&ctx->bstrm, 0, 0);
            if (x != BZ_OK) { fclose(ctx->f); return 4; }
        break;
        case TYPE_XZ:
            xz_crc32_init();
            xz_crc64_init();
            ctx->xz = xz_dec_init(XZ_DYNALLOC, 1 << 26);
            if (!ctx->xz) { fclose(ctx->f); return 4; }
        break;
    }
    if(!ctx->compSize && !ctx->fileSize) { fclose(ctx->f); return 1; }
    if(verbose) printf("  type %d compSize %" SPFLG "u fileSize %" SPFLG
        "u data offset %" SPFLG "u\r\n",
        ctx->type, ctx->compSize, ctx->fileSize, (uint64_t)ftell(ctx->f));

    ctx->start = time(NULL);
    return 0;
}

/**
 * Read no more than BUFFER_SIZE uncompressed bytes of source data
 */
int input_read(input_t *ctx, char *buffer)
{
    int ret = 0;
    int64_t size = 0, insiz;

    errno = 0;
    size = ctx->fileSize - ctx->readSize;
    if(size < 1) { if(ctx->fileSize) return 0; size = 0; }
    if(size > BUFFER_SIZE) size = BUFFER_SIZE;
    if(verbose)
        printf("input_read() readSize %" SPFLG "u / fileSize %" SPFLG "u (input size %"
            SPFLG "d), cmrdSize %" SPFLG "u / compSize %" SPFLG "u\r\n",
            ctx->readSize, ctx->fileSize, size, ctx->cmrdSize, ctx->compSize);
    
    switch(ctx->type) {
        case TYPE_PLAIN:
            fread(buffer, size, 1, ctx->f);
        break;
        case TYPE_DEFLATE:
            ctx->zstrm.next_out = (unsigned char*)buffer;
            ctx->zstrm.avail_out = size;
            do {
                if(!ctx->zstrm.avail_in) {
                    insiz = ctx->compSize - ctx->cmrdSize;
                    if(insiz < 1) { ret = Z_STREAM_END; break; }
                    if(insiz > BUFFER_SIZE) insiz = BUFFER_SIZE;
                    if(verbose) printf("  deflate cmrdSize %" SPFLG
                        "u insiz %" SPFLG "d\r\n", ctx->cmrdSize, insiz);
                    ctx->zstrm.next_in = ctx->compBuf;
                    ctx->zstrm.avail_in = insiz;
                    if(!fread(&ctx->compBuf, insiz, 1, ctx->f)) break;
                    ctx->cmrdSize += (uint64_t)insiz;
                }
                ret = inflate(&ctx->zstrm, Z_NO_FLUSH);
            } while(ret == Z_OK && ctx->zstrm.avail_out > 0);
            if(ret != Z_OK && ret != Z_STREAM_END) {
                if(verbose) printf("  zlib inflate error %d\r\n", ret);
                return -1;
            }
        break;
        case TYPE_BZIP2:
            ctx->bstrm.next_out = buffer;
            ctx->bstrm.avail_out = BUFFER_SIZE;
            do {
                if(!ctx->bstrm.avail_in) {
                    insiz = ctx->compSize - ctx->cmrdSize;
                    if(insiz < 1) { ret = BZ_STREAM_END; break; }
                    if(insiz > BUFFER_SIZE) insiz = BUFFER_SIZE;
                    if(verbose) printf("  bzip2 cmrdSize %" SPFLG
                        "u insiz %" SPFLG "d\r\n", ctx->cmrdSize, insiz);
                    ctx->bstrm.next_in = (char*)ctx->compBuf;
                    ctx->bstrm.avail_in = insiz;
                    if(!fread(&ctx->compBuf, insiz, 1, ctx->f)) break;
                    ctx->cmrdSize += (uint64_t)insiz;
                }
                ret = BZ2_bzDecompress(&ctx->bstrm);
            } while(ret == BZ_OK && ctx->bstrm.avail_out > 0);
            if(ret != BZ_OK && ret != BZ_STREAM_END) {
                if(verbose) printf("  bzip2 decompress error %d\r\n", ret);
                return -1;
            }
            size = BUFFER_SIZE - ctx->bstrm.avail_out;
        break;
        case TYPE_XZ:
            ctx->xstrm.out = (unsigned char*)buffer;
            ctx->xstrm.out_pos = 0;
            ctx->xstrm.out_size = BUFFER_SIZE;
            do {
                if(ctx->xstrm.in_pos == ctx->xstrm.in_size) {
                    insiz = ctx->compSize - ctx->cmrdSize;
                    if(insiz < 1) { ret = XZ_STREAM_END; break; }
                    if(insiz > BUFFER_SIZE) insiz = BUFFER_SIZE;
                    if(verbose) printf("  xz cmrdSize %" SPFLG
                        "u insiz %" SPFLG "d\r\n", ctx->cmrdSize, insiz);
                    ctx->xstrm.in = (unsigned char*)ctx->compBuf;
                    ctx->xstrm.in_pos = 0;
                    ctx->xstrm.in_size = insiz;
                    if(!fread(&ctx->compBuf, insiz, 1, ctx->f)) break;
                    if(insiz < BUFFER_SIZE)
                        memset(ctx->compBuf + insiz, 0, BUFFER_SIZE - insiz);
                    ctx->cmrdSize += (uint64_t)insiz;
                }
                ret = xz_dec_run(ctx->xz, &ctx->xstrm);
                if(ret == XZ_UNSUPPORTED_CHECK) ret = XZ_OK;
            } while(ret == XZ_OK && ctx->xstrm.out_pos < ctx->xstrm.out_size);
            if(ret != XZ_OK && ret != XZ_STREAM_END) {
                if(verbose) printf("  xz decompress error %d\r\n", ret);
                return -1;
            }
            size = ctx->xstrm.out_pos;
        break;
    }
    while(size & 511) buffer[size++] = 0;
    if(verbose) printf("input_read() output size %" SPFLG "d\r\n", size);
    ctx->readSize += (uint64_t)size;
    return size;
}

/**
 * Close source descriptors
 */
void input_close(input_t *ctx)
{
    if(ctx->f) fclose(ctx->f);
    switch(ctx->type) {
        case TYPE_DEFLATE: inflateEnd(&ctx->zstrm); break;
        case TYPE_BZIP2: BZ2_bzDecompressEnd(&ctx->bstrm); break;
        case TYPE_XZ: xz_dec_end(ctx->xz); break;
    }
}
