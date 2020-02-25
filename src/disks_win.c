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

#include <windows.h>
#include <winioctl.h>
#include <commctrl.h>
#include <ddk/ntdddisk.h>
#include "lang.h"
#include "main.h"
#include "disks.h"

int disks_serial = 0, disks_targets[DISKS_MAX];
uint64_t disks_capacity[DISKS_MAX];

HANDLE hTargetVolume = NULL;

/**
 * Refresh target device list in the combobox
 */
void disks_refreshlist() {
    int i = 0, j;
    wchar_t szLbText[1024];
    HANDLE hTargetDevice;
    DISK_GEOMETRY diskGeometry;
    DISK_GEOMETRY_EX diskGeometryEx;
    DWORD bytesReturned;
    long long int totalNumberOfBytes = 0;
    STORAGE_PROPERTY_QUERY Query;
    char Buf[1024] = {0}, letter, *c, siz[64], fn[64] = "\\\\.\\X:";
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
    for(letter = 'A'; letter <= 'Z'; letter++) {
        fn[4] = letter;
        /* fn[6] = '\\'; if(GetDriveType(fn) != DRIVE_REMOVABLE) continue; else fn[6] = 0; */
        if(letter == 'C') continue;
        hTargetDevice = CreateFileA(fn, GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, 0, NULL);
        if (hTargetDevice != INVALID_HANDLE_VALUE) {
            totalNumberOfBytes = 0;
            if (DeviceIoControl(hTargetDevice, IOCTL_DISK_GET_DRIVE_GEOMETRY_EX, NULL, 0, &diskGeometryEx, sizeof diskGeometryEx, &bytesReturned, NULL)) {
                totalNumberOfBytes = (long long int)diskGeometryEx.DiskSize.QuadPart;
            } else
            if (DeviceIoControl(hTargetDevice, IOCTL_DISK_GET_DRIVE_GEOMETRY, NULL, 0, &diskGeometry, sizeof diskGeometry, &bytesReturned, NULL)) {
                totalNumberOfBytes = (long long int)diskGeometry.Cylinders.QuadPart * (long long int)diskGeometry.TracksPerCylinder * (long long int)diskGeometry.SectorsPerTrack * (long long int)diskGeometry.BytesPerSector;
            }
            szLbText[0] = (wchar_t)letter; szLbText[1] = (wchar_t)':';
            j = 2;
            if (totalNumberOfBytes > 0) {
                int sizeInGbTimes10 = (int)((ULONG)(10 * (totalNumberOfBytes + 1024L*1024L*1024L-1L)) >> 30L);
                char unit = 'G';
                if(!sizeInGbTimes10) { unit = 'M'; sizeInGbTimes10 = (int)((ULONG)(10 * (totalNumberOfBytes + 1024L*1024L-1L)) >> 20L); }
                sprintf(siz, " [%d.%d %ciB]", sizeInGbTimes10 / 10, sizeInGbTimes10 % 10, unit);
                for(c = siz; *c; c++, j++) szLbText[j] = (wchar_t)*c;
            }
            if (DeviceIoControl(hTargetDevice, IOCTL_STORAGE_QUERY_PROPERTY, &Query,
                sizeof(STORAGE_PROPERTY_QUERY), pDevDesc, pDevDesc->Size, &bytesReturned,  (LPOVERLAPPED)NULL)) {
                if (pDevDesc->VendorIdOffset != 0) {
                    szLbText[j++] = (wchar_t)' ';
                    for(c = (PCHAR)((PBYTE)pDevDesc + pDevDesc->VendorIdOffset); *c; c++, j++) szLbText[j] = (wchar_t)*c;
                }
                if (pDevDesc->ProductIdOffset != 0) {
                    szLbText[j++] = (wchar_t)' ';
                    for(c = (PCHAR)((PBYTE)pDevDesc + pDevDesc->ProductIdOffset); *c; c++, j++) szLbText[j] = (wchar_t)*c;
                }
            }
            szLbText[j] = 0;
            CloseHandle(hTargetDevice);
            disks_capacity[i] = (uint64_t)totalNumberOfBytes;
            disks_targets[i++] = letter;
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
    char szVolumePathName[256] = "\\\\.\\X:", szDevicePathName[256];
    VOLUME_DISK_EXTENTS volumeDiskExtents;
    DWORD bytesReturned;
    DCB config;
    COMMTIMEOUTS timeouts;
    int k;

    if(targetId < 0 || targetId >= DISKS_MAX || disks_targets[targetId] == -1 || disks_targets[targetId] == 'C') return (HANDLE)-1;
    if(size && disks_capacity[targetId] && size > disks_capacity[targetId]) return (HANDLE)-1;

    if(disks_targets[targetId] >= 1024) {
        sprintf(szDevicePathName, "\\\\.\\COM%d", disks_targets[targetId] - 1024);
        ret = CreateFileA(szDevicePathName, GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING, FILE_FLAG_NO_BUFFERING, NULL);
        if(verbose) printf("disks_open(%s) serial\r\n  fd=%d errno=%d\r\n", szDevicePathName, ret, errno);
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
        config.BaudRate = 115200;
        config.ByteSize = 8;
        config.Parity = NOPARITY;
        config.StopBits = ONESTOPBIT;
        if(SetCommState(ret, &config) == 0) {
            if(verbose) printf("  SetCommState error\r\n");
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
                if(ReadFile(ret, szVolumePathName, (DWORD)1, (LPDWORD)&bytesReturned, NULL) && bytesReturned == 1) {
                    if(szVolumePathName[0] == 3) k++;
                    else {
                        k = 0;
                        if(verbose) printf("%c", szVolumePathName[0]);
                    }
                }
            }
            if(verbose) printf("\r\n  client connected\r\n");
            if(!WriteFile(ret, (char*)&size, (DWORD)4, (LPDWORD)&bytesReturned, NULL) || bytesReturned != 4) {
                if(verbose)
                    printf(" unable to send size errno=%d\r\n", errno);
                goto sererr;
            }
            if(!ReadFile(ret, szVolumePathName, (DWORD)2, (LPDWORD)&bytesReturned, NULL) || bytesReturned != 2 ||
                szVolumePathName[0] != 'O' || szVolumePathName[1] != 'K') {
                if(verbose)
                    printf(" didn't received ACK from client, got '%c%c' errno=%d\r\n",
                        szVolumePathName[0], szVolumePathName[1], errno);
                goto sererr;
            }
        }
        return (void*)ret;
    }
#if DISKS_TEST
    if((char)disks_targets[targetId] == 'T') {
        hTargetVolume = NULL;
        sprintf(szDevicePathName, ".\\test.bin");
        ret = CreateFileA(szDevicePathName, GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, CREATE_ALWAYS, FILE_FLAG_NO_BUFFERING, NULL);
        if(verbose)
            printf("disks_open(%s)\r\n  fd=%d\r\n", szDevicePathName, ret);
        if (ret == INVALID_HANDLE_VALUE) {
            main_getErrorMessage();
            return NULL;
        }
        return (void*)ret;
    }
#endif
    szVolumePathName[4] = (char)disks_targets[targetId];
    hTargetVolume = CreateFileA(szVolumePathName, GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, 0, NULL);
    if (hTargetVolume == INVALID_HANDLE_VALUE) {
        main_getErrorMessage();
        return (HANDLE)-3;
    }

    if (DeviceIoControl(hTargetVolume, FSCTL_LOCK_VOLUME, NULL, 0, NULL, 0, &bytesReturned, NULL) &&
        DeviceIoControl(hTargetVolume, IOCTL_VOLUME_GET_VOLUME_DISK_EXTENTS, NULL, 0, &volumeDiskExtents, sizeof volumeDiskExtents, &bytesReturned, NULL) &&
        DeviceIoControl(hTargetVolume, FSCTL_DISMOUNT_VOLUME, NULL, 0, NULL, 0, &bytesReturned, NULL)) {
            sprintf(szDevicePathName, "\\\\.\\PhysicalDrive%u", (unsigned int)volumeDiskExtents.Extents[0].DiskNumber);
            ret = CreateFileA(szDevicePathName, GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, FILE_FLAG_NO_BUFFERING, NULL);
            if(verbose)
                printf("disks_open(%s)\r\n  fd=%d\r\n", szDevicePathName, ret);
            if (ret == INVALID_HANDLE_VALUE) {
                main_getErrorMessage();
                DeviceIoControl(hTargetVolume, FSCTL_UNLOCK_VOLUME, NULL, 0, NULL, 0, &bytesReturned, NULL);
                CloseHandle(hTargetVolume);
                hTargetVolume = NULL;
                return NULL;
            }
    } else {
        main_getErrorMessage();
        CloseHandle(hTargetVolume);
        hTargetVolume = NULL;
        return (HANDLE)-2;
    }
    return (void*)ret;
}

/**
 * Close the target disk
 */
void disks_close(void *data)
{
    DWORD bytesReturned;

    CloseHandle((HANDLE)data);

    if(hTargetVolume) {
        DeviceIoControl(hTargetVolume, FSCTL_UNLOCK_VOLUME, NULL, 0, NULL, 0, &bytesReturned, NULL);
        CloseHandle(hTargetVolume);
        hTargetVolume = NULL;
    }
}
