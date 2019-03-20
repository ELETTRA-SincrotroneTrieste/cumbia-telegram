#include "cubotserverevents.h"



CuBotServerProcessEvent::CuBotServerProcessEvent(CuBotModule *mod, int chat_i) : QEvent (EventTypes::type(EventTypes::ServerProcess))
{
    module = mod;
    chat_id = chat_i;
}

CuBotServerProcessEvent::~CuBotServerProcessEvent()
{

}


CuBotServerSendMsgEvent::CuBotServerSendMsgEvent(int cha_id, const QString &mess,
                                                 bool _silent, bool _wait_for_reply)
    : QEvent(EventTypes::type(EventTypes::SendMsgRequest))
{
    chat_id = cha_id;
    msg = mess;
    silent = _silent;
    wait_for_reply = _wait_for_reply;
}

CuBotServerSendMsgEvent::~CuBotServerSendMsgEvent()
{

}
