/*
 * usbimager/main_unix.c
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
 * @brief User interface for Linux and MacOSX
 *
 */

#include <pthread.h>
#include "input.h"
#include "disks.h"
#include "libui/ui.h"

static uiWindow *mainwin;
static uiButton *sourceButton;
static uiEntry *source;
static uiBox *targetCont;
static uiCombobox *target;
static uiCheckbox *verify;
static uiButton *button;
static uiProgressBar *pbar;
static uiLabel *status;
static pthread_t thread;

void onDone(void *data)
{
    uiControlEnable(uiControl(source));
    uiControlEnable(uiControl(sourceButton));
    uiControlEnable(uiControl(target));
    uiControlEnable(uiControl(verify));
    uiControlEnable(uiControl(button));
    uiProgressBarSetValue(pbar, 0);
    uiLabelSetText(status, (char*)data);
}

void onProgress(void *data)
{
    char textstat[128];
    int pos;

    pos = input_status((input_t*)data, textstat);
    uiProgressBarSetValue(pbar, pos);
    uiLabelSetText(status, textstat);
}

void onThreadError(void *data)
{
    uiMsgBoxError(mainwin, "Error", (char*)data);
}

static void *writerRoutine(void *data)
{
    int dst, needVerify = uiCheckboxChecked(verify), numberOfBytesRead;
    size_t numberOfBytesWritten, numberOfBytesVerify;
    char buffer[BUFFER_SIZE], verifyBuf[BUFFER_SIZE];
    input_t ctx;
    (void)data;

    dst = input_open(&ctx, uiEntryText(source));
    if(!dst) {
        dst = uiComboboxSelected(target);
        if(dst < 0) {
            uiQueueMain(onThreadError, "Please select a target.");
        } else {
            dst = (int)((long int)disks_open(dst));
            if(dst > 0) {
                while(1) {
                    if((numberOfBytesRead = input_read(&ctx, buffer)) >= 0) {
                        if(numberOfBytesRead == 0) {
                            if(!ctx.fileSize) ctx.fileSize = ctx.readSize;
                            break;
                        } else {
                            numberOfBytesWritten = write(dst, buffer, numberOfBytesRead);
                            if(verbose) printf("writerRoutine() numberOfBytesRead %d numberOfBytesWritten %lu\n",
                                numberOfBytesRead, numberOfBytesWritten);
                            if((int)numberOfBytesWritten == numberOfBytesRead) {
                                if(needVerify) {
                                    lseek(dst, -((off_t)numberOfBytesWritten), SEEK_CUR);
                                    numberOfBytesVerify = read(dst, verifyBuf, numberOfBytesWritten);
                                    if(verbose) printf("  numberOfBytesVerify %lu\n", numberOfBytesVerify);
                                    if(numberOfBytesVerify != numberOfBytesWritten ||
                                        memcmp(buffer, verifyBuf, numberOfBytesWritten)) {
                                            uiQueueMain(onThreadError,
                                                "Write verification failed.");
                                        break;
                                    }
                                }
                                uiQueueMain(onProgress, &ctx);
                            } else {
                                uiQueueMain(onThreadError,
                                    "An error occurred while writing to the target device.");
                                break;
                            }
                        }
                    } else {
                        uiQueueMain(onThreadError,
                            "An error occurred while reading the source file.");
                        break;
                    }
                }
                disks_close((void*)((long int)dst));
            } else {
                uiQueueMain(onThreadError,
                    "An error occurred while opening the target device.");
            }
        }
        input_close(&ctx);
    } else {
        uiQueueMain(onThreadError,
            dst == 2 ? "Encrypted ZIP not supported" :
            (dst == 3 ? "Unsupported compression method in ZIP" :
            (dst == 4 ? "Decompressor error" :
            "Please select a readable source file.")));
    }
    uiQueueMain(onDone, ctx.fileSize && ctx.readSize == ctx.fileSize ?
        "Done. Image written successfully." : "");
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

    if(verbose) printf("Starting worker thread\r\n");
    pthread_create(&thread, NULL, writerRoutine, NULL);
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
    disks_refreshlist(target);
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
    if(thread) pthread_cancel(thread);
    uiQuit();
    return 1;
}

static int onShouldQuit(void *data)
{
    if(thread) pthread_cancel(thread);
    uiControlDestroy(uiControl(uiWindow(data)));
    return 1;
}

int main(int argc, char **argv)
{
    uiInitOptions options;
    const char *err;
    uiGrid *grid;
    uiBox *vbox;

    if(argc > 1 && argv[1] && argv[1][0] == '-' && argv[1][1] == 'v')
        verbose = 1;

    memset(&thread, 0, sizeof(pthread_t));
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

    verify = uiNewCheckbox("Verify");
    uiGridAppend(grid, uiControl(verify), 0, 3, 2, 1, 0, uiAlignStart, 0, uiAlignFill);

    button = uiNewButton("Write");
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
    if(thread) pthread_cancel(thread);
    uiQuit();
    return 0;
}
