/*
 * usbimager/disks_darwin.c
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
 * @brief Disk iteration and umount for MacOSX. NOTE: this is Obj-C
 *
 */

#import <stdio.h>
#import <stdlib.h>
#import <string.h>
#import <unistd.h>
#import <fcntl.h>
#import <errno.h>
#import <termios.h>
#import <sys/param.h>
#import <sys/ucred.h>
#import <sys/mount.h>
#import <sys/stat.h>
#import <sys/ioctl.h>
#import <sys/ttycom.h>
#import <Foundation/Foundation.h>
#import <CoreFoundation/CoreFoundation.h>
#import <DiskArbitration/DiskArbitration.h>
#import <IOKit/IOKitLib.h>
#import <IOKit/IOMessage.h>
#import <IOKit/IOCFPlugIn.h>
#import <IOKit/usb/IOUSBLib.h>
#import <IOKit/serial/IOSerialKeys.h>
#import <IOKit/serial/ioss.h>
#import <IOKit/IOBSD.h>

#import "lang.h"
#import "main.h"
#import "disks.h"

int disks_serial = 0, disks_targets[DISKS_MAX], currTarget = 0;
uint64_t disks_capacity[DISKS_MAX];
char disks_serials[DISKS_MAX][64];

static int numUmount = 0;
void disks_umountDone(DADiskRef disk, DADissenterRef dis, void *context)
{
    (void)disk;
    (void)dis;
    (void)context;
    numUmount--;
    if(verbose) printf("\r\n  umountDone num=%d\r\n", numUmount);
}

/**
 * Refresh target device list in the combobox
 */
void disks_refreshlist()
{
    NSAutoreleasePool * pool = [[NSAutoreleasePool alloc] init];

    char str[1024];
    mach_port_t             master_port;
    kern_return_t           k_result = KERN_FAILURE;
    io_iterator_t           iterator = 0;
    io_service_t            usb_device_ref;
    CFMutableDictionaryRef  matching_dictionary = NULL;
    long int size = 0;
    int i = 0, j = 1024, writ = 0;
    const char *deviceName = 0, *vendorName = NULL, *productName = NULL;
    CFTypeRef writable = NULL, bsdName = NULL, vendor = NULL, product = NULL, disksize = NULL;
    struct stat st;

    memset(disks_targets, 0xff, sizeof(disks_targets));
    memset(disks_capacity, 0, sizeof(disks_capacity));
#if DISKS_TEST
    disks_targets[i++] = 999;
    main_addToCombobox("disk999 ./test.bin");
#endif
    k_result = IOMasterPort(MACH_PORT_NULL, &master_port);
    if (KERN_SUCCESS != k_result) return;
    if ((matching_dictionary = IOServiceMatching(kIOUSBDeviceClassName)) == NULL) return;
    k_result = IOServiceGetMatchingServices(master_port, matching_dictionary, &iterator);
    if (KERN_SUCCESS != k_result) return;

    while ((usb_device_ref = IOIteratorNext(iterator))) {
        writable = (CFTypeRef) IORegistryEntrySearchCFProperty (usb_device_ref,
                                                               kIOServicePlane,
                                                               CFSTR( "Writable" ),
                                                               kCFAllocatorDefault,
                                                               kIORegistryIterateRecursively  | kIORegistryIterateParents);
        if (writable) {
            CFNumberGetValue((CFNumberRef)writable, kCFNumberSInt32Type, &writ);
            CFRelease(writable);
            if(!writ) continue;
        }

        bsdName = (CFTypeRef) IORegistryEntrySearchCFProperty (usb_device_ref,
                                                               kIOServicePlane,
                                                               CFSTR ( "BSD Name" ),
                                                               kCFAllocatorDefault,
                                                               kIORegistryIterateRecursively );
        if (!bsdName) continue;
        deviceName = [[NSString stringWithFormat: @"%@", bsdName] UTF8String];
        /* kIOUSBDeviceClassName lists some non-disks as writable disks (like USB-dongles with device driver storages) */
        if (deviceName[0] == 'e' && deviceName[1] == 'n' && deviceName[2] >= '0' && deviceName[2] <= '9') {
            CFRelease(bsdName); bsdName = NULL;
            continue;
        }

        vendor = (CFTypeRef) IORegistryEntrySearchCFProperty (usb_device_ref,
                                                               kIOServicePlane,
                                                               CFSTR("USB Vendor Name"),
                                                               kCFAllocatorDefault,
                                                               kIORegistryIterateRecursively  | kIORegistryIterateParents);
        if (vendor)
            vendorName = [[NSString stringWithFormat: @"%@", vendor] UTF8String];
        else
            vendorName = "";

        product = (CFTypeRef) IORegistryEntrySearchCFProperty (usb_device_ref,
                                                               kIOServicePlane,
                                                               CFSTR("USB Product Name"),
                                                               kCFAllocatorDefault,
                                                               kIORegistryIterateRecursively  | kIORegistryIterateParents);
        if(!product)
            product = (CFTypeRef) IORegistryEntrySearchCFProperty (usb_device_ref,
                                                               kIOServicePlane,
                                                               CFSTR("Product Name"),
                                                               kCFAllocatorDefault,
                                                               kIORegistryIterateRecursively  | kIORegistryIterateParents);
        if (product)
            productName = [[NSString stringWithFormat: @"%@", product] UTF8String];
        else
            productName = "";

        size = 0;
        disksize = (CFTypeRef) IORegistryEntrySearchCFProperty (usb_device_ref,
                                                               kIOServicePlane,
                                                               CFSTR("Size"),
                                                               kCFAllocatorDefault,
                                                               kIORegistryIterateRecursively  | kIORegistryIterateParents);
        if (disksize) {
            CFNumberGetValue((CFNumberRef)disksize, kCFNumberSInt64Type, &size);
            CFRelease(disksize);
        }
        if (!size) {
            sprintf(str, "/dev/r%s", deviceName);
            if(!stat(str, &st))
                size = st.st_blocks ? st.st_blocks * 512 : st.st_size;
        }
        if(size) {
            int sizeInGbTimes10 = (int)((long int)(10 * (size + 1024L*1024L*1024L-1L)) >> 30L);
            char *unit = lang[L_GIB];
            if(!sizeInGbTimes10) { unit = lang[L_MIB]; sizeInGbTimes10 = (int)((uint64_t)(10 * (size + 1024L*1024L-1L)) >> 20L); }
            snprintf(str, sizeof(str)-1, "%s [%d.%d %s] %s %s", deviceName,
                sizeInGbTimes10 / 10, sizeInGbTimes10 % 10, unit, vendorName, productName);
        } else
            snprintf(str, sizeof(str)-1, "%s %s %s", deviceName, vendorName, productName);
        str[128] = 0;
        disks_capacity[i] = size;
        disks_targets[i++] = atoi(deviceName + (deviceName[0] == 'r' ? 5 : 4));
        main_addToCombobox(str);

        CFRelease(bsdName); bsdName = NULL;
        if(vendor) CFRelease(vendor); vendor = NULL;
        if(product) CFRelease(product); product = NULL;
        IOObjectRelease(usb_device_ref);
        if(i >= DISKS_MAX) break;
    }
    if(disks_serial) {
        matching_dictionary = NULL;
        if ((matching_dictionary = IOServiceMatching(kIOSerialBSDServiceValue)) == NULL)
            CFDictionarySetValue(matching_dictionary, CFSTR(kIOSerialBSDTypeKey), CFSTR(kIOSerialBSDModemType));
        if (!matching_dictionary) return;
        k_result = IOServiceGetMatchingServices(master_port, matching_dictionary, &iterator);
        if (KERN_SUCCESS != k_result) return;

        while ((usb_device_ref = IOIteratorNext(iterator))) {
            bsdName = (CFTypeRef) IORegistryEntrySearchCFProperty (usb_device_ref,
                                                                   kIOServicePlane,
                                                                   CFSTR ( "BSD Name" ),
                                                                   kCFAllocatorDefault,
                                                                   kIORegistryIterateRecursively );
            if (!bsdName) continue;
            deviceName = [[NSString stringWithFormat: @"%@", bsdName] UTF8String];

            strncpy(disks_serials[j], deviceName, 63);
            snprintf(str, sizeof(str)-1, "%s %s", deviceName, lang[L_SERIAL]);
            disks_targets[i++] = 1024 + j++;
            main_addToCombobox(str);

            CFRelease(bsdName); bsdName = NULL;
            IOObjectRelease(usb_device_ref);
            if(i >= DISKS_MAX) break;
        }
    }

    [pool release];
}

/**
 * Return mount points and bookmarks file
 */
char *disks_volumes(int *num, char ***mounts)
{
    struct statfs *buf;
    int i, n;
    char *env = getenv("HOME"), fn[1024];
    struct stat st;

    *mounts = (char**)realloc(*mounts, ((*num) + 2) * sizeof(char*));
    if(!*mounts) return NULL;

    if(env) {
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

    n = getfsstat(NULL, 0, MNT_NOWAIT);
    if(n > 0) {
        buf = (struct statfs *)malloc(n * sizeof(struct statfs));
        *mounts = (char**)realloc(*mounts, ((*num) + n) * sizeof(char*));
        if(buf && *mounts) {
            n = getfsstat(buf, n * sizeof(struct statfs), MNT_NOWAIT);
            for (i = 0; i < n; i++) {
                if(!buf[i].f_mntfromname[0] || !memcmp(buf[i].f_mntfromname, "/dev", 4) ||
                    !strcmp(buf[i].f_mntonname, "/")) continue;
                (*mounts)[*num] = (char*)malloc(strlen(buf[i].f_mntonname)+1);
                if((*mounts)[*num]) { strcpy((*mounts)[*num], buf[i].f_mntonname); (*num)++; }
            }
            free(buf);
        }
    }
    return NULL;
}

/**
 * Lock, umount and open the target disk for writing
 */
void *disks_open(int targetId, uint64_t size)
{
    int ret = 0, i, l, n;
    char deviceName[16], tmp[8];
    struct termios termios;
    struct statfs *buf;

    if(targetId < 0 || targetId >= DISKS_MAX || disks_targets[targetId] == -1) return (void*)-1;
    if(size && disks_capacity[targetId] && size > disks_capacity[targetId]) return (void*)-1;
    currTarget = disks_targets[targetId];

    if(currTarget >= 1024) {
        sprintf(deviceName, "/dev/%s", disks_serials[currTarget-1024]);
        errno = 0;
        ret = open(deviceName, O_RDWR | O_NOCTTY | O_NONBLOCK);
        if(verbose)
            printf("disks_open(%s) serial\r\n  fd=%d errno=%d err=%s\r\n",
                deviceName, ret, errno, strerror(errno));
        if(ret < 0 || errno || ioctl(ret, TIOCEXCL) == -1 || fcntl(ret, F_SETFL, 0) == -1) {
            main_getErrorMessage();
            return NULL;
        }
        if(isatty(ret)) {
            if(tcgetattr(ret, &termios) != -1) {
                termios.c_cc[VTIME] = 0;
                termios.c_cc[VMIN] = 0;
                termios.c_iflag = 0;
                termios.c_oflag = 0;
                termios.c_cflag = CS8 | CREAD | CLOCAL;
                termios.c_lflag = 0;
                if((cfsetispeed(&termios, B115200) < 0) || (cfsetospeed(&termios, B115200) < 0))
                {
                    if(verbose)
                        printf(" failed to set baud errno=%d err=%s\r\n", errno, strerror(errno));
                    goto sererr;
                }
                if(tcsetattr(ret, TCSAFLUSH, &termios) == -1) {
                    if(verbose)
                        printf(" failed to set attr errno=%d err=%s\r\n", errno, strerror(errno));
                    goto sererr;
                }
            } else {
                if(verbose)
                    printf(" tcgetattr error errno=%d err=%s\r\n", errno, strerror(errno));
sererr:         main_getErrorMessage();
                close(ret);
                return (void*)-1;
            }
            if(disks_serial == 2) {
                /* Raspbootin Serial Protocol:
                 *   \003\003\003 - client to server
                 *   4 bytes little-endian, size of image - server to client
                 *   'OK' or 'SE' (size error) - client to server
                 *   image content - server to client
                 */
                if(verbose) printf("  awaiting client\r\n");
                for(l = 0; l < 3;) {
                    if(read(ret, &tmp, 1)) {
                        if(tmp[0] == 3) l++;
                        else {
                            l = 0;
                            if(verbose) printf("%c", tmp[0]);
                        }
                    }
                    usleep(1000);
                }
                if(verbose) printf("\r\n  client connected\r\n");
                if(write(ret, &size, 4) != 4) {
                    if(verbose)
                        printf(" unable to send size errno=%d err=%s\r\n", errno, strerror(errno));
                    goto sererr;
                }
                if(read(ret, &tmp, 2) != 2 || tmp[0] != 'O' || tmp[1] != 'K') {
                    if(verbose)
                        printf(" didn't received ACK from client, got '%c%c' errno=%d err=%s\r\n",
                            tmp[0], tmp[1], errno, strerror(errno));
                    goto sererr;
                }
            }
        }
        return (void*)((long int)ret);
    }

#if DISKS_TEST
    if(currTarget == 999) {
        sprintf(deviceName, "./test.bin");
        unlink(deviceName);
        errno = 0;
        ret = open(deviceName, O_RDWR | O_EXCL | O_CREAT, 0644);
        if(verbose)
            printf("disks_open(%s)\r\n  fd=%d errno=%d err=%s\r\n",
                deviceName, ret, errno, strerror(errno));
        if(ret < 0) {
            main_getErrorMessage();
            return NULL;
        }
        return (void*)((long int)ret);
    }
#endif

    /* Fix issue #16: check if we have write permission before we try to umount */
    sprintf(deviceName, "/dev/rdisk%d", disks_targets[targetId]);
    errno = 0;
    ret = open(deviceName, O_RDWR);
    if(errno == EPERM || errno == EACCES) {
        main_getErrorMessage();
        return NULL;
    }
    if(ret > 0) close(ret);

    NSAutoreleasePool * pool = [[NSAutoreleasePool alloc] init];
    CFURLRef path;
    DADiskRef disk;
    DASessionRef session = DASessionCreate(kCFAllocatorDefault);

    n = getfsstat(NULL, 0, MNT_NOWAIT);
    if(verbose) printf("  getfsstat = %d\r\n", n);
    if(n > 0) {
        buf = (struct statfs *)malloc(n * sizeof(struct statfs));
        if(buf) {
            n = getfsstat(buf, n * sizeof(struct statfs), MNT_NOWAIT);
            sprintf(deviceName, "/dev/disk%d", disks_targets[targetId]);
            l = strlen(deviceName);
            DASessionScheduleWithRunLoop(session, CFRunLoopGetCurrent(), kCFRunLoopDefaultMode);
            if(verbose) printf(" checking mounted fs list, n=%d\r\n", n);
            for (i = 0; i < n; i++) {
                if(!strncmp(buf[i].f_mntfromname, deviceName, l) && (!buf[i].f_mntfromname[l] || buf[i].f_mntfromname[l] == 's')) {
                    if(!strcmp(buf[i].f_mntonname, "/")) {
                        DASessionUnscheduleFromRunLoop(session, CFRunLoopGetCurrent(), kCFRunLoopDefaultMode);
                        CFRelease(session);
                        return (void*)-2;
                    }
                    if(verbose) printf("  fsstat %s %s\r\n", buf[i].f_mntfromname, buf[i].f_mntonname);
                    path = CFURLCreateWithBytes(kCFAllocatorDefault, (UInt8*)&buf[i].f_mntonname, strlen(buf[i].f_mntonname), kCFStringEncodingUTF8, NULL);
                    if(path) {
                        disk = DADiskCreateFromVolumePath(kCFAllocatorDefault, session, path);
                        if(disk) {
                            numUmount++;
                            DADiskUnmount(disk, kDADiskUnmountOptionForce, disks_umountDone, NULL);
                            CFRelease(disk);
                            if(verbose) printf("  umount(%s)\n", buf[i].f_mntonname);
                        }
                        CFRelease(path);
                    }
                }
            }
            free(buf);
            for(i = 0; numUmount > 0 && i < 30000; i++) {
               CFRunLoopRun();
               if(verbose) { printf("  waiting for umount num=%d i=%d\r", numUmount, i); fflush(stdout); }
               if(numUmount) usleep(100);
            }
            if(verbose) printf("\n");
            DASessionUnscheduleFromRunLoop(session, CFRunLoopGetCurrent(), kCFRunLoopDefaultMode);
        }
    }

    sprintf(deviceName, "/dev/rdisk%d", disks_targets[targetId]);
    if(verbose) printf("disks_open(%s)\r\n", deviceName);
    path = CFURLCreateWithBytes(kCFAllocatorDefault, (UInt8*)&deviceName, strlen(deviceName), kCFStringEncodingUTF8, NULL);
    if(path) {
        disk = DADiskCreateFromVolumePath(kCFAllocatorDefault, session, path);
        if(disk) {
            DADiskClaim(disk, kDADiskClaimOptionDefault, NULL, NULL, NULL, NULL);
            CFRelease(disk);
            if(verbose) printf("  Claimed\r\n");
        }
        CFRelease(path);
    }
    CFRelease(session);

    [pool release];

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
    char deviceName[16];

    close(fd);
    sync();
    if(verbose) printf("disks_close(%d)\r\n", fd);
#if DISKS_TEST
    if(currTarget == 999) return;
#endif

    NSAutoreleasePool * pool = [[NSAutoreleasePool alloc] init];
    CFURLRef path;
    DADiskRef disk;
    DASessionRef session = DASessionCreate(kCFAllocatorDefault);

    sprintf(deviceName, "/dev/rdisk%d", currTarget);
    if(verbose) printf("  device %s\r\n", deviceName);
    path = CFURLCreateWithBytes(kCFAllocatorDefault, (UInt8*)&deviceName, strlen(deviceName), kCFStringEncodingUTF8, NULL);
    if(path) {
        disk = DADiskCreateFromVolumePath(kCFAllocatorDefault, session, path);
        if(disk) {
            DADiskUnclaim(disk);
            CFRelease(disk);
            if(verbose) printf("  unClaim\r\n");
        }
        CFRelease(path);
    }
    CFRelease(session);

    [pool release];
}
