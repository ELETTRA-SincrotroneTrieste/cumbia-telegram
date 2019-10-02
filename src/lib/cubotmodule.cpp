#include "cubotmodule.h"

class CuBotModulePrivate {
public:
    BotDb *db;
    BotConfig *botconf;
    CuBotModuleListener *listener;
    QMap<QString, QVariant> options;
};

CuBotModule::CuBotModule()
{
    d = new CuBotModulePrivate;
    d->botconf = nullptr;
    d->db = nullptr;
    d->listener = nullptr;
}

CuBotModule::CuBotModule(CuBotModuleListener *lis, BotDb *db, BotConfig *conf)
{
    d = new CuBotModulePrivate;
    d->db = db;
    d->botconf = conf;
    d->listener = lis;
}

CuBotModule::~CuBotModule() {
    delete  d;
}

void CuBotModule::setBotmoduleListener(CuBotModuleListener *l)
{
    d->listener = l;
}

void CuBotModule::setDb(BotDb *db)
{
    d->db = db;
}

BotDb *CuBotModule::getDb() const
{
    return  d->db;
}

BotConfig *CuBotModule::getBotConfig() const
{
    return d->botconf;
}

QVariant CuBotModule::getOption(const QString &key) const
{
    return d->options[key];
}

void CuBotModule::setConf(BotConfig *conf)
{
    d->botconf = conf;
}

void CuBotModule::setOption(const QString &key, const QVariant &value)
{
    d->options.insert(key, value);
}

/** \brief returns a map containing the configuration description as key and the *option key* as value
 *
 * @return QMap key: the option description; value: the *option "key"* that can be used with setOption
 */
QList<OptionDesc> CuBotModule::getOptionsDesc() const
{
    return QList<OptionDesc>  ();
}

OptionDesc CuBotModule::optionMatch(const QString &txt) const {
    QList<OptionDesc> ods = getOptionsDesc();
    foreach (OptionDesc od, ods) {
        if(od.matches(txt))
            return od;
    }
    return OptionDesc();
}

CuBotModuleListener *CuBotModule::getModuleListener() const
{
    return d->listener;
}

bool CuBotModule::isPlugin() const {
    return false;
}
