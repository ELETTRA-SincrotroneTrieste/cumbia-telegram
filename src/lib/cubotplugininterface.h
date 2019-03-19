#ifndef CUBOTPLUGININTERFACE_H
#define CUBOTPLUGININTERFACE_H


#include <cubotmodule.h>


class CuBotPluginInterface : public CuBotModule
{
public:
    virtual ~CuBotPluginInterface() {}
};

#define CuBotPluginInterface_iid "eu.elettra.cumbia-telegram.CuBotPluginInterface"

Q_DECLARE_INTERFACE(CuBotPluginInterface, CuBotPluginInterface_iid)

#endif // CUBOTPLUGININTERFACE_H
