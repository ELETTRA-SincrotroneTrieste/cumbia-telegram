#include "botplot.h"
#include <cudata.h>
#include <QDir>
#include <cuvariant.h>
#include <QTime>
#include <QFile>
#include <QTextStream>
#include <QtDebug>

BotPlot::BotPlot()
{
}

QString BotPlot::toCsv(const QString &src, const std::vector<double> &ve)
{
    error_message = QString();
    QByteArray ba;
    QString fna = QString("plot_%1.csv").arg(QDateTime::currentDateTime().toMSecsSinceEpoch());
    QString fpath = "/home/test/devel/cumbia-telegram/d3plots/" + fna;
    QFile f(fpath);
    if(f.open(QIODevice::WriteOnly)) {
        QTextStream out(&f);
        out << "xval,yval\n";
        for(size_t i = 0; i < ve.size(); i++) {
            out << QString("%1,%2\n").arg(i).arg(ve[i]);
        }
        f.close();
    }
    return fna;
}
