#include "WinMouseTracker.h"

#include <QDebug>
#include <QGuiApplication>
#include <QWindow>

#include <windows.h>

class WinMouseTracker::Worker : public QThread {
    Q_OBJECT

public:
    Worker(WinMouseTracker *tracker, WId windowId)
        : QThread(tracker)
        , m_tracker(tracker)
        , m_windowId(windowId)
    {
    }

    ~Worker() override {
        stopHook();
        quit();
        wait(3000);
        if (isRunning()) {
            terminate();
            wait();
        }
    }

    void stopHook() {
        if (m_threadId != 0) {
            PostThreadMessageW(m_threadId, WM_QUIT, 0, 0);
        }
    }

protected:
    void run() override {
        m_threadId = GetCurrentThreadId();
        s_instance = this;

        m_hook = SetWindowsHookExW(WH_MOUSE_LL, mouseProc, nullptr, 0);
        if (!m_hook) {
            qWarning("WinMouseTracker: failed to install mouse hook (error %lu)",
                     GetLastError());
            s_instance = nullptr;
            return;
        }

        MSG msg;
        while (GetMessageW(&msg, nullptr, 0, 0) > 0) {
            TranslateMessage(&msg);
            DispatchMessageW(&msg);
        }

        UnhookWindowsHookEx(m_hook);
        m_hook = nullptr;
        s_instance = nullptr;
        m_threadId = 0;
    }

private:
    static LRESULT CALLBACK mouseProc(int nCode, WPARAM wParam, LPARAM lParam) {
        if (nCode >= 0 && s_instance) {
            auto *ms = reinterpret_cast<MSLLHOOKSTRUCT *>(lParam);
            QPoint globalPos(ms->pt.x, ms->pt.y);

            QPoint relPos = globalPos;
            QWindow *window = nullptr;

            const auto topLevels = QGuiApplication::topLevelWindows();
            for (QWindow *w : topLevels) {
                if (w->winId() == s_instance->m_windowId) {
                    window = w;
                    break;
                }
            }

            if (window) {
                QPoint windowTopLeft = window->position();
                relPos = globalPos - windowTopLeft;
            }

            double sens = s_instance->m_tracker->m_sensibility;
            QPoint scaledPos(static_cast<int>(relPos.x() * sens),
                             static_cast<int>(relPos.y() * sens));

            switch (wParam) {
            case WM_MOUSEMOVE:
                emit s_instance->m_tracker->mouseMoved(scaledPos, globalPos);
                break;
            case WM_LBUTTONDOWN:
                emit s_instance->m_tracker->mousePressed(scaledPos, globalPos);
                break;
            case WM_LBUTTONUP:
                emit s_instance->m_tracker->mouseReleased(scaledPos, globalPos);
                break;
            default:
                break;
            }
        }
        return CallNextHookEx(nullptr, nCode, wParam, lParam);
    }

    static thread_local Worker *s_instance;

    WinMouseTracker *m_tracker = nullptr;
    WId m_windowId = 0;
    HHOOK m_hook = nullptr;
    DWORD m_threadId = 0;
};

thread_local WinMouseTracker::Worker *WinMouseTracker::Worker::s_instance = nullptr;

WinMouseTracker::WinMouseTracker(WId windowId, double sensibility, QObject *parent)
    : IMouseTracker(parent)
    , m_windowId(windowId)
    , m_sensibility(sensibility)
{
}

WinMouseTracker::~WinMouseTracker() {
    stop();
}

void WinMouseTracker::start() {
    if (m_worker) return;

    m_worker = new Worker(this, m_windowId);
    m_worker->start();
}

void WinMouseTracker::stop() {
    if (!m_worker) return;

    m_worker->stopHook();
    m_worker->quit();
    m_worker->wait(3000);
    if (m_worker->isRunning()) {
        m_worker->terminate();
        m_worker->wait();
    }
    delete m_worker;
    m_worker = nullptr;
}

void WinMouseTracker::setMouseSensibility(double s) {
    m_sensibility = s;
}

#include "WinMouseTracker.moc"
