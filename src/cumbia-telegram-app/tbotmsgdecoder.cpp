#include "tbotmsgdecoder.h"
#include "lib/tbotmsg.h"
#include "lib/cuformulaparsehelper.h"

#include <QString>
#include <QtDebug>
#include <QRegularExpression>
#include <cutango-world.h>

TBotMsgDecoder::TBotMsgDecoder()
{
    m_type = Invalid;
    m_cmdLinkIdx = -1;
}

TBotMsgDecoder::TBotMsgDecoder(const TBotMsg& msg, const QString &normalizedFormulaPattern)
{
    m_normalizedFormulaPattern = normalizedFormulaPattern; // first
    m_type = decode(msg);                                  // after!
}

TBotMsgDecoder::Type TBotMsgDecoder::type() const
{
    return m_type;
}

QString TBotMsgDecoder::host() const
{
    return m_host;
}

QString TBotMsgDecoder::source() const
{
    return m_source;
}

QString TBotMsgDecoder::text() const
{
    return m_text;
}

//
//  Decode incoming messages
//
//  start decoding from easiest cases and let the most challenging ones be the last
//
TBotMsgDecoder::Type TBotMsgDecoder::decode(const TBotMsg &msg)
{
    m_cmdLinkIdx = -1;
    m_type = Invalid;
    m_text = msg.text();
    //
    // (1) easiest
    //
    if(m_text == "/start") m_type = Start;
    else if(m_text == "/stop") m_type = Stop;
    else if(m_text == "/help" || m_text == "help") m_type = Help;
    else if(m_text == "/help_alerts" || m_text == "help_alerts") m_type = HelpAlerts;
    else if(m_text == "/help_monitor" || m_text == "help_monitor") m_type = HelpMonitor;
    else if(m_text == "/help_search" || m_text == "help_search") m_type = HelpSearch;
    else if(m_text == "/help_host" || m_text == "help_host") m_type = HelpHost;
    else {
        // 1. host moved to plugin

        // 2. re.setPattern("/(?:read|monitor|alert)(\\d{1,2})\\b");
        // moved to plugin
        // 3. try "/X1, /X2, shortcuts to stop monitor...
        // /X1, /X2 to stop monitors moved to monitor MODULE
        // delete bookmarks moved to plugin
    }
    //
    // Difficult cases last
    if(m_cmdLinkIdx > 0 && m_type == Invalid) {
        // cmd link moved to PLUGINS
    }
    else if(m_type == Invalid) {
        // moved to botreader_mod
    } //  m_cmdLinkIdx < 0
    return m_type;
}

TBotMsgDecoder::Type TBotMsgDecoder::m_decodeSrcCmd(const QString &txt)
{
    m_type = Invalid;
    QStringList cmd_parts = txt.split(QRegExp("\\s+"), QString::SkipEmptyParts);
    QRegularExpression re;
    QRegularExpressionMatch match;
    m_type = m_StrToCmdType(txt);
    if(m_type == Invalid) {
        if(m_tryDecodeFormula(txt))
            m_type = Read;
    }
    else {
        // first string must be a valid command on a source (or formula)
        // such as monitor or alert
        //        if(m_type == AttSearch)
        //            m_source = m_findDevice(cmd_parts.at(1));
        //        else  if

        if(m_type != Invalid) { // monitor|stop or some other action on tango source
            QString restOfLine;
            for(int i = 1; i < cmd_parts.size(); i++) {
                i < cmd_parts.size() - 1 ? restOfLine += cmd_parts[i] + " " : restOfLine += cmd_parts[i];
            }
            m_tryDecodeFormula(restOfLine); // find source in second param
            if((m_type == Monitor || m_type == Alert ) && m_detectedSources.size() == 0) {
                // no monitor without real sources
                m_type = Invalid;
                m_msg = "TBotMsgDecoder: cannot monitor (alert) without valid sources";
                perr("%s", qstoc(m_msg));
            }
        }
        else {
            m_msg = "TBotMsgDecoder::m_decodeSrcCmd: unable to parse \"%1\"" + txt;
        }
    }

    return m_type;
}

bool TBotMsgDecoder::m_tryDecodeFormula(const QString &text)
{
    bool is_formula = true;
    // text does not start with either monitor or alarm
    m_source = QString();

    CuFormulaParseHelper ph;
    !ph.isNormalizedForm(text, m_normalizedFormulaPattern) ? m_source = ph.toNormalizedForm(text) : m_source = text;
    m_detectedSources = ph.sources(m_source);
    return is_formula;
}

TBotMsgDecoder::Type TBotMsgDecoder::m_StrToCmdType(const QString &cmd)
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

QString TBotMsgDecoder::m_findSource(const QString &text)
{
    m_msg.clear();
    QString src;
    // admitted chars for Tango names
    const char* tname_pattern = "[A-Za-z0-9_\\-\\.]+";
    // tango attribute pattern: join tname_pattern with three '/'
    const QString tango_attr_src_pattern = QString("^%1/%1/%1/%1$").arg(tname_pattern);
    // allow multiple pattern search in the future (commands?)
    QStringList patterns = QStringList() << tango_attr_src_pattern;
    return m_findByPatterns(text, patterns);
}

QString TBotMsgDecoder::m_findDevice(const QString &text)
{
    m_msg.clear();
    QString src;
    // admitted chars for Tango names
    const char* tname_pattern = "[A-Za-z0-9_\\-\\.]+";
    // tango attribute pattern: join tname_pattern with three '/'
    const QString tango_attr_src_pattern = QString("%1/%1/%1").arg(tname_pattern);
    // allow multiple pattern search in the future (commands?)
    QStringList patterns = QStringList() << tango_attr_src_pattern;
    return m_findByPatterns(text, patterns);
}

QString TBotMsgDecoder::m_findByPatterns(const QString &text, const QStringList &patterns)
{
    m_msg.clear();
    QString out;
    QRegularExpression re;
    QRegularExpressionMatch match;
    for(int i = 0; i < patterns.size() && out.isEmpty(); i++) {
        QString s = patterns[i];
        re.setPattern(s);
        match = re.match(text);
        if(match.hasMatch()) {
            out = match.captured(0);
        }
    } // for
    if(out.isEmpty())
        m_msg = "TBotMsgDecoder: \"" + text + "\" is not a valid source";
    return out;
}

QString TBotMsgDecoder::m_getFormula(const QString &f)
{
    QString validated_formula;
    // do some validation checks in the future?
    validated_formula = f;
    return validated_formula;
}

/**
 * @brief TBotMsgDecoder::getArgs returns the list of arguments after the first detected word
 *
 * This is used to detect arguments following a command, for example after "stop".
 *
 * @return a QStringList with the arguments following the first
 *
 * \par example
 * Running m_getArgs on  "stop double_scalar long_scalar" would return QStringList("double_scalar", "long_scalar")
 */
QStringList TBotMsgDecoder::getArgs() const
{
    QStringList a(m_text.split(QRegExp("\\s+")));
    a.removeAt(0);
    return a;
}

QStringList TBotMsgDecoder::detectedSources() const
{
    return m_detectedSources;
}


int TBotMsgDecoder::cmdLinkIdx() const
{
    return  m_cmdLinkIdx;
}

bool TBotMsgDecoder::error() const
{
    return m_type == Invalid;
}

QString TBotMsgDecoder::message() const
{
    return m_msg;
}
