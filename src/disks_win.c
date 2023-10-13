/*
 * usbimager/disks_win.c
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
 * @brief Disk iteration and umount for Windows
 *
 */

#define _CRT_SECURE_NO_WARNINGS
#include <windows.h>
#include <winioctl.h>
#include <commctrl.h>
#include <ntdddisk.h>
#include "lang.h"
#include "main.h"
#include "disks.h"

int disks_all = 0, disks_serial = 0, disks_maxsize = DISKS_MAXSIZE, disks_targets[DISKS_MAX], cdrive = 0, nLocks = 0;
uint64_t disks_capacity[DISKS_MAX];

HANDLE hLocks[32];

/**
 * Refresh target device list in the combobox
 */
void disks_refreshlist(void) {
    int i = 0, j, k;
    VOLUME_DISK_EXTENTS volumeDiskExtents;
    wchar_t szLbText[1024], volName[MAX_PATH+1], siz[64], *wc;
    HANDLE hTargetDevice;
    DISK_GEOMETRY diskGeometry;
    DISK_GEOMETRY_EX diskGeometryEx;
    DWORD bytesReturned;
    long long int totalNumberOfBytes = 0;
    STORAGE_PROPERTY_QUERY Query;
    char Buf[1024] = {0}, letter, sl, el, *c, fn[64] = "\\\\.\\C:";
    PSTORAGE_DEVICE_DESCRIPTOR pDevDesc = (PSTORAGE_DEVICE_DESCRIPTOR)Buf;
    pDevDesc->Size = sizeof(Buf);
    Query.PropertyId = StorageDeviceProperty;
    Query.QueryType = PropertyStandardQuery;

    memset(disks_targets, 0xff, sizeof(disks_targets));
    memset(disks_capacity, 0, sizeof(disks_capacity));
#if DISKS_TEST
    disks_targets[i++] = 'T';
    wsprintfW(szLbText, L"T: .\\test.bin");
    main_addToCombobox((char*)szLbText);
#endif
    hTargetDevice = CreateFileA(fn, GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, 0, NULL);
    if (hTargetDevice != INVALID_HANDLE_VALUE) {
        if(DeviceIoControl(hTargetDevice, IOCTL_VOLUME_GET_VOLUME_DISK_EXTENTS, NULL, 0, &volumeDiskExtents, sizeof volumeDiskExtents, &bytesReturned, NULL))
            cdrive = (int)volumeDiskExtents.Extents[0].DiskNumber;
        CloseHandle(hTargetDevice);
    }
    if(!disks_all) { sl = 'A'; el = 'Z'; } else { sl = '0'; el = '9'; }
    for(letter = sl; letter <= el; letter++) {
        hTargetDevice = INVALID_HANDLE_VALUE;
        if(!disks_all) {
            fn[4] = letter;
            /* fn[6] = '\\'; if(GetDriveType(fn) != DRIVE_REMOVABLE) continue; else fn[6] = 0; */
            if(letter == 'C') continue;
            hTargetDevice = CreateFileA(fn, GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, 0, NULL);
            if (hTargetDevice != INVALID_HANDLE_VALUE) {
                /* skip drive letters that are on the same physical disk as the C: drive */
                if(DeviceIoControl(hTargetDevice, IOCTL_VOLUME_GET_VOLUME_DISK_EXTENTS, NULL, 0, &volumeDiskExtents,
                        sizeof volumeDiskExtents, &bytesReturned, NULL) &&
                        cdrive != (int)volumeDiskExtents.Extents[0].DiskNumber) {
                    disks_targets[i] = (int)volumeDiskExtents.Extents[0].DiskNumber;
                } else {
                    if(verbose > 1) printf("%c: SKIP sysdisk\r\n", letter);
                    CloseHandle(hTargetDevice);
                    continue;
                }
            }
        } else {
            sprintf(fn, "\\\\.\\PhysicalDrive%c", letter);
            hTargetDevice = CreateFileA(fn, GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, FILE_FLAG_NO_BUFFERING, NULL);
            if (hTargetDevice != INVALID_HANDLE_VALUE) {
                disks_targets[i] = (int)(letter - '0');
            }
        }
        if(hTargetDevice != INVALID_HANDLE_VALUE) {
            totalNumberOfBytes = 0;
            if (DeviceIoControl(hTargetDevice, IOCTL_DISK_GET_DRIVE_GEOMETRY_EX, NULL, 0, &diskGeometryEx, sizeof diskGeometryEx, &bytesReturned, NULL)) {
                totalNumberOfBytes = (long long int)diskGeometryEx.DiskSize.QuadPart;
                if(verbose > 1) printf("%c: GeoEx %llu\r\n", letter, totalNumberOfBytes);
            } else
            if (DeviceIoControl(hTargetDevice, IOCTL_DISK_GET_DRIVE_GEOMETRY, NULL, 0, &diskGeometry, sizeof diskGeometry, &bytesReturned, NULL)) {
                totalNumberOfBytes = (long long int)diskGeometry.Cylinders.QuadPart * (long long int)diskGeometry.TracksPerCylinder * (long long int)diskGeometry.SectorsPerTrack * (long long int)diskGeometry.BytesPerSector;
                if(verbose > 1) printf("%c: Geo Cyl %llu Track %lu Sec %lu Bps %lu\r\n", letter, diskGeometry.Cylinders.QuadPart, diskGeometry.TracksPerCylinder, diskGeometry.SectorsPerTrack, diskGeometry.BytesPerSector);
            }
            if(!disks_all) {
                if(disks_maxsize > 0 && totalNumberOfBytes/1024LL > (long long int)disks_maxsize*1024LL*1024LL) {
                    if(verbose > 1) printf("%c: SKIP too big\r\n", letter);
                    continue;
                }
                /* don't use GetVolumeInformationByHandleW, that requires Vista / Server 2008 */
                memset(volName, 0, sizeof(volName));
                szLbText[0] = (wchar_t)letter; szLbText[1] = (wchar_t)':'; szLbText[2] = (wchar_t)'\\'; szLbText[3] = 0;
                k = MAX_PATH;
                GetVolumeInformationW(szLbText, volName, k, NULL, NULL, NULL, NULL, 0);
                j = 2;
            } else {
                wsprintfW(szLbText, L"Disk %d:", (int)(letter - '0'));
                j = 7;
            }
            if (totalNumberOfBytes > 0) {
                long long int sizeInGbTimes10 = ((long long int)(10 * totalNumberOfBytes) >> 30LL);
                wchar_t *unit = lang[L_GIB];
                if(sizeInGbTimes10 < 10) { unit = lang[L_MIB]; sizeInGbTimes10 = ((long long int)(10 * (totalNumberOfBytes + 1024LL*1024LL-1LL)) >> 20LL); }
                wsprintfW(siz, L" [%d.%d %s]", (int)(sizeInGbTimes10 / 10), (int)(sizeInGbTimes10 % 10), unit);
                for(wc = siz; *wc; wc++, j++) szLbText[j] = *wc;
            }
            if (DeviceIoControl(hTargetDevice, IOCTL_STORAGE_QUERY_PROPERTY, &Query,
                sizeof(STORAGE_PROPERTY_QUERY), pDevDesc, pDevDesc->Size, &bytesReturned,  (LPOVERLAPPED)NULL)) {
                if (pDevDesc->VendorIdOffset != 0) {
                    szLbText[j++] = (wchar_t)' ';
                    for(c = (PCHAR)((PBYTE)pDevDesc + pDevDesc->VendorIdOffset); *c == ' '; c++);
                    for(; *c && j < 1023; c++, j++) szLbText[j] = (wchar_t)*c;
                    while(j > 0 && szLbText[j-1] == (wchar_t)' ') j--;
                }
                if (pDevDesc->ProductIdOffset != 0) {
                    szLbText[j++] = (wchar_t)' ';
                    for(c = (PCHAR)((PBYTE)pDevDesc + pDevDesc->ProductIdOffset); *c == ' '; c++);
                    for(; *c && j < 1023; c++, j++) szLbText[j] = (wchar_t)*c;
                    while(j > 0 && szLbText[j-1] == (wchar_t)' ') j--;
                }
            }
            if(!disks_all && volName[0] && j < 1020) {
                szLbText[j++] = (wchar_t)' ';
                szLbText[j++] = (wchar_t)'(';
                for(k = 0; volName[k] == (wchar_t)' '; k++);
                for(; volName[k] && j < 1022; j++, k++) szLbText[j] = volName[k];
                while(j > 0 && szLbText[j-1] == (wchar_t)' ') j--;
                szLbText[j++] = (wchar_t)')';
            }
            szLbText[j] = 0;
            CloseHandle(hTargetDevice);
            disks_capacity[i++] = (uint64_t)totalNumberOfBytes;
            main_addToCombobox((char*)szLbText);
            if(i >= DISKS_MAX) break;
        }
    }
    if(disks_serial) {
        for(j = 0; j < 256; j++) {
            sprintf(fn, "\\\\.\\COM%d", j);
            hTargetDevice = CreateFileA(fn, GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING, FILE_FLAG_NO_BUFFERING, NULL);
            if(hTargetDevice != INVALID_HANDLE_VALUE) {
                CloseHandle(hTargetDevice);
                wsprintfW(szLbText, L"COM%d: %s", j, lang[L_SERIAL]);
                disks_targets[i++] = 1024 + j;
                main_addToCombobox((char*)szLbText);
            }
        }
    }
}

/**
 * Return mount points and bookmarks file
 */
char *disks_volumes(int *num, char ***mounts)
{
    /* nothing to do, we always use the OpenFile dialog box */
    (void)num;
    (void)mounts;
    return NULL;
}

/**
 * Lock, umount and open the target disk for writing
 */
void *disks_open(int targetId, uint64_t size)
{
    HANDLE ret;
    char fn[256], letter;
    VOLUME_DISK_EXTENTS volumeDiskExtents;
    DWORD bytesReturned;
    DCB config;
    COMMTIMEOUTS timeouts;
    int k;

    if(verbose) {
        if(targetId < 0 || targetId >= DISKS_MAX) printf("disks_open(%d) out of bounds\r\n", targetId);
        else if(disks_targets[targetId] == -1) printf("disks_open(%d) invalid handle\r\n", targetId);
        else if(disks_targets[targetId] == cdrive) printf("disks_open(%d) system disk\r\n", targetId);
        else if(size && disks_capacity[targetId] && size > disks_capacity[targetId]) printf("disks_open(%d) too small\r\n", targetId);
    }
    if(targetId < 0 || targetId >= DISKS_MAX || disks_targets[targetId] == -1 || (!disks_all && disks_targets[targetId] == cdrive)) return (HANDLE)-1;
    if(size && disks_capacity[targetId] && size > disks_capacity[targetId]) return (HANDLE)-1;

    if(disks_targets[targetId] >= 1024) {
        sprintf(fn, "\\\\.\\COM%d", disks_targets[targetId] - 1024);
        ret = CreateFileA(fn, GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING, FILE_FLAG_NO_BUFFERING, NULL);
        if(verbose) printf("disks_open(%s) serial\r\n  fd=%d errno=%d\r\n", fn, (int)ret, errno);
        if(ret == INVALID_HANDLE_VALUE) {
            main_getErrorMessage();
            return NULL;
        }
        if(GetCommState(ret, &config) == 0) {
            if(verbose) printf("  GetCommState error\r\n");
sererr:     main_getErrorMessage();
            CloseHandle(ret);
            return (HANDLE)-4;
        }
        config.BaudRate = baud;
        config.ByteSize = 8;
        config.Parity = NOPARITY;
        config.StopBits = ONESTOPBIT;
        if(SetCommState(ret, &config) == 0) {
            if(verbose) printf("  SetCommState error baud %d\r\n", baud);
            goto sererr;
        }
        timeouts.ReadIntervalTimeout = 1;
        timeouts.ReadTotalTimeoutMultiplier = 1;
        timeouts.ReadTotalTimeoutConstant = 1;
        timeouts.WriteTotalTimeoutMultiplier = 1;
        timeouts.WriteTotalTimeoutConstant = 1;
        if(SetCommTimeouts(ret, &timeouts) == 0) {
            if(verbose) printf("  SetCommTimeouts error\r\n");
            goto sererr;
        }
        if(disks_serial == 2) {
            /* Raspbootin Serial Protocol:
             *   \003\003\003 - client to server
             *   4 bytes little-endian, size of image - server to client
             *   'OK' or 'SE' (size error) - client to server
             *   image content - server to client
             */
            if(verbose) printf("  awaiting client\r\n");
            for(k = 0; k < 3;) {
                main_onProgress(NULL);
                if(ReadFile(ret, fn, (DWORD)1, (LPDWORD)&bytesReturned, NULL) && bytesReturned == 1) {
                    if(fn[0] == 3) k++;
                    else {
                        k = 0;
                        if(verbose) printf("%c", fn[0]);
                    }
                }
            }
            if(verbose) printf("\r\n  client connected\r\n");
            if(!WriteFile(ret, (char*)&size, (DWORD)4, (LPDWORD)&bytesReturned, NULL) || bytesReturned != 4) {
                if(verbose)
                    printf(" unable to send size errno=%d\r\n", errno);
                goto sererr;
            }
            if(!ReadFile(ret, fn, (DWORD)2, (LPDWORD)&bytesReturned, NULL) || bytesReturned != 2 ||
                fn[0] != 'O' || fn[1] != 'K') {
                if(verbose)
                    printf(" didn't received ACK from client, got '%c%c' errno=%d\r\n",
                        fn[0], fn[1], errno);
                goto sererr;
            }
        }
        return (void*)ret;
    }
#if DISKS_TEST
    if((char)disks_targets[targetId] == 'T') {
        sprintf(fn, ".\\test.bin");
        ret = CreateFileA(fn, GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, CREATE_ALWAYS, FILE_FLAG_NO_BUFFERING, NULL);
        if(verbose)
            printf("disks_open(%s)\r\n  fd=%p\r\n", fn, ret);
        if (ret == INVALID_HANDLE_VALUE) {
            main_getErrorMessage();
            return NULL;
        }
        return (void*)ret;
    }
#endif
    /* dismount volumes */
    memset(hLocks, 0, sizeof(hLocks));
    nLocks = 0;
    for(letter = 'A'; letter <= 'Z'; letter++) {
        sprintf(fn, "\\\\.\\%c:", letter);
        ret = CreateFileA(fn, GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, 0, NULL);
        if (ret == INVALID_HANDLE_VALUE) continue;
        if(!DeviceIoControl(ret, IOCTL_VOLUME_GET_VOLUME_DISK_EXTENTS, NULL, 0, &volumeDiskExtents, sizeof volumeDiskExtents, &bytesReturned, NULL) ||
            (unsigned int)disks_targets[targetId] != (unsigned int)volumeDiskExtents.Extents[0].DiskNumber) {
                CloseHandle(ret);
                continue;
        }
        k = DeviceIoControl(ret, FSCTL_LOCK_VOLUME, NULL, 0, NULL, 0, &bytesReturned, NULL);
        DWORD dw = GetLastError();
        if(verbose)
            printf("lock(%d) (%s) ret=%d, gle=%d\r\n", nLocks, fn, k, dw);
        if(k) {
            hLocks[nLocks++] = ret;
            k = DeviceIoControl(ret, FSCTL_DISMOUNT_VOLUME, NULL, 0, NULL, 0, &bytesReturned, NULL);
            if(verbose)
                printf("umount(%s) ret=%d\r\n", fn, k);
        }
        if(!k) {
            main_getErrorMessage();
            disks_close(NULL);
            return (HANDLE)-2;
        }
    }
    /* open raw disk */
    sprintf(fn, "\\\\.\\PhysicalDrive%d", disks_targets[targetId]);
    ret = CreateFileA(fn, GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, FILE_FLAG_NO_BUFFERING, NULL);
    if(verbose)
        printf("disks_open(%s)\r\n  fd=%d nLocks=%d\r\n", fn, (int)ret, nLocks);
    if (ret == INVALID_HANDLE_VALUE) {
        main_getErrorMessage();
        disks_close(NULL);
        return (HANDLE)-3;
    }
    return (void*)ret;
}

/**
 * Close the target disk
 */
void disks_close(void *data)
{
    DWORD bytesReturned;
    HANDLE hnd = (HANDLE)data;
    int i, k;

    if(verbose)
        printf("disks_close(%d) nLocks=%d\r\n", (int)data, nLocks);

    if(hnd != NULL && hnd != (HANDLE)-1 && hnd != (HANDLE)-2 && hnd != (HANDLE)-3 && hnd != (HANDLE)-4)
        CloseHandle(hnd);

    for(i = 0; i < nLocks; i++) {
        k = DeviceIoControl(hLocks[i], FSCTL_UNLOCK_VOLUME, NULL, 0, NULL, 0, &bytesReturned, NULL);
        if(verbose)
            printf("unlock(%d) ret=%d\r\n", i, k);
        CloseHandle(hLocks[i]);
        hLocks[i] = NULL;
    }
    nLocks = 0;
}
