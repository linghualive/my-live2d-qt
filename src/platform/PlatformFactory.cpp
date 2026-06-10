#include "PlatformFactory.h"

#include <QGuiApplication>
#include <QString>
#include <QDebug>

#include "IMouseTracker.h"
#include "IInputPassthrough.h"

#ifdef HAS_X11
#include "x11/X11MouseTracker.h"
#include "x11/X11InputPassthrough.h"
#endif

#ifdef HAS_WAYLAND
#include "wayland/WaylandMouseTracker.h"
#include "wayland/WaylandInputPassthrough.h"
#endif

#ifdef HAS_MACOS
#include "macos/MacMouseTracker.h"
#include "macos/MacInputPassthrough.h"
#endif

std::unique_ptr<IMouseTracker> PlatformFactory::createMouseTracker(WId windowId,
                                                                    double sensibility,
                                                                    QObject *parent) {
    const QString platform = QGuiApplication::platformName();

#ifdef HAS_X11
    if (platform == QStringLiteral("xcb")) {
        return std::make_unique<X11MouseTracker>(windowId, sensibility, parent);
    }
#endif

#ifdef HAS_WAYLAND
    if (platform == QStringLiteral("wayland")) {
        return std::make_unique<WaylandMouseTracker>(windowId, sensibility, parent);
    }
#endif

#ifdef HAS_MACOS
    if (platform == QStringLiteral("cocoa")) {
        return std::make_unique<MacMouseTracker>(windowId, sensibility, parent);
    }
#endif

    qWarning() << "PlatformFactory: unsupported platform" << platform
               << "- mouse tracker not available";
    return nullptr;
}

std::unique_ptr<IInputPassthrough> PlatformFactory::createInputPassthrough() {
    const QString platform = QGuiApplication::platformName();

#ifdef HAS_X11
    if (platform == QStringLiteral("xcb")) {
        return std::make_unique<X11InputPassthrough>();
    }
#endif

#ifdef HAS_WAYLAND
    if (platform == QStringLiteral("wayland")) {
        return std::make_unique<WaylandInputPassthrough>();
    }
#endif

#ifdef HAS_MACOS
    if (platform == QStringLiteral("cocoa")) {
        return std::make_unique<MacInputPassthrough>();
    }
#endif

    qWarning() << "PlatformFactory: unsupported platform" << platform
               << "- input passthrough not available";
    return nullptr;
}

QString PlatformFactory::currentPlatform() {
    return QGuiApplication::platformName();
}
