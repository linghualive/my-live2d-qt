#ifndef MODELINFO_H
#define MODELINFO_H

#include <QDateTime>
#include <QString>

struct ModelInfo {
    QString id;           // directory name
    QString displayName;  // from model3.json "Name" field, or fallback to id
    QString path;         // absolute path to model directory
    QString modelFile;    // the .model3.json filename
    qint64 sizeBytes = 0;
    QDateTime importDate;
};

#endif // MODELINFO_H
