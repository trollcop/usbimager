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
#import <fcntl.h>
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

#import "libui/ui.h"
#import "disks.h"

extern int verbose;

int disks_targets[DISKS_MAX], currTarget = 0;

/**
 * Refresh target device list in the combobox
 */
void disks_refreshlist(void *data)
{
    NSAutoreleasePool * pool = [[NSAutoreleasePool alloc] init];

    uiCombobox *target = (uiCombobox *)data;
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
    uiComboboxAppend(target, "disk999 Testfile ./test.bin");
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
        uiComboboxAppend(target, str);

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
    struct statfs buf[256];

    if(targetId < 0 || targetId >= DISKS_MAX || disks_targets[targetId] == -1) return (void*)-1;
    currTarget = disks_targets[targetId];

#if DISKS_TEST
    if(currTarget == 999) {
        sprintf(deviceName, "./test.bin");
        unlink(deviceName);
        ret = open(deviceName, O_RDWR | O_EXCL | O_CREAT, 0644);
        if(verbose) printf("disks_open(%s)\r\n  fd=%d\r\n", deviceName, ret);
        if(ret < 0) return NULL;
        return (void*)((long int)ret);
    }
#endif

    NSAutoreleasePool * pool = [[NSAutoreleasePool alloc] init];
    CFURLRef path;
    DADiskRef disk;
    DASessionRef session = DASessionCreate(kCFAllocatorDefault);

    n = getfsstat(buf, 256, MNT_NOWAIT);
    if(n > 0) {
        sprintf(deviceName, "/dev/disk%d", disks_targets[targetId]);
        l = strlen(deviceName);
        for (i = 0; i < n; i++)
            if(!strncmp(buf[i].f_mntfromname, deviceName, l)) {
                if(!strcmp(buf[i].f_mntonname, "/")) { CFRelease(session); return (void*)-1; }
                path = CFURLCreateWithBytes(kCFAllocatorDefault, (UInt8*)&buf[i].f_mntonname, strlen(buf[i].f_mntonname), kCFStringEncodingUTF8, NULL);
                if(path) {
                    disk = DADiskCreateFromVolumePath(kCFAllocatorDefault, session, path);
                    if(disk) {
                        DADiskUnmount(disk, kDADiskUnmountOptionForce, NULL , NULL);
                        CFRelease(disk);
                        if(verbose) printf("  umount(%s)\n", buf[i].f_mntonname);
                    }
                    CFRelease(path);
                }
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
    char deviceName[16];

    close(fd);
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
