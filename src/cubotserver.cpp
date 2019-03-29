#include "cubotserver.h"
#include "cubotlistener.h"
#include "cubotsender.h"
#include "tbotmsg.h"
#include "tbotmsgdecoder.h"
#include "botreader.h"
#include "cumbiasupervisor.h"
#include "generic_msgformatter.h"
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
#include "botreader_mod.h"
#include "host_mod.h"
#include "help_mod.h"
#include "cubotmsgtracker.h"

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
    CuBotMsgTracker *msg_tracker;
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
    d->msg_tracker = nullptr;
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
    /*
     * if(e->type() == EventTypes::ServerProcess) {
     * see comment in cubotserverevents.h
     */
    if(e->type() == EventTypes::type(EventTypes::ReinjectMsgRequest)) {
        CuBotServerReinjectMsgEvent *reinj_e = static_cast<CuBotServerReinjectMsgEvent *>(e);
        onMessageReceived(reinj_e->tbotmsg);
    }
    else if(e->type() == EventTypes::type(EventTypes::SendMsgRequest)) {
        CuBotServerSendMsgEvent *sendMsgE = static_cast<CuBotServerSendMsgEvent *>(e);
        d->bot_sender->sendMessage(sendMsgE->chat_id, sendMsgE->msg, sendMsgE->silent, sendMsgE->wait_for_reply, sendMsgE->key);
        sendMsgE->accept();
    }
    else if(e->type() == EventTypes::type(EventTypes::EditMsgRequest)) {
        CuBotServerEditMsgEvent *editMsgE = static_cast<CuBotServerEditMsgEvent *>(e);
        d->bot_sender->editMessage(editMsgE->chat_id, editMsgE->key, editMsgE->msg, editMsgE->message_id, editMsgE->wait_for_reply);
        editMsgE->accept();
    }
    else if(e->type() == EventTypes::type(EventTypes::SendPicRequest)) {
        CuBotServerSendPicEvent *sendPicE = static_cast<CuBotServerSendPicEvent *>(e);
        d->bot_sender->sendPic(sendPicE->chat_id, sendPicE->img_ba);
        sendPicE->accept();
    }
    else if(e->type() == EventTypes::type(EventTypes::AddStatsRequest)) {
        CuBotServerAddStatsEvent *ase = static_cast<CuBotServerAddStatsEvent *>(e);
        d->stats->add(ase->chat_id, ase->data);
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
            if(!d->auth->isAuthorized(m.user_id, module->name())) {
                d->bot_sender->sendMessage(m.chat_id, m_unauthorized_msg(m.username, module->name(),
                                                                         d->auth->reason()));
                fprintf(stderr, "\e[1;31;4mUNAUTH\e[0m: \e[1m%s\e[0m [uid %d] not authorized to exec \e[1;35m%s\e[0m: \e[3m\"%s\"\e[0m\n",
                        qstoc(m.username), m.user_id, qstoc(module->name()), qstoc(d->auth->reason()));
            }
            else {
                printf("CuBotServer::onMessageReceived: module %s [%d] decoded the message\n", qstoc(module->name()), t);
                module->process();
            }
            break;
        }
    }
}

void CuBotServer::onMessageSent(int chat_id, int message_id, int key)
{
    printf("\e[1;32mCuBotServer:onMessageSent: chat_id %d message_id %d key %d\e[0m\n",
           chat_id, message_id, key);
    key > -1 ? d->msg_tracker->addMsg(chat_id, message_id, key) : d->msg_tracker->addMsg(chat_id, message_id);
}

void CuBotServer::onReaderUpdate(int chat_id, const CuData &data)
{
    // moved to botreader_mod
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
            connect(d->bot_sender, SIGNAL(messageSent(int, int, int)), this, SLOT(onMessageSent(int,int,int)));
        }
        if(!d->auth)
            d->auth = new Auth(d->bot_db, d->botconf);

        d->control_server = new BotControlServer(this);
        connect(d->control_server, SIGNAL(newMessage(int, int, ControlMsg::Type, QString, QLocalSocket*)),
                this, SLOT(onNewControlServerData(int, int, ControlMsg::Type, QString, QLocalSocket*)));

        d->msg_tracker = new CuBotMsgTracker(4);

        if(!d->stats)
            d->stats = new BotStats(this);

        // load modules
        m_loadModules();
        m_loadPlugins();

        // after loading modules and plugins load help: needs the list of registered modules
        HelpMod *helpMod = new HelpMod(this);
        m_registerModule(helpMod);
        helpMod->setModuleList(d->modules_map.values());

        m_restoreProcs();

    }
}

void CuBotServer::stop()
{
    if(!d->bot_db)
        perr("CuBotServer.stop: already stopped\n");
    else {

        m_saveProcs();

        m_unloadAll();

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
        if(d->msg_tracker) {
            delete d->msg_tracker;
            d->msg_tracker = nullptr;
        }
    }
}

void CuBotServer::m_setupMonitor()
{
    if(!d->bot_mon) {
        qDebug() << __PRETTY_FUNCTION__ << "creating bot mon " << d->botconf->ttl() << d->botconf->poll_period();
        d->bot_mon = new BotMonitor(this, d->cu_supervisor, d->botconf->ttl(), d->botconf->poll_period());
        d->bot_mon->setDb(d->bot_db);
        d->bot_mon->setBotmoduleListener(this);
        d->bot_mon->setConf(d->botconf);
    }
}

void CuBotServer::m_loadModules()
{
    // alloc and register modules in modules_map
    m_setupMonitor();
    // register monitor
    m_registerModule(d->bot_mon);

    printf("\n[bot] \e[1;4mregistering modules\e[0m...\n");
    // alias
    AliasModule *alias_proc = new AliasModule(d->bot_db, d->botconf, this);
    // register alias proc
    m_registerModule(alias_proc);
    // register reader
    BotReaderModule *botRmod = new BotReaderModule(this, d->cu_supervisor, this, d->bot_db, d->botconf);
    m_registerModule(botRmod);
    // register host manager
    HostMod *hostMod = new HostMod(this, d->bot_db);
    m_registerModule(hostMod);

    //
    foreach(int k, d->modules_map.keys()) {
        QString desc = d->modules_map[k]->description();
        if(desc.size() > 68) {
            desc.truncate(65);
            desc += "...";
        }
        printf("[bot] \e[0;32m+\e[0m module \"\e[1;32m%20s\e[0m | type \e[1;32m%4d\e[0m | [%70s]\"\n",
               qstoc(d->modules_map[k]->name()), k, qstoc(desc));
    }
}

void CuBotServer::m_loadPlugins()
{
    CuPluginLoader pl;
    QRegExp plugin_name_re("cumbia-telegram-.+plugin.so");
    printf("\n[bot] \e[1;4mloading plugins\e[0m from \"%s\"", CUMBIA_TELEGRAM_PLUGIN_DIR);
    pl.getPluginPath().length() > 0 ? printf(" and \"%s\"...\n", qstoc(pl.getPluginPath())) : printf("...\n");

    bool register_success;
    QMap<int, QString> pluginsonames_map;
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
                register_success = m_registerModule(botplui);
                if(register_success)
                    pluginsonames_map[botplui->type()] = pa.section('/', -1);
                else
                    perr("CuBotServer: \e[1;31mfailed to register plugin \"\e[0;31m%s\e[1;31m\"\e[0m", qstoc(pa.section('/', -1)));
            }
        }
        else {
            perr("failed to load plugin loader under path %s: %s", qstoc(pa), qstoc(loader.errorString()));
        }
    }
    foreach(int key, d->modules_map.keys()) {
        CuBotModule *i = d->modules_map[key];
        if(i->isPlugin())
            printf("[bot] \e[1;32m+\e[0m plugin \"\e[1;32m%20s\e[0m | type \e[1;32m%4d\e[0m | [%70s] | \e[0;36m%50s\e[0m\n",
                   qstoc(i->name()), i->type(), qstoc(i->description()), qstoc(pluginsonames_map[key]));
    }
}

void CuBotServer::m_unloadAll()
{
    QString modtyp;
    printf("\n[bot] \e[1;4mloading modules and plugins\e[0m...\n");
    foreach(CuBotModule *mod, d->modules_map) {
        mod->isPlugin() ? modtyp = "plugin" : modtyp = "module";
        printf("[bot] \e[1;32m-\e[0m unloading %s \e[1;32m%s\e[0m\n", qstoc(modtyp), qstoc(mod->name()));
        delete mod;
    }
    d->modules_map.clear();
}

void CuBotServer::onNewControlServerData(int uid, int chat_id, ControlMsg::Type t, const QString &msg, QLocalSocket *so)
{
    qDebug() << __PRETTY_FUNCTION__ << uid << chat_id << t << msg;
    if(t == ControlMsg::Statistics) {
        QString stats = BotStatsFormatter().toJson(d->stats, d->bot_db, d->bot_mon);
        d->control_server->sendControlMessage(so, stats);
    }
    else if(chat_id > -1 && d->bot_sender) {
        d->bot_sender->sendMessage(chat_id, GenMsgFormatter().fromControlData(t, msg));
    }
    else if(uid > -1 && chat_id < 0) {
        foreach(int chat_id, d->bot_db->chatsForUser(uid)) {
            d->bot_sender->sendMessage(chat_id, GenMsgFormatter().fromControlData(t, msg));
        }
    }
}

bool CuBotServer::m_saveProcs()
{
    if(d->bot_mon) {
        foreach(BotReader *r, d->bot_mon->readers()) {
            HistoryEntry he(r->userId(), r->command(),
                            r->priority() == BotReader::High ? "alert" :  "monitor",
                            r->getAppliedHost(), QString()); // QString(): empty description
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
    QStringList restore_cmds;
    for(int i =0; i < hes.size() && !d->bot_db->error(); i++) {
        const HistoryEntry& he = hes[i];
        //        printf("restoring proc %s type %s host %s formula %s chat id %d\n", qstoc(he.name), qstoc(he.type), qstoc(he.host), qstoc(he.formula), he.chat_id);
        TBotMsg msg(he);
        onMessageReceived(msg);
        restore_cmds << he.command;
    }
    if(success)
        d->bot_db->clearProcTable();

    int fixed = d->bot_db->fixStaleItems(restore_cmds);
    if(fixed > 0)
        perr("CuBotServer: database fixes were needed and affected %d entries resulting still running while they are not", fixed);

    return success;
}

bool CuBotServer::m_broadcastShutdown()
{
//    printf("\n\e[1;35mCuBotServer.m_broadcastShutdown --- TEMPORARILY DISABLED ---- \e[0m\n\n");
        QList<int> chat_ids = d->bot_db->getChatsWithActiveMonitors();
        foreach(int id, chat_ids) // last param true: wait for reply
            d->bot_sender->sendMessage(id, GenMsgFormatter().botShutdown(), false, true);
    return true;
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

bool CuBotServer::m_registerModule(CuBotModule *mod)
{
    if(!d->modules_map.contains(mod->type())) {
        d->modules_map[mod->type()] = mod;
        return true;
    }
    perr("CuBotServer: cannot add module \"%s\" duplicate type key %d", qstoc(mod->name()), mod->type());
    return false;
}

void CuBotServer::onSendMessageRequest(int chat_id, const QString &msg, bool silent, bool wait_for_reply)
{
    CuBotServerSendMsgEvent *sendMsgEvent = new CuBotServerSendMsgEvent(chat_id, msg, silent, wait_for_reply, -1);
    qApp->postEvent(this, sendMsgEvent);
}

void CuBotServer::onEditMessageRequest(int chat_id, int key, const QString &msg, bool wait_for_reply)
{
    int message_id = d->msg_tracker->getMessageId(chat_id, key); // key is reader idx in database
    if(message_id < 0) {
        printf("CuBotServer::onEditMessageRequest: \e[1;31mfalling back to new message for chat_id %d key %d msg %s\e[0m\e[0m\n",
               chat_id, key, qstoc(msg));
        CuBotServerSendMsgEvent *sendMsgEvent = new CuBotServerSendMsgEvent(chat_id, msg, true, wait_for_reply, key);
        qApp->postEvent(this, sendMsgEvent);
    }
    else {
        printf("\e[1;33mCuBotServer.onEditMessageRequest: chat_id %d msg_id %d KEY %d\e[0m\n", chat_id, message_id, key);
        CuBotServerEditMsgEvent *editMsgE = new CuBotServerEditMsgEvent(chat_id, msg, key, message_id, wait_for_reply);
        qApp->postEvent(this, editMsgE);
    }
}

void CuBotServer::onStatsUpdateRequest(int chat_id, const CuData &data)
{
    qApp->postEvent(this, new CuBotServerAddStatsEvent(chat_id, data));
}

void CuBotServer::onSendPictureRequest(int chat_id, const QByteArray &pic_ba)
{
    qApp->postEvent(this, new CuBotServerSendPicEvent(chat_id, pic_ba));
}


void CuBotServer::onReinjectMessage(const TBotMsg &msg_mod)
{
    qApp->postEvent(this, new CuBotServerReinjectMsgEvent(msg_mod));
}

QString CuBotServer::m_unauthorized_msg(const QString &username, const QString& op_type, const QString &reason) const
{
    QString s = QString("❌   user <i>%1</i> (%2) <b>unauthorized</b>:\n<i>%3</i>")
            .arg(username).arg(op_type).arg(reason);

    return s;
}


