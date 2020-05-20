/*
 * usbimager/stream.c
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

#include <time.h>
#include <errno.h>
#include <sys/types.h>
#include "lang.h"
#include "stream.h"

#ifdef WINVER
#include <windows.h>
extern int _fileno(FILE *f);
#else
extern int fileno(FILE *f);
#endif
#if !defined(WINVER) && !defined(MACOSX)
uint64_t mytell (FILE * stream)
{ fpos_t pos; return (fgetpos(stream, &pos)) ? 0L : (uint64_t)(pos.__pos); }
int myseek (FILE* stream, uint64_t offset)
{ fpos_t pos = {0}; pos.__pos = offset; return fsetpos(stream, &pos); }
#else
uint64_t mytell (FILE * stream)
{ fpos_t pos; return (fgetpos(stream, &pos)) ? 0LL : (uint64_t)(pos); }
int myseek (FILE* stream, uint64_t offset)
{ fpos_t pos = (fpos_t)offset; return fsetpos(stream, &pos); }
#endif

int verbose = 0;
int buffer_size = 1024*1024;

/**
 * Returns progress percentage and the status string in str
 */
int stream_status(stream_t *ctx, char *str, int done)
{
    time_t t = time(NULL);
    uint64_t d = 0;
    int h,m,s;
#ifdef WINVER
    wchar_t rem[64];
#else
    char rem[64];
#endif
    if(done) {
        memset(str, 0, 2);
        if(ctx->fileSize && ctx->readSize >= ctx->fileSize
#ifndef WINVER
            && !errno
#endif
        ) {
            d = t - ctx->start;
            h = d / 3600; d %= 3600; m = d / 60; s = d % 60;
            if(h<0 || h>23) h = 0;
            if(m<0) m = 0;
            if(s<0) s = 0;
#ifdef WINVER
            wsprintfW((wchar_t*)str, lang[L_DONE], h, m, s);
#else
            sprintf(str, lang[L_DONE], h, m, s);
#endif
        }
        return 0;
    }
    rem[0] = 0;
    if(ctx->start < t) {
        if(ctx->readSize) {
            if(ctx->fileSize)
                d = ctx->readSize / (t - ctx->start);
            else
                d = ctx->cmrdSize / (t - ctx->start);
            ctx->avgSpeedBytes += d;
            ctx->avgSpeedNum++;
            d = ctx->avgSpeedBytes / ctx->avgSpeedNum;
            if(verbose) printf("  average speed %" PRIu64" bytes / sec\r\n", d);
        }
        if(ctx->avgSpeedNum > 2) {
            d = d ? (ctx->fileSize ? ctx->fileSize - ctx->readSize : ctx->compSize - ctx->cmrdSize) / d : 0;
            h = d / 3600; d %= 3600; m = d / 60; if(h<0 || h>23) h = 0; if(m<0) m = 0;
#ifdef WINVER
            if(h > 0) wsprintfW(rem, (wchar_t*)lang[h>1 && m>1 ? L_STATHSMS : (h>1 && m<2 ? L_STATHSM :
                    (h==1 && m>0 ? L_STATHMS : L_STATHM))], h, m);
            else if(m > 0) wsprintfW(rem, (wchar_t*)lang[m>1 ? L_STATMS : L_STATM], m);
            else wsprintfW(rem, (wchar_t*)lang[L_STATLM]);
#else
            if(h > 0) sprintf(rem, lang[h>1 && m>1 ? L_STATHSMS : (h>1 && m<2 ? L_STATHSM :
                    (h==1 && m>0 ? L_STATHMS : L_STATHM))], h, m);
            else if(m > 0) sprintf(rem, lang[m>1 ? L_STATMS : L_STATM], m);
            else strcpy(rem, lang[L_STATLM]);
#endif
        }
    }
#ifdef WINVER
    if(ctx->fileSize)
        wsprintfW((wchar_t*)str, L"%6" PRIu64 " %s / %" PRIu64 " %s%s%s",
            (ctx->readSize >> 20), lang[L_MIB],
            (ctx->fileSize >> 20), lang[L_MIB], rem[0] ? L", " : L"", rem);
    else
        wsprintfW((wchar_t*)str, L"%6" PRIu64 " %s %s%s%s",
            (ctx->readSize >> 20), lang[L_MIB], lang[L_SOFAR], rem[0] ? L", " : L"", rem);
#else
    if(ctx->fileSize)
        sprintf(str, "%6" PRIu64 " %s / %" PRIu64 " %s%s%s",
            (ctx->readSize >> 20), lang[L_MIB],
            (ctx->fileSize >> 20), lang[L_MIB], rem[0] ? ", " : "", rem);
    else
        sprintf(str, "%6" PRIu64 " %s %s%s%s",
            (ctx->readSize >> 20), lang[L_MIB], lang[L_SOFAR], rem[0] ? ", " : "", rem);
#endif
    d = ctx->fileSize ? (ctx->readSize * 1000) / (ctx->fileSize * 10) :
        (ctx->cmrdSize * 1000) / (ctx->compSize * 10 + 1);
    /* readSize can be greater than fileSize because it's rounded up to 512 bytes */
    return d > 100 ? 100 : d;
}

/**
 * Open file and determine the source's format
 */
int stream_open(stream_t *ctx, char *fn, int uncompr)
{
    unsigned char hdr[65536], *buff;
    uint64_t fs = 0;
    int x, y;
#ifndef WINVER
    struct stat st;
#endif

    errno = 0;
    memset(ctx, 0, sizeof(stream_t));
    if(!fn || !*fn) return 1;

    if(verbose) printf("stream_open(%s)\r\n", fn);

    ctx->compBuf = (unsigned char*)malloc(buffer_size);
    if(!ctx->compBuf) {
        main_getErrorMessage();
        return 1;
    }
    ctx->verifyBuf = (char*)malloc(buffer_size);
    if(!ctx->verifyBuf) {
        main_getErrorMessage();
        free(ctx->compBuf); ctx->compBuf = NULL;
        return 1;
    }
    ctx->buffer = (char*)malloc(buffer_size);
    if(!ctx->buffer) {
        main_getErrorMessage();
        free(ctx->compBuf); ctx->compBuf = NULL;
        free(ctx->verifyBuf); ctx->verifyBuf = NULL;
        return 1;
    }

    ctx->f = fopen(fn, "rb");
    if(!ctx->f) return 1;
#ifdef WINVER
    fs = (uint64_t)_filelengthi64(_fileno(ctx->f));
#else
    if(!fstat(fileno(ctx->f), &st))
        fs = (uint64_t)st.st_size;
#endif
    memset(hdr, 0, sizeof(hdr));
    if(!uncompr) {
        if(!fread(hdr, sizeof(hdr), 1, ctx->f)) {}
    }

    /* detect input format */
    if(hdr[0] == 0x1f && hdr[1] == 0x8b) {
        /* gzip */
        if(verbose) printf(" gzip\r\n");
        myseek(ctx->f, fs - 4L);
        if(!fread(&ctx->fileSize, 4, 1, ctx->f))
            ctx->fileSize = 0;
        ctx->compSize = fs - 8;
        buff = hdr + 3;
        x = *buff++; buff += 6;
        if(x & 4) { y = *buff++; y += (*buff++ << 8); buff += y; }
        if(x & 8) { while(*buff++ != 0); }
        if(x & 16) { while(*buff++ != 0); }
        if(x & 2) buff += 2;
        ctx->compSize -= (uint64_t)(buff - hdr);
        myseek(ctx->f, (uint64_t)(buff - hdr));
        ctx->type = TYPE_DEFLATE;
    } else
    if(hdr[0] == 'B' && hdr[1] == 'Z' && hdr[2] == 'h') {
        /* bzip2 */
        if(verbose) printf(" bzip2\r\n");
        ctx->compSize = fs;
        myseek(ctx->f, 0L);
        ctx->type = TYPE_BZIP2;
    } else
    if(hdr[0] == 0xFD && hdr[1] == '7' && hdr[2] == 'z' && hdr[3] == 'X' && hdr[4] == 'Z') {
        /* xz */
        if(verbose) printf(" xz\r\n");
        ctx->compSize = fs;
        myseek(ctx->f, 0L);
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
        myseek(ctx->f, (uint64_t)(30 + hdr[26] + (hdr[27]<<8) + hdr[28] + (hdr[29]<<8)));
    } else
    if(hdr[0] == '7' && hdr[1] == 'z' && hdr[2] == 0xBC && hdr[3] == 0xAF) {
        /* 7zip */
        if(verbose) printf(" 7z (deliberately not supported, use xz instead)\r\n");
        /* this is a badly designed, non-transmission-error-proof, inadeqvate for long term preservation format
         * with a particularly badly written, badly documented, non-portable SDK which I simply refuse to support.
         * Don't use this format if your data is precious to you. Better to use tar.xz.  You have been warned. */
        fclose(ctx->f); return 3;
    } else {
        /* uncompressed image */
        if(verbose) printf(" raw image\r\n");
        ctx->fileSize = fs;
        myseek(ctx->f, 0L);
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
    if(verbose) printf(" type %d compSize %" PRIu64 " fileSize %" PRIu64
        " data offset %" PRIu64 "\r\n",
        ctx->type, ctx->compSize, ctx->fileSize, mytell(ctx->f));
    if(!ctx->compSize && !ctx->fileSize) { fclose(ctx->f); return 1; }

    ctx->start = time(NULL);
    return 0;
}

/**
 * Read no more than buffer_size uncompressed bytes of source data
 */
int stream_read(stream_t *ctx)
{
    int ret = 0;
    int64_t size = 0, insiz;

    errno = 0;
    size = ctx->fileSize - ctx->readSize;
    if(size < 1) { if(ctx->fileSize) return 0; size = 0; }
    if(size > buffer_size) size = buffer_size;
    if(verbose)
        printf("stream_read() readSize %" PRIu64 " / fileSize %" PRIu64 " (input size %"
            PRId64 "), cmrdSize %" PRIu64 " / compSize %" PRIu64 "u\r\n",
            ctx->readSize, ctx->fileSize, size, ctx->cmrdSize, ctx->compSize);

    switch(ctx->type) {
        case TYPE_PLAIN:
            if(!fread(ctx->buffer, size, 1, ctx->f)) {}
        break;
        case TYPE_DEFLATE:
            ctx->zstrm.next_out = (unsigned char*)ctx->buffer;
            ctx->zstrm.avail_out = size;
            do {
                if(!ctx->zstrm.avail_in) {
                    insiz = ctx->compSize - ctx->cmrdSize;
                    if(insiz < 1) { ret = Z_STREAM_END; break; }
                    if(insiz > buffer_size) insiz = buffer_size;
                    if(verbose) printf("  deflate cmrdSize %" PRIu64
                        " insiz %" PRId64 "\r\n", ctx->cmrdSize, insiz);
                    ctx->zstrm.next_in = ctx->compBuf;
                    ctx->zstrm.avail_in = insiz;
                    if(!fread(ctx->compBuf, insiz, 1, ctx->f)) break;
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
            ctx->bstrm.next_out = ctx->buffer;
            ctx->bstrm.avail_out = buffer_size;
            do {
                if(!ctx->bstrm.avail_in) {
                    insiz = ctx->compSize - ctx->cmrdSize;
                    if(insiz < 1) { ret = BZ_STREAM_END; break; }
                    if(insiz > buffer_size) insiz = buffer_size;
                    if(verbose) printf("  bzip2 cmrdSize %" PRIu64
                        " insiz %" PRId64 "\r\n", ctx->cmrdSize, insiz);
                    ctx->bstrm.next_in = (char*)ctx->compBuf;
                    ctx->bstrm.avail_in = insiz;
                    if(!fread(ctx->compBuf, insiz, 1, ctx->f)) break;
                    ctx->cmrdSize += (uint64_t)insiz;
                }
                ret = BZ2_bzDecompress(&ctx->bstrm);
            } while(ret == BZ_OK && ctx->bstrm.avail_out > 0);
            if(ret != BZ_OK && ret != BZ_STREAM_END) {
                if(verbose) printf("  bzip2 decompress error %d\r\n", ret);
                return -1;
            }
            size = buffer_size - ctx->bstrm.avail_out;
        break;
        case TYPE_XZ:
            ctx->xstrm.out = (unsigned char*)ctx->buffer;
            ctx->xstrm.out_pos = 0;
            ctx->xstrm.out_size = buffer_size;
            do {
                if(ctx->xstrm.in_pos == ctx->xstrm.in_size) {
                    insiz = ctx->compSize - ctx->cmrdSize;
                    if(insiz < 1) { ret = XZ_STREAM_END; break; }
                    if(insiz > buffer_size) insiz = buffer_size;
                    if(verbose) printf("  xz cmrdSize %" PRIu64
                        " insiz %" PRId64 "\r\n", ctx->cmrdSize, insiz);
                    ctx->xstrm.in = (unsigned char*)ctx->compBuf;
                    ctx->xstrm.in_pos = 0;
                    ctx->xstrm.in_size = insiz;
                    if(!fread(ctx->compBuf, insiz, 1, ctx->f)) break;
                    if(insiz < buffer_size)
                        memset(ctx->compBuf + insiz, 0, buffer_size - insiz);
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
    while(size & 511) ctx->buffer[size++] = 0;
    if(verbose) printf("stream_read() output size %" PRId64 "\r\n", size);
    ctx->readSize += (uint64_t)size;
    return size;
}

/**
 * Open file for writing
 */
int stream_create(stream_t *ctx, char *fn, int comp, uint64_t size)
{
    errno = 0;
    memset(ctx, 0, sizeof(stream_t));
    if(!fn || !*fn || !size) return 1;

    if(verbose)
        printf("stream_create(%s) comp %d size %" PRIu64 ")\r\n", fn, comp, size);

    ctx->compBuf = (unsigned char*)malloc(buffer_size);
    if(!ctx->compBuf) {
        main_getErrorMessage();
        return 1;
    }
    ctx->buffer = (char*)malloc(buffer_size);
    if(!ctx->buffer) {
        main_getErrorMessage();
        free(ctx->compBuf); ctx->compBuf = NULL;
        return 1;
    }

    if(comp) {
        ctx->type = TYPE_BZIP2;
        ctx->b = BZ2_bzopen(fn, "wb");
        if(!ctx->b) {
            main_getErrorMessage();
            free(ctx->buffer); ctx->buffer = NULL;
            free(ctx->compBuf); ctx->compBuf = NULL;
            return 1;
        }
    } else {
        ctx->type = TYPE_PLAIN;
        ctx->f = fopen(fn, "wb");
        if(!ctx->f) {
            main_getErrorMessage();
            free(ctx->buffer); ctx->buffer = NULL;
            free(ctx->compBuf); ctx->compBuf = NULL;
            return 1;
        }
    }

    ctx->fileSize = size;
    ctx->start = time(NULL);
    return 0;
}

/**
 * Compress and write out data
 */
int stream_write(stream_t *ctx, char *buffer, int size)
{
    if(verbose)
        printf("stream_write() readSize %" PRIu64 " / fileSize %" PRIu64 " (output size %d)\r\n",
            ctx->readSize, ctx->fileSize, size);
    errno = 0;
    ctx->readSize += (uint64_t)size;

    switch(ctx->type) {
        case TYPE_PLAIN:
            if(!fwrite(buffer, size, 1, ctx->f))
                size = 0;
        break;
        case TYPE_BZIP2:
            if(BZ2_bzwrite(ctx->b, buffer, size) < 1)
                size = 0;
        break;
    }
    if(verbose) printf("stream_write() output size %d\r\n", size);
    return size;
}

/**
 * Close stream descriptors
 */
void stream_close(stream_t *ctx)
{
    if(ctx->compBuf) free(ctx->compBuf);
    if(ctx->verifyBuf) free(ctx->verifyBuf);
    if(ctx->buffer) free(ctx->buffer);
    if(ctx->f) fclose(ctx->f);
    switch(ctx->type) {
        case TYPE_DEFLATE: inflateEnd(&ctx->zstrm); break;
        case TYPE_BZIP2: if(ctx->b) BZ2_bzclose(ctx->b); else BZ2_bzDecompressEnd(&ctx->bstrm); break;
        case TYPE_XZ: xz_dec_end(ctx->xz); break;
    }
}
