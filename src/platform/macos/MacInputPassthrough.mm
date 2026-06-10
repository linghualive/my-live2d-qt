#include "MacInputPassthrough.h"

#include <QWidget>

void MacInputPassthrough::enablePassthrough(QWidget *widget) {
    if (!widget) return;
    widget->setAttribute(Qt::WA_TransparentForMouseEvents, true);
}

void MacInputPassthrough::disablePassthrough(QWidget *widget) {
    if (!widget) return;
    widget->setAttribute(Qt::WA_TransparentForMouseEvents, false);
}
