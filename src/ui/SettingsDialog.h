#ifndef SETTINGSDIALOG_H
#define SETTINGSDIALOG_H

#include <QDialog>

class QSlider;
class QSpinBox;
class QRadioButton;
class QCheckBox;
class QLabel;
class Configuration;

class SettingsDialog : public QDialog {
    Q_OBJECT

public:
    explicit SettingsDialog(Configuration *config, QWidget *parent = nullptr);
    ~SettingsDialog() override = default;

signals:
    void settingsChanged();

private slots:
    void onAccepted();

private:
    void setupUi();
    void loadFromConfig();

    Configuration *m_config;

    QSlider *m_sensibilitySlider;
    QLabel *m_sensibilityValueLabel;
    QSpinBox *m_widthSpin;
    QSpinBox *m_heightSpin;
    QRadioButton *m_leftRadio;
    QRadioButton *m_rightRadio;
    QCheckBox *m_hideOnHoverCheck;
};

#endif // SETTINGSDIALOG_H
