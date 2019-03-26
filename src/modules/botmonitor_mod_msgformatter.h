#ifndef BOTMONITORMSGFORMATTER_H
#define BOTMONITORMSGFORMATTER_H

#include <QString>

class QDateTime;

class BotmonitorMsgFormatter
{
public:
    BotmonitorMsgFormatter();

    QString formulaChanged(const QString &src, const QString &old, const QString &new_f);
    QString monitorTypeChanged(const QString &old_cmd, const QString &new_cmd);
    QString srcMonitorStartError(const QString &src, const QString &message) const;
    QString monitorUntil(const QString& src, const QDateTime& until) const;
    QString monitorStopped(const QString& cmd, const QString& msg) const;
    QString alreadyMonitoring(const QString& src, const QString& host) const;
};

#endif // BOTMONITORMSGFORMATTER_H
