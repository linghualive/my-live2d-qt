#include "MacMouseTracker.h"

#include <QDebug>
#include <QGuiApplication>
#include <QWindow>

#import <CoreGraphics/CoreGraphics.h>
#import <Foundation/Foundation.h>

// ---------------------------------------------------------------------------
// Worker thread -- runs a CFRunLoop with a CGEventTap
// ---------------------------------------------------------------------------

class MacMouseTracker::Worker : public QThread {
    Q_OBJECT

public:
    Worker(MacMouseTracker *tracker, WId windowId)
        : QThread(tracker)
        , m_tracker(tracker)
        , m_windowId(windowId)
    {
    }

    ~Worker() override {
        stopTap();
        quit();
        wait(3000);
        if (isRunning()) {
            terminate();
            wait();
        }
    }

    void stopTap() {
        if (m_runLoop) {
            CFRunLoopStop(m_runLoop);
            m_runLoop = nullptr;
        }
        if (m_eventTap) {
            CGEventTapEnable(m_eventTap, false);
            CFRelease(m_eventTap);
            m_eventTap = nullptr;
        }
        if (m_runLoopSource) {
            CFRelease(m_runLoopSource);
            m_runLoopSource = nullptr;
        }
    }

protected:
    void run() override {
        CGEventMask eventMask =
            CGEventMaskBit(kCGEventMouseMoved) |
            CGEventMaskBit(kCGEventLeftMouseDown) |
            CGEventMaskBit(kCGEventLeftMouseUp) |
            CGEventMaskBit(kCGEventLeftMouseDragged);

        m_eventTap = CGEventTapCreate(
            kCGHIDEventTap,
            kCGHeadInsertEventTap,
            kCGEventTapOptionListenOnly,
            eventMask,
            eventTapCallback,
            this);

        if (!m_eventTap) {
            qWarning("MacMouseTracker: failed to create event tap. "
                     "Accessibility permissions may be required.");
            return;
        }

        m_runLoopSource = CFMachPortCreateRunLoopSource(
            kCFAllocatorDefault, m_eventTap, 0);

        if (!m_runLoopSource) {
            qWarning("MacMouseTracker: failed to create run loop source");
            CFRelease(m_eventTap);
            m_eventTap = nullptr;
            return;
        }

        m_runLoop = CFRunLoopGetCurrent();

        CFRunLoopAddSource(m_runLoop, m_runLoopSource, kCFRunLoopCommonModes);
        CGEventTapEnable(m_eventTap, true);

        // This blocks until CFRunLoopStop is called
        CFRunLoopRun();
    }

private:
    static CGEventRef eventTapCallback(CGEventTapProxy /*proxy*/,
                                       CGEventType type,
                                       CGEventRef event,
                                       void *userInfo) {
        auto *self = static_cast<Worker *>(userInfo);

        // Handle tap being disabled by the system (e.g., timeout)
        if (type == kCGEventTapDisabledByTimeout ||
            type == kCGEventTapDisabledByUserInput) {
            if (self->m_eventTap) {
                CGEventTapEnable(self->m_eventTap, true);
            }
            return event;
        }

        CGPoint location = CGEventGetLocation(event);
        QPoint globalPos(static_cast<int>(location.x),
                         static_cast<int>(location.y));

        // Compute relative position to the target window
        QPoint relPos = globalPos;
        QWindow *window = nullptr;

        const auto topLevels = QGuiApplication::topLevelWindows();
        for (QWindow *w : topLevels) {
            if (w->winId() == self->m_windowId) {
                window = w;
                break;
            }
        }

        if (window) {
            QPoint windowTopLeft = window->position();
            relPos = globalPos - windowTopLeft;
        }

        double sens = self->m_tracker->m_sensibility;
        QPoint scaledPos(static_cast<int>(relPos.x() * sens),
                         static_cast<int>(relPos.y() * sens));

        switch (type) {
        case kCGEventMouseMoved:
        case kCGEventLeftMouseDragged:
            emit self->m_tracker->mouseMoved(scaledPos, globalPos);
            break;

        case kCGEventLeftMouseDown:
            emit self->m_tracker->mousePressed(scaledPos, globalPos);
            break;

        case kCGEventLeftMouseUp:
            emit self->m_tracker->mouseReleased(scaledPos, globalPos);
            break;

        default:
            break;
        }

        return event;
    }

    MacMouseTracker *m_tracker = nullptr;
    WId m_windowId = 0;
    CFMachPortRef m_eventTap = nullptr;
    CFRunLoopSourceRef m_runLoopSource = nullptr;
    CFRunLoopRef m_runLoop = nullptr;
};

// ---------------------------------------------------------------------------
// MacMouseTracker
// ---------------------------------------------------------------------------

MacMouseTracker::MacMouseTracker(WId windowId, double sensibility, QObject *parent)
    : IMouseTracker(parent)
    , m_windowId(windowId)
    , m_sensibility(sensibility)
{
}

MacMouseTracker::~MacMouseTracker() {
    stop();
}

void MacMouseTracker::start() {
    if (m_worker) return;

    m_worker = new Worker(this, m_windowId);
    m_worker->start();
}

void MacMouseTracker::stop() {
    if (!m_worker) return;

    m_worker->stopTap();
    m_worker->quit();
    m_worker->wait(3000);
    if (m_worker->isRunning()) {
        m_worker->terminate();
        m_worker->wait();
    }
    delete m_worker;
    m_worker = nullptr;
}

void MacMouseTracker::setMouseSensibility(double s) {
    m_sensibility = s;
}

#include "MacMouseTracker.moc"
