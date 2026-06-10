#include "WaylandMouseTracker.h"

#include <QCursor>
#include <QGuiApplication>
#include <QWindow>
#include <QWidget>

WaylandMouseTracker::WaylandMouseTracker(WId windowId, double sensibility, QObject *parent)
    : IMouseTracker(parent)
    , m_windowId(windowId)
    , m_sensibility(sensibility)
{
    m_pollTimer.setInterval(16); // ~60 fps
    connect(&m_pollTimer, &QTimer::timeout, this, &WaylandMouseTracker::pollCursorPosition);
}

WaylandMouseTracker::~WaylandMouseTracker() {
    stop();
}

void WaylandMouseTracker::start() {
    m_lastPosition = QCursor::pos();
    m_pollTimer.start();
}

void WaylandMouseTracker::stop() {
    m_pollTimer.stop();
}

void WaylandMouseTracker::setMouseSensibility(double s) {
    m_sensibility = s;
}

void WaylandMouseTracker::pollCursorPosition() {
    QPoint globalPos = QCursor::pos();

    // Find the window from the stored WId to compute relative coordinates
    QPoint relPos = globalPos;
    QWindow *window = nullptr;

    const auto topLevels = QGuiApplication::topLevelWindows();
    for (QWindow *w : topLevels) {
        if (w->winId() == m_windowId) {
            window = w;
            break;
        }
    }

    if (window) {
        QPoint windowTopLeft = window->position();
        relPos = globalPos - windowTopLeft;
    }

    // Detect button state changes via Qt's button state query
    Qt::MouseButtons buttons = QGuiApplication::mouseButtons();
    bool leftDown = buttons.testFlag(Qt::LeftButton);

    if (leftDown && !m_leftButtonDown) {
        m_leftButtonDown = true;
        emit mousePressed(
            QPoint(static_cast<int>(relPos.x() * m_sensibility),
                   static_cast<int>(relPos.y() * m_sensibility)),
            globalPos);
    } else if (!leftDown && m_leftButtonDown) {
        m_leftButtonDown = false;
        emit mouseReleased(
            QPoint(static_cast<int>(relPos.x() * m_sensibility),
                   static_cast<int>(relPos.y() * m_sensibility)),
            globalPos);
    }

    if (globalPos != m_lastPosition) {
        m_lastPosition = globalPos;
        emit mouseMoved(
            QPoint(static_cast<int>(relPos.x() * m_sensibility),
                   static_cast<int>(relPos.y() * m_sensibility)),
            globalPos);
    }
}
