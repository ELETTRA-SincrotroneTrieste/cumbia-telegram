#include "datamsgformatter.h"
#include <QStringList>
#include <cudata.h>
#include <QDateTime>
#include <formulahelper.h>
#include <cudataquality.h>

#define MAXVALUELEN 45

QString DataMsgFormatter::fromData_msg(const CuData &d, FormatOption f, const QString &description)
{
    FormulaHelper fh;
    QString msg, eval_value, vector_info, src, value, quality_str;
    QStringList pointSet, deviceSet, hostSet;
    long int li = d["timestamp_ms"].toLongInt();
    QDateTime datet = QDateTime::fromMSecsSinceEpoch(li);
    bool ok = !d["err"].toBool();
    ok ? msg = "" : msg = "👎";
    if(d.containsKey("srcs")) {
        QString point, device, host;
        src = QString::fromStdString(d["srcs"].toString());
        QStringList srcs = src.split(",", QString::SkipEmptyParts);
        for(int i = 0; i < srcs.size(); i++) {
            cleanSource(srcs[i], point, device, host, f);
            if(!pointSet.contains(point)) pointSet << point;
            if(!deviceSet.contains(device)) deviceSet << device;
            if(!hostSet.contains(host)) hostSet << host;
        }
    }
    else {
        QString point, device, host;
        src = QString::fromStdString(d["src"].toString());
        cleanSource(src, point, device, host, f);
    }

    if(!description.isEmpty()) {
        msg += "<i>" + fh.escape(description) + "</i>\n";
    }
    QString cmd = QString::fromStdString(d["command"].toString());
    if(!cmd.isEmpty())
        msg += "<i>" + fh.escape(cmd) + "</i>\n";

    if(!ok) {
        msg += "\n";
        msg += "😔  <i>" + QString::fromStdString(d["msg"].toString()) + "</i>\n";
    }
    else { // ok
        if(d.containsKey("value")) {
            bool ok;
            const CuVariant &va = d["value"];
            std::string print_format, value_str;

            d.containsKey("print_format") && !d.containsKey("values") ? value_str = va.toString(&ok, d["print_format"].toString().c_str()) :
                value_str = va.toString();
            QString v_str = QString::fromStdString(value_str);
            if(v_str.length() > MAXVALUELEN - 3) {
                v_str.truncate(MAXVALUELEN-3);
                v_str += "...";
            }
            eval_value = value = v_str;

            if(va.getSize() > 1) {
                vector_info = getVectorInfo(va);
            }
        }

        if(d.containsKey("evaluation"))
            eval_value = QString::fromStdString(d["evaluation"].toString());

        // value
        msg += "<b>" + eval_value + "</b>";

        // measurement unit if available
        QString du = QString::fromStdString(d["display_unit"].toString());
        if(!du.isEmpty())
            msg += "  <b>" + fh.escape(QString::fromStdString(d["display_unit"].toString())) +  "</b>";

        eval_value.length() < 10 ? msg += "   " : msg += "\n";

        // if vector, provide some info about len, min and max
        // and then a link to plot it!
        if(!vector_info.isEmpty()) {
            msg += "\n" + vector_info + " /plot\n";
        }

        CuDataQuality quality(d["quality"].toInt());
        CuDataQuality::Type qt = quality.type();
        if(qt != CuDataQuality::Valid) {
            msg += "   ";
            quality_str = QString::fromStdString(quality.name());
            if(qt == CuDataQuality::Warning)
                msg += "😮   ";
            else if(qt == CuDataQuality::Alarm)
                msg += "😱   ";
            else if(qt == CuDataQuality::Invalid)
                msg += "👎   ";
            msg += "<i>" + quality_str + "</i>\n";
        }

        if(f > FormatShort) {
            msg += "\ndata format: <i>" + QString::fromStdString(d["data_format_str"].toString()) + "</i>\n";
            msg += "\nmode:        <i>" + QString::fromStdString(d["mode"].toString()) + "</i>\n";
        }
    }
    // date time
    msg +=  "<i>" + timeRepr(datet) + "</i>";

    QString host = hostSet.join(", ");
    !host.isEmpty() ? msg+= " [<i>" + host + "</i>]" : msg += "";
    int idx = d["index"].toInt();
    //  /Xn command used to stop monitor
    if(idx > -1)
        msg += QString("   /X%1").arg(idx);
    return msg;
}

void DataMsgFormatter::cleanSource(const QString &src, QString& point, QString& device,QString&  host, FormatOption f) const
{
    QString  s(src);
    if(s.count('/') > 3)
        host = s.section('/', 0, 0);
    if(f <= FormatShort && s.count('/') > 3) // remove host:PORT/ so that src is not too long
        s.replace(0, s.indexOf('/', 0) + 1, "");

    if(s.count('/') > 1) {
        point = s.section('/', -1);
        device = s.section('/', 0, 2);
    }
}

QString DataMsgFormatter::timeRepr(const QDateTime &dt) const
{
    QString tr; // time repr
    QTime midnight = QTime(23, 59, 59);
    QDateTime today_23_59 = QDateTime::currentDateTime();
    today_23_59.setTime(midnight);
    QDateTime today_midnite = QDateTime::currentDateTime();
    today_midnite.setTime(QTime(0, 0));
    QDateTime yesterday_midnite = today_midnite.addDays(-1);
    QString date, time = dt.time().toString("hh:mm:ss");

    if(dt > today_23_59)
        date = "tomorrow";
    else if(dt >= today_midnite)
        date = "today";
    else if(dt >= yesterday_midnite)
        date = "yesterday";
    else
        date = dt.date().toString("yyyy.MM.dd");
    tr = date + " " + time;
    return tr;
}

QString DataMsgFormatter::getVectorInfo(const CuVariant &v)
{
    QString s;
    std::vector<double> vd;
    v.toVector<double>(vd);
    auto minmax = std::minmax_element(vd.begin(),vd.end());
    s += QString("vector size: <b>%1</b> min: <b>%2</b> max: <b>%3</b>").arg(vd.size())
            .arg(*minmax.first).arg(*minmax.second);
    return s;
}


