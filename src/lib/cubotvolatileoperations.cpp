#include "cubotvolatileoperations.h"
#include <QtDebug>
#include <QTimer>

CuBotVolatileOperations::CuBotVolatileOperations()
{
    QTimer * cleanTimer = new QTimer(this);
    cleanTimer->setSingleShot(false);
    cleanTimer->setInterval(1000);
    connect(cleanTimer, SIGNAL(timeout()), this, SLOT(cleanOld()));
}

CuBotVolatileOperations::~CuBotVolatileOperations()
{
    foreach(CuBotVolatileOperation *vop, m_op_map.values())
        delete vop;
    m_op_map.clear();
}

void CuBotVolatileOperations::addOperation(int chat_id, CuBotVolatileOperation *op)
{
    if(m_op_map.size() == 0)
        findChild<QTimer *>()->start();
    m_op_map.insertMulti(chat_id, op);
}

/**
 * @brief VolatileOperations::replaceOperation replaces operations with the same type as op with
 *        op
 * @param chat_id the chat id where the VolatileOperation has to be replaced
 * @param op the new operation. op type is used to find other operations with the same type
 */
void CuBotVolatileOperations::replaceOperation(int chat_id, CuBotVolatileOperation *op)
{
    QMutableMapIterator<int, CuBotVolatileOperation*> i(m_op_map);
    while(i.hasNext()) {
        i.next();
        if(i.key() == chat_id) {
            CuBotVolatileOperation *vop = i.value();
            if(vop->type() == op->type()) {
                i.remove();
                delete vop;
            }
        }
    }
    addOperation(chat_id, op);
}

void CuBotVolatileOperations::consume(int chat_id, int moduletype)
{
    QMutableMapIterator<int, CuBotVolatileOperation*> i(m_op_map);
    while(i.hasNext()) {
        i.next();
        if(i.key() == chat_id) {
            CuBotVolatileOperation *vop = i.value();
            vop->consume(moduletype);
            if(vop->lifeCount() < 0) {
                i.remove();
                delete vop;
            }
        }
    }

    if(m_op_map.size() == 0)
        findChild<QTimer *>()->stop();
}

CuBotVolatileOperation *CuBotVolatileOperations::get(int chat_id, int type) const
{
    QList<CuBotVolatileOperation *> voplist = m_op_map.values(chat_id);
    foreach(CuBotVolatileOperation *vop, voplist) {
        if(vop->type() == type)
            return vop;
    }
    if(!m_op_map.contains(type)) {
    }

    return nullptr;
}

void CuBotVolatileOperations::cleanOld()
{
    QMutableMapIterator<int, CuBotVolatileOperation*> i(m_op_map);
    QDateTime now = QDateTime::currentDateTime();
    while(i.hasNext()) {
        i.next();
        CuBotVolatileOperation *vop = i.value();
        if(vop->creationTime().secsTo(now) > vop->ttl()) {
            vop->signalTtlExpired();
            i.remove();
            delete vop;
        }
    }
    if(m_op_map.size() == 0)
        findChild<QTimer *>()->stop();
}
