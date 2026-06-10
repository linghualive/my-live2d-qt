#include <QApplication>
#include <QDir>
#include <QFile>
#include <QMessageBox>
#include <QSurfaceFormat>
#include <QTextStream>
#include <memory>

#include "core/AppPaths.h"
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

static void handleCrashedModel(Configuration *config)
{
    QFile pendingFile(AppPaths::pendingModelFile());
    if (!pendingFile.exists()) return;

    QString crashedModelId;
    if (pendingFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
        crashedModelId = QString::fromUtf8(pendingFile.readAll()).trimmed();
        pendingFile.close();
    }
    pendingFile.remove();

    if (crashedModelId.isEmpty()) return;

    // Add to blacklist
    QFile blacklist(AppPaths::modelBlacklistFile());
    if (blacklist.open(QIODevice::Append | QIODevice::Text)) {
        QTextStream out(&blacklist);
        out << crashedModelId << '\n';
    }

    // Revert to a different model
    if (config->modelId() == crashedModelId) {
        config->setModelId(QString());
    }

    QMessageBox::warning(nullptr, QStringLiteral("Model Error"),
        QStringLiteral("The model \"%1\" caused a crash and has been disabled. "
                       "It will be skipped in future loading attempts.")
            .arg(crashedModelId));
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

    AppPaths::ensureDirsExist();
    auto config = std::make_unique<Configuration>();

    handleCrashedModel(config.get());

    ProcessLock lock;
    if (!lock.tryLock()) {
        QMessageBox::warning(nullptr, QStringLiteral("QDesktopPet"),
                             QStringLiteral("Another instance is already running."));
        return 1;
    }

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
