#ifndef CUBOTMODULE_H
#define CUBOTMODULE_H

class BotDb;
class QString;
class BotStats;
class BotConfig;
class QSqlDatabase;

#include <tbotmsg.h>
#include <cubotvolatileoperation.h>
#include <QVariant>

class CuBotModuleListener {
public:
    virtual ~CuBotModuleListener() {}

    virtual void onSendMessageRequest(int chat_id,
                                      const QString& msg,
                                      bool silent = false,
                                      bool wait_for_reply = false) = 0;

    virtual void onReplaceVolatileOperationRequest(int chat_id, CuBotVolatileOperation *vo) = 0;

    virtual void onReinjectMessage(const TBotMsg& msg_mod) = 0;


};

class CuBotModule
{
public:

    enum AccessMode { None, Read, ReadWrite };

    virtual ~CuBotModule() { }

    virtual void setBotmoduleListener(CuBotModuleListener *l) = 0;

    virtual int type() const = 0;

    virtual QString name() const = 0;

    virtual QString description() const = 0;

    virtual QString help() const = 0;

    virtual AccessMode needsDb() const = 0;

    virtual AccessMode needsStats() const = 0;

    virtual void setDb(BotDb *db) = 0;

    virtual void setConf(BotConfig *conf) = 0;

    virtual void setOption(const QString& key, const QVariant& value) = 0;

    /** \brief returns the type of the module if msg has been successfully decoded
     *
     * @return an integer *corresponding to the same type of the module* as returned by
     * the type method, <= 0 otherwise
     */
    virtual int decode(const TBotMsg& msg) = 0;

    /**
     * @brief process process the received message (or messages)
     * @return true if process was successful, false otherwise
     *
     * The bot calls process on the module after *decode* has decoded the message and
     * returned a value matching the *type* of the module.
     *
     * The module must save internally its state (e.g. the message received and the
     * decoded information) so that process can operate on valid input data.
     *
     * Subsequent calls to decode may require a queue to store multiple decoded messages
     * that will be processed at once when this method is invoked by the bot.
     *
     */
    virtual bool process() = 0;

    /**
     * @brief error returns true if an error condition occurred
     * @return true if the last operation has encountered an error, false otherwise
     *
     * @see message
     */
    virtual bool error() const = 0;

    /**
     * @brief message returns a message associated to the last operation
     * @return a string with a message stored by the last operation
     *
     * @see error
     */
    virtual QString message() const = 0;
};

#endif // CUBOTMODULE_H
