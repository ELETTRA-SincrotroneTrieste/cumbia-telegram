#include "help_mod.h"
#include <QList>
#include <cumacros.h>
#include <QTextStream>

class HelpModPrivate {
public:
    int chat_id;
    QString help_section, msg;
    bool err;
    QList<CuBotModule *>modules;
};

HelpMod::HelpMod(CuBotModuleListener *lis)
{
    d = new HelpModPrivate;
    setBotmoduleListener(lis);
    reset();
}

void HelpMod::setModuleList(const QList<CuBotModule *> ml)
{
    d->modules = ml;
}

HelpMod::~HelpMod()
{
    delete d;
}

void HelpMod::reset()
{
    d->chat_id = -1;
    d->help_section.clear();
    d->msg.clear();
    d->err = false;
}

int HelpMod::type() const
{
    return HelpModType;
}

QString HelpMod::name() const
{
    return "help";
}

QString HelpMod::description() const
{
    return "Provides help facilities";
}

QString HelpMod::help() const
{
    return "help";
}

int HelpMod::decode(const TBotMsg &msg)
{
    reset();
    d->chat_id = msg.chat_id;

    if(msg.text().startsWith("/help") || msg.text().startsWith("help") || msg.text() == "modules" || msg.text() == "/modules") {
        d->help_section = msg.text().remove('/');
        return type();
    }
    return -1;
}

bool HelpMod::process()
{
    if(d->help_section.contains("modules"))
        getModuleListener()->onSendMessageRequest(d->chat_id, m_module_list(), true);
    else {
        printf("FORKING CRAPPPPP sending msg %s to %d\n", qstoc(m_text(d->help_section)), d->chat_id);
        getModuleListener()->onSendMessageRequest(d->chat_id, m_text(d->help_section), true); // true: silent
    }
    return true;
}

bool HelpMod::error() const {
    return d->err;
}

QString HelpMod::message() const {
    return  d->msg;
}

QString HelpMod::m_text(const QString& help_fnam) const {
    QString h;
    foreach(CuBotModule *mod, d->modules) {
        if(help_fnam == "help" || help_fnam == mod->help() || help_fnam == "help_" + mod->help()) {
            QString f = ":/help/res/" + help_fnam + ".html";
            QFile hf(f);
            if(hf.open(QIODevice::ReadOnly)) {
                QTextStream in(&hf);
                h = "📚   " + in.readAll();
                hf.close();
            }
            else {
                perr("HelpMod: failed to open file \"%s\" in read only mode: %s",
                     qstoc(f), qstoc(hf.errorString()));
                h += "😞   error getting help: internal error";
            }

            return h;
        }
    }
    // not already returned
    return  "😞   error getting help: no help available for \"" + help_fnam + "\"\n\n" + m_help_list();
}

QString HelpMod::m_help_list() const {
    QString h = "<b>HELP module list</b>\n";
    foreach(CuBotModule *m, d->modules) {
        if(!m->help().isEmpty())
            h += "- <i>/help_" + m->help() + "</i>\n";
    }
    return h;
}

QString HelpMod::m_module_list() const
{
    QString h = "<b>MODULES</b>\n";
    h += "This is the list of modules and plugins loaded by the bot.\n"
            "On every message received, they are employed in the following order.\n\n";

    int i = 1;
    h += QString("%1. <b>help</b> [/help] - <i>%2</i>\n").arg(i).arg(help());
    foreach(CuBotModule *m, d->modules) {
        ++i;
        h += QString("%1. <b>%2</b>").arg(i).arg(m->name());
        m->isPlugin() ? h += "  (<b>plugin</b>) " : h += "";
        if(!m->help().isEmpty())
            h += QString(" [/help_%1]").arg(m->help());
        h += QString(" - <i>%1</i>\n").arg(m->description());

    }
    return h;
}
