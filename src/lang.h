/*
 * usbimager/lang.h
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
 * @brief Multilanguage definitions
 *
 */

#define NUMLANGS         16

enum {
    /* user interface */
    L_VERIFY,
    L_COMPRESS,
    L_WRITE,
    L_READ,
    L_SEND,
    L_SERIAL,
    L_MIB,
    L_GIB,
    /* messages */
    L_ERROR,
    L_WAITING,
    L_COMMERR,
    L_VRFYERR,
    L_WRTRGERR,
    L_WRIMGERR,
    L_RDSRCERR,
    L_TRGERR,
    L_DISMOUNTERR,
    L_UMOUNTERR,
    L_OPENVOLERR,
    L_OPENTRGERR,
    L_OPENIMGERR,
    L_ENCZIPERR,
    L_CMPZIPERR,
    L_CMPERR,
    L_SRCERR,
    L_DONE,
    L_STATHSMS,
    L_STATHSM,
    L_STATHMS,
    L_STATHM,
    L_STATMS,
    L_STATM,
    L_STATLM,
    L_SOFAR,
    /* X11 only (Open File dialog) */
    L_OK,
    L_WDAY0, L_WDAY1, L_WDAY2, L_WDAY3, L_WDAY4, L_WDAY5, L_WDAY6,
    L_YESTERDAY,
    L_NOW,
    L_MSAGO,
    L_HSAGO,
    L_HAGO,
    L_OPEN,
    L_CANCEL,
    L_RECENT,
    L_HOME,
    L_DESKTOP,
    L_DOWNLOADS,
    L_ROOTFS,
    L_NAME,
    L_SIZE,
    L_MODIFIED,
    L_ALLFILES,

    /* must be the last */
    NUMTEXTS
};

#ifdef WINVER
#include <windows.h>
extern wchar_t **lang;
#else
extern char **lang;
#endif
