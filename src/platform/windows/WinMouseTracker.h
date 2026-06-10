#ifndef WINMOUSETRACKER_H
#define WINMOUSETRACKER_H

#include "../IMouseTracker.h"

#include <QThread>
#include <QWindow>

class WinMouseTracker : public IMouseTracker {
    Q_OBJECT

public:
    explicit WinMouseTracker(WId windowId, double sensibility, QObject *parent = nullptr);
    ~WinMouseTracker() override;

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

#endif // WINMOUSETRACKER_H
