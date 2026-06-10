#ifndef TRAYICONMANAGER_H
#define TRAYICONMANAGER_H

#include <QObject>

class QSystemTrayIcon;
class QMenu;
class QAction;

class TrayIconManager : public QObject {
    Q_OBJECT

public:
    explicit TrayIconManager(QObject *parent = nullptr);
    ~TrayIconManager() override = default;

    void show();

signals:
    void toggleVisibility(bool visible);
    void openPreferences();

private:
    QSystemTrayIcon *m_trayIcon;
    QMenu *m_menu;
    QAction *m_showHideAction;
};

#endif // TRAYICONMANAGER_H
