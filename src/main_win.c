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
#include <winioctl.h>
#include <commctrl.h>
#include "resource.h"
#include "input.h"
#include "disks.h"

#ifndef DBT_DEVICEARRIVAL
#define DBT_DEVICEARRIVAL 0x8000
#endif
#ifndef DBT_DEVICEREMOVECOMPLETE
#define DBT_DEVICEREMOVECOMPLETE 0x8004
#endif

static DWORD WINAPI writerRoutine(LPVOID lpParam) {
    HWND hwndDlg = (HWND) lpParam;
    TCHAR szFilePathName[MAX_PATH];
    LARGE_INTEGER totalNumberOfBytesWritten;
    DWORD pos = 0;
    HANDLE hTargetDevice;
    unsigned short int wFn[MAX_PATH];
    input_t ctx;
    int ret = 1, len, wlen;
    char *fn;

    GetDlgItemText(hwndDlg, IDC_MAINDLG_SOURCE, szFilePathName, sizeof(szFilePathName) / sizeof(szFilePathName[0]));
    for(wlen = 0; szFilePathName[wlen]; wlen++) wFn[wlen] = szFilePathName[wlen];
    len = WideCharToMultiByte(CP_UTF8, 0, wFn, wlen, 0, 0, NULL, NULL);
    if(len > 0) {
        fn = (char*)malloc(len+1);
        if(fn) {
            WideCharToMultiByte(CP_UTF8, 0, wFn, wlen, fn, len, NULL, NULL);
            fn[len] = 0;
            ret = input_open(&ctx, fn);
            free(fn);
        }
    }

    if (!ret) {
        LRESULT index = SendDlgItemMessage(hwndDlg, IDC_MAINDLG_TARGET_LIST, CB_GETCURSEL, 0, 0);
        int needVerify = IsDlgButtonChecked(hwndDlg, IDC_MAINDLG_VERIFY);

        if(index == CB_ERR) {
            MessageBox(hwndDlg, TEXT("Please select a target."), TEXT("Error"), MB_ICONEXCLAMATION);
        } else
        if ((hTargetDevice = (HANDLE)disks_open((int)index)) != NULL) {
            totalNumberOfBytesWritten.QuadPart = 0;

            while(1) {
                static CHAR lpBuffer[BUFFER_SIZE], lpVerifyBuf[BUFFER_SIZE];
                int numberOfBytesRead;

                if((numberOfBytesRead = input_read(&ctx, lpBuffer)) >= 0) {
                    if(numberOfBytesRead == 0) {
                        if(!ctx.fileSize) ctx.fileSize = ctx.readSize;
                        break;
                    } else {
                        DWORD numberOfBytesWritten, numberOfBytesVerify;

                        if (WriteFile(hTargetDevice, lpBuffer, numberOfBytesRead, &numberOfBytesWritten, NULL)) {
                            static CHAR lpStatus[128];

                            if(needVerify) {
                                SetFilePointerEx(hTargetDevice, totalNumberOfBytesWritten, NULL, FILE_BEGIN);
                                if(!ReadFile(hTargetDevice, lpVerifyBuf, numberOfBytesWritten, &numberOfBytesVerify, NULL) ||
                                    numberOfBytesWritten != numberOfBytesVerify || memcmp(lpBuffer, lpVerifyBuf, numberOfBytesWritten)) {
                                    MessageBox(hwndDlg, TEXT("Write verification failed."), TEXT("Error"), MB_ICONERROR);
                                    break;
                                }
                                if(verbose) printf("  numberOfBytesVerify %lu\n", numberOfBytesVerify);
                                totalNumberOfBytesWritten.QuadPart += numberOfBytesWritten;
                            }

                            pos = (DWORD) input_status(&ctx, lpStatus);
                            SendDlgItemMessage(hwndDlg, IDC_MAINDLG_PROGRESSBAR, PBM_SETPOS, pos, 0);
                            SetWindowText(GetDlgItem(hwndDlg, IDC_MAINDLG_STATUS), lpStatus);
                            ShowWindow(GetDlgItem(hwndDlg, IDC_MAINDLG_STATUS), SW_HIDE);
                            ShowWindow(GetDlgItem(hwndDlg, IDC_MAINDLG_STATUS), SW_SHOW);
                        } else {
                            MessageBox(hwndDlg, TEXT("An error occurred while writing to the target device."), TEXT("Error"), MB_ICONERROR);
                            break;
                        }
                    }
                } else {
                    MessageBox(hwndDlg, TEXT("An error occurred while reading the source file."), TEXT("Error"), MB_ICONERROR);
                    break;
                }
            }
            disks_close((void*)hTargetDevice);
        } else {
            MessageBox(hwndDlg, TEXT("An error occurred while opening the target volume."), TEXT("Error"), MB_ICONERROR);
        }
        input_close(&ctx);
    } else {
        MessageBox(hwndDlg, ret == 2 ? TEXT("Encrypted ZIP not supported") :
            (ret == 3 ? TEXT("Unsupported compression method in ZIP") :
            (ret == 4 ? TEXT("Decompressor error") :
            TEXT("Please select a readable source file."))), TEXT("Error"), MB_ICONEXCLAMATION);
    }

    EnableWindow(GetDlgItem(hwndDlg, IDC_MAINDLG_SOURCE), TRUE);
    EnableWindow(GetDlgItem(hwndDlg, IDC_MAINDLG_SELECT), TRUE);
    EnableWindow(GetDlgItem(hwndDlg, IDC_MAINDLG_TARGET_LIST), TRUE);
    EnableWindow(GetDlgItem(hwndDlg, IDC_MAINDLG_VERIFY), TRUE);
    EnableWindow(GetDlgItem(hwndDlg, IDC_MAINDLG_BUTTON), TRUE);
    SendDlgItemMessage(hwndDlg, IDC_MAINDLG_PROGRESSBAR, PBM_SETPOS, 0, 0);
    SetWindowText(GetDlgItem(hwndDlg, IDC_MAINDLG_STATUS),
        ctx.fileSize && ctx.readSize == ctx.fileSize ? "Done. Image written successfully." : "");
    ShowWindow(GetDlgItem(hwndDlg, IDC_MAINDLG_STATUS), SW_HIDE);
    ShowWindow(GetDlgItem(hwndDlg, IDC_MAINDLG_STATUS), SW_SHOW);
    return 0;
}

INT_PTR MainDlgButtonClick(HWND hwndDlg) {
    EnableWindow(GetDlgItem(hwndDlg, IDC_MAINDLG_SOURCE), FALSE);
    EnableWindow(GetDlgItem(hwndDlg, IDC_MAINDLG_SELECT), FALSE);
    EnableWindow(GetDlgItem(hwndDlg, IDC_MAINDLG_TARGET_LIST), FALSE);
    EnableWindow(GetDlgItem(hwndDlg, IDC_MAINDLG_VERIFY), FALSE);
    EnableWindow(GetDlgItem(hwndDlg, IDC_MAINDLG_BUTTON), FALSE);
    SendDlgItemMessage(hwndDlg, IDC_MAINDLG_PROGRESSBAR, PBM_SETPOS, 0, 0);

    CloseHandle(CreateThread(NULL, 0, writerRoutine, hwndDlg, 0, NULL));
    return TRUE;
}

INT_PTR MainDlgRefreshTarget(HWND hwndDlg) {
    LRESULT index = SendDlgItemMessage(hwndDlg, IDC_MAINDLG_TARGET_LIST, CB_GETCURSEL, 0, 0);
    SendDlgItemMessage(hwndDlg, IDC_MAINDLG_TARGET_LIST, CB_RESETCONTENT, 0, 0);
    disks_refreshlist(hwndDlg);
    if(index != CB_ERR)
        SendDlgItemMessage(hwndDlg, IDC_MAINDLG_TARGET_LIST, CB_SETCURSEL, (WPARAM) index, 0);
    return TRUE;
}

INT_PTR MainDlgSelectClick(HWND hwndDlg) {
    OPENFILENAME ofn;
    TCHAR lpstrFile[MAX_PATH];

    MainDlgRefreshTarget(hwndDlg);

    SetWindowText(GetDlgItem(hwndDlg, IDC_MAINDLG_STATUS), "");
    ZeroMemory(&ofn, sizeof ofn);
    ZeroMemory(lpstrFile, sizeof lpstrFile);

    ofn.lStructSize = sizeof ofn;
    ofn.hwndOwner = hwndDlg;
    ofn.lpstrFilter = TEXT("All files (*.*)\0*.*\0\0");
    ofn.lpstrFile = lpstrFile;
    ofn.nMaxFile = MAX_PATH;
    ofn.Flags = OFN_FILEMUSTEXIST;

    if (GetOpenFileName(&ofn)) {
        SetDlgItemText(hwndDlg, IDC_MAINDLG_SOURCE, ofn.lpstrFile);
    }

    return TRUE;
}

static INT_PTR CALLBACK MainDlgProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    UNREFERENCED_PARAMETER(lParam);
    switch (uMsg) {
        case WM_INITDIALOG:
            SendMessage(hwndDlg, WM_SETICON, ICON_SMALL, (LPARAM) LoadIcon((HINSTANCE) lParam, MAKEINTRESOURCE(IDI_APP_ICON)));
            MainDlgRefreshTarget(hwndDlg);
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
        
                case IDC_MAINDLG_BUTTON:
                    return (HIWORD(wParam) == BN_CLICKED) ? MainDlgButtonClick(hwndDlg) : FALSE;
        
                case IDC_MAINDLG_VERIFY:
                    CheckDlgButton(hwndDlg, IDC_MAINDLG_VERIFY, IsDlgButtonChecked(hwndDlg, IDC_MAINDLG_VERIFY) ? BST_UNCHECKED : BST_CHECKED);
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

    char *cmdline = GetCommandLine();
    for(; cmdline && cmdline[0] && cmdline[1] && cmdline[2] && !verbose; cmdline++)
        if(cmdline[0] == ' ' && cmdline[1] == '-' && cmdline[2] == 'v')
            verbose = 1;

    return DialogBoxParam(hInstance, MAKEINTRESOURCE(IDC_MAINDLG), NULL, MainDlgProc, (LPARAM) hInstance);
}
