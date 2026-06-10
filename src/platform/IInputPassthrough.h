#ifndef IINPUTPASSTHROUGH_H
#define IINPUTPASSTHROUGH_H

class QWidget;

class IInputPassthrough {
public:
    virtual ~IInputPassthrough() = default;

    virtual void enablePassthrough(QWidget *widget) = 0;
    virtual void disablePassthrough(QWidget *widget) = 0;
};

#endif // IINPUTPASSTHROUGH_H
