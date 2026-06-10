#ifndef PREFERENCESDIALOG_H
#define PREFERENCESDIALOG_H

#include <QDialog>
#include <QMap>
#include <QPixmap>

class QListWidget;
class QListWidgetItem;
class QStackedWidget;
class QLabel;
class QPushButton;
class QSlider;
class QSpinBox;
class QRadioButton;
class QCheckBox;
class QComboBox;
class ModelManager;
class Configuration;

class PreferencesDialog : public QDialog {
    Q_OBJECT

public:
    explicit PreferencesDialog(ModelManager *manager,
                               Configuration *config,
                               QWidget *parent = nullptr);
    ~PreferencesDialog() override = default;

public slots:
    void setPreviewImage(const QImage &image);

signals:
    void modelSelected(const QString &modelId);
    void modelPreviewRequested(const QString &modelId);
    void settingsChanged();

private slots:
    void refreshModelList();
    void onModelSelectionChanged();
    void onImport();
    void onDelete();
    void onApply();
    void onSettingsAccepted();

private:
    void setupUi();
    QWidget *createModelsPage();
    QWidget *createSettingsPage();
    void loadSettingsFromConfig();
    QPixmap loadThumbnail(const QString &modelPath) const;
    QString formatSize(qint64 bytes) const;

    ModelManager *m_manager;
    Configuration *m_config;

    // Navigation
    QListWidget *m_navList;
    QStackedWidget *m_stack;

    // Models page
    QListWidget *m_modelGrid;
    QLabel *m_previewImage;
    QLabel *m_modelName;
    QLabel *m_modelSize;
    QLabel *m_modelMotions;
    QLabel *m_modelTextures;
    QPushButton *m_previewButton;
    QPushButton *m_applyButton;
    QPushButton *m_deleteButton;
    QPushButton *m_importButton;

    // Settings page
    QSlider *m_sensibilitySlider;
    QLabel *m_sensibilityValueLabel;
    QSpinBox *m_widthSpin;
    QSpinBox *m_heightSpin;
    QRadioButton *m_leftRadio;
    QRadioButton *m_rightRadio;
    QCheckBox *m_hideOnHoverCheck;
    QComboBox *m_frameRateCombo;
    QCheckBox *m_autoStartCheck;

    QMap<QString, QPixmap> m_thumbnailCache;
};

#endif // PREFERENCESDIALOG_H
