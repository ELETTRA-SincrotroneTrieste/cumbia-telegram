#include "tbotmsg.h"
#include "historyentry.h"
#include <QJsonValue>
#include <cumacros.h>
#include <QDateTime>
#include <QRegularExpression>
#include <QRegularExpressionMatch>

TBotMsg::TBotMsg()
{
    chat_id = -1;
    user_id = -1;
    is_bot = false;
    update_id = -1;
    message_id = -1;
    from_private_chat = true;
}

TBotMsg::TBotMsg(const QJsonValue &v)
{
    decode(v);
}

TBotMsg::TBotMsg(const HistoryEntry &he)
{
    chat_id = he.chat_id;
    user_id = he.user_id;
    m_host = he.host;
    // msg_recv_datetime left invalid
    start_dt = he.datetime;
    m_text = he.toCommand();
    m_description = he.description;
    // unavailable from HistoryEntry
    is_bot = false;
    update_id = -1;
    message_id = -1;
    from_history = true;
    from_real_msg = false;
}

void TBotMsg::decode(const QJsonValue &m)
{
    QJsonValue msg = m["message"];
    QJsonValue chat = msg["chat"];
    QJsonValue chat_type = chat["type"].toString();
    chat_first_name = chat["first_name"].toString();
    chat_id = chat["id"].toInt();
    chat_username = chat["username"].toString();
    chat_lang = chat["language_code"].toString();
    from_private_chat = chat["type"].toString() == "private";

    // start_dt is left invalid
    msg_recv_datetime = QDateTime::fromTime_t(msg["date"].toInt());

    QJsonValue from = msg["from"];
    first_name = chat["first_name"].toString();
    user_id = from["id"].toInt();
    username = from["username"].toString();
    lang = from["language_code"].toString();
    is_bot = from["is_bot"].toString();

    message_id = msg["message_id"].toInt();
    m_text  = msg["text"].toString();
    m_description = m_stripDescription(m_text);

    update_id = m["update_id"].toInt();

    from_history = false;
    from_real_msg = true;
}

void TBotMsg::print() const
{
    printf("\e[1;32m--> \e[0;32m%d\e[0m:\t[received: %s]\tfrom:\e[1;32m%s\e[0m [\e[1;34m%s\e[0m]: \e[1;37;4mMSG\e[0m\t"
           "\e[0;32m[\e[0m\"%s\"\e[0;32m]\e[0m\t[msg.id: %d] from_history %d from_real_msg %d\n",
           update_id, qstoc(msg_recv_datetime.toString()), qstoc(username), qstoc(first_name),
           qstoc(m_text), message_id, from_history, from_real_msg);
}

void TBotMsg::setHost(const QString &h)
{
    m_host = h;
}

QString TBotMsg::host() const
{
    return m_host;
}

bool TBotMsg::hasHost() const
{
    return !m_host.isEmpty();
}

/**
 * @brief TBotMsg::text returns the text of the message
 * @return
 */
QString TBotMsg::text() const
{
    return m_text;
}

/**
 * @brief TBotMsg::setText can be used to alter the text of the message after alias replacement
 * @param t the new text
 */
void TBotMsg::setText(const QString &t)
{
    m_text = t;
}

QString TBotMsg::description() const
{
    return m_description;
}

void TBotMsg::setDescription(const QString &desc)
{
    m_description = desc;
}

/**
 * @brief BotMonitorMsgDecoder::m_stripDescription remove description from text, if present
 * @param text the text passed *as reference*. Description will be removed from text, if present
 * @return the description, if present, an empty string otherwise
 *
 * \par Example
 * From the textual command
 * *monitor sr/diagnostics/dcct/Current < 10  // beam dump*
 * \li text will be *monitor sr/diagnostics/dcct/Current < 10*
 * \li the returned string will be *beam dump*
 *
 */
QString TBotMsg::m_stripDescription(QString &text)
{
    QRegularExpression re("//(.+)");
    QRegularExpressionMatch ma = re.match(text);
    if(ma.hasMatch()) {
        text = text.remove(ma.captured(0)).trimmed();
        return ma.captured(1).trimmed();
    }
    text = text.trimmed();
    return QString();
}
