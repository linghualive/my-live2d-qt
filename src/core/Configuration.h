#ifndef CONFIGURATION_H
#define CONFIGURATION_H

#include <QObject>
#include <QSettings>
#include <QSize>
#include <QString>
#include <memory>

class Configuration : public QObject {
    Q_OBJECT

public:
    explicit Configuration(QObject *parent = nullptr);
    ~Configuration() override = default;

    QString modelId() const;
    void setModelId(const QString &id);

    bool hideOnHover() const;
    void setHideOnHover(bool hide);

    bool widgetOnLeft() const;
    void setWidgetOnLeft(bool left);

    double mouseSensibility() const;
    void setMouseSensibility(double sensibility);

    QSize widgetSize() const;
    void setWidgetSize(const QSize &size);

    void save();

signals:
    void modelIdChanged(const QString &modelId);
    void settingsChanged();

private:
    void load();
    void migrateOldConfig();

    std::unique_ptr<QSettings> m_settings;

    QString m_modelId;
    bool m_hideOnHover = true;
    bool m_widgetOnLeft = true;
    double m_mouseSensibility = 1.0;
    QSize m_widgetSize{500, 500};
};

#endif // CONFIGURATION_H
