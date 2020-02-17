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
#import <sys/param.h>
#import <sys/ucred.h>
#import <sys/mount.h>
#import <sys/stat.h>
#import <Foundation/Foundation.h>
#import <CoreFoundation/CoreFoundation.h>
#import <DiskArbitration/DiskArbitration.h>
#import <IOKit/IOKitLib.h>
#import <IOKit/IOMessage.h>
#import <IOKit/IOCFPlugIn.h>
#import <IOKit/usb/IOUSBLib.h>
#import <IOKit/IOBSD.h>

#import "disks.h"

int disks_targets[DISKS_MAX], currTarget = 0;

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
    int i = 0, writ = 0;
    const char *deviceName = 0, *vendorName = NULL, *productName = NULL;
    CFTypeRef writable = NULL, bsdName = NULL, vendor = NULL, product = NULL, disksize = NULL;
    struct stat st;

    memset(disks_targets, 0xff, sizeof(disks_targets));
#if DISKS_TEST
    disks_targets[i++] = 999;
    main_addToCombobox("disk999 Testfile ./test.bin");
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
            long int sizeInGbTimes10 = (long int)(10 * (size + 1024L*1024L*1024L-1L) / 1024L / 1024L / 1024L);
            snprintf(str, sizeof(str)-1, "%s [%ld.%ld GiB] %s %s", deviceName,
                sizeInGbTimes10 / 10, sizeInGbTimes10 % 10, vendorName, productName);
        } else
            snprintf(str, sizeof(str)-1, "%s %s %s", deviceName, vendorName, productName);
        str[128] = 0;
        disks_targets[i++] = atoi(deviceName + (deviceName[0] == 'r' ? 5 : 4));
        main_addToCombobox(str);

        CFRelease(bsdName); bsdName = NULL;
        if(vendor) CFRelease(vendor); vendor = NULL;
        if(product) CFRelease(product); product = NULL;
        IOObjectRelease(usb_device_ref);
        if(i >= DISKS_MAX) break;
    }

    [pool release];
}

/**
 * Lock, umount and open the target disk for writing
 */
void *disks_open(int targetId)
{
    int ret = 0, i, l, n;
    char deviceName[16];
    struct statfs *buf;

    if(targetId < 0 || targetId >= DISKS_MAX || disks_targets[targetId] == -1) return (void*)-1;
    currTarget = disks_targets[targetId];

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
