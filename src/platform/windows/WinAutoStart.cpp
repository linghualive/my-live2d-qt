#include "platform/AutoStart.h"

#include <QCoreApplication>
#include <QString>
#include <windows.h>

static const wchar_t *kRunKey = L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Run";
static const wchar_t *kAppName = L"QDesktopPet";

bool AutoStart::isEnabled()
{
    HKEY hKey = nullptr;
    if (RegOpenKeyExW(HKEY_CURRENT_USER, kRunKey, 0, KEY_READ, &hKey) != ERROR_SUCCESS) {
        return false;
    }

    wchar_t value[MAX_PATH] = {};
    DWORD size = sizeof(value);
    DWORD type = 0;
    LSTATUS status = RegQueryValueExW(hKey, kAppName, nullptr, &type,
                                       reinterpret_cast<BYTE *>(value), &size);
    RegCloseKey(hKey);

    if (status != ERROR_SUCCESS || type != REG_SZ) {
        return false;
    }

    QString regPath = QString::fromWCharArray(value);
    return regPath == QCoreApplication::applicationFilePath().replace('/', '\\');
}

void AutoStart::setEnabled(bool enabled)
{
    HKEY hKey = nullptr;
    if (RegOpenKeyExW(HKEY_CURRENT_USER, kRunKey, 0, KEY_WRITE, &hKey) != ERROR_SUCCESS) {
        return;
    }

    if (enabled) {
        QString appPath = QCoreApplication::applicationFilePath().replace('/', '\\');
        const auto wPath = appPath.toStdWString();
        RegSetValueExW(hKey, kAppName, 0, REG_SZ,
                       reinterpret_cast<const BYTE *>(wPath.c_str()),
                       static_cast<DWORD>((wPath.size() + 1) * sizeof(wchar_t)));
    } else {
        RegDeleteValueW(hKey, kAppName);
    }

    RegCloseKey(hKey);
}
