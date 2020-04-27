#include "botdb.h"
#include <QtDebug>
#include <QSqlRecord>
#include <cumacros.h>

/**
 * @brief BotDb::BotDb class constructor.
 *
 * Creates database tables if they do not exist.
 *
 * \section Tables
 *
 * \par proc table
 *
 * procs table saves active monitors/aletrs in order to be restored if the server goes down and up again later.
 * Must contain all the fields necessary to restart a monitor / alert exactly as it was before:
 * - user and chat id
 * - date time started
 * - command (monitor|alert|...) plus source plus formula
 * - host
 *
 * Inserts are made by registerProc *only if a service has been successfully started*
 * CuBotServer must deal with this, and registerProc is called from CuBotServer::onSrcMonitorStarted
 *
 * \par users table
 * Stores information about registered users
 *
 * \par history table
 * Stores the per user history of reads/monitors/alerts and other commands.
 * History is a circular buffer, older entries are deleted.
 * history table entries where bookmark field is true are not deleted.
 * When a bookmark is removed, bookmark field is set to false and that entry
 * is treated as a normal history entry, so it will be deleted if old.
 *
 * \par hosts table
 * Stores the name, the description and the creation date of the configured hosts (i.e. TANGO_HOSTs)
 *
 * \par host_selection
 * Records the currently selected host for each registered user
 *
 */
BotDb::BotDb(const QString& db_file)
{
    m_db = QSqlDatabase::addDatabase("QSQLITE");
    m_db.setDatabaseName(db_file);
    m_err = !m_db.open("tbotdb", "tbotdb");
    if(m_err)
        m_msg = m_db.lastError().text();
    qDebug() << __PRETTY_FUNCTION__ << "database open" << m_db.isOpen() << "tables" << m_db.tables();

    QStringList tables = QStringList() << "users" << "history" << "history_types" << "hosts" << "host_selection"
                                       << "proc" << "config" << "bookmarks" << "auth" << "auth_limits"
                                       << "private_chats";

    foreach(QString t, tables) {
        if(!m_db.tables().contains(t))
            createDb(t);
        if(m_err)
            perr("BotDb: failed to create table: %s: %s", qstoc(t), qstoc(m_msg));
    }

    m_initUserChatsMap();
    qDebug() << __PRETTY_FUNCTION__ << m_user_chatsMap;
}

BotDb::~BotDb()
{
    printf("\e[1;31m~BotDb %p: closing db...\e[0m", this);
    if(m_db.isOpen())
        m_db.close();
}

bool BotDb::addUser(int uid, const QString &uname, const QString &firstName, const QString &lastName)
{
    if(!m_db.isOpen())
        return false;
    m_msg.clear();
    QSqlQuery q(m_db);
    m_err = !q.exec(QString("INSERT INTO users VALUES(%1,'%2','%3','%4', datetime())").arg(uid).arg(uname).arg(firstName).arg(lastName));
    if(m_err)
        m_msg = q.lastError().text();
    return !m_err;
}

bool BotDb::removeUser(int uid)
{
    if(!m_db.isOpen())
        return false;
    m_msg.clear();

    return !m_err;
}
/**
 * @brief BotDb::registerProc registers a new monitor/alert or any other service that
 *        needs to be restored if CuBotServer goes down and then up again.
 *
 * @param he HistoryEntry filled by the CuBotServer when a new service has been successfully started.
 *        The field chat_id *must be > -1*
 *
 * @return true if successful, false otherwise
 */
bool BotDb::saveProc(const HistoryEntry &he)
{
    if(!m_db.isOpen())
        return false;
    m_msg.clear();
    QSqlQuery q(m_db);
    // (user_id INTEGER NOT NULL, chat_id INTEGER NOT NULL,
    // start_dt DATETIME NOT NULL, command TEXT NOT NULL,
    // formula TEXT, host TEXT NOT NULL)
    m_err = !q.exec(QString("INSERT INTO proc VALUES(%1, %2, '%3', '%4', '%5', '%6')").
                    arg(he.user_id).arg(he.chat_id).arg(he.datetime.toString("yyyy-MM-dd hh:mm:ss")).
                    arg(he.command).arg(he.type).arg(he.host));
    printf("\e[1;32m---> registered process: \"%s\"\e[0m SUCCESS: %d\n", qstoc(q.lastQuery()), !m_err);
    if(m_err)
        m_msg = "BotDb.registerProc: failed to register process " + he.toCommand() + ": " + q.lastError().text();
    return !m_err;
}

bool BotDb::clearProcTable()
{
    if(!m_db.isOpen())
        return false;
    m_msg.clear();
    QSqlQuery q(m_db);
    m_err = !q.exec("DELETE FROM proc");
    if(m_err)
        m_msg = "BotDb.clearProcTable: failed to clear proc table: " + q.lastError().text();
    return !m_err;
}

/**
 * @brief BotDb::unregisterProc called from the CuBotServer when a process (monitor or alert) stops
 *
 * @param he a partially filled HistoryEntry containing only the fields necessary to individuate and
 *        delete a process from the proc table
 *        History entry contains
 *        - user_id
 *        - src
 *        - host
 *        Formula is not necessary because there can be only one monitor with that source at one time,
 *        unregarding the formula
 *
 * @return true if the database proc table has been successfully updated, false otherwise
 */
bool BotDb::unregisterProc(const HistoryEntry &he)
{
    if(!m_db.isOpen())
        return false;
    m_msg.clear();
    QSqlQuery q(m_db);
    // (user_id INTEGER NOT NULL, chat_id INTEGER NOT NULL,
    // start_dt DATETIME NOT NULL, command TEXT NOT NULL, host TEXT NOT NULL)
    m_err = !q.exec(QString("DELETE FROM proc WHERE user_id=%1 AND command='%2'").
                    arg(he.user_id).arg(he.command));
    printf("\e[0;35m<-- unregistered process: \"%s\"\e[0m SUCCESS: %d\n", qstoc(q.lastQuery()), !m_err);

    if(m_err)
        m_msg = "BotDb.registerProc: failed to register process " + he.toCommand() + ": " + q.lastError().text();
    return !m_err;
}

bool BotDb::registerHistoryType(const QString &newtype, const QString& description)
{
    if(!m_db.isOpen())
        return false;
    m_msg.clear();
    QSqlQuery q(m_db);
    m_err = !q.exec(QString("INSERT INTO history_types VALUES('%1','%2')").arg(newtype).arg(description));
    return !m_err;
}

QList<HistoryEntry> BotDb::loadProcs()
{
    QList<HistoryEntry> hel;
    if(!m_db.isOpen())
        return hel;
    m_msg.clear();
    QSqlQuery q(m_db);
    // (user_id INTEGER NOT NULL, chat_id INTEGER NOT NULL,
    // start_dt DATETIME NOT NULL, command TEXT NOT NULL,
    // formula TEXT, host TEXT NOT NULL)
    m_err = !q.exec(QString("SELECT user_id,command,type,host,chat_id,start_dt FROM proc"));
    if(m_err)
        m_msg = "BotDb::loadProcs: database error: " + q.lastError().text();
    else {
        while(q.next()) {
            HistoryEntry he;
            QSqlRecord r = q.record();
            // fromDbProc(int u_id, int chatid,
            // const QString& cmd, const QString& type,
            // const QString& formula, const QString& host,
            // const QDateTime& dt)
            he.fromDbProc(r.value("user_id").toInt(), r.value("chat_id").toInt(),
                          r.value("command").toString(),  r.value("type").toString(),
                          r.value("host").toString(),
                          r.value("start_dt").toDateTime());
            hel << he;
        }
    }
    return hel;
}

/**
 * @brief BotDb::addToHistory adds the entry to the history database
 * @param in a HistoryEntry object
 * @return < 0 if the operation failed. The index (h_idx) of the newly inserted row
 */
int BotDb::addToHistory(const HistoryEntry &in, BotConfig *bconf)
{
    if(!m_db.isOpen())
        return -1;
    m_msg.clear();
    int h_index = -1;
    int uid = in.user_id;
    const QString &cmd = in.command;
    const QString& type = in.type;
    const QString& host = in.host;
    const QString& description = in.description;

    QSqlQuery q(m_db);
    // first try to see if there is a matching row into the database for the new entry
    // if yes, only the timestamp field needs update
    m_err = !q.exec(QString("SELECT timestamp,h_idx FROM history WHERE "
                            " user_id=%1 AND command='%2'"
                            " AND type=(SELECT _rowid_ FROM history_types WHERE type='%3')"
                            " AND host='%4'")
                    .arg(uid).arg(cmd).arg(type).arg(host));

    bool update_timestamp_only = q.next() && !m_err;


    printf("\e[1;33mexecuted %s to see if found entry.. found? %d\e[0m\n", qstoc(q.lastQuery()), update_timestamp_only);

    if(update_timestamp_only) {
        h_index = q.value(1).toInt(); // PRIMARY KEY(user_id,type,h_idx)

        // no description? leave description field as is
        // a new description is provided? update description too
        if(description.isEmpty()) {
            m_err = !q.exec(QString("UPDATE history SET timestamp=datetime(), stop_timestamp=NULL "
                                    " WHERE user_id=%1 "
                                    " AND type=(SELECT _rowid_ FROM history_types WHERE type='%2') "
                                    " AND h_idx=%3;")
                            .arg(uid).arg(type).arg(h_index));
        }
        else {
            m_err = !q.exec(QString("UPDATE history SET timestamp=datetime(), stop_timestamp=NULL, description='%4'"
                                    "where user_id=%1 AND type=(SELECT _rowid_ FROM history_types WHERE type='%2') "
                                    " AND h_idx=%3")
                            .arg(uid).arg(type).arg(h_index).arg(description));
        }
    }

    if(!m_err && !update_timestamp_only) {
        QMap<QString, int> typesMap;
        QSqlQuery typesQ(m_db);
        m_err = !typesQ.exec("SELECT _rowid_,type from history_types");
        while(!m_err && typesQ.next())
            typesMap.insert(typesQ.value(1).toString(), typesQ.value(0).toInt());
        m_err = typesMap.isEmpty();
        if(m_err) {
            this->m_setErrorMessage("BotDb.addToHistory", typesQ);
            return false;
        }

        qDebug() << __PRETTY_FUNCTION__ << "hitoru types" << typesMap << "QUERY WAS " << typesQ.lastQuery();
        QSqlQuery bookmarks_q(m_db);
        m_err = !bookmarks_q.exec("SELECT history_rowid FROM bookmarks");
        QList<int> bookmarks_idxs; // list of history._rowid_ that are bookmarked
        while(bookmarks_q.next()) // collect all bookmarks into bookmarks_idxs
            bookmarks_idxs << bookmarks_q.value(0).toInt();

        // Keep history table small
        // For each type, get all rows for the given user in the history table
        //
        // 1. Do not delete running alerts or monitors (whose stop_timestamp is still NULL)
        //    so either select rows where the type is neither monitor nor alert OR where the stop_timestamp
        //    is not null (stop timestamp NOT NULL means there is a stop date
        // 2. take care history entries that are bookmarks are not deleted (with the bookmarks_q query
        //    later on)
        //

        for(int k = 0; !m_err && k < typesMap.keys().size(); k++) {
            QList<int> h_idxs;
            const QString htype = typesMap.keys()[k];
            qDebug() <<__PRETTY_FUNCTION__ << "TYPE " << htype;
            int rowcnt = 0;
            //                                    0    1        2        3
            m_err = !q.exec(QString("SELECT timestamp,h_idx,_rowid_,stop_timestamp FROM history WHERE "
                " user_id=%1 AND type=%2 ORDER BY timestamp DESC").arg(uid).arg(typesMap[htype]));

            printf("BotDb.addToHistory executed %s\n", qstoc(q.lastQuery()));
            // go through all the history entries that are not running monitors or alerts
            while(q.next()) {
                int history_rowid = q.value(2).toInt();
                h_idxs  << q.value(1).toInt();

                bool is_running_monitor = (htype == "monitor" || htype == "alert") && q.value(3).isNull();
                bool is_bookmark = bookmarks_idxs.contains(history_rowid);
                // count the number of entries excluding bookmarked rows
                if(!is_bookmark)
                    rowcnt++;
                int history_depth = bconf->getInt(QString("history_%1_depth").arg(htype));
                if(rowcnt > history_depth && !is_bookmark && !is_running_monitor) {
                    QSqlQuery delq(m_db);
                    QString timestamp_str = q.value(0).toDateTime().toString("yyyy-MM-dd hh:mm:ss");
                    int history_idx = q.value(1).toInt();
                    // we delete older rows
                     m_err = !delq.exec(QString("DELETE FROM history WHERE user_id=%1 AND timestamp='%2' "
                                               "AND h_idx=%3")
                                       .arg(uid).arg(timestamp_str).arg(history_idx));
                    if(m_err) {
                        m_msg = "BotDb.addToHistory: " +  delq.lastError().text();
                    }
                    else {
                        printf("\e[1;35mBotDb.insertOperation: removed old history entry from %s (%s)\e[0m\n",
                               qstoc(q.value(0).toDateTime().toString()), qstoc(delq.lastQuery()));
                    }
                }
            } // end while(q.next())


            printf("BotDb.addToHistory: m_err %d htype %s in.type %s dafuq is this shit\n",
                   m_err, qstoc(htype), qstoc(in.type));
            if(!m_err && htype == in.type) {
                qDebug() << __PRETTY_FUNCTION__ << "finding available idxs in " << h_idxs;
                // calculate first per history index available
                h_index = m_findFirstAvailableIdx(h_idxs);
                // user_id INTEGER NOT NULL, timestamp DATETIME NOT NULL, command TEXT NOT NULL,type TEXT NOT NULL,host TEXT DEFAULT, stop_timestamp ''
                m_err = !q.exec(QString("INSERT INTO history VALUES(%1, datetime(), '%2',"
                                        " (SELECT _rowid_ FROM history_types WHERE type='%3'), '%4', '%5', %6, NULL)").
                                arg(uid).arg(cmd).arg(htype).arg(host).arg(description).arg(h_index));
                if(m_err)
                    m_msg =  "BotDb.addToHistory: " + q.lastError().text();

            }
        }
    }

    if(m_err) {
        perr("BotDb.addToHistory: database error: query %s %s",  qstoc(q.lastQuery()), qstoc(m_msg));
        return -1;
    }
    printf("BotDb::addToHistory returning index %d\n", h_index);
    return h_index;
}

QList<HistoryEntry> BotDb::history(int user_id, const QString& type) {
    QList<HistoryEntry> hel;
    if(!m_db.isOpen())
        return hel;
    m_err = false;
    m_msg.clear();
    QSqlQuery q(m_db);
    QString t = type;
    if(type != "bookmarks") {
        m_err = !q.exec(QString("SELECT user_id,timestamp,command,host,description,h_idx,_rowid_,stop_timestamp"
                                " FROM history WHERE user_id=%1 AND"
                                " type=(SELECT _rowid_ FROM history_types WHERE type='%2') "
                                " ORDER BY timestamp ASC").arg(user_id).arg(t));
    }
    else { // query with no AND type='%s'
        //                                 0                1        2     3    4    5       6     7                  8
        m_err = !q.exec(QString("SELECT user_id,history.timestamp,command,host,description,h_idx,history._rowid_,stop_timestamp"
                                " FROM history,bookmarks WHERE user_id=%1 "
                                " AND bookmarks.history_rowid=history.h_idx "
                                " ORDER BY history.timestamp ASC").arg(user_id));
    }
    printf("executed %s\n", qstoc(q.lastQuery()));
    /*HistoryEntry(int index,
     * int u_id,
     * const QDateTime& ts,
     * const QString& cmd,
     * const QString& type,
     * const QString& host);
    */
    while(!m_err && q.next()) {
        QSqlRecord r = q.record();
        HistoryEntry he(r.value("h_idx").toInt(),
                        r.value("user_id").toInt(),
                        r.value("timestamp").toDateTime(),
                        r.value("command").toString(),
                        type,
                        r.value("host").toString(),
                        r.value("description").toString());
        he.stop_datetime = r.value("stop_timestamp").toDateTime();
        he.is_active = he.stop_datetime.isNull();
        hel << he;
    }
    if(m_err) {
        m_setErrorMessage("BotDb.history", q);
        perr("BotDb.history: database error: %s", qstoc(m_msg));
    }
    return hel;
}

int BotDb::fixStaleItems(const QStringList &cmd_list)
{
    int cnt = 0;
    if(!m_db.isOpen())
        return cnt;
    m_err = false;
    m_msg.clear();
    QSqlQuery q(m_db);
    m_err = !q.exec("SELECT command,h_idx FROM history WHERE stop_timestamp IS NULL AND "
                    " (type=(SELECT _rowid_ FROM history_types WHERE type='monitor') OR"
                    " type=(SELECT _rowid_ FROM history_types WHERE type='alert') ) ");
    while(!m_err && q.next()) {
        QString cmd = q.value(0).toString();
        QSqlQuery q2(m_db);
        if(!cmd_list.contains(cmd))
            m_err = !q2.exec(QString("UPDATE history SET stop_timestamp=datetime() WHERE command='%1' AND h_idx=%2 ")
                             .arg(cmd).arg(q.value(1).toInt()));
        if(q2.numRowsAffected() > 0) {
            printf("\e[1;31mBotDb.fixStaleItem\e[0m: fixing \e[1;31mstale cmd \"%s\"\e[0m a valid stop date was missing",
                   qstoc(cmd));
            cnt += q2.numRowsAffected();
        }
    }

    if(m_err)  {
        m_setErrorMessage("BotDb.fixStaleItems", q);
        perr("%s", qstoc(m_msg));
    }
    return cnt;
}

QMap<int, QString> BotDb::usersById()
{
    QMap<int, QString> umap;
    if(!m_db.isOpen())
        return umap;
    m_err = false;
    m_msg.clear();
    QSqlQuery q(m_db);
    m_err = !q.exec("SELECT id,uname FROM users");
    while(!m_err && q.next())
        umap[q.value(0).toInt()] = q.value(1).toString();
    return umap;
}

//QList<HistoryEntry> BotDb::bookmarks(int uid)
//{
//    QList<HistoryEntry> hel;
//    if(!m_db.isOpen())
//        return hel;
//    m_err = false;
//    m_msg.clear();
//    QSqlQuery q(m_db);
//    m_err = !q.exec(QString("SELECT user_id,timestamp,command,type,formula,host,h_idx"
//                            " FROM history WHERE user_id=%1 AND bookmark_idx>0 "
//                            " ORDER BY timestamp ASC").arg(uid));
//    while(!m_err && q.next()) {
//        HistoryEntry he(q.value(6).toInt(), q.value(0).toInt(), q.value(1).toDateTime(), q.value(2).toString(),
//                        q.value(3).toString(), q.value(4).toString(), q.value(5).toString());
//        hel << he;
//    }
//    if(m_err)
//        m_msg = "BotDb.history: " + q.lastError().text();
//    return hel;
//}

HistoryEntry BotDb::lastOperation(int uid)
{
    HistoryEntry he;
    if(!m_db.isOpen())
        return he;
    m_msg.clear();
    QSqlQuery q(m_db);
    m_err = !q.exec(QString("SELECT MAX(timestamp),command,history_types.type,host "
                            " FROM history,history_types WHERE user_id=%1 "
                            " AND history.type=history_types._rowid_").arg(uid));
    if(m_err)
        m_msg = q.lastError().text();
    else {
        if(q.next()) {
            he.user_id = uid;
            he.datetime = q.value(0).toDateTime();
            he.command = q.value(1).toString();
            he.type = q.value(2).toString();
            he.host = q.value(3).toString();
        }
    }
    return he;
}

HistoryEntry BotDb::bookmarkLast(int uid)
{
    HistoryEntry he;
    if(!m_db.isOpen())
        return he;
    m_msg.clear();
    QSqlQuery q(m_db);
    m_err = !q.exec(QString("SELECT MAX(timestamp),command,history_types.type,host,h_idx "
                            " FROM history,history_types WHERE user_id=%1 "
                            " AND history_types._rowid_=history.type").arg(uid));
    if(m_err)
        m_msg = q.lastError().text();
    else {
        if(q.next()) {
            he.user_id = uid;
            he.datetime = q.value(0).toDateTime();
            he.command = q.value(1).toString();
            he.type = q.value(2).toString();
            he.host = q.value(3).toString();
            int rowid = q.value(4).toInt();
            // mark last entry as bookmark!
            // OK, we found most recent entry in table
            // find available per user bookmark index
            QList<int> bkm_indexes;
            QSqlQuery bookmarksQ(m_db);
            m_err = !q.exec(QString("INSERT INTO bookmarks VALUES(%1,datetime())").arg(rowid));
        }
    }
    if(m_err) {
        m_setErrorMessage("BotDb.bookmarkLast", q);
        perr("%s", qstoc(m_msg));
    }
    return he;
}

bool BotDb::removeBookmark(int uid, int index)
{
    m_msg.clear();
    m_err = false;
    QSqlQuery q(m_db);
    m_err = !q.exec(QString("DELETE FROM bookmarks WHERE history_rowid=(SELECT history.rowid FROM history "
                            "WHERE user_id=%1 AND h_idx=%2)").arg(uid).arg(index));
    if(m_err) {
        m_msg = "BotDb.removeBookmark: query  " + q.lastQuery() + "error: " + q.lastError().text();
        perr(qstoc(m_msg));
    }
    return !m_err;
}

HistoryEntry BotDb::commandFromIndex(int uid, const QString& type, int index)
{
    m_msg.clear();
    m_err = false;
    QString typ(type);
    typ.remove('/').remove(QString::number(index));
    const QList<HistoryEntry> hes = history(uid, typ);
    for(int i = 0; i < hes.size(); i++) {
        if(hes[i].index == index && typ.startsWith(hes[i].type)) {
            return hes[i];
        }
    }
    if(hes.size() == 0)
        m_msg = "BotDb: history empty";
    else if(index > hes.size())
        m_msg = "BotDb: index out of range";
    return HistoryEntry();
}

HistoryEntry BotDb::historyEntryFromCmd(int uid, const QString &command)
{
    HistoryEntry he;
    if(!m_db.isOpen())
        return he;
    m_msg.clear();
    QSqlQuery q(m_db);
    m_err = !q.exec(QString("SELECT h_idx,timestamp,history_types.type,host,history.description"
                            " FROM history,history_types WHERE command='%1' AND user_id=%2"
                            " AND history.type=history_types._rowid_")
                    .arg(command).arg(uid));
    while(!m_err && q.next()) {
        he = HistoryEntry(q.value(0).toInt(), uid, q.value(1).toDateTime(), command,
                          q.value(2).toString(), q.value(3).toString(), q.value(4).toString());
    }
    if(m_err) {
        m_setErrorMessage("historyEntryFromCmd", q);
        perr("%s", qstoc(m_msg));
    }
    return he;
}

bool BotDb::monitorStopped(int user_id, const QString &command, const QString &host)
{
    if(!m_db.isOpen())
        return false;
    m_msg.clear();
    QSqlQuery q(m_db);
    m_err = !q.exec(QString("UPDATE history SET stop_timestamp=datetime() WHERE user_id=%1 AND command='%2' AND host='%3'")
                    .arg(user_id).arg(command).arg(host));
    printf("\e[1;33mBotDb.monitorStopped: updating HISTORY: %s\e[0m\n", qstoc(q.lastQuery()));
    if(m_err) {
        m_msg = "BotDb::monitorStopped: error updating history for " + QString::number(user_id) + ": "
                + command + ": " + q.lastError().text();
        perr("%s", qstoc(m_msg));
    }
    return !m_err;
}

bool BotDb::error() const
{
    return m_err;
}

QString BotDb::message() const
{
    return m_msg;
}

BotDb::Quality BotDb::strToQuality(const QString &qs) const
{
    if(qs.contains("alarm", Qt::CaseInsensitive) || qs.contains("error", Qt::CaseInsensitive))
        return Alarm;
    if(qs.contains("warning", Qt::CaseInsensitive))
        return Warning;
    if(qs.contains("invalid", Qt::CaseInsensitive))
        return Invalid;
    return Ok;
}

QString BotDb::qualityToStr(BotDb::Quality q) const
{
    switch (q) {
    case Alarm:
        return "alarm";
    case Warning:
        return "warning";
    case Invalid:
        return "invalid";
    case Ok:
        return "ok";
    default:
        return "undefined";
    }
}

bool BotDb::userExists(int uid)
{
    if(!m_db.isOpen())
        return false;
    m_msg.clear();
    QSqlQuery q(m_db);
    m_err = !q.exec(QString("SELECT id FROM users WHERE id=%1").arg(uid));
    if(m_err)
        m_msg = q.lastError().text();
    return !m_err && q.next();
}

QList<HostEntry> BotDb::getHostList()
{
    QList<HostEntry> hlist;
    if(!m_db.isOpen())
        return hlist;
    m_msg.clear();

    QSqlQuery q(m_db);
    m_err = !q.exec(QString("SELECT name,id,description FROM hosts"));
    while(!m_err && q.next()) {
        hlist << HostEntry(q.value(0).toString(), q.value(1).toInt(), q.value(2).toString());
    }
    return hlist;
}

bool BotDb::setHost(int user_id, int  chat_id, int host_idx, QString &new_host, QString& new_host_description) {
    if(!m_db.isOpen())
        return false;
    m_msg.clear();
    QSqlQuery q(m_db);
    m_err = !q.exec(QString("SELECT name from hosts WHERE id=%1").arg(host_idx));
    if(!m_err && !q.next())
        m_msg = QString("no host with the given index [%1]").arg(host_idx);
    else if(!m_err) {
        new_host = q.value(0).toString();
        return setHost(user_id, chat_id, new_host, new_host_description);
    }
    if(m_err)
        m_setErrorMessage("BotDb.setHost (by idx)", q);
    return false;
}

bool BotDb::setHost(int user_id, int chat_id, const QString& host, QString &new_host_description) {
    if(!m_db.isOpen())
        return false;
    m_msg.clear();

    QSqlQuery q(m_db);
    m_err = !q.exec(QString("SELECT id,description FROM hosts WHERE name='%1'").arg(host));
    if(!m_err) {
        int host_id = -1;
        if(q.next()) {
            host_id = q.value(0).toInt();
            new_host_description = q.value(1).toString();
        }
        // is user allowed to set host  database
        printf("BotDb.setHost host id %d yser id %d\n", host_id, user_id);
        bool allowed = user_id > 0;
        if(host_id >= 0 && allowed) {
            m_err = !q.exec(QString("INSERT INTO host_selection VALUES(%1, %2, datetime())").arg(host_id).arg(chat_id));
            if(m_err)
                m_msg = "BotDb.setHost: failed to select host \"" + host + "\": " + q.lastError().text();
        }
        else {
            m_err = true;
            m_msg = "BotDb.setHost: user is not allowed to set host to \"" + host + "\"";
        }
    }
    return !m_err;
}

/**
 * @brief BotDb::getSelectedHost returns the host associated to chat_id or the value of the TANGO_HOST environment
 *        variable if no host has been selected by the user
 * @param chat_id the chat id of the message
 * @return the name of the TANGO_HOST
 */
QString BotDb::getSelectedHost(int chat_id)
{
    QString host;
    if(!m_db.isOpen())
        return host; // empty
    m_msg.clear();
    QSqlQuery q(m_db);
    m_err = !q.exec(QString("SELECT name FROM hosts,host_selection where chat_id=%1 AND host_id=hosts.id").arg(chat_id));
    if(!m_err) {
        if(q.next())
            host = q.value(0).toString();
        // having an empty result is not an error. The application will pick up TANGO_HOST from env
    }
    else
        m_msg = "BotDb.setHost: failed to select host \"" + host + "\": " + q.lastError().text();

    return host;
}

/**
 * @brief BotDb::getConfig fills in two maps that are passed by reference by BotConfig
 *
 * \par Note
 * The function does not clear the input maps before filling them in.
 * This allows BotConfig to prepare a map with defaults that will remain untouched if the
 * corresponding keys are not configured into the database (e.g. ttl)
 *
 */
bool BotDb::getConfig(QMap<QString, QVariant> &datamap, QMap<QString, QString> &descmap)
{
    if(!m_db.isOpen())
        return false;
    m_msg.clear();
    QSqlQuery q(m_db);
    m_err = !q.exec("SELECT * FROM config");
    if(m_err)
        m_msg = "BotDb.getConfig: error in query " + q.lastError().text();
    else {
        QMap<QString, QVariant> map;
        while(!m_err && q.next()) {
            QSqlRecord rec = q.record();
            m_err = rec.count() != 3;
            if(!m_err) {
                datamap[rec.value(0).toString()] = rec.value(1);
                descmap[rec.value(0).toString()] = rec.value(2).toString();
            }
            else {
                m_msg = "BotDb.getConfig: unexpected number of columns in table config";
            }
        }
    }
    return !m_err;
}

/**
 * @brief BotDb::isAuthorized returns an integer indicating if the operation is allowed or not
 * @param uid the user id
 * @param operation the name of the operation: must match one of the fields of the auth_limits table
 * @param unregistered a pointer to a boolean value that will be set to true if the user is not
 *        registered in the *auth* table, false otherwise.
 *
 * @return -1 the user is not authorized (user still not in auth table - waiting for authorization)
 * @return 0  user is authorized and the value associated to the operation must be taken from the global
 *            configuration because there is no specific authorization for this operation and uid
 * @return >0 the value stored into the auth_limits table for the given uid and operation
 * @return < 0 the auth_limits table *value* for the given operation and uid is negative: this means
 *         the given (uid,operation) pair has been explicitly denied by the administrator.
 */
int BotDb::isAuthorized(int uid, const QString& operation, bool* unregistered) {
    bool user_unregistered;
    // operations contains the list of auth_limits columns
    if(!m_db.isOpen())
        return -1;
    m_msg.clear();
    QSqlQuery q(m_db);
    m_err = !q.exec(QString("SELECT user_id,authorized FROM auth WHERE user_id=%1").arg(uid));
    if(m_err)
        m_msg = "BotDb.isAuthorized: error in query " + q.lastError().text();
    else {
        user_unregistered = !q.next();
        if(unregistered != nullptr)
            *unregistered = user_unregistered;
        if(user_unregistered)
            return -1;
        else {
            bool authorized = q.value(1).toInt() > 0;
            if(!authorized)
                return -1;

            m_err = !q.exec(QString("SELECT value FROM auth_limits WHERE operation='%1' AND user_id=%2").arg(operation).arg(uid));
            if(m_err)
                m_msg = "BotDb.isAuthorized: error in query " + q.lastError().text();
            else {
                if(!q.next()) { // pick defaults from config
                    return 0;
                }
                else {
                    return q.value(0).toInt();
                }
            }
        }
    }
    return -1;
}

bool BotDb::userInPrivateChat(int uid, int chat_id)
{
    if(!m_db.isOpen())
        return -1;
    m_msg.clear();
    QSqlQuery q(m_db);
    m_err = !q.exec(QString("SELECT user_id FROM private_chats WHERE user_id=%1"
                            " AND chat_id=%2").arg(uid).arg(chat_id));
    if(!m_err) {
        return q.next();
    }
    else {
        m_setErrorMessage("BotDb.userInPrivateChat", q);
    }
    return false;
}

bool BotDb::addUserInPrivateChat(int uid, int chat_id)
{
    if(!m_db.isOpen())
        return false;
    m_msg.clear();
    QSqlQuery q(m_db);
    m_err = !q.exec(QString("INSERT INTO private_chats VALUES(%1,%2)").
                    arg(uid).arg(chat_id));
    if(m_err)
        m_setErrorMessage("BotDb.addUserInPrivateChat", q);
    return !m_err;
}

QList<int> BotDb::chatsForUser(int uid)
{
    QList<int> ch_ids;
    m_msg.clear();
    if(m_db.isOpen()) {
        QSqlQuery q(m_db);
        m_err = !q.exec(QString("SELECT chat_id FROM private_chats WHERE user_id=%1").arg(uid));
        while(!m_err && q.next())
            ch_ids << q.value(0).toInt();
        if(m_err)
            m_setErrorMessage("BotDb.chatsForUser", q);
    }
    return ch_ids;
}


QList<int> BotDb::getChatsWithActiveMonitors()
{
    QList <int>ids;
    if(m_db.isOpen()) {
        m_msg.clear();
        QSqlQuery q(m_db);
        m_err = !q.exec("SELECT DISTINCT chat_id FROM proc");
        while(q.next()) {
            ids << q.value(0).toInt();
            printf("BotDb::getChatsWithActiveMonitors: active is %d\n", ids.last());
        }
    }
    return ids;
}

bool BotDb::canWrite(int user_id)  {
    printf("\e[1;31mBotDb.canWrite: returns true for now! USER ID %d\e[0m\n", user_id);
    return true;
}

const QSqlDatabase *BotDb::getSqlDatabase() const
{
    return &m_db;
}

void BotDb::createDb(const QString& tablename)
{
    if(m_db.isOpen()) {
        printf("- BotDb.createDb: creating tables....\n");
        QSqlQuery q(m_db);
        m_err= false;
        if(tablename == "users") {
            m_err = !q.exec("CREATE TABLE users (id INTEGER PRIMARY_KEY NOT NULL, uname TEXT NOT NULL, first_name TEXT, last_name TEXT, "
                            "join_date DATETIME NOT NULL)");
        }
        else if(tablename == "hosts") {
            m_err = !q.exec("CREATE TABLE hosts (id INTEGER PRIMARY KEY, name TEXT UNIQUE NOT NULL, description TEXT, created DATETIME NOT NULL)");
        }
        else if(tablename == "host_selection") {
            m_err = !q.exec(" CREATE TABLE host_selection (host_id INTEGER NOT NULL,chat_id INTEGER NOT NULL,"
                            " timestamp DATETIME NOT NULL, UNIQUE(chat_id) ON CONFLICT REPLACE)");
        }
        else if(tablename == "history") {
            // unique user_id,operation. timestamp only is updated on repeated operations
            m_err = !q.exec("CREATE TABLE history (user_id INTEGER NOT NULL, "
                            "timestamp DATETIME NOT NULL, "
                            "command TEXT NOT NULL,"
                            "type INTEGER NOT NULL,"
                            "host TEXT DEFAULT '',"
                            "description TEXT DEFAULT '',"
                            "h_idx INTEGER NOT NULL,"
                            "stop_timestamp DATETIME DEFAULT NULL,"
                            "UNIQUE (user_id, command, type, host, h_idx) ON CONFLICT REPLACE,"
                            "PRIMARY KEY(user_id,type,h_idx) )");
        }
        else if(tablename == "proc") {
            // procs table saves active monitors/aletrs in order to be restored
            // if the server goes down and up again later.
            m_err = !q.exec("CREATE TABLE proc (user_id INTEGER NOT NULL, chat_id INTEGER NOT NULL, "
                            " start_dt DATETIME NOT NULL, command TEXT NOT NULL, "
                            " type TEXT NOT NULL, host TEXT NOT NULL,"
                            " UNIQUE(user_id,chat_id,command,type,host) ON CONFLICT REPLACE)");
        }
        else if(tablename == "config") {
            // procs table saves active monitors/aletrs in order to be restored
            // if the server goes down and up again later.
            m_err = !q.exec("CREATE TABLE config (name TEXT UNIQUE NOT NULL, "
                            "value DOUBLE NOT NULL, description TEXT NOT NULL)");
        }
        else if(tablename == "bookmarks") {
            m_err = !q.exec("CREATE TABLE bookmarks (history_rowid INTEGER NOT NULL,"
                            " timestamp DATETIME NOT NULL,"
                            " UNIQUE(history_rowid) ON CONFLICT REPLACE)");
        }
        else if(tablename == "auth") {
            m_err = !q.exec("CREATE TABLE auth (user_id INTEGER  PRIMARY KEY NOT NULL,"
                            " authorized INTEGER NOT NULL, "
                            " timestamp DATETIME NOT NULL )");
        }
        else if(tablename == "auth_limits") {
            m_err = !q.exec("create table auth_limits (operation TEXT NOT NULL, "
                            "user_id INTEGER NOT NULL, value INTEGER NOT NULL, "
                            "timestamp DATETIME NOT NULL, "
                            "PRIMARY KEY(user_id,operation) )");
        }
        else if(tablename == "private_chats") {
            m_err = !q.exec("CREATE TABLE private_chats (user_id INTEGER NOT NULL,"
                            " chat_id INTEGER NOT NULL, "
                            " PRIMARY KEY(user_id,chat_id) )");
        }
        else if(tablename == "history_types") {
            m_err = !q.exec("CREATE TABLE history_types(type TEXT PRIMARY KEY, description TEXT)");
        }

        if(m_err)
            m_msg = q.lastError().text();
        else
            printf("- BotDb.createDb: successfully created table \"%s\"\n", qstoc(tablename));
    }
    else {
        m_msg = "BotDb.createDb: database not open";
    }
    if(m_err)
        perr("BotDb.createDb: failed to create table %s: %s", qstoc(tablename), qstoc(m_msg));
}

void BotDb::m_printTable(const QString &table)
{
    if(!m_db.isOpen())
        perr("BotDb.m_printTable: Database is not open");
    else {
        QSqlQuery q(m_db);
        m_err = !q.exec(QString("SELECT * FROM %1").arg(table));
        if(!m_err) {
            printf("\e[1;32m%s\e[0m\n", qstoc(table));
            while(q.next()) {
                QSqlRecord rec = q.record();
                for(int i = 0; i < rec.count(); i++) {
                    printf("%s\t\t", qstoc(q.value(i).toString()));
                }
                printf("\n");
            }
        }
    }
}

int BotDb::m_findFirstAvailableIdx(const QList<int>& in_idxs)
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

bool BotDb::m_initUserChatsMap()
{
    m_msg.clear();
    if(!m_db.isOpen())
        return false;
    else {
        QSqlQuery q(m_db);
        m_err = !q.exec("SELECT user_id,chat_id FROM private_chats");
        while(!m_err && q.next())
            m_user_chatsMap.insert(q.value(0).toInt(), q.value(1).toInt());
        if(m_err)
            m_msg = "BotDb.m_initUserChatsMap: query  " + q.lastQuery() + "error: " + q.lastError().text();
    }
    return !m_err;
}

void BotDb::m_setErrorMessage(const QString &origin, const QSqlQuery &q)
{
    m_msg = origin + ": " + q.lastQuery() + ": " + q.lastError().text();
}
