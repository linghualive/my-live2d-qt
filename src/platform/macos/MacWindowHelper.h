#ifndef MACWINDOWHELPER_H
#define MACWINDOWHELPER_H

#include <QWidget>

namespace MacWindowHelper {
    void makeFullyTransparent(QWidget *window);
    void setIgnoresMouseEvents(QWidget *window, bool ignores);
}

#endif
