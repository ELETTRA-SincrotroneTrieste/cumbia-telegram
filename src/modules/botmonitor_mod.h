#ifndef BOTMONITOR_H
#define BOTMONITOR_H

#include <QObject>
#include <QDateTime>

#include "lib/botreader.h"
#include "lib/cubotmodule.h"
#include "botmonitor_msgdecoder.h"

class CuData;
class QString;
class BotMonitorPrivate;
class CumbiaSupervisor;

class BotMonitor : public QObject, public CuBotModule
{
    Q_OBJECT
public:

    enum ModuleType { BotMonitorType = 1000 };

    ~BotMonitor();

    explicit BotMonitor(QObject *parent, const CumbiaSupervisor& cu_s,
                        int time_to_live, int poll_period);

    bool error() const;

    QString message() const;

    BotReader* findReader(int chat_id, const QString& expression, const QString& host) const;

    BotReader* findReaderByUid(int user_id, const QString& src, const QString& host) const;

    QString srcDescription() const;

    QList<BotReader *>  readers() const;

    void setMaxAveragePollingPeriod(int millis);

    int maxAveragePollingPeriod() const;

signals:

    void stopped(int user_id, int chat_id, const QString& src, const QString& command, const QString& host, const QString& message);

    void started(int user_id, int chat_id, const QString& src, const QString& host, const QString& formula);

    void startError(int chat_id, const QString& src, const QString& message);

    void newMonitorData(int chat_id, const CuData& data);

    void onFormulaChanged(int user_id, int chat_id, const QString& src, const QString& host, const QString& old, const QString& new_f);

    void readerRefreshModeChanged(int user_id, int chat_id, const QString &src, const QString &host,  BotReader::RefreshMode);

public slots:

    bool stopAll(int chat_id, const QStringList &srcs);

    bool stopByIdx(int chat_id, int index);

    bool startRequest(int user_id, int chat_id, int uid_monitor_limit,
                      const QString& src,
                      const QString& cmd, BotReader::Priority priority,
                      const QString& host,
                      const QString &description,
                      const QDateTime& startedOn);

private:
    BotMonitorPrivate *d;

private slots:
    void m_onNewData(int, const CuData&);

    void m_onFormulaChanged(int user_id, int chat_id, const QString &src, const QString &old, const QString &new_f, const QString& host);

    int m_onPriorityChanged(int user_id, int chat_id,
                             const QString& oldcmd,
                             const QString& newcmd,
                            BotReader::Priority newpri,
                             const QString &host);

    void m_onAlreadyMonitoring(int chat_id, const QString& cmd, const QString& host);

    void m_onLastUpdate(int chat_id, const CuData& dat);

//    int m_findIndexForNewReader(int chat_id);

    void m_onReaderModeChanged(BotReader::RefreshMode rm);


    void onNewMonitorData(int chat_id, const CuData& da);

    void onSrcMonitorStopped(int user_id, int chat_id, const QString& src, const QString& command, const QString& host, const QString& message);

    void onSrcMonitorStarted(int user_id, int chat_id, const QString& src, const QString &formula, const QString &host);

    void onSrcMonitorStartError(int chat_id, const QString& src, const QString& message);

    void onSrcMonitorFormulaChanged(int user_id, int chat_id, const QString &new_s,
                                    const QString &host, const QString &old, const QString &new_f);

public:
    void setOption(const QString& key, const QVariant& value);
    int type() const;
    QString name() const;
    QString description() const;
    QString help() const;
    AccessMode needsDb() const;
    AccessMode needsStats() const ;
    int decode(const TBotMsg &msg);
    bool process();

private:

    bool m_isBigSizeVector(const CuData &da) const;

    // database
};

#endif // BOTMONITOR_H
