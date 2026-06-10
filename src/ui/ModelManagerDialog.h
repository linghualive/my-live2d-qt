#ifndef MODELMANAGERDIALOG_H
#define MODELMANAGERDIALOG_H

#include <QDialog>
#include <QMap>
#include <QPixmap>

class QListWidget;
class QListWidgetItem;
class QLabel;
class QPushButton;
class QStackedLayout;
class ModelManager;
class Configuration;

class ModelManagerDialog : public QDialog {
    Q_OBJECT

public:
    explicit ModelManagerDialog(ModelManager *manager,
                                Configuration *config,
                                QWidget *parent = nullptr);
    ~ModelManagerDialog() override = default;

signals:
    void modelSelected(const QString &modelId);

private slots:
    void refreshList();
    void onSelectionChanged();
    void onImport();
    void onDelete();
    void onApply();

private:
    void setupUi();
    QPixmap loadThumbnail(const QString &modelPath) const;
    QString formatSize(qint64 bytes) const;

    ModelManager *m_manager;
    Configuration *m_config;

    QListWidget *m_listWidget;
    QLabel *m_previewImage;
    QLabel *m_modelName;
    QLabel *m_modelSize;
    QLabel *m_modelMotions;
    QLabel *m_modelTextures;
    QPushButton *m_applyButton;
    QPushButton *m_deleteButton;
    QPushButton *m_importButton;

    QMap<QString, QPixmap> m_thumbnailCache;
};

#endif // MODELMANAGERDIALOG_H
