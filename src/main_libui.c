/*
 * usbimager/main_libui.c
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
 * @brief User interface for Linux and MacOSX using libui
 *
 */

#ifdef MACOSX
#import <CoreFoundation/CoreFoundation.h>
#endif
#include <pthread.h>
#include <errno.h>
#include <sys/stat.h>
#include "lang.h"
#include "stream.h"
#include "disks.h"
#include "libui/ui.h"

char **lang = NULL;
extern char *dict[NUMLANGS][NUMTEXTS + 1];

static uiWindow *mainwin;
static uiButton *sourceButton;
static uiEntry *source;
static uiBox *targetCont;
static uiCombobox *target;
static uiCheckbox *verify;
static uiCheckbox *compr;
static uiButton *writeButton;
static uiButton *readButton;
static uiProgressBar *pbar;
static uiLabel *status;
pthread_t thrd;
pthread_attr_t tha;
char *main_errorMessage;

void main_addToCombobox(char *option)
{
    uiComboboxAppend(target, option);
}

void main_getErrorMessage()
{
    main_errorMessage = errno ? strerror(errno) : NULL;
}

static void onDone(void *data)
{
    uiControlEnable(uiControl(source));
    uiControlEnable(uiControl(sourceButton));
    uiControlEnable(uiControl(target));
    uiControlEnable(uiControl(writeButton));
    uiControlEnable(uiControl(readButton));
    uiControlEnable(uiControl(verify));
    uiControlEnable(uiControl(compr));
    uiProgressBarSetValue(pbar, 0);
    uiLabelSetText(status, (char*)data);
    main_errorMessage = NULL;
}

static void onProgress(void *data)
{
    char textstat[128];
    int pos;

    pos = stream_status((stream_t*)data, textstat);
    uiProgressBarSetValue(pbar, pos);
    uiLabelSetText(status, textstat);
#ifdef MACOSX
    uiMainStep(0);
    CFRunLoopRun();
#endif
}

static void onThreadError(void *data)
{
    uiMsgBoxError(mainwin, main_errorMessage && *main_errorMessage ? main_errorMessage : lang[L_ERROR], (char*)data);
}

/**
 * Function that reads from input and writes to disk
 */
static void *writerRoutine(void *data)
{
    int dst, needVerify = uiCheckboxChecked(verify), numberOfBytesRead;
    int numberOfBytesWritten, numberOfBytesVerify;
    static stream_t ctx;
    (void)data;

    ctx.fileSize = 0;
    dst = stream_open(&ctx, uiEntryText(source));
    if(!dst) {
        dst = (int)((long int)disks_open(uiComboboxSelected(target)));
        if(dst > 0) {
            while(1) {
                if((numberOfBytesRead = stream_read(&ctx, ctx.buffer)) >= 0) {
                    if(numberOfBytesRead == 0) {
                        if(!ctx.fileSize) ctx.fileSize = ctx.readSize;
                        break;
                    } else {
                        errno = 0;
                        numberOfBytesWritten = (int)write(dst, ctx.buffer, numberOfBytesRead);
                        if(verbose) printf("writerRoutine() numberOfBytesRead %d numberOfBytesWritten %d errno=%d\n",
                            numberOfBytesRead, numberOfBytesWritten, errno);
                        if(numberOfBytesWritten == numberOfBytesRead) {
                            if(needVerify) {
                                lseek(dst, -((off_t)numberOfBytesWritten), SEEK_CUR);
                                numberOfBytesVerify = read(dst, ctx.verifyBuf, numberOfBytesWritten);
                                if(verbose) printf("  numberOfBytesVerify %d\n", numberOfBytesVerify);
                                if(numberOfBytesVerify != numberOfBytesWritten ||
                                    memcmp(ctx.buffer, ctx.verifyBuf, numberOfBytesWritten)) {
                                        uiQueueMain(onThreadError, lang[L_VRFYERR]);
                                    break;
                                }
                            }
                            uiQueueMain(onProgress, &ctx);
                        } else {
                            if(errno) main_errorMessage = strerror(errno);
                            uiQueueMain(onThreadError, lang[L_WRTRGERR]);
                            break;
                        }
                    }
                } else {
                    uiQueueMain(onThreadError, lang[L_RDSRCERR]);
                    break;
                }
            }
            disks_close((void*)((long int)dst));
        } else {
            uiQueueMain(onThreadError, lang[dst == -1 ? L_TRGERR : (dst == -2 ? L_UMOUNTERR : L_OPENTRGERR)]);
        }
        stream_close(&ctx);
    } else {
        if(errno) main_errorMessage = strerror(errno);
        uiQueueMain(onThreadError, lang[dst == 2 ? L_ENCZIPERR : (dst == 3 ? L_CMPZIPERR : (dst == 4 ? L_CMPERR : L_SRCERR))]);
    }
    uiQueueMain(onDone, !errno && ctx.fileSize && ctx.readSize >= ctx.fileSize ? lang[L_DONE] : "");
    if(verbose) printf("Worker thread finished.\r\n");
    return NULL;
}

static void onWriteButtonClicked(uiButton *b, void *data)
{
    (void)b;
    (void)data;
    uiControlDisable(uiControl(source));
    uiControlDisable(uiControl(sourceButton));
    uiControlDisable(uiControl(target));
    uiControlDisable(uiControl(writeButton));
    uiControlDisable(uiControl(readButton));
    uiControlDisable(uiControl(verify));
    uiControlDisable(uiControl(compr));
    uiProgressBarSetValue(pbar, 0);
    uiLabelSetText(status, "");
    main_errorMessage = NULL;

    if(verbose) printf("Starting worker thread\r\n");
#ifndef MACOSX
    pthread_create(&thrd, &tha, writerRoutine, NULL);
#else
    CFRunLoopRun();
    writerRoutine(NULL);
#endif
}

/**
 * Function that reads from disk and writes to output file
 */
static void *readerRoutine(void *data)
{
    int src, size, needCompress = uiCheckboxChecked(compr), numberOfBytesRead;
    static stream_t ctx;
    char *env, fn[PATH_MAX];
    struct stat st;
    struct tm *lt;
    time_t now = time(NULL);
    int i, targetId = uiComboboxSelected(target);
    (void)data;

    ctx.fileSize = 0;
    src = (int)((long int)disks_open(targetId));
    if(src > 0) {
        fn[0] = 0;
        if((env = getenv("HOME")))
            strncpy(fn, env, sizeof(fn)-1);
        else if((env = getenv("LOGNAME")))
            snprintf(fn, sizeof(fn)-1, "/home/%s", env);
        if(!fn[0]) strcpy(fn, "./");
        i = strlen(fn);
        strncpy(fn + i, "/Desktop", sizeof(fn)-1-i);
        if(stat(fn, &st)) {
            strncpy(fn + i, "/Downloads", sizeof(fn)-1-i);
            if(stat(fn, &st)) fn[i] = 0;
        }
        i = strlen(fn);
        lt = localtime(&now);
        snprintf(fn + i, sizeof(fn)-1-i, "/usbimager-%04d%02d%02d%02d%02d.dd%s",
            lt->tm_year+1900, lt->tm_mon+1, lt->tm_mday, lt->tm_hour, lt->tm_min,
            needCompress ? ".bz2" : "");
        uiEntrySetText(source, fn);

        if(!stream_create(&ctx, fn, needCompress, disks_capacity[targetId])) {
            while(ctx.readSize < ctx.fileSize) {
                errno = 0;
                size = ctx.fileSize - ctx.readSize < BUFFER_SIZE ? (int)(ctx.fileSize - ctx.readSize) : BUFFER_SIZE;
                numberOfBytesRead = (int)read(src, ctx.buffer, size);
                if(numberOfBytesRead == size) {
                    if(stream_write(&ctx, ctx.buffer, size)) {
                        uiQueueMain(onProgress, &ctx);
                    } else {
                        if(errno) main_errorMessage = strerror(errno);
                        uiQueueMain(onThreadError, lang[L_WRIMGERR]);
                    }
                } else {
                    if(errno) main_errorMessage = strerror(errno);
                    uiQueueMain(onThreadError, lang[L_RDSRCERR]);
                    break;
                }
            }
            stream_close(&ctx);
        } else {
            if(errno) main_errorMessage = strerror(errno);
            uiQueueMain(onThreadError, lang[L_OPENIMGERR]);
        }
        disks_close((void*)((long int)src));
    } else {
        uiQueueMain(onThreadError, lang[src == -1 ? L_TRGERR : (src == -2 ? L_UMOUNTERR : L_OPENTRGERR)]);
    }
    uiLabelSetText(status, !errno && ctx.fileSize && ctx.readSize >= ctx.fileSize ? lang[L_DONE] : "");
    if(verbose) printf("Worker thread finished.\r\n");
    return NULL;
}

static void onReadButtonClicked(uiButton *b, void *data)
{
    (void)b;
    (void)data;

    uiControlDisable(uiControl(source));
    uiControlDisable(uiControl(sourceButton));
    uiControlDisable(uiControl(target));
    uiControlDisable(uiControl(writeButton));
    uiControlDisable(uiControl(readButton));
    uiControlDisable(uiControl(verify));
    uiControlDisable(uiControl(compr));
    uiProgressBarSetValue(pbar, 0);
    uiLabelSetText(status, "");
    main_errorMessage = NULL;
    if(verbose) printf("Starting worker thread\r\n");
#ifndef MACOSX
    pthread_create(&thrd, &tha, readerRoutine, NULL);
#else
    CFRunLoopRun();
    readerRoutine(NULL);
#endif
}

static void refreshTarget(uiCombobox *cb, void *data)
{
    int current = uiComboboxSelected(target);
    (void)cb;
    (void)data;
    uiBoxDelete(targetCont, 0);
    target = uiNewCombobox();
    uiBoxAppend(targetCont, uiControl(target), 1);
    uiComboboxOnSelected(target, refreshTarget, NULL);
    disks_refreshlist();
    uiComboboxSetSelected(target, current);
}

static void onSelectClicked(uiButton *b, void *data)
{
    uiEntry *entry = uiEntry(data);
    char *filename;

    (void)b;
    refreshTarget(target, NULL);
    uiLabelSetText(status, "");
    filename = uiOpenFile(mainwin);
    if (filename == NULL) return;
    uiEntrySetText(entry, filename);
    uiFreeText(filename);
}

static int onClosing(uiWindow *w, void *data)
{
    (void)w;
    (void)data;
    if(thrd) { pthread_cancel(thrd); thrd = 0; }
    uiQuit();
    return 1;
}

static int onShouldQuit(void *data)
{
    if(thrd) { pthread_cancel(thrd); thrd = 0; }
    uiControlDestroy(uiControl(uiWindow(data)));
    return 1;
}

int main(int argc, char **argv)
{
    uiInitOptions options;
    const char *err;
    uiGrid *grid;
    uiBox *vbox;
    uiBox *bbox;
    uiLabel *sep;
    int i;
    char *lc = getenv("LANG"), btntext[256];

    if(argc > 1 && argv[1] && argv[1][0] == '-')
        for(i = 1; argv[1][i]; i++)
            if(argv[1][1] == 'v') verbose++;

    if(!lc) lc = "en";
    for(i = 0; i < NUMLANGS; i++) {
        if(!memcmp(lc, dict[i][0], strlen(dict[i][0]))) {
            lang = &dict[i][1];
            break;
        }
    }
    if(!lang) lang = &dict[0][1];

    pthread_attr_init(&tha);
    memset(&thrd, 0, sizeof(pthread_t));

    memset(&options, 0, sizeof (uiInitOptions));
    err = uiInit(&options);
    if (err != NULL) {
        fprintf(stderr, "error initializing libui: %s", err);
        uiFreeInitError(err);
        return 1;
    }

    mainwin = uiNewWindow("USBImager", 320, 160, 1);
    uiWindowOnClosing(mainwin, onClosing, NULL);
    uiOnShouldQuit(onShouldQuit, mainwin);
    uiWindowSetMargined(mainwin, 1);

    grid = uiNewGrid();
    uiGridSetPadded(grid, 1);
    uiWindowSetChild(mainwin, uiControl(grid));

    sourceButton = uiNewButton("...");
    source = uiNewEntry();
    uiButtonOnClicked(sourceButton, onSelectClicked, source);
    uiGridAppend(grid, uiControl(source), 0, 0, 7, 1, 1, uiAlignFill, 0, uiAlignFill);
    uiGridAppend(grid, uiControl(sourceButton), 7, 0, 1, 1, 0, uiAlignFill, 0, uiAlignFill);

    bbox = uiNewHorizontalBox();

    uiGridAppend(grid, uiControl(bbox), 0, 1, 8, 1, 0, uiAlignFill, 0, uiAlignFill);
    snprintf(btntext, sizeof(btntext)-1, "▼ %s", lang[L_WRITE]);
    writeButton = uiNewButton(btntext);
    uiButtonOnClicked(writeButton, onWriteButtonClicked, NULL);
    uiBoxAppend(bbox, uiControl(writeButton), 1);

    sep = uiNewLabel(" ");
    uiBoxAppend(bbox, uiControl(sep), 0);

    snprintf(btntext, sizeof(btntext)-1, "▲ %s", lang[L_READ]);
    readButton = uiNewButton(btntext);
    uiButtonOnClicked(readButton, onReadButtonClicked, NULL);
    uiBoxAppend(bbox, uiControl(readButton), 1);

    targetCont = uiNewHorizontalBox();
    uiGridAppend(grid, uiControl(targetCont), 0, 2, 8, 1, 0, uiAlignFill, 0, uiAlignFill);
    target = uiNewCombobox();
    uiBoxAppend(targetCont, uiControl(target), 1);
    uiComboboxOnSelected(target, refreshTarget, NULL);
    refreshTarget(target, NULL);

    verify = uiNewCheckbox(lang[L_VERIFY]);
    uiGridAppend(grid, uiControl(verify), 0, 3, 4, 1, 0, uiAlignFill, 0, uiAlignFill);

    compr = uiNewCheckbox(lang[L_COMPRESS]);
    uiGridAppend(grid, uiControl(compr), 4, 3, 4, 1, 0, uiAlignFill, 0, uiAlignFill);

    pbar = uiNewProgressBar();
    uiGridAppend(grid, uiControl(pbar), 0, 4, 8, 1, 0, uiAlignFill, 4, uiAlignFill);

    vbox = uiNewVerticalBox();
    uiGridAppend(grid, uiControl(vbox), 0, 5, 8, 1, 0, uiAlignFill, 0, uiAlignFill);

    status = uiNewLabel("");
    uiBoxAppend(vbox, uiControl(status), 0);

    uiControlShow(uiControl(mainwin));
    uiMain();
    if(thrd) { pthread_cancel(thrd); thrd = 0; }
    pthread_attr_destroy(&tha);
    uiQuit();
    return 0;
}
