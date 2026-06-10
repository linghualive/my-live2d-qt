#include "ModelManagerDialog.h"

#include "core/Configuration.h"
#include "core/ModelManager.h"
#include "core/ModelInfo.h"

#include <QDir>
#include <QDirIterator>
#include <QFileDialog>
#include <QFont>
#include <QFrame>
#include <QHBoxLayout>
#include <QLabel>
#include <QListWidget>
#include <QMessageBox>
#include <QPushButton>
#include <QVBoxLayout>

static const char *kDialogStyle = R"(
    QDialog {
        background-color: #1e1e2e;
    }
    QListWidget {
        background-color: #1e1e2e;
        border: none;
        outline: none;
        padding: 8px;
    }
    QListWidget::item {
        background-color: #313244;
        border-radius: 12px;
        padding: 8px;
        margin: 4px;
        color: #cdd6f4;
    }
    QListWidget::item:selected {
        background-color: #585b70;
        border: 2px solid #89b4fa;
    }
    QListWidget::item:hover {
        background-color: #45475a;
    }
    QLabel {
        color: #cdd6f4;
    }
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
    QFrame#detailPanel {
        background-color: #181825;
        border-radius: 16px;
        padding: 16px;
    }
    QLabel#previewImage {
        background-color: #11111b;
        border-radius: 12px;
    }
    QLabel#modelName {
        font-size: 20px;
        font-weight: 700;
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
    QLabel#emptyHint {
        font-size: 14px;
        color: #6c7086;
    }
)";

ModelManagerDialog::ModelManagerDialog(ModelManager *manager,
                                       Configuration *config,
                                       QWidget *parent)
    : QDialog(parent)
    , m_manager(manager)
    , m_config(config)
{
    setWindowTitle(QStringLiteral("Model Manager"));
    setMinimumSize(780, 520);
    resize(860, 560);
    setupUi();

    connect(m_manager, &ModelManager::modelsChanged,
            this, &ModelManagerDialog::refreshList);

    refreshList();
}

void ModelManagerDialog::setupUi()
{
    setStyleSheet(QString::fromUtf8(kDialogStyle));

    auto *root = new QHBoxLayout(this);
    root->setSpacing(0);
    root->setContentsMargins(12, 12, 12, 12);

    // === Left: model grid ===
    auto *leftPanel = new QVBoxLayout();
    leftPanel->setSpacing(8);

    auto *headerLayout = new QHBoxLayout();
    auto *title = new QLabel(QStringLiteral("Models"), this);
    QFont titleFont = title->font();
    titleFont.setPointSize(18);
    titleFont.setBold(true);
    title->setFont(titleFont);
    title->setStyleSheet(QStringLiteral("color: #cdd6f4; padding-left: 8px;"));
    headerLayout->addWidget(title);
    headerLayout->addStretch();

    m_importButton = new QPushButton(QStringLiteral("+ Import"), this);
    connect(m_importButton, &QPushButton::clicked,
            this, &ModelManagerDialog::onImport);
    headerLayout->addWidget(m_importButton);
    leftPanel->addLayout(headerLayout);

    m_listWidget = new QListWidget(this);
    m_listWidget->setViewMode(QListView::IconMode);
    m_listWidget->setIconSize(QSize(100, 100));
    m_listWidget->setGridSize(QSize(130, 140));
    m_listWidget->setResizeMode(QListView::Adjust);
    m_listWidget->setSpacing(6);
    m_listWidget->setWordWrap(true);
    m_listWidget->setSelectionMode(QAbstractItemView::SingleSelection);
    m_listWidget->setMovement(QListView::Static);
    m_listWidget->setMinimumWidth(420);
    connect(m_listWidget, &QListWidget::currentItemChanged,
            this, [this](QListWidgetItem *, QListWidgetItem *) {
        onSelectionChanged();
    });
    connect(m_listWidget, &QListWidget::itemDoubleClicked,
            this, [this](QListWidgetItem *) {
        onApply();
    });
    leftPanel->addWidget(m_listWidget);

    root->addLayout(leftPanel, 3);
    root->addSpacing(12);

    // === Right: detail panel ===
    auto *detailFrame = new QFrame(this);
    detailFrame->setObjectName(QStringLiteral("detailPanel"));
    detailFrame->setFixedWidth(260);
    auto *detailLayout = new QVBoxLayout(detailFrame);
    detailLayout->setSpacing(12);
    detailLayout->setContentsMargins(16, 16, 16, 16);

    // Preview image
    m_previewImage = new QLabel(detailFrame);
    m_previewImage->setObjectName(QStringLiteral("previewImage"));
    m_previewImage->setFixedSize(228, 228);
    m_previewImage->setAlignment(Qt::AlignCenter);
    m_previewImage->setScaledContents(false);
    detailLayout->addWidget(m_previewImage, 0, Qt::AlignCenter);

    // Model name
    m_modelName = new QLabel(detailFrame);
    m_modelName->setObjectName(QStringLiteral("modelName"));
    m_modelName->setAlignment(Qt::AlignCenter);
    detailLayout->addWidget(m_modelName);

    // Separator
    auto *sep = new QFrame(detailFrame);
    sep->setFrameShape(QFrame::HLine);
    sep->setStyleSheet(QStringLiteral("color: #313244;"));
    detailLayout->addWidget(sep);

    // Info rows
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

    // Buttons
    m_applyButton = new QPushButton(QStringLiteral("Use This Model"), detailFrame);
    m_applyButton->setObjectName(QStringLiteral("applyButton"));
    m_applyButton->setCursor(Qt::PointingHandCursor);
    connect(m_applyButton, &QPushButton::clicked,
            this, &ModelManagerDialog::onApply);
    detailLayout->addWidget(m_applyButton);

    m_deleteButton = new QPushButton(QStringLiteral("Delete"), detailFrame);
    m_deleteButton->setObjectName(QStringLiteral("deleteButton"));
    m_deleteButton->setCursor(Qt::PointingHandCursor);
    connect(m_deleteButton, &QPushButton::clicked,
            this, &ModelManagerDialog::onDelete);
    detailLayout->addWidget(m_deleteButton);

    root->addWidget(detailFrame);
}

void ModelManagerDialog::refreshList()
{
    m_listWidget->clear();

    const QList<ModelInfo> models = m_manager->models();
    const QString currentModelId = m_config->modelId();
    QListWidgetItem *selectedItem = nullptr;

    for (const ModelInfo &info : models) {
        auto *item = new QListWidgetItem(info.displayName, m_listWidget);

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
        m_listWidget->setCurrentItem(selectedItem);
    } else if (m_listWidget->count() > 0) {
        m_listWidget->setCurrentRow(0);
    }
}

void ModelManagerDialog::onSelectionChanged()
{
    QListWidgetItem *current = m_listWidget->currentItem();
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

    // Count motions and textures
    QDir modelDir(info.path);
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

    // Preview thumbnail
    QPixmap thumb = loadThumbnail(info.path);
    if (!thumb.isNull()) {
        m_previewImage->setPixmap(
            thumb.scaled(220, 220, Qt::KeepAspectRatio, Qt::SmoothTransformation));
    } else {
        m_previewImage->setText(info.displayName);
    }
}

QPixmap ModelManagerDialog::loadThumbnail(const QString &modelPath) const
{
    if (m_thumbnailCache.contains(modelPath)) {
        return m_thumbnailCache[modelPath];
    }

    // Look for texture files — prefer the first PNG found in a texture subdirectory
    QStringList patterns = {
        QStringLiteral("*.png"),
    };
    QDirIterator it(modelPath, patterns, QDir::Files, QDirIterator::Subdirectories);
    while (it.hasNext()) {
        it.next();
        QPixmap pix(it.filePath());
        if (!pix.isNull()) {
            const_cast<ModelManagerDialog *>(this)->m_thumbnailCache[modelPath] = pix;
            return pix;
        }
    }

    return {};
}

QString ModelManagerDialog::formatSize(qint64 bytes) const
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

void ModelManagerDialog::onImport()
{
    const QString dir = QFileDialog::getExistingDirectory(
        this,
        QStringLiteral("Select Model Directory"),
        QString(),
        QFileDialog::ShowDirsOnly);

    if (dir.isEmpty()) {
        return;
    }

    if (!m_manager->importModel(dir)) {
        QMessageBox::warning(this,
                             QStringLiteral("Import Failed"),
                             QStringLiteral("Failed to import model. Make sure the directory "
                                            "contains a valid .model3.json file."));
    }
}

void ModelManagerDialog::onDelete()
{
    QListWidgetItem *current = m_listWidget->currentItem();
    if (!current) {
        return;
    }

    const QString modelId = current->data(Qt::UserRole).toString();
    const QString displayName = current->text();

    QMessageBox::StandardButton answer = QMessageBox::question(
        this,
        QStringLiteral("Delete Model"),
        QStringLiteral("Are you sure you want to delete \"%1\"?").arg(displayName),
        QMessageBox::Yes | QMessageBox::No,
        QMessageBox::No);

    if (answer == QMessageBox::Yes) {
        m_manager->deleteModel(modelId);
    }
}

void ModelManagerDialog::onApply()
{
    QListWidgetItem *current = m_listWidget->currentItem();
    if (!current) {
        return;
    }

    const QString modelId = current->data(Qt::UserRole).toString();
    m_config->setModelId(modelId);
    m_config->save();
    emit modelSelected(modelId);
    close();
}
