#include "settings_mod.h"
#include <optiondesc.h>

SettingsMod::SettingsMod(CuBotModuleListener *lis) : CuBotModule(lis)
{
    m_user_id = m_chat_id = -1;
    m_error = false;
}


int SettingsMod::type() const
{
    return SettingsModule;
}

QString SettingsMod::name() const
{
    return "settings module";
}

QString SettingsMod::description() const
{
    return "gathers the configuration options from all the modules";
}

QString SettingsMod::help() const
{
    return "settings";
}

void SettingsMod::setModuleList(const QList<CuBotModule *> ml) {
    m_module_list = ml;
}

int SettingsMod::decode(const TBotMsg &msg)
{
    m_chat_id = msg.chat_id;
    m_user_id = msg.user_id;
    if( msg.text() == "settings" || msg.text() == "/settings") {
        return type();
    }
    return -1;
}

bool SettingsMod::process()
{
    m_error = false;
    QString msg;
    foreach (CuBotModule *m, m_module_list) {
        const QList<OptionDesc> &om = m->getOptionsDesc();
        if(!om.isEmpty()) {
            msg += "<b>" + m->name() + "</b>\n";
            foreach(const OptionDesc &od, om) {
                // get current value
                QVariant v = m->getOption(od.key);
                QString current_op = v.toString();
                const QMap<QString, QString> opmap = od.options;
                foreach(QString desc, opmap.keys()) {
                    msg += "- " + desc + " /set_" + od.key + "_" + opmap[desc];
                    (current_op == opmap[desc]) ? msg += " [&lt--]\n" : msg += "\n";
                }
            }
        }
    }
    if(msg.isEmpty())
        msg = "<b>settings module</b>: no loaded modules export any option";
    getModuleListener()->onSendMessageRequest(m_chat_id, msg);
    return !m_error;
}

bool SettingsMod::error() const
{
    return m_error;
}

QString SettingsMod::message() const
{
    return m_message;
}
