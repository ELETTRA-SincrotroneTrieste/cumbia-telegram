#ifndef CUBOTVOLATILEOPERATION_H
#define CUBOTVOLATILEOPERATION_H

#include <QDateTime>

class CuBotVolatileOperation
{
public:
    CuBotVolatileOperation();

    virtual ~CuBotVolatileOperation();

    virtual void consume(int module_type) = 0;

    /**
     * @brief disposeWhenOver return true if VolatileOperations is allowed to delete the object, false otherwise
     * @return true: CuBotVolatileOperations can delete this operation either when life is over or from within
     *                CuBotVolatileOperations destructor
     *
     * @return false: CuBotVolatileOperations is *never* allowed to delete this operation
     */
    virtual bool disposeWhenOver() const = 0;

    virtual int lifeCount() const;

    virtual int type() const = 0;

    virtual QString name() const = 0;

    virtual void signalTtlExpired() = 0;

    // default time to live of a volatile operation, in seconds
    virtual int ttl() const;

    QDateTime creationTime();

    QString message() const;

    bool error() const;

protected:
    int d_life_cnt;

    QString d_msg;
    bool d_error;

private:
    QDateTime m_datetime;
};

#endif // VOLATILEOPERATION_H
