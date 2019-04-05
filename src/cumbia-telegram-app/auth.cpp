#include "auth.h"
#include <botdb.h>
#include <botconfig.h>

Auth::Auth(BotDb *db, BotConfig *cfg)
{
    m_db = db;
    m_cfg = cfg;
    m_limit = -1;
}

/**
 * @brief Auth::isAuthorized returns true if the type of operation is authorized to the user uid
 *
 * The first step is to test whether the user has been globally authorized to use the bot
 * (line: \code m_limit = m_db->isAuthorized(uid, operation);  \endcode ) (*auth* table)
 *
 * The next step is to look into the *auth_limits* table for the given operation and user id
 * If no entry is found, isAuthorized fetches the default value from the BotConfig::getDefaultAuth
 * method. BotConfig::getDefaultAuth returns a default value for the operation regardless of the
 * user id. If BotConfig::getDefaultAuth returns negative, no value has been found for the given
 * operation. In that case, this method returns true, because no explicit limitation has been found
 * for the (operation,uid) pair.
 *
 * @param uid the user id
 * @param operation  a string describing the operation
 * @return true the user is authorized
 * @return false the user is not authorized
 *
 * In case isAuthorized returns false, you can call the method reason to know why.
 *
 * \par Important
 * Read BotDb::isAuthorized documentation for further details.
 */
bool Auth::isAuthorized(int uid, const QString& operation)
{
    bool unregistered;
    m_limit = -1;
    m_reason.clear();
    if(m_limit < 0) {
        m_limit = m_db->isAuthorized(uid, operation, &unregistered);
        if(m_limit < 0 && unregistered) {
            m_reason = "Unauthorized: still waiting for authorization";
        }
        else if(m_limit < 0) {
            m_reason ="Unauthorized: authorization denied for the \"" + operation + "\" operation";
        }
        else if(!operation.isEmpty() && m_limit == 0) {
            // get defaults from BotConfig
            int default_op_limit = m_cfg->getDefaultAuth(operation);
            if(default_op_limit < 0) // operation unknown by BotConfig
                m_limit = 0;
        }
        else { // operation empty: special auth not required
            m_limit = 1;
        }
    }
    return  m_limit >= 0;
}

int Auth::limit() const
{
    return m_limit;
}

QString Auth::reason() const
{
    return m_reason;
}
