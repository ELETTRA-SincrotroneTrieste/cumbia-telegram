#ifndef CUBOTSERVEREVENTS_H
#define CUBOTSERVEREVENTS_H

#include <QEvent>
#include <QString>

#include <cudata.h>
#include "lib/tbotmsg.h"

class CuBotModule;
class CuBotVolatileOperation;

class EventTypes {
public:
    enum ETypes { /* ServerProcess = QEvent::User + 10, */ SendMsgRequest = QEvent::User + 12, EditMsgRequest,
        SendPicRequest,
                  ReinjectMsgRequest, AddStatsRequest };

    static QEvent::Type type(ETypes t) {
        return static_cast<QEvent::Type>(t);
    }
};

class CuBotServerSendMsgEvent : public QEvent {
public:
    CuBotServerSendMsgEvent(int cha_id, const QString& mess, bool _silent, bool _wait_for_reply, int akey);
    bool wait_for_reply, silent;
    QString msg;
    int chat_id, key;
};

class CuBotServerEditMsgEvent : public QEvent {
public:
    CuBotServerEditMsgEvent(int cha_id, const QString& mess, int reader_idx_key, int _msg_id, bool _wait_for_reply);
    bool wait_for_reply, silent;
    QString msg;
    int chat_id, key, message_id;
};

class CuBotServerMakeD3JsPlotUrl : public QEvent {
public:
    CuBotServerMakeD3JsPlotUrl(int cha_id, const QString &csv_datafilenam);

    QString jsplot_url;
    int chat_id;
};

// this has been removed because posting events to invoke process
// requires each module to implement a queue of decoded data
// not to discard events when multiple subsequent
// messages are delivered to the module
//
// This was the code in cubotserver.cpp CuBotServer::event method
 /* if(e->type() == EventTypes::ServerProcess) {

   CuBotServerProcessEvent *spe = static_cast<CuBotServerProcessEvent *>(e);
   CuBotModule *module = spe->module;
   int t = module->type();
   bool success = module->process();
   if(!success) {
       perr("CuBotServer: module \"%s\" failed to process: %s", qstoc(module->name()),
            qstoc(module->message()));
   }
   d->volatile_ops->consume(spe->chat_id, t);
   spe->accept();
   return true;
}
else */
//
// this is the class declaration
//
/*
class CuBotServerProcessEvent : public QEvent
{
public:
    CuBotServerProcessEvent(CuBotModule *mod, int chat_i);
    int chat_id;
    CuBotModule *module;
};
*/

class CuBotServerReinjectMsgEvent : public QEvent {
public:
    CuBotServerReinjectMsgEvent(const TBotMsg& msg);

    TBotMsg tbotmsg;
};

class CuBotServerAddStatsEvent : public QEvent {
public:
    CuBotServerAddStatsEvent(int ch_id, const CuData& da);
    int chat_id;
    CuData data;
};

#endif // CUBOTSERVEREVENTS_H
