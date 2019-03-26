#include "cubotserverevents.h"


// see comments in header file
/*
CuBotServerProcessEvent::CuBotServerProcessEvent(CuBotModule *mod, int chat_i) : QEvent (EventTypes::type(EventTypes::ServerProcess))
{
    module = mod;
    chat_id = chat_i;
}
*/

CuBotServerSendMsgEvent::CuBotServerSendMsgEvent(int cha_id, const QString &mess,
                                                 bool _silent, bool _wait_for_reply)
    : QEvent(EventTypes::type(EventTypes::SendMsgRequest))
{
    chat_id = cha_id;
    msg = mess;
    silent = _silent;
    wait_for_reply = _wait_for_reply;
}

CuBotServerSendPicEvent::CuBotServerSendPicEvent(int cha_id, const QByteArray &img)
    : QEvent(EventTypes::type(EventTypes::SendPicRequest))
{
    chat_id = cha_id;
    img_ba = img;
}

CuBotServerReinjectMsgEvent::CuBotServerReinjectMsgEvent(const TBotMsg &msg)
    : QEvent (EventTypes::type(EventTypes::ReinjectMsgRequest))
{
    tbotmsg = msg;
}

CuBotServerReplaceVolatileOpEvent::CuBotServerReplaceVolatileOpEvent(int ch_id, CuBotVolatileOperation *vo) :
    QEvent(EventTypes::type(EventTypes::ReplaceVolatileOp))
{
    vop = vo;
    chat_id = ch_id;
}

CuBotServerAddVolatileOpEvent::CuBotServerAddVolatileOpEvent(int ch_id, CuBotVolatileOperation *vo)
 : QEvent(EventTypes::type(EventTypes::AddVolatileOp))
{
    vop = vo;
    chat_id = ch_id;
}

CuBotServerAddStatsEvent::CuBotServerAddStatsEvent(int ch_id, const CuData &da)
    : QEvent(EventTypes::type(EventTypes::AddStatsRequest))
{
    chat_id = ch_id;
    data = da;
}
