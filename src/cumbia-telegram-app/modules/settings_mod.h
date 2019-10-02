#ifndef SETTINGS_MOD_H
#define SETTINGS_MOD_H

#include <cubotmodule.h>
#include <QList>

class SettingsMod : public CuBotModule
{
public:
    enum Type { SettingsModule = 620 };

    SettingsMod(CuBotModuleListener *lis);

    // CuBotModule interface
public:
    int type() const;
    QString name() const;
    QString description() const;
    QString help() const;
    int decode(const TBotMsg &msg);
    bool process();
    bool error() const;
    QString message() const;

    void setModuleList(const QList<CuBotModule *> ml);
private:
    int m_user_id, m_chat_id;
    QList<CuBotModule *>  m_module_list;
    bool m_error;
    QString m_message;
};

#endif // SETTINGS_MOD_H
