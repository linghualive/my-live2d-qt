#include "X11MouseTracker.h"

#include <QDebug>

#include <X11/X.h>
#include <X11/Xlibint.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/XKBlib.h>
#include <X11/extensions/record.h>
#include <X11/extensions/XTest.h>

// Undef X11 macros that clash with Qt
#undef Bool
#undef CursorShape
#undef Expose
#undef KeyPress
#undef KeyRelease
#undef FocusIn
#undef FocusOut
#undef FontChange
#undef None
#undef Status
#undef Unsorted

typedef union {
    unsigned char    type;
    xEvent           event;
    xResourceReq     req;
    xGenericReply    reply;
    xError           error;
    xConnSetupPrefix setup;
} XRecordDatum;

// X11 event type constants (re-defined after undef above)
static constexpr int X11_KeyPress      = 2;
static constexpr int X11_KeyRelease    = 3;
static constexpr int X11_ButtonPress   = 4;
static constexpr int X11_ButtonRelease = 5;
static constexpr int X11_MotionNotify  = 6;

// ---------------------------------------------------------------------------
// Worker thread -- runs the XRecord blocking event loop on its own connection
// ---------------------------------------------------------------------------

class X11MouseTracker::Worker : public QThread {
    Q_OBJECT

public:
    Worker(X11MouseTracker *tracker, WId windowId)
        : QThread(tracker)
        , m_tracker(tracker)
        , m_appWindow(static_cast<Window>(windowId))
    {
        m_controlDisplay = XOpenDisplay(nullptr);
        m_dataDisplay    = XOpenDisplay(nullptr);

        if (!m_controlDisplay || !m_dataDisplay) {
            qCritical("X11MouseTracker::Worker: cannot open X display");
            return;
        }

        XSynchronize(m_controlDisplay, True);

        m_rootWindow = XRootWindow(m_controlDisplay, 0);

        int major = 0, minor = 0;
        if (!XRecordQueryVersion(m_controlDisplay, &major, &minor)) {
            qCritical("X11MouseTracker::Worker: RECORD extension not supported");
            return;
        }
        qInfo("X11 RECORD extension version: %d.%d", major, minor);

        m_valid = true;
    }

    ~Worker() override {
        cleanup();
    }

    bool isValid() const { return m_valid; }

    int queryCursor(int &relX, int &relY, int &absX, int &absY) {
        if (!m_controlDisplay) return 1;

        Window root_return, child_return;
        int root_x, root_y, win_x, win_y;
        unsigned int mask;

        int retval = XQueryPointer(m_controlDisplay, m_rootWindow,
                                   &root_return, &child_return,
                                   &root_x, &root_y,
                                   &win_x, &win_y,
                                   &mask);
        if (retval != True) {
            return 1;
        }

        int local_x = 0, local_y = 0;
        XTranslateCoordinates(m_controlDisplay, m_rootWindow, m_appWindow,
                              root_x, root_y,
                              &local_x, &local_y, &child_return);
        relX = local_x;
        relY = local_y;
        absX = root_x;
        absY = root_y;
        return 0;
    }

protected:
    void run() override {
        if (!m_valid) return;

        XRecordRange *rr = XRecordAllocRange();
        if (!rr) {
            qCritical("X11MouseTracker::Worker: could not allocate record range");
            return;
        }

        rr->device_events.first = X11_KeyPress;
        rr->device_events.last  = X11_MotionNotify;

        XRecordClientSpec rcs = XRecordAllClients;
        m_ctx = XRecordCreateContext(m_controlDisplay, 0, &rcs, 1, &rr, 1);
        if (!m_ctx) {
            XFree(rr);
            qCritical("X11MouseTracker::Worker: could not create record context");
            return;
        }

        // This call blocks until the context is disabled
        XRecordEnableContext(m_dataDisplay, m_ctx, xrecordCallback,
                             reinterpret_cast<XPointer>(this));

        XFree(rr);
    }

private:
    static void xrecordCallback(XPointer closure, XRecordInterceptData *hook) {
        auto *self = reinterpret_cast<Worker *>(closure);
        self->processEvent(hook);
    }

    void processEvent(XRecordInterceptData *hook) {
        if (hook->category != XRecordFromServer) {
            XRecordFreeData(hook);
            return;
        }

        auto *data = reinterpret_cast<XRecordDatum *>(hook->data);
        unsigned char buttonCode = data->event.u.u.detail;
        int absX = 0, absY = 0, relX = 0, relY = 0;
        int eventType = data->type;

        double sens = m_tracker->m_sensibility;

        switch (eventType) {
        case X11_ButtonPress:
            if (buttonCode == 1) {
                if (queryCursor(relX, relY, absX, absY) == 0) {
                    emit m_tracker->mousePressed(
                        QPoint(static_cast<int>(relX * sens),
                               static_cast<int>(relY * sens)),
                        QPoint(absX, absY));
                }
            }
            break;

        case X11_ButtonRelease:
            if (buttonCode == 1) {
                if (queryCursor(relX, relY, absX, absY) == 0) {
                    emit m_tracker->mouseReleased(
                        QPoint(static_cast<int>(relX * sens),
                               static_cast<int>(relY * sens)),
                        QPoint(absX, absY));
                }
            }
            break;

        case X11_MotionNotify:
            if (queryCursor(relX, relY, absX, absY) == 0) {
                emit m_tracker->mouseMoved(
                    QPoint(static_cast<int>(relX * sens),
                           static_cast<int>(relY * sens)),
                    QPoint(absX, absY));
            }
            break;

        default:
            break;
        }

        XRecordFreeData(hook);
    }

    void cleanup() {
        if (m_ctx && m_controlDisplay) {
            XRecordDisableContext(m_controlDisplay, m_ctx);
            XRecordFreeContext(m_controlDisplay, m_ctx);
            m_ctx = 0;
        }
        if (m_controlDisplay) {
            XCloseDisplay(m_controlDisplay);
            m_controlDisplay = nullptr;
        }
        if (m_dataDisplay) {
            XCloseDisplay(m_dataDisplay);
            m_dataDisplay = nullptr;
        }
    }

    X11MouseTracker *m_tracker = nullptr;
    Display *m_controlDisplay = nullptr;
    Display *m_dataDisplay = nullptr;
    XRecordContext m_ctx = 0;
    Window m_rootWindow = 0;
    Window m_appWindow = 0;
    bool m_valid = false;
};

// ---------------------------------------------------------------------------
// X11MouseTracker
// ---------------------------------------------------------------------------

X11MouseTracker::X11MouseTracker(WId windowId, double sensibility, QObject *parent)
    : IMouseTracker(parent)
    , m_windowId(windowId)
    , m_sensibility(sensibility)
{
}

X11MouseTracker::~X11MouseTracker() {
    stop();
}

void X11MouseTracker::start() {
    if (m_worker) return;

    m_worker = new Worker(this, m_windowId);
    if (!m_worker->isValid()) {
        delete m_worker;
        m_worker = nullptr;
        qWarning("X11MouseTracker::start: worker initialization failed");
        return;
    }
    m_worker->start();
}

void X11MouseTracker::stop() {
    if (!m_worker) return;

    // XRecordDisableContext will cause XRecordEnableContext to return,
    // which allows the thread's run() to finish.
    m_worker->requestInterruption();
    m_worker->quit();
    m_worker->wait(3000);
    if (m_worker->isRunning()) {
        m_worker->terminate();
        m_worker->wait();
    }
    delete m_worker;
    m_worker = nullptr;
}

void X11MouseTracker::setMouseSensibility(double s) {
    m_sensibility = s;
}

// Include the moc file for the nested Worker class defined in this .cpp
#include "X11MouseTracker.moc"
