#ifndef ALIAS_MOD_H
#define ALIAS_MOD_H

#include <QList>
#include "lib/cubotmodule.h"

class QSqlDatabase;
class QStringList;
class AliasProcPrivate;

class AliasEntry
{
public:
    AliasEntry(const QString& nam, const QString& repl, const QString& descrip);
    AliasEntry();

    QString name, replaces, description;

    bool in_history;
    QStringList in_history_hosts;
    QList<int> in_history_idxs;
};

class AliasModule : public CuBotModule
{
public:
    enum Type { AliasProcType = 2 }; // very small number: processed early!

    enum Mode { Invalid, SetAlias, ShowAlias, DelAlias, ExecAlias, AliasCmdErr, AliasFindAndReplace, NoOp };

    AliasModule(BotDb *db, BotConfig *conf, CuBotModuleListener *l);
    ~AliasModule();

    QString findAndReplace(const QString& in, const QList<AliasEntry>& aliases);
    QList<AliasEntry> getAlias(int user_id, const QString &name, const QSqlDatabase &m_db);

    // reinit attributes
    void reset();

    // CuBotModule interface
public:
    void setBotmoduleListener(CuBotModuleListener *l);
    int type() const;
    QString name() const;
    QString description() const;
    QString help() const;
    void setDb(BotDb *db);
    void setConf(BotConfig *conf);
    int decode(const TBotMsg &msg);
    bool process();
    bool error() const;
    QString message() const;
    bool isVolatileOperation() const;
private:
    AliasProcPrivate *d;


    bool m_db_insertAlias(int user_id, const QStringList& parts, int max_alias_cnt);
    HistoryEntry m_db_getFromHistory(int user_id, int index);
    QString m_aliasInsertMsg(bool success, const QStringList &alias_parts, const QString &additional_message) const;
    QString m_aliasListMsg(const QString &name, const QList<AliasEntry> &alist) const;
};

#endif // ALIASPROC_H
