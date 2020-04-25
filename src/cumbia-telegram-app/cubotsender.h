#ifndef CUBOTSENDER_H
#define CUBOTSENDER_H

#include <QObject>
#include <QNetworkReply>

class CuBotSenderPrivate;
class CuData;

class CuBotSender : public QObject
{
    Q_OBJECT
public:
    explicit CuBotSender(QObject *parent, const QString &bot_tok);

signals:
    void messageSent(int chat_id, int message_id, int key = -1);

public slots:
    /**
     * @brief sendMessage
     * @param chat_id
     * @param msg
     * @param silent if true, sends the message silently. Users will receive a notification with no sound.
     */
    void sendMessage(int chat_id, const QString& msg, bool silent = false,
                     bool wait_for_reply = false, int key = -1, bool disable_web_preview = true);

    void editMessage(int chat_id, int key, const QString& msg, int msg_id, bool wait_for_reply = false);

    void sendPic(int chat_id, const QByteArray& imgBytes, bool silent = false);

private slots:
    void onNetworkError(QNetworkReply::NetworkError e);
    void onReply();

private:
    CuBotSenderPrivate *d;

    QString m_truncateMsg(const QString &in);

    void m_getIds(const QByteArray& ba, int& chat_id, int& message_id) const;

    void m_do_sendMsg(int chat_id, int key, const QString &method,
                      const QString& msg, QUrlQuery& params, bool wait_for_reply,
                      bool disable_web_preview = true);
};

#endif // CUBOTSENDER_H
