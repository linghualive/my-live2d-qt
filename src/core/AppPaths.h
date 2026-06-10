#ifndef APPPATHS_H
#define APPPATHS_H

#include <QDir>
#include <QString>

namespace AppPaths {

inline QString baseDir()
{
    return QDir::homePath() + QStringLiteral("/.qdesktoppet/");
}

inline QString modelsDir()
{
    return baseDir() + QStringLiteral("models/");
}

inline QString configFile()
{
    return baseDir() + QStringLiteral("config.ini");
}

inline QString lockFile()
{
    return baseDir() + QStringLiteral("QDesktopPet.lock");
}

inline QString pendingModelFile()
{
    return baseDir() + QStringLiteral(".pending_model");
}

inline QString modelBlacklistFile()
{
    return baseDir() + QStringLiteral(".model_blacklist");
}

inline void ensureDirsExist()
{
    QDir().mkpath(baseDir());
    QDir().mkpath(modelsDir());
}

}

#endif // APPPATHS_H
