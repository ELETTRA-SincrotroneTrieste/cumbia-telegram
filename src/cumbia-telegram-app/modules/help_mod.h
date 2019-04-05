#ifndef HELP_MOD_H
#define HELP_MOD_H

#include <cubotmodule.h>
#include <QList>

class HelpModPrivate;

class HelpMod : public CuBotModule
{
public:
    enum Type { HelpModType = 125 };

    HelpMod(CuBotModuleListener *lis);

    void setModuleList(const QList<CuBotModule *> ml);
    ~HelpMod();

    void reset();

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

private:
    HelpModPrivate *d;
    QString m_text(const QString &help_fnam) const;
    QString m_help_list() const;
    QString m_module_list() const;
};

#endif // HELP_MOD_H
