#include "cubotsender.h"
#include <cudata.h>
#include <QString>
#include <QNetworkReply>
#include <QUrlQuery>
#include <QNetworkAccessManager>
#include <QHttpMultiPart>
#include <QHttpPart>
#include <QIODevice>
#include <QFile>
#include <QFileInfo>
#include <QMimeDatabase>
#include <cumacros.h>
#include <unistd.h>
#include <QEventLoop>
#include <tbotmsg.h>
#include <QJsonDocument>
#include <QJsonParseError>
#include <QJsonArray>
#include <QJsonValue>

#define TELEGRAM_MAX_MSGLEN 4096

class CuBotSenderPrivate {
public:
    QString key;
    QNetworkAccessManager *manager;
    QNetworkRequest netreq;
    QString msg;
};

CuBotSender::CuBotSender(QObject *parent, const QString& bot_tok) : QObject(parent)
{
    d = new CuBotSenderPrivate;
    d->manager = new QNetworkAccessManager(this);
    d->netreq.setSslConfiguration(QSslConfiguration::defaultConfiguration());
    d->netreq.setRawHeader("User-Agent", "cumbia-telegram-bot 1.0");
    d->key = bot_tok;
}

void CuBotSender::sendMessage(int chat_id, const QString &msg, bool silent, bool wait_for_reply, int key)
{
    QUrlQuery params;
    if(silent)
        params.addQueryItem("disable_notification", "true");
    printf("FUCKIN SEND MESSAGE CuBotSender;;sendMessaeg to %d %s\n", chat_id, qstoc(msg));
    m_do_sendMsg(chat_id, key, "sendMessage", msg, params, wait_for_reply);
}

void CuBotSender::editMessage(int chat_id, int key, const QString &msg, int msg_id, bool wait_for_reply)
{
    QUrlQuery params;
    params.addQueryItem("message_id", QString::number(msg_id));
    printf("\e[1;32mCuBotSender::editMessage: editMessageRequest! chat %d key %d message_id %d\e[0m\n", chat_id, key, msg_id);
    m_do_sendMsg(chat_id, key, "editMessageText", msg, params, wait_for_reply);
}

void CuBotSender::m_do_sendMsg(int chat_id, int key, const QString& method,
                               const QString &msg, QUrlQuery &params,
                               bool wait_for_reply)
{
    QString u = QString("https://api.telegram.org/%1/%2").arg(d->key).arg(method);
    params.addQueryItem("chat_id", QString::number(chat_id));
    params.addQueryItem("parse_mode", "HTML");
    if(msg.length() > TELEGRAM_MAX_MSGLEN) {
        params.addQueryItem("text", m_truncateMsg(msg));
    }
    else {
        params.addQueryItem("text", msg);
    }

    // disable link preview (currently only help would contain links)
    params.addQueryItem("disable_web_page_preview", "true");

    QUrl url(u);
    url.setQuery(params);
    d->netreq.setUrl(url);

    QNetworkReply *reply = d->manager->get(d->netreq);
    reply->setProperty("key", key);
    if(wait_for_reply) {
        QEventLoop loop;
        QObject::connect(reply, SIGNAL(readyRead()), &loop, SLOT(quit()));
        reply->deleteLater();
    }
    else {
        connect(reply, SIGNAL(readyRead()), this, SLOT(onReply()));
        connect(reply, SIGNAL(error(QNetworkReply::NetworkError )), this, SLOT(onNetworkError(QNetworkReply::NetworkError)));
    }
}

void CuBotSender::sendPic(int chat_id, const QByteArray &imgBytes, bool silent)
{
    QHttpMultiPart *mpart = new QHttpMultiPart(QHttpMultiPart::FormDataType);
    QHttpPart imgPart;
    imgPart.setHeader(QNetworkRequest::ContentDispositionHeader, QVariant(QString("form-data; name=\"photo\";"
                                                                                  "filename=\"plot.png\"")));
    imgPart.setHeader(QNetworkRequest::ContentTypeHeader, QMimeDatabase().mimeTypeForData(imgBytes).name());
    imgPart.setBody(imgBytes);
    mpart->append(imgPart);

    QString u = "https://api.telegram.org/bot635922604:AAEgG6db_3kkzYZqh-LBxi-ubvl5UIEW7gE/sendPhoto";
    QUrlQuery params;
    params.addQueryItem("chat_id", QString::number(chat_id));
    if(silent)
        params.addQueryItem("disable_notification", "true");
    QUrl url(u);
    url.setQuery(params);
    d->netreq.setUrl(url);
    QNetworkReply *reply = d->manager->post(d->netreq, mpart);
    mpart->setParent(reply);  // delete the multiPart with the reply
    connect(reply, SIGNAL(readyRead()), this, SLOT(onReply()));
    connect(reply, SIGNAL(error(QNetworkReply::NetworkError )), this, SLOT(onNetworkError(QNetworkReply::NetworkError)));

}

void CuBotSender::onNetworkError(QNetworkReply::NetworkError nerr)
{
    QNetworkReply *reply = qobject_cast<QNetworkReply *>(sender());
    perr("CuBotSender::onNetworkError: %s [%d]", qstoc(reply->errorString()), nerr);
    reply->deleteLater();
}

void CuBotSender::onReply()
{
    QNetworkReply *reply = qobject_cast<QNetworkReply *>(sender());
    const QByteArray ba = reply->readAll();
    int mid, chat_id;
    m_getIds(ba, chat_id, mid);
    printf("\e[1;32mCuBotSender:onReply: message_id %d got from \e[0m%s KEY %d\n", mid, ba.data(), reply->property("key").toInt());
    emit messageSent(chat_id, mid, reply->property("key").toInt());
    reply->deleteLater();
}

QString CuBotSender::m_truncateMsg(const QString &in)
{
    QString trunc = in;
    QString suffix = " ... \n\n(<b>msg too long</b>)";
    trunc.truncate(TELEGRAM_MAX_MSGLEN - suffix.length());
    // remove all html tags
    trunc.remove("<b>").remove("</b>").remove("<i>").remove("</i>");
    return trunc + suffix;
}

void CuBotSender::m_getIds(const QByteArray &ba, int& chat_id, int& message_id) const
{
    d->msg.clear();
    QJsonParseError pe;
    QJsonDocument jdoc = QJsonDocument::fromJson(ba, &pe);
    bool decode_err = (pe.error != QJsonParseError::NoError);
    if(decode_err)
        d->msg = "CuBotSender::m_getId: " + pe.errorString() + ": offset: " + QString::number(pe.offset);
    else {
        const QJsonValue& result = jdoc["result"];
        if(!result.isNull()) {
            const QJsonValue &jmsg_id = result["message_id"];
            const QJsonValue &jchat = result["chat"];
            const QJsonValue &jchat_id = jchat["id"];
            if(jmsg_id.isDouble() && jchat_id.isDouble()) {
                message_id = jmsg_id.toInt();
                chat_id = jchat_id.toInt();
            }
        }
    }
}

