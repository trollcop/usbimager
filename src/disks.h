/*
 * usbimager/disks.h
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
 * @brief Disk iteration and umount routines
 *
 */

#include <stdio.h>
#include <stdint.h>
#include <limits.h>

#define DISKS_MAX 128

extern int disks_all, disks_serial, disks_targets[DISKS_MAX];
extern uint64_t disks_capacity[DISKS_MAX];

/* some defines if not defined in limit.h */
#ifndef PATH_MAX
# ifdef MAXPATHLEN
#  define PATH_MAX MAXPATHLEN
# else
#  define PATH_MAX 65536
# endif
#endif
#ifndef FILENAME_MAX
# define FILENAME_MAX 256
#endif

/**
 * Get UI language
 */
char *disks_getlang();

/**
 * Refresh target device list in the combobox
 * Should set disks_targets[] and call main_addToCombobox()
 */
void disks_refreshlist();

/**
 * Return mount points and bookmarks file
 */
char *disks_volumes(int *num, char ***mounts);

/**
 * Lock, umount and open the target disk for writing
 * this returns FD on unices, and HANDLE on Windows
 */
void *disks_open(int targetId, uint64_t size);

/**
 * Close the target disk
 * Receives FD or HANDLE
 */
void disks_close(void *ctx);
