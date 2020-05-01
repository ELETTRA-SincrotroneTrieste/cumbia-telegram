#ifndef CUBOTWRITER_MOD_H
#define CUBOTWRITER_MOD_H

#include <QStringList>
#include "../lib/cubotmodule.h"

class BotWriterModulePrivate;
class CumbiaSupervisor;
class CuVariant;

class BotWriterMod : public QObject, public CuBotModule
{
Q_OBJECT

public:
    enum ModuleType { WriterType = 200 };

    BotWriterMod(QObject*parent, const CumbiaSupervisor &cu_s, CuBotModuleListener*lis, BotDb *db, BotConfig *conf);

    virtual ~BotWriterMod();

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

    // CuBotPluginInterface interface
public:
    void init(CuBotModuleListener *listener, BotDb *db, BotConfig *bot_conf);

private slots:
    void onWriterData(const CuData& da);

private:

    QString m_history_op_msg(const QDateTime& dt, const QString& name) const;

    BotWriterModulePrivate *d;

};

#endif // CUTELEGRAM_WRITE_PLUGIN_H
