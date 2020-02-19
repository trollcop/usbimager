/*
 * usbimager/main_x11.c
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
 * @brief User interface based on X11 for Linux and MacOSX
 *
 */

#define LOADFONT    0   /* load our own font */
#define USEUTF8     1   /* do UTF-8 to UNICODE conversion */

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <errno.h>
#include <limits.h>
#include <stdlib.h>
#include <unistd.h>
#include <dirent.h>
#include <sys/stat.h>
#include "input.h"
#include "disks.h"
#include "misc/icons.xbm"

/* some defines and prototypes if not defined in limit.h or unistd.h */
#ifndef PATH_MAX
# ifdef MAXPATHLEN
#  define PATH_MAX MAXPATHLEN
# else
#  define PATH_MAX 65536
# endif
#endif
#ifndef FILENAME_MAX
# define FILENAME_MAX 256
#endif
int usleep(unsigned long int);

typedef struct {
    char *name;
    char type;
    uint64_t size;
    time_t time;
} filelist_t;

enum {
    color_winbg, color_inputbg, color_inpdrk, color_inplght,
    color_inpbrd0, color_inpbrd1, color_inpbrd2, color_inpbrd3,
    color_btnbg0, color_btnbg1, color_btnbg2, color_btnbg3,
    color_wbtnbg0, color_wbtnbg1, color_wbtnbg2, color_wbtnbg3,
    color_btnbrd0, color_btnbrd1, color_btnbrd2, color_fg,
    NUM_COLOR
};
static unsigned long int palette[NUM_COLOR] = {
    0xF2F2F2, 0xFFFFFF, 0x2065DE, 0xB4CDF8,
    0xE9ECF0, 0xABADB3, 0xE2E3EA, 0xE3E9EF,
    0xFCFCFC, 0xF0F0F0, 0xF6F6F6, 0xD6D6D6,
    0xEFC8C1, 0xE49E90, 0xDDA398, 0xD47E6A,
    0x919191, 0x777777, 0x707070, 0x303030
};

static Display* dpy;
static int scr;
static Window mainwin;
static XColor colors[NUM_COLOR];
static XFontStruct *font = NULL;
static GC txtgc, shdgc, statgc, gc;
static Atom delAtom;
static Pixmap icons_act, icons_ina;

static char source[PATH_MAX], targetList[DISKS_MAX][128], status[128];
static int fonth = 0, fonta = 0, inactive = 0, pressedBtn = 0, half;
static int needVerify = 0, progress = 0, numTargetList = 0, targetId = -1;
static int sorting = 0;

char *main_errorMessage = NULL;

static int fncmp(const void *a, const void *b)
{
    filelist_t *A = (filelist_t*)a, *B = (filelist_t*)b;
    if(A->type && !B->type) return 1;
    if(!A->type && B->type) return -1;
    switch(sorting) {
        case 0: return strcmp(A->name, B->name);
        case 1: return strcmp(B->name, A->name);
        case 2: return (int)(A->size - B->size);
        case 3: return (int)(B->size - A->size);
        case 4: return (int)(A->time - B->time);
        case 5: return (int)(B->time - A->time);
    }
    return 0;
}

/* because X11 is a low level protocol, we need some widgets */

static int mainPrint(Window win, GC gc, int x, int y, int w, int style, char *s)
{
    XRectangle clip = { x, y, w - (style & 3 ? 0 : 8), fonth };
    int l, tw;
#if USEUTF8 == 1
    unsigned int c;
    XChar2b *str = NULL;
    int i;
#endif

    if(!s || !*s) return 0;

    l = strlen(s);
#if USEUTF8 == 1
    str = (XChar2b*)malloc(l * sizeof(XChar2b));
    if(!str) return 0;
    for(i = 0; i < l && *s; i++) {
        if((*s & 128) != 0) {
            if(!(*s & 32)) { c = ((*s & 0x1F)<<6)|(*(s+1) & 0x3F); s++; } else
            if(!(*s & 16)) { c = ((*s & 0xF)<<12)|((*(s+1) & 0x3F)<<6)|(*(s+2) & 0x3F); s += 2; } else
            if(!(*s & 8)) { c = ((*s & 0x7)<<18)|((*(s+1) & 0x3F)<<12)|((*(s+2) & 0x3F)<<6)|(*(s+3) & 0x3F); *s += 3; }
            else c = 0;
        } else c = *s;
        s++;
        str[i].byte2 = c & 0xFF;
        str[i].byte1 = c >> 8;
    }
    tw = XTextWidth16(font, str, i);
#else
    tw = XTextWidth(font, s, l);
#endif
    if(w < 1) return tw;

    switch(style & 3) {
        case 1: x += (w - tw) / 2 - 1; break;
        case 2:
            if(tw > w) {
                XDrawPoint(dpy, win, gc, x, y +fonta);
                XDrawPoint(dpy, win, gc, x+2, y +fonta);
                XDrawPoint(dpy, win, gc, x+4, y +fonta);
                clip.x += 8; clip.width -= 8;
                x -= tw - w;
            }
        break;
        case 3: x -= tw - w; break;
    }
    if((style & 4) && !inactive) {
        XSetClipRectangles(dpy, shdgc, 0, 0, &clip, 1, Unsorted);
#if USEUTF8 == 1
        XDrawString16(dpy, win, shdgc, x+1, y+fonta+1, str, i);
#else
        XDrawString(dpy, win, shdgc, x+1, y+fonta+1, s, l);
#endif
        XSetClipMask(dpy, shdgc, None);
    }
    XSetClipRectangles(dpy, gc, 0, 0, &clip, 1, Unsorted);
#if USEUTF8 == 1
    XDrawString16(dpy, win, gc, x, y+fonta, str, i);
    free(str);
#else
    XDrawString(dpy, win, gc, x, y+fonta, s, l);
#endif
    XSetClipMask(dpy, gc, None);
    return tw;
}

static void mainInputBox(Window win, int x, int y, int w, char *txt)
{
    XPoint points[8] = {
        { x, y }, { x + 1, y + 1 }, { x + w, y }, { x + w -1, y + 1 },
        { x, y + fonth + 8 }, { x + 1, y + fonth + 7 },
        { x + w, y + fonth + 8 }, { x + w - 1, y + fonth + 7 }};
    if(w < 1) return;
    XSetForeground(dpy, gc, colors[color_inpbrd1].pixel);
    XDrawLine(dpy, win, gc, x+1, y, x+w-1, y);
    XSetForeground(dpy, gc, colors[color_inpbrd2].pixel);
    XDrawLine(dpy, win, gc, x, y+1, x, y+fonth+7);
    XDrawLine(dpy, win, gc, x+w, y+1, x+w, y+fonth+7);
    XSetForeground(dpy, gc, colors[color_inpbrd3].pixel);
    XDrawLine(dpy, win, gc, x+1, y+fonth+8, x+w-1, y+fonth+8);
    XSetForeground(dpy, gc, colors[inactive ? color_winbg : color_inputbg].pixel);
    XFillRectangle(dpy, win, gc, x+1, y+1, w-1, fonth+7);
    XSetForeground(dpy, gc, colors[color_inpbrd0].pixel);
    XDrawPoints(dpy, win, gc, points, 8, CoordModeOrigin);
    mainPrint(win, txtgc, x+4, y+4, w-8, 2, txt);
}

static void mainButton(Window win, int x, int y, int w, int pressed, int style, char *txt)
{
    int bg0 = color_btnbg0, bg1 = color_btnbg1, bg2 = color_btnbg2, bg3 = color_btnbg3;
    XSegment border1[4] = {
        { x+3, y, x+w-3, y }, { x, y+3, x, y+fonth+5 },
        { x+w, y+3, x+w, y+fonth+5 }, { x+3, y+fonth+8, x+w-3, y+fonth+8 }
    }, border2[3] = {
        { x+3, y+1, x+w-3, y+1 }, { x+1, y+3, x+1, y+fonta },
        { x+w-1, y+3, x+w-1, y+fonta }
    }, border3[3] = {
        { x+1, y+fonta+1, x+1, y+fonth+5 }, { x+w-1, y+fonta+1, x+w-1, y+fonth+5 },
        { x+3, y+fonth+7, x+w-3, y+fonth+7 }
    };
    XPoint border4[8] = {
        { x+2, y}, { x+w-2, y }, { x, y+2 }, { x+w, y+2 },
        { x, y+fonth+6 }, { x+w, y+fonth+6}, { x+2, y+fonth+8 }, { x+w-2, y+fonth+8 }
    }, border5[12] = {
        { x+1, y }, { x+w-1, y }, { x, y+1 }, { x+1, y+1 }, { x+w-2, y+1 },
        { x+w-1, y+1 }, { x, y+fonth+7 }, { x+1, y+fonth+7 }, { x+w-1, y+fonth+7 },
        { x+w, y+fonth+7 }, { x+1, y+fonth+8 }, { x+w-1, y+fonth+8 }
    }, border6[4] = {
        { x+2, y+1 }, { x+w-2, y+1 }, { x+1, y+2 }, { x+w-1, y+2 }
    }, border7[4] = {
        { x+1, y+fonth+6 }, { x+2, y+fonth+7 }, { x+w-1, y+fonth+6}, { x+w-2, y+fonth+7 }
    };
    if(w < 8) return;
    switch(pressed) {
        case 1:
            bg0 = color_btnbg2; bg1 = color_btnbg3; bg2 = color_btnbg0; bg3 = color_btnbg1;
        break;
        case 2:
            bg0 = color_wbtnbg0; bg1 = color_wbtnbg1; bg2 = color_wbtnbg2; bg3 = color_wbtnbg3;
        break;
        case 3:
            bg0 = color_wbtnbg2; bg1 = color_wbtnbg3; bg2 = color_wbtnbg0; bg3 = color_wbtnbg1;
        break;
    }
    XSetForeground(dpy, gc, colors[bg1].pixel);
    XFillRectangle(dpy, win, gc, x+2, y+2, w-2, fonta);
    XDrawPoints(dpy, win, gc, border6, 4, CoordModeOrigin);
    XSetForeground(dpy, gc, colors[bg0].pixel);
    XDrawSegments(dpy, win, gc, border2, 3);
    XDrawPoint(dpy, win, gc, x+2, y+2);
    XDrawPoint(dpy, win, gc, x+w-2, y+2);

    XSetForeground(dpy, gc, colors[inactive ? bg1 : bg3].pixel);
    XFillRectangle(dpy, win, gc, x+2, y+fonta+1, w-2, fonth+8-fonta-1);
    XDrawPoints(dpy, win, gc, border7, 4, CoordModeOrigin);
    XSetForeground(dpy, gc, colors[bg2].pixel);
    XDrawSegments(dpy, win, gc, border3, 3);
    XDrawPoint(dpy, win, gc, x+2, y+fonth+6);
    XDrawPoint(dpy, win, gc, x+w-2, y+fonth+6);

    mainPrint(win, txtgc, x+7, y+5, w-10, style, txt);

    XSetForeground(dpy, gc, colors[color_btnbrd2].pixel);
    XDrawSegments(dpy, win, gc, border1, 4);
    XSetForeground(dpy, gc, colors[color_btnbrd1].pixel);
    XDrawPoints(dpy, win, gc, border4, 8, CoordModeOrigin);
    XSetForeground(dpy, gc, colors[color_btnbrd0].pixel);
    XDrawPoints(dpy, win, gc, border5, 12, CoordModeOrigin);

}

static void mainBox(Window win, int x, int y, int w, int pressed, char *txt)
{
    XSetForeground(dpy, gc, colors[pressed ? color_btnbg1 : color_btnbg2].pixel);
    XFillRectangle(dpy, win, gc, x+1, y+1, w-2, fonth/2+3);
    XSetForeground(dpy, gc, colors[pressed ? color_btnbg2 : color_btnbg0].pixel);
    XFillRectangle(dpy, win, gc, x+1, y+4+fonth/2, w-2, fonth/2+3);
    mainPrint(win, txtgc, x+6, y+2, w-4, 0, txt);
    XSetForeground(dpy, gc, colors[pressed ? color_btnbrd0 : color_btnbg0].pixel);
    XDrawLine(dpy, win, gc, x, y, x+w, y);
    XDrawLine(dpy, win, gc, x, y+1, x, y+4+fonth);
    XSetForeground(dpy, gc, colors[pressed ? color_btnbg0 : color_btnbrd0].pixel);
    XDrawLine(dpy, win, gc, x+w, y+1, x+w, y+3+fonth);
    XDrawLine(dpy, win, gc, x, y+4+fonth, x+w, y+4+fonth);
}

static void mainCheckbox(Window win, int x, int y, int checked)
{
    XSetForeground(dpy, gc, colors[color_btnbrd2].pixel);
    XDrawRectangle(dpy, win, gc, x, y, fonth, fonth);

    XSetForeground(dpy, gc, colors[color_inpbrd1].pixel);
    XDrawLine(dpy, win, gc, x+2, y+2, x+fonth-2, y+2);
    XSetForeground(dpy, gc, colors[color_inpbrd2].pixel);
    XDrawLine(dpy, win, gc, x+2, y+3, x+2, y+fonth-2);
    XDrawLine(dpy, win, gc, x+fonth-2, y+2, x+fonth-2, y+fonth-3);
    XSetForeground(dpy, gc, colors[color_inpbrd3].pixel);
    XDrawLine(dpy, win, gc, x+3, y+fonth-2, x+fonth-2, y+fonth-2);
    XSetForeground(dpy, gc, colors[inactive ? color_winbg : color_inputbg].pixel);
    XFillRectangle(dpy, win, gc, x+3, y+3, fonth-5, fonth-5);

    if(checked) {
        XSetForeground(dpy, gc, colors[inactive ? color_btnbg3 : color_inplght].pixel);
        XDrawLine(dpy, win, gc, x+2, y+fonth/2, x+fonth/2-1, y+fonth-3);
        XDrawLine(dpy, win, gc, x+5, y+fonth/2, x+fonth/2+2, y+fonth-3);
        XDrawLine(dpy, win, gc, x+fonth/2, y+fonth-3, x+fonth-4, y+3);
        XDrawLine(dpy, win, gc, x+fonth/2+1, y+fonth-3, x+fonth-2, y+3);
        XSetForeground(dpy, gc, colors[inactive ? color_btnbrd1 : color_inpdrk].pixel);
        XDrawLine(dpy, win, gc, x+3, y+fonth/2, x+fonth/2, y+fonth-3);
        XDrawLine(dpy, win, gc, x+4, y+fonth/2, x+fonth/2+1, y+fonth-3);
        XDrawLine(dpy, win, gc, x+fonth/2+1, y+fonth-3, x+fonth-3, y+3);
    }
}

static void mainProgress(Window win, int x, int y, int w, int percentage)
{
    int s;
    XSegment border[4] = {
        { x+1, y, x+w-1, y }, { x, y+1, x, y+4 },
        { x+w, y+1, x+w, y+4 }, { x+1, y+5, x+w-1, y+5 }
    };
    if(w < 2) return;
    s = (w - 2) * percentage / 100;
    XSetForeground(dpy, gc, colors[color_btnbrd2].pixel);
    XDrawSegments(dpy, win, gc, border, 4);
    XSetForeground(dpy, gc, colors[inactive ? color_btnbg1 : color_btnbg3].pixel);
    XDrawLine(dpy, win, gc, x+1+s, y+1, x+w-1, y+1);
    if(inactive) XSetForeground(dpy, gc, colors[color_btnbg0].pixel);
    XDrawLine(dpy, win, gc, x+1+s, y+2, x+w-1, y+2);
    if(inactive) XSetForeground(dpy, gc, colors[color_inputbg].pixel);
    XDrawLine(dpy, win, gc, x+1+s, y+3, x+w-1, y+3);
    XDrawLine(dpy, win, gc, x+1+s, y+4, x+w-1, y+4);
    if(s) {
        XSetForeground(dpy, gc, colors[color_inplght].pixel);
        XDrawLine(dpy, win, gc, x+1, y+1, x+1+s, y+1);
        XDrawLine(dpy, win, gc, x+1, y+1, x+1, y+4);
        XSetForeground(dpy, gc, colors[color_inpdrk].pixel);
        XDrawLine(dpy, win, gc, x+2, y+2, x+1+s, y+2);
        XDrawLine(dpy, win, gc, x+2, y+3, x+1+s, y+3);
        XDrawLine(dpy, win, gc, x+2, y+4, x+1+s, y+4);
    }
}

static Window mainModal(int *mw, int *mh, int bgcolor)
{
    XWindowAttributes  wa;
    XSetWindowAttributes swa;
    Window win, child, root = RootWindow(dpy, scr);
    int x, y, nx, ny, i, sw, sh, ws, hs;

    XTranslateCoordinates(dpy, mainwin, root, 0, 0, &x, &y, &child);
    XGetWindowAttributes(dpy, mainwin, &wa);
    sw = XWidthOfScreen(wa.screen); if(*mw + 60 > sw) *mw = sw - 60;
    sh = XHeightOfScreen(wa.screen); if(*mh + 60 > sh) *mh = sh - 60;
    ws = *mw / 20; hs = *mh / 10;
    x -= wa.x; y -= wa.y;
    nx = x + wa.border_width + wa.width/2; ny = y + 20;
    win = XCreateSimpleWindow(dpy, RootWindow(dpy, scr), nx, ny, 1, 1,
        0, 0, colors[bgcolor].pixel);
    XSelectInput(dpy, win, ExposureMask | ButtonPressMask | ButtonReleaseMask | PointerMotionMask |
        KeyPressMask | KeyReleaseMask);
    swa.override_redirect = True;
    XChangeWindowAttributes(dpy, win, CWOverrideRedirect, &swa);
    XMapWindow(dpy, win);
    XRaiseWindow(dpy, win);
    for(i = 1; i < 11; i++) {
        if(nx < i*ws) { x += i*ws-nx; nx = i*ws; XMoveWindow(dpy, mainwin, x, y); }
        if(nx + i*ws > sw) { nx = sw-i*ws; x = nx - wa.width/2; XMoveWindow(dpy, mainwin, x, y); }
        if(ny + i*hs > sh) { ny = sh-i*hs; y = ny - 20; XMoveWindow(dpy, mainwin, x, y); }
        XSync(dpy, True);
        XMoveResizeWindow(dpy, win, nx-i*ws, ny, i*2*ws, i*hs);
        XFlush(dpy);
        usleep(25000);
    }

    return win;
}

static void mainRedraw()
{
    XWindowAttributes  wa;
    XGetWindowAttributes(dpy, mainwin, &wa);
    half = wa.width/2;
    XSetForeground(dpy, txtgc, colors[inactive ? color_btnbrd2 : color_fg].pixel);
    mainInputBox(mainwin, 10,10, wa.width-55, source);
    mainButton(mainwin, wa.width>50?wa.width-40:10,10, 30, pressedBtn == 1 ? 1 : 0, 5, "...");
    mainButton(mainwin, 10, 25+fonth, wa.width>30?wa.width-20:10, pressedBtn == 2 ? 1 : 0, 4,
        targetId >= 0 && targetId < numTargetList ? targetList[targetId] : "");
    XDrawLine(dpy, mainwin, txtgc, wa.width - 23, 28+fonth+fonth/2, wa.width - 17, 28+fonth+fonth/2);
    XDrawLine(dpy, mainwin, txtgc, wa.width - 22, 29+fonth+fonth/2, wa.width - 18, 29+fonth+fonth/2);
    XDrawLine(dpy, mainwin, txtgc, wa.width - 21, 30+fonth+fonth/2, wa.width - 19, 30+fonth+fonth/2);
    XDrawLine(dpy, mainwin, txtgc, wa.width - 20, 31+fonth+fonth/2, wa.width - 20, 31+fonth+fonth/2);
    mainCheckbox(mainwin, 10, 44+2*fonth, needVerify);
    mainPrint(mainwin, txtgc, 15 + fonth, 44+2*fonth, half - 10 - fonth, 0, "Verify");
    mainButton(mainwin, half, 40+2*fonth, half - 10, pressedBtn == 3 ? 3 : 2, 5, "Write");
    mainProgress(mainwin, 10, 55+3*fonth, wa.width - 20, progress);
    XSetForeground(dpy, gc, colors[color_winbg].pixel);
    XFillRectangle(dpy, mainwin, gc, 10, 65+3*fonth, wa.width - 20, fonth);
    mainPrint(mainwin, statgc, 10, 65+3*fonth, wa.width - 20, 0, status);
}

/* the usual high-level stuff */

void main_addToCombobox(char *option)
{
    strncpy(targetList[numTargetList++], option, 128);
}

void main_getErrorMessage()
{
    main_errorMessage = errno ? strerror(errno) : NULL;
}

static void onQuit()
{
#if LOADFONT == 1
    if(font) XFreeFont(dpy, font);
#endif
    XFreeGC(dpy, gc);
    XFreeGC(dpy, shdgc);
    XFreeGC(dpy, statgc);
    XFreePixmap(dpy, icons_act);
    XFreePixmap(dpy, icons_ina);
    XDestroyWindow(dpy, mainwin);
    XCloseDisplay(dpy);
}

static void onProgress(void *data)
{
    XWindowAttributes  wa;
    XEvent e;

    progress = input_status((input_t*)data, status);

    if(XPending(dpy)) {
        XNextEvent(dpy, &e);
        if(e.type == ClientMessage && (Atom)(e.xclient.data.l[0]) == delAtom) {
            onQuit();
            exit(1);
        }
        if(e.type == Expose && !e.xexpose.count) {
            mainRedraw();
            XFlush(dpy);
            return;
        }
    }
    XGetWindowAttributes(dpy, mainwin, &wa);
    half = wa.width/2;
    mainProgress(mainwin, 10, 55+3*fonth, wa.width - 20, progress);
    XSetForeground(dpy, gc, colors[color_winbg].pixel);
    XFillRectangle(dpy, mainwin, gc, 10, 65+3*fonth, wa.width - 20, fonth);
    mainPrint(mainwin, statgc, 10, 65+3*fonth, wa.width - 20, 0, status);
    XFlush(dpy);
}

static void onThreadError(void *data)
{
    char *err = main_errorMessage && *main_errorMessage ? main_errorMessage : "Error";
#ifdef MACOSX
    int el = strlen(err), dl = data ? strlen(data) : 0;
    NSAutoreleasePool * pool = [[NSAutoreleasePool alloc] init];
    NSApplication *app = [NSApplication] sharedApplication];
    NSAlert* alert = [[NSAlert alloc] init];

    [[alert window] setTitle:@"Error"];
    [alert setMessageText:[[NSString alloc] initWithBytes:err length:el encoding:NSUTF8StringEncoding]];
    [alert setInformativeText:[[NSString alloc] initWithBytes:data length:dl encoding:NSUTF8StringEncoding]];
    [alert setIcon:[NSImage imageNamed:NSImageNameCaution]];
    [alert setAlertStyle:style];
    [alert setShowsHelp:NO];
    [alert addButtonWithTitle:@"OK"];
    [alert beginSheetModalForWindow:[app mainWindow] completionHandler:^(NSInteger result) {
        [app stopModalWithCode:result];
    }];
    [app runModalForWindow:alert];

    [pool release];
#else
    XEvent e;
    Window win;
    int mw, mh = 4*fonth+40, pressed = 0, w, old = inactive;

    w = mainPrint(mainwin, txtgc, 0, 0, 0, 2, err) + 40;
    mw = mainPrint(mainwin, txtgc, 0, 0, 0, 2, (char*)data) + 40;
    if(w > mw) mw = w;

    win = mainModal(&mw, &mh, color_inputbg);

    while(1) {
        XNextEvent(dpy, &e);
        if(e.type == ClientMessage && (Atom)(e.xclient.data.l[0]) == delAtom) {
            onQuit();
            exit(1);
        }
        if(e.type == KeyPress && XLookupKeysym(&e.xkey, 0) == XK_Return) break;
        if(e.type == ButtonPress && e.xbutton.x > mw/2 - 25 && e.xbutton.x < mw/2 + 25 &&
            e.xbutton.y >= mh-fonth-20 && e.xbutton.y < mh-10) {
                pressed = 1;
                e.type = Expose; e.xexpose.count = 0;
        }
        if(e.type == ButtonRelease) {
            if(e.xbutton.x > mw/2 - 25 && e.xbutton.x < mw/2 + 25 &&
                e.xbutton.y >= mh-fonth-20 && e.xbutton.y < mh-10) break;
            pressed = 0;
            e.type = Expose; e.xexpose.count = 0;
        }
        if(e.type == Expose && !e.xexpose.count) {
            inactive = 0;
            XSetForeground(dpy, txtgc, colors[color_wbtnbg3].pixel);
            mainPrint(win, txtgc, 10, 10, mw-20, 2, err);
            XSetForeground(dpy, txtgc, colors[color_fg].pixel);
            mainPrint(win, txtgc, 10, 20+fonth, mw-20, 2, (char*)data);
            mainButton(win, mw/2 - 25, mh-fonth-20, 50, pressed, 5, "OK");
            inactive = old;
            mainRedraw();
            XFlush(dpy);
        }
    }
    XDestroyWindow(dpy, win);
#endif
    mainRedraw();
    XRaiseWindow(dpy, mainwin);
}

static void *writerRoutine()
{
    int dst, numberOfBytesRead;
    int numberOfBytesWritten, numberOfBytesVerify;
    char buffer[BUFFER_SIZE], verifyBuf[BUFFER_SIZE];
    input_t ctx;

    dst = input_open(&ctx, source);
    if(!dst) {
        dst = (int)((long int)disks_open(targetId));
        if(dst > 0) {
            while(1) {
                if((numberOfBytesRead = input_read(&ctx, buffer)) >= 0) {
                    if(numberOfBytesRead == 0) {
                        if(!ctx.fileSize) ctx.fileSize = ctx.readSize;
                        break;
                    } else {
                        errno = 0;
                        numberOfBytesWritten = (int)write(dst, buffer, numberOfBytesRead);
                        if(verbose) printf("writerRoutine() numberOfBytesRead %d numberOfBytesWritten %d errno=%d\n",
                            numberOfBytesRead, numberOfBytesWritten, errno);
                        if(numberOfBytesWritten == numberOfBytesRead) {
                            if(needVerify) {
                                lseek(dst, -((off_t)numberOfBytesWritten), SEEK_CUR);
                                numberOfBytesVerify = read(dst, verifyBuf, numberOfBytesWritten);
                                if(verbose) printf("  numberOfBytesVerify %d\n", numberOfBytesVerify);
                                if(numberOfBytesVerify != numberOfBytesWritten ||
                                    memcmp(buffer, verifyBuf, numberOfBytesWritten)) {
                                    onThreadError("Write verification failed.");
                                    break;
                                }
                            }
                            onProgress(&ctx);
                        } else {
                            if(errno) main_errorMessage = strerror(errno);
                            onThreadError("An error occurred while writing to the target device.");
                            break;
                        }
                    }
                } else {
                    onThreadError("An error occurred while reading the source file.");
                    break;
                }
            }
            disks_close((void*)((long int)dst));
        } else {
            onThreadError(dst == -1 ? "Please select a valid target." :
                (dst == -2 ? "Unable to umount volumes on target device" :
                "An error occurred while opening the target device."));
        }
        input_close(&ctx);
    } else {
        if(errno) main_errorMessage = strerror(errno);
        onThreadError(dst == 2 ? "Encrypted ZIP not supported" :
            (dst == 3 ? "Unsupported compression method in ZIP" :
            (dst == 4 ? "Decompressor error" :
            "Please select a readable source file.")));
    }
    strcpy(status, !errno && ctx.fileSize && ctx.readSize >= ctx.fileSize ?
        "Done. Image written successfully." : "");
    if(verbose) printf("Worker thread finished.\r\n");
    return NULL;
}

static void onWriteButtonClicked()
{
    inactive = 1;
    progress = 0;
    mainRedraw();
    XFlush(dpy);
    writerRoutine();
    inactive = progress = 0;
    mainRedraw();
    main_errorMessage = NULL;
    memset(status, 0, sizeof(status));
    XSync(dpy, True);
}

static void refreshTarget()
{
    memset(targetList, 0, sizeof(targetList));
    numTargetList = 0;
    disks_refreshlist();
}

static void onSelectClicked()
{
#ifdef MACOSX
    NSAutoreleasePool * pool = [[NSAutoreleasePool alloc] init];
    NSOpenPanel *o = [NSOpenPanel openPanel];
    NSApplication *app = [[NSApplication] sharedApplication];
    [o setCanChooseFiles:YES];
    [o setCanChooseDirectories:NO];
    [o setResolvesAliases:NO];
    [o setAllowsMultipleSelection:NO];
    [o setCanCreateDirectories:NO];
    [o setShowsHiddenFiles:YES];
    [o setExtensionHidden:NO];
    [o setCanSelectHiddenExtension:NO];
    [o setTreatsFilePackagesAsDirectories:YES];
    [o beginSheetModalForWindow:[app mainWindow] completionHandler:^(NSInteger result) {
        [app stopModalWithCode:result];
    }];
    if ([app runModalForWindow:o] == NSFileHandlingPanelOKButton) {
        char *filename = [[NSString stringWithFormat: @"%@", [o URL]] UTF8String];
        if(filename) strncpy(source, filename, PATH_MAX-1);
    }
    [pool release];
#else
    XEvent e;
    Window win;
    char *s, *t, fn[PATH_MAX], path[PATH_MAX/FILENAME_MAX][FILENAME_MAX], **mounts = NULL, *recent;
    char tmp[PATH_MAX+FILENAME_MAX+1], *days[] = { "Sunday", "Monday", "Tuesday", "Wendesday", "Thursday", "Friday", "Saturday" };
    int i, j, x, y, mw = 800, mh = 600, pathlen = 0, pathX[PATH_MAX/FILENAME_MAX+1], numMounts = 0;
    int refresh = 1, pressedPath = -1, pressedBtn = -1, allfiles = 0, fns = 200, ds = 100;
    int scrollMounts = 0, overMount = -1, numFiles = 0, scrollFiles = 0, selFile = -1, lastFile = -2;
    filelist_t *files = NULL;
    uint64_t size;
    DIR *dir;
    FILE *f;
    struct dirent *de;
    struct stat st;
    struct tm *lt;
    time_t now = time(NULL), diff;

    win = mainModal(&mw, &mh, color_winbg);
    recent = disks_volumes(&numMounts, &mounts);
    strcpy(fn, source);
    if(!source[0] && !sorting && recent) sorting = 5;

    while(1) {
        XNextEvent(dpy, &e);
        if(e.type == MotionNotify) {
            i = overMount; overMount = -1;
            if(e.xbutton.x >= 10 && e.xbutton.x < 190 &&
                e.xbutton.y >=20+fonth && e.xbutton.y <= mw-fonth-25) {
                    overMount = (e.xbutton.y - 20-fonth) / (fonth+8) + scrollMounts;
                    if(overMount < 0 || overMount >= numMounts) overMount = -1;
            }
            if(i != overMount) { e.type = Expose; e.xexpose.count = 0; }
        }
        if(e.type == ButtonPress) {
            pressedPath = pressedBtn = -1;
            if(e.xbutton.x >= 10 && e.xbutton.x < 190 &&
                e.xbutton.y >=20+fonth && e.xbutton.y <= mw-fonth-25) {
                    if(e.xbutton.button == 4 && scrollMounts > 0) scrollMounts--;
                    if(e.xbutton.button == 5 && scrollMounts < numMounts-1) scrollMounts++;
                    if(e.xbutton.button < 4 && overMount >=0 && overMount < numMounts) {
                        if(mounts[overMount] != recent) {
                            strcpy(fn, mounts[overMount]);
                            if(mounts[overMount][1]) strcat(fn, "/");
                        } else fn[0] = 0;
                        if(!source[0] && sorting == 5 && recent) sorting = 0;
                        selFile = -1;
                        lastFile = -2;
                        refresh = 1;
                    }
                    e.type = Expose; e.xexpose.count = 0;
            }
            if(e.xbutton.x >= 10 && e.xbutton.y >=10 && e.xbutton.y <= 20+fonth) {
                for(i = 0; i < pathlen; i++)
                    if(e.xbutton.x >= pathX[i] && e.xbutton.x < pathX[i+1]) {
                        pressedPath = i;
                        selFile = -1;
                        lastFile = -2;
                        e.type = Expose; e.xexpose.count = 0;
                        break;
                    }
            }
            if(e.xbutton.x >= 226 && e.xbutton.y >=20+fonth && e.xbutton.y < 28+2*fonth) {
                if(e.xbutton.x < mw - 10 - fns) pressedBtn = 2; else
                if(e.xbutton.x < mw - 10 - ds) pressedBtn = 3; else
                    pressedBtn = 4;
                e.type = Expose; e.xexpose.count = 0; refresh = 1;
            }
            if(e.xbutton.x >= 205 && e.xbutton.x < mw - 15 &&
                e.xbutton.y >=28+2*fonth && e.xbutton.y < mh-fonth-16) {
                    i = (mh-3*fonth-51) / (fonth+8);
                    if(e.xbutton.button == 4 && scrollFiles > 0) scrollFiles--;
                    if(e.xbutton.button == 5 && scrollFiles < numFiles-i) scrollFiles++;
                    if(i >= numFiles) scrollFiles = 0;
                    if(e.xbutton.button < 4) {
                        lastFile = selFile;
                        selFile = (e.xbutton.y - 28-2*fonth) / (fonth+8) + scrollFiles;
                        if(selFile < 0 || selFile >= numFiles) selFile = -1;
                    }
                    e.type = Expose; e.xexpose.count = 0;
            }
            if(e.xbutton.y >=mh-fonth-16 && e.xbutton.y <= mh - 10) {
                if(e.xbutton.x >= 10 && e.xbutton.x < mw - 210 - fonth) {
                    allfiles ^= 1; refresh = 1;
                    e.type = Expose; e.xexpose.count = 0;
                }
                if(e.xbutton.x >= mw - 100 && e.xbutton.x < mw - 10) {
                    pressedBtn = 0;
                    e.type = Expose; e.xexpose.count = 0;
                }
                if(e.xbutton.x >= mw - 200 && e.xbutton.x < mw - 120) {
                    pressedBtn = 1;
                    e.type = Expose; e.xexpose.count = 0;
                }
            }
        }
        if(e.type == ButtonRelease) {
            if(e.xbutton.x >= 205 && e.xbutton.x < mw - 15 &&
                e.xbutton.y >=28+2*fonth && e.xbutton.y < mh-fonth-16 && e.xbutton.button < 4) {
                    i = (e.xbutton.y - 28-2*fonth) / (fonth+8) + scrollFiles;
                    if(i == lastFile && i == selFile) goto ok;
                }
            if(pressedBtn == 1) break;
            if(pressedBtn == 0) {
ok:             if(selFile >=0 && selFile < numFiles) {
                    for(i = 0, tmp[0] = 0; i < pathlen; i++)
                        strcat(tmp, path[i]);
                    strcat(tmp, files[selFile].name);
                    if(!files[selFile].type) {
                        strcpy(fn, tmp);
                        strcat(fn, "/");
                        refresh = 1;
                        selFile = -1;
                        lastFile = -2;
                    } else { strcpy(source, tmp); break; }
                }
            }
            if(pressedBtn > 1) {
                switch(pressedBtn) {
                    case 2: if(sorting==0) sorting = 1; else sorting = 0; break;
                    case 3: if(sorting==2) sorting = 3; else sorting = 2; break;
                    case 4: if(sorting==4) sorting = 5; else sorting = 4; break;
                }
                refresh = 1;
            }
            if(pressedPath != -1) {
                for(i = 0, fn[0] = 0; i <= pressedPath; i++)
                    strcat(fn, path[i]);
                refresh = 1;
            }
            if(pressedPath != -1 || pressedBtn != -1 || refresh) {
                e.type = Expose; e.xexpose.count = 0;
            }
            pressedPath = pressedBtn = -1;
        }
        if(e.type == Expose && !e.xexpose.count) {
            XSetForeground(dpy, gc, colors[color_inpbrd2].pixel);
            XFillRectangle(dpy, win, gc, mw-15, 26+2*fonth, 5, mh-3*fonth-51);
            mainButton(win, mw-110, mh-fonth-20, 100, pressedBtn == 0, 1, "Open");
            mainButton(win, mw-200, mh-fonth-20, 80, pressedBtn == 1, 1, "Cancel");
            y = 20+fonth;
            if(mounts) {
                for(i = scrollMounts; i < numMounts && y+fonth+8 < mh-fonth-20; i++)
                    if(mounts[i]) {
                        XSetForeground(dpy, gc, colors[i == overMount ? color_inpdrk : color_inputbg].pixel);
                        XFillRectangle(dpy, win, gc, 10, y, 190, fonth+8);
                        if(mounts[i] == recent) { s = "Recently Used"; j = 0; } else
                        if(!memcmp(mounts[i], "/home/", 6)) { s = "Home"; j = 1; } else
                        if(!strcmp(mounts[i], "/")) { s = "Filesystem"; j = 2; } else
                            { s = mounts[i]; j = 3; }
                        XCopyArea(dpy, i == overMount ? icons_act : icons_ina, win, gc, 0, j*16, 16, 16, 14, y-4+fonth/2);
                        mainPrint(win, i == overMount ? shdgc : txtgc, 34, y+4, 162, 2, s);
                        y += fonth + 8;
                    }
            }
            XSetForeground(dpy, gc, colors[color_inputbg].pixel);
            XFillRectangle(dpy, win, gc, 10, y, 190, mh-fonth-25-y);
            if(refresh || pressedPath != -1 || pressedBtn > 1) {
                refresh = 0;
                XSetForeground(dpy, gc, colors[color_winbg].pixel);
                XFillRectangle(dpy, win, gc, 10, 10, mw-20, 10+fonth);
                memset(path, 0, sizeof(path));
                for(x = y = pathlen = 0, s = fn; *s; s++) {
                    if(y < FILENAME_MAX-1) path[pathlen][y++] = *s;
                    if(*s == '/') { y = 0; pathlen++; }
                }
                memset(pathX, 0, sizeof(pathX));
                for(i = 0, x = 10; i < pathlen; i++) {
                    pathX[i] = x;
                    y = mainPrint(win, txtgc, 0, 0, 0, 0, path[i]);
                    mainBox(win, x, 10, y+12, pressedPath == i, path[i]);
                    x += y+20;
                }
                pathX[i] = x;
                if(!pathlen)
                    mainPrint(win, txtgc, 10, 10, mw-20, 1, "Recently Used Files");
                XSetForeground(dpy, gc, colors[color_inpbrd1].pixel);
                XDrawLine(dpy, win, gc, 9, 19+fonth, 201, 19+fonth);
                XDrawLine(dpy, win, gc, 204, 19+fonth, mw-10, 19+fonth);
                XDrawLine(dpy, win, gc, 9, 20+fonth, 9, mh-fonth-25);
                XDrawLine(dpy, win, gc, 204, 20+fonth, 204, mh-fonth-25);
                XSetForeground(dpy, gc, colors[color_inpbrd3].pixel);
                XDrawLine(dpy, win, gc, 201, 21+fonth, 201, mh-fonth-26);
                XDrawLine(dpy, win, gc, mw-9, 21+fonth, mw-9, mh-fonth-26);
                XDrawLine(dpy, win, gc, 9, mh-fonth-24, 201, mh-fonth-24);
                XDrawLine(dpy, win, gc, 204, mh-fonth-24, mw-9, mh-fonth-24);
                mainBox(win, 205, 20+fonth, 20, 0, "");
                mainBox(win, 226, 20+fonth, mw - 237 - fns, pressedBtn == 2, "Name");
                mainBox(win, mw-fns-10, 20+fonth, (mw-ds) - (mw-fns) - 1, pressedBtn == 3, "Size");
                mainBox(win, mw-ds-10, 20+fonth, ds, pressedBtn == 4, "Modified");
                switch(sorting) {
                    case 0: x = mw-fns-20; i = 0; break;
                    case 1: x = mw-fns-20; i = 1; break;
                    case 2: x = mw-ds-20; i = 0; break;
                    case 3: x = mw-ds-20; i = 1; break;
                    case 4: x = mw-20; i = 0; break;
                    case 5: x = mw-20; i = 1; break;
                    break;
                }
                if(i) {
                    XDrawLine(dpy, win, txtgc, x - 0, 21+fonth+fonth/2, x + 0, 21+fonth+fonth/2);
                    XDrawLine(dpy, win, txtgc, x - 1, 22+fonth+fonth/2, x + 1, 22+fonth+fonth/2);
                    XDrawLine(dpy, win, txtgc, x - 2, 23+fonth+fonth/2, x + 2, 23+fonth+fonth/2);
                    XDrawLine(dpy, win, txtgc, x - 3, 24+fonth+fonth/2, x + 3, 24+fonth+fonth/2);
                } else {
                    XDrawLine(dpy, win, txtgc, x - 3, 21+fonth+fonth/2, x + 3, 21+fonth+fonth/2);
                    XDrawLine(dpy, win, txtgc, x - 2, 22+fonth+fonth/2, x + 2, 22+fonth+fonth/2);
                    XDrawLine(dpy, win, txtgc, x - 1, 23+fonth+fonth/2, x + 1, 23+fonth+fonth/2);
                    XDrawLine(dpy, win, txtgc, x - 0, 24+fonth+fonth/2, x + 0, 24+fonth+fonth/2);
                }
                mainPrint(win, txtgc, 20 + fonth, mh-fonth-16, mw - 220 - fonth, 0, "All Files");
                mainCheckbox(win, 15, mh-fonth-16, allfiles);
                XSetForeground(dpy, gc, colors[color_inputbg].pixel);
                XFillRectangle(dpy, win, gc, 205, 26+2*fonth, mw-220, mh-3*fonth-51);
                XFlush(dpy);
                if(files) {
                    for(i = 0; i < numFiles; i++)
                        if(files[i].name)
                            free(files[i].name);
                    free(files);
                    files = NULL;
                }
                numFiles = scrollFiles = 0;
                if(fn[0]) {
                    for(i = 0, tmp[0] = 0; i < pathlen; i++)
                        strcat(tmp, path[i]);
                    x = strlen(tmp);
                    dir = opendir(tmp);
                    if(dir) {
                        while((de = readdir(dir))) {
                            if(!strcmp(de->d_name, ".") || !strcmp(de->d_name, "..") ||
                                (de->d_name[0] == '.' && !allfiles)) continue;
                            strcpy(tmp + x, de->d_name);
                            if(stat(tmp, &st)) continue;
                            if(!allfiles && !S_ISREG(st.st_mode) && !S_ISDIR(st.st_mode) && !S_ISBLK(st.st_mode))
                                continue;
                            i = numFiles++;
                            files = (filelist_t*)realloc(files, numFiles * sizeof(filelist_t));
                            if(!files) { numFiles = 0; break; }
                            files[i].name = (char*)malloc(strlen(de->d_name)+1);
                            if(!files[i].name) { numFiles--; continue; }
                            strcpy(files[i].name, de->d_name);
                            files[i].type = S_ISDIR(st.st_mode) ? 0 : (S_ISBLK(st.st_mode) ? 1 : 2);
                            files[i].size = st.st_size;
                            files[i].time = st.st_mtime;
                        }
                        closedir(dir);
                    }
                } else if(recent) {
                    f = fopen(recent, "r");
                    if(f) {
                        while(!feof(f)) {
                            memset(tmp, 0, sizeof(tmp));
                            fgets(tmp, sizeof(tmp) - 1, f);
                            for(s = tmp, x = 0; *s; s++) {
                                if(!memcmp(s, "<bookmark ", 10)) x = 1;
                                if(x == 1 && !memcmp(s, "href=", 5)) x = 2;
                                if(x == 2 && !memcmp(s, "file://", 7)) {
                                    s += 7; for(t = s; *t && *t != '\"'; t++);
                                    *t = 0;
                                    if(!stat(s, &st)) {
                                        i = numFiles++;
                                        files = (filelist_t*)realloc(files, numFiles * sizeof(filelist_t));
                                        if(!files) { numFiles = 0; break; }
                                        files[i].name = (char*)malloc(t-s+1);
                                        if(!files[i].name) { numFiles--; continue; }
                                        strcpy(files[i].name, s);
                                        files[i].type = S_ISDIR(st.st_mode) ? 0 : (S_ISBLK(st.st_mode) ? 1 : 2);
                                        files[i].size = st.st_size;
                                        files[i].time = st.st_atime ? st.st_atime : st.st_mtime;
                                    }
                                    s = t;
                                }
                            }
                        }
                        fclose(f);
                    }
                }
                qsort(files, numFiles, sizeof(filelist_t), fncmp);
            }
            y = 26+2*fonth;
            for(i = scrollFiles; i < numFiles && y+fonth+8 < mh-fonth-20; i++)
                if(files[i].name && files[i].name[0]) {
                    if(selFile == -1 && path[pathlen][0] && !strcmp(files[i].name, path[pathlen])) selFile = i;
                    XSetForeground(dpy, gc, colors[i == selFile ? color_inpdrk : color_inputbg].pixel);
                    XFillRectangle(dpy, win, gc, 205, y, mw-220, fonth+8);
                    XCopyArea(dpy, i == selFile ? icons_act : icons_ina, win, gc, 0, (files[i].type+3)*16, 16, 16, 209, y-4+fonth/2);
                    s = strrchr(files[i].name,'/');
                    if(s) s++; else s = files[i].name;
                    mainPrint(win, i == selFile ? shdgc : txtgc, 230, y+4, mw-241-fns, 2, s);
                    if(files[i].type) {
                        size = files[i].size;
                        if(size < 1024L*1024L)
                            sprintf(tmp, "%u", (unsigned int)size);
                        else {
                            size >>= 20;
                            if(size < 1024L)
                                sprintf(tmp, "%u Mb", (unsigned int)size);
                            else {
                                size >>= 10;
                                sprintf(tmp, "%u Gb", (unsigned int)size);
                            }
                        }
                        mainPrint(win, i == selFile ? shdgc : txtgc, mw-fns-14, y+4, (mw-ds) - (mw-fns) - 1, 3, tmp);
                    }
                    diff = now - files[i].time;
                    if(diff < 60) strcpy(tmp, "Just now"); else
                    if(diff < 3600) sprintf(tmp, "%d minute%s ago", (int)(diff/60), diff >= 120 ? "s" : ""); else
                    if(diff < 24*3600) sprintf(tmp, "%d hour%s ago", (int)(diff/3600), diff >= 7200 ? "s" : ""); else
                    if(diff < 48*3600) strcpy(tmp, "Yesterday"); else {
                        lt = localtime(&files[i].time);
                        if(diff < 7*24*3600) strcpy(tmp, days[lt->tm_wday]); else
                            sprintf(tmp, "%04d-%02d-%02d", lt->tm_year+1900, lt->tm_mon+1, lt->tm_mday);
                    }
                    mainPrint(win, i == selFile ? shdgc : txtgc, mw-ds-6, y+4, ds-12, 2, tmp);
                    y += fonth + 8;
                }
            XSetForeground(dpy, gc, colors[color_inputbg].pixel);
            XFillRectangle(dpy, win, gc, 205, y, mw-220, mh-fonth-25-y);
            x = mh-3*fonth-51;
            i = x / (fonth+8);
            if(i < numFiles) {
                y = x;
                x = x * i / numFiles;
                y = (y - x) * scrollFiles / (numFiles - i);
            } else y = 0;
            XSetForeground(dpy, gc, colors[color_btnbrd2].pixel);
            XFillRectangle(dpy, win, gc, mw-15, 26+2*fonth+y, 5, x);
        }
        XFlush(dpy);
    }
    if(mounts) {
        for(i = 0; i < numMounts; i++)
            if(mounts[i]) free(mounts[i]);
        free(mounts);
    }
    if(files) {
        for(i = 0; i < numFiles; i++)
            if(files[i].name)
                free(files[i].name);
        free(files);
    }
    XDestroyWindow(dpy, win);
    XSync(dpy, True);
#endif
    mainRedraw();
    XRaiseWindow(dpy, mainwin);
}

static void onTargetClicked()
{
    XWindowAttributes  wa;
    XSetWindowAttributes swa;
    XEvent e;
    Window win, child, root = RootWindow(dpy, scr);
    int x, y, sel;

    refreshTarget();
    if(numTargetList < 1) return;

    XTranslateCoordinates(dpy, mainwin, root, 0, 0, &x, &y, &child);
    XGetWindowAttributes(dpy, mainwin, &wa);
    x -= wa.x - wa.border_width; y -= wa.y;
    win = XCreateSimpleWindow(dpy, root, x+15, y+25+fonth+20-(targetId < 0 ? 0 : targetId)*(fonth+8),
        wa.width-25, numTargetList*(fonth+8), 0, 0, colors[color_inputbg].pixel);
    XSelectInput(dpy, win, ExposureMask | ButtonPressMask | ButtonReleaseMask | PointerMotionMask |
        KeyPressMask | KeyReleaseMask | FocusChangeMask | LeaveWindowMask);
    swa.override_redirect = True;
    XChangeWindowAttributes(dpy, win, CWOverrideRedirect, &swa);
    XMapWindow(dpy, win);
    XRaiseWindow(dpy, win);
    sel = targetId;
    if(sel < 0 || sel >= numTargetList) sel = 0;
    while(1) {
        XNextEvent(dpy, &e);
        if(e.type == MotionNotify) {
            x = e.xmotion.y / (fonth+8);
            if(x != targetId) {
                sel = x;
                e.type = Expose;
                e.xexpose.count = 0;
            }
        }
        if(e.type == Expose && !e.xexpose.count) {
            for(x = 0; x < numTargetList; x++) {
                XSetForeground(dpy, gc, colors[sel == x ? color_inpdrk : color_inputbg].pixel);
                XFillRectangle(dpy, win, gc, 0, x*(fonth+8), wa.width - 25, fonth+8);
                mainPrint(win, sel == x ? shdgc : txtgc, 4, 4 + x*(fonth+8), wa.width - 29, 0, targetList[x]);
            }
        }
        if(e.type == ButtonRelease) { targetId = sel; break; }
        if(e.type == FocusOut || e.type == LeaveNotify) break;
        XFlush(dpy);
    }
    XDestroyWindow(dpy, win);
    XSync(dpy, True);
    mainRedraw();
    XRaiseWindow(dpy, mainwin);
}

int main(int argc, char **argv)
{
    XEvent e;
    char colorName[16];
    int i;

    if(argc > 1 && argv[1] && argv[1][0] == '-')
        for(i = 1; argv[1][i]; i++)
            if(argv[1][1] == 'v') verbose++;

    dpy = XOpenDisplay(NULL);
    if(!dpy) { fprintf(stderr, "Unable to open display\n"); return 1; }
    scr = DefaultScreen(dpy);

    for(i = 0; i < NUM_COLOR; i++) {
        sprintf(colorName, "rgb:%02x/%02x/%02x", ((unsigned char*)&palette[i])[2],
            ((unsigned char*)&palette[i])[1], ((unsigned char*)&palette[i])[0]);
        if(!XParseColor(dpy, DefaultColormap(dpy, scr), colorName, &colors[i]) ||
            !XAllocColor(dpy, DefaultColormap(dpy, scr), &colors[i]))
                colors[i].pixel = 0xFF000000 | palette[i];
    }

    txtgc = DefaultGC(dpy, scr);
    XSetForeground(dpy, txtgc, BlackPixel(dpy, scr));
#if LOADFONT == 1
    font = XLoadQueryFont(dpy, "-*-clean-medium-r-*-*-16-*-*-*-*-*-iso10646-1");
    if(!font) font = XLoadQueryFont(dpy, "-*-*-medium-r-*-*-16-*-*-*-*-*-iso10646-1");
    if(!font) font = XLoadQueryFont(dpy, "-*-*-medium-r-*-*-16-*-*-*-*-*-*-*");
    if(font)
        XSetFont(dpy, txtgc, font->fid);
    else
        /* fallback to default bitmap font */
#endif
        font = XQueryFont(dpy, XGContextFromGC(txtgc));
    if(!font) { fprintf(stderr, "Unable to get font\n"); return 1; }
    fonth = font->max_bounds.ascent + font->max_bounds.descent;
    fonta = font->max_bounds.ascent;

    mainwin = XCreateSimpleWindow(dpy, RootWindow(dpy, scr), 0, 0,
        320, 75+4*fonth, 0, 0, colors[color_winbg].pixel);
    XSelectInput(dpy, mainwin, ExposureMask | ButtonPressMask | ButtonReleaseMask | KeyPressMask);
    XStoreName(dpy, mainwin, "USBImager");
    delAtom= XInternAtom(dpy, "WM_DELETE_WINDOW", False);
    XSetWMProtocols(dpy, mainwin, &delAtom, 1);
    XMapWindow(dpy, mainwin);
    XRaiseWindow(dpy, mainwin);
    shdgc = XCreateGC(dpy, mainwin, 0, NULL);
    XCopyGC(dpy, txtgc, 0xFFFFFFFF, shdgc);
    XSetForeground(dpy, shdgc, colors[color_btnbg1].pixel);
    statgc = XCreateGC(dpy, mainwin, 0, NULL);
    XCopyGC(dpy, txtgc, 0xFFFFFFFF, statgc);
    gc = XCreateGC(dpy, mainwin, 0, NULL);
    XCopyGC(dpy, txtgc, 0xFFFFFFFF, gc);
    icons_act = XCreatePixmapFromBitmapData(dpy, mainwin, (char*)icons_bits,
        icons_width, icons_height, colors[color_inputbg].pixel, colors[color_inpdrk].pixel, DefaultDepth(dpy, scr));
    icons_ina = XCreatePixmapFromBitmapData(dpy, mainwin, (char*)icons_bits,
        icons_width, icons_height, BlackPixel(dpy, scr), colors[color_inputbg].pixel, DefaultDepth(dpy, scr));

    memset(source, 0, sizeof(source));
    memset(status, 0, sizeof(status));

    while(1) {
        XNextEvent(dpy, &e);
        if((e.type == ClientMessage && (Atom)(e.xclient.data.l[0]) == delAtom) ||
           (e.type == KeyPress && XLookupKeysym(&e.xkey,0) == XK_Escape))
            break;
        if(e.type == Expose && !e.xexpose.count) mainRedraw();
        if(e.type == ButtonPress && !inactive) {
            pressedBtn = 0;
            if(e.xbutton.y >=10 && e.xbutton.y < 10 + fonth + 8) pressedBtn = 1; else
            if(e.xbutton.y >=25 + fonth && e.xbutton.y < 25 + 2*fonth + 8) pressedBtn = 2; else
            if(e.xbutton.y >=40 + 2*fonth && e.xbutton.y < 40 + 3*fonth + 8) {
                if(e.xbutton.x < half) needVerify ^= 1;
                else pressedBtn = 3;
            }
            mainRedraw();
        }
        if(e.type == ButtonRelease) {
            i = pressedBtn; pressedBtn = 0;
            mainRedraw();
            switch(i) {
                case 1: onSelectClicked(); break;
                case 2: onTargetClicked(); break;
                case 3: onWriteButtonClicked(); break;
            }
        }
        XFlush(dpy);
    }

    onQuit();
    return 0;
}
