#ifndef MODULEUTILS_H
#define MODULEUTILS_H

#include <QString>
#include <cucontrolsfactorypool.h>
class CuControlsFactoryPool;
class BotDb;

class ModuleUtils
{
public:
    ModuleUtils();

    QString getHost(int chat_id, BotDb* Db,
                    const CuControlsFactoryPool& ctrl_factory_pool = CuControlsFactoryPool(),
                    const QString& src = QString());
};

#endif // MODULEUTILS_H
