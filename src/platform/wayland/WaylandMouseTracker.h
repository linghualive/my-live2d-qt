#ifndef WAYLANDMOUSETRACKER_H
#define WAYLANDMOUSETRACKER_H

#include "../IMouseTracker.h"

#include <QTimer>
#include <QWindow>

class WaylandMouseTracker : public IMouseTracker {
    Q_OBJECT

public:
    explicit WaylandMouseTracker(WId windowId, double sensibility, QObject *parent = nullptr);
    ~WaylandMouseTracker() override;

    void start() override;
    void stop() override;
    void setMouseSensibility(double s) override;

private slots:
    void pollCursorPosition();

private:
    QTimer m_pollTimer;
    WId m_windowId = 0;
    double m_sensibility = 1.0;
    QPoint m_lastPosition;
    bool m_leftButtonDown = false;
};

#endif // WAYLANDMOUSETRACKER_H
