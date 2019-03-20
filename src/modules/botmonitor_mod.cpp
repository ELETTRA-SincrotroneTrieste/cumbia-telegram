#include "botmonitor_mod.h"
#include "botconfig.h"
#include "monitorhelper.h"
#include <QString>
#include <QMap>
#include <QDateTime>
#include <QVariant>
#include <cudata.h>
#include <botreader.h>
#include <cumbiapool.h>
#include <cucontrolsfactorypool.h>
#include <cuformulaparsehelper.h>
#include <QtDebug>
#include <QRegularExpression>
#include <QRegularExpressionMatch>
#include "cumbiasupervisor.h"
#include "botplotgenerator.h"
#include "msgformatter.h"
#include "botconfig.h"
#include "botdb.h"
#include "botstats.h"
#include "botmonitor_msgdecoder.h"
#include "cuformulaplugininterface.h"
#include "auth.h"

class BotMonitorPrivate {
public:
    QMultiMap<int, BotReader *> readersMap;
    bool err;
    QString msg;
    CumbiaPool *cu_pool;
    CuControlsFactoryPool ctrl_factory_pool;
    int ttl, poll_period, max_avg_poll_period;
    CuBotModuleListener *listener;
    BotDb *db;
    BotStats *stats;
    BotConfig *bot_conf;
    BotMonitorMsgDecoder mon_msg_decoder;
    QMap<QString, QVariant> options;
};

BotMonitor::~BotMonitor()
{
    printf("\e[1;31m~BotMonitor %p deleting %d readers\e[0m\n", this, readers().size());
    foreach(BotReader *r, readers())
        delete r;
}

BotMonitor::BotMonitor(QObject *parent, const CumbiaSupervisor &cu_s,
                       int time_to_live, int poll_period)
    : QObject(parent)
{
    d = new BotMonitorPrivate;
    d->err = false;
    d->ctrl_factory_pool = cu_s.ctrl_factory_pool;
    d->cu_pool = cu_s.cu_pool;
    d->mon_msg_decoder.setNormalizedFormulaPattern(cu_s.formulaPlugin()->getFormulaParserInstance()->normalizedFormulaPattern());

    d->ttl = time_to_live;
    d->poll_period = poll_period;
    d->max_avg_poll_period = 1000;
    d->listener = nullptr;
    d->stats = nullptr;
    d->bot_conf = nullptr;

    connect(this, SIGNAL(newMonitorData(int, const CuData&)),
            this, SLOT(onNewMonitorData(int, const CuData&)));
    connect(this, SIGNAL(stopped(int, int, QString, QString, QString)),
            this, SLOT(onSrcMonitorStopped(int, int, QString, QString, QString)));
    connect(this, SIGNAL(started(int,int, QString,QString,QString)), this, SLOT(onSrcMonitorStarted(int,int, QString,QString,QString)));
    connect(this, SIGNAL(onFormulaChanged(int, int, QString,QString,QString,QString)),
            this, SLOT(onSrcMonitorFormulaChanged(int, int, QString,QString,QString,QString)));
    connect(this, SIGNAL(onMonitorTypeChanged(int,int, QString,QString,QString,QString)),
            this, SLOT(onSrcMonitorTypeChanged(int,int, QString,QString,QString,QString)));
    connect(this, SIGNAL(startError(int, const QString&, const QString&)), this,
            SLOT(onSrcMonitorStartError(int, const QString&, const QString&)));
    connect(this, SIGNAL(readerRefreshModeChanged(int, int, const QString &, const QString &,BotReader::RefreshMode )),
            this, SLOT(m_onReaderRefreshModeChanged(int, int , const QString&, const QString&, BotReader::RefreshMode)));
}

bool BotMonitor::error() const
{
    return d->err;
}

QString BotMonitor::message() const
{
    return d->msg;
}

BotReader *BotMonitor::findReader(int chat_id, const QString &expression, const QString &host) const
{
    CuFormulaParseHelper ph;
    QSet<QString> srcs = ph.sources(expression).toSet(); // extract sources from expression
    for(QMap<int, BotReader *>::iterator it = d->readersMap.begin(); it != d->readersMap.end(); ++it) {
        qDebug() << __PRETTY_FUNCTION__ << "find srcs" << srcs << "reader srcs" << it.value()->sources() <<
                    "find host" << host << " reader host " << it.value()->host();
        if(it.key() == chat_id && it.value()->sameSourcesAs(srcs) && it.value()->host() == host)
            return *it;
    }
    printf("\e[1;31mreader for %d %s not found\e[0m\n", chat_id, qstoc(expression));
    return nullptr;
}

BotReader *BotMonitor::findReaderByUid(int user_id, const QString &expression, const QString& host) const
{
    CuFormulaParseHelper ph;
    QString expr = ph.toNormalizedForm(expression);
    QSet<QString> srcs = ph.sources(expr).toSet(); // extract sources from expression
    for(QMap<int, BotReader *>::iterator it = d->readersMap.begin(); it != d->readersMap.end(); ++it) {
        if(it.key() == user_id && it.value()->sameSourcesAs(srcs) && it.value()->host() == host) {
            return *it;
        }
    }
    return nullptr;
}

QList<BotReader *> BotMonitor::readers() const
{
    return d->readersMap.values();
}

void BotMonitor::setMaxAveragePollingPeriod(int millis)
{
    d->max_avg_poll_period = millis;
}

int BotMonitor::maxAveragePollingPeriod() const
{
    return d->max_avg_poll_period;
}

bool BotMonitor::stopAll(int chat_id, const QStringList &srcs)
{
    qDebug() << __PRETTY_FUNCTION__ << chat_id << srcs;
    bool found = false;
    d->err = !d->readersMap.contains(chat_id);
    QMutableMapIterator<int, BotReader *> it(d->readersMap);
    while(it.hasNext()) {
        it.next();
        if(it.key() == chat_id) {
            BotReader *r = it.value();
            bool delet = srcs.isEmpty();
            for(int i = 0; i < srcs.size() && !delet; i++) {
                delet = r->sourceMatch(srcs[i]);
                qDebug() << __PRETTY_FUNCTION__ << chat_id << "fickin match" << srcs[i] << delet;
            }
            if(delet) {
                emit stopped(r->userId(), chat_id, r->source(), r->host(), "user request");
                r->deleteLater();
                it.remove();
                found = true;
            }
        }
    }
    if(!found)
        d->msg = "BotMonitor.stop: none of the sources matching one of the patterns"
                 " \"" + srcs.join(", ") + "\" are monitored";
    return !d->err && found;
}


bool BotMonitor::stopByIdx(int chat_id, int index)
{
    printf("BotMonitor.stopByIdx chat id %d index %d\n", chat_id, index);
    bool found = false;
    QMutableMapIterator<int, BotReader *> it(d->readersMap);
    while(it.hasNext() && !found) {
        it.next();
        if(it.key() == chat_id) {
            BotReader *r = it.value();
            if(r->index() == index) {
                emit stopped(r->userId(), chat_id, r->source(), r->host(), "user request");
                r->deleteLater();
                it.remove();
                found = true;
            }
        }
    }
    if(!found)
        d->msg = "BotMonitor.stopByIdx: no reader found with index " + QString::number(index);
    return found;
}

bool BotMonitor::startRequest(int user_id,
                              int chat_id,
                              int uid_monitor_limit,
                              const QString &src,
                              const QString& cmd,
                              BotReader::Priority priority,
                              const QString &host,
                              const QDateTime& started_on)
{
    qDebug() << __PRETTY_FUNCTION__ << "startRequest with host " << host << "SOURCE " << src;
    d->err = false;
    BotReader *reader = findReader(chat_id, src, host);
    d->err = (reader && src == reader->source() && priority == reader->priority() );
    if(d->err){
        d->msg = "BotMonitor.startRequest: chat_id " + QString::number(chat_id) + " is already monitoring \"" + cmd;
        !reader->command().isEmpty() ? (d->msg += " " + reader->command() + "\"") : (d->msg += "\"");
        perr("%s", qstoc(d->msg));
    }
    else if( reader && src == reader->source() ) { // same source but priority changed
        reader->setPriority(priority);
    }
    else if(reader) {
        QString oldcmd = reader->command();
        reader->unsetSource();
        reader->setCommand(cmd);
        reader->setSource(src);
        m_onFormulaChanged(user_id, chat_id, reader->sources().join(","), oldcmd, reader->command(), host);
    }
    else {
        int uid_readers_cnt = d->readersMap.values(chat_id).size();
        if(uid_readers_cnt < uid_monitor_limit) {
            bool monitor = true;
            BotReader *reader = new BotReader(user_id, chat_id, this,
                                              d->cu_pool, d->ctrl_factory_pool,
                                              d->ttl, d->poll_period, cmd, priority, host, monitor);
            connect(reader, SIGNAL(newData(int, const CuData&)), this, SLOT(m_onNewData(int, const CuData&)));
            connect(reader, SIGNAL(formulaChanged(int, int, QString,QString, QString,QString)),
                    this, SLOT(m_onFormulaChanged(int, int, QString,QString, QString,QString)));
            connect(reader, SIGNAL(priorityChanged(int, const QString&, BotReader::Priority , BotReader::Priority)),
                    this, SLOT(m_onPriorityChanged(int, const QString&, BotReader::Priority , BotReader::Priority)));
            connect(reader, SIGNAL(startSuccess(int, int, QString, QString)),
                    this, SLOT(readerStartSuccess(int, int, QString, QString)));
            connect(reader, SIGNAL(lastUpdate(int, const CuData&)), this, SLOT(m_onLastUpdate(int, const CuData&)));
            reader->setSource(src);
            reader->setStartedOn(started_on);
            reader->setIndex(m_findIndexForNewReader(chat_id));
        }
        else {
            emit startError(chat_id, src, QString("limit of %1 monitors already reached").arg(uid_monitor_limit));
        }

        // will be inserted in map upon successful "property" delivery
    }

    return !d->err;
}

void BotMonitor::readerStartSuccess(int user_id, int chat_id, const QString &src, const QString &formula)
{
    printf("\e[1;32m+++ readerStart success %d %d  %s %s \e[0m\n", user_id, chat_id, qstoc(src), qstoc(formula));
    BotReader *reader = qobject_cast<BotReader *>(sender());
    d->readersMap.insert(chat_id, reader);
    emit started(user_id, chat_id, src, reader->host(), formula);
}

void BotMonitor::m_onNewData(int chat_id, const CuData &da)
{
    BotReader *reader = qobject_cast<BotReader *>(sender());
    const QString src = reader->source();
    const QString host = reader->host();
    const QString msg = QString::fromStdString(da["msg"].toString());
    d->err = da["err"].toBool();
    if(d->err) {
        perr("BotMonitor.m_onNewData: error chat_id %d msg %s", chat_id, qstoc(msg));
        emit stopped(reader->userId(), chat_id, src, host, msg);
    }
    // emit new data if it has a value
    else if(da.containsKey("value"))
        emit newMonitorData(chat_id, da);
}

void BotMonitor::m_onFormulaChanged(int user_id, int chat_id, const QString &src, const QString &old, const QString &new_f, const QString& host)
{
    emit onFormulaChanged(user_id, chat_id, src, host, old, new_f);
}

void BotMonitor::m_onPriorityChanged(int chat_id, const QString &src, BotReader::Priority oldpri, BotReader::Priority newpri)
{
    BotReader *reader = qobject_cast<BotReader *>(sender());
    int user_id = reader->userId();
    QString oldtype, newtype;
    oldpri == BotReader::Low ? oldtype = "monitor" : oldtype = "alert";
    newpri == BotReader::Low ? newtype = "monitor" : newtype = "alert";
    emit onMonitorTypeChanged(user_id, chat_id, src, reader->host(), oldtype, newtype);
}

void BotMonitor::m_onLastUpdate(int chat_id, const CuData &)
{
    BotReader *reader = qobject_cast<BotReader *>(sender());
    emit stopped(reader->userId(), chat_id, reader->source(), reader->host(), "end of TTL");
    reader->deleteLater();
    QMap<int, BotReader *>::iterator it = d->readersMap.begin();
    while(it != d->readersMap.end())
        it.value() == reader ? it =  d->readersMap.erase(it) : ++it;
}

int BotMonitor::m_findIndexForNewReader(int chat_id)
{
    QList<int> indexes;
    // readers map populated by readerStartSuccess. key is chat_id
    foreach(BotReader* r, d->readersMap.values(chat_id)) {
        if(r->index() > -1)
            indexes << r->index();
    }
    int i = 1;
    for(i = 1; i <= indexes.size(); i++)
        if(!indexes.contains(i)) // found a "hole"
            return i;
    return i;
}

void BotMonitor::m_onReaderModeChanged(BotReader::RefreshMode rm)
{
    BotReader *r = qobject_cast<BotReader *>(sender());
    emit readerRefreshModeChanged(r->userId(), r->chatId(), r->source(), r->host(), rm);
}


/**
 * @brief BotMonitor::onNewMonitorData send message on new monitored data
 *
 * Message is sent silently
 *
 * @param chat_id
 * @param da
 *
 */
void BotMonitor::onNewMonitorData(int chat_id, const CuData &da)
{
    MsgFormatter mf;
    d->listener->onSendMessageRequest(chat_id, mf.fromData(da), da["silent"].toBool());
    if(m_isBigSizeVector(da)) {
        d->listener->onReplaceVolatileOperationRequest(chat_id, new BotPlotGenerator(chat_id, da));
    }
    d->stats->addRead(chat_id, da); // da is passed for error stats
}

void BotMonitor::onSrcMonitorStopped(int user_id, int chat_id, const QString &src,
                                     const QString &host, const QString &message)
{
    const bool silent = true;
    MonitorHelper mh;
    mh.adjustPollers(this, d->bot_conf->poll_period(), d->bot_conf->max_avg_poll_period());

    BotReader *r = this->findReader(chat_id, src, host);
    d->listener->onSendMessageRequest(chat_id,  MsgFormatter().monitorStopped(r->command(), message), silent);
    // update database, remove rows for chat_id and src
    d->db->monitorStopped(chat_id, src);
}

void BotMonitor::onSrcMonitorStarted(int user_id, int chat_id, const QString &src,
                                     const QString& host, const QString& formula)
{
    // when a new reader starts monitoring, the polling period of all readers must be adjusted
    // not to overload the poll operation. This is done every time the refresh mode of a reader
    // changes (signalled by BotMonitor::readerRefreshModeChanged to this slot m_onReaderRefreshModeChanged)
    // and every time a monitor is stopped (BotMonitor::onSrcMonitorStopped)
    // We can't do this here because at this stage we do not have the information about the mode yet.
    // See BotMonitor::m_onReaderRefreshModeChanged
    const QDateTime until = QDateTime::currentDateTime().addSecs(d->bot_conf->ttl());
    BotReader *r = findReader(chat_id, src, host);
    BotReader::Priority pri = r->priority();
    d->listener->onSendMessageRequest(chat_id, MsgFormatter().monitorUntil(r->command(), until));
    // record new monitor into the database
    qDebug() << __PRETTY_FUNCTION__ << "adding history entry with formula " << formula << "host " << r->host();
    HistoryEntry he(user_id, r->command(), pri == BotReader::High ? "alert" :  "monitor", host);
    d->db->addToHistory(he);
}

void BotMonitor::onSrcMonitorStartError(int chat_id, const QString &src, const QString &message)
{
    d->listener->onSendMessageRequest(chat_id, MsgFormatter().srcMonitorStartError(src, message));
}

void BotMonitor::onSrcMonitorFormulaChanged(int user_id, int chat_id, const QString &new_s, const QString& host,
                                            const QString &old, const QString &new_f)
{
    d->listener->onSendMessageRequest(chat_id, MsgFormatter().formulaChanged(new_s, old, new_f));
    BotReader *r = findReader(chat_id, new_s, host);
    printf("\e[1;33mADD TO HISTORY NEW ENTRY: %s\e[0m\n", qstoc(new_s));
    HistoryEntry he(user_id, new_f, r->priority() == BotReader::Low ? "monitor" : "alert", r->host());
    d->db->addToHistory(he);
}

void BotMonitor::onSrcMonitorTypeChanged(int user_id, int chat_id, const QString &src,
                                         const QString& host, const QString &old_type, const QString &new_type)
{
    d->listener->onSendMessageRequest(chat_id, MsgFormatter().monitorTypeChanged(src, old_type, new_type));
    BotReader *r = findReader(chat_id, src, host);
    HistoryEntry he(user_id, r->command(), new_type,  r->host());
    d->db->addToHistory(he);
}

void BotMonitor::setBotmoduleListener(CuBotModuleListener *l)
{
    d->listener = l;
}

void BotMonitor::setOption(const QString &key, const QVariant &value)
{
    d->options[key] = value;
}

int BotMonitor::type() const
{
    return  BotMonitorType;
}

QString BotMonitor::name() const
{
    return "BotMonitor";
}

QString BotMonitor::description() const
{
    return "Provides monitor and alert services for the cumbia-telegram bot";
}

QString BotMonitor::help() const
{
    return QString();
}

CuBotModule::AccessMode BotMonitor::needsDb() const
{
    return CuBotModule::ReadWrite;
}

CuBotModule::AccessMode BotMonitor::needsStats() const
{
    return CuBotModule::ReadWrite;
}

void BotMonitor::setDb(BotDb *db)
{
    d->db = db;
}

void BotMonitor::setStats(BotStats *stats)
{
    d->stats = stats;
}

void BotMonitor::setConf(BotConfig *conf)
{
    d->bot_conf = conf;
}

int BotMonitor::decode(const TBotMsg &msg)
{
    BotMonitorMsgDecoder::Type t = d->mon_msg_decoder.decode(msg);
    if(t != BotMonitorMsgDecoder::Invalid)
        return type();
    return -1;
}

bool BotMonitor::process()
{
    bool success = false;
    BotMonitorMsgDecoder::Type t = d->mon_msg_decoder.type();
    int chat_id = d->mon_msg_decoder.chatId();
    if(t == BotMonitorMsgDecoder::Monitor || t == BotMonitorMsgDecoder::Alert) {
        QString src = d->mon_msg_decoder.source();
        QString host; // if m.hasHost use it, it comes from a fake history message created ad hoc by History
        d->mon_msg_decoder.hasHost() ? host = d->mon_msg_decoder.host() : host = m_getHost(d->mon_msg_decoder.chatId()); // may be empty. If so, TANGO_HOST will be used
        // src = CuFormulaParseHelper().injectHost(host, src);
        // m.start_dt will be invalid if m is decoded by a real message
        // m.start_dt is forced to a given date and time when m is a fake msg built
        // from the database history
        success = startRequest(d->mon_msg_decoder.userId(), chat_id,
                               d->bot_conf->getDefaultAuth("monitors"), src, d->mon_msg_decoder.text(),
                               t == BotMonitorMsgDecoder::Monitor ? BotReader::Low : BotReader::High,
                               host, d->mon_msg_decoder.startDateTime());
        if(!success)
            d->listener->onSendMessageRequest(d->mon_msg_decoder.chatId(),
                                              MsgFormatter().error("CuBotServer", d->msg));
    }
    else if(t == BotMonitorMsgDecoder::StopMonitor && d->mon_msg_decoder.cmdLinkIdx() < 0) {
        QStringList srcs = CuFormulaParseHelper().sources(d->mon_msg_decoder.source());
        printf("\e[1;32mBotMonitor::process: received StopMonitor cmd source is %s\e[0m\n", qstoc(srcs.join(", ")));
        printf("\e[1;32mBotMonitor::process: received StopMonitor stop pattern is %s\e[0m\n", qstoc(d->mon_msg_decoder.getArgs().join(", ")));
        success = stopAll(d->mon_msg_decoder.chatId(), srcs.isEmpty() ? d->mon_msg_decoder.getArgs() : srcs);
        if(!success) {
            d->listener->onSendMessageRequest(chat_id, MsgFormatter().error("CuBotServer", d->msg));
            // failure in this phase means reader is already stopped (not in monitor's map).
            // make sure it is deleted from the database too
            // ..
        }
    }
    else if(t == BotMonitorMsgDecoder::StopMonitor && d->mon_msg_decoder.cmdLinkIdx() > 0) {
        // stop by reader index!
        printf("\e[1;32mBotMonitor::process: received StopMonitor by index %d\n", d->mon_msg_decoder.cmdLinkIdx());
        success = stopByIdx(chat_id, d->mon_msg_decoder.cmdLinkIdx());
        if(!success) {
            d->listener->onSendMessageRequest(chat_id, MsgFormatter().error("CuBotServer", d->msg));
        }
    }
    return success;
}

bool BotMonitor::isVolatileOperation() const
{
    return false;
}

bool BotMonitor::m_isBigSizeVector(const CuData &da) const
{
    if(da.containsKey("value") && da.containsKey("data_format_str")
            && da["data_format_str"].toString() == std::string("vector")) {
        const CuVariant& val = da["value"];
        return val.getSize() > 5;
    }
    return false;
}

QString BotMonitor::m_getHost(int chat_id, const QString &src)
{

    QString host;
    bool needs_host = true;
    if(!src.isEmpty()) {
        std::string domain = d->ctrl_factory_pool.guessDomainBySrc(src.toStdString());
        needs_host = (domain == "tango");
    }
    if(needs_host) {
        host = d->db->getSelectedHost(chat_id);
        if(host.isEmpty())
            host = QString(secure_getenv("TANGO_HOST"));
    }
    return host;
}



