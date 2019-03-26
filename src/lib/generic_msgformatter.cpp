#include "generic_msgformatter.h"
#include "formulahelper.h"
#include "botconfig.h"
#include <QDateTime>
#include <cudata.h>
#include <cudataquality.h>
#include <QFile>        // for help*.html in res/ resources
#include <QTextStream>  // for help*.html
#include <QtDebug>
#include <algorithm>

#define MAXVALUELEN 45

GenMsgFormatter::GenMsgFormatter()
{

}

QString GenMsgFormatter::error(const QString &origin, const QString &message)
{
    QString msg;
    FormulaHelper fh;
    msg += "👎   " + fh.escape(origin) + ": <i>" + fh.escape(message) + "</i>";
    return msg;
}

QString GenMsgFormatter::qualityString() const
{
    return m_quality;
}

QString GenMsgFormatter::source() const
{
    return m_src;
}

QString GenMsgFormatter::value() const
{
    return m_value;
}

QString GenMsgFormatter::hostChanged(const QString &host, bool success, const QString &description) const
{
    QString s = "<i>" + timeRepr(QDateTime::currentDateTime()) + "</i>\n";
    if(success) {
        s += QString("successfully set host to <b>%1</b>:\n<i>%2</i>").arg(host).arg(description);
    }
    else {
        s += "👎   failed to set host to <b>" + host + "</b>";
    }
    return s;
}

QString GenMsgFormatter::host(const QString &host) const
{
    QString s;
    s = "host is set to <b>" + host + "</b>:\n";
    s += "It can be changed with:\n"
         "<i>host tango-host:PORT_NUMBER</i>";
    return s;
}

QString GenMsgFormatter::volatileOpExpired(const QString &opnam, const QString &text) const
{
    QString s;
    s += QString("⌛️   info: data for the operation \"%1\" has been cleared\n"
                 "Please execute <i>%2</i> again if needed").arg(opnam).arg(text);
    return s;
}

QString GenMsgFormatter::unauthorized(const QString &username, const char *op_type, const QString &reason) const
{
    QString s = QString("❌   user <i>%1</i> (%2) <b>unauthorized</b>:\n<i>%3</i>")
            .arg(username).arg(op_type).arg(reason);

    return s;
}

QString GenMsgFormatter::fromControlData(const ControlMsg::Type t, const QString &msg) const
{
    QString s;
    if(t == ControlMsg::Authorized) {
        s = "🎉   <b>congratulations</b>: you have been <b>authorized</b> to access the cumbia-telegram bot";
    }
    else if(t == ControlMsg::AuthRevoked) {
        s = "❌   your authorization to interact with the cumbia-telegram bot has been <b>revoked</b>";
    }
    return s;
}



QString GenMsgFormatter::botShutdown()
{
    return "😴   bot has gone to <i>sleep</i>\n"
           "Monitors and alerts will be suspended";
}


QString GenMsgFormatter::timeRepr(const QDateTime &dt) const
{
    QString tr; // time repr
    QTime midnight = QTime(23, 59, 59);
    QDateTime today_23_59 = QDateTime::currentDateTime();
    today_23_59.setTime(midnight);
    QDateTime today_midnite = QDateTime::currentDateTime();
    today_midnite.setTime(QTime(0, 0));
    QDateTime yesterday_midnite = today_midnite.addDays(-1);
    QString date, time = dt.time().toString("hh:mm:ss");

    if(dt > today_23_59)
        date = "tomorrow";
    else if(dt >= today_midnite)
        date = "today";
    else if(dt >= yesterday_midnite)
        date = "yesterday";
    else
        date = dt.date().toString("yyyy.MM.dd");
    tr = date + " " + time;
    return tr;
}

QString GenMsgFormatter::m_getVectorInfo(const CuVariant &v)
{
    QString s;
    std::vector<double> vd;
    v.toVector<double>(vd);
    auto minmax = std::minmax_element(vd.begin(),vd.end());
    s += QString("vector size: <b>%1</b> min: <b>%2</b> max: <b>%3</b>").arg(vd.size())
            .arg(*minmax.first).arg(*minmax.second);
    return s;
}

