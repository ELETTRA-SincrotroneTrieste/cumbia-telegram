#include "optiondesc.h"
#include <QRegularExpression>

OptionDesc::OptionDesc(const QString &_key)
{
    key = _key;
}

OptionDesc::OptionDesc()
{

}

bool OptionDesc::setCurrentValueFromCmd(const QString &s)
{
    this->current_set.clear();
    QRegularExpression re("/{0,1}set_([a-zA-Z0-9_\\-]+)_([a-zA-Z0-9]+)");
    QRegularExpressionMatch match = re.match(s);
    if(!match.hasMatch() || match.capturedTexts().size() < 3)
        return false;
    if(match.captured(1) == this->key) {
        this->current_set = match.captured(2);
        return true;
    }
    return false;
}

bool OptionDesc::matches(const QString &txt) const
{
    QRegularExpression re("/{0,1}set_([a-zA-Z0-9_\\-]+)_([a-zA-Z0-9]+)");
    QRegularExpressionMatch match = re.match(txt);
    if(!match.hasMatch() || match.capturedTexts().size() < 3)
        return false;
    const QString k = match.captured(1);
    const QString val = match.captured(2);
    foreach(const QString& v, this->options.values()) {
        if(k == this->key  && v == val)
            return true;
    }
    return false;
}

bool OptionDesc::isValid() const
{
    return !key.isEmpty() && options.size() > 0;
}

bool OptionDesc::valueSet() const
{
    return !this->current_set.isEmpty();
}
