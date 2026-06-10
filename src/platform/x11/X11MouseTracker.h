#ifndef X11MOUSETRACKER_H
#define X11MOUSETRACKER_H

#include "../IMouseTracker.h"

#include <QThread>
#include <QWindow>

class X11MouseTracker : public IMouseTracker {
    Q_OBJECT

public:
    explicit X11MouseTracker(WId windowId, double sensibility, QObject *parent = nullptr);
    ~X11MouseTracker() override;

    void start() override;
    void stop() override;
    void setMouseSensibility(double s) override;

private:
    class Worker;
    friend class Worker;

    Worker *m_worker = nullptr;
    WId m_windowId = 0;
    double m_sensibility = 1.0;
};

#endif // X11MOUSETRACKER_H
