#include "PetWindow.h"

#include "TrayIconManager.h"
#include "PreferencesDialog.h"

#include "core/Configuration.h"
#include "core/ModelManager.h"
#include "core/ModelInfo.h"
#include "platform/IMouseTracker.h"
#include "platform/IInputPassthrough.h"
#include "platform/PlatformFactory.h"

#include "QLive2dWidget.hpp"

#ifdef HAS_MACOS
#include "platform/macos/MacWindowHelper.h"
#endif

#include "core/AppPaths.h"

#include <QDir>
#include <QFile>
#include <QGuiApplication>
#include <QMessageBox>
#include <QMouseEvent>
#include <QScreen>
#include <QShowEvent>
#include <QTextStream>

#ifdef HAS_MACOS
#include "platform/macos/MacWindowHelper.h"
#endif

PetWindow::PetWindow(Configuration *config, ModelManager *modelManager,
                     QWidget *parent)
    : QWidget(parent)
    , m_config(config)
    , m_modelManager(modelManager)
    , m_live2dWidget(nullptr)
    , m_trayManager(nullptr)
    , m_preferencesDialog(nullptr)
    , m_initialized(false)
{
    m_hoverTimer.setSingleShot(true);
    connect(&m_hoverTimer, &QTimer::timeout, this, [this]() {
        if (m_hiddenByHover) {
            m_hiddenByHover = false;
#ifdef HAS_MACOS
            MacWindowHelper::setIgnoresMouseEvents(this, false);
#else
            setAttribute(Qt::WA_TransparentForMouseEvents, false);
#endif
            if (m_live2dWidget)
                m_live2dWidget->show();
        }
    });
    // Window flags: frameless, always on top, tool window
    setWindowFlags(Qt::FramelessWindowHint
                   | Qt::WindowStaysOnTopHint
                   | Qt::Tool);
    setAttribute(Qt::WA_TranslucentBackground);
#ifdef Q_OS_MACOS
    setAttribute(Qt::WA_MacAlwaysShowToolWindow);
#endif

    // Apply configuration to set size and position
    applyConfiguration();

    // Create the Live2D widget as a child
    m_live2dWidget = new QLive2dWidget(this);
    m_live2dWidget->setGeometry(0, 0, width(), height());
    m_live2dWidget->installEventFilter(this);
    connect(m_live2dWidget, &QLive2dWidget::initialized,
            this, &PetWindow::onLive2dInitialized);

    // Create tray icon manager
    m_trayManager = new TrayIconManager(this);
    connect(m_trayManager, &TrayIconManager::toggleVisibility,
            this, [this](bool visible) {
        if (visible) {
            show();
        } else {
            hide();
        }
    });
    connect(m_trayManager, &TrayIconManager::openPreferences,
            this, [this]() {
        if (!m_preferencesDialog) {
            m_preferencesDialog = new PreferencesDialog(m_modelManager, m_config);
            connect(m_preferencesDialog, &PreferencesDialog::modelSelected,
                    this, &PetWindow::switchModel);
            connect(m_preferencesDialog, &PreferencesDialog::modelPreviewRequested,
                    this, &PetWindow::previewModel);
            connect(m_preferencesDialog, &PreferencesDialog::settingsChanged,
                    this, [this]() {
                applyConfiguration();
                if (m_mouseTracker) {
                    m_mouseTracker->setMouseSensibility(m_config->mouseSensibility());
                }
                if (m_live2dWidget) {
                    m_live2dWidget->setFrameRate(m_config->frameRate());
                }
            });
            connect(m_preferencesDialog, &QDialog::finished,
                    this, &PetWindow::restoreOriginalModel);
        }
        m_preferencesDialog->show();
        m_preferencesDialog->raise();
        m_preferencesDialog->activateWindow();
    });
    m_trayManager->show();

    // Input passthrough: on X11 use XShape to pass clicks through transparent pixels.
    // On macOS/Wayland, WA_TransparentForMouseEvents blocks ALL interaction
    // (including drag), so we skip it — the small window is acceptable.
#if defined(HAS_X11) || defined(HAS_WINDOWS)
    m_inputPassthrough = PlatformFactory::createInputPassthrough();
    if (m_inputPassthrough) {
        m_inputPassthrough->enablePassthrough(this);
    }
#endif
}

PetWindow::~PetWindow()
{
    if (m_mouseTracker) {
        m_mouseTracker->stop();
    }
    delete m_preferencesDialog;
}

void PetWindow::applyConfiguration()
{
    QSize sz = m_config->widgetSize();
    updatePosition();

    if (m_live2dWidget) {
        m_live2dWidget->setGeometry(0, 0, sz.width(), sz.height());
        m_live2dWidget->update();
    }
}

void PetWindow::updatePosition()
{
    QScreen *screen = QGuiApplication::primaryScreen();
    QRect screenRect = screen->availableGeometry();
    QSize sz = m_config->widgetSize();
    int x, y;
    if (m_config->widgetOnLeft()) {
        x = screenRect.left();
    } else {
        x = screenRect.right() - sz.width();
    }
    y = screenRect.bottom() - sz.height();
    setGeometry(x, y, sz.width(), sz.height());
}

void PetWindow::switchModel(const QString &modelId)
{
    ModelInfo info = m_modelManager->modelInfo(modelId);
    if (info.id.isEmpty()) return;

    if (isModelBlacklisted(modelId)) {
        QMessageBox::warning(m_preferencesDialog ? static_cast<QWidget*>(m_preferencesDialog) : static_cast<QWidget*>(this),
            QStringLiteral("Model Error"),
            QStringLiteral("This model was previously detected as incompatible "
                           "with the current Live2D SDK and cannot be used."));
        return;
    }

    if (!loadModelIntoWidget(modelId)) {
        QMessageBox::warning(m_preferencesDialog ? static_cast<QWidget*>(m_preferencesDialog) : static_cast<QWidget*>(this),
            QStringLiteral("Model Error"),
            QStringLiteral("Failed to load model \"%1\". It may be incompatible "
                           "with the current Live2D SDK.").arg(modelId));
        return;
    }

    m_config->setModelId(modelId);
    m_config->save();
    m_previewOriginalModelId.clear();
}

bool PetWindow::loadModelIntoWidget(const QString &modelId)
{
    if (!m_initialized || !m_live2dWidget) return false;

    ModelInfo info = m_modelManager->modelInfo(modelId);
    if (info.id.isEmpty()) return false;

    writePendingModel(modelId);

    QDir modelDir(info.path);
    QString dirName = modelDir.dirName();
    modelDir.cdUp();
    QString parentPath = modelDir.absolutePath() + QStringLiteral("/");
    m_live2dWidget->setResDir(parentPath.toStdString());
    bool ok = m_live2dWidget->setModel(dirName.toStdString(),
                                       info.modelFile.toStdString());

    clearPendingModel();

    if (!ok) {
        blacklistModel(modelId);
    }

    return ok;
}

void PetWindow::previewModel(const QString &modelId)
{
    if (!m_initialized || !m_live2dWidget) return;

    if (isModelBlacklisted(modelId)) {
        QMessageBox::warning(m_preferencesDialog ? static_cast<QWidget*>(m_preferencesDialog) : static_cast<QWidget*>(this),
            QStringLiteral("Model Error"),
            QStringLiteral("This model was previously detected as incompatible "
                           "with the current Live2D SDK and cannot be loaded."));
        return;
    }

    if (m_previewOriginalModelId.isEmpty()) {
        m_previewOriginalModelId = m_config->modelId();
    }

    if (!loadModelIntoWidget(modelId)) {
        QMessageBox::warning(m_preferencesDialog ? static_cast<QWidget*>(m_preferencesDialog) : static_cast<QWidget*>(this),
            QStringLiteral("Model Error"),
            QStringLiteral("Failed to load model \"%1\". It may be incompatible "
                           "with the current Live2D SDK.").arg(modelId));
        if (!m_previewOriginalModelId.isEmpty()) {
            loadModelIntoWidget(m_previewOriginalModelId);
        }
        return;
    }

    QImage preview = m_live2dWidget->grabFramebuffer();
    if (m_preferencesDialog) {
        m_preferencesDialog->setPreviewImage(preview);
    }
}

void PetWindow::restoreOriginalModel()
{
    if (m_previewOriginalModelId.isEmpty()) return;

    loadModelIntoWidget(m_previewOriginalModelId);
    m_previewOriginalModelId.clear();
}

bool PetWindow::isModelBlacklisted(const QString &modelId) const
{
    QFile file(AppPaths::modelBlacklistFile());
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) return false;
    QTextStream in(&file);
    while (!in.atEnd()) {
        if (in.readLine().trimmed() == modelId) return true;
    }
    return false;
}

void PetWindow::blacklistModel(const QString &modelId)
{
    if (isModelBlacklisted(modelId)) return;
    QFile file(AppPaths::modelBlacklistFile());
    if (file.open(QIODevice::Append | QIODevice::Text)) {
        QTextStream out(&file);
        out << modelId << '\n';
    }
}

void PetWindow::writePendingModel(const QString &modelId)
{
    QFile file(AppPaths::pendingModelFile());
    if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        file.write(modelId.toUtf8());
    }
}

void PetWindow::clearPendingModel()
{
    QFile::remove(AppPaths::pendingModelFile());
}

void PetWindow::onLive2dInitialized(QLive2dWidget *wid)
{
    m_initialized = true;

    const QString modelId = m_config->modelId();
    if (!modelId.isEmpty()) {
        if (!loadModelIntoWidget(modelId)) {
            QMessageBox::warning(this, QStringLiteral("Model Error"),
                QStringLiteral("Failed to load model \"%1\". It may be incompatible "
                               "with the current Live2D SDK.").arg(modelId));
            m_config->setModelId(QString());
        }
    }

    wid->setFrameRate(m_config->frameRate());

    initMouseTracker();
}

void PetWindow::initMouseTracker()
{
    m_mouseTracker = PlatformFactory::createMouseTracker(
        winId(), m_config->mouseSensibility(), this);

    if (m_mouseTracker) {
        connect(m_mouseTracker.get(), &IMouseTracker::mouseMoved,
                this, &PetWindow::onMouseMoved, Qt::QueuedConnection);
        connect(m_mouseTracker.get(), &IMouseTracker::mousePressed,
                this, &PetWindow::onMousePressed, Qt::QueuedConnection);
        connect(m_mouseTracker.get(), &IMouseTracker::mouseReleased,
                this, &PetWindow::onMouseReleased, Qt::QueuedConnection);
        m_mouseTracker->start();
    }
}

void PetWindow::onMouseMoved(QPoint rel, QPoint raw)
{
    if (!m_initialized || !m_live2dWidget) {
        return;
    }

    m_live2dWidget->mouseMove(rel);

    // Hide-on-hover: hide model when mouse enters, restore after a delay
    if (m_config->hideOnHover()) {
        QPoint localPos = mapFromGlobal(raw);
        bool inside = rect().contains(localPos);

        if (inside && !m_hiddenByHover) {
            m_hiddenByHover = true;
            m_live2dWidget->hide();
#ifdef HAS_MACOS
            MacWindowHelper::setIgnoresMouseEvents(this, true);
#else
            setAttribute(Qt::WA_TransparentForMouseEvents, true);
#endif
            m_hoverTimer.start(1500);
        } else if (inside && m_hiddenByHover) {
            m_hoverTimer.start(1500);
        }
    }
}

void PetWindow::onMousePressed(QPoint rel, QPoint raw)
{
    if (!m_initialized || !m_live2dWidget) {
        return;
    }

    QPoint localPos = m_live2dWidget->mapFromGlobal(raw);
    if (m_live2dWidget->rect().contains(localPos)) {
        m_live2dWidget->mousePress(rel);
    }
}

void PetWindow::onMouseReleased(QPoint rel, QPoint raw)
{
    if (!m_initialized || !m_live2dWidget) {
        return;
    }

    QPoint localPos = m_live2dWidget->mapFromGlobal(raw);
    if (m_live2dWidget->rect().contains(localPos)) {
        m_live2dWidget->mouseRelease(rel);
    }
}

void PetWindow::showEvent(QShowEvent *event)
{
    QWidget::showEvent(event);
#ifdef HAS_MACOS
    MacWindowHelper::makeFullyTransparent(this);
#endif
}

bool PetWindow::eventFilter(QObject *obj, QEvent *event)
{
    if (obj == m_live2dWidget) {
        switch (event->type()) {
        case QEvent::MouseButtonPress: {
            auto *me = static_cast<QMouseEvent *>(event);
            if (me->button() == Qt::LeftButton) {
                m_dragging = true;
                m_dragStartPos = me->globalPosition().toPoint() - frameGeometry().topLeft();
            }
            break;
        }
        case QEvent::MouseMove: {
            auto *me = static_cast<QMouseEvent *>(event);
            if (m_dragging && (me->buttons() & Qt::LeftButton)) {
                move(me->globalPosition().toPoint() - m_dragStartPos);
            }
            break;
        }
        case QEvent::MouseButtonRelease: {
            auto *me = static_cast<QMouseEvent *>(event);
            if (me->button() == Qt::LeftButton) {
                m_dragging = false;
#ifdef HAS_MACOS
                MacWindowHelper::makeFullyTransparent(this);
#endif
            }
            break;
        }
        default:
            break;
        }
    }
    return QWidget::eventFilter(obj, event);
}

void PetWindow::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        m_dragging = true;
        m_dragStartPos = event->globalPosition().toPoint() - frameGeometry().topLeft();
        event->accept();
    }
}

void PetWindow::mouseMoveEvent(QMouseEvent *event)
{
    if (m_dragging && (event->buttons() & Qt::LeftButton)) {
        move(event->globalPosition().toPoint() - m_dragStartPos);
        event->accept();
    }
}

void PetWindow::mouseReleaseEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        m_dragging = false;
#ifdef HAS_MACOS
        MacWindowHelper::makeFullyTransparent(this);
#endif
        event->accept();
    }
}

