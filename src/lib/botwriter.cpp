#include "botwriter.h"
#include "cucontrolswriter_abs.h"
#include "cuformulaparsehelper.h"
#include <cumacros.h>
#include <cumbiapool.h>
#include <cudata.h>
#include <cucontrolsfactories_i.h>
#include <cucontrolsfactorypool.h>
#include <cucontext.h>
#include <cucontrolsutils.h>

/** @private */
class BotWriterPrivate
{
public:
    bool ok;
    CuContext *context;
    QString host;
    bool needs_host;
};

BotWriter::BotWriter(QObject *w, CumbiaPool *cumbia_pool, const CuControlsFactoryPool &fpool, const QString &host) :
    QObject(w), CuDataListener()
{
    m_init();
    d->context = new CuContext(cumbia_pool, fpool);
    const char* env_tg_host = nullptr;
    if(host.isEmpty() && (env_tg_host = secure_getenv("TANGO_HOST")) != nullptr) {
        d->host = QString(env_tg_host);
    }
    else
        d->host = host;
}

void BotWriter::m_init()
{
    d = new BotWriterPrivate;
    d->context = NULL;
    d->ok = false;
    d->needs_host = true;
}

BotWriter::~BotWriter()
{
    pdelete("~BotWriter %p", this);
    delete d->context;
    delete d;
}

QString BotWriter::target() const {
    CuControlsWriterA *w = d->context->getWriter();
    if(w != NULL)
        return w->target();
    return "";
}

CuContext *BotWriter::getContext() const
{
    return d->context;
}

QString BotWriter::host() const {
    return d->host;
}

bool BotWriter::needsHost() const
{

}

/**
 * @brief BotReader::getAppliedHost returns the host used by the reader to connect to the given source (e.g. Tango host)
 *        or an empty string if the engine does not need a host to connect to a source (e.g. epics)
 * @return the host used by the reader (TANGO_HOST) or an empty string (if the reader relies on EPICS sources)
 *
 * @see needsHost
 * @see host
 */
QString BotWriter::getAppliedHost() const
{
    if(d->needs_host)
        return d->host;
    return QString();
}

void BotWriter::setTarget(const QString &target)
{
    CuFormulaParseHelper fph(d->context->getControlsFactoryPool());
    bool needs_host = fph.needsHost(target);
    QString tgt;
    needs_host ? tgt = d->host + "/" + target : tgt = target;
    CuControlsWriterA* w = d->context->replace_writer(tgt.toStdString(), this);
    if(w)
        w->setTarget(tgt);
}
void BotWriter::onUpdate(const CuData &da)
{
    printf("\e[1;33mBotWriter.onUpdate %s\e[0m\n", da.toString().c_str());
    d->ok = !da["err"].toBool();

    if(!d->ok) {
        perr("BotWriter [%s]: error %s target: \"%s\" format %s (writable: %d)", qstoc(objectName()),
             da["src"].toString().c_str(), da["msg"].toString().c_str(),
                da["data_format_str"].toString().c_str(), da["writable"].toInt());   
    }
    if(!d->ok || da["is_result"].toBool())
        emit newData(da);
}

void BotWriter::write() {
    CuControlsUtils cu;
    CuVariant args = cu.getArgs(target(), this);
    CuControlsWriterA *w = d->context->getWriter();
    if(w) {
        w->setArgs(args);
        w->execute();
    }
}


