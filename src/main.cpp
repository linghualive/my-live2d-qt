#include <QApplication>
#include <QDir>
#include <QMessageBox>
#include <QSurfaceFormat>
#include <memory>

#include "core/ProcessLock.h"
#include "core/Configuration.h"
#include "core/ModelManager.h"
#include "ui/PetWindow.h"

static QString findBundledResourcesDir()
{
    QDir appDir(QCoreApplication::applicationDirPath());
    if (appDir.exists(QStringLiteral("Resources"))) {
        return appDir.absoluteFilePath(QStringLiteral("Resources"));
    }
#ifdef Q_OS_MACOS
    QDir macOSDir(appDir);
    macOSDir.cdUp();
    if (macOSDir.exists(QStringLiteral("Resources"))) {
        return macOSDir.absoluteFilePath(QStringLiteral("Resources"));
    }
#endif
    return {};
}

int main(int argc, char *argv[])
{
    QSurfaceFormat fmt;
    fmt.setAlphaBufferSize(8);
    fmt.setDepthBufferSize(24);
    fmt.setStencilBufferSize(8);
    QSurfaceFormat::setDefaultFormat(fmt);

    QApplication app(argc, argv);
    app.setQuitOnLastWindowClosed(false);
    QCoreApplication::setOrganizationName(QStringLiteral("QDesktopPet"));
    QCoreApplication::setApplicationName(QStringLiteral("QDesktopPet"));

    ProcessLock lock;
    if (!lock.tryLock()) {
        QMessageBox::warning(nullptr, QStringLiteral("QDesktopPet"),
                             QStringLiteral("Another instance is already running."));
        return 1;
    }

    auto config = std::make_unique<Configuration>();
    auto modelManager = std::make_unique<ModelManager>();
    modelManager->scanModels();

    // Auto-import bundled models that haven't been imported yet
    const QString bundledDir = findBundledResourcesDir();
    if (!bundledDir.isEmpty()) {
        QDir resDir(bundledDir);
        const QStringList subdirs = resDir.entryList(QDir::Dirs | QDir::NoDotAndDotDot);
        for (const QString &subdir : subdirs) {
            if (modelManager->modelInfo(subdir).id.isEmpty()) {
                modelManager->importModel(resDir.absoluteFilePath(subdir));
            }
        }
    }

    if (config->modelId().isEmpty() && !modelManager->models().isEmpty()) {
        config->setModelId(modelManager->models().first().id);
    }

    PetWindow window(config.get(), modelManager.get());
    window.show();

    return app.exec();
}
