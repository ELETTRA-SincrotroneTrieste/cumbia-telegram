#ifndef CUBOTSERVER_H
#define CUBOTSERVER_H



#include <QObject>
#include <tbotmsg.h>
#include <tbotmsgdecoder.h>
#include <botreader.h> // for BotReader::RefreshMode
#include <cubotmodule.h>

#include "../../cumbia-telegram-defs.h" // for ControlMsg::Type

class CuBotServerPrivate;
class QJsonValue;
class CuData;
class QLocalSocket;

class CuBotServer : public QObject, public CuBotModuleListener
{
    Q_OBJECT
public:
    explicit CuBotServer(QObject *parent, const QString &bot_key, const QString& sqlite_db_filenam);

    virtual ~CuBotServer();

    bool isRunning() const;

protected:
    bool event(QEvent *e);

signals:

private slots:
    void onMessageReceived(const TBotMsg &m);

    void onMessageSent(int chat_id, int message_id, int key );

    // control server data
    void onNewControlServerData(int uid, int chat_id, ControlMsg::Type t, const QString& msg, QLocalSocket *so);

public slots:
    void start();
    void stop();

private:
    CuBotServerPrivate *d;

    void setupCumbia();
    void disposeCumbia();

    void m_loadModules();
    bool m_registerModule(CuBotModule *mod);
    void m_loadPlugins();
    void m_unloadAll();

    void m_setupMonitor();

    bool m_saveProcs();
    bool m_restoreProcs();

    bool m_broadcastShutdown();

    void m_removeExpiredProcs(QList<HistoryEntry> &in);

    bool m_isBigSizeVector(const CuData &da) const;

    QString m_unauthorized_msg(const QString &username, const QString &op_type, const QString &reason) const;

    // CuBotModuleListener interface
public:
    void onSendMessageRequest(int chat_id, const QString &msg, bool silent, bool wait_for_reply);
    void onEditMessageRequest(int chat_id, int key, const QString &msg, bool wait_for_reply);
    void onStatsUpdateRequest(int chat_id, const CuData& data);
    void onSendPictureRequest(int chat_id, const QByteArray &pic_ba);

    // CuBotModuleListener interface
public:
    void onReinjectMessage(const TBotMsg &msg_mod);

};

#endif // CUBOTSERVER_H
