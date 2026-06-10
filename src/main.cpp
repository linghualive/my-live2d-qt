#include <QApplication>
#include <QDir>
#include <QFile>
#include <QMessageBox>
#include <QOffscreenSurface>
#include <QOpenGLContext>
#include <QOpenGLFunctions>
#include <QSurfaceFormat>
#include <QTextStream>
#include <memory>

#include "core/AppPaths.h"
#include "core/ProcessLock.h"
#include "core/Configuration.h"
#include "core/ModelManager.h"
#include "ui/PetWindow.h"

#ifdef HAS_X11
#include <X11/Xlib.h>
#include <X11/extensions/record.h>
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
#endif

static void printDiagnostics(bool verbose)
{
    auto out = [](const char *label, const QString &val) {
        fprintf(stderr, "  %-20s %s\n", label, qPrintable(val));
    };
    auto ok  = [](const char *label, bool pass, const QString &detail = {}) {
        fprintf(stderr, "  %s %-18s %s\n", pass ? "[OK]" : "[!!]", label,
                qPrintable(detail));
    };

    fprintf(stderr, "\n=== QDesktopPet Diagnostics ===\n\n");

    // Qt
    fprintf(stderr, "Qt:\n");
    out("Version:", QString::fromLatin1(qVersion()));
    out("Platform:", QGuiApplication::platformName());
    out("Compiled with:", QStringLiteral("Qt %1").arg(QT_VERSION_STR));

    // OpenGL
    fprintf(stderr, "\nOpenGL:\n");
    {
        QOpenGLContext ctx;
        QOffscreenSurface surface;
        surface.setFormat(QSurfaceFormat::defaultFormat());
        surface.create();
        bool glOk = ctx.create() && ctx.makeCurrent(&surface);
        if (glOk) {
            auto *f = ctx.functions();
            auto glStr = [&](GLenum name) {
                auto *s = reinterpret_cast<const char *>(f->glGetString(name));
                return s ? QString::fromLatin1(s) : QStringLiteral("(null)");
            };
            ok("Context:", true, glStr(GL_VERSION));
            out("Renderer:", glStr(GL_RENDERER));
            out("Vendor:", glStr(GL_VENDOR));
            ctx.doneCurrent();
        } else {
            ok("Context:", false, QStringLiteral("FAILED to create OpenGL context"));
            fprintf(stderr, "         >>> Install mesa / GPU drivers\n");
        }
    }

    // Display server
    fprintf(stderr, "\nDisplay:\n");
    QString platform = QGuiApplication::platformName();
#ifdef HAS_X11
    if (platform == QStringLiteral("xcb")) {
        ok("X11:", true, QStringLiteral("running on xcb"));

        Display *dpy = XOpenDisplay(nullptr);
        if (dpy) {
            int major = 0, minor = 0;
            bool record = XRecordQueryVersion(dpy, &major, &minor);
            ok("RECORD ext:", record,
               record ? QStringLiteral("v%1.%2").arg(major).arg(minor)
                      : QStringLiteral("NOT available — mouse tracking won't work"));
            if (!record)
                fprintf(stderr, "         >>> Install libxtst (Arch) / libxtst-dev (Debian)\n");
            XCloseDisplay(dpy);
        } else {
            ok("X11 Display:", false, QStringLiteral("cannot open $DISPLAY"));
            fprintf(stderr, "         >>> Is DISPLAY set? Are you in a graphical session?\n");
        }
    }
#endif
#ifdef HAS_WAYLAND
    if (platform == QStringLiteral("wayland")) {
        ok("Wayland:", true, QStringLiteral("running on wayland"));
    }
#endif
    if (platform != QStringLiteral("xcb") && platform != QStringLiteral("wayland")
        && platform != QStringLiteral("cocoa") && platform != QStringLiteral("windows")) {
        ok("Platform:", false, QStringLiteral("unknown platform \"%1\"").arg(platform));
    }

    // Data dirs
    fprintf(stderr, "\nData:\n");
    QString dataDir = AppPaths::baseDir();
    ok("Data dir:", QDir(dataDir).exists(), dataDir);
    QString modelsDir = AppPaths::modelsDir();
    QDir md(modelsDir);
    int modelCount = md.exists() ? md.entryList(QDir::Dirs | QDir::NoDotAndDotDot).count() : 0;
    ok("Models dir:", md.exists(),
       QStringLiteral("%1 (%2 models)").arg(modelsDir).arg(modelCount));
    if (modelCount == 0) {
        fprintf(stderr, "         >>> No models found. Put Live2D model folders in %s\n",
                qPrintable(modelsDir));
    }

    // Config
    QString configPath = dataDir + QStringLiteral("/config.ini");
    ok("Config:", QFile::exists(configPath), configPath);

    // Build info
    if (verbose) {
        fprintf(stderr, "\nBuild:\n");
#ifdef HAS_X11
        out("HAS_X11:", QStringLiteral("yes"));
#else
        out("HAS_X11:", QStringLiteral("no"));
#endif
#ifdef HAS_WAYLAND
        out("HAS_WAYLAND:", QStringLiteral("yes"));
#else
        out("HAS_WAYLAND:", QStringLiteral("no"));
#endif
#ifdef HAS_MACOS
        out("HAS_MACOS:", QStringLiteral("yes"));
#else
        out("HAS_MACOS:", QStringLiteral("no"));
#endif
#ifdef HAS_WINDOWS
        out("HAS_WINDOWS:", QStringLiteral("yes"));
#else
        out("HAS_WINDOWS:", QStringLiteral("no"));
#endif
        out("CXX Standard:", QString::fromLatin1(__cplusplus > 201703L ? "C++20+" : "C++17"));
    }

    fprintf(stderr, "\n");
}

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

    bool diagnoseMode = app.arguments().contains(QStringLiteral("--diagnose"));

    AppPaths::ensureDirsExist();

    if (diagnoseMode) {
        printDiagnostics(true);
        return 0;
    }

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
