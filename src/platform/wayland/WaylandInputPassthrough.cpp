#include "WaylandInputPassthrough.h"

#include <QWidget>

void WaylandInputPassthrough::enablePassthrough(QWidget *widget) {
    if (!widget) return;
    widget->setAttribute(Qt::WA_TransparentForMouseEvents, true);
}

void WaylandInputPassthrough::disablePassthrough(QWidget *widget) {
    if (!widget) return;
    widget->setAttribute(Qt::WA_TransparentForMouseEvents, false);
}
