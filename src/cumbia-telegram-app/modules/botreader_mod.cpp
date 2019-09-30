#include "botreader_mod.h"

#include "../lib/botreader.h"
#include "../lib/botconfig.h"
#include "../lib/datamsgformatter.h"
#include "../lib/formulahelper.h"
#include "../lib/botplot.h"
#include "../lib/botplotgenerator.h"
#include "../lib/historyentry.h"
#include "../lib/cubotvolatileoperations.h"
#include "../lib/botdb.h"

#include "moduleutils.h"
#include <QRegularExpression>
#include <QRegularExpressionMatch>
#include "lib/cuformulaparsehelper.h"
#include "lib/cumbiasupervisor.h"
#include <cuformulaplugininterface.h>
#include <cudataquality.h>

class BotReaderModulePrivate {
public:
    QString msg, host, source, msg_text;
    QStringList detected_sources;
    BotReaderModule::State state;
    int user_id, chat_id;
    bool err;
    CuBotVolatileOperations volatile_ops;
    CumbiaSupervisor cu_s;
    CuFormulaParseHelper *formula_parser_helper;
};

BotReaderModule::BotReaderModule(QObject*parent, const CumbiaSupervisor &cu_s, CuBotModuleListener *lis, BotDb *db, BotConfig *conf) : QObject(parent)
{
    d = new BotReaderModulePrivate;
    d->cu_s = cu_s;
    d->formula_parser_helper = new CuFormulaParseHelper(cu_s.ctrl_factory_pool);
    setDb(db);
    setConf(conf);
    setBotmoduleListener(lis);
    reset();
}

BotReaderModule::~BotReaderModule()
{
    pdelete("\e[1;31m~BotReaderModule %p\e[0m\n", this);
    delete d->formula_parser_helper;
    delete d;
}

void BotReaderModule::reset() {
    d->state = Undefined;
    d->err = false;
    d->detected_sources.clear();
    d->host = d->msg = d->source = d->msg_text = QString();
    d->user_id = d->chat_id = -1;
}

void BotReaderModule::onReaderUpdate(int chat_id, const CuData &data)
{
    bool err = data["err"].toBool();
    DataMsgFormatter mf;
    CuBotModuleListener *mlis = getModuleListener();
    mlis->onSendMessageRequest(chat_id, mf.fromData_msg(data, DataMsgFormatter::FormatShort, QString()));
    if(!err && m_isBigSizeVector(data)) {
        BotPlotGenerator *plotgen = new BotPlotGenerator(chat_id, data);
        d->volatile_ops.replaceOperation(chat_id, plotgen);
    }
    if(!err) {
        BotReader *reader = qobject_cast<BotReader *>(sender());
        // last QString in HistoryEntry constructor is for description
        HistoryEntry he(reader->userId(), reader->command(), "read", reader->getAppliedHost(), QString());
        getDb()->addToHistory(he, getBotConfig());
    }
}

void BotReaderModule::m_updateStats(int chat_id, const CuData &dat)
{
    getModuleListener()->onStatsUpdateRequest(chat_id, dat);
}

QString BotReaderModule::m_plotUnavailable() const
{
    return "👎   /plot command  must be imparted right after a vector read operation";
}

int BotReaderModule::type() const {
    return ReaderType;
}

QString BotReaderModule::name() const
{
    return "reader";
}

QString BotReaderModule::description() const
{
    return "performs single shot readings";
}

QString BotReaderModule::help() const
{
    return QString();
}

int BotReaderModule::decode(const TBotMsg &msg)
{
    reset();
    d->msg_text = msg.text().trimmed();
    d->msg_text.replace(QRegularExpression("\\s+"), " ");
    d->host = msg.host();
    d->chat_id = msg.chat_id;
    d->user_id = msg.user_id;
    if(d->msg_text == "plot" || d->msg_text == "/plot")
        d->state = PlotRequest;
    else if(m_tryDecodeFormula(d->msg_text))
        d->state = SourceDetected;
    if(d->state != Undefined)
        return type();
    return -1;
}

bool BotReaderModule::process()
{
    if(d->state == SourceDetected) {
        QString src = d->source;
        QString host; // if m.hasHost then m comes from a fake message created ad hoc by Last: use this host
        d->host.length() > 0 ? host = d->host :
                host = ModuleUtils().getHost(d->chat_id, getDb(), d->cu_s.ctrl_factory_pool); // may be empty. If so, TANGO_HOST will be used
        // inject host into src using CuFormulaParserHelper
        //    src = CuFormulaParseHelper().injectHost(host, src);
        BotConfig *bcfg = getBotConfig();
        BotReader *r = new BotReader(d->user_id, d->chat_id, this, d->cu_s.cu_pool,
                                     d->cu_s.ctrl_factory_pool, bcfg->ttl(),
                                     bcfg->poll_period(), d->msg_text, BotReader::High, host);
        connect(r, SIGNAL(newDataIn(int, const CuData&)), this, SLOT(m_updateStats(int, const CuData& )));
        connect(r, SIGNAL(newData(int, const CuData&)), this, SLOT(onReaderUpdate(int, const CuData& )));
        r->setPropertiesOnly(true); // only configure! no reads!
        r->setSource(src); // insert in  history db only upon successful connection
        d->volatile_ops.consume(d->chat_id, BotPlotGenerator::PlotGen);
    }
    else if(d->state == PlotRequest) {
        BotPlotGenerator *plotgen =  static_cast<BotPlotGenerator *> (d->volatile_ops.get(d->chat_id, BotPlotGenerator::PlotGen));
        if(plotgen) {
            getModuleListener()->onSendPictureRequest(d->chat_id, plotgen->generate());
        }
        else {
            getModuleListener()->onSendMessageRequest(d->chat_id, m_plotUnavailable(), true);
        }
    }
    return true;
}

bool BotReaderModule::error() const
{
    return d->err;
}

QString BotReaderModule::message() const
{
    return  d->msg;
}

bool BotReaderModule::m_tryDecodeFormula(const QString &text)
{
    bool is_formula = true;
    // text does not start with either monitor or alarm
    d->source = QString();

    QString norm_fpattern = d->cu_s.formulaPlugin()->getFormulaParserInstance()->normalizedFormulaPattern();
    !d->formula_parser_helper->isNormalizedForm(text, norm_fpattern) ? d->source = d->formula_parser_helper->toNormalizedForm(text) : d->source = text;
    d->detected_sources = d->formula_parser_helper->sources(d->source);
    return is_formula;
}

bool BotReaderModule::m_isBigSizeVector(const CuData &da) const
{
    if(da.containsKey("value") && da.containsKey("data_format_str")
            && da["data_format_str"].toString() == std::string("vector")) {
        const CuVariant& val = da["value"];
        return val.getSize() > 5;
    }
    return false;
}
