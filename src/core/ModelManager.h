#ifndef MODELMANAGER_H
#define MODELMANAGER_H

#include "ModelInfo.h"

#include <QList>
#include <QObject>
#include <QString>

class ModelManager : public QObject {
    Q_OBJECT

public:
    explicit ModelManager(QObject *parent = nullptr);
    ~ModelManager() override = default;

    static QString modelsBaseDir();

    void scanModels();
    QList<ModelInfo> models() const;
    ModelInfo modelInfo(const QString &modelId) const;

    bool importModel(const QString &sourceDir);
    bool deleteModel(const QString &modelId);

signals:
    void modelsChanged();

private:
    static qint64 calculateDirSize(const QString &dirPath);
    static void copyDirectoryRecursively(const QString &srcPath, const QString &dstPath);
    static void migrateOldModelsDir();

    QList<ModelInfo> m_models;
};

#endif // MODELMANAGER_H
