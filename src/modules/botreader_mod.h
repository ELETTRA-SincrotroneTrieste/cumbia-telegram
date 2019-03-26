#ifndef CUBOTREADERMODULE_H
#define CUBOTREADERMODULE_H

#include "../lib/cubotmodule.h"

class BotReaderModulePrivate;
class CumbiaSupervisor;
class CuVariant;

class BotReaderModule : public QObject, public CuBotModule
{
    Q_OBJECT
public:
    enum ModuleType { ReaderType = 2000 };

    enum State { Undefined = 0, SourceDetected, PlotRequest };

    BotReaderModule(QObject*parent, const CumbiaSupervisor &cu_s, CuBotModuleListener*lis, BotDb *db, BotConfig *conf);

    ~BotReaderModule();

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

    void reset();

private slots:
    void onReaderUpdate(int chat_id, const CuData& data);

private:
    BotReaderModulePrivate *d;

    BotReaderModule::State m_decodeSrcCmd(const QString &txt);
    bool m_tryDecodeFormula(const QString &text);
    bool m_isBigSizeVector(const CuData &da) const;
};

#endif // CUBOTREADERMODULE_H
