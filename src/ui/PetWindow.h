#ifndef PETWINDOW_H
#define PETWINDOW_H

#include <QWidget>
#include <QPoint>
#include <memory>

class QLive2dWidget;
class Configuration;
class ModelManager;
class TrayIconManager;
class PreferencesDialog;
class IMouseTracker;
class IInputPassthrough;

class PetWindow : public QWidget {
    Q_OBJECT

public:
    explicit PetWindow(Configuration *config, ModelManager *modelManager,
                       QWidget *parent = nullptr);
    ~PetWindow() override;

    void applyConfiguration();
    void updatePosition();
    void switchModel(const QString &modelId);

protected:
    bool eventFilter(QObject *obj, QEvent *event) override;
    void showEvent(QShowEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;

private slots:
    void onLive2dInitialized(QLive2dWidget *wid);
    void onMouseMoved(QPoint rel, QPoint raw);
    void onMousePressed(QPoint rel, QPoint raw);
    void onMouseReleased(QPoint rel, QPoint raw);

private:
    void initMouseTracker();

    Configuration *m_config;
    ModelManager *m_modelManager;

    QLive2dWidget *m_live2dWidget;
    TrayIconManager *m_trayManager;
    PreferencesDialog *m_preferencesDialog;

    std::unique_ptr<IMouseTracker> m_mouseTracker;
    std::unique_ptr<IInputPassthrough> m_inputPassthrough;

    bool m_initialized;
    bool m_dragging = false;
    QPoint m_dragStartPos;
};

#endif // PETWINDOW_H
