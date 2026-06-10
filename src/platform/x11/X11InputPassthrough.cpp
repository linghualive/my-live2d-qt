#include "X11InputPassthrough.h"

#include <QWidget>
#include <QGuiApplication>

#include <X11/Xlib.h>
#include <X11/extensions/shape.h>

// Undef X11 macros that clash with Qt
#undef Bool
#undef CursorShape
#undef Expose
#undef KeyPress
#undef KeyRelease
#undef FocusIn
#undef FocusOut
#undef FontChange
#undef None
#undef Status
#undef Unsorted

#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
#include <QNativeInterface>
#else
#include <QX11Info>
#endif

static Display *getX11Display() {
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    auto *x11App = qApp->nativeInterface<QNativeInterface::QX11Application>();
    return x11App ? x11App->display() : nullptr;
#else
    return QX11Info::display();
#endif
}

void X11InputPassthrough::enablePassthrough(QWidget *widget) {
    if (!widget) return;

    Display *dpy = getX11Display();
    if (!dpy) {
        qWarning("X11InputPassthrough: not running on X11 platform");
        return;
    }

    XShapeCombineRectangles(dpy, widget->winId(), ShapeInput,
                            0, 0, nullptr, 0, ShapeSet, YXBanded);
}

void X11InputPassthrough::disablePassthrough(QWidget *widget) {
    if (!widget) return;

    Display *dpy = getX11Display();
    if (!dpy) return;

    XShapeCombineMask(dpy, widget->winId(), ShapeInput, 0, 0, 0 /*None*/, ShapeSet);
}
