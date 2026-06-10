#include "Configuration.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>

Configuration::Configuration(QObject *parent)
    : QObject(parent)
{
    m_settings = std::make_unique<QSettings>(
        QSettings::UserScope,
        QStringLiteral("QDesktopPet"),
        QStringLiteral("QDesktopPet"));

    migrateOldConfig();
    load();
}

QString Configuration::modelId() const
{
    return m_modelId;
}

void Configuration::setModelId(const QString &id)
{
    if (m_modelId != id) {
        m_modelId = id;
        emit modelIdChanged(m_modelId);
        emit settingsChanged();
    }
}

bool Configuration::hideOnHover() const
{
    return m_hideOnHover;
}

void Configuration::setHideOnHover(bool hide)
{
    if (m_hideOnHover != hide) {
        m_hideOnHover = hide;
        emit settingsChanged();
    }
}

bool Configuration::widgetOnLeft() const
{
    return m_widgetOnLeft;
}

void Configuration::setWidgetOnLeft(bool left)
{
    if (m_widgetOnLeft != left) {
        m_widgetOnLeft = left;
        emit settingsChanged();
    }
}

double Configuration::mouseSensibility() const
{
    return m_mouseSensibility;
}

void Configuration::setMouseSensibility(double sensibility)
{
    if (!qFuzzyCompare(m_mouseSensibility, sensibility)) {
        m_mouseSensibility = sensibility;
        emit settingsChanged();
    }
}

QSize Configuration::widgetSize() const
{
    return m_widgetSize;
}

void Configuration::setWidgetSize(const QSize &size)
{
    if (m_widgetSize != size) {
        m_widgetSize = size;
        emit settingsChanged();
    }
}

int Configuration::frameRate() const
{
    return m_frameRate;
}

void Configuration::setFrameRate(int fps)
{
    if (m_frameRate != fps) {
        m_frameRate = fps;
        emit settingsChanged();
    }
}

void Configuration::load()
{
    m_modelId = m_settings->value(QStringLiteral("modelId"), QString()).toString();
    m_hideOnHover = m_settings->value(QStringLiteral("hideOnHover"), true).toBool();
    m_widgetOnLeft = m_settings->value(QStringLiteral("widgetOnLeft"), true).toBool();
    m_mouseSensibility = m_settings->value(QStringLiteral("mouseSensibility"), 1.0).toDouble();
    m_widgetSize = m_settings->value(QStringLiteral("widgetSize"), QSize(500, 500)).toSize();
    m_frameRate = m_settings->value(QStringLiteral("frameRate"), 30).toInt();
}

void Configuration::save()
{
    m_settings->setValue(QStringLiteral("modelId"), m_modelId);
    m_settings->setValue(QStringLiteral("hideOnHover"), m_hideOnHover);
    m_settings->setValue(QStringLiteral("widgetOnLeft"), m_widgetOnLeft);
    m_settings->setValue(QStringLiteral("mouseSensibility"), m_mouseSensibility);
    m_settings->setValue(QStringLiteral("widgetSize"), m_widgetSize);
    m_settings->setValue(QStringLiteral("frameRate"), m_frameRate);
    m_settings->sync();
}

void Configuration::migrateOldConfig()
{
    const QString oldConfigPath = QDir::homePath()
                                  + QStringLiteral("/.config/lsk/QDesktopPet.conf");
    QFileInfo oldFileInfo(oldConfigPath);
    if (!oldFileInfo.exists()) {
        return;
    }

    QSettings oldSettings(oldConfigPath, QSettings::IniFormat);
    const QString resourceDir = oldSettings.value(QStringLiteral("resourceDir")).toString();

    if (!resourceDir.isEmpty()) {
        // Extract the directory name from the old resourceDir path as the modelId
        const QDir dir(resourceDir);
        const QString dirName = dir.dirName();
        if (!dirName.isEmpty()) {
            m_settings->setValue(QStringLiteral("modelId"), dirName);
        }
    }

    m_settings->sync();

    // Remove the old config file
    QFile::remove(oldConfigPath);
}
