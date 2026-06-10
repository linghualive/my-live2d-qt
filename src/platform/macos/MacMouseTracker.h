#ifndef MACMOUSETRACKER_H
#define MACMOUSETRACKER_H

#include "../IMouseTracker.h"

#include <QThread>
#include <QWindow>

class MacMouseTracker : public IMouseTracker {
    Q_OBJECT

public:
    explicit MacMouseTracker(WId windowId, double sensibility, QObject *parent = nullptr);
    ~MacMouseTracker() override;

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

#endif // MACMOUSETRACKER_H
