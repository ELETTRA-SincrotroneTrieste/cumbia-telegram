#include "botmonitor_mod.h"
#include "../lib/botconfig.h"
#include "../lib/cuformulaparsehelper.h"
#include "../lib/cumbiasupervisor.h"
#include "../lib/botplotgenerator.h"
#include "../lib/botconfig.h"
#include "../lib/botdb.h"

#include "monitorhelper.h"
#include <QString>
#include <QMap>
#include <QDateTime>
#include <QVariant>
#include <cudata.h>
#include "../lib/botreader.h"
#include <cumbiapool.h>
#include <cucontrolsfactorypool.h>
#include <QtDebug>
#include <QRegularExpression>
#include <QRegularExpressionMatch>
#include "botstats.h"
#include "botmonitor_msgdecoder.h"
#include "cuformulaplugininterface.h"
#include "auth.h"
#include "moduleutils.h"
#include "../lib/datamsgformatter.h"
#include "botmonitor_mod_msgformatter.h" // in modules dir
#include "../lib/generic_msgformatter.h"

class BotMonitorPrivate {
public:
    QMultiMap<int, BotReader *> readersMap;
    bool err;
    QString msg;
    CumbiaPool *cu_pool;
    CuControlsFactoryPool ctrl_factory_pool;
    int ttl, poll_period, max_avg_poll_period;
    BotMonitorMsgDecoder *mon_msg_decoder;
    CuFormulaParseHelper *formula_parser_helper;
    QString src_description;
};

BotMonitor::~BotMonitor()
{
    printf("\e[1;31m~BotMonitor %p deleting %d readers\e[0m\n", this, readers().size());
    delete d->formula_parser_helper;
    delete d->mon_msg_decoder;
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
    d->mon_msg_decoder = new BotMonitorMsgDecoder(d->ctrl_factory_pool);
    d->mon_msg_decoder->setNormalizedFormulaPattern(cu_s.formulaPlugin()->getFormulaParserInstance()->normalizedFormulaPattern());

    d->ttl = time_to_live;
    d->poll_period = poll_period;
    d->max_avg_poll_period = 1000;
    d->formula_parser_helper = new CuFormulaParseHelper(d->ctrl_factory_pool);

    connect(this, SIGNAL(newMonitorData(int, const CuData&, int)),
            this, SLOT(onNewMonitorData(int, const CuData&, int)));
    connect(this, SIGNAL(stopped(int, int, QString, QString, QString, QString)),
            this, SLOT(onSrcMonitorStopped(int, int, QString, QString,QString, QString)));
    connect(this, SIGNAL(onFormulaChanged(int, int, QString,QString,QString,QString)),
            this, SLOT(onSrcMonitorFormulaChanged(int, int, QString,QString,QString,QString)));
    connect(this, SIGNAL(startError(int, const QString&, const QString&)), this,
            SLOT(onSrcMonitorStartError(int, const QString&, const QString&)));

    // init options used before sending message
    setOption("sound", "off");
    setOption("new_msg", "off");
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
    QSet<QString> srcs = d->formula_parser_helper->sources(expression).toSet(); // extract sources from expression
    for(QMap<int, BotReader *>::iterator it = d->readersMap.begin(); it != d->readersMap.end(); ++it) {
        qDebug() << __PRETTY_FUNCTION__ << "find srcs" << srcs << "reader srcs" << it.value()->sources() <<
                    "find host" << host << " reader host " << it.value()->host() <<
                    "find formula " << expression << "reader formula " << it.value()->source()
                 << "find expression " << expression << " reader command " << it.value()->command();
        if(it.key() == chat_id && it.value()->sameSourcesAs(srcs) && it.value()->host() == host
                && expression == it.value()->source())
            return *it;
    }
    printf("\e[1;31mreader for %d %s NOT FOUND\e[0m\n", chat_id, qstoc(expression));
    return nullptr;
}

BotReader *BotMonitor::findReaderByUid(int user_id, const QString &expression, const QString& host) const
{
    QString expr = d->formula_parser_helper->toNormalizedForm(expression);
    QSet<QString> srcs = d->formula_parser_helper->sources(expr).toSet(); // extract sources from expression
    for(QMap<int, BotReader *>::iterator it = d->readersMap.begin(); it != d->readersMap.end(); ++it) {
        if(it.key() == user_id && it.value()->sameSourcesAs(srcs) && it.value()->host() == host
                && expression == it.value()->source()) {
            return *it;
        }
    }
    return nullptr;
}

QString BotMonitor::srcDescription() const
{
    return d->src_description;
}

QList<BotReader *> BotMonitor::readers() const
{
    return d->readersMap.values();
}

QList<BotReader *> BotMonitor::readers(int chat_id) const
{
    return  d->readersMap.values(chat_id);
}

int BotMonitor::readersCount(int chat_id) const
{
    return d->readersMap.count(chat_id);
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
    d->err = !d->readersMap.contains(chat_id);
    QMutableMapIterator<int, BotReader *> it(d->readersMap); // multimap
    while(!d->err && it.hasNext()) {
        it.next();
        if(it.key() == chat_id) {
            BotReader *r = it.value();
            for(int i = 0; i < srcs.size(); i++) {
                if(r->sourceMatch(srcs[i])) {
                    qDebug() << __PRETTY_FUNCTION__ << chat_id << "match" << srcs[i];
                    emit stopped(r->userId(), chat_id, r->source(), r->command(), r->host(), "user request");
                    r->deleteLater();
                }
            }
            it.remove();
        }
    }
    if(d->err)
        d->msg = "BotMonitor.stop: none of the sources matching one of the patterns"
                 " \"" + srcs.join(", ") + "\" are monitored";
    return !d->err;
}


bool BotMonitor::stopByIdx(int chat_id, int index)
{
    printf("BotMonitor.stopByIdx chat id %d index %d\n", chat_id, index);
    bool found = false;
    BotReader *r = nullptr;
    QMutableMapIterator<int, BotReader *> it(d->readersMap); // multimap!
    while(it.hasNext()) {
        it.next();
        if(it.key() == chat_id) {
            r = it.value();
            if(r->index() == index) {
                it.remove();
                found = true;
                emit stopped(r->userId(), chat_id, r->source(), r->command(), r->host(), "user request");
                r->deleteLater();
                // found but this is a multimap, so there may be multiple entries with chat_id
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
                              const QString& description,
                              const QDateTime& started_on)
{
    qDebug() << __PRETTY_FUNCTION__ << "startRequest with host " << host << "SOURCE " << src;
    d->err = false;
    BotReader *reader = findReader(chat_id, src, host);
    d->err = (reader && src == reader->source() && priority == reader->priority() );
    d->src_description = description;
    if(d->err){
        d->msg = "BotMonitor.startRequest: chat_id " + QString::number(chat_id) + " is already monitoring \"" + cmd;
        !reader->command().isEmpty() ? (d->msg += " " + reader->command() + "\"") : (d->msg += "\"");
        perr("%s", qstoc(d->msg));
        m_onAlreadyMonitoring(chat_id, reader->command(), host);
    }
    else if( reader && src == reader->source() ) { // same source but priority changed
        int history_idx = m_onPriorityChanged(user_id, chat_id, reader->command(),cmd,  priority,  reader->getAppliedHost());
        reader->setIndex(history_idx);
        reader->setPriority(priority);
        reader->setCommand(cmd);
    }
    else if(reader) {
        QString oldcmd = reader->command();
        reader->unsetSource();
        reader->setCommand(cmd);
        reader->setSource(src);
        m_onFormulaChanged(user_id, chat_id, reader->sources().join(","), oldcmd, reader->command(), reader->getAppliedHost());
    }
    else {
        int uid_readers_cnt = d->readersMap.values(chat_id).size();
        if(uid_readers_cnt < uid_monitor_limit) {
            bool monitor = true;
            BotReader *reader = new BotReader(user_id, chat_id, this,
                                              d->cu_pool, d->ctrl_factory_pool,
                                              d->ttl, d->poll_period, cmd, priority, host, monitor);
            connect(reader, SIGNAL(newData(int, const CuData&)), this, SLOT(m_onNewData(int, const CuData&)));
            connect(reader, SIGNAL(newDataIn(int, const CuData&)), this, SLOT(m_updateStats(int, const CuData&)));
            connect(reader, SIGNAL(formulaChanged(int, int, QString,QString, QString,QString)),
                    this, SLOT(m_onFormulaChanged(int, int, QString,QString, QString,QString)));
            connect(reader, SIGNAL(startSuccess(int, int, QString, QString,QString)),
                    this, SLOT(onSrcMonitorStarted(int, int, QString, QString,QString)));
            connect(reader, SIGNAL(lastUpdate(int, const CuData&)), this, SLOT(m_onLastUpdate(int, const CuData&)));
            connect(reader, SIGNAL(modeChanged(BotReader::RefreshMode)), this, SLOT(m_onReaderModeChanged(BotReader::RefreshMode)));
            reader->setSource(src);
            reader->setStartedOn(started_on);
            // reader index is set upon successful start and db insert
        }
        else {
            emit startError(chat_id, src, QString("limit of %1 monitors already reached").arg(uid_monitor_limit));
        }

        // will be inserted in map upon successful "property" delivery
    }
    return !d->err;
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
        emit stopped(reader->userId(), chat_id, src, reader->command(), host, msg);
    }
    // emit new data if it has a value
    else if(da.containsKey("value"))
        emit newMonitorData(chat_id, da, reader->index());
}

void BotMonitor::m_onFormulaChanged(int user_id, int chat_id, const QString &src, const QString &old, const QString &new_f, const QString& host)
{
    emit onFormulaChanged(user_id, chat_id, src, host, old, new_f);
}

// returns the index of the updated/newly inserted history entry
int BotMonitor::m_onPriorityChanged(int user_id,
                                    int chat_id,
                                    const QString &oldcmd,
                                    const QString &newcmd,
                                    BotReader::Priority newpri,
                                    const QString& host)
{
    qDebug() << __PRETTY_FUNCTION__ << "old type " << oldcmd << "new " << newcmd;
    getModuleListener()->onSendMessageRequest(chat_id, BotmonitorMsgFormatter().monitorTypeChanged(oldcmd, newcmd));
    getDb()->monitorStopped(user_id, oldcmd, host);
    HistoryEntry he(user_id, newcmd,  newpri == BotReader::Low ?  "monitor" : "alert", host, d->src_description);
    return getDb()->addToHistory(he, getBotConfig());
}

void BotMonitor::m_onAlreadyMonitoring(int chat_id, const QString &cmd, const QString &host)
{
    getModuleListener()->onSendMessageRequest(chat_id, BotmonitorMsgFormatter().alreadyMonitoring(cmd, host));
}

void BotMonitor::m_onLastUpdate(int chat_id, const CuData &)
{
    BotReader *reader = qobject_cast<BotReader *>(sender());
    emit stopped(reader->userId(), chat_id, reader->source(), reader->command(), reader->host(), "end of TTL");
    reader->deleteLater();

    QMutableMapIterator<int, BotReader *> it(d->readersMap); // multimap
    while(it.hasNext()) {
        it.next();
        if(it.value() == reader)
            it.remove();
    }
}

void BotMonitor::m_onReaderModeChanged(BotReader::RefreshMode rm)
{
    BotReader *r = qobject_cast<BotReader *>(sender());
    if(rm == BotReader::Polled || rm == BotReader::Event)
        MonitorHelper().adjustPollers(this, getBotConfig()->poll_period(), getBotConfig()->max_avg_poll_period());
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
void BotMonitor::onNewMonitorData(int chat_id, const CuData &da, int reader_idx)
{
    DataMsgFormatter mf;
    CuBotModuleListener *lis = getModuleListener();
    bool silent = getOption("sound").toString() == "off";
    bool edit = getOption("new_msg").toString() == "off";
    const QString m = mf.fromData_msg(da, DataMsgFormatter::FormatShort, d->src_description);
    if(edit) {
        lis->onEditMessageRequest(chat_id, reader_idx, m);
    }
    else
        lis->onSendMessageRequest(chat_id, m, silent);

}

void BotMonitor::m_updateStats(int chat_id, const CuData &da)
{
    getModuleListener()->onStatsUpdateRequest(chat_id, da);
}

void BotMonitor::onSrcMonitorStopped(int user_id, int chat_id, const QString &src,
                                     const QString &command,
                                     const QString &host, const QString &message)
{
    const bool silent = true;
    MonitorHelper mh;
    BotmonitorMsgFormatter monf;
    BotConfig *conf = getBotConfig();
    mh.adjustPollers(this, conf->poll_period(), conf->max_avg_poll_period());
    HistoryEntry he = getDb()->historyEntryFromCmd(user_id, command);
    getModuleListener()->onSendMessageRequest(chat_id,  monf.monitorStopped(he, message), silent);
    // update database, remove rows for chat_id and src
    getDb()->monitorStopped(user_id, command, host);
}

void BotMonitor::onSrcMonitorStarted(int user_id, int chat_id, const QString &src, const QString& formula, const QString& host)
{
    BotmonitorMsgFormatter monf;
    // when a new reader starts monitoring, the polling period of all readers must be adjusted
    // not to overload the poll operation. This is done every time the refresh mode of a reader
    // changes (signalled by BotReader::modeChanged to this slot m_onReaderModeChanged)
    // and every time a monitor is stopped (BotMonitor::onSrcMonitorStopped)
    // We can't do this here because at this stage we do not have the information about the mode yet.
    // See BotMonitor::m_onReaderModeChanged

    printf("\e[1;32m+++ onSrcMonitorStarted success %d %d  %s %s \e[0m\n", user_id, chat_id, qstoc(src), qstoc(formula));
    BotReader *r = qobject_cast<BotReader *>(sender());
    d->readersMap.insert(chat_id, r);

    const QDateTime until = QDateTime::currentDateTime().addSecs(getBotConfig()->ttl());
    BotReader::Priority pri = r->priority();
    getModuleListener()->onSendMessageRequest(chat_id, monf.monitorUntil(r->command(), until));
    // record new monitor into the database
    qDebug() << __PRETTY_FUNCTION__ << "adding history entry with formula " << formula << "host " << r->host() << "priority " << pri;
    HistoryEntry he(user_id, r->command(), pri == BotReader::High ? "alert" :  "monitor", host, d->src_description);
    int history_idx = getDb()->addToHistory(he, getBotConfig()); // returns the index of the history table
    // set the index of the reader according to the index in the database table
    // if database insert failed (history_idx < 0) delete the reader
    history_idx > 0 ? r->setIndex(history_idx) : r->deleteLater();
}

void BotMonitor::onSrcMonitorStartError(int chat_id, const QString &src, const QString &message)
{
    getModuleListener()->onSendMessageRequest(chat_id, BotmonitorMsgFormatter().srcMonitorStartError(src, message));
}

void BotMonitor::onSrcMonitorFormulaChanged(int user_id, int chat_id, const QString &new_s, const QString& host,
                                            const QString &old, const QString &new_f)
{
    getModuleListener()->onSendMessageRequest(chat_id, BotmonitorMsgFormatter().formulaChanged(new_s, old, new_f));
    BotReader *r = findReader(chat_id, new_s, host);
    // mark monitor with old formula stopped
    getDb()->monitorStopped(user_id, r->command(), r->host());
    printf("\e[1;33mADD TO HISTORY NEW ENTRY: %s\e[0m\n", qstoc(new_s));
    HistoryEntry he(user_id, new_f, r->priority() == BotReader::Low ? "monitor" : "alert", r->getAppliedHost(), d->src_description);
    r->setIndex(getDb()->addToHistory(he, getBotConfig()));
}

int BotMonitor::type() const
{
    return  BotMonitorType;
}

QString BotMonitor::name() const
{
    return "monitor";
}

QString BotMonitor::description() const
{
    return "Provides monitor and alert services for the cumbia-telegram bot";
}

QString BotMonitor::help() const
{
    return "monitor";
}

CuBotModule::AccessMode BotMonitor::needsDb() const
{
    return CuBotModule::ReadWrite;
}

CuBotModule::AccessMode BotMonitor::needsStats() const {
    return CuBotModule::ReadWrite;
}

int BotMonitor::decode(const TBotMsg &msg)
{
    BotMonitorMsgDecoder::Type t = d->mon_msg_decoder->decode(msg);
    if(t != BotMonitorMsgDecoder::Invalid)
        return type();
    return -1;
}

bool BotMonitor::process()
{
    bool success = false;
    BotMonitorMsgDecoder::Type t = d->mon_msg_decoder->type();
    const int & chat_id = d->mon_msg_decoder->chatId();
    const int &uid = d->mon_msg_decoder->userId();
    const QString& txt = d->mon_msg_decoder->text();
    CuBotModuleListener *lis = getModuleListener();
    if(t == BotMonitorMsgDecoder::Monitor || t == BotMonitorMsgDecoder::Alert) {
        QString src = d->mon_msg_decoder->source();
        QString host; // if m.hasHost use it, it comes from a fake history message created ad hoc by History
        d->mon_msg_decoder->hasHost() ? host = d->mon_msg_decoder->host()
                : host = ModuleUtils().getHost(d->mon_msg_decoder->chatId(), getDb(), d->ctrl_factory_pool); // may be empty. If so, TANGO_HOST will be used
        // authorization limits
        int uid_mon_limit = getDb()->isAuthorized(uid, name());
        success = startRequest(uid, chat_id,
                               uid_mon_limit != 0 ? uid_mon_limit : getBotConfig()->getDefaultAuth(name()),
                               src, txt,
                               t == BotMonitorMsgDecoder::Monitor ? BotReader::Low : BotReader::High,
                               host,
                               d->mon_msg_decoder->description(),
                               d->mon_msg_decoder->startDateTime());
        if(!success)
            lis->onSendMessageRequest(d->mon_msg_decoder->chatId(),
                                      GenMsgFormatter().error("CuBotServer", d->msg));
    }
    else if(t == BotMonitorMsgDecoder::StopMonitor && d->mon_msg_decoder->cmdLinkIdx() < 0) {
        QStringList srcs = d->formula_parser_helper->sources(d->mon_msg_decoder->source());
        printf("\e[1;32mBotMonitor::process: received StopMonitor cmd source is %s\e[0m\n", qstoc(srcs.join(", ")));
        printf("\e[1;32mBotMonitor::process: received StopMonitor stop pattern is %s\e[0m\n", qstoc(d->mon_msg_decoder->getArgs().join(", ")));
        success = stopAll(d->mon_msg_decoder->chatId(), srcs.isEmpty() ? d->mon_msg_decoder->getArgs() : srcs);
        if(!success) {
            lis->onSendMessageRequest(chat_id, GenMsgFormatter().error("CuBotServer", d->msg));
            // failure in this phase means reader is already stopped (not in monitor's map).
            // make sure it is deleted from the database too
            // ..
        }
    }
    else if(t == BotMonitorMsgDecoder::StopMonitor && d->mon_msg_decoder->cmdLinkIdx() > 0) {
        // stop by reader index!
        success = stopByIdx(chat_id, d->mon_msg_decoder->cmdLinkIdx());
        if(!success) {
            lis->onSendMessageRequest(chat_id, GenMsgFormatter().error("CuBotServer", d->msg));
        }
    }
    else if(t == BotMonitorMsgDecoder::Settings) {
        OptionDesc od = optionMatch(txt);
        success = od.isValid();
        if(!success)
            d->msg = "invalid key or value in " + txt + " or command not starting with \"/set_\"";
        if(success && (success = od.setCurrentValueFromCmd(txt))) {
            setOption(od.key, od.current_set);
        }
        if(!success)
            d->msg = "invalid value in command \"" + txt + "\"";
        success ? lis->onSendMessageRequest(chat_id, QString("successfully set <i>" + od.key + "</i> to <b>" + od.current_set + "</b>")) :
                  lis->onSendMessageRequest(chat_id, GenMsgFormatter().error("CuBotServer", d->msg));
    }
    return success;
}

QList<OptionDesc>  BotMonitor::getOptionsDesc() const
{
    OptionDesc sndopt("sound");
    sndopt.options["enable sound on notifications"] = "on";
    sndopt.options["disable sound on notifications (default)"] = "off";
    OptionDesc newmsgopt("new_msg");
    newmsgopt.options["updates always in new message"] = "on";
    newmsgopt.options["updates change current message when possible"] = "off";
    return QList<OptionDesc> () << sndopt << newmsgopt;
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

