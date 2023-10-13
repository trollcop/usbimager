/*
 * usbimager/main.h
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
 * @brief Main interface functions
 *
 */

#define USBIMAGER_VERSION "1.0.10"
#define USBIMAGER_BUILD "40"

/* filters */
enum {
    TYPE_PLAIN = 0,
    TYPE_DEFLATE,
    TYPE_BZIP2,
    TYPE_XZ,
    TYPE_ZSTD
};

extern int verbose;
extern int buffer_size;
extern int baud;
extern int force;

/**
 * Add an option to the combobox
 */
void main_addToCombobox(char *option);

/**
 * Get the last error message
 */
void main_getErrorMessage(void);

/**
 * Update the progress and status bars
 */
void main_onProgress(void *data);
