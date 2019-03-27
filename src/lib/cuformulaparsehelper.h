#ifndef CUFORMULAPARSEHELPER_H
#define CUFORMULAPARSEHELPER_H

#include <QMultiMap>
#include <cucontrolsfactorypool.h>

class CuFormulaParseHelper
{
public:
    CuFormulaParseHelper(const CuControlsFactoryPool &fap);

    bool isNormalizedForm(const QString& f, const QString &norm_pattern) const;

    QString toNormalizedForm(const QString& f) const;

    QString injectHostIfNeeded(const QString& host, const QString& src, bool *needs_host);

    QStringList sources(const QString& formula) const;

    QStringList srcPatterns() const;

    void addSrcPattern(const QString &domain, const QString& p);

    bool sourceMatch(const QString &src, const QString &pattern) const;

    bool identityFunction(const QString &expression) const;

    bool needsHost(const QString& src) const;

private:
    QMultiMap<std::string, std::string> m_src_patterns;

    QString m_buildSrcPattern() const;

    CuControlsFactoryPool m_fap;
};

#endif // CUFORMULAPARSEHELPER_H
