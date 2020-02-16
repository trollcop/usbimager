/*
 * libui
 *
 * Copyright (c) 2014 Pietro Gagliardi
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, in
 * cluding without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to
 * do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRIN
 * GEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CO
 * NNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 * (this is called the MIT License or Expat License; see http://www.opensource.org/licenses/MIT)
 *
 * @brief See https://github.com/andlabs/libui/
 */

#ifndef __LIBUI_UI_H__
#define __LIBUI_UI_H__

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifdef libui_EXPORTS
#ifdef _WIN32
#define _UI_EXTERN __declspec(dllexport) extern
#else
#define _UI_EXTERN __attribute__((visibility("default"))) extern
#endif
#else
#define _UI_EXTERN extern
#endif

#define _UI_ENUM(s) typedef unsigned int s; enum

#define uiPi 3.14159265358979323846264338327950288419716939937510582097494459

_UI_ENUM(uiForEach) {
    uiForEachContinue,
    uiForEachStop
};

typedef struct uiInitOptions uiInitOptions;

struct uiInitOptions {
    size_t Size;
};

_UI_EXTERN const char *uiInit(uiInitOptions *options);
_UI_EXTERN void uiUninit(void);
_UI_EXTERN void uiFreeInitError(const char *err);

_UI_EXTERN void uiMain(void);
_UI_EXTERN void uiMainSteps(void);
_UI_EXTERN int uiMainStep(int wait);
_UI_EXTERN void uiQuit(void);

_UI_EXTERN void uiQueueMain(void (*f)(void *data), void *data);

_UI_EXTERN void uiTimer(int milliseconds, int (*f)(void *data), void *data);

_UI_EXTERN void uiOnShouldQuit(int (*f)(void *data), void *data);

_UI_EXTERN void uiFreeText(char *text);

typedef struct uiControl uiControl;

struct uiControl {
    uint32_t Signature;
    uint32_t OSSignature;
    uint32_t TypeSignature;
    void (*Destroy)(uiControl *);
    uintptr_t (*Handle)(uiControl *);
    uiControl *(*Parent)(uiControl *);
    void (*SetParent)(uiControl *, uiControl *);
    int (*Toplevel)(uiControl *);
    int (*Visible)(uiControl *);
    void (*Show)(uiControl *);
    void (*Hide)(uiControl *);
    int (*Enabled)(uiControl *);
    void (*Enable)(uiControl *);
    void (*Disable)(uiControl *);
};
#define uiControl(this) ((uiControl *) (this))
_UI_EXTERN void uiControlDestroy(uiControl *);
_UI_EXTERN uintptr_t uiControlHandle(uiControl *);
_UI_EXTERN uiControl *uiControlParent(uiControl *);
_UI_EXTERN void uiControlSetParent(uiControl *, uiControl *);
_UI_EXTERN int uiControlToplevel(uiControl *);
_UI_EXTERN int uiControlVisible(uiControl *);
_UI_EXTERN void uiControlShow(uiControl *);
_UI_EXTERN void uiControlHide(uiControl *);
_UI_EXTERN int uiControlEnabled(uiControl *);
_UI_EXTERN void uiControlEnable(uiControl *);
_UI_EXTERN void uiControlDisable(uiControl *);

_UI_EXTERN uiControl *uiAllocControl(size_t n, uint32_t OSsig, uint32_t typesig, const char *typenamestr);
_UI_EXTERN void uiFreeControl(uiControl *);

_UI_EXTERN void uiControlVerifySetParent(uiControl *, uiControl *);
_UI_EXTERN int uiControlEnabledToUser(uiControl *);

_UI_EXTERN void uiUserBugCannotSetParentOnToplevel(const char *type);

typedef struct uiWindow uiWindow;
#define uiWindow(this) ((uiWindow *) (this))
_UI_EXTERN char *uiWindowTitle(uiWindow *w);
_UI_EXTERN void uiWindowSetTitle(uiWindow *w, const char *title);
_UI_EXTERN void uiWindowContentSize(uiWindow *w, int *width, int *height);
_UI_EXTERN void uiWindowSetContentSize(uiWindow *w, int width, int height);
_UI_EXTERN int uiWindowFullscreen(uiWindow *w);
_UI_EXTERN void uiWindowSetFullscreen(uiWindow *w, int fullscreen);
_UI_EXTERN void uiWindowOnContentSizeChanged(uiWindow *w, void (*f)(uiWindow *, void *), void *data);
_UI_EXTERN void uiWindowOnClosing(uiWindow *w, int (*f)(uiWindow *w, void *data), void *data);
_UI_EXTERN int uiWindowBorderless(uiWindow *w);
_UI_EXTERN void uiWindowSetBorderless(uiWindow *w, int borderless);
_UI_EXTERN void uiWindowSetChild(uiWindow *w, uiControl *child);
_UI_EXTERN int uiWindowMargined(uiWindow *w);
_UI_EXTERN void uiWindowSetMargined(uiWindow *w, int margined);
_UI_EXTERN uiWindow *uiNewWindow(const char *title, int width, int height, int hasMenubar);

typedef struct uiButton uiButton;
#define uiButton(this) ((uiButton *) (this))
_UI_EXTERN char *uiButtonText(uiButton *b);
_UI_EXTERN void uiButtonSetText(uiButton *b, const char *text);
_UI_EXTERN void uiButtonOnClicked(uiButton *b, void (*f)(uiButton *b, void *data), void *data);
_UI_EXTERN uiButton *uiNewButton(const char *text);

typedef struct uiBox uiBox;
#define uiBox(this) ((uiBox *) (this))
_UI_EXTERN void uiBoxAppend(uiBox *b, uiControl *child, int stretchy);
_UI_EXTERN void uiBoxDelete(uiBox *b, int index);
_UI_EXTERN int uiBoxPadded(uiBox *b);
_UI_EXTERN void uiBoxSetPadded(uiBox *b, int padded);
_UI_EXTERN uiBox *uiNewHorizontalBox(void);
_UI_EXTERN uiBox *uiNewVerticalBox(void);

typedef struct uiCheckbox uiCheckbox;
#define uiCheckbox(this) ((uiCheckbox *) (this))
_UI_EXTERN char *uiCheckboxText(uiCheckbox *c);
_UI_EXTERN void uiCheckboxSetText(uiCheckbox *c, const char *text);
_UI_EXTERN void uiCheckboxOnToggled(uiCheckbox *c, void (*f)(uiCheckbox *c, void *data), void *data);
_UI_EXTERN int uiCheckboxChecked(uiCheckbox *c);
_UI_EXTERN void uiCheckboxSetChecked(uiCheckbox *c, int checked);
_UI_EXTERN uiCheckbox *uiNewCheckbox(const char *text);

typedef struct uiEntry uiEntry;
#define uiEntry(this) ((uiEntry *) (this))
_UI_EXTERN char *uiEntryText(uiEntry *e);
_UI_EXTERN void uiEntrySetText(uiEntry *e, const char *text);
_UI_EXTERN void uiEntryOnChanged(uiEntry *e, void (*f)(uiEntry *e, void *data), void *data);
_UI_EXTERN int uiEntryReadOnly(uiEntry *e);
_UI_EXTERN void uiEntrySetReadOnly(uiEntry *e, int readonly);
_UI_EXTERN uiEntry *uiNewEntry(void);
_UI_EXTERN uiEntry *uiNewPasswordEntry(void);
_UI_EXTERN uiEntry *uiNewSearchEntry(void);

typedef struct uiLabel uiLabel;
#define uiLabel(this) ((uiLabel *) (this))
_UI_EXTERN char *uiLabelText(uiLabel *l);
_UI_EXTERN void uiLabelSetText(uiLabel *l, const char *text);
_UI_EXTERN uiLabel *uiNewLabel(const char *text);

typedef struct uiTab uiTab;
#define uiTab(this) ((uiTab *) (this))
_UI_EXTERN void uiTabAppend(uiTab *t, const char *name, uiControl *c);
_UI_EXTERN void uiTabInsertAt(uiTab *t, const char *name, int before, uiControl *c);
_UI_EXTERN void uiTabDelete(uiTab *t, int index);
_UI_EXTERN int uiTabNumPages(uiTab *t);
_UI_EXTERN int uiTabMargined(uiTab *t, int page);
_UI_EXTERN void uiTabSetMargined(uiTab *t, int page, int margined);
_UI_EXTERN uiTab *uiNewTab(void);

typedef struct uiGroup uiGroup;
#define uiGroup(this) ((uiGroup *) (this))
_UI_EXTERN char *uiGroupTitle(uiGroup *g);
_UI_EXTERN void uiGroupSetTitle(uiGroup *g, const char *title);
_UI_EXTERN void uiGroupSetChild(uiGroup *g, uiControl *c);
_UI_EXTERN int uiGroupMargined(uiGroup *g);
_UI_EXTERN void uiGroupSetMargined(uiGroup *g, int margined);
_UI_EXTERN uiGroup *uiNewGroup(const char *title);

typedef struct uiSpinbox uiSpinbox;
#define uiSpinbox(this) ((uiSpinbox *) (this))
_UI_EXTERN int uiSpinboxValue(uiSpinbox *s);
_UI_EXTERN void uiSpinboxSetValue(uiSpinbox *s, int value);
_UI_EXTERN void uiSpinboxOnChanged(uiSpinbox *s, void (*f)(uiSpinbox *s, void *data), void *data);
_UI_EXTERN uiSpinbox *uiNewSpinbox(int min, int max);

typedef struct uiSlider uiSlider;
#define uiSlider(this) ((uiSlider *) (this))
_UI_EXTERN int uiSliderValue(uiSlider *s);
_UI_EXTERN void uiSliderSetValue(uiSlider *s, int value);
_UI_EXTERN void uiSliderOnChanged(uiSlider *s, void (*f)(uiSlider *s, void *data), void *data);
_UI_EXTERN uiSlider *uiNewSlider(int min, int max);

typedef struct uiProgressBar uiProgressBar;
#define uiProgressBar(this) ((uiProgressBar *) (this))
_UI_EXTERN int uiProgressBarValue(uiProgressBar *p);
_UI_EXTERN void uiProgressBarSetValue(uiProgressBar *p, int n);
_UI_EXTERN uiProgressBar *uiNewProgressBar(void);

typedef struct uiSeparator uiSeparator;
#define uiSeparator(this) ((uiSeparator *) (this))
_UI_EXTERN uiSeparator *uiNewHorizontalSeparator(void);
_UI_EXTERN uiSeparator *uiNewVerticalSeparator(void);

typedef struct uiCombobox uiCombobox;
#define uiCombobox(this) ((uiCombobox *) (this))
_UI_EXTERN void uiComboboxAppend(uiCombobox *c, const char *text);
_UI_EXTERN int uiComboboxSelected(uiCombobox *c);
_UI_EXTERN void uiComboboxSetSelected(uiCombobox *c, int n);
_UI_EXTERN void uiComboboxOnSelected(uiCombobox *c, void (*f)(uiCombobox *c, void *data), void *data);
_UI_EXTERN uiCombobox *uiNewCombobox(void);

typedef struct uiEditableCombobox uiEditableCombobox;
#define uiEditableCombobox(this) ((uiEditableCombobox *) (this))
_UI_EXTERN void uiEditableComboboxAppend(uiEditableCombobox *c, const char *text);
_UI_EXTERN char *uiEditableComboboxText(uiEditableCombobox *c);
_UI_EXTERN void uiEditableComboboxSetText(uiEditableCombobox *c, const char *text);
_UI_EXTERN void uiEditableComboboxOnChanged(uiEditableCombobox *c, void (*f)(uiEditableCombobox *c, void *data), void *data);
_UI_EXTERN uiEditableCombobox *uiNewEditableCombobox(void);

typedef struct uiRadioButtons uiRadioButtons;
#define uiRadioButtons(this) ((uiRadioButtons *) (this))
_UI_EXTERN void uiRadioButtonsAppend(uiRadioButtons *r, const char *text);
_UI_EXTERN int uiRadioButtonsSelected(uiRadioButtons *r);
_UI_EXTERN void uiRadioButtonsSetSelected(uiRadioButtons *r, int n);
_UI_EXTERN void uiRadioButtonsOnSelected(uiRadioButtons *r, void (*f)(uiRadioButtons *, void *), void *data);
_UI_EXTERN uiRadioButtons *uiNewRadioButtons(void);

struct tm;
typedef struct uiDateTimePicker uiDateTimePicker;
#define uiDateTimePicker(this) ((uiDateTimePicker *) (this))
_UI_EXTERN void uiDateTimePickerTime(uiDateTimePicker *d, struct tm *time);
_UI_EXTERN void uiDateTimePickerSetTime(uiDateTimePicker *d, const struct tm *time);
_UI_EXTERN void uiDateTimePickerOnChanged(uiDateTimePicker *d, void (*f)(uiDateTimePicker *, void *), void *data);
_UI_EXTERN uiDateTimePicker *uiNewDateTimePicker(void);
_UI_EXTERN uiDateTimePicker *uiNewDatePicker(void);
_UI_EXTERN uiDateTimePicker *uiNewTimePicker(void);

typedef struct uiMultilineEntry uiMultilineEntry;
#define uiMultilineEntry(this) ((uiMultilineEntry *) (this))
_UI_EXTERN char *uiMultilineEntryText(uiMultilineEntry *e);
_UI_EXTERN void uiMultilineEntrySetText(uiMultilineEntry *e, const char *text);
_UI_EXTERN void uiMultilineEntryAppend(uiMultilineEntry *e, const char *text);
_UI_EXTERN void uiMultilineEntryOnChanged(uiMultilineEntry *e, void (*f)(uiMultilineEntry *e, void *data), void *data);
_UI_EXTERN int uiMultilineEntryReadOnly(uiMultilineEntry *e);
_UI_EXTERN void uiMultilineEntrySetReadOnly(uiMultilineEntry *e, int readonly);
_UI_EXTERN uiMultilineEntry *uiNewMultilineEntry(void);
_UI_EXTERN uiMultilineEntry *uiNewNonWrappingMultilineEntry(void);

typedef struct uiMenuItem uiMenuItem;
#define uiMenuItem(this) ((uiMenuItem *) (this))
_UI_EXTERN void uiMenuItemEnable(uiMenuItem *m);
_UI_EXTERN void uiMenuItemDisable(uiMenuItem *m);
_UI_EXTERN void uiMenuItemOnClicked(uiMenuItem *m, void (*f)(uiMenuItem *sender, uiWindow *window, void *data), void *data);
_UI_EXTERN int uiMenuItemChecked(uiMenuItem *m);
_UI_EXTERN void uiMenuItemSetChecked(uiMenuItem *m, int checked);

typedef struct uiMenu uiMenu;
#define uiMenu(this) ((uiMenu *) (this))
_UI_EXTERN uiMenuItem *uiMenuAppendItem(uiMenu *m, const char *name);
_UI_EXTERN uiMenuItem *uiMenuAppendCheckItem(uiMenu *m, const char *name);
_UI_EXTERN uiMenuItem *uiMenuAppendQuitItem(uiMenu *m);
_UI_EXTERN uiMenuItem *uiMenuAppendPreferencesItem(uiMenu *m);
_UI_EXTERN uiMenuItem *uiMenuAppendAboutItem(uiMenu *m);
_UI_EXTERN void uiMenuAppendSeparator(uiMenu *m);
_UI_EXTERN uiMenu *uiNewMenu(const char *name);

_UI_EXTERN char *uiOpenFile(uiWindow *parent);
_UI_EXTERN char *uiSaveFile(uiWindow *parent);
_UI_EXTERN void uiMsgBox(uiWindow *parent, const char *title, const char *description);
_UI_EXTERN void uiMsgBoxError(uiWindow *parent, const char *title, const char *description);

typedef struct uiArea uiArea;
typedef struct uiAreaHandler uiAreaHandler;
typedef struct uiAreaDrawParams uiAreaDrawParams;
typedef struct uiAreaMouseEvent uiAreaMouseEvent;
typedef struct uiAreaKeyEvent uiAreaKeyEvent;

typedef struct uiDrawContext uiDrawContext;

struct uiAreaHandler {
    void (*Draw)(uiAreaHandler *, uiArea *, uiAreaDrawParams *);
    void (*MouseEvent)(uiAreaHandler *, uiArea *, uiAreaMouseEvent *);
    void (*MouseCrossed)(uiAreaHandler *, uiArea *, int left);
    void (*DragBroken)(uiAreaHandler *, uiArea *);
    int (*KeyEvent)(uiAreaHandler *, uiArea *, uiAreaKeyEvent *);
};

_UI_ENUM(uiWindowResizeEdge) {
    uiWindowResizeEdgeLeft,
    uiWindowResizeEdgeTop,
    uiWindowResizeEdgeRight,
    uiWindowResizeEdgeBottom,
    uiWindowResizeEdgeTopLeft,
    uiWindowResizeEdgeTopRight,
    uiWindowResizeEdgeBottomLeft,
    uiWindowResizeEdgeBottomRight
};

#define uiArea(this) ((uiArea *) (this))
_UI_EXTERN void uiAreaSetSize(uiArea *a, int width, int height);
_UI_EXTERN void uiAreaQueueRedrawAll(uiArea *a);
_UI_EXTERN void uiAreaScrollTo(uiArea *a, double x, double y, double width, double height);
_UI_EXTERN void uiAreaBeginUserWindowMove(uiArea *a);
_UI_EXTERN void uiAreaBeginUserWindowResize(uiArea *a, uiWindowResizeEdge edge);
_UI_EXTERN uiArea *uiNewArea(uiAreaHandler *ah);
_UI_EXTERN uiArea *uiNewScrollingArea(uiAreaHandler *ah, int width, int height);

struct uiAreaDrawParams {
    uiDrawContext *Context;

    double AreaWidth;
    double AreaHeight;

    double ClipX;
    double ClipY;
    double ClipWidth;
    double ClipHeight;
};

typedef struct uiDrawPath uiDrawPath;
typedef struct uiDrawBrush uiDrawBrush;
typedef struct uiDrawStrokeParams uiDrawStrokeParams;
typedef struct uiDrawMatrix uiDrawMatrix;

typedef struct uiDrawBrushGradientStop uiDrawBrushGradientStop;

_UI_ENUM(uiDrawBrushType) {
    uiDrawBrushTypeSolid,
    uiDrawBrushTypeLinearGradient,
    uiDrawBrushTypeRadialGradient,
    uiDrawBrushTypeImage
};

_UI_ENUM(uiDrawLineCap) {
    uiDrawLineCapFlat,
    uiDrawLineCapRound,
    uiDrawLineCapSquare
};

_UI_ENUM(uiDrawLineJoin) {
    uiDrawLineJoinMiter,
    uiDrawLineJoinRound,
    uiDrawLineJoinBevel
};

#define uiDrawDefaultMiterLimit 10.0

_UI_ENUM(uiDrawFillMode) {
    uiDrawFillModeWinding,
    uiDrawFillModeAlternate
};

struct uiDrawMatrix {
    double M11;
    double M12;
    double M21;
    double M22;
    double M31;
    double M32;
};

struct uiDrawBrush {
    uiDrawBrushType Type;

    double R;
    double G;
    double B;
    double A;

    double X0;
    double Y0;
    double X1;
    double Y1;
    double OuterRadius;
    uiDrawBrushGradientStop *Stops;
    size_t NumStops;
};

struct uiDrawBrushGradientStop {
    double Pos;
    double R;
    double G;
    double B;
    double A;
};

struct uiDrawStrokeParams {
    uiDrawLineCap Cap;
    uiDrawLineJoin Join;
    double Thickness;
    double MiterLimit;
    double *Dashes;
    size_t NumDashes;
    double DashPhase;
};

_UI_EXTERN uiDrawPath *uiDrawNewPath(uiDrawFillMode fillMode);
_UI_EXTERN void uiDrawFreePath(uiDrawPath *p);

_UI_EXTERN void uiDrawPathNewFigure(uiDrawPath *p, double x, double y);
_UI_EXTERN void uiDrawPathNewFigureWithArc(uiDrawPath *p, double xCenter, double yCenter, double radius, double startAngle, double sweep, int negative);
_UI_EXTERN void uiDrawPathLineTo(uiDrawPath *p, double x, double y);
_UI_EXTERN void uiDrawPathArcTo(uiDrawPath *p, double xCenter, double yCenter, double radius, double startAngle, double sweep, int negative);
_UI_EXTERN void uiDrawPathBezierTo(uiDrawPath *p, double c1x, double c1y, double c2x, double c2y, double endX, double endY);
_UI_EXTERN void uiDrawPathCloseFigure(uiDrawPath *p);

_UI_EXTERN void uiDrawPathAddRectangle(uiDrawPath *p, double x, double y, double width, double height);

_UI_EXTERN void uiDrawPathEnd(uiDrawPath *p);

_UI_EXTERN void uiDrawStroke(uiDrawContext *c, uiDrawPath *path, uiDrawBrush *b, uiDrawStrokeParams *p);
_UI_EXTERN void uiDrawFill(uiDrawContext *c, uiDrawPath *path, uiDrawBrush *b);

_UI_EXTERN void uiDrawMatrixSetIdentity(uiDrawMatrix *m);
_UI_EXTERN void uiDrawMatrixTranslate(uiDrawMatrix *m, double x, double y);
_UI_EXTERN void uiDrawMatrixScale(uiDrawMatrix *m, double xCenter, double yCenter, double x, double y);
_UI_EXTERN void uiDrawMatrixRotate(uiDrawMatrix *m, double x, double y, double amount);
_UI_EXTERN void uiDrawMatrixSkew(uiDrawMatrix *m, double x, double y, double xamount, double yamount);
_UI_EXTERN void uiDrawMatrixMultiply(uiDrawMatrix *dest, uiDrawMatrix *src);
_UI_EXTERN int uiDrawMatrixInvertible(uiDrawMatrix *m);
_UI_EXTERN int uiDrawMatrixInvert(uiDrawMatrix *m);
_UI_EXTERN void uiDrawMatrixTransformPoint(uiDrawMatrix *m, double *x, double *y);
_UI_EXTERN void uiDrawMatrixTransformSize(uiDrawMatrix *m, double *x, double *y);

_UI_EXTERN void uiDrawTransform(uiDrawContext *c, uiDrawMatrix *m);

_UI_EXTERN void uiDrawClip(uiDrawContext *c, uiDrawPath *path);

_UI_EXTERN void uiDrawSave(uiDrawContext *c);
_UI_EXTERN void uiDrawRestore(uiDrawContext *c);

typedef struct uiAttribute uiAttribute;

_UI_EXTERN void uiFreeAttribute(uiAttribute *a);

_UI_ENUM(uiAttributeType) {
    uiAttributeTypeFamily,
    uiAttributeTypeSize,
    uiAttributeTypeWeight,
    uiAttributeTypeItalic,
    uiAttributeTypeStretch,
    uiAttributeTypeColor,
    uiAttributeTypeBackground,
    uiAttributeTypeUnderline,
    uiAttributeTypeUnderlineColor,
    uiAttributeTypeFeatures
};

_UI_EXTERN uiAttributeType uiAttributeGetType(const uiAttribute *a);

_UI_EXTERN uiAttribute *uiNewFamilyAttribute(const char *family);

_UI_EXTERN const char *uiAttributeFamily(const uiAttribute *a);

_UI_EXTERN uiAttribute *uiNewSizeAttribute(double size);

_UI_EXTERN double uiAttributeSize(const uiAttribute *a);

_UI_ENUM(uiTextWeight) {
    uiTextWeightMinimum = 0,
    uiTextWeightThin = 100,
    uiTextWeightUltraLight = 200,
    uiTextWeightLight = 300,
    uiTextWeightBook = 350,
    uiTextWeightNormal = 400,
    uiTextWeightMedium = 500,
    uiTextWeightSemiBold = 600,
    uiTextWeightBold = 700,
    uiTextWeightUltraBold = 800,
    uiTextWeightHeavy = 900,
    uiTextWeightUltraHeavy = 950,
    uiTextWeightMaximum = 1000
};

_UI_EXTERN uiAttribute *uiNewWeightAttribute(uiTextWeight weight);

_UI_EXTERN uiTextWeight uiAttributeWeight(const uiAttribute *a);

_UI_ENUM(uiTextItalic) {
    uiTextItalicNormal,
    uiTextItalicOblique,
    uiTextItalicItalic
};

_UI_EXTERN uiAttribute *uiNewItalicAttribute(uiTextItalic italic);

_UI_EXTERN uiTextItalic uiAttributeItalic(const uiAttribute *a);

_UI_ENUM(uiTextStretch) {
    uiTextStretchUltraCondensed,
    uiTextStretchExtraCondensed,
    uiTextStretchCondensed,
    uiTextStretchSemiCondensed,
    uiTextStretchNormal,
    uiTextStretchSemiExpanded,
    uiTextStretchExpanded,
    uiTextStretchExtraExpanded,
    uiTextStretchUltraExpanded
};

_UI_EXTERN uiAttribute *uiNewStretchAttribute(uiTextStretch stretch);

_UI_EXTERN uiTextStretch uiAttributeStretch(const uiAttribute *a);

_UI_EXTERN uiAttribute *uiNewColorAttribute(double r, double g, double b, double a);

_UI_EXTERN void uiAttributeColor(const uiAttribute *a, double *r, double *g, double *b, double *alpha);

_UI_EXTERN uiAttribute *uiNewBackgroundAttribute(double r, double g, double b, double a);

_UI_ENUM(uiUnderline) {
    uiUnderlineNone,
    uiUnderlineSingle,
    uiUnderlineDouble,
    uiUnderlineSuggestion
};

_UI_EXTERN uiAttribute *uiNewUnderlineAttribute(uiUnderline u);

_UI_EXTERN uiUnderline uiAttributeUnderline(const uiAttribute *a);

_UI_ENUM(uiUnderlineColor) {
    uiUnderlineColorCustom,
    uiUnderlineColorSpelling,
    uiUnderlineColorGrammar,
    uiUnderlineColorAuxiliary
};

_UI_EXTERN uiAttribute *uiNewUnderlineColorAttribute(uiUnderlineColor u, double r, double g, double b, double a);

_UI_EXTERN void uiAttributeUnderlineColor(const uiAttribute *a, uiUnderlineColor *u, double *r, double *g, double *b, double *alpha);

typedef struct uiOpenTypeFeatures uiOpenTypeFeatures;

typedef uiForEach (*uiOpenTypeFeaturesForEachFunc)(const uiOpenTypeFeatures *otf, char a, char b, char c, char d, uint32_t value, void *data);

_UI_EXTERN uiOpenTypeFeatures *uiNewOpenTypeFeatures(void);
_UI_EXTERN void uiFreeOpenTypeFeatures(uiOpenTypeFeatures *otf);
_UI_EXTERN uiOpenTypeFeatures *uiOpenTypeFeaturesClone(const uiOpenTypeFeatures *otf);
_UI_EXTERN void uiOpenTypeFeaturesAdd(uiOpenTypeFeatures *otf, char a, char b, char c, char d, uint32_t value);
_UI_EXTERN void uiOpenTypeFeaturesRemove(uiOpenTypeFeatures *otf, char a, char b, char c, char d);
_UI_EXTERN int uiOpenTypeFeaturesGet(const uiOpenTypeFeatures *otf, char a, char b, char c, char d, uint32_t *value);
_UI_EXTERN void uiOpenTypeFeaturesForEach(const uiOpenTypeFeatures *otf, uiOpenTypeFeaturesForEachFunc f, void *data);
_UI_EXTERN uiAttribute *uiNewFeaturesAttribute(const uiOpenTypeFeatures *otf);
_UI_EXTERN const uiOpenTypeFeatures *uiAttributeFeatures(const uiAttribute *a);

typedef struct uiAttributedString uiAttributedString;
typedef uiForEach (*uiAttributedStringForEachAttributeFunc)(const uiAttributedString *s, const uiAttribute *a, size_t start, size_t end, void *data);
_UI_EXTERN uiAttributedString *uiNewAttributedString(const char *initialString);
_UI_EXTERN void uiFreeAttributedString(uiAttributedString *s);
_UI_EXTERN const char *uiAttributedStringString(const uiAttributedString *s);
_UI_EXTERN size_t uiAttributedStringLen(const uiAttributedString *s);
_UI_EXTERN void uiAttributedStringAppendUnattributed(uiAttributedString *s, const char *str);
_UI_EXTERN void uiAttributedStringInsertAtUnattributed(uiAttributedString *s, const char *str, size_t at);
_UI_EXTERN void uiAttributedStringDelete(uiAttributedString *s, size_t start, size_t end);
_UI_EXTERN void uiAttributedStringSetAttribute(uiAttributedString *s, uiAttribute *a, size_t start, size_t end);
_UI_EXTERN void uiAttributedStringForEachAttribute(const uiAttributedString *s, uiAttributedStringForEachAttributeFunc f, void *data);
_UI_EXTERN size_t uiAttributedStringNumGraphemes(uiAttributedString *s);
_UI_EXTERN size_t uiAttributedStringByteIndexToGrapheme(uiAttributedString *s, size_t pos);
_UI_EXTERN size_t uiAttributedStringGraphemeToByteIndex(uiAttributedString *s, size_t pos);
typedef struct uiFontDescriptor uiFontDescriptor;

struct uiFontDescriptor {
    char *Family;
    double Size;
    uiTextWeight Weight;
    uiTextItalic Italic;
    uiTextStretch Stretch;
};
typedef struct uiDrawTextLayout uiDrawTextLayout;

_UI_ENUM(uiDrawTextAlign) {
    uiDrawTextAlignLeft,
    uiDrawTextAlignCenter,
    uiDrawTextAlignRight
};
typedef struct uiDrawTextLayoutParams uiDrawTextLayoutParams;

struct uiDrawTextLayoutParams {
    uiAttributedString *String;
    uiFontDescriptor *DefaultFont;
    double Width;
    uiDrawTextAlign Align;
};

_UI_EXTERN uiDrawTextLayout *uiDrawNewTextLayout(uiDrawTextLayoutParams *params);
_UI_EXTERN void uiDrawFreeTextLayout(uiDrawTextLayout *tl);
_UI_EXTERN void uiDrawText(uiDrawContext *c, uiDrawTextLayout *tl, double x, double y);
_UI_EXTERN void uiDrawTextLayoutExtents(uiDrawTextLayout *tl, double *width, double *height);
typedef struct uiFontButton uiFontButton;
#define uiFontButton(this) ((uiFontButton *) (this))
_UI_EXTERN void uiFontButtonFont(uiFontButton *b, uiFontDescriptor *desc);
_UI_EXTERN void uiFontButtonOnChanged(uiFontButton *b, void (*f)(uiFontButton *, void *), void *data);
_UI_EXTERN uiFontButton *uiNewFontButton(void);
_UI_EXTERN void uiFreeFontButtonFont(uiFontDescriptor *desc);

_UI_ENUM(uiModifiers) {
    uiModifierCtrl = 1 << 0,
    uiModifierAlt = 1 << 1,
    uiModifierShift = 1 << 2,
    uiModifierSuper = 1 << 3
};

struct uiAreaMouseEvent {
    double X;
    double Y;

    double AreaWidth;
    double AreaHeight;

    int Down;
    int Up;

    int Count;

    uiModifiers Modifiers;

    uint64_t Held1To64;
};

_UI_ENUM(uiExtKey) {
    uiExtKeyEscape = 1,
    uiExtKeyInsert,
    uiExtKeyDelete,
    uiExtKeyHome,
    uiExtKeyEnd,
    uiExtKeyPageUp,
    uiExtKeyPageDown,
    uiExtKeyUp,
    uiExtKeyDown,
    uiExtKeyLeft,
    uiExtKeyRight,
    uiExtKeyF1,
    uiExtKeyF2,
    uiExtKeyF3,
    uiExtKeyF4,
    uiExtKeyF5,
    uiExtKeyF6,
    uiExtKeyF7,
    uiExtKeyF8,
    uiExtKeyF9,
    uiExtKeyF10,
    uiExtKeyF11,
    uiExtKeyF12,
    uiExtKeyN0,
    uiExtKeyN1,
    uiExtKeyN2,
    uiExtKeyN3,
    uiExtKeyN4,
    uiExtKeyN5,
    uiExtKeyN6,
    uiExtKeyN7,
    uiExtKeyN8,
    uiExtKeyN9,
    uiExtKeyNDot,
    uiExtKeyNEnter,
    uiExtKeyNAdd,
    uiExtKeyNSubtract,
    uiExtKeyNMultiply,
    uiExtKeyNDivide
};

struct uiAreaKeyEvent {
    char Key;
    uiExtKey ExtKey;
    uiModifiers Modifier;

    uiModifiers Modifiers;

    int Up;
};

typedef struct uiColorButton uiColorButton;
#define uiColorButton(this) ((uiColorButton *) (this))
_UI_EXTERN void uiColorButtonColor(uiColorButton *b, double *r, double *g, double *bl, double *a);
_UI_EXTERN void uiColorButtonSetColor(uiColorButton *b, double r, double g, double bl, double a);
_UI_EXTERN void uiColorButtonOnChanged(uiColorButton *b, void (*f)(uiColorButton *, void *), void *data);
_UI_EXTERN uiColorButton *uiNewColorButton(void);

typedef struct uiForm uiForm;
#define uiForm(this) ((uiForm *) (this))
_UI_EXTERN void uiFormAppend(uiForm *f, const char *label, uiControl *c, int stretchy);
_UI_EXTERN void uiFormDelete(uiForm *f, int index);
_UI_EXTERN int uiFormPadded(uiForm *f);
_UI_EXTERN void uiFormSetPadded(uiForm *f, int padded);
_UI_EXTERN uiForm *uiNewForm(void);

_UI_ENUM(uiAlign) {
    uiAlignFill,
    uiAlignStart,
    uiAlignCenter,
    uiAlignEnd
};

_UI_ENUM(uiAt) {
    uiAtLeading,
    uiAtTop,
    uiAtTrailing,
    uiAtBottom
};

typedef struct uiGrid uiGrid;
#define uiGrid(this) ((uiGrid *) (this))
_UI_EXTERN void uiGridAppend(uiGrid *g, uiControl *c, int left, int top, int xspan, int yspan, int hexpand, uiAlign halign, int vexpand, uiAlign valign);
_UI_EXTERN void uiGridInsertAt(uiGrid *g, uiControl *c, uiControl *existing, uiAt at, int xspan, int yspan, int hexpand, uiAlign halign, int vexpand, uiAlign valign);
_UI_EXTERN int uiGridPadded(uiGrid *g);
_UI_EXTERN void uiGridSetPadded(uiGrid *g, int padded);
_UI_EXTERN uiGrid *uiNewGrid(void);
typedef struct uiImage uiImage;
_UI_EXTERN uiImage *uiNewImage(double width, double height);
_UI_EXTERN void uiFreeImage(uiImage *i);
_UI_EXTERN void uiImageAppend(uiImage *i, void *pixels, int pixelWidth, int pixelHeight, int byteStride);
typedef struct uiTableValue uiTableValue;
_UI_EXTERN void uiFreeTableValue(uiTableValue *v);
_UI_ENUM(uiTableValueType) {
    uiTableValueTypeString,
    uiTableValueTypeImage,
    uiTableValueTypeInt,
    uiTableValueTypeColor
};
_UI_EXTERN uiTableValueType uiTableValueGetType(const uiTableValue *v);
_UI_EXTERN uiTableValue *uiNewTableValueString(const char *str);
_UI_EXTERN const char *uiTableValueString(const uiTableValue *v);
_UI_EXTERN uiTableValue *uiNewTableValueImage(uiImage *img);
_UI_EXTERN uiImage *uiTableValueImage(const uiTableValue *v);
_UI_EXTERN uiTableValue *uiNewTableValueInt(int i);
_UI_EXTERN int uiTableValueInt(const uiTableValue *v);
_UI_EXTERN uiTableValue *uiNewTableValueColor(double r, double g, double b, double a);
_UI_EXTERN void uiTableValueColor(const uiTableValue *v, double *r, double *g, double *b, double *a);
typedef struct uiTableModel uiTableModel;
typedef struct uiTableModelHandler uiTableModelHandler;
struct uiTableModelHandler {
    int (*NumColumns)(uiTableModelHandler *, uiTableModel *);
    uiTableValueType (*ColumnType)(uiTableModelHandler *, uiTableModel *, int);
    int (*NumRows)(uiTableModelHandler *, uiTableModel *);
    uiTableValue *(*CellValue)(uiTableModelHandler *mh, uiTableModel *m, int row, int column);
    void (*SetCellValue)(uiTableModelHandler *, uiTableModel *, int, int, const uiTableValue *);
};
_UI_EXTERN uiTableModel *uiNewTableModel(uiTableModelHandler *mh);
_UI_EXTERN void uiFreeTableModel(uiTableModel *m);
_UI_EXTERN void uiTableModelRowInserted(uiTableModel *m, int newIndex);
_UI_EXTERN void uiTableModelRowChanged(uiTableModel *m, int index);
_UI_EXTERN void uiTableModelRowDeleted(uiTableModel *m, int oldIndex);

#define uiTableModelColumnNeverEditable (-1)
#define uiTableModelColumnAlwaysEditable (-2)
typedef struct uiTableTextColumnOptionalParams uiTableTextColumnOptionalParams;
typedef struct uiTableParams uiTableParams;

struct uiTableTextColumnOptionalParams {
    int ColorModelColumn;
};

struct uiTableParams {
    uiTableModel *Model;
    int RowBackgroundColorModelColumn;
};

typedef struct uiTable uiTable;
#define uiTable(this) ((uiTable *) (this))

_UI_EXTERN void uiTableAppendTextColumn(uiTable *t,
    const char *name,
    int textModelColumn,
    int textEditableModelColumn,
    uiTableTextColumnOptionalParams *textParams);
_UI_EXTERN void uiTableAppendImageColumn(uiTable *t,
    const char *name,
    int imageModelColumn);
_UI_EXTERN void uiTableAppendImageTextColumn(uiTable *t,
    const char *name,
    int imageModelColumn,
    int textModelColumn,
    int textEditableModelColumn,
    uiTableTextColumnOptionalParams *textParams);
_UI_EXTERN void uiTableAppendCheckboxColumn(uiTable *t,
    const char *name,
    int checkboxModelColumn,
    int checkboxEditableModelColumn);
_UI_EXTERN void uiTableAppendCheckboxTextColumn(uiTable *t,
    const char *name,
    int checkboxModelColumn,
    int checkboxEditableModelColumn,
    int textModelColumn,
    int textEditableModelColumn,
    uiTableTextColumnOptionalParams *textParams);
_UI_EXTERN void uiTableAppendProgressBarColumn(uiTable *t,
    const char *name,
    int progressModelColumn);
_UI_EXTERN void uiTableAppendButtonColumn(uiTable *t,
    const char *name,
    int buttonModelColumn,
    int buttonClickableModelColumn);
_UI_EXTERN uiTable *uiNewTable(uiTableParams *params);

#ifdef __cplusplus
}
#endif

#endif
