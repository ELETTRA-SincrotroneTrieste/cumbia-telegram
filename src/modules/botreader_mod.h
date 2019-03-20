#ifndef CUBOTREADERMODULE_H
#define CUBOTREADERMODULE_H

#include <cubotmodule.h>

class BotReaderModule : public CuBotModule
{
public:
    BotReaderModule();

    // CuBotModule interface
public:
    int type() const;
    QString name() const;
    QString description() const;
    QString help() const;
    bool isVolatileOperation() const;
    int decode(const TBotMsg &msg);
    bool process();
    bool error() const;
    QString message() const;
};

#endif // CUBOTREADERMODULE_H
