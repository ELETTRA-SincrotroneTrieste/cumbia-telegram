#include "cubotserver.h"
#include "cubotlistener.h"
#include "cubotsender.h"
#include "tbotmsg.h"
#include "tbotmsgdecoder.h"
#include "botreader.h"
#include "cumbiasupervisor.h"
#include "msgformatter.h"
#include "botmonitor_mod.h"
#include "botconfig.h"
#include "cubotvolatileoperations.h"
#include "botcontrolserver.h"
#include "auth.h"
#include "botstats.h"
#include "botplotgenerator.h"
#include "cuformulaparsehelper.h"
#include "modules/alias_mod.h"
#include "monitorhelper.h"

#include <cumacros.h>
#include <QtDebug>

#include <QJsonValue>

#include <cucontrolsfactorypool.h>
#include <cumbiapool.h>
#include <cumbiatango.h>
#include <cutango-world.h>
#include <cuthreadfactoryimpl.h>
#include <qthreadseventbridgefactory.h>
#include <cutcontrolsreader.h>
#include <cuformulaplugininterface.h>
#include <cutcontrolswriter.h>
#include <cupluginloader.h>
#include "cubotserverevents.h"

#ifdef QUMBIA_EPICS_CONTROLS
#include <cumbiaepics.h>
#include <cuepcontrolsreader.h>
#include <cuepcontrolswriter.h>
#include <cuepics-world.h>
#include <cuepreadoptions.h>
#endif

class CuBotServerPrivate {
public:
    CuBotListener *bot_listener;
    CuBotSender *bot_sender;
    BotDb *bot_db;
    BotMonitor *bot_mon;
    CumbiaSupervisor cu_supervisor;
    BotConfig *botconf;
    CuBotVolatileOperations *volatile_ops;
    Auth* auth;
    BotControlServer *control_server;
    BotStats *stats;
    QString bot_token, db_filenam;

    QMap<int, CuBotModule *> modules_map;
};

CuBotServer::CuBotServer(QObject *parent, const QString& bot_token, const QString &sqlite_db_filenam) : QObject(parent)
{
    d = new CuBotServerPrivate;
    d->bot_listener = nullptr;
    d->bot_sender = nullptr;
    d->bot_mon = nullptr;
    d->botconf = nullptr;
    d->bot_db = nullptr;
    d->volatile_ops = nullptr;
    d->auth = nullptr;
    d->control_server = nullptr;
    d->stats = nullptr;
    d->bot_token = bot_token;
    d->db_filenam = sqlite_db_filenam;
}

CuBotServer::~CuBotServer()
{
    predtmp("~CuBotServer %p", this);
    if(isRunning())
        stop();
    delete d;
}

bool CuBotServer::isRunning() const
{
    return d->bot_db != nullptr;
}

bool CuBotServer::event(QEvent *e)
{
    if(e->type() == EventTypes::ServerProcess) {
        CuBotServerProcessEvent *spe = static_cast<CuBotServerProcessEvent *>(e);
        CuBotModule *module = spe->module;
        int t = module->type();
        printf("b. CuBotServer::event: module %s [%d] will process the message...\t", qstoc(module->name()), t); fflush(stdout);

        bool success = module->process();
        if(!success) {
            perr("CuBotServer: module \"%s\" failed to process: %s", qstoc(module->name()),
                 qstoc(module->message()));
        }
        d->volatile_ops->consume(spe->chat_id, t);
        spe->accept();
        printf("\t[ module process ended with successfully ? %d ]\n", success);
        return true;
    }
    else if(e->type() == EventTypes::type(EventTypes::SendMsgRequest)) {
        CuBotServerSendMsgEvent *sendMsgE = static_cast<CuBotServerSendMsgEvent *>(e);
        printf("b. CuBotServer::event: will send a message with length %d to chat %d...\t",
               sendMsgE->msg.length(), sendMsgE->chat_id); fflush(stdout);
        d->bot_sender->sendMessage(sendMsgE->chat_id, sendMsgE->msg, sendMsgE->silent, sendMsgE->wait_for_reply);
        sendMsgE->accept();
        printf("\t[ msg sent ]\n");

    }
    return QObject::event(e);
}

void CuBotServer::onMessageReceived(const TBotMsg &m)
{
    bool success = true;
    m.print();
    int uid = m.user_id;

    // 1. see if user exists, otherwise add him
    if(!d->bot_db->userExists(uid)) {
        success = d->bot_db->addUser(uid, m.username, m.first_name, m.last_name);
        if(!success)
            perr("CuBotServer.onMessageReceived: error adding user with id %d: %s", uid, qstoc(d->bot_db->message()));
    }
    if(success && !d->bot_db->userInPrivateChat(uid, m.chat_id)) {
        success = d->bot_db->addUserInPrivateChat(uid, m.chat_id);
    }

    //
    foreach(int type, d->modules_map.keys()) {
        CuBotModule *module = d->modules_map[type];
        int t = module->decode(m);
        if(t > 0) {
            printf("a. CuBotServer::onMessageReceived: module %s [%d] decoded the message\n", qstoc(module->name()), t);
            qApp->postEvent(this, new CuBotServerProcessEvent(module, m.chat_id));
            break;
        }
    }
    printf("   CuBotServer::onMessageReceived: waiting for new message...\n");

    /*
    TBotMsgDecoder msg_dec(m, normalizedFormulaPattern);
    //    printf("type of message is %s [%d]\n", msg_dec.types[msg_dec.type()], msg_dec.type());
    TBotMsgDecoder::Type t = msg_dec.type();
    if(!d->auth->isAuthorized(m.user_id, t)) {
        d->bot_sender->sendMessage(m.chat_id, MsgFormatter().unauthorized(m.username, msg_dec.types[t],
                                                                          d->auth->reason()));
        fprintf(stderr, "\e[1;31;4mUNAUTH\e[0m: \e[1m%s\e[0m [uid %d] not authorized to exec \e[1;35m%s\e[0m: \e[3m\"%s\"\e[0m\n",
                qstoc(m.username), m.user_id, msg_dec.types[t], qstoc(d->auth->reason()));
    }
    else {
        //  user is authorized to perform operation type t
        //
        if(t == TBotMsgDecoder::Host) {
            QString host = msg_dec.host();
            QString new_host_description;
            success = d->bot_db->setHost(m.user_id, m.chat_id, host, new_host_description);
            if(success)
                success = d->bot_db->addToHistory(HistoryEntry(m.user_id, m.text(), "host", "")); // "host" is type

            d->bot_sender->sendMessage(m.chat_id, MsgFormatter().hostChanged(host, success, new_host_description), true); // silent
            if(!success)
                perr("CuBotServer::onMessageReceived: database error: %s", qstoc(d->bot_db->message()));
        }
        else if(t == TBotMsgDecoder::QueryHost) {
            QString host = m_getHost(m.chat_id);
            d->bot_sender->sendMessage(m.chat_id, MsgFormatter().host(host));
        }
        else if(t == TBotMsgDecoder::Last) {
            QDateTime dt;
            HistoryEntry he = d->bot_db->lastOperation(m.user_id);
            if(he.isValid()) {
                TBotMsg lm = m;
                lm.setHost(he.host);
                lm.setText(he.toCommand());
                d->bot_sender->sendMessage(m.chat_id, MsgFormatter().lastOperation(he.datetime, lm.text()));
                //
                // call ourselves with the updated copy of the received message
                //
                onMessageReceived(lm);
            }
        }
        else if(t == TBotMsgDecoder::Read) {
            QString src = msg_dec.source();
            QString host; // if m.hasHost then m comes from a fake message created ad hoc by Last: use this host
            m.hasHost() ? host = m.host() : host = m_getHost(m.chat_id); // may be empty. If so, TANGO_HOST will be used
            // inject host into src using CuFormulaParserHelper
            //    src = CuFormulaParseHelper().injectHost(host, src);
            BotReader *r = new BotReader(m.user_id, m.chat_id, this, d->cu_supervisor.cu_pool,
                                         d->cu_supervisor.ctrl_factory_pool, d->botconf->ttl(),
                                         d->botconf->poll_period(), msg_dec.text(), BotReader::High, host);
            connect(r, SIGNAL(newData(int, const CuData&)), this, SLOT(onReaderUpdate(int, const CuData& )));
            r->setPropertiesOnly(true); // only configure! no reads!
            r->setSource(src); // insert in  history db only upon successful connection
        }

        else if(t == TBotMsgDecoder::ReadHistory || t == TBotMsgDecoder::MonitorHistory ||
                t == TBotMsgDecoder::AlertHistory || t == TBotMsgDecoder::Bookmarks) {
            QList<HistoryEntry> hel = m_prepareHistory(m.user_id, t);
            d->bot_sender->sendMessage(m.chat_id, MsgFormatter().history(hel, d->botconf->ttl(), msg_dec.toHistoryTableType(t)));
        }
        else if(t == TBotMsgDecoder::AddBookmark) {
            HistoryEntry he = d->bot_db->bookmarkLast(m.user_id);
            d->bot_sender->sendMessage(m.chat_id, MsgFormatter().bookmarkAdded(he));
        }
        else if(t == TBotMsgDecoder::DelBookmark) {
            int cmd_idx = msg_dec.cmdLinkIdx();
            if(cmd_idx > 0 && (success = d->bot_db->removeBookmark(m.user_id, cmd_idx))) {
                d->bot_sender->sendMessage(m.chat_id, MsgFormatter().bookmarkRemoved(success));
            }
        }
        else if(t == TBotMsgDecoder::Search) { // moved to plugin }
        else if(t == TBotMsgDecoder::AttSearch) {  // moved to plugin  }
        else if(t == TBotMsgDecoder::ReadFromAttList) {  // moved to plugin }
        else if(t == TBotMsgDecoder::CmdLink) {
            int cmd_idx = msg_dec.cmdLinkIdx();
            if(cmd_idx > -1) {
                QDateTime dt;
                QString operation;
                HistoryEntry he = d->bot_db->commandFromIndex(m.user_id, m.text(), cmd_idx);
                if(he.isValid()) {
                    operation = he.toCommand();
                    // 1. remind the user what was the command linked to /commandN
                    d->bot_sender->sendMessage(m.chat_id, MsgFormatter().lastOperation(dt, operation));
                    // 2.
                    // call ourselves with the updated copy of the received message
                    //
                    TBotMsg lnkm = m;
                    lnkm.setText(operation);
                    lnkm.setHost(he.host);
                    onMessageReceived(lnkm);
                }
            }
        }
        else if(t >= TBotMsgDecoder::Help && t <= TBotMsgDecoder::HelpSearch) {
            d->bot_sender->sendMessage(m.chat_id,
                                       MsgFormatter().help(t));
        }
        else if(t == TBotMsgDecoder::Start) {
            d->bot_sender->sendMessage(m.chat_id, MsgFormatter().help(TBotMsgDecoder::Help));
        }
        else if(t == TBotMsgDecoder::Plot) {
            printf("PLOT\n");
            BotPlotGenerator *plotgen =  static_cast<BotPlotGenerator *> (d->volatile_ops->get(m.chat_id, BotPlotGenerator::PlotGen));
            if(plotgen) {
                d->bot_sender->sendPic(m.chat_id, plotgen->generate());
            }
        }
        else if(t == TBotMsgDecoder::SetAlias) {
            // MOVED TO AliasProc
        }
        else if(t == TBotMsgDecoder::ShowAlias) {
            // moved to AliasProc
        }
        else if(t == TBotMsgDecoder::ShowAlias) {
            // moved to AliasProc
        }
        else if(t == TBotMsgDecoder::Invalid || t == TBotMsgDecoder::Error) {
            d->bot_sender->sendMessage(m.chat_id, MsgFormatter().error("TBotMsgDecoder", msg_dec.message()));
        }

        d->volatile_ops->consume(m.chat_id, msg_dec.type());
    } // else user is authorized

    */
}

void CuBotServer::onReaderUpdate(int chat_id, const CuData &data)
{
    MsgFormatter mf;
    bool err = data["err"].toBool();
    d->stats->addRead(chat_id, data); // data is passed for error stats
    d->bot_sender->sendMessage(chat_id, mf.fromData(data));
    if(!err && m_isBigSizeVector(data)) {
        BotPlotGenerator *plotgen = new BotPlotGenerator(chat_id, data);
        d->volatile_ops->replaceOperation(chat_id, plotgen);
    }
    if(!err) {
        BotReader *reader = qobject_cast<BotReader *>(sender());
        HistoryEntry he(reader->userId(), reader->command(), "read", reader->host());
        d->bot_db->addToHistory(he);
    }
}

void CuBotServer::start()
{
    if(d->bot_db)
        perr("CuBotServer.start: already started\n");
    else {
        d->cu_supervisor.setup();
        d->bot_db = new BotDb(d->db_filenam);
        d->botconf = new BotConfig(d->bot_db);
        if(d->bot_db->error())
            perr("CuBotServer.start: error opening QSQLITE telegram bot db: %s", qstoc(d->bot_db->message()));

        if(!d->bot_listener) {
            d->bot_listener = new CuBotListener(this, d->bot_token,
                                                d->botconf->getBotListenerMsgPollMillis(),
                                                d->botconf->getBotListenerOldMsgDiscardSecs());
            connect(d->bot_listener, SIGNAL(onNewMessage(const TBotMsg &)),
                    this, SLOT(onMessageReceived(const TBotMsg&)));
            d->bot_listener->start();
        }
        if(!d->bot_sender) {
            d->bot_sender = new CuBotSender(this, d->bot_token);
        }
        if(!d->volatile_ops)
            d->volatile_ops = new CuBotVolatileOperations();

        if(!d->auth)
            d->auth = new Auth(d->bot_db, d->botconf);

        d->control_server = new BotControlServer(this);
        connect(d->control_server, SIGNAL(newMessage(int, int, ControlMsg::Type, QString, QLocalSocket*)),
                this, SLOT(onNewControlServerData(int, int, ControlMsg::Type, QString, QLocalSocket*)));

        if(!d->stats)
            d->stats = new BotStats(this);

        // load modules
        m_loadModules();
        m_loadPlugins();

        m_restoreProcs();

    }
}

void CuBotServer::stop()
{
    if(!d->bot_db)
        perr("CuBotServer.stop: already stopped\n");
    else {

        m_saveProcs();

        // broadcast a message to users with active monitors
        m_broadcastShutdown();

        if(d->bot_listener) {
            d->bot_listener->stop();
            delete d->bot_listener;
            d->bot_listener = nullptr;
        }

        if(d->bot_db) {
            delete d->bot_db;
            d->bot_db = nullptr;
        }
        if(d->bot_mon) {

        }
        d->cu_supervisor.dispose();

        if(d->volatile_ops) {
            delete d->volatile_ops;
            d->volatile_ops = nullptr;
        }
        if(d->auth){
            delete d->auth;
            d->auth = nullptr;
        }
        if(d->control_server)
            delete d->control_server;

        if(d->stats) {
            delete d->stats;
            d->stats = nullptr;
        }
    }
}

void CuBotServer::m_loadModules()
{
    // alloc and register modules in modules_map
    m_setupMonitor();

    printf("\n[bot] \e[1;4mregistering modules\e[0m...\n");
    // alias
    AliasModule *alias_proc = new AliasModule(d->bot_db, d->botconf, this);
    // register alias proc
    d->modules_map[alias_proc->type()] = alias_proc;

    // register monitor
    d->modules_map[d->bot_mon->type()] = d->bot_mon;
    //
    foreach(int k, d->modules_map.keys()) {
        QString desc = d->modules_map[k]->description();
        if(desc.size() > 68) {
            desc.truncate(65);
            desc += "...";
        }
        printf("[bot] \e[0;32m+\e[0m module \"\e[1;32m%15s\e[0m | type \e[1;32m%4d\e[0m | [%70s]\"\n",
               qstoc(d->modules_map[k]->name()), k, qstoc(desc));
    }
}

void CuBotServer::m_loadPlugins()
{
    CuPluginLoader pl;
    QRegExp plugin_name_re("cumbia-telegram-.+plugin.so");
    printf("\n[bot] \e[1;4mloading plugins\e[0m from \"%s\"", CUMBIA_TELEGRAM_PLUGIN_DIR);
    pl.getPluginPath().length() > 0 ? printf(" and \"%s\"...\n", qstoc(pl.getPluginPath())) : printf("...\n");

    QStringList paths = pl.getPluginAbsoluteFilePaths(CUMBIA_TELEGRAM_PLUGIN_DIR, plugin_name_re);
    foreach(QString pa, paths) {
        QPluginLoader loader(pa);
        QObject  *plugin = (loader.instance());
        if (plugin){
            CuBotPluginInterface *botplui = qobject_cast<CuBotPluginInterface *>(plugin);
            if(!botplui)
                perr("Failed to load telegram-bot plugin \"%s\"", qstoc(pa));
            else {
                botplui->init(this, d->bot_db, d->botconf);
                d->modules_map.insert(botplui->type(), botplui);
                printf("[bot] \e[1;32m+\e[0m plugin \"\e[1;32m%15s\e[0m | type \e[1;32m%4d\e[0m | [%70s] | \e[0;34m%40s\e[0m\"\n",
                        qstoc(d->modules_map[botplui->type()]->name()), botplui->type(), qstoc(botplui->description()), qstoc(pa.section('/', -1)));
            }
        }
        else {
            perr("failed to load plugin loader under path %s: %s", qstoc(pa), qstoc(loader.errorString()));
        }
    }
}

void CuBotServer::onVolatileOperationExpired(int chat_id, const QString &opnam, const QString &text)
{
    d->bot_sender->sendMessage(chat_id, MsgFormatter().volatileOpExpired(opnam, text));
}

void CuBotServer::onNewControlServerData(int uid, int chat_id, ControlMsg::Type t, const QString &msg, QLocalSocket *so)
{
    qDebug() << __PRETTY_FUNCTION__ << uid << chat_id << t << msg;
    if(t == ControlMsg::Statistics) {
        QString stats = BotStatsFormatter().toJson(d->stats, d->bot_db, d->bot_mon);
        d->control_server->sendControlMessage(so, stats);
    }
    else if(chat_id > -1 && d->bot_sender) {
        d->bot_sender->sendMessage(chat_id, MsgFormatter().fromControlData(t, msg));
    }
    else if(uid > -1 && chat_id < 0) {
        foreach(int chat_id, d->bot_db->chatsForUser(uid)) {
            d->bot_sender->sendMessage(chat_id, MsgFormatter().fromControlData(t, msg));
        }
    }
}

void CuBotServer::m_onReaderRefreshModeChanged(int user_id, int chat_id,
                                               const QString &src, const QString &host,
                                               BotReader::RefreshMode rm)
{
    Q_UNUSED(user_id); Q_UNUSED(chat_id); Q_UNUSED(src); Q_UNUSED(host);
    if(rm == BotReader::Polled || rm == BotReader::Event)
        MonitorHelper().adjustPollers(d->bot_mon, d->botconf->poll_period(), d->botconf->max_avg_poll_period());
}

void CuBotServer::m_setupMonitor()
{
    if(!d->bot_mon) {
        qDebug() << __PRETTY_FUNCTION__ << "creating bot mon " << d->botconf->ttl() << d->botconf->poll_period();
        d->bot_mon = new BotMonitor(this, d->cu_supervisor, d->botconf->ttl(), d->botconf->poll_period());
        d->bot_mon->setDb(d->bot_db);
        d->bot_mon->setBotmoduleListener(this);
        d->bot_mon->setConf(d->botconf);
        d->bot_mon->setStats(d->stats);
    }
}

bool CuBotServer::m_saveProcs()
{
    if(d->bot_mon) {
        foreach(BotReader *r, d->bot_mon->readers()) {
            HistoryEntry he(r->userId(), r->command(),
                            r->priority() == BotReader::High ? "alert" :  "monitor",
                            r->host());
            he.chat_id = r->chatId(); // chat_id is needed to restore process at restart
            he.datetime = r->startedOn();
            d->bot_db->saveProc(he);
        }
    }
    return true;
}

bool CuBotServer::m_restoreProcs()
{
    bool success = true;
    QList<HistoryEntry> hes = d->bot_db->loadProcs();
    m_removeExpiredProcs(hes);
    if(d->bot_db->error())
        perr("CuBotServer:m_restoreProcs: database error: \"%s\"", qstoc(d->bot_db->message()));
    for(int i =0; i < hes.size() && !d->bot_db->error(); i++) {
        const HistoryEntry& he = hes[i];
        //        printf("restoring proc %s type %s host %s formula %s chat id %d\n", qstoc(he.name), qstoc(he.type), qstoc(he.host), qstoc(he.formula), he.chat_id);
        TBotMsg msg(he);
        onMessageReceived(msg);
    }
    if(success)
        d->bot_db->clearProcTable();
    return success;
}

bool CuBotServer::m_broadcastShutdown()
{
    QList<int> chat_ids = d->bot_db->getChatsWithActiveMonitors();
    foreach(int id, chat_ids) // last param true: wait for reply
        d->bot_sender->sendMessage(id, MsgFormatter().botShutdown(), false, true);
    return true;
}

/**
 * @brief CuBotServer::m_prepareHistory modify the list of HistoryEntry so that it can be pretty
 *        printed by MsgFormatter before sending.
 * @param in the list of history entries as returned by the database operations table
 * @return the list of history entries with some fields modified according to the state of some entries
 *         (running, not running, and so on)
 *
 * The purpose is to provide an interactive list where stopped monitors and alerts can be easily restarted
 * through a link, while running ones are just listed
 *
 */
QList<HistoryEntry> CuBotServer::m_prepareHistory(int uid, TBotMsgDecoder::Type t)
{
    QString type = TBotMsgDecoder().toHistoryTableType(t);
    QList<HistoryEntry> out = d->bot_db->history(uid, type);
    if(!d->bot_mon && out.size() > 0)
        m_setupMonitor();
    for(int i = 0; i < out.size(); i++) {
        BotReader *r = nullptr;
        HistoryEntry &he = out[i];
        if(t == TBotMsgDecoder::MonitorHistory  || t == TBotMsgDecoder::AlertHistory) {
            r = d->bot_mon->findReaderByUid(he.user_id, he.command, he.host);
            he.is_active = (r != nullptr && he.command == r->command());
            if(r)
                he.index = r->index();
        }
    }
    return out;
}

void CuBotServer::m_removeExpiredProcs(QList<HistoryEntry> &in)
{
    d->botconf->ttl();
    QDateTime now = QDateTime::currentDateTime();
    QList<HistoryEntry >::iterator it = in.begin();
    while(it != in.end()) {
        if((*it).datetime.secsTo(now) >= d->botconf->ttl())
            it = in.erase(it);
        else
            ++it;
    }
}

bool CuBotServer::m_isBigSizeVector(const CuData &da) const
{
    if(da.containsKey("value") && da.containsKey("data_format_str")
            && da["data_format_str"].toString() == std::string("vector")) {
        const CuVariant& val = da["value"];
        return val.getSize() > 5;
    }
    return false;
}

void CuBotServer::onSendMessageRequest(int chat_id, const QString &msg, bool silent, bool wait_for_reply)
{
    CuBotServerSendMsgEvent *sendMsgEvent = new CuBotServerSendMsgEvent(chat_id, msg, silent, wait_for_reply);
    qApp->postEvent(this, sendMsgEvent);
}

void CuBotServer::onReplaceVolatileOperationRequest(int chat_id, CuBotVolatileOperation *vo)
{
    d->volatile_ops->replaceOperation(chat_id, vo);
}

void CuBotServer::onAddVolatileOperationRequest(int chat_id, CuBotVolatileOperation *vo)
{
    d->volatile_ops->addOperation(chat_id, vo);
}

void CuBotServer::onStatsUpdateRequest(int chat_id, const CuData &data)
{
    d->stats->addRead(chat_id, data);
}


void CuBotServer::onReinjectMessage(const TBotMsg &msg_mod)
{
    onMessageReceived(msg_mod);
}
