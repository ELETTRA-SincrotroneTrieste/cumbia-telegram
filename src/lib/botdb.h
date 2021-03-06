#ifndef BOTDB_H
#define BOTDB_H

#include <QString>
#include <QtSql>
#include <QMap>

#include <historyentry.h>
#include <botconfig.h>
#include <hostentry.h>

class BotDb
{
public:
    BotDb(const QString &db_file);

    ~BotDb();

    enum Quality { Undefined = -1, Ok, Warning, Alarm, Invalid };

    bool addUser(int uid, const QString& uname, const QString& firstName = QString(), const QString& lastName = QString());

    bool removeUser(int uid);

    int addToHistory(const HistoryEntry &in, BotConfig *bconf);

    bool saveProc(const HistoryEntry &he);

    bool clearProcTable();

    bool unregisterProc(const HistoryEntry &he);

    bool registerHistoryType(const QString& newtype, const QString &description);

    QList<HistoryEntry> loadProcs();

    HistoryEntry lastOperation(int uid);

    HistoryEntry bookmarkLast(int uid);

    bool removeBookmark(int uid, int index);

    QList<HistoryEntry> history(int uid, const QString &type);

    int fixStaleItems(const QStringList& cmd_list);

    QMap<int, QString> usersById();

//    QList<HistoryEntry> bookmarks(int uid);

    HistoryEntry commandFromIndex(int uid, const QString &type, int index);

    HistoryEntry historyEntryFromCmd(int uid, const QString& command);

    bool monitorStopped(int user_id, const QString& command, const QString& host);

    bool error() const;

    QString message() const;

    Quality strToQuality(const QString& qs) const;

    QString qualityToStr(Quality q) const;

    bool userExists(int uid);

    QList<HostEntry> getHostList();

    bool setHost(int user_id, int chat_id, int host_idx, QString& new_host, QString &new_host_description);
    bool setHost(int user_id, int chat_id, const QString &host, QString& new_host_description);

    QString getSelectedHost(int chat_id);

    bool getConfig(QMap<QString, QVariant>& datamap, QMap<QString, QString> &descmap);

    int isAuthorized(int uid, const QString &operation, bool *unregistered = nullptr);

    bool userInPrivateChat(int uid, int chat_id);
    bool addUserInPrivateChat(int uid, int chat_id);
    QList<int> chatsForUser(int uid);

    QList<int> getChatsWithActiveMonitors();

    const QSqlDatabase *getSqlDatabase() const;

    bool canWrite(int user_id);
private:
    QSqlDatabase m_db;
    bool m_err;
    QString m_msg;

    void createDb(const QString &tablename) ;

    void m_printTable(const QString &table);

    int m_findFirstAvailableIdx(const QList<int> &in_idxs);

    bool m_initUserChatsMap();

    void m_setErrorMessage(const QString& origin, const QSqlQuery& q);

    QMultiMap<int, int> m_user_chatsMap;

};

#endif // BOTDB_H
