#include "cubotvolatileoperation.h"

CuBotVolatileOperation::CuBotVolatileOperation()
{
    d_life_cnt = 1;
    m_datetime = QDateTime::currentDateTime();
}

CuBotVolatileOperation::~CuBotVolatileOperation()
{

}

/**
 * @brief VolatileOperation::lifeCount number of lives remaining for the volatile operation
 * @return an integer representing the life count of the operation
 *
 * When lifeCount returns less than zero, operation will be removed and destroyed
 * by VolatileOperations
 */
int CuBotVolatileOperation::lifeCount() const
{
    return d_life_cnt;
}

/**
 * @brief VolatileOperation::ttl time to live of a volatile operation.
 *        Default 10 minutes
 * @return 600 seconds;
 */
int CuBotVolatileOperation::ttl() const {
    return 60 * 10;
}

QDateTime CuBotVolatileOperation::creationTime()
{
    return m_datetime;
}

QString CuBotVolatileOperation::message() const {
    return d_msg;
}

bool CuBotVolatileOperation::error() const {
    return d_error;
}
