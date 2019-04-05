#ifndef GENERIC_MSGFORMATTER_H
#define GENERIC_MSGFORMATTER_H

class QDateTime;
class CuData;
class CuVariant;

#include <QString>
#include <historyentry.h>
#include "../../cumbia-telegram-defs.h"

class GenMsgFormatter
{
public:
    enum FormatOption { Short, Medium, Long, AllCuDataKeys, MaxOption = 32 };

    GenMsgFormatter();

    QString fromData(const CuData& d, FormatOption f = Short);

    QString error(const QString& origin, const QString& message);

    QString qualityString() const;  // available if fromData is called

    QString source() const;  // available if fromData is called

    QString value() const;  // available if fromData is called

    QString timeRepr(const QDateTime& dt) const;


    QString hostChanged(const QString& host, bool success, const QString& description) const;

    QString host(const QString& host) const;

    QString volatileOpExpired(const QString &opnam, const QString &text) const;

    QString unauthorized(const QString& username, const char* op_type, const QString& reason) const;

    QString fromControlData(const ControlMsg::Type t, const QString& msg) const;

    QString help(int moduletype) const;

    QString aliasInsert(bool success, const QStringList &alias_parts, const QString& additional_message) const;

    QString botShutdown();

private:
    QString m_quality, m_value, m_src;


    QString m_getVectorInfo(const CuVariant& v);

    void m_cleanSource(const QString& s, QString &point, QString &device, QString& host, GenMsgFormatter::FormatOption f) const;
};

#endif // MSGFORMATTER_H
