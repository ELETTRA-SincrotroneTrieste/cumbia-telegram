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
                                                 bool _silent, bool _wait_for_reply, int akay)
    : QEvent(EventTypes::type(EventTypes::SendMsgRequest))
{
    chat_id = cha_id;
    msg = mess;
    silent = _silent;
    wait_for_reply = _wait_for_reply;
    key = akay;
}

CuBotServerMakeD3JsPlotUrl::CuBotServerMakeD3JsPlotUrl(int cha_id, const QString &csv_datafilenam)
    : QEvent(EventTypes::type(EventTypes::SendPicRequest))
{
    chat_id = cha_id;
    jsplot_url = QString("<a href=\"http://gaia.elettra.trieste.it:12800?s=%1\">plot</a>").arg(csv_datafilenam);
}

CuBotServerReinjectMsgEvent::CuBotServerReinjectMsgEvent(const TBotMsg &msg)
    : QEvent (EventTypes::type(EventTypes::ReinjectMsgRequest))
{
    tbotmsg = msg;
}

CuBotServerAddStatsEvent::CuBotServerAddStatsEvent(int ch_id, const CuData &da)
    : QEvent(EventTypes::type(EventTypes::AddStatsRequest))
{
    chat_id = ch_id;
    data = da;
}

CuBotServerEditMsgEvent::CuBotServerEditMsgEvent(int cha_id, const QString &mess,
                                                 int reader_idx_key, int _msg_id, bool _wait_for_reply)
 : QEvent(EventTypes::type(EventTypes::EditMsgRequest))
{
    chat_id = cha_id;
    msg = mess;
    key = reader_idx_key;
    message_id = _msg_id;
    wait_for_reply = _wait_for_reply;
}
