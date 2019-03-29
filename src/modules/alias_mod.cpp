#include "alias_mod.h"
#include <QSqlDatabase>
#include <QtSql>
#include <QRegularExpression>
#include <cumacros.h>
#include <botdb.h>
#include <formulahelper.h>

AliasEntry::AliasEntry()
{
    in_history = false;
    index = -1;
}

AliasEntry::AliasEntry(const QString &nam, const QString &repl, const QString &descrip, int idx)
{
     description = descrip;
     replaces = repl;
     name = nam;
     in_history = false;
     index = idx;
}

class AliasProcPrivate {
public:
    CuBotModuleListener *listener;
    QString msg, db_msg;
    QSqlDatabase qsqld;
    QString out;
    TBotMsg msg_in;
    int link_idx;
    AliasModule::Mode mode;
    QString name, replaces; // alias name, replacement
    BotConfig *bot_conf;
};


void AliasModule::reset() {
    d->link_idx = -1;
    d->replaces.clear();
    d->mode = Invalid;
    d->out.clear();
    d->name.clear();
    d->replaces.clear();
}

AliasModule::AliasModule(BotDb *db, BotConfig *conf, CuBotModuleListener *l)
    : CuBotModule(l, db, conf)
{
    d = new AliasProcPrivate;
    reset();
    setDb(db);
    setConf(conf);
    setBotmoduleListener(l);
}

AliasModule::~AliasModule(){
    delete d;
}

/**
 * @brief BotDb::getAlias returns the list of aliases for the given user id and optional alias name
 * @param user_id the user id
 * @param name the name of the alias you want to find
 * @return a list of AliasEntry ordered by the alias name length from longer to shorter
 *
 * \par Note
 * The list is ordered from longer to shorter alias name so that accessing the list
 * will return longer alias names first
 *
 */
QList<AliasEntry> AliasModule::getAlias(int user_id, const QString &name, const QSqlDatabase& m_db)
{
    bool err;
    QList<AliasEntry> ae;
    if(!m_db.isOpen())
        return ae;
    d->msg.clear();
    QSqlQuery q(m_db);
    QString query = QString("SELECT name,replaces,description,idx FROM alias WHERE user_id=%1 ").arg(user_id);
    !name.isEmpty() ?query += QString(" AND name='%1' ORDER BY timestamp ASC").arg(name)
            : query += "ORDER BY timestamp ASC";
    err = !q.exec(query);
    while(!err && q.next()) {
        AliasEntry e(q.value(0).toString(), q.value(1).toString(), q.value(2).toString(), q.value(3).toInt());
        QSqlQuery q2(m_db);
        err = !q2.exec(QString("SELECT host,h_idx FROM history WHERE user_id=%1 AND command='%2'")
                       .arg(user_id).arg(e.replaces));
        while(!err && q2.next()) {
            e.in_history = true;
            e.in_history_hosts << q2.value(0).toString();
            e.in_history_idxs << q2.value(1).toInt();
        }
        ae<< e;
    }
    if(err) {
        d->msg = "AliasProc.getAlias: error executing query " + q.lastQuery() + ": " + q.lastError().text();
        perr("%s", qstoc(d->msg));
    }
    return ae;
}

QString AliasModule::findAndReplace(const QString &in, const QList<AliasEntry> &aliases)
{
    QString out(in);
    foreach(const AliasEntry& e, aliases) {
        // \bdouble\b
        QRegularExpression re(QString("\\b%1\\b").arg(e.name));
        QRegularExpressionMatch match = re.match(out);
        if(match.hasMatch()) {
            QString capture = match.captured(0);
            capture.replace(e.name, e.replaces);
            out.replace(match.captured(0), capture);
        }
    }
    return out;
}


void AliasModule::setBotmoduleListener(CuBotModuleListener *l)
{
    d->listener = l;
}

int AliasModule::type() const
{
    return AliasProcType;
}

QString AliasModule::name() const
{
    return "alias";
}

QString AliasModule::description() const
{
    return "Alias anything with this module";
}

QString AliasModule::help() const
{
    return QString();
}

void AliasModule::setDb(BotDb *db)
{
    bool err = false;
    d->qsqld = *db->getSqlDatabase();
    if(!d->qsqld.tables().contains("alias")) {
        QSqlQuery q(d->qsqld);
        err = !q.exec("create table alias (idx INTEGER NOT NULL, user_id INTEGER NOT NULL, name TEXT NOT NULL, "
                      "replaces TEXT NOT NULL, description TEXT DEFAULT '', "
                      "timestamp DATETIME NOT NULL, PRIMARY KEY(user_id,name), UNIQUE(user_id,idx) )");
        if(err)
            d->msg = "AliasProc.setDb: failed to create table \"alias\": " + q.lastQuery() + ": " + q.lastError().text();
    }
}

void AliasModule::setConf(BotConfig *conf)
{
    d->bot_conf = conf;
}

int AliasModule::decode(const TBotMsg &msg)
{
    reset(); // clear data
    d->msg_in = msg;
    QString text = msg.text();
    if(text == "/alias" || text == "alias") {
        d->mode = ShowAlias;
    }
    else {
        QRegularExpression re;
        QRegularExpressionMatch match;
        re.setPattern("/XA(\\d{1,2})\\b");
        match = re.match(text);
        if(match.hasMatch()) {
            d->link_idx = match.captured(1).toInt();
            d->mode = DelAlias;
        }
        else {
            re.setPattern("/A(\\d{1,2})\\b");
            match = re.match(text);
            if(match.hasMatch()) {
                d->link_idx = match.captured(1).toInt();
                d->mode = ExecAlias;
            }
        }
        if(d->mode == Invalid) {
            // ^alias\s+([A-Za-z0-9_]+)\s+(.*)
            const char *alias_match = "^alias\\s+([A-Za-z0-9_]+)\\s+(.*)"; // escaped
            re.setPattern(alias_match);
            match = re.match(text);
            if(match.hasMatch()) {
                d->mode = SetAlias;
                d->name = match.captured(1);
                d->replaces = match.captured(2);
            }
            else {
                // alias something: provide info about the replacement
                // ^alias\s+([A-Za-z0-9_]+)
                re.setPattern("^alias\\s+([A-Za-z0-9_]+)");
                match = re.match(text);
                if(match.hasMatch()) {
                    d->name = match.captured(1);
                    d->mode = ShowAlias;
                }
                else if(text.startsWith("alias")) {
                    d->mode = AliasCmdErr;
                    d->msg = "error: alias  name  something/to/replace  [some description] (/help)";
                }
            }
        }
    }
    if(d->mode == Invalid) {
        QList<AliasEntry> aliases  = getAlias(msg.user_id, QString(), d->qsqld);
        d->out = findAndReplace(msg.text(), aliases);
        if(d->out != msg.text()) {
            d->mode = AliasFindAndReplace;
            return type();
        }
    }

    if(d->mode == Invalid)
        return -1;
    return type();
}

bool AliasModule::process()
{
    bool success = false;
    if(d->mode == AliasFindAndReplace) {
        d->msg_in.setText(d->out);
        d->listener->onReinjectMessage(d->msg_in);
    }
    else if(d->mode == SetAlias) {
        success = m_db_insertAlias(d->msg_in.user_id, d->name, d->replaces, d->msg_in.description(), d->bot_conf->getInt("max_alias_cnt"));
        d->listener->onSendMessageRequest(d->msg_in.chat_id, m_aliasInsertMsg(success, d->name, d->replaces, d->msg_in.description(), d->db_msg));
    }
    else if(d->mode == ExecAlias) {
        HistoryEntry he = m_db_getFromHistory(d->msg_in.user_id, d->link_idx);
        if(he.isValid()) {
            TBotMsg modmsg(d->msg_in);
            modmsg.setHost(he.host);
            modmsg.setText(he.command);
            modmsg.from_history = true;
            d->listener->onReinjectMessage(modmsg);
        }
    }
    else if(d->mode == ShowAlias) {
        QList<AliasEntry> alist = getAlias(d->msg_in.user_id, d->name, d->qsqld);
        d->listener->onSendMessageRequest(d->msg_in.chat_id, m_aliasListMsg(d->name, alist));
    }
    else if(d->mode == DelAlias) {
        QString name, replaces;
        m_db_deleteAlias(d->msg_in.user_id, d->link_idx, name, replaces);
        d->listener->onSendMessageRequest(d->msg_in.chat_id, m_aliasDeleteMsg(name, replaces));
    }
    else if(d->mode == AliasCmdErr)
        return false;
    return true;
}

bool AliasModule::error() const
{
    return !d->msg.isEmpty();
}

QString AliasModule::message() const
{
    return d->msg;
}

bool AliasModule::isVolatileOperation() const
{
    return false;
}

bool AliasModule::m_db_deleteAlias(int user_id, int alias_idx, QString& name, QString& replaces)
{
    if(!d->qsqld.isOpen())
        return -1;
    d->msg.clear();
    d->db_msg.clear();
    QSqlQuery q(d->qsqld);
    bool err = !q.exec(QString("SELECT name,replaces FROM alias WHERE user_id=%1 AND idx=%2").arg(user_id).arg(alias_idx));
    if(!err && q.next()) {
        name = q.value(0).toString();
        replaces = q.value(1).toString();
        err = !q.exec(QString("DELETE FROM alias WHERE user_id=%1 AND idx=%2").arg(user_id).arg(alias_idx));
    }
    else if(!err)
        d->db_msg = QString("AliasModule: alias index /XA%1 invalid. Use /alias to get an up to date list of aliases")
                .arg(alias_idx);
    if(err)
        d->msg = "AliasModule.m_db_deleteAlias: error executing " + q.lastQuery() + ": " + q.lastError().text();
    return !err;
}

bool AliasModule::m_db_insertAlias(int user_id, const QString& name,
                                   const QString& replaces,
                                   const QString& description, int max_alias_cnt)
{
    if(!d->qsqld.isOpen())
        return -1;
    d->msg.clear();
    d->db_msg.clear();
    QSqlQuery q(d->qsqld);

    bool err = !q.exec(QString("SELECT idx,replaces FROM alias WHERE user_id=%1 AND name='%2'").
                    arg(user_id).arg(name));
    if(!err) {
        if(q.next()) {
            QString old_replaces = q.value(1).toString();
            err = !q.exec(QString("UPDATE alias SET timestamp=datetime(), replaces='%1', description='%2'"
                                    " WHERE idx=%3").arg(replaces).arg(description).arg(q.value(0).toInt()));
            d->db_msg = QString("%1 alias has been updated from %2")
                    .arg(name).arg(old_replaces);
        }
        else {
            err = !q.exec(QString("SELECT idx,timestamp,name FROM alias WHERE user_id=%1 ORDER BY timestamp DESC")
                            .arg(user_id));

            // remove older entries according to the maximum number of allowed alias entries
            QList<int> indexes;
            int count = 0, index;
            while(q.next() && !err) {
                count++;
                index = q.value(0).toInt();
                indexes << index; // gather per user idx from alias table
                if(count > max_alias_cnt) {
                    QSqlQuery delq;
                    err = !delq.exec(QString("DELETE FROM alias WHERE idx=%1").arg(index));
                    d->db_msg += QString("removed old alias \"%1\" from %2 (max allowed aliases: %3)\n")
                            .arg(q.value(2).toString()).arg(q.value(1).toDateTime().toString("yyyy.MM.dd hh.mm.ss"))
                            .arg(max_alias_cnt);
                }
            } // end while q.next
            if(!err) {
                int available_idx = m_findFirstAvailableIdx(indexes); // UNIQUE constraint (user_id,idx)
                err = !q.exec(QString("INSERT INTO alias VALUES(%1, %2, '%3', '%4', '%5', datetime())")
                                .arg(available_idx).arg(user_id).arg(name).arg(replaces).arg(description));
            }
        }
    }


    if(err) {
        d->msg = QString("AliasProc.insertAlias: query %1 failed: %2").arg(q.lastQuery()).arg(q.lastError().text());
        perr("%s", qstoc(d->msg));
    }
    return !err;
}

HistoryEntry AliasModule::m_db_getFromHistory(int user_id, int index)
{
    HistoryEntry he;
    if(!d->qsqld.isOpen())
        return he;
    d->msg.clear();
    d->db_msg.clear();
    QSqlQuery q(d->qsqld); //             0         1     2     3        4
    bool err = !q.exec(QString("SELECT timestamp,command,type,host,description FROM history WHERE user_id=%1 AND h_idx=%2")
                       .arg(user_id).arg(index));
    while(!err && q.next()) {
        he = HistoryEntry (index, user_id, q.value(0).toDateTime(), q.value(1).toString(),
                           q.value(2).toString(), q.value(3).toString(), q.value(4).toString());
    }
    if(err) {
        d->msg = QString("AliasProc.m_db_getFromHistory: failed to exec %1: %2").arg(q.lastQuery()).arg(q.lastError().text());
        perr("%s", qstoc(d->msg));
    }
    return he;
}

QString AliasModule::m_aliasInsertMsg(bool success, const QString& nam, const QString& replac, const QString& descriptio, const QString &additional_message) const {
    QString s;
    FormulaHelper fh;
    QString name = fh.escape(nam);
    QString replaces = fh.escape(replac);
    if(success) {
        s += "👍   successfully added alias:\n";
        s += QString("<b>%1</b> replaces: <i>%2</i>").arg(name).arg(replaces);
        if(!descriptio.isEmpty()) {
            s += "\n📝   <i>" + descriptio + "</i>";
        }
    }
    else {
        s = "👎   failed to insert alias <b>" + name + "</b>";
    }
    if(!additional_message.isEmpty())
        s += "\n[<i>" + fh.escape(additional_message) + "</i>]";

    return s;
}

QString AliasModule::m_aliasListMsg(const QString& name, const QList<AliasEntry> &alist) const
{
    FormulaHelper fh;
    QString s;
    if(alist.isEmpty())  {
        name.isEmpty()? s += "no alias defined" : s += "no alias defined for <i>" + name + "</i>.";
    }
    else {
        if(name.isEmpty()) s = "<b>alias list</b>\n";
    }
    int i = 1;
    foreach(AliasEntry a, alist) {
        s += QString("%1. <i>%2 --> %3</i>").arg(i).arg(fh.escape(a.name)).arg(fh.escape(a.replaces));
        for(int i = 0; i < a.in_history_idxs.size(); i++) {
            s += QString(" [/A%1] [%2]").arg(a.in_history_idxs[i]).arg(a.in_history_hosts[i]);
        }

        a.description.isEmpty() ? s += "" : s += "   (" + fh.escape(a.description) + ")";

        s += QString(" [/XA%1]\n").arg(a.index);

        i++;
    }
    return s;
}

QString AliasModule::m_aliasDeleteMsg(const QString &name, const QString &replaces) const
{
    QString s;
    if(!name.isEmpty() && !replaces.isEmpty()) {
        s += "👍   successfully removed alias: ";
        s += QString("<b>%1</b> --> <i>%2</i>").arg(name).arg(replaces);
    }
    else {
        s = "👎   failed to remove alias: " + d->db_msg;
    }
    return s;
}


int AliasModule::m_findFirstAvailableIdx(const QList<int>& in_idxs)
{
    int a_idx = -1, i;
    for(i = 1; i <= in_idxs.size() && a_idx < 0; i++) {
        if(!in_idxs.contains(i))
            a_idx = i;
    }
    if(a_idx < 0)
        a_idx = i;
    return a_idx;
}
