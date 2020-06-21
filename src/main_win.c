/*
 * usbimager/main_win.c
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
 * @brief User interface for Windows
 *
 */

#include <windows.h>
#include <winnls.h>
#include <winioctl.h>
#include <commctrl.h>
#include <shlobj.h>
#include <io.h>
#include <fcntl.h>
#include <sys/stat.h>
#include "lang.h"
#include "resource.h"
#include "stream.h"
#include "disks.h"

#ifndef DBT_DEVICEARRIVAL
#define DBT_DEVICEARRIVAL 0x8000
#endif
#ifndef DBT_DEVICEREMOVECOMPLETE
#define DBT_DEVICEREMOVECOMPLETE 0x8004
#endif

_CRTIMP __cdecl __MINGW_NOTHROW  FILE * _fdopen (int, const char *);

wchar_t **lang;
extern char *dict[NUMLANGS][NUMTEXTS + 1];
int blksizesel = 0;

static HWND mainHwndDlg;
static wchar_t *bkpdir = NULL;

wchar_t *main_errorMessage;

void main_addToCombobox(char *option)
{
    wchar_t *msg = (wchar_t*)option;
    SendDlgItemMessageW(mainHwndDlg, IDC_MAINDLG_TARGET_LIST, CB_ADDSTRING, 0, (LPARAM)msg);
}

void main_getErrorMessage()
{
    if(main_errorMessage) {
        LocalFree(main_errorMessage);
        main_errorMessage = NULL;
    }
    if(GetLastError())
        FormatMessageW(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS, NULL,
            GetLastError(), MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPWSTR)&main_errorMessage, 0, NULL);
}

void main_onProgress(void *data)
{
    (void)data;
    SendDlgItemMessage(mainHwndDlg, IDC_MAINDLG_PROGRESSBAR, PBM_SETPOS, 0, 0);
    SetWindowTextW(GetDlgItem(mainHwndDlg, IDC_MAINDLG_STATUS), lang[L_WAITING]);
    ShowWindow(GetDlgItem(mainHwndDlg, IDC_MAINDLG_STATUS), SW_HIDE);
    ShowWindow(GetDlgItem(mainHwndDlg, IDC_MAINDLG_STATUS), SW_SHOW);
}

static void MainDlgMsgBox(HWND hwndDlg, wchar_t *message)
{
    wchar_t msg[1024], *err = main_errorMessage;
    int i = 0;
    if(main_errorMessage && *main_errorMessage) {
        for(; err[i]; i++) msg[i] = err[i];
        msg[i++] = (wchar_t)'\r';
        msg[i++] = (wchar_t)'\n';
    }
    for(; *message; i++, message++)
        msg[i] = *message;
    MessageBoxW(hwndDlg, msg, lang[L_ERROR], MB_ICONERROR);
}

static void onDone(HWND hwndDlg)
{
    LRESULT index = SendDlgItemMessage(hwndDlg, IDC_MAINDLG_TARGET_LIST, CB_GETCURSEL, 0, 0);
    EnableWindow(GetDlgItem(hwndDlg, IDC_MAINDLG_SOURCE), TRUE);
    EnableWindow(GetDlgItem(hwndDlg, IDC_MAINDLG_SELECT), TRUE);
    EnableWindow(GetDlgItem(hwndDlg, IDC_MAINDLG_TARGET_LIST),TRUE);
    EnableWindow(GetDlgItem(hwndDlg, IDC_MAINDLG_WRITE), TRUE);
    EnableWindow(GetDlgItem(hwndDlg, IDC_MAINDLG_READ), index >= 0 && index < DISKS_MAX && disks_targets[index] >= 1024 ? FALSE : TRUE);
    EnableWindow(GetDlgItem(hwndDlg, IDC_MAINDLG_VERIFY), TRUE);
    EnableWindow(GetDlgItem(hwndDlg, IDC_MAINDLG_COMPRESS), TRUE);
    EnableWindow(GetDlgItem(hwndDlg, IDC_MAINDLG_BLKSIZE), TRUE);
    SendDlgItemMessage(hwndDlg, IDC_MAINDLG_PROGRESSBAR, PBM_SETPOS, 0, 0);
    ShowWindow(GetDlgItem(hwndDlg, IDC_MAINDLG_STATUS), SW_HIDE);
    ShowWindow(GetDlgItem(hwndDlg, IDC_MAINDLG_STATUS), SW_SHOW);
}

/**
 * Function that reads from input and writes to disk
 */
static DWORD WINAPI writerRoutine(LPVOID lpParam) {
    HWND hwndDlg = (HWND) lpParam;
    wchar_t szFilePathName[MAX_PATH];
    LARGE_INTEGER totalNumberOfBytesWritten;
    DWORD pos = 0;
    HANDLE hTargetDevice;
    LRESULT index = SendDlgItemMessage(hwndDlg, IDC_MAINDLG_TARGET_LIST, CB_GETCURSEL, 0, 0);
    static wchar_t lpStatus[128];
    static stream_t ctx;
    int ret = 1, len, wlen;
    char *fn;

    ctx.fileSize = 0;
    GetDlgItemTextW(hwndDlg, IDC_MAINDLG_SOURCE, szFilePathName, sizeof(szFilePathName) / sizeof(szFilePathName[0]));
    for(wlen = 0; szFilePathName[wlen]; wlen++);
    len = WideCharToMultiByte(CP_UTF8, 0, szFilePathName, wlen, 0, 0, NULL, NULL);
    if(len > 0) {
        fn = (char*)malloc(len+1);
        if(fn) {
            WideCharToMultiByte(CP_UTF8, 0, szFilePathName, wlen, fn, len, NULL, NULL);
            fn[len] = 0;
            ret = stream_open(&ctx, fn, index >= 0 && index < DISKS_MAX && disks_targets[index] >= 1024);
            free(fn);
        }
    }

    if (!ret) {
        int needVerify = IsDlgButtonChecked(hwndDlg, IDC_MAINDLG_VERIFY);

        hTargetDevice = index == CB_ERR ? (HANDLE)-1 : (HANDLE)disks_open((int)index, ctx.fileSize);
        if (hTargetDevice != NULL && hTargetDevice != (HANDLE)-1 && hTargetDevice != (HANDLE)-2 && hTargetDevice != (HANDLE)-3 && hTargetDevice != (HANDLE)-4) {
            totalNumberOfBytesWritten.QuadPart = 0;

            while(1) {
                int numberOfBytesRead;

                if((numberOfBytesRead = stream_read(&ctx)) >= 0) {
                    if(numberOfBytesRead == 0) {
                        if(!ctx.fileSize) ctx.fileSize = ctx.readSize;
                        break;
                    } else {
                        DWORD numberOfBytesWritten, numberOfBytesVerify;

                        if (WriteFile(hTargetDevice, ctx.buffer, numberOfBytesRead, &numberOfBytesWritten, NULL)) {
                            if(verbose) printf("WriteFile(%d) numberOfBytesWritten %d\r\n", numberOfBytesRead, numberOfBytesWritten);
                            if(needVerify) {
                                SetFilePointerEx(hTargetDevice, totalNumberOfBytesWritten, NULL, FILE_BEGIN);
                                if(!ReadFile(hTargetDevice, ctx.verifyBuf, numberOfBytesWritten, &numberOfBytesVerify, NULL) ||
                                    numberOfBytesWritten != numberOfBytesVerify || memcmp(ctx.buffer, ctx.verifyBuf, numberOfBytesWritten)) {
                                    MessageBoxW(hwndDlg, lang[L_VRFYERR], lang[L_ERROR], MB_ICONERROR);
                                    break;
                                }
                                if(verbose) printf("  numberOfBytesVerify %lu\n", numberOfBytesVerify);
                                totalNumberOfBytesWritten.QuadPart += numberOfBytesWritten;
                            }

                            pos = (DWORD) stream_status(&ctx, (char*)&lpStatus, 0);
                            SendDlgItemMessage(hwndDlg, IDC_MAINDLG_PROGRESSBAR, PBM_SETPOS, pos, 0);
                            SetWindowTextW(GetDlgItem(hwndDlg, IDC_MAINDLG_STATUS), lpStatus);
                            ShowWindow(GetDlgItem(hwndDlg, IDC_MAINDLG_STATUS), SW_HIDE);
                            ShowWindow(GetDlgItem(hwndDlg, IDC_MAINDLG_STATUS), SW_SHOW);
                        } else {
                            if(verbose) printf("WriteFile(%d) numberOfBytesWritten %d ERROR\r\n", numberOfBytesRead, numberOfBytesWritten);
                            main_getErrorMessage();
                            MainDlgMsgBox(hwndDlg, lang[L_WRTRGERR]);
                            break;
                        }
                    }
                } else {
                    MainDlgMsgBox(hwndDlg, lang[L_RDSRCERR]);
                    break;
                }
            }
            disks_close((void*)hTargetDevice);
        } else {
            MainDlgMsgBox(hwndDlg, lang[
                hTargetDevice == (HANDLE)-1 ? L_TRGERR :
                (hTargetDevice == (HANDLE)-2 ? L_DISMOUNTERR :
                (hTargetDevice == (HANDLE)-3 ? L_OPENVOLERR :
                (hTargetDevice == (HANDLE)-4 ? L_COMMERR :
                L_OPENTRGERR)))]);
        }
        stream_close(&ctx);
    } else {
        main_getErrorMessage();
        MainDlgMsgBox(hwndDlg, lang[ret == 2 ? L_ENCZIPERR :
            (ret == 3 ? L_CMPZIPERR :
            (ret == 4 ? L_CMPERR :
            L_SRCERR))]);
    }
    if(main_errorMessage) {
        LocalFree(main_errorMessage);
        main_errorMessage = NULL;
    }

    stream_status(&ctx, (char*)&lpStatus, 1);
    SetWindowTextW(GetDlgItem(hwndDlg, IDC_MAINDLG_STATUS), lpStatus);
    onDone(hwndDlg);
    if(verbose) printf("Worker thread finished.\r\n");
    return 0;
}

INT_PTR MainDlgWriteClick(HWND hwndDlg) {
    EnableWindow(GetDlgItem(hwndDlg, IDC_MAINDLG_SOURCE), FALSE);
    EnableWindow(GetDlgItem(hwndDlg, IDC_MAINDLG_SELECT), FALSE);
    EnableWindow(GetDlgItem(hwndDlg, IDC_MAINDLG_TARGET_LIST), FALSE);
    EnableWindow(GetDlgItem(hwndDlg, IDC_MAINDLG_WRITE), FALSE);
    EnableWindow(GetDlgItem(hwndDlg, IDC_MAINDLG_READ), FALSE);
    EnableWindow(GetDlgItem(hwndDlg, IDC_MAINDLG_VERIFY), FALSE);
    EnableWindow(GetDlgItem(hwndDlg, IDC_MAINDLG_COMPRESS), FALSE);
    EnableWindow(GetDlgItem(hwndDlg, IDC_MAINDLG_BLKSIZE), FALSE);
    SendDlgItemMessage(hwndDlg, IDC_MAINDLG_PROGRESSBAR, PBM_SETPOS, 0, 0);
    SetWindowTextW(GetDlgItem(hwndDlg, IDC_MAINDLG_STATUS), L"");
    ShowWindow(GetDlgItem(hwndDlg, IDC_MAINDLG_STATUS), SW_HIDE);
    ShowWindow(GetDlgItem(hwndDlg, IDC_MAINDLG_STATUS), SW_SHOW);
    if(main_errorMessage) {
        LocalFree(main_errorMessage);
        main_errorMessage = NULL;
    }

    if(verbose) printf("Starting worker thread.\r\n");
    CloseHandle(CreateThread(NULL, 0, writerRoutine, hwndDlg, 0, NULL));
    return TRUE;
}

/**
 * Function that reads from disk and writes to output file
 */
static DWORD WINAPI readerRoutine(LPVOID lpParam) {
    HWND hwndDlg = (HWND) lpParam;
    HANDLE src;
    LRESULT targetId = SendDlgItemMessage(hwndDlg, IDC_MAINDLG_TARGET_LIST, CB_GETCURSEL, 0, 0);
    int size, wlen, len, needCompress = IsDlgButtonChecked(hwndDlg, IDC_MAINDLG_COMPRESS);
    DWORD numberOfBytesRead;
    char *fn = NULL;
    static wchar_t lpStatus[128];
    static stream_t ctx;
    struct _stat st;
    wchar_t home[MAX_PATH], d[16], t[8], wFn[MAX_PATH+512];

    if(targetId >= 0 && targetId < DISKS_MAX && disks_targets[targetId] >= 1024) return 0;

    ctx.fileSize = 0;
    src = targetId == CB_ERR ? (HANDLE)-1 : (HANDLE)disks_open((int)targetId, 0);
    if(src != NULL && src != (HANDLE)-1 && src != (HANDLE)-2 && src != (HANDLE)-3 && src != (HANDLE)-4) {
        if(SHGetFolderPathW(HWND_DESKTOP, CSIDL_DESKTOPDIRECTORY, NULL, 0, home))
            wsprintfW(home, L".\\");
        GetDateFormatW(LOCALE_USER_DEFAULT, 0, NULL, L"yyyyMMdd", (LPWSTR)&d, 16);
        GetTimeFormatW(LOCALE_USER_DEFAULT, 0, NULL, L"HHmm", (LPWSTR)&t, 8);
        wsprintfW(wFn, L"%s\\usbimager-%sT%s.dd%s", bkpdir && !_wstat(bkpdir, &st) ? bkpdir : home, d, t, needCompress ? L".bz2" : L"");
        for(wlen = 0; wFn[wlen]; wlen++);
        len = WideCharToMultiByte(CP_UTF8, 0, wFn, wlen, 0, 0, NULL, NULL);
        if(len > 0) {
            fn = (char*)malloc(len+1);
            if(fn) {
                WideCharToMultiByte(CP_UTF8, 0, wFn, wlen, fn, len, NULL, NULL);
                fn[len] = 0;
            }
        }
        SetWindowTextW(GetDlgItem(hwndDlg, IDC_MAINDLG_SOURCE), wFn);
        ShowWindow(GetDlgItem(hwndDlg, IDC_MAINDLG_SOURCE), SW_HIDE);
        ShowWindow(GetDlgItem(hwndDlg, IDC_MAINDLG_SOURCE), SW_SHOW);
        if(fn && !stream_create(&ctx, fn, needCompress, disks_capacity[targetId])) {
            while(ctx.readSize < ctx.fileSize) {
                errno = 0;
                size = ctx.fileSize - ctx.readSize < (uint64_t)buffer_size ? (int)(ctx.fileSize - ctx.readSize) : buffer_size;
                if(ReadFile(src, ctx.buffer, size, &numberOfBytesRead, NULL)) {
                    if(verbose) printf("ReadFile(%d) numberOfBytesRead %d\r\n", size, numberOfBytesRead);
                    if(stream_write(&ctx, ctx.buffer, size)) {
                        DWORD pos = (DWORD) stream_status(&ctx, (char*)&lpStatus, 0);
                        SendDlgItemMessage(hwndDlg, IDC_MAINDLG_PROGRESSBAR, PBM_SETPOS, pos, 0);
                        SetWindowTextW(GetDlgItem(hwndDlg, IDC_MAINDLG_STATUS), lpStatus);
                        ShowWindow(GetDlgItem(hwndDlg, IDC_MAINDLG_STATUS), SW_HIDE);
                        ShowWindow(GetDlgItem(hwndDlg, IDC_MAINDLG_STATUS), SW_SHOW);
                    } else {
                        MainDlgMsgBox(hwndDlg, lang[L_WRIMGERR]);
                    }
                } else {
                    if(verbose) printf("ReadFile(%d) numberOfBytesRead %d ERROR\r\n", size, numberOfBytesRead);
                    MainDlgMsgBox(hwndDlg, lang[L_RDSRCERR]);
                    break;
                }
            }
            stream_close(&ctx);
        } else {
            MainDlgMsgBox(hwndDlg, lang[L_OPENIMGERR]);
        }
        disks_close((void*)((long int)src));
    } else {
        MainDlgMsgBox(hwndDlg, lang[src == (HANDLE)-1 ? L_TRGERR : (src == (HANDLE)-2 ? L_UMOUNTERR : (src == (HANDLE)-4 ? L_COMMERR : L_OPENTRGERR))]);
    }
    if(fn) free(fn);
    stream_status(&ctx, (char*)&lpStatus, 1);
    SetWindowTextW(GetDlgItem(hwndDlg, IDC_MAINDLG_STATUS), lpStatus);
    onDone(hwndDlg);
    SendDlgItemMessage(hwndDlg, IDC_MAINDLG_SOURCE, EM_SETSEL, 0, -1);
    SendDlgItemMessage(hwndDlg, IDC_MAINDLG_SOURCE, WM_COPY, 0, 0);
    if(verbose) printf("Worker thread finished.\r\n");
    return 0;
}

INT_PTR MainDlgReadClick(HWND hwndDlg) {
    EnableWindow(GetDlgItem(hwndDlg, IDC_MAINDLG_SOURCE), FALSE);
    EnableWindow(GetDlgItem(hwndDlg, IDC_MAINDLG_SELECT), FALSE);
    EnableWindow(GetDlgItem(hwndDlg, IDC_MAINDLG_TARGET_LIST), FALSE);
    EnableWindow(GetDlgItem(hwndDlg, IDC_MAINDLG_WRITE), FALSE);
    EnableWindow(GetDlgItem(hwndDlg, IDC_MAINDLG_READ), FALSE);
    EnableWindow(GetDlgItem(hwndDlg, IDC_MAINDLG_VERIFY), FALSE);
    EnableWindow(GetDlgItem(hwndDlg, IDC_MAINDLG_COMPRESS), FALSE);
    EnableWindow(GetDlgItem(hwndDlg, IDC_MAINDLG_BLKSIZE), FALSE);
    SendDlgItemMessage(hwndDlg, IDC_MAINDLG_PROGRESSBAR, PBM_SETPOS, 0, 0);
    SetWindowTextW(GetDlgItem(hwndDlg, IDC_MAINDLG_STATUS), L"");
    ShowWindow(GetDlgItem(hwndDlg, IDC_MAINDLG_STATUS), SW_HIDE);
    ShowWindow(GetDlgItem(hwndDlg, IDC_MAINDLG_STATUS), SW_SHOW);
    if(main_errorMessage) {
        LocalFree(main_errorMessage);
        main_errorMessage = NULL;
    }

    if(verbose) printf("Starting worker thread for reading.\r\n");
    CloseHandle(CreateThread(NULL, 0, readerRoutine, hwndDlg, 0, NULL));
    return TRUE;
}

INT_PTR MainDlgRefreshTarget(HWND hwndDlg) {
    LRESULT index = SendDlgItemMessage(hwndDlg, IDC_MAINDLG_TARGET_LIST, CB_GETCURSEL, 0, 0);
    SendDlgItemMessage(hwndDlg, IDC_MAINDLG_TARGET_LIST, CB_RESETCONTENT, 0, 0);
    disks_refreshlist();
    if(index != CB_ERR) {
        SendDlgItemMessage(hwndDlg, IDC_MAINDLG_TARGET_LIST, CB_SETCURSEL, (WPARAM) index, 0);
        if(index >= 0 && index < DISKS_MAX && disks_targets[index] >= 1024) {
            SetDlgItemTextW(hwndDlg, IDC_MAINDLG_WRITE, lang[L_SEND]);
            EnableWindow(GetDlgItem(hwndDlg, IDC_MAINDLG_READ), FALSE);
        } else {
            SetDlgItemTextW(hwndDlg, IDC_MAINDLG_WRITE, lang[L_WRITE]);
            EnableWindow(GetDlgItem(hwndDlg, IDC_MAINDLG_READ), TRUE);
        }
    }
    return TRUE;
}

INT_PTR MainDlgSelectClick(HWND hwndDlg) {
    OPENFILENAME ofn;
    TCHAR lpstrFile[MAX_PATH];

    MainDlgRefreshTarget(hwndDlg);

    SetWindowTextW(GetDlgItem(hwndDlg, IDC_MAINDLG_STATUS), L"");
    ZeroMemory(&ofn, sizeof ofn);
    ZeroMemory(lpstrFile, sizeof lpstrFile);

    ofn.lStructSize = sizeof ofn;
    ofn.hwndOwner = hwndDlg;
    ofn.lpstrFilter = TEXT("(*.*)\0*.*\0\0");
    ofn.lpstrFile = lpstrFile;
    ofn.nMaxFile = MAX_PATH;
    ofn.Flags = OFN_FILEMUSTEXIST;

    if (GetOpenFileName(&ofn)) {
        SetDlgItemText(hwndDlg, IDC_MAINDLG_SOURCE, ofn.lpstrFile);
    }

    return TRUE;
}

static INT_PTR CALLBACK MainDlgProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    HICON wrico, rdico;
    UNREFERENCED_PARAMETER(lParam);
    LRESULT index;
    wchar_t tmp[16];

    switch (uMsg) {
        case WM_INITDIALOG:
            mainHwndDlg = hwndDlg;
            SendMessage(hwndDlg, WM_SETICON, ICON_SMALL, (LPARAM) LoadIcon((HINSTANCE) lParam, MAKEINTRESOURCE(IDI_APP_ICON)));
            /* ▲ 25b2  ▼ 28bc */
            wrico=LoadImage(GetModuleHandle(NULL),MAKEINTRESOURCE(IDI_WRITE), IMAGE_ICON, 16, 16, 0);
            SendMessage(GetDlgItem(hwndDlg,IDC_MAINDLG_WRITE),BM_SETIMAGE, (WPARAM)IMAGE_ICON,(LPARAM)wrico);
            SetDlgItemTextW(hwndDlg, IDC_MAINDLG_WRITE, lang[L_WRITE]);
            rdico=LoadImage(GetModuleHandle(NULL),MAKEINTRESOURCE(IDI_READ), IMAGE_ICON, 16, 16, 0);
            SendMessage(GetDlgItem(hwndDlg,IDC_MAINDLG_READ),BM_SETIMAGE, (WPARAM)IMAGE_ICON,(LPARAM)rdico);
            SetDlgItemTextW(hwndDlg, IDC_MAINDLG_READ, lang[L_READ]);
            SetWindowTextW(GetDlgItem(hwndDlg, IDC_MAINDLG_VERIFY), lang[L_VERIFY]);
            SetWindowTextW(GetDlgItem(hwndDlg, IDC_MAINDLG_COMPRESS), lang[L_COMPRESS]);
            for(index = 0; index < 10; index++) {
                wsprintfW(tmp, L"%dM", (1<<index));
                SendDlgItemMessageW(mainHwndDlg, IDC_MAINDLG_BLKSIZE, CB_ADDSTRING, 0, (LPARAM)tmp);
            }
            SendDlgItemMessage(hwndDlg, IDC_MAINDLG_BLKSIZE, CB_SETCURSEL, blksizesel, 0);
            MainDlgRefreshTarget(hwndDlg);
            CheckDlgButton(hwndDlg, IDC_MAINDLG_VERIFY, BST_CHECKED);
            return TRUE;

        case WM_CLOSE:
            EndDialog(hwndDlg, 0);
            return TRUE;

        case WM_COMMAND:
            switch (LOWORD(wParam)) {
                case IDC_MAINDLG_SELECT:
                    if(HIWORD(wParam) == BN_CLICKED)
                        return MainDlgSelectClick(hwndDlg);
                    else {
                        MainDlgRefreshTarget(hwndDlg);
                        return FALSE;
                    }

                case IDC_MAINDLG_TARGET_LIST:
                    MainDlgRefreshTarget(hwndDlg);
                    return TRUE;

                case IDC_MAINDLG_WRITE:
                    return (HIWORD(wParam) == BN_CLICKED) ? MainDlgWriteClick(hwndDlg) : FALSE;

                case IDC_MAINDLG_READ:
                    return (HIWORD(wParam) == BN_CLICKED) ? MainDlgReadClick(hwndDlg) : FALSE;

                case IDC_MAINDLG_VERIFY:
                    CheckDlgButton(hwndDlg, IDC_MAINDLG_VERIFY, IsDlgButtonChecked(hwndDlg, IDC_MAINDLG_VERIFY) ? BST_UNCHECKED : BST_CHECKED);
                    return TRUE;

                case IDC_MAINDLG_COMPRESS:
                    CheckDlgButton(hwndDlg, IDC_MAINDLG_COMPRESS, IsDlgButtonChecked(hwndDlg, IDC_MAINDLG_COMPRESS) ? BST_UNCHECKED : BST_CHECKED);
                    return TRUE;

                case IDC_MAINDLG_BLKSIZE:
                    index = SendDlgItemMessage(hwndDlg, IDC_MAINDLG_BLKSIZE, CB_GETCURSEL, 0, 0);
                    buffer_size = (1ULL<<index) * 1024ULL * 1024ULL;
                    return TRUE;

                default:
                    return FALSE;
            }

        case WM_DEVICECHANGE:
            if (DBT_DEVICEARRIVAL == wParam || DBT_DEVICEREMOVECOMPLETE == wParam)
                MainDlgRefreshTarget(hwndDlg);
            return TRUE;

        default:
            return FALSE;
    }
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpszArgument, int nCmdShow) {
    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpszArgument);
    UNREFERENCED_PARAMETER(nCmdShow);
    int lid = 0;
    int i, j, ret;
    unsigned int c;
    char *s, *e;
    wchar_t *d;

    char *cmdline = GetCommandLineA(), *loc = NULL;
    if(cmdline) {
        s = cmdline;
        if(*s == '\"') for(s++; *s && *s != '\"'; s++);
        for(; *s && *s != ' '; s++);
        for(; *s; s++) {
            while(*s == ' ') s++;
            if(*s == '-') {
                for(s++; *s && *s != ' '; s++) {
                    switch(*s) {
                        case 'v':
                            verbose++;
                            if(verbose == 1) {
                                AllocConsole();
                                HANDLE ConsoleHandle = GetStdHandle(STD_OUTPUT_HANDLE);
                                FILE *f = _fdopen(_open_osfhandle((intptr_t)ConsoleHandle, _O_TEXT), "w");
                                *stdout = *f;
                                setvbuf(stdout, NULL, _IONBF, 0);
                                SetConsoleTitle(TEXT("USBImager Debug"));
                                CONSOLE_SCREEN_BUFFER_INFO csbi;
                                if(GetConsoleScreenBufferInfo(ConsoleHandle, &csbi)) {
                                    COORD bs;
                                    bs.X = 132; bs.Y = 32767;
                                    SetConsoleScreenBufferSize(ConsoleHandle, bs);
                                }
                                printf("USBImager " USBIMAGER_VERSION
#ifdef USBIMAGER_BUILD
                                    " (build " USBIMAGER_BUILD ")"
#endif
                                    " - MIT license, Copyright (C) 2020 bzt\r\n\r\n"
                                    "usbimager.exe [-v|-vv|-s[baud]|-S[baud]|-1|-2|-3|-4|-5|-6|-7|-8|-9|-L(xx)] <backup path>\r\n\r\n"
                                    "https://gitlab.com/bztsrc/usbimager\r\n\r\n");
                            }
                        break;
                        case 's':
                            disks_serial = 1;
                            if(s[1] >= '0' && s[1] <= '9') {
                                stream_baud(atoi(s + 1));
                                while(s[1] >= '0' && s[1] <= '9') s++;
                            }
                            break;
                        case 'S':
                            disks_serial = 2;
                            if(s[1] >= '0' && s[1] <= '9') {
                                stream_baud(atoi(s + 1));
                                while(s[1] >= '0' && s[1] <= '9') s++;
                            }
                            break;
                        case '1': blksizesel = 1; buffer_size = 2*1024*1024; break;
                        case '2': blksizesel = 2; buffer_size = 4*1024*1024; break;
                        case '3': blksizesel = 3; buffer_size = 8*1024*1024; break;
                        case '4': blksizesel = 4; buffer_size = 16*1024*1024; break;
                        case '5': blksizesel = 5; buffer_size = 32*1024*1024; break;
                        case '6': blksizesel = 6; buffer_size = 64*1024*1024; break;
                        case '7': blksizesel = 7; buffer_size = 128*1024*1024; break;
                        case '8': blksizesel = 8; buffer_size = 256*1024*1024; break;
                        case '9': blksizesel = 9; buffer_size = 512*1024*1024; break;
                        case 'L': loc = ++s; ++s; break;
                    }
                }
            } else {
                if(bkpdir) break;
                if(*s == '\"') s++;
                for(e = s; *e && *e != ' ' && *e != '\"'; e++);
                bkpdir = malloc(2 * ((unsigned long)e - (unsigned long)s + 1));
                if(bkpdir) {
                    for(i = 0; s < e; s++, i++)
                        bkpdir[i] = (wchar_t)s[0];
                    bkpdir[i] = 0;
                }
            }
        }
    }
    if(!loc) {
        lid = GetUserDefaultLangID(); /* GetUserDefaultUILanguage(); */
        /* see https://docs.microsoft.com/en-us/windows/win32/intl/language-identifier-constants-and-strings */
        switch(lid & 0xFF) {
            case 0x01: loc = "ar"; break;   case 0x02: loc = "bg"; break;
            case 0x03: loc = "ca"; break;   case 0x04: loc = "zh"; break;
            case 0x05: loc = "cs"; break;   case 0x06: loc = "da"; break;
            case 0x07: loc = "de"; break;   case 0x08: loc = "el"; break;
            case 0x0A: loc = "es"; break;   case 0x0B: loc = "fi"; break;
            case 0x0C: loc = "fr"; break;   case 0x0D: loc = "he"; break;
            case 0x0E: loc = "hu"; break;   case 0x0F: loc = "is"; break;
            case 0x10: loc = "it"; break;   case 0x11: loc = "jp"; break;
            case 0x12: loc = "ko"; break;   case 0x13: loc = "nl"; break;
            case 0x14: loc = "no"; break;   case 0x15: loc = "pl"; break;
            case 0x16: loc = "pt"; break;   case 0x17: loc = "rm"; break;
            case 0x18: loc = "ro"; break;   case 0x19: loc = "ru"; break;
            case 0x1A: loc = "hr"; break;   case 0x1B: loc = "sk"; break;
            case 0x1C: loc = "sq"; break;   case 0x1D: loc = "sv"; break;
            case 0x1E: loc = "th"; break;   case 0x1F: loc = "tr"; break;
            case 0x20: loc = "ur"; break;   case 0x21: loc = "id"; break;
            case 0x22: loc = "uk"; break;   case 0x23: loc = "be"; break;
            case 0x24: loc = "sl"; break;   case 0x25: loc = "et"; break;
            case 0x26: loc = "lv"; break;   case 0x27: loc = "lt"; break;
            case 0x29: loc = "fa"; break;   case 0x2A: loc = "vi"; break;
            case 0x2B: loc = "hy"; break;   case 0x2D: loc = "bq"; break;
            case 0x2F: loc = "mk"; break;   case 0x36: loc = "af"; break;
            case 0x37: loc = "ka"; break;   case 0x38: loc = "fo"; break;
            case 0x39: loc = "hi"; break;   case 0x3A: loc = "mt"; break;
            case 0x3C: loc = "gd"; break;   case 0x3E: loc = "ms"; break;
            case 0x3F: loc = "kk"; break;   case 0x40: loc = "ky"; break;
            case 0x45: loc = "bn"; break;   case 0x47: loc = "gu"; break;
            case 0x4D: loc = "as"; break;   case 0x4E: loc = "mr"; break;
            case 0x4F: loc = "sa"; break;   case 0x53: loc = "kh"; break;
            case 0x54: loc = "lo"; break;   case 0x56: loc = "gl"; break;
            case 0x5E: loc = "am"; break;   case 0x62: loc = "fy"; break;
            case 0x68: loc = "ha"; break;   case 0x6D: loc = "ba"; break;
            case 0x6E: loc = "lb"; break;   case 0x6F: loc = "kl"; break;
            case 0x7E: loc = "br"; break;   case 0x92: loc = "ku"; break;
            case 0x09: default: loc = "en"; break;
        }
    }
    for(i = 0; i < NUMLANGS; i++)
        if(!strncmp(loc, dict[i][0], strlen(dict[i][0]))) break;
    if(i >= NUMLANGS) { i = 0; loc = "en"; }

    lang=(wchar_t**)malloc(NUMTEXTS * sizeof(wchar_t*));
    if(!lang) return 1;
    for(j = 0; j < NUMTEXTS; j++) {
        lang[j] = (wchar_t*)malloc((strlen(dict[i][j+1])+1)*sizeof(wchar_t));
        if(!lang[j]) return 1;
        for(s = dict[i][j+1], d = lang[j]; *s; d++) {
            if((*s & 128) != 0) {
                if(!(*s & 32)) { c = ((*s & 0x1F)<<6)|(*(s+1) & 0x3F); s++; } else
                if(!(*s & 16)) { c = ((*s & 0xF)<<12)|((*(s+1) & 0x3F)<<6)|(*(s+2) & 0x3F); s += 2; } else
                if(!(*s & 8)) { c = ((*s & 0x7)<<18)|((*(s+1) & 0x3F)<<12)|((*(s+2) & 0x3F)<<6)|(*(s+3) & 0x3F); *s += 3; }
                else c = 0;
            } else c = *s;
            s++;
            *d = (wchar_t)c;
        }
        *d = 0;
    }
    if(verbose) {
        printf("GetUserDefaultLangID %04x '%s', dict '%s', serial %d, buffer_size %d MiB\r\n",
            lid, loc, dict[i][0], disks_serial, buffer_size/1024/1024);
        if(disks_serial) printf("Serial %d,8,n,1\r\n", baud);
        if(bkpdir) wprintf(L"bkpdir '%s'\r\n", bkpdir);
    }

    ret = DialogBoxParam(hInstance, MAKEINTRESOURCE(IDC_MAINDLG), NULL, MainDlgProc, (LPARAM) hInstance);

    for(j = 0; j < NUMTEXTS; j++)
        free(lang[j]);
    free(lang);
    if(bkpdir) free(bkpdir);
    return ret;
}
