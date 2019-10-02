#include "cuformulaparsehelper.h"
#include <QRegularExpression>
#include <QRegularExpressionMatchIterator>
#include <QtDebug>
#include <cumacros.h>

CuFormulaParseHelper::CuFormulaParseHelper(const CuControlsFactoryPool &fap)
{
    // Patterns are no more defined statically in this class but are rather obtained
    // from the reference to CuControlsFactoryPool
    //
    // old pattern definitions (please compare them with the definitions in CumbiaSupervisor
    // in case of problems.
    //
//    const char* tg_host_pattern_p = "(?:[A-Za-z0-9\\.\\-_]+:[\\d]+/){0,1}";
//    const char* tg_pattern_p = "[A-Za-z0-9\\.\\-_:]+";
//    const char* ep_pattern = "[A-Za-z0-9\\.\\-_]+:[A-Za-z0-9\\.\\-_]+";
//    QString tg_att_pattern = QString("%1%2/%2/%2/%2").arg(tg_host_pattern_p).arg(tg_pattern_p);
//    m_src_patterns.insertMulti("tango",  tg_att_pattern.toStdString());
//    m_src_patterns.insertMulti("epics",  ep_pattern);

    //
    // source patterns for the available domains are obtained by CuControlsFactoryPool
    //
    m_fap = fap;
    std::vector<std::string> domains = fap.getSrcPatternDomains();
    foreach(std::string domain, domains) {
        if(domain != "formula")
            foreach(std::string patt, fap.getSrcPatterns(domain))
                m_src_patterns.insertMulti(domain, patt);
    }
}

bool CuFormulaParseHelper::isNormalizedForm(const QString &f, const QString& norm_pattern) const
{
    QRegularExpression re(norm_pattern);
    QString e(f);
    e.remove("formula://");
    QRegularExpressionMatch match = re.match(e);
    printf("\e[1;35mCuFormulaParseHelper::isNormalizedForm %d pattern is \"%s\" %s\e[0m\n", match.hasMatch() , qstoc(norm_pattern),
           qstoc(f));
    return match.hasMatch();
}

QString CuFormulaParseHelper::toNormalizedForm(const QString &f) const
{
    QString norm(f);
    norm.remove("formula://");
    QString pattern = m_buildSrcPattern();
    QRegularExpression re(pattern);
    QRegularExpressionMatchIterator i = re.globalMatch(f);
    QStringList srcs;
    while(i.hasNext()) {
        QRegularExpressionMatch match = i.next();
        QString src = match.captured(1);
        if(!srcs.contains(src))
            srcs << src;
    }

    if(srcs.size() > 0) {
        QString function_decl;
        QString function_body(f);
        QString params;
        norm = "formula://{" + srcs.join(",") + "}";
        // now build function arguments
        char c = 'a', maxc = 'Z';
        for(char i = 0; i < srcs.size() && i < maxc ; i++) {
            c = c + static_cast<char>(i);
            i < srcs.size() - 1 ? params += QString("%1,").arg(c) : params += QString("%1").arg(c);
            function_body.replace(srcs[i], QString(c));
        }
        function_decl = QString("function (%1)").arg(params);
        function_body = QString( "{"
                                 "return %1"
                                 "}").arg(function_body);

        norm += function_decl + function_body;
        qDebug() << __PRETTY_FUNCTION__ << "input" << f << "pattern" << pattern
                 <<  "matched sources " << srcs <<  "normalized to"     << norm;
    }
    else {
        if(!norm.startsWith("formula://"))
            norm = "formula://" + norm;
    }

    return norm;
}

QString CuFormulaParseHelper::injectHostIfNeeded(const QString &host, const QString &src, bool *needs_host)
{
    QString s(src);
    *needs_host = false;

    if(!host.isEmpty()) {
        QString src_section;
        // (\{.+\}).+
        // example {test/device/1/double_scalar} function (a){return 2 + a}
        QString srclist_section_pattern = "(\\{.+\\}).+";
        QRegularExpression re_srcs_section(srclist_section_pattern);
        QRegularExpressionMatch match = re_srcs_section.match(src);
        if(match.hasMatch()) {
            const QString srclist = match.captured(1);
            // in the example above captured(1): {test/device/1/double_scalar}
            QRegularExpression re(m_buildSrcPattern());
            QRegularExpressionMatchIterator i = re.globalMatch(srclist);
            QStringList srcs;
            while(i.hasNext()) {
                QRegularExpressionMatch match = i.next();
                QString src = match.captured(1);
                if(needsHost(src)) {
                    src = host + "/" + src;
                    *needs_host = true;
                }
                if(!srcs.contains(src))
                    srcs << src;
            }
            QString s_inj = srcs.join(",");
            s.replace(srclist, "{" + s_inj + "}");
        }

    }
    qDebug() << __PRETTY_FUNCTION__ <<  "source with host (" << host << ") becomes " << s;
    return s;
}

/**
 * @brief CuFormulaParseHelper::sources extract sources from a formula
 * @param srcformula a source (also not in normal form {src1,src2}(@0 + @1) )
 * @return list of detected formulas according to the source patterns
 */
QStringList CuFormulaParseHelper::sources(const QString &srcformula) const
{
    QString pattern = m_buildSrcPattern();
    QRegularExpression re(pattern);
    QRegularExpressionMatchIterator i = re.globalMatch(srcformula);
    QStringList srcs;
    while(i.hasNext()) {
        QRegularExpressionMatch match = i.next();
        QString src = match.captured(1);
        if(!srcs.contains(src))
            srcs << src;
    }
    return srcs;
}

QStringList CuFormulaParseHelper::srcPatterns() const
{
    QStringList p;
    for(int i = 0; i < m_src_patterns.values().size(); i++)
        p << QString::fromStdString(m_src_patterns.values()[i]);
    return p;
}

void CuFormulaParseHelper::addSrcPattern(const QString& domain, const QString &p)
{
    m_src_patterns.insertMulti(domain.toStdString(), p.toStdString());
}

/**
 * @brief CuFormulaParseHelper::sourceMatch finds if one source matches the given patter (regexp match)
 * @param pattern the regexp pattern
 * @return true if *one of* the sources matches pattern
 *
 * \par Example
 * \code
 * reader->setSource("{test/device/1/long_scalar, test/device/2/double_scalar}(@1 + @2)");
 * CuFormulaParserHelper ph;
 * bool match = ph.sourceMatch(reader->source(), "double_scalar");
 * // match is true
 *
 * \endcode
 */
bool CuFormulaParseHelper::sourceMatch(const QString& src, const QString &pattern) const
{
    if(src == pattern)
        return true;
    QStringList srcs = sources(src);
    QRegularExpression re(pattern);
    foreach(QString s, srcs) {
        QRegularExpressionMatch match = re.match(s);
        if(match.hasMatch())
            return true;
    }
    return false;
}

bool CuFormulaParseHelper::identityFunction(const QString& expression) const
{
    // pattern for a function that does NOT imply formulas
    // function\s*\(a\)\s*\{\s*return\s+a\s*\}
    //
    QString pat = "function\\s*\\(a\\)\\s*\\{\\s*return\\s+a\\s*\\}";
    QRegularExpression re(pat);
    QRegularExpressionMatch match = re.match(expression);
    return match.hasMatch();
}

bool CuFormulaParseHelper::needsHost(const QString &src) const
{
    foreach(std::string key, m_src_patterns.keys()) {
        foreach(std::string pat, m_src_patterns.values(key)) {
            QRegularExpression re(pat.c_str());
            QRegularExpressionMatch match = re.match(src);
            if(match.hasMatch()) {
                 if(key == "tango") return true;
                 else if(key == "epics") return false;
            }
        }
    }
    return false;
}

QString CuFormulaParseHelper::m_buildSrcPattern() const
{
    if(m_src_patterns.isEmpty()) {
        printf("\e[1;31mCuFormulaParseHelper.m_buildSrcPattern: m_src_patterns map empty!!\e[0m\n\n\n");
        return QString();
    }
    int siz = m_src_patterns.values().size();
    QString pattern = "((?:";
    for(int i = 0; i < siz; i++) {
        pattern += QString::fromStdString(m_src_patterns.values()[i]);
        if(i < siz - 1)
            pattern += ")|(?:";
    }
    pattern += "))";
    return pattern;
}
