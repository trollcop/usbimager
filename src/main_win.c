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
#include <io.h>
#include <fcntl.h>
#include "lang.h"
#include "resource.h"
#include "input.h"
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

static HWND mainHwndDlg;

wchar_t *main_errorMessage;

void main_addToCombobox(char *option)
{
/* this is tricky under Windows:
 *  char - always 1 byte. Because this is a multiplatform function, the prototype has this
 *  TCHAR - either 1 byte or 2 bytes, this is what we actually got in "option"
 *  wchar_t - always 2 bytes */
    TCHAR *topt = (TCHAR*)option;
    wchar_t msg[128];
    int i = 0;
    for(; *topt; i++, topt++)
        msg[i] = (wchar_t)*topt;
    SendDlgItemMessageW(mainHwndDlg, IDC_MAINDLG_TARGET_LIST, CB_ADDSTRING, 0, (LPARAM)&msg);
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

        hTargetDevice = index == CB_ERR ? (HANDLE)-1 : (HANDLE)disks_open((int)index);
        if (hTargetDevice != NULL && hTargetDevice != (HANDLE)-1 && hTargetDevice != (HANDLE)-2 && hTargetDevice != (HANDLE)-3) {
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
                                    MessageBoxW(hwndDlg, lang[L_VRFYERR], lang[L_ERROR], MB_ICONERROR);
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
                L_OPENTRGERR))]);
        }
        input_close(&ctx);
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

    EnableWindow(GetDlgItem(hwndDlg, IDC_MAINDLG_SOURCE), TRUE);
    EnableWindow(GetDlgItem(hwndDlg, IDC_MAINDLG_SELECT), TRUE);
    EnableWindow(GetDlgItem(hwndDlg, IDC_MAINDLG_TARGET_LIST), TRUE);
    EnableWindow(GetDlgItem(hwndDlg, IDC_MAINDLG_VERIFY), TRUE);
    EnableWindow(GetDlgItem(hwndDlg, IDC_MAINDLG_BUTTON), TRUE);
    SendDlgItemMessage(hwndDlg, IDC_MAINDLG_PROGRESSBAR, PBM_SETPOS, 0, 0);
    SetWindowTextW(GetDlgItem(hwndDlg, IDC_MAINDLG_STATUS),
        ctx.fileSize && ctx.readSize == ctx.fileSize ? lang[L_DONE] : L"");
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
    if(main_errorMessage) {
        LocalFree(main_errorMessage);
        main_errorMessage = NULL;
    }

    CloseHandle(CreateThread(NULL, 0, writerRoutine, hwndDlg, 0, NULL));
    return TRUE;
}

INT_PTR MainDlgRefreshTarget(HWND hwndDlg) {
    LRESULT index = SendDlgItemMessage(hwndDlg, IDC_MAINDLG_TARGET_LIST, CB_GETCURSEL, 0, 0);
    SendDlgItemMessage(hwndDlg, IDC_MAINDLG_TARGET_LIST, CB_RESETCONTENT, 0, 0);
    disks_refreshlist();
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
    UNREFERENCED_PARAMETER(lParam);
    switch (uMsg) {
        case WM_INITDIALOG:
            mainHwndDlg = hwndDlg;
            SendMessage(hwndDlg, WM_SETICON, ICON_SMALL, (LPARAM) LoadIcon((HINSTANCE) lParam, MAKEINTRESOURCE(IDI_APP_ICON)));
            SetDlgItemTextW(hwndDlg, IDC_MAINDLG_BUTTON, lang[L_WRITE]);
            SetWindowTextW(GetDlgItem(hwndDlg, IDC_MAINDLG_VERIFY), lang[L_VERIFY]);
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
    int i, j, ret;
    unsigned int c;
    char *s;
    wchar_t *d;

    char *cmdline = GetCommandLine();
    for(; cmdline && cmdline[0] && cmdline[1] && cmdline[2] && !verbose; cmdline++)
        if(cmdline[0] == ' ' && cmdline[1] == '-' && cmdline[2] == 'v') {
            verbose = 1;
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
        }

    char *loc;
    int lid = GetUserDefaultLangID();
    /* see https://docs.microsoft.com/en-us/windows/win32/intl/language-identifier-constants-and-strings */
    switch(lid & 0xFF) {
        case 0x04: loc = "ch"; break;
        case 0x07: loc = "de"; break;
        case 0x08: loc = "el"; break;
        case 0x0A: loc = "es"; break;
        case 0x0C: loc = "fr"; break;
        case 0x0D: loc = "he"; break;
        case 0x0E: loc = "hu"; break;
        case 0x10: loc = "it"; break;
        case 0x11: loc = "jp"; break;
        case 0x15: loc = "pl"; break;
        case 0x16: loc = "pt"; break;
        case 0x19: loc = "ru"; break;
        case 0x39: loc = "hu"; break;
        case 0x09: default: loc = "en"; break;
    }
    if(verbose) printf("GetUserDefaultLangID %04x, locale %s\r\n", lid, loc);
    lang=(wchar_t**)malloc(NUMTEXTS * sizeof(wchar_t*));
    if(!lang) return 1;
    for(i = 0; i < NUMLANGS; i++)
        if(!strcmp(loc, dict[i][0])) {
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
            break;
        }
    ret = DialogBoxParam(hInstance, MAKEINTRESOURCE(IDC_MAINDLG), NULL, MainDlgProc, (LPARAM) hInstance);

    for(j = 0; j < NUMTEXTS; j++)
        free(lang[j]);
    free(lang);
    return ret;
}
