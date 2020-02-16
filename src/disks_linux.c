/*
 * usbimager/disks_linux.c
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
 * @brief Disk iteration and umount for Linux
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <dirent.h>
#include <sys/mount.h>
#include <fcntl.h>
#include "libui/ui.h"
#include "disks.h"

extern int verbose;

int disks_targets[DISKS_MAX];

/**
 * Refresh target device list in the combobox
 */
void disks_refreshlist(void *data)
{
    uiCombobox *target = (uiCombobox *)data;
    FILE *f;
    DIR *dir;
    struct dirent *de;
    char str[1024], vendorName[128], productName[128], path[512];
    uint64_t size, sizeInGbTimes10;
    size_t s;
    int i = 0;

    memset(disks_targets, 0xff, sizeof(disks_targets));
#if DISKS_TEST
    disks_targets[i++] = 'T';
    uiComboboxAppend(target, "sdT Testfile ./test.bin");
#endif
    dir = opendir("/sys/block");
    if(dir) {
        while((de = readdir(dir))) {
            if(de->d_name[0] != 's' || de->d_name[1] != 'd') continue;
            sprintf(path, "/sys/block/%s/removable", de->d_name);
            f = fopen(path, "r");
            if(f) {
                fread(path, 1, 1, f);
                fclose(f);
            }
            if(path[0] != '1') continue;
            size = 0;
            sprintf(path, "/sys/block/%s/size", de->d_name);
            f = fopen(path, "r");
            if(f) {
                memset(path, 0, 32);
                fread(path, 31, 1, f);
                size = (uint64_t)atol(path);
                fclose(f);
            }
            memset(vendorName, 0, sizeof(vendorName));
            sprintf(path, "/sys/block/%s/device/vendor", de->d_name);
            f = fopen(path, "r");
            if(f) {
                fread(vendorName, sizeof(vendorName) - 1, 1, f);
                for(s = 0; s < sizeof(vendorName) && vendorName[s]; s++)
                    if(vendorName[s] == '\n') { vendorName[s] = 0; break; }
                fclose(f);
            }
            memset(productName, 0, sizeof(productName));
            sprintf(path, "/sys/block/%s/device/model", de->d_name);
            f = fopen(path, "r");
            if(f) {
                fread(productName, sizeof(productName) - 1, 1, f);
                for(s = 0; s < sizeof(productName) && productName[s]; s++)
                    if(productName[s] == '\n') { productName[s] = 0; break; }
                fclose(f);
            }
            if(size) {
                sizeInGbTimes10 = (uint64_t)(10 * (size + 1024L*1024L-1L) / 1024L / 1024L);
                snprintf(str, sizeof(str)-1, "%s [%ld.%ld GiB] %s %s", de->d_name,
                    sizeInGbTimes10 / 10, sizeInGbTimes10 % 10, vendorName, productName);
            } else
                snprintf(str, sizeof(str)-1, "%s %s %s", de->d_name, vendorName, productName);
            str[128] = 0;
            disks_targets[i++] = de->d_name[2];
            uiComboboxAppend(target, str);
            if(i >= DISKS_MAX) break;
        }
        closedir(dir);
    }
}

/**
 * Lock, umount and open the target disk for writing
 */
void *disks_open(int targetId)
{
    int ret = 0, l, k;
    char deviceName[16], *c, buf[1024], *path, *device;
    FILE *m;

    if(targetId < 0 || targetId >= DISKS_MAX || disks_targets[targetId] == -1) return (void*)-1;

#if DISKS_TEST
    if((char)disks_targets[targetId] == 'T') {
        sprintf(deviceName, "./test.bin");
        unlink(deviceName);
        ret = open(deviceName, O_RDWR | O_EXCL | O_CREAT, 0644);
        if(verbose) printf("disks_open(%s)\r\n  fd=%d\r\n", deviceName, ret);
        if(ret < 0) return NULL;
        return (void*)((long int)ret);
    }
#endif
    sprintf(deviceName, "/dev/sd%c", (char)disks_targets[targetId]);
    if(verbose) printf("disks_open(%s)\r\n", deviceName);
    m = fopen("/proc/self/mountinfo", "r");
    if(m) {
        l = strlen(deviceName);
        while(!feof(m)) {
            if(fgets(buf, sizeof(buf), m)) {
                for(k = 0, c = buf, path = device = NULL; k < 11 && *c && *c != '\n';) {
                    if(k == 5 || k == 10) *c++ = 0;
                    while(*c == ' ' || *c == '\t') c++;
                    if(k == 4) path = c;
                    if(k == 9) device = c;
                    k++;
                    while(*c && *c != ' ' && *c != '\t' && *c != '\n') c++;
                }
                if(device && !strncmp(device, deviceName, l)) {
                    if(!strcmp(path, "/") || !strcmp(path, "/boot")) { fclose(m); return (void*)-1; }
                    if(verbose) printf("  umount(%s)\r\n", path);
                    umount2(path, MNT_FORCE);
                }
            }
        }
        fclose(m);
    }

    ret = open(deviceName, O_RDWR | O_SYNC | O_EXCL);
    if(verbose) printf("  fd=%d\r\n", ret);
    if(ret < 0) return NULL;
    return (void*)((long int)ret);
}

/**
 * Close the target disk
 */
void disks_close(void *data)
{
    int fd = (int)((long int)data);
    close(fd);
    if(verbose) printf("disks_close(%d)\r\n", fd);
}
