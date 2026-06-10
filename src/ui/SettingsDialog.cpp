#include "SettingsDialog.h"

#include "core/Configuration.h"

#include <QCheckBox>
#include <QDialogButtonBox>
#include <QFormLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QRadioButton>
#include <QSlider>
#include <QSpinBox>
#include <QVBoxLayout>

static const char *kSettingsStyle = R"(
    QDialog {
        background-color: #1e1e2e;
    }
    QLabel {
        color: #cdd6f4;
        font-size: 13px;
    }
    QSlider::groove:horizontal {
        height: 6px;
        background: #313244;
        border-radius: 3px;
    }
    QSlider::handle:horizontal {
        width: 16px;
        height: 16px;
        margin: -5px 0;
        background: #89b4fa;
        border-radius: 8px;
    }
    QSlider::sub-page:horizontal {
        background: #89b4fa;
        border-radius: 3px;
    }
    QSpinBox {
        background-color: #313244;
        color: #cdd6f4;
        border: 1px solid #45475a;
        border-radius: 6px;
        padding: 4px 8px;
        font-size: 13px;
    }
    QSpinBox:focus {
        border-color: #89b4fa;
    }
    QGroupBox {
        border: 1px solid #45475a;
        border-radius: 8px;
        margin-top: 4px;
        padding-top: 4px;
    }
    QRadioButton {
        color: #cdd6f4;
        font-size: 13px;
        spacing: 6px;
    }
    QRadioButton::indicator {
        width: 16px;
        height: 16px;
    }
    QCheckBox {
        color: #cdd6f4;
        font-size: 13px;
        spacing: 6px;
    }
    QDialogButtonBox QPushButton {
        background-color: #313244;
        color: #cdd6f4;
        border: 1px solid #45475a;
        border-radius: 8px;
        padding: 8px 20px;
        font-size: 13px;
    }
    QDialogButtonBox QPushButton:hover {
        background-color: #45475a;
        border-color: #89b4fa;
    }
)";

SettingsDialog::SettingsDialog(Configuration *config, QWidget *parent)
    : QDialog(parent)
    , m_config(config)
{
    setWindowTitle(QStringLiteral("Settings"));
    setMinimumWidth(380);
    setStyleSheet(QString::fromUtf8(kSettingsStyle));
    setupUi();
    loadFromConfig();
}

void SettingsDialog::setupUi()
{
    auto *mainLayout = new QVBoxLayout(this);
    mainLayout->setSpacing(16);
    mainLayout->setContentsMargins(20, 20, 20, 20);

    auto *titleLabel = new QLabel(QStringLiteral("Settings"), this);
    QFont f = titleLabel->font();
    f.setPointSize(18);
    f.setBold(true);
    titleLabel->setFont(f);
    mainLayout->addWidget(titleLabel);

    auto *formLayout = new QFormLayout();
    formLayout->setSpacing(12);
    formLayout->setLabelAlignment(Qt::AlignRight | Qt::AlignVCenter);

    // Mouse Sensibility
    auto *sensibilityLayout = new QHBoxLayout();
    m_sensibilitySlider = new QSlider(Qt::Horizontal, this);
    m_sensibilitySlider->setRange(1, 20);
    m_sensibilitySlider->setTickInterval(1);
    m_sensibilityValueLabel = new QLabel(this);
    m_sensibilityValueLabel->setMinimumWidth(30);
    m_sensibilityValueLabel->setAlignment(Qt::AlignCenter);
    sensibilityLayout->addWidget(m_sensibilitySlider);
    sensibilityLayout->addWidget(m_sensibilityValueLabel);
    connect(m_sensibilitySlider, &QSlider::valueChanged,
            this, [this](int value) {
        m_sensibilityValueLabel->setText(QString::number(value * 0.1, 'f', 1));
    });
    formLayout->addRow(QStringLiteral("Sensibility"), sensibilityLayout);

    // Widget size
    m_widthSpin = new QSpinBox(this);
    m_widthSpin->setRange(200, 2000);
    m_widthSpin->setSuffix(QStringLiteral(" px"));
    formLayout->addRow(QStringLiteral("Width"), m_widthSpin);

    m_heightSpin = new QSpinBox(this);
    m_heightSpin->setRange(200, 2000);
    m_heightSpin->setSuffix(QStringLiteral(" px"));
    formLayout->addRow(QStringLiteral("Height"), m_heightSpin);

    // Position
    auto *positionGroup = new QGroupBox(this);
    auto *positionLayout = new QHBoxLayout(positionGroup);
    m_leftRadio = new QRadioButton(QStringLiteral("Left"), positionGroup);
    m_rightRadio = new QRadioButton(QStringLiteral("Right"), positionGroup);
    positionLayout->addWidget(m_leftRadio);
    positionLayout->addWidget(m_rightRadio);
    positionLayout->addStretch();
    formLayout->addRow(QStringLiteral("Position"), positionGroup);

    // Hide on Hover
    m_hideOnHoverCheck = new QCheckBox(QStringLiteral("Enabled"), this);
    formLayout->addRow(QStringLiteral("Hide on Hover"), m_hideOnHoverCheck);

    mainLayout->addLayout(formLayout);
    mainLayout->addStretch();

    auto *buttonBox = new QDialogButtonBox(
        QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    connect(buttonBox, &QDialogButtonBox::accepted,
            this, &SettingsDialog::onAccepted);
    connect(buttonBox, &QDialogButtonBox::rejected,
            this, &QDialog::reject);
    mainLayout->addWidget(buttonBox);
}

void SettingsDialog::loadFromConfig()
{
    int sliderValue = static_cast<int>(m_config->mouseSensibility() * 10.0 + 0.5);
    if (sliderValue < 1) sliderValue = 1;
    if (sliderValue > 20) sliderValue = 20;
    m_sensibilitySlider->setValue(sliderValue);

    QSize sz = m_config->widgetSize();
    m_widthSpin->setValue(sz.width());
    m_heightSpin->setValue(sz.height());

    if (m_config->widgetOnLeft()) {
        m_leftRadio->setChecked(true);
    } else {
        m_rightRadio->setChecked(true);
    }

    m_hideOnHoverCheck->setChecked(m_config->hideOnHover());
}

void SettingsDialog::onAccepted()
{
    double sensibility = m_sensibilitySlider->value() * 0.1;
    m_config->setMouseSensibility(sensibility);
    m_config->setWidgetSize(QSize(m_widthSpin->value(), m_heightSpin->value()));
    m_config->setWidgetOnLeft(m_leftRadio->isChecked());
    m_config->setHideOnHover(m_hideOnHoverCheck->isChecked());
    m_config->save();

    emit settingsChanged();
    accept();
}
