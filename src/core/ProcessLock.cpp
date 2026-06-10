#include "ProcessLock.h"
#include "AppPaths.h"

ProcessLock::ProcessLock()
{
    AppPaths::ensureDirsExist();
    m_lockFile = std::make_unique<QLockFile>(AppPaths::lockFile());
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
