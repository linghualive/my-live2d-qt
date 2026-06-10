#include "TrayIconManager.h"

#include <QAction>
#include <QCoreApplication>
#include <QMenu>
#include <QStyle>
#include <QApplication>
#include <QSystemTrayIcon>

TrayIconManager::TrayIconManager(QObject *parent)
    : QObject(parent)
{
    m_trayIcon = new QSystemTrayIcon(this);
    m_trayIcon->setToolTip(QStringLiteral("QDesktopPet"));
    m_trayIcon->setIcon(QApplication::style()->standardIcon(QStyle::SP_ComputerIcon));

    m_menu = new QMenu();

    // Show/Hide action (checkable, default checked = visible)
    m_showHideAction = new QAction(QStringLiteral("Show/Hide"), this);
    m_showHideAction->setCheckable(true);
    m_showHideAction->setChecked(true);
    connect(m_showHideAction, &QAction::toggled,
            this, &TrayIconManager::toggleVisibility);
    m_menu->addAction(m_showHideAction);

    m_menu->addSeparator();

    // Preferences... action
    QAction *prefsAction = new QAction(QStringLiteral("Preferences..."), this);
    connect(prefsAction, &QAction::triggered,
            this, &TrayIconManager::openPreferences);
    m_menu->addAction(prefsAction);

    m_menu->addSeparator();

    // Quit action
    QAction *quitAction = new QAction(QStringLiteral("Quit"), this);
    connect(quitAction, &QAction::triggered,
            QCoreApplication::instance(), &QCoreApplication::quit,
            Qt::QueuedConnection);
    m_menu->addAction(quitAction);

    m_trayIcon->setContextMenu(m_menu);
}

void TrayIconManager::show()
{
    m_trayIcon->setVisible(true);
    m_trayIcon->show();
}
