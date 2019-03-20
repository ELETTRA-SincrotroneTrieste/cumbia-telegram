#ifndef BOTMONITOR_H
#define BOTMONITOR_H

#include <QObject>
#include <botreader.h>
#include <QDateTime>
#include <cubotmodule.h>

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

    QList<BotReader *>  readers() const;

    void setMaxAveragePollingPeriod(int millis);

    int maxAveragePollingPeriod() const;

signals:

    void stopped(int user_id, int chat_id, const QString& src, const QString& host, const QString& message);

    void started(int user_id, int chat_id, const QString& src, const QString& host, const QString& formula);

    void startError(int chat_id, const QString& src, const QString& message);

    void newMonitorData(int chat_id, const CuData& data);

    void onFormulaChanged(int user_id, int chat_id, const QString& src, const QString& host, const QString& old, const QString& new_f);
    void onMonitorTypeChanged(int user_id, int chat_id, const QString& src, const QString& host, const QString& old_t, const QString& new_t);

    void readerRefreshModeChanged(int user_id, int chat_id, const QString &src, const QString &host,  BotReader::RefreshMode);

public slots:

    bool stopAll(int chat_id, const QStringList &srcs);

    bool stopByIdx(int chat_id, int index);

    bool startRequest(int user_id, int chat_id, int uid_monitor_limit,
                      const QString& src,
                      const QString& cmd, BotReader::Priority priority,
                      const QString& host = QString(),
                      const QDateTime& startedOn = QDateTime());

    void readerStartSuccess(int user_id, int chat_id, const QString& src, const QString& formula);

private:
    BotMonitorPrivate *d;

private slots:
    void m_onNewData(int, const CuData&);

    void m_onFormulaChanged(int user_id, int chat_id, const QString &src, const QString &old, const QString &new_f, const QString& host);

    void m_onPriorityChanged(int chat_id, const QString& src,
                                BotReader::Priority oldpri, BotReader::Priority newpri);

    void m_onLastUpdate(int chat_id, const CuData& dat);

    int m_findIndexForNewReader(int chat_id);

    void m_onReaderModeChanged(BotReader::RefreshMode rm);


    void onNewMonitorData(int chat_id, const CuData& da);

    void onSrcMonitorStopped(int user_id, int chat_id, const QString& src, const QString& host, const QString& message);

    void onSrcMonitorStarted(int user_id, int chat_id, const QString& src, const QString &host, const QString &formula);

    void onSrcMonitorStartError(int chat_id, const QString& src, const QString& message);

    void onSrcMonitorFormulaChanged(int user_id, int chat_id, const QString &new_s,
                                    const QString &host, const QString &old, const QString &new_f);

    void onSrcMonitorTypeChanged(int user_id, int chat_id, const QString& src,
                                 const QString& host, const QString& old_type, const QString& new_type);


    // CuBotModule interface
public:
    void setBotmoduleListener(CuBotModuleListener *l);
    void setOption(const QString& key, const QVariant& value);
    int type() const;
    QString name() const;
    QString description() const;
    QString help() const;
    AccessMode needsDb() const;
    AccessMode needsStats() const ;
    void setDb(BotDb *db);
    void setStats(BotStats *stats) ;
    void setConf(BotConfig *conf);
    int decode(const TBotMsg &msg);
    bool process();
    bool isVolatileOperation() const;

private:

    bool m_isBigSizeVector(const CuData &da) const;
    QString m_getHost(int chat_id, const QString& src = QString());

    // database
};

#endif // BOTMONITOR_H
