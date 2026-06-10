#include "PreferencesDialog.h"

#include "core/Configuration.h"
#include "core/ModelManager.h"
#include "core/ModelInfo.h"

#include "platform/AutoStart.h"

#include <QCheckBox>
#include <QComboBox>
#include <QDir>
#include <QDirIterator>
#include <QFileDialog>
#include <QFont>
#include <QFormLayout>
#include <QFrame>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QListWidget>
#include <QMessageBox>
#include <QPushButton>
#include <QRadioButton>
#include <QSlider>
#include <QSpinBox>
#include <QStackedWidget>
#include <QImage>
#include <QVBoxLayout>

static const char *kStyleSheet = R"(
    QDialog {
        background-color: #1e1e2e;
    }

    /* --- Navigation sidebar --- */
    QListWidget#navList {
        background-color: #181825;
        border: none;
        border-right: 1px solid #313244;
        outline: none;
        padding: 8px 0;
        font-size: 14px;
        font-weight: 600;
    }
    QListWidget#navList::item {
        color: #a6adc8;
        padding: 12px 24px;
        border-radius: 0;
        margin: 0;
    }
    QListWidget#navList::item:selected {
        color: #89b4fa;
        background-color: #1e1e2e;
        border-left: 3px solid #89b4fa;
    }
    QListWidget#navList::item:hover:!selected {
        color: #cdd6f4;
        background-color: #1e1e2e;
    }

    /* --- Model grid --- */
    QListWidget#modelGrid {
        background-color: #1e1e2e;
        border: none;
        outline: none;
        padding: 8px;
    }
    QListWidget#modelGrid::item {
        background-color: #313244;
        border-radius: 12px;
        padding: 8px;
        margin: 4px;
        color: #cdd6f4;
    }
    QListWidget#modelGrid::item:selected {
        background-color: #585b70;
        border: 2px solid #89b4fa;
    }
    QListWidget#modelGrid::item:hover {
        background-color: #45475a;
    }

    /* --- Labels --- */
    QLabel {
        color: #cdd6f4;
    }
    QLabel#infoLabel {
        font-size: 12px;
        color: #a6adc8;
    }
    QLabel#infoValue {
        font-size: 13px;
        color: #bac2de;
    }
    QLabel#modelName {
        font-size: 20px;
        font-weight: 700;
        color: #cdd6f4;
    }
    QLabel#previewImage {
        background-color: #11111b;
        border-radius: 12px;
    }
    QLabel#sectionTitle {
        font-size: 18px;
        font-weight: 700;
        color: #cdd6f4;
        padding-left: 8px;
    }

    /* --- Buttons --- */
    QPushButton {
        background-color: #313244;
        color: #cdd6f4;
        border: 1px solid #45475a;
        border-radius: 8px;
        padding: 8px 20px;
        font-size: 13px;
        font-weight: 500;
    }
    QPushButton:hover {
        background-color: #45475a;
        border-color: #89b4fa;
    }
    QPushButton:pressed {
        background-color: #585b70;
    }
    QPushButton#applyButton {
        background-color: #89b4fa;
        color: #1e1e2e;
        border: none;
        font-weight: 600;
    }
    QPushButton#applyButton:hover {
        background-color: #74c7ec;
    }
    QPushButton#deleteButton {
        color: #f38ba8;
        border-color: #f38ba8;
    }
    QPushButton#deleteButton:hover {
        background-color: #f38ba8;
        color: #1e1e2e;
    }
    QPushButton#saveButton {
        background-color: #a6e3a1;
        color: #1e1e2e;
        border: none;
        font-weight: 600;
    }
    QPushButton#saveButton:hover {
        background-color: #94e2d5;
    }

    /* --- Detail panel --- */
    QFrame#detailPanel {
        background-color: #181825;
        border-radius: 16px;
        padding: 16px;
    }

    /* --- Settings controls --- */
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
        margin-top: 16px;
        padding-top: 20px;
        font-size: 13px;
        font-weight: 600;
        color: #a6adc8;
    }
    QGroupBox::title {
        subcontrol-origin: margin;
        subcontrol-position: top left;
        padding: 0 8px;
        left: 12px;
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
)";

PreferencesDialog::PreferencesDialog(ModelManager *manager,
                                     Configuration *config,
                                     QWidget *parent)
    : QDialog(parent)
    , m_manager(manager)
    , m_config(config)
{
    setWindowTitle(QStringLiteral("Preferences"));
    setMinimumSize(900, 560);
    resize(960, 600);
    setStyleSheet(QString::fromUtf8(kStyleSheet));
    setupUi();

    m_previewDebounce.setSingleShot(true);
    m_previewDebounce.setInterval(300);
    connect(&m_previewDebounce, &QTimer::timeout, this, [this]() {
        if (!m_pendingPreviewModelId.isEmpty()) {
            emit modelPreviewRequested(m_pendingPreviewModelId);
        }
    });

    connect(m_manager, &ModelManager::modelsChanged,
            this, &PreferencesDialog::refreshModelList);

    refreshModelList();
    loadSettingsFromConfig();
}

void PreferencesDialog::setupUi()
{
    auto *root = new QHBoxLayout(this);
    root->setSpacing(0);
    root->setContentsMargins(0, 0, 0, 0);

    // === Left: navigation sidebar ===
    m_navList = new QListWidget(this);
    m_navList->setObjectName(QStringLiteral("navList"));
    m_navList->setFixedWidth(140);
    m_navList->setFocusPolicy(Qt::NoFocus);

    auto *modelsItem = new QListWidgetItem(QStringLiteral("Models"));
    auto *settingsItem = new QListWidgetItem(QStringLiteral("Settings"));
    modelsItem->setSizeHint(QSize(140, 44));
    settingsItem->setSizeHint(QSize(140, 44));
    m_navList->addItem(modelsItem);
    m_navList->addItem(settingsItem);
    m_navList->setCurrentRow(0);

    root->addWidget(m_navList);

    // === Right: stacked content ===
    m_stack = new QStackedWidget(this);
    m_stack->addWidget(createModelsPage());
    m_stack->addWidget(createSettingsPage());
    root->addWidget(m_stack, 1);

    connect(m_navList, &QListWidget::currentRowChanged,
            m_stack, &QStackedWidget::setCurrentIndex);
}

// ============================================================
//  Models page
// ============================================================
QWidget *PreferencesDialog::createModelsPage()
{
    auto *page = new QWidget(this);
    auto *outer = new QHBoxLayout(page);
    outer->setSpacing(0);
    outer->setContentsMargins(12, 12, 12, 12);

    // --- Left: grid ---
    auto *leftPanel = new QVBoxLayout();
    leftPanel->setSpacing(8);

    auto *headerLayout = new QHBoxLayout();
    auto *title = new QLabel(QStringLiteral("Models"), page);
    title->setObjectName(QStringLiteral("sectionTitle"));
    headerLayout->addWidget(title);
    headerLayout->addStretch();

    m_importButton = new QPushButton(QStringLiteral("+ Import"), page);
    connect(m_importButton, &QPushButton::clicked,
            this, &PreferencesDialog::onImport);
    headerLayout->addWidget(m_importButton);
    leftPanel->addLayout(headerLayout);

    m_modelGrid = new QListWidget(page);
    m_modelGrid->setObjectName(QStringLiteral("modelGrid"));
    m_modelGrid->setViewMode(QListView::IconMode);
    m_modelGrid->setIconSize(QSize(100, 100));
    m_modelGrid->setGridSize(QSize(130, 140));
    m_modelGrid->setResizeMode(QListView::Adjust);
    m_modelGrid->setSpacing(6);
    m_modelGrid->setWordWrap(true);
    m_modelGrid->setSelectionMode(QAbstractItemView::SingleSelection);
    m_modelGrid->setMovement(QListView::Static);
    m_modelGrid->setMinimumWidth(380);
    connect(m_modelGrid, &QListWidget::currentItemChanged,
            this, [this](QListWidgetItem *, QListWidgetItem *) {
        onModelSelectionChanged();
    });
    connect(m_modelGrid, &QListWidget::itemDoubleClicked,
            this, [this](QListWidgetItem *) { onApply(); });
    leftPanel->addWidget(m_modelGrid);

    outer->addLayout(leftPanel, 3);
    outer->addSpacing(12);

    // --- Right: detail panel ---
    auto *detailFrame = new QFrame(page);
    detailFrame->setObjectName(QStringLiteral("detailPanel"));
    detailFrame->setFixedWidth(260);
    auto *detailLayout = new QVBoxLayout(detailFrame);
    detailLayout->setSpacing(12);
    detailLayout->setContentsMargins(16, 16, 16, 16);

    m_previewImage = new QLabel(detailFrame);
    m_previewImage->setObjectName(QStringLiteral("previewImage"));
    m_previewImage->setFixedSize(228, 228);
    m_previewImage->setAlignment(Qt::AlignCenter);
    m_previewImage->setScaledContents(false);
    detailLayout->addWidget(m_previewImage, 0, Qt::AlignCenter);

    m_modelName = new QLabel(detailFrame);
    m_modelName->setObjectName(QStringLiteral("modelName"));
    m_modelName->setAlignment(Qt::AlignCenter);
    detailLayout->addWidget(m_modelName);

    auto *sep = new QFrame(detailFrame);
    sep->setFrameShape(QFrame::HLine);
    sep->setStyleSheet(QStringLiteral("color: #313244;"));
    detailLayout->addWidget(sep);

    auto addInfoRow = [&](const QString &label, QLabel *&valueLabel) {
        auto *row = new QHBoxLayout();
        auto *lbl = new QLabel(label, detailFrame);
        lbl->setObjectName(QStringLiteral("infoLabel"));
        valueLabel = new QLabel(detailFrame);
        valueLabel->setObjectName(QStringLiteral("infoValue"));
        valueLabel->setAlignment(Qt::AlignRight);
        row->addWidget(lbl);
        row->addStretch();
        row->addWidget(valueLabel);
        detailLayout->addLayout(row);
    };

    addInfoRow(QStringLiteral("Size"), m_modelSize);
    addInfoRow(QStringLiteral("Motions"), m_modelMotions);
    addInfoRow(QStringLiteral("Textures"), m_modelTextures);

    detailLayout->addStretch();

    m_applyButton = new QPushButton(QStringLiteral("Use This Model"), detailFrame);
    m_applyButton->setObjectName(QStringLiteral("applyButton"));
    m_applyButton->setCursor(Qt::PointingHandCursor);
    connect(m_applyButton, &QPushButton::clicked,
            this, &PreferencesDialog::onApply);
    detailLayout->addWidget(m_applyButton);

    m_deleteButton = new QPushButton(QStringLiteral("Delete"), detailFrame);
    m_deleteButton->setObjectName(QStringLiteral("deleteButton"));
    m_deleteButton->setCursor(Qt::PointingHandCursor);
    connect(m_deleteButton, &QPushButton::clicked,
            this, &PreferencesDialog::onDelete);
    detailLayout->addWidget(m_deleteButton);

    outer->addWidget(detailFrame);
    return page;
}

// ============================================================
//  Settings page
// ============================================================
QWidget *PreferencesDialog::createSettingsPage()
{
    auto *page = new QWidget(this);
    auto *mainLayout = new QVBoxLayout(page);
    mainLayout->setSpacing(20);
    mainLayout->setContentsMargins(28, 24, 28, 24);

    auto *title = new QLabel(QStringLiteral("Settings"), page);
    title->setObjectName(QStringLiteral("sectionTitle"));
    mainLayout->addWidget(title);

    auto makeHint = [&](const QString &text) {
        auto *hint = new QLabel(text, page);
        hint->setObjectName(QStringLiteral("infoLabel"));
        hint->setWordWrap(true);
        return hint;
    };

    // ── Appearance ──
    auto *appearGroup = new QGroupBox(QStringLiteral("Appearance"), page);
    auto *appearLayout = new QFormLayout(appearGroup);
    appearLayout->setSpacing(10);
    appearLayout->setContentsMargins(16, 20, 16, 12);
    appearLayout->setLabelAlignment(Qt::AlignRight | Qt::AlignVCenter);

    auto *sizeLayout = new QHBoxLayout();
    m_widthSpin = new QSpinBox(page);
    m_widthSpin->setRange(100, 2000);
    m_widthSpin->setSuffix(QStringLiteral(" px"));
    auto *sizeX = new QLabel(QStringLiteral("×"), page);
    sizeX->setAlignment(Qt::AlignCenter);
    sizeX->setFixedWidth(20);
    m_heightSpin = new QSpinBox(page);
    m_heightSpin->setRange(100, 2000);
    m_heightSpin->setSuffix(QStringLiteral(" px"));
    sizeLayout->addWidget(m_widthSpin);
    sizeLayout->addWidget(sizeX);
    sizeLayout->addWidget(m_heightSpin);
    sizeLayout->addStretch();
    appearLayout->addRow(QStringLiteral("Size"), sizeLayout);

    auto *positionLayout = new QHBoxLayout();
    m_leftRadio = new QRadioButton(QStringLiteral("Left"), page);
    m_rightRadio = new QRadioButton(QStringLiteral("Right"), page);
    positionLayout->addWidget(m_leftRadio);
    positionLayout->addWidget(m_rightRadio);
    positionLayout->addStretch();
    appearLayout->addRow(QStringLiteral("Position"), positionLayout);

    mainLayout->addWidget(appearGroup);

    // ── Behavior ──
    auto *behaviorGroup = new QGroupBox(QStringLiteral("Behavior"), page);
    auto *behaviorLayout = new QFormLayout(behaviorGroup);
    behaviorLayout->setSpacing(10);
    behaviorLayout->setContentsMargins(16, 20, 16, 12);
    behaviorLayout->setLabelAlignment(Qt::AlignRight | Qt::AlignVCenter);

    auto *sensibilityLayout = new QHBoxLayout();
    m_sensibilitySlider = new QSlider(Qt::Horizontal, page);
    m_sensibilitySlider->setRange(1, 20);
    m_sensibilitySlider->setTickInterval(1);
    m_sensibilityValueLabel = new QLabel(page);
    m_sensibilityValueLabel->setMinimumWidth(30);
    m_sensibilityValueLabel->setAlignment(Qt::AlignCenter);
    sensibilityLayout->addWidget(m_sensibilitySlider, 1);
    sensibilityLayout->addWidget(m_sensibilityValueLabel);
    connect(m_sensibilitySlider, &QSlider::valueChanged,
            this, [this](int value) {
        m_sensibilityValueLabel->setText(QString::number(value * 0.1, 'f', 1));
    });
    behaviorLayout->addRow(QStringLiteral("Sensibility"), sensibilityLayout);
    behaviorLayout->addRow(QString(), makeHint(QStringLiteral("Mouse tracking sensitivity for eye follow")));

    m_frameRateCombo = new QComboBox(page);
    m_frameRateCombo->addItem(QStringLiteral("15 FPS (Power Saver)"), 15);
    m_frameRateCombo->addItem(QStringLiteral("30 FPS (Balanced)"), 30);
    m_frameRateCombo->addItem(QStringLiteral("60 FPS (Smooth)"), 60);
    behaviorLayout->addRow(QStringLiteral("Frame Rate"), m_frameRateCombo);
    behaviorLayout->addRow(QString(), makeHint(QStringLiteral("Higher FPS = smoother animation but more CPU usage")));

    m_hideOnHoverCheck = new QCheckBox(QStringLiteral("Hide when mouse hovers over pet"), page);
    behaviorLayout->addRow(QString(), m_hideOnHoverCheck);

    mainLayout->addWidget(behaviorGroup);

    // ── System ──
    auto *systemGroup = new QGroupBox(QStringLiteral("System"), page);
    auto *systemLayout = new QFormLayout(systemGroup);
    systemLayout->setSpacing(10);
    systemLayout->setContentsMargins(16, 20, 16, 12);

    m_autoStartCheck = new QCheckBox(QStringLiteral("Launch at login"), page);
    systemLayout->addRow(QString(), m_autoStartCheck);

    mainLayout->addWidget(systemGroup);

    mainLayout->addStretch();

    // ── Save button ──
    auto *btnLayout = new QHBoxLayout();
    btnLayout->addStretch();
    auto *saveButton = new QPushButton(QStringLiteral("Save"), page);
    saveButton->setObjectName(QStringLiteral("saveButton"));
    saveButton->setCursor(Qt::PointingHandCursor);
    connect(saveButton, &QPushButton::clicked,
            this, &PreferencesDialog::onSettingsAccepted);
    btnLayout->addWidget(saveButton);
    mainLayout->addLayout(btnLayout);

    return page;
}

// ============================================================
//  Models logic
// ============================================================
void PreferencesDialog::refreshModelList()
{
    m_modelGrid->clear();

    const QList<ModelInfo> models = m_manager->models();
    const QString currentModelId = m_config->modelId();
    QListWidgetItem *selectedItem = nullptr;

    for (const ModelInfo &info : models) {
        auto *item = new QListWidgetItem(info.displayName, m_modelGrid);
        QPixmap thumb = loadThumbnail(info.path);
        if (!thumb.isNull()) {
            item->setIcon(QIcon(thumb));
        }
        item->setData(Qt::UserRole, info.id);
        item->setSizeHint(QSize(120, 130));
        if (info.id == currentModelId) {
            selectedItem = item;
        }
    }

    if (selectedItem) {
        m_modelGrid->setCurrentItem(selectedItem);
    } else if (m_modelGrid->count() > 0) {
        m_modelGrid->setCurrentRow(0);
    }
}

void PreferencesDialog::onModelSelectionChanged()
{
    QListWidgetItem *current = m_modelGrid->currentItem();
    if (!current) {
        m_previewImage->clear();
        m_modelName->clear();
        m_modelSize->clear();
        m_modelMotions->clear();
        m_modelTextures->clear();
        m_applyButton->setEnabled(false);
        m_deleteButton->setEnabled(false);
        return;
    }

    m_applyButton->setEnabled(true);
    m_deleteButton->setEnabled(true);

    const QString modelId = current->data(Qt::UserRole).toString();
    const ModelInfo info = m_manager->modelInfo(modelId);

    m_modelName->setText(info.displayName);
    m_modelSize->setText(formatSize(info.sizeBytes));

    int motionCount = 0;
    QDirIterator motionIt(info.path, QStringList() << QStringLiteral("*.motion3.json"),
                          QDir::Files, QDirIterator::Subdirectories);
    while (motionIt.hasNext()) { motionIt.next(); motionCount++; }
    m_modelMotions->setText(QString::number(motionCount));

    int textureCount = 0;
    QDirIterator texIt(info.path, QStringList() << QStringLiteral("*.png"),
                       QDir::Files, QDirIterator::Subdirectories);
    while (texIt.hasNext()) { texIt.next(); textureCount++; }
    m_modelTextures->setText(QString::number(textureCount));

    QPixmap thumb = loadThumbnail(info.path);
    if (!thumb.isNull()) {
        m_previewImage->setPixmap(
            thumb.scaled(220, 220, Qt::KeepAspectRatio, Qt::SmoothTransformation));
    } else {
        m_previewImage->setText(info.displayName);
    }

    m_pendingPreviewModelId = modelId;
    m_previewDebounce.start();
}

QPixmap PreferencesDialog::loadThumbnail(const QString &modelPath) const
{
    if (m_thumbnailCache.contains(modelPath)) {
        return m_thumbnailCache[modelPath];
    }

    QDirIterator it(modelPath,
                    QStringList() << QStringLiteral("*.png"),
                    QDir::Files, QDirIterator::Subdirectories);
    while (it.hasNext()) {
        it.next();
        QPixmap pix(it.filePath());
        if (!pix.isNull()) {
            const_cast<PreferencesDialog *>(this)->m_thumbnailCache[modelPath] = pix;
            return pix;
        }
    }
    return {};
}

QString PreferencesDialog::formatSize(qint64 bytes) const
{
    if (bytes < 1024) {
        return QString::number(bytes) + QStringLiteral(" B");
    }
    double kb = bytes / 1024.0;
    if (kb < 1024.0) {
        return QString::number(kb, 'f', 1) + QStringLiteral(" KB");
    }
    double mb = kb / 1024.0;
    return QString::number(mb, 'f', 1) + QStringLiteral(" MB");
}

void PreferencesDialog::onImport()
{
    const QString dir = QFileDialog::getExistingDirectory(
        this, QStringLiteral("Select Model Directory"),
        QString(), QFileDialog::ShowDirsOnly);
    if (dir.isEmpty()) return;

    if (!m_manager->importModel(dir)) {
        QMessageBox::warning(this, QStringLiteral("Import Failed"),
                             QStringLiteral("Failed to import model. Make sure the directory "
                                            "contains a valid .model3.json file."));
    }
}

void PreferencesDialog::onDelete()
{
    QListWidgetItem *current = m_modelGrid->currentItem();
    if (!current) return;

    const QString modelId = current->data(Qt::UserRole).toString();
    const QString displayName = current->text();

    if (QMessageBox::question(this, QStringLiteral("Delete Model"),
            QStringLiteral("Are you sure you want to delete \"%1\"?").arg(displayName),
            QMessageBox::Yes | QMessageBox::No, QMessageBox::No) == QMessageBox::Yes) {
        m_manager->deleteModel(modelId);
    }
}

void PreferencesDialog::onApply()
{
    QListWidgetItem *current = m_modelGrid->currentItem();
    if (!current) return;

    const QString modelId = current->data(Qt::UserRole).toString();
    m_config->setModelId(modelId);
    m_config->save();
    emit modelSelected(modelId);
}

// ============================================================
//  Settings logic
// ============================================================
void PreferencesDialog::loadSettingsFromConfig()
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

    int fps = m_config->frameRate();
    int comboIndex = m_frameRateCombo->findData(fps);
    m_frameRateCombo->setCurrentIndex(comboIndex >= 0 ? comboIndex : 1);

    m_autoStartCheck->setChecked(AutoStart::isEnabled());
}

void PreferencesDialog::setPreviewImage(const QImage &image)
{
    if (image.isNull()) return;
    QPixmap pix = QPixmap::fromImage(image);
    m_previewImage->setPixmap(
        pix.scaled(220, 220, Qt::KeepAspectRatio, Qt::SmoothTransformation));
}

void PreferencesDialog::onSettingsAccepted()
{
    m_config->setMouseSensibility(m_sensibilitySlider->value() * 0.1);
    m_config->setWidgetSize(QSize(m_widthSpin->value(), m_heightSpin->value()));
    m_config->setWidgetOnLeft(m_leftRadio->isChecked());
    m_config->setHideOnHover(m_hideOnHoverCheck->isChecked());
    m_config->setFrameRate(m_frameRateCombo->currentData().toInt());
    m_config->save();

    AutoStart::setEnabled(m_autoStartCheck->isChecked());

    emit settingsChanged();
}
