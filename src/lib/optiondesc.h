#ifndef OPTIONDESC_H
#define OPTIONDESC_H

#include <QString>
#include <QMap>

/*!
 * \brief The OptionDesc class describes the options exported by a module
 */
class OptionDesc
{
public:
    OptionDesc(const QString& _key);
    OptionDesc();
    bool setCurrentValueFromCmd(const QString& s);
    bool matches(const QString& s) const;

    QString key, current_set;
    QMap<QString, QString> options;

    bool isValid() const;
    bool valueSet() const;
};

#endif // OPTIONDESC_H
