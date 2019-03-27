#include "host_mod.h"
#include "../lib/botdb.h"
#include "../lib/datamsgformatter.h"
#include "../lib/generic_msgformatter.h"
#include "moduleutils.h"

#include <cumacros.h>
#include <QRegularExpression>
#include <QRegularExpressionMatch>

HostMod::HostMod(CuBotModuleListener *lis, BotDb *db, BotConfig *conf)
    : CuBotModule (lis, db, conf)
{
    reset();
}

void HostMod::reset()
{
    m_state = Undefined;
    m_host = m_text = m_msg = QString();
    m_chat_id = m_user_id = m_host_id = -1;
    m_err = false;
}

int HostMod::type() const
{
    return HostModule;
}

QString HostMod::name() const
{
    return "Host";
}

QString HostMod::description() const
{
    return "provides utilities to get and set the host name that some engines require";
}

QString HostMod::help() const
{
    return "host";
}

int HostMod::decode(const TBotMsg &msg)
{
    reset();
    m_chat_id = msg.chat_id;
    m_user_id = msg.user_id;
    m_text = msg.text();
    if(m_text == "host" || m_text == "/host") {
        m_state = GetHost;
    }
    else if(m_text == "hosts" || m_text == "/hosts") {
        m_state = GetHostList;
    }
    else {
        QRegularExpression re("\\bhost(?:\\s+|\\s*=\\s*)([A-Za-z_0-9\\:\\.\\-]*)");
        QRegularExpressionMatch match = re.match(m_text);
        if(match.hasMatch()) {
            m_host = match.captured(1);
            m_state = SetHost;
        }
        else {
            re.setPattern("/(?:H)(\\d{1,2})\\b");
            QRegularExpressionMatch match = re.match(m_text);
            if(match.hasMatch()) {
                m_host_id = match.captured(1).toInt();
                m_state = SetHost;
            }
        }
    }
    if(m_state != Undefined)
        return  type();
    return -1;
}

bool HostMod::process()
{
    QString new_host = m_host;
    m_err = false;
    BotDb *db = getDb();
    if(m_state == GetHost) {
        QString host = ModuleUtils().getHost(m_chat_id, db);
        getModuleListener()->onSendMessageRequest(m_chat_id, m_host_msg(host));
    }
    else if(m_state == SetHost) {
        QString new_host_description;
        if(new_host.length() > 0 && m_host_id < 0)
            m_err = !db->setHost(m_user_id, m_chat_id, new_host, new_host_description);
        else if(m_host_id >= 0) // setHost by host id, new_host will contain the name
            m_err = !db->setHost(m_user_id, m_chat_id, m_host_id, new_host, new_host_description);
        if(!m_err)
            m_err = !db->addToHistory(HistoryEntry(m_user_id, m_text, "host", "", "")); // "host" is type, `host` and `description` are empty
        getModuleListener()->onSendMessageRequest(m_chat_id, m_hostChanged_msg(new_host, !m_err, new_host_description, db->message()), true);
    }
    else if(m_state == GetHostList) {
        QStringList descs;
        QList<HostEntry> hli = db->getHostList();
        m_err = db->error();
        getModuleListener()->onSendMessageRequest(m_chat_id, m_hostList_msg(hli, !m_err, descs), true);
    }
    if(m_err)
        m_msg = db->message();
    return !m_err;
}

bool HostMod::error() const
{
    return m_err;
}

QString HostMod::message() const
{
    return m_msg;
}

QString HostMod::m_hostChanged_msg(const QString &host, bool success, const QString &description, const QString& errormsg) const
{
    DataMsgFormatter dmsgf;
    QString s = "<i>" + dmsgf.timeRepr(QDateTime::currentDateTime()) + "</i>\n";
    if(success) {
        s += QString("successfully set host to <b>%1</b>:\n<i>%2</i>").arg(host).arg(description);
    }
    else {
        s += "👎   failed to set host to <b>" + host + "</b>:\n<i>" + errormsg + "</i>";
    }
    return s;
}

QString HostMod::m_hostList_msg(const QList<HostEntry> &hli, bool success, const QStringList &descs) const
{
    DataMsgFormatter dmsgf;
    QString s;
    if(success) {
        s += QString("<b>host list</b>   <i>" + dmsgf.timeRepr(QDateTime::currentDateTime()) + "</i>\n\n");
        for(int i = 0; i < hli.size(); i++) {
            s += QString("- <i>%1</i> [/H%2]  * <i>%3</i> ]\n\n").arg(hli[i].name)
                    .arg(hli[i].index).arg(hli[i].description);
        }
    }
    else {
        s += "👎   failed to get host list";
    }
    return s;
}

QString HostMod::m_host_msg(const QString &host) const
{
    QString s;
    s = "host is set to <b>" + host + "</b>:\n";
    s += "It can be changed with:\n"
         "<i>host tango-host:PORT_NUMBER</i>";
    return s;
}
