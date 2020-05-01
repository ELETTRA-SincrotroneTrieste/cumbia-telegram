#include "botwriter_mod.h"
#include "cumbiasupervisor.h"
#include "botwriter.h"
#include "moduleutils.h"
#include <botconfig.h>
#include <qustring.h>
#include <botdb.h>
#include <QRegularExpression>
#include <QRegularExpressionMatch>
#include <generic_msgformatter.h>
#include <formulahelper.h>
#include <QtDebug>

class BotWriterModulePrivate {
public:
    bool err;
    QString msg, type_as_str;
    QString target;
    QString arg;
    QString host;
    TBotMsg tbotmsg;
    CumbiaSupervisor cu_s;
};

BotWriterMod::BotWriterMod(QObject*parent, const CumbiaSupervisor &cu_s,
                           CuBotModuleListener*lis, BotDb *db, BotConfig *conf) : QObject (parent)
{
    d = new BotWriterModulePrivate;
    d->cu_s = cu_s;
    init(lis, db, conf);
}

void BotWriterMod::init(CuBotModuleListener *listener, BotDb *db, BotConfig *bot_conf)
{
    setBotmoduleListener(listener);
    setDb(db);
    setConf(bot_conf);
    reset();
}

void BotWriterMod::onWriterData(const CuData &da) {
    bool ok = !da["err"].toBool();
    QString m = QuString(da["msg"].toString());
    QString msg;
    if(ok && d->arg.isEmpty())
        msg = QString("🕹 <i>%1</i> successfully imparted").arg(d->target);
    else if(ok)
        msg = QString("✍️  set <b>%1</b> on <i>%2</i>").arg(d->arg).arg(d->target);
    else if(!ok && d->arg.isEmpty())
        msg = QString("👎 command <i>%1</i> failed: %2").arg(d->target).arg(m);
    else
        msg = QString("👎 set <i>%1</i> on <i>%2</i> <b>failed</b>:\n<i>%3</i>").arg(d->arg).arg(d->target)
                .arg(m);
    if(da.containsKey("write_value")){
        msg += QString("\n<i>wrote</i>: <b>%1</b>").arg(QuString(da["write_value"]));
    }
    if(da.containsKey("value")) {
        msg += QString(" --> <i>out:</i> <b>%1</b>").arg(QuString(da["value"]));
    }
    msg += QString("\n[<i>%1</i>]").arg(qobject_cast<BotWriter *>(sender())->host());
    getModuleListener()->onSendMessageRequest(d->tbotmsg.chat_id, msg, false);

    sender()->deleteLater();
}

QString BotWriterMod::m_history_op_msg(const QDateTime &dt, const QString &name) const
{
    if(name.isEmpty())
        return "You haven't performed any operation yet!";
    QString msg = "<i>" + GenMsgFormatter().timeRepr(dt) + "</i>\n";
    msg += "operation: <b>" + FormulaHelper().escape(name) + "</b>";
    return msg;
}

BotWriterMod::~BotWriterMod()
{
    delete d;
}

void BotWriterMod::reset()
{
    d->err = false;
    d->tbotmsg = TBotMsg();
    d->msg.clear();
    d->arg.clear();
    d->target.clear();
}

int BotWriterMod::type() const {
    return WriterType;
}

QString BotWriterMod::name() const
{
    return "writer";
}

QString BotWriterMod::description() const {
    return "write values and impart command";
}

QString BotWriterMod::help() const
{
    return QString();
}

int BotWriterMod::decode(const TBotMsg &msg)
{
    printf("\e[1;32m--------------- BotWriterMod: decoding %s\e[0m\n", qstoc(msg.text()));
    reset();
    d->tbotmsg = msg;
    d->host = msg.host();
    // 1. set test/device/1->On  (command with no args)
    // 2. set 10.2 on test/device/1/double_scalar (write a value on sthg)
    QRegularExpression re("set(?:\\s+(.*)\\s+on){0,1}\\s+(.*)");
    QRegularExpressionMatch ma = re.match(msg.text());
    qDebug() << __PRETTY_FUNCTION__ << ma.hasMatch() << ma.capturedTexts();
    if(ma.hasMatch()) {
        if(ma.capturedTexts().size() == 3) {
            if(!ma.capturedTexts().at(1).isEmpty())
                d->arg = ma.capturedTexts().at(1);
            d->target = ma.capturedTexts().at(2);
        }
        return type();
    }
    return -1;
}

bool BotWriterMod::process()
{
    bool write_allowed = getDb()->canWrite(d->tbotmsg.user_id);
    if(write_allowed) {
        QString host;
        qDebug() << __PRETTY_FUNCTION__ << "d->host is " << d->host;
        d->host.length() > 0 ? host = d->host :
                host = ModuleUtils().getHost(d->tbotmsg.chat_id, getDb(), d->cu_s.ctrl_factory_pool);
        qDebug() << __PRETTY_FUNCTION__ << "host will be " << host;
        // may be empty. If so, TANGO_HOST will be used
        BotWriter *w  = new BotWriter(this, d->cu_s.cu_pool, d->cu_s.ctrl_factory_pool, host);
        if(!d->arg.isEmpty())
            w->setTarget(QString("%1(%2)").arg(d->target).arg(d->arg));
        else
            w->setTarget(d->target);
        connect(w, SIGNAL(newData(CuData)), this, SLOT(onWriterData(CuData)));
        w->write();
        return true;
    }
    return false;
}

bool BotWriterMod::error() const {
    return d->err;
}

QString BotWriterMod::message() const {
    return d->msg;
}



