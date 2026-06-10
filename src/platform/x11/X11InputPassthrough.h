#ifndef X11INPUTPASSTHROUGH_H
#define X11INPUTPASSTHROUGH_H

#include "../IInputPassthrough.h"

class X11InputPassthrough : public IInputPassthrough {
public:
    X11InputPassthrough() = default;
    ~X11InputPassthrough() override = default;

    void enablePassthrough(QWidget *widget) override;
    void disablePassthrough(QWidget *widget) override;
};

#endif // X11INPUTPASSTHROUGH_H
