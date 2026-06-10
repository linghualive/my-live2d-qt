#ifndef PLATFORMFACTORY_H
#define PLATFORMFACTORY_H

#include <memory>
#include <QString>
#include <QWindow>

class IMouseTracker;
class IInputPassthrough;
class QObject;

class PlatformFactory {
public:
    PlatformFactory() = delete;

    static std::unique_ptr<IMouseTracker> createMouseTracker(WId windowId,
                                                              double sensibility,
                                                              QObject *parent = nullptr);

    static std::unique_ptr<IInputPassthrough> createInputPassthrough();

    static QString currentPlatform();
};

#endif // PLATFORMFACTORY_H
