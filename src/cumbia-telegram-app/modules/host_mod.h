#ifndef HOSTMOD_H
#define HOSTMOD_H

#include "../lib/cubotmodule.h"
#include "../lib/hostentry.h"

class HostMod : public CuBotModule
{
public:
    enum Type { HostModule = 120 };

    enum State { Undefined, GetHost, SetHost, GetHostList };

    HostMod(CuBotModuleListener *lis, BotDb *db, BotConfig *conf = nullptr);

    void reset();

    // CuBotModule interface
public:
    int type() const;
    QString name() const;
    QString description() const;
    QString help() const;
    int decode(const TBotMsg &msg);
    bool process();
    bool error() const;
    QString message() const;

private:
    QString m_host,m_text;
    int m_chat_id, m_user_id;
    int m_host_id;
    State m_state;
    QString m_msg;
    bool m_err;
    QString m_hostChanged_msg(const QString &host, bool success, const QString &description, const QString &errormsg = "") const;
    QString m_hostList_msg(const QList<HostEntry> &hli, bool success, const QStringList& descs) const;
    QString m_host_msg(const QString &host) const;
};

#endif // HOSTMOD_H
