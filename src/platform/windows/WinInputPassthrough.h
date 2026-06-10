#ifndef WININPUTPASSTHROUGH_H
#define WININPUTPASSTHROUGH_H

#include "../IInputPassthrough.h"

class WinInputPassthrough : public IInputPassthrough {
public:
    WinInputPassthrough() = default;
    ~WinInputPassthrough() override = default;

    void enablePassthrough(QWidget *widget) override;
    void disablePassthrough(QWidget *widget) override;
};

#endif // WININPUTPASSTHROUGH_H
