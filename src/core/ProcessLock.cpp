#include "ProcessLock.h"

ProcessLock::ProcessLock()
{
    const QString lockPath = QStandardPaths::writableLocation(QStandardPaths::TempLocation)
                             + QStringLiteral("/QDesktopPet.lock");
    m_lockFile = std::make_unique<QLockFile>(lockPath);
}

ProcessLock::~ProcessLock()
{
    unlock();
}

bool ProcessLock::tryLock()
{
    return m_lockFile->tryLock(500);
}

void ProcessLock::unlock()
{
    if (m_lockFile && m_lockFile->isLocked()) {
        m_lockFile->unlock();
    }
}
