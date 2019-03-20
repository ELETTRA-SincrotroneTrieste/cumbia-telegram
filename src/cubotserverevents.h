#ifndef CUBOTSERVEREVENTS_H
#define CUBOTSERVEREVENTS_H

#include <QEvent>
#include <QString>

class CuBotModule;

class EventTypes {
public:
    enum ETypes { ServerProcess = QEvent::User + 10, SendMsgRequest, AddVolatileOp, ReplaceVolatileOp };

    static QEvent::Type type(ETypes t) {
        return static_cast<QEvent::Type>(t);
    }
};

class CuBotServerSendMsgEvent : public QEvent {
public:
    CuBotServerSendMsgEvent(int cha_id, const QString& mess, bool _silent, bool _wait_for_reply);

    ~CuBotServerSendMsgEvent();

    bool wait_for_reply, silent;
    QString msg;
    int chat_id;
};

class CuBotServerProcessEvent : public QEvent
{
public:
    CuBotServerProcessEvent(CuBotModule *mod, int chat_i);
    ~CuBotServerProcessEvent();
    int chat_id;
    CuBotModule *module;
};

#endif // CUBOTSERVEREVENTS_H
