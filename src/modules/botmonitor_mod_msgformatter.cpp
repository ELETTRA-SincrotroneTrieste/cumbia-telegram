#include "botmonitor_mod_msgformatter.h"
#include "../lib/formulahelper.h"
#include "../lib/datamsgformatter.h" // for "timeRepr"

BotmonitorMsgFormatter::BotmonitorMsgFormatter()
{

}

QString BotmonitorMsgFormatter::formulaChanged(const QString &src, const QString &old, const QString &new_f)
{
    FormulaHelper fh;
    QString s;
    if(!old.isEmpty() && new_f.isEmpty())
        s = "formula <i>" + fh.escape(old) + "</i> has been <b>removed</b>";
    else if(old.isEmpty() && new_f.size() > 0)
        s = "formula <b>" + fh.escape(new_f)  + "</b> has been introduced";
    else
        s = "formula <i>" + fh.escape(old) + "</i>\nchanged into\n<b>" + fh.escape(new_f) + "</b>";
    return s;
}

QString BotmonitorMsgFormatter::monitorTypeChanged(const QString &old_cmd, const QString &new_cmd)
{
    QString s;
    FormulaHelper fh;
    s = "☝   <i>" + fh.escape(old_cmd) + "</i> changed into <b>" + fh.escape(new_cmd) + "</b>";
    return s;
}

QString BotmonitorMsgFormatter::srcMonitorStartError(const QString &src, const QString &message) const
{
    QString s = QString("👎   failed to start monitor for <i>%1</i>:\n"
                        "<b>%2</b>").arg(FormulaHelper().escape(src)).arg(message);
    return s;
}

QString BotmonitorMsgFormatter::monitorUntil(const QString &src, const QDateTime &until) const
{
    QString m = FormulaHelper().escape(src);
    return QString("🕐   started monitoring <i>%1</i> until <i>%2</i>").arg(m).arg(DataMsgFormatter().timeRepr(until));
}

QString BotmonitorMsgFormatter::monitorStopped(const QString &cmd, const QString &msg) const
{
    FormulaHelper fh;
    QString m = fh.escape(cmd);
    return "stopped monitoring <i>" + m + "</i>: <i>" + fh.escape(msg) + "</i>";
}

QString BotmonitorMsgFormatter::alreadyMonitoring(const QString &src, const QString &host) const
{
    FormulaHelper fh;
    QString m = fh.escape(src);
    return "the source " + m + " is already being monitored [<i>" + host + "</i>]";
}
