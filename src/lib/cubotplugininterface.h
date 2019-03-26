#ifndef CUBOTPLUGININTERFACE_H
#define CUBOTPLUGININTERFACE_H


#include <cubotmodule.h>


class CuBotPluginInterface : public CuBotModule
{
public:
    virtual ~CuBotPluginInterface() {}

    virtual void init(CuBotModuleListener *listener, BotDb *db = nullptr, BotConfig *bot_conf = nullptr) = 0;

    virtual bool isPlugin() const {
        return true;
    }
};

#define CuBotPluginInterface_iid "eu.elettra.cumbia-telegram.CuBotPluginInterface"

Q_DECLARE_INTERFACE(CuBotPluginInterface, CuBotPluginInterface_iid)

#endif // CUBOTPLUGININTERFACE_H
