#ifndef MACINPUTPASSTHROUGH_H
#define MACINPUTPASSTHROUGH_H

#include "../IInputPassthrough.h"

class MacInputPassthrough : public IInputPassthrough {
public:
    MacInputPassthrough() = default;
    ~MacInputPassthrough() override = default;

    void enablePassthrough(QWidget *widget) override;
    void disablePassthrough(QWidget *widget) override;
};

#endif // MACINPUTPASSTHROUGH_H
