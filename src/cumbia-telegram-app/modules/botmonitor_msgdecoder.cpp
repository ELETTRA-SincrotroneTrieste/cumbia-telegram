#include "botmonitor_msgdecoder.h"
#include "../lib/tbotmsg.h"
#include "../lib/cuformulaparsehelper.h"

#include <QString>
#include <QtDebug>
#include <QRegularExpression>
#include <cutango-world.h>
#include <cucontrolsfactorypool.h>

BotMonitorMsgDecoder::BotMonitorMsgDecoder(const CuControlsFactoryPool &fap) {
    m_type = Invalid;
    m_cmdLinkIdx = -1;
    m_chat_id = m_user_id = -1;
    m_formula_parser_helper = new CuFormulaParseHelper(fap);
}

BotMonitorMsgDecoder::~BotMonitorMsgDecoder()
{
    delete m_formula_parser_helper;
}

void BotMonitorMsgDecoder::setNormalizedFormulaPattern(const QString &nfp) {
    m_normalizedFormulaPattern = nfp;
}

BotMonitorMsgDecoder::Type BotMonitorMsgDecoder::type() const {
    return m_type;
}

bool BotMonitorMsgDecoder::hasHost() const {
    return m_host.size() > 0;
}

QString BotMonitorMsgDecoder::host() const {
    return m_host;
}

QString BotMonitorMsgDecoder::source() const {
    return m_source;
}

QString BotMonitorMsgDecoder::text() const {
    return m_text;
}

QString BotMonitorMsgDecoder::description() const
{
    return m_description;
}

BotMonitorMsgDecoder::Type BotMonitorMsgDecoder::decode(const TBotMsg &msg) {
    m_cmdLinkIdx = -1;
    m_type = Invalid;
    m_text = msg.text();
    m_host = msg.host();
    // description: a comment after the command text, starting with `//' and extracted
    // by TBotMsg
    m_description = msg.description();
    m_text.replace(QRegularExpression("\\s+"), " ");
    m_chat_id = msg.chat_id;
    m_user_id = msg.user_id;
    m_startDt = msg.start_dt;

    QRegularExpression re;
    QRegularExpressionMatch match;
    re.setPattern("/(?:read|monitor|alert)(\\d{1,2})\\b");
    match = re.match(m_text);
    if(match.hasMatch()) {
        m_cmdLinkIdx = match.captured(1).toInt();
        m_type = CmdLink;
    }
    else  { // try "/X1, /X2, shortcuts to stop monitor...
        re.setPattern("/X(\\d{1,2})\\b");
        match = re.match(m_text);
        if(match.hasMatch()) {
            m_cmdLinkIdx = match.captured(1).toInt();
            m_type = StopMonitor;
        }
        else { // settings
            re.setPattern("/{0,1}set_");
            if(re.match(m_text).hasMatch())
                m_type = Settings;
        }

    }
    if(m_type == Invalid)
        m_type = m_decodeSrcCmd(m_text.trimmed());
    return m_type;
}

int BotMonitorMsgDecoder::cmdLinkIdx() const {
    return m_cmdLinkIdx;
}

BotMonitorMsgDecoder::Type BotMonitorMsgDecoder::m_decodeSrcCmd(const QString &txt) {
    m_type = Invalid;
    QStringList cmd_parts = txt.split(QRegExp("\\s+"), QString::SkipEmptyParts);
    m_type = m_StrToCmdType(txt);
    printf("\e[0;34mBotMonitorMsgDecoder::m_decodeSrcCmd decoded type is %d\e[0m\n", m_type);
    if(m_type != Invalid)  {
        // monitor|stop or some other action on tango source
        QString restOfLine;
        for(int i = 1; i < cmd_parts.size(); i++) {
            i < cmd_parts.size() - 1 ? restOfLine += cmd_parts[i] + " " : restOfLine += cmd_parts[i];
        }
        printf("\e[0;34mtryDecodeFrmula with rest of line %s\e[0m\n", qstoc(restOfLine));
        m_tryDecodeFormula(restOfLine); // find source in second param
        if((m_type == Monitor || m_type == Alert ) && m_detectedSources.size() == 0) {
            // no monitor without real sources
            m_type = Invalid;
            m_msg = "BotMonitorMsgDecoder: cannot monitor (alert) without valid sources";
            perr("%s", qstoc(m_msg));
        }
    }
    return m_type;
}

bool BotMonitorMsgDecoder::m_tryDecodeFormula(const QString &text) {
    bool is_formula = true;
    // text does not start with either monitor or alarm
    m_source = QString();
    const CuFormulaParseHelper &ph = *m_formula_parser_helper;
    !ph.isNormalizedForm(text, m_normalizedFormulaPattern) ? m_source = ph.toNormalizedForm(text) : m_source = text;
    m_detectedSources = ph.sources(m_source);
    return is_formula;
}

BotMonitorMsgDecoder::Type BotMonitorMsgDecoder::m_StrToCmdType(const QString &cmd)
{
    m_msg.clear();
    if(cmd.startsWith("monitor ") || cmd.startsWith("mon "))
        return  Monitor;
    else if(cmd.startsWith("alert "))
        return Alert;
    else if(cmd.startsWith("stop"))
        return StopMonitor;
    return Invalid;
}

QStringList BotMonitorMsgDecoder::detectedSources() const {
    return m_detectedSources;
}

int BotMonitorMsgDecoder::chatId() const {
    return m_chat_id;
}

int BotMonitorMsgDecoder::userId() const {
    return m_user_id;
}

QDateTime BotMonitorMsgDecoder::startDateTime() const {
    return m_startDt;
}

bool BotMonitorMsgDecoder::error() const {
    return m_type == Invalid;
}

QString BotMonitorMsgDecoder::message() const {
    return m_msg;
}

/**
 * @brief BotMonitorMsgDecoder::getArgs returns the list of arguments after the first detected word
 *
 * This is used to detect arguments following a command, for example after "stop".
 *
 * @return a QStringList with the arguments following the first
 *
 * \par example
 * Running m_getArgs on  "stop double_scalar long_scalar" would return QStringList("double_scalar", "long_scalar")
 */
QStringList BotMonitorMsgDecoder::getArgs() const {
    QStringList a(m_text.split(QRegExp("\\s+")));
    a.removeAt(0);
    return a;
}
