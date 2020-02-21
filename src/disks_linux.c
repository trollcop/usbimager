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

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/mount.h>
#include <sys/stat.h>
#include "disks.h"

extern int fdatasync(int);

int disks_targets[DISKS_MAX];

/* helper to read a string from a file */
void filegetcontent(char *fn, char *buf, int maxlen)
{
    FILE *f;
    char *s, *o, tmp[1024];

    memset(buf, 0, maxlen);
    f = fopen(fn, "r");
    if(f) {
        memset(tmp, 0, sizeof(tmp));
        fread(tmp, maxlen - 1, 1, f);
        fclose(f);
        for(s = tmp; *s && *s <= ' '; s++);
        for(o = buf; *s && *s != '\n';) *o++ = *s++;
        while(o > buf && *(o-1) <= ' ') o--;
        *o = 0;
        if(verbose > 1) {
            printf(" %s: ", fn);
            for(s = tmp; s < tmp + maxlen && *s; s++)
                printf("%02x ", *s);
            printf("trim() = '%s'\n", buf);
        }
    }
}

/**
 * Refresh target device list in the combobox
 */
void disks_refreshlist()
{
    DIR *dir;
    struct dirent *de;
    char str[1024], vendorName[128], productName[128], path[512];
    uint64_t size;
    int i = 0, sizeInGbTimes10;

    memset(disks_targets, 0xff, sizeof(disks_targets));
#if DISKS_TEST
    disks_targets[i++] = 'T';
    main_addToCombobox("sdT ./test.bin");
#endif
    dir = opendir("/sys/block");
    if(dir) {
        while((de = readdir(dir))) {
            if(de->d_name[0] != 's' || de->d_name[1] != 'd') continue;
            if(verbose > 1) printf("\n");
            sprintf(path, "/sys/block/%s/removable", de->d_name);
            filegetcontent(path, vendorName, 2);
            if(vendorName[0] != '1') continue;
            sprintf(path, "/sys/block/%s/ro", de->d_name);
            filegetcontent(path, vendorName, 2);
            if(vendorName[0] != '0') continue;
            size = 0;
            sprintf(path, "/sys/block/%s/size", de->d_name);
            filegetcontent(path, vendorName, sizeof(vendorName));
            size = (uint64_t)atoll(vendorName) * 512UL;
            memset(vendorName, 0, sizeof(vendorName));
            sprintf(path, "/sys/block/%s/device/vendor", de->d_name);
            filegetcontent(path, vendorName, sizeof(vendorName));
            sprintf(path, "/sys/block/%s/device/model", de->d_name);
            filegetcontent(path, productName, sizeof(productName));
            str[0] = 0;
            if(size) {
                sizeInGbTimes10 = (int)((uint64_t)(10 * (size + 1024L*1024L*1024L-1L)) >> 30L);
                snprintf(str, sizeof(str)-1, "%s [%d.%d GiB] %s %s", de->d_name,
                    sizeInGbTimes10 / 10, sizeInGbTimes10 % 10, vendorName, productName);
            } else
                snprintf(str, sizeof(str)-1, "%s %s %s", de->d_name, vendorName, productName);
            str[128] = 0;
            disks_targets[i++] = de->d_name[2];
            main_addToCombobox(str);
            if(i >= DISKS_MAX) break;
        }
        closedir(dir);
    }
}

/**
 * Return mount points and bookmarks file
 */
char *disks_volumes(int *num, char ***mounts)
{
    FILE *m;
    int k;
    char buf[1024], *c, *path, *recent = NULL;
    char *env = getenv("HOME"), fn[1024];
    struct stat st;

    *mounts = (char**)realloc(*mounts, ((*num) + 3) * sizeof(char*));
    if(!*mounts) return NULL;
    
    if(env) {
        snprintf(fn, sizeof(fn)-1, "%s/.local/share/recently-used.xbel", env);
        if(!stat(fn, &st)) {
            (*mounts)[*num] = recent = (char*)malloc(strlen(fn)+1);
            if((*mounts)[*num]) { strcpy((*mounts)[*num], fn); (*num)++; }
        }
        (*mounts)[*num] = (char*)malloc(strlen(env)+1);
        if((*mounts)[*num]) { strcpy((*mounts)[*num], env); (*num)++; }
    } else {
        env = getenv("LOGNAME");
        if(env) {
            snprintf(fn, sizeof(fn)-1, "/home/%s", env);
            if(!stat(fn, &st)) {
                (*mounts)[*num] = (char*)malloc(strlen(fn)+1);
                if((*mounts)[*num]) { strcpy((*mounts)[*num], fn); (*num)++; }
            }
        }
    }

    (*mounts)[*num] = (char*)malloc(2);
    if((*mounts)[*num]) { strcpy((*mounts)[*num], "/"); (*num)++; }

    m = fopen("/proc/self/mountinfo", "r");
    if(m) {
        while(!feof(m)) {
            if(fgets(buf, sizeof(buf), m)) {
                for(k = 0, c = buf, path = NULL; k < 11 && *c && *c != '\n';) {
                    if(k == 5) *c++ = 0;
                    while(*c == ' ' || *c == '\t') c++;
                    if(k == 4) path = c;
                    k++;
                    while(*c && *c != ' ' && *c != '\t' && *c != '\n') c++;
                }
                if(!path[0] || !memcmp(path, "/dev", 4) || !memcmp(path, "/sys", 4) ||
                    !memcmp(path, "/run", 4) || !memcmp(path, "/proc", 5) || !strcmp(path, "/")) continue;
                *mounts = (char**)realloc(*mounts, ((*num) + 1) * sizeof(char*));
                if(*mounts) {
                    (*mounts)[*num] = (char*)malloc(strlen(path)+1);
                    if((*mounts)[*num]) { strcpy((*mounts)[*num], path); (*num)++; }
                }
            }
        }
        fclose(m);
    }
    return recent;
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
        errno = 0;
        ret = open(deviceName, O_RDWR | O_EXCL | O_CREAT, 0644);
        if(verbose)
            printf("disks_open(%s)\r\n  fd=%d errno=%d err=%s\r\n",
                deviceName, ret, errno, strerror(errno));
        if(ret < 0 || errno) {
            main_getErrorMessage();
            return NULL;
        }
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
                    if(!strcmp(path, "/") || !strcmp(path, "/boot")) { fclose(m); return (void*)-2; }
                    if(verbose) printf("  umount(%s)\r\n", path);
                    if(umount2(path, MNT_FORCE)) {
                        if(verbose) printf("  errno=%d err=%s\r\n", errno, strerror(errno));
                        main_getErrorMessage();
                        return (void*)-2;
                    }
                }
            }
        }
        fclose(m);
    }

    errno = 0;
    ret = open(deviceName, O_RDWR | O_SYNC | O_EXCL);
    if(verbose) printf("  fd=%d errno=%d err=%s\r\n", ret, errno, strerror(errno));
    if(ret < 0 || errno) {
        main_getErrorMessage();
        return NULL;
    }
    return (void*)((long int)ret);
}

/**
 * Close the target disk
 */
void disks_close(void *data)
{
    int fd = (int)((long int)data);
    fdatasync(fd);
    close(fd);
    if(verbose) printf("disks_close(%d)\r\n", fd);
}
