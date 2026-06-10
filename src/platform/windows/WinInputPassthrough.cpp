#include "WinInputPassthrough.h"

#include <QWidget>
#include <windows.h>

void WinInputPassthrough::enablePassthrough(QWidget *widget) {
    if (!widget) return;

    HWND hwnd = reinterpret_cast<HWND>(widget->winId());
    LONG_PTR exStyle = GetWindowLongPtrW(hwnd, GWL_EXSTYLE);
    SetWindowLongPtrW(hwnd, GWL_EXSTYLE,
                      exStyle | WS_EX_TRANSPARENT | WS_EX_LAYERED);
}

void WinInputPassthrough::disablePassthrough(QWidget *widget) {
    if (!widget) return;

    HWND hwnd = reinterpret_cast<HWND>(widget->winId());
    LONG_PTR exStyle = GetWindowLongPtrW(hwnd, GWL_EXSTYLE);
    SetWindowLongPtrW(hwnd, GWL_EXSTYLE,
                      exStyle & ~WS_EX_TRANSPARENT);
}
