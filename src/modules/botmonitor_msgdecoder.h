#ifndef BotMonitorMsgDecoder_H
#define BotMonitorMsgDecoder_H

class TBotMsg;

#include <QStringList>
#include <QDateTime>

class BotMonitorMsgDecoder
{
public:


    enum Type { Invalid, Monitor, Alert, StopMonitor, CmdLink, MaxType = 16 };

    const char types[MaxType][48] = { "Invalid",  "Monitor", "Alert", "StopMonitor",
                                      "CmdLink",
                                      "MaxType" };

    BotMonitorMsgDecoder();

    void setNormalizedFormulaPattern(const QString& nfp);

    Type type() const;

    bool hasHost() const;

    QString host() const;

    QString source() const;

    QString text() const;

    Type decode(const TBotMsg &msg);

    int cmdLinkIdx() const;

    bool error() const;

    QString message() const;

    QStringList getArgs() const;

    QStringList detectedSources() const;

    QStringList getAliasSections() const;

    int chatId() const;

    int userId() const;

    QDateTime startDateTime() const;

private:

    bool m_tryDecodeFormula(const QString& text);
    Type m_decodeSrcCmd(const QString& text);
    Type m_StrToCmdType(const QString& cmd);
    QString m_findSource(const QString& text);
    QString m_findDevice(const QString &text);
    QString m_findByPatterns(const QString& text, const QStringList &patterns);
    QString m_getFormula(const QString& f);

    Type m_type;

    QString m_host;
    QString m_source;
    QString m_text;

    int m_cmdLinkIdx;

    QString m_msg;

    QString m_normalizedFormulaPattern;

    QStringList m_detectedSources, m_args;

    int m_chat_id, m_user_id;
    QDateTime m_startDt;
};

#endif // BotMonitorMsgDecoder_H
