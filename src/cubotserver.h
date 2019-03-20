#ifndef CUBOTSERVER_H
#define CUBOTSERVER_H

#include <QObject>
#include <tbotmsg.h>
#include <tbotmsgdecoder.h>
#include <botdb.h>
#include <botreader.h> // for BotReader::RefreshMode
#include "cubotmodule.h"

#include "../cumbia-telegram-defs.h" // for ControlMsg::Type

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

    void onReaderUpdate(int chat_id, const CuData& d);

    void onVolatileOperationExpired(int chat_id, const QString& opnam, const QString& text);


    // control server data
    void onNewControlServerData(int uid, int chat_id, ControlMsg::Type t, const QString& msg, QLocalSocket *so);

    void m_onReaderRefreshModeChanged(int user_id, int chat_id, const QString& src, const QString& host, BotReader::RefreshMode rm);


public slots:
    void start();
    void stop();

private:
    CuBotServerPrivate *d;

    void setupCumbia();
    void disposeCumbia();

    void m_loadModules();

    void m_loadPlugins();

    void m_setupMonitor();

    bool m_saveProcs();

    bool m_restoreProcs();

    bool m_broadcastShutdown();

    QList<HistoryEntry> m_prepareHistory(int uid, TBotMsgDecoder::Type t);
    void m_removeExpiredProcs(QList<HistoryEntry> &in);

    bool m_isBigSizeVector(const CuData &da) const;


    // CuBotModuleListener interface
public:
    void onSendMessageRequest(int chat_id, const QString &msg, bool silent, bool wait_for_reply);
    void onReplaceVolatileOperationRequest(int chat_id, CuBotVolatileOperation *vo);
    void onAddVolatileOperationRequest(int chat_id, CuBotVolatileOperation *vo);
    void onStatsUpdateRequest(int chat_id, const CuData& data);

    // CuBotModuleListener interface
public:
    void onReinjectMessage(const TBotMsg &msg_mod);
};

#endif // CUBOTSERVER_H
