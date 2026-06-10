#ifndef WAYLANDINPUTPASSTHROUGH_H
#define WAYLANDINPUTPASSTHROUGH_H

#include "../IInputPassthrough.h"

class WaylandInputPassthrough : public IInputPassthrough {
public:
    WaylandInputPassthrough() = default;
    ~WaylandInputPassthrough() override = default;

    void enablePassthrough(QWidget *widget) override;
    void disablePassthrough(QWidget *widget) override;
};

#endif // WAYLANDINPUTPASSTHROUGH_H
