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
#include "lang.h"
#include "input.h"
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
static uiButton *button;
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
    uiControlEnable(uiControl(verify));
    uiControlEnable(uiControl(button));
    uiProgressBarSetValue(pbar, 0);
    uiLabelSetText(status, (char*)data);
    main_errorMessage = NULL;
}

static void onProgress(void *data)
{
    char textstat[128];
    int pos;

    pos = input_status((input_t*)data, textstat);
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

static void *writerRoutine(void *data)
{
    int dst, needVerify = uiCheckboxChecked(verify), numberOfBytesRead;
    int numberOfBytesWritten, numberOfBytesVerify;
    char buffer[BUFFER_SIZE], verifyBuf[BUFFER_SIZE];
    input_t ctx;
    (void)data;

    dst = input_open(&ctx, uiEntryText(source));
    if(!dst) {
        dst = (int)((long int)disks_open(uiComboboxSelected(target)));
        if(dst > 0) {
            while(1) {
                if((numberOfBytesRead = input_read(&ctx, buffer)) >= 0) {
                    if(numberOfBytesRead == 0) {
                        if(!ctx.fileSize) ctx.fileSize = ctx.readSize;
                        break;
                    } else {
                        errno = 0;
                        numberOfBytesWritten = (int)write(dst, buffer, numberOfBytesRead);
                        if(verbose) printf("writerRoutine() numberOfBytesRead %d numberOfBytesWritten %d errno=%d\n",
                            numberOfBytesRead, numberOfBytesWritten, errno);
                        if(numberOfBytesWritten == numberOfBytesRead) {
                            if(needVerify) {
                                lseek(dst, -((off_t)numberOfBytesWritten), SEEK_CUR);
                                numberOfBytesVerify = read(dst, verifyBuf, numberOfBytesWritten);
                                if(verbose) printf("  numberOfBytesVerify %d\n", numberOfBytesVerify);
                                if(numberOfBytesVerify != numberOfBytesWritten ||
                                    memcmp(buffer, verifyBuf, numberOfBytesWritten)) {
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
        input_close(&ctx);
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
    uiControlDisable(uiControl(verify));
    uiControlDisable(uiControl(button));
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
    int i;
    char *lc = getenv("LANG");

    if(argc > 1 && argv[1] && argv[1][0] == '-')
        for(i = 1; argv[1][i]; i++)
            if(argv[1][1] == 'v') verbose++;

    if(!lc) lc = "en";
    for(i = 0; i < NUMLANGS; i++) {
        if(!strcmp(lc, dict[i][0])) {
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

    targetCont = uiNewHorizontalBox();
    uiGridAppend(grid, uiControl(targetCont), 0, 2, 8, 1, 0, uiAlignFill, 0, uiAlignFill);
    target = uiNewCombobox();
    uiBoxAppend(targetCont, uiControl(target), 1);
    uiComboboxOnSelected(target, refreshTarget, NULL);
    refreshTarget(target, NULL);

    verify = uiNewCheckbox(lang[L_VERIFY]);
    uiGridAppend(grid, uiControl(verify), 0, 3, 2, 1, 0, uiAlignStart, 0, uiAlignFill);

    button = uiNewButton(lang[L_WRITE]);
    uiButtonOnClicked(button, onWriteButtonClicked, NULL);
    uiGridAppend(grid, uiControl(button), 2, 3, 6, 1, 0, uiAlignFill, 0, uiAlignFill);

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
