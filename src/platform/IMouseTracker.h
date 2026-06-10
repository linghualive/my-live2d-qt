#ifndef IMOUSETRACKER_H
#define IMOUSETRACKER_H

#include <QObject>
#include <QPoint>

class IMouseTracker : public QObject {
    Q_OBJECT

public:
    virtual ~IMouseTracker() override = default;

    virtual void start() = 0;
    virtual void stop() = 0;
    virtual void setMouseSensibility(double s) = 0;

signals:
    void mouseMoved(QPoint relPosition, QPoint rawPosition);
    void mousePressed(QPoint relPosition, QPoint rawPosition);
    void mouseReleased(QPoint relPosition, QPoint rawPosition);

protected:
    explicit IMouseTracker(QObject *parent = nullptr)
        : QObject(parent) {}
};

#endif // IMOUSETRACKER_H
