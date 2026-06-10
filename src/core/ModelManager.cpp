#include "ModelManager.h"
#include "AppPaths.h"

#include <QDateTime>
#include <QDir>
#include <QDirIterator>
#include <QFile>
#include <QFileInfo>
#include <QJsonDocument>
#include <QJsonObject>
#include <QStandardPaths>

ModelManager::ModelManager(QObject *parent)
    : QObject(parent)
{
}

QString ModelManager::modelsBaseDir()
{
    AppPaths::ensureDirsExist();
    return AppPaths::modelsDir();
}

void ModelManager::migrateOldModelsDir()
{
    const QString oldBase = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation)
                            + QStringLiteral("/models/");
    const QDir oldDir(oldBase);
    if (!oldDir.exists()) {
        return;
    }

    const QString newBase = AppPaths::modelsDir();
    const QStringList subdirs = oldDir.entryList(QDir::Dirs | QDir::NoDotAndDotDot);
    for (const QString &sub : subdirs) {
        const QString src = oldDir.absoluteFilePath(sub);
        const QString dst = newBase + sub;
        if (!QDir(dst).exists()) {
            copyDirectoryRecursively(src, dst);
        }
    }
}

void ModelManager::scanModels()
{
    migrateOldModelsDir();
    m_models.clear();

    const QDir baseDir(modelsBaseDir());
    const QStringList subdirs = baseDir.entryList(QDir::Dirs | QDir::NoDotAndDotDot);

    for (const QString &dirName : subdirs) {
        const QString dirPath = baseDir.absoluteFilePath(dirName);
        const QDir modelDir(dirPath);

        // Look for *.model3.json files
        const QStringList model3Files = modelDir.entryList(
            QStringList() << QStringLiteral("*.model3.json"),
            QDir::Files);

        if (model3Files.isEmpty()) {
            continue;
        }

        const QString modelFileName = model3Files.first();
        const QString modelFilePath = modelDir.absoluteFilePath(modelFileName);

        // Parse the model3.json to get the display name
        QString displayName = dirName; // fallback

        QFile jsonFile(modelFilePath);
        if (jsonFile.open(QIODevice::ReadOnly)) {
            const QJsonDocument doc = QJsonDocument::fromJson(jsonFile.readAll());
            jsonFile.close();

            if (doc.isObject()) {
                const QJsonObject obj = doc.object();
                const QString name = obj.value(QStringLiteral("Name")).toString();
                if (!name.isEmpty()) {
                    displayName = name;
                }
            }
        }

        // Read import date from .metadata.json if available
        QDateTime importDate;
        const QString metadataPath = modelDir.absoluteFilePath(QStringLiteral(".metadata.json"));
        QFile metadataFile(metadataPath);
        if (metadataFile.open(QIODevice::ReadOnly)) {
            const QJsonDocument metaDoc = QJsonDocument::fromJson(metadataFile.readAll());
            metadataFile.close();

            if (metaDoc.isObject()) {
                const QString dateStr = metaDoc.object()
                                            .value(QStringLiteral("importDate"))
                                            .toString();
                importDate = QDateTime::fromString(dateStr, Qt::ISODate);
            }
        }

        ModelInfo info;
        info.id = dirName;
        info.displayName = displayName;
        info.path = dirPath;
        info.modelFile = modelFileName;
        info.sizeBytes = calculateDirSize(dirPath);
        info.importDate = importDate;

        m_models.append(info);
    }

    emit modelsChanged();
}

QList<ModelInfo> ModelManager::models() const
{
    return m_models;
}

ModelInfo ModelManager::modelInfo(const QString &modelId) const
{
    for (const ModelInfo &info : m_models) {
        if (info.id == modelId) {
            return info;
        }
    }
    return ModelInfo();
}

bool ModelManager::importModel(const QString &sourceDir)
{
    const QDir srcDir(sourceDir);
    if (!srcDir.exists()) {
        return false;
    }

    // Validate that the source directory contains a .model3.json file
    const QStringList model3Files = srcDir.entryList(
        QStringList() << QStringLiteral("*.model3.json"),
        QDir::Files);

    if (model3Files.isEmpty()) {
        return false;
    }

    const QString dirName = srcDir.dirName();
    const QString destPath = modelsBaseDir() + dirName;

    // If destination already exists, remove it first
    QDir destDir(destPath);
    if (destDir.exists()) {
        destDir.removeRecursively();
    }

    // Copy the entire directory recursively
    copyDirectoryRecursively(sourceDir, destPath);

    // Write .metadata.json with import date
    const QString metadataPath = destPath + QStringLiteral("/.metadata.json");
    QFile metadataFile(metadataPath);
    if (metadataFile.open(QIODevice::WriteOnly)) {
        QJsonObject metaObj;
        metaObj[QStringLiteral("importDate")] = QDateTime::currentDateTime().toString(Qt::ISODate);

        const QJsonDocument metaDoc(metaObj);
        metadataFile.write(metaDoc.toJson(QJsonDocument::Indented));
        metadataFile.close();
    }

    scanModels();
    return true;
}

bool ModelManager::deleteModel(const QString &modelId)
{
    const QString modelPath = modelsBaseDir() + modelId;
    QDir dir(modelPath);
    if (!dir.exists()) {
        return false;
    }

    const bool removed = dir.removeRecursively();
    if (removed) {
        scanModels();
    }
    return removed;
}

qint64 ModelManager::calculateDirSize(const QString &dirPath)
{
    qint64 totalSize = 0;
    QDirIterator it(dirPath, QDir::Files | QDir::Hidden, QDirIterator::Subdirectories);
    while (it.hasNext()) {
        it.next();
        totalSize += it.fileInfo().size();
    }
    return totalSize;
}

void ModelManager::copyDirectoryRecursively(const QString &srcPath, const QString &dstPath)
{
    QDir srcDir(srcPath);
    QDir dstDir(dstPath);

    if (!dstDir.exists()) {
        dstDir.mkpath(QStringLiteral("."));
    }

    // Copy files
    const QStringList files = srcDir.entryList(QDir::Files | QDir::Hidden);
    for (const QString &fileName : files) {
        const QString srcFilePath = srcDir.absoluteFilePath(fileName);
        const QString dstFilePath = dstDir.absoluteFilePath(fileName);
        QFile::copy(srcFilePath, dstFilePath);
    }

    // Recurse into subdirectories
    const QStringList subdirs = srcDir.entryList(QDir::Dirs | QDir::NoDotAndDotDot);
    for (const QString &subDir : subdirs) {
        const QString srcSubPath = srcDir.absoluteFilePath(subDir);
        const QString dstSubPath = dstDir.absoluteFilePath(subDir);
        copyDirectoryRecursively(srcSubPath, dstSubPath);
    }
}
