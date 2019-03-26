#ifndef HOSTENTRY_H
#define HOSTENTRY_H

#include <QString>

class HostEntry
{
public:
    HostEntry(const QString& nam, int idx, const QString& desc);

    QString name, description;
    int index;
};

#endif // HOSTENTRY_H
