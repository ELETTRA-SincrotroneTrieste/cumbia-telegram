#ifndef BOTPLOT_H
#define BOTPLOT_H

#include <QString>
#include <vector>


class BotPlot
{
public:
    BotPlot();

    QString toCsv(const QString& src, const std::vector<double> &ve);

    QString error_message;

private:
};

#endif // BOTPLOT_H
