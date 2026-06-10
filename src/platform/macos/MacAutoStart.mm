#include "platform/AutoStart.h"

#import <Foundation/Foundation.h>
#include <QCoreApplication>
#include <QDir>
#include <QFile>

static QString plistPath()
{
    return QDir::homePath()
           + QStringLiteral("/Library/LaunchAgents/com.qdesktoppet.QDesktopPet.plist");
}

bool AutoStart::isEnabled()
{
    return QFile::exists(plistPath());
}

void AutoStart::setEnabled(bool enabled)
{
    const QString path = plistPath();

    if (!enabled) {
        QFile::remove(path);
        return;
    }

    QDir().mkpath(QDir::homePath() + QStringLiteral("/Library/LaunchAgents"));

    NSString *execPath = QCoreApplication::applicationFilePath().toNSString();

    NSDictionary *plist = @{
        @"Label": @"com.qdesktoppet.QDesktopPet",
        @"ProgramArguments": @[execPath],
        @"RunAtLoad": @YES,
        @"KeepAlive": @NO,
    };

    NSData *data = [NSPropertyListSerialization
        dataWithPropertyList:plist
        format:NSPropertyListXMLFormat_v1_0
        options:0
        error:nil];

    [data writeToFile:path.toNSString() atomically:YES];
}
