#ifndef ABOUTDATA_H
#define ABOUTDATA_H

#include <QString>

class AboutData
{
public:
    const static QString& Organization;
    const static QString& ComponentName;
    const static QString& ShortName;
    const static QString& DisplayName;
    const static QString& Version;
    const static QString& ShortDescription;
    const static QString& CopyrightStatement;
    const static QString& BugAddress;

    static QString applicationName()
    {
        return ComponentName + '_' + Version;
    }
};

#endif // ABOUTDATA_H
