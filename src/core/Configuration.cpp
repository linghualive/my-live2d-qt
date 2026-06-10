#include "Configuration.h"
#include "AppPaths.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>

Configuration::Configuration(QObject *parent)
    : QObject(parent)
{
    AppPaths::ensureDirsExist();
    m_settings = std::make_unique<QSettings>(
        AppPaths::configFile(), QSettings::IniFormat);

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
    // Migrate from v1 config (~/.config/lsk/QDesktopPet.conf)
    const QString v1Path = QDir::homePath()
                           + QStringLiteral("/.config/lsk/QDesktopPet.conf");
    if (QFileInfo::exists(v1Path)) {
        QSettings oldSettings(v1Path, QSettings::IniFormat);
        const QString resourceDir = oldSettings.value(QStringLiteral("resourceDir")).toString();
        if (!resourceDir.isEmpty()) {
            const QDir dir(resourceDir);
            const QString dirName = dir.dirName();
            if (!dirName.isEmpty()) {
                m_settings->setValue(QStringLiteral("modelId"), dirName);
            }
        }
        m_settings->sync();
        QFile::remove(v1Path);
    }

    // Migrate from v2 config (QStandardPaths-based QSettings)
    if (m_settings->allKeys().isEmpty()) {
        QSettings v2Settings(QSettings::UserScope,
                             QStringLiteral("QDesktopPet"),
                             QStringLiteral("QDesktopPet"));
        static const QStringList knownKeys = {
            QStringLiteral("modelId"),
            QStringLiteral("hideOnHover"),
            QStringLiteral("widgetOnLeft"),
            QStringLiteral("mouseSensibility"),
            QStringLiteral("widgetSize"),
            QStringLiteral("frameRate"),
        };
        bool migrated = false;
        for (const QString &key : knownKeys) {
            if (v2Settings.contains(key)) {
                m_settings->setValue(key, v2Settings.value(key));
                migrated = true;
            }
        }
        if (migrated) {
            m_settings->sync();
        }
    }
}
