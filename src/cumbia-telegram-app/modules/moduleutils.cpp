#include "moduleutils.h"
#include <cucontrolsfactorypool.h>
#include "../lib/botdb.h"

ModuleUtils::ModuleUtils()
{

}

QString ModuleUtils::getHost(int chat_id, BotDb* db, const CuControlsFactoryPool& ctrl_factory_pool, const QString &src)
{
    QString host;
    bool needs_host = true;
    if(!src.isEmpty()) {
        std::string domain = ctrl_factory_pool.guessDomainBySrc(src.toStdString());
        needs_host = (domain == "tango");
    }
    if(needs_host) {
        host = db->getSelectedHost(chat_id);
        if(host.isEmpty())
            host = QString(secure_getenv("TANGO_HOST"));
    }
    return host;
}
