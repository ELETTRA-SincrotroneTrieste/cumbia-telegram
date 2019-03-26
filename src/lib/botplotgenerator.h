#ifndef BOTPLOTGENERATOR_H
#define BOTPLOTGENERATOR_H

#include <cubotvolatileoperation.h>
#include <cubotplugininterface.h>

#include <QByteArray>
#include <vector>

class CuData;

class BotPlotGenerator : public CuBotVolatileOperation
{
public:
    enum Type { PlotGen = 0x04 };

    BotPlotGenerator(int chat_id, const CuData& data);

    ~BotPlotGenerator();

    QByteArray generate() const;

    // VolatileOperation interface
public:
    void consume(int moduletype);
    int type() const;
    QString name() const;
    void signalTtlExpired();
    bool disposeWhenOver() const;

private:
    std::vector<double> m_data;
    int m_chat_id;
    QString m_source;

public:
};

#endif // BOTPLOTGENERATOR_H
