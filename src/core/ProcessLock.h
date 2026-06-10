#ifndef PROCESSLOCK_H
#define PROCESSLOCK_H

#include <QLockFile>
#include <QStandardPaths>
#include <memory>

class ProcessLock {
public:
    ProcessLock();
    ~ProcessLock();
    bool tryLock();
    void unlock();
private:
    std::unique_ptr<QLockFile> m_lockFile;
};

#endif // PROCESSLOCK_H
