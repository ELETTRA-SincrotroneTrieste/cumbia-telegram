#ifndef DATAMSGFORMATTER_H
#define DATAMSGFORMATTER_H

#include <QString>

class CuData;
class QDateTime;
class CuVariant;

class DataMsgFormatter
{
public:
    enum FormatOption { FormatShort, FormatLong };
    QString fromData_msg(const CuData &d, FormatOption f, const QString& description = QString());
    QString getVectorInfo(const CuVariant &v);
    QString timeRepr(const QDateTime &dt) const;
    void cleanSource(const QString &src, QString &point, QString &device, QString &host, FormatOption f) const;
private:
};

#endif // DATAMSGFORMATTER_H
