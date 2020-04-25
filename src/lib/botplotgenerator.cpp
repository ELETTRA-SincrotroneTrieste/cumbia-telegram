#include "botplotgenerator.h"
#include <cudata.h>
#include "botplot.h"

BotPlotGenerator::BotPlotGenerator(int chat_id, const CuData &data)
{
    m_chat_id = chat_id;
    if(data.containsKey("value")) {
        const CuVariant& v = data["value"];
        v.toVector<double>(m_data);
        m_source = QString::fromStdString(data["src"].toString());
    }
}

BotPlotGenerator::~BotPlotGenerator()
{
    printf("\e[1;31m~BotPlotGenerator %p\e[0m\n", this);
}

QString BotPlotGenerator::generate() const
{
    return BotPlot().toCsv(m_source, m_data);
}


void BotPlotGenerator::consume(int moduletype)
{
    d_life_cnt--;
}

int BotPlotGenerator::type() const
{
    return PlotGen;
}

QString BotPlotGenerator::name() const
{
     return "generate a plot";
}

void BotPlotGenerator::signalTtlExpired()
{
}


bool BotPlotGenerator::disposeWhenOver() const
{
    return true;
}
