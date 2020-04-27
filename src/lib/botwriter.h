#ifndef BotWriter_H
#define BotWriter_H

#include <QObject>
#include <cudatalistener.h>
#include <cucontexti.h>
#include <cudata.h>

#include <QString>
class QContextMenuEvent;

class BotWriterPrivate;
class Cumbia;
class CumbiaPool;
class CuControlsWriterFactoryI;
class CuControlsFactoryPool;
class CuContext;
class CuLinkStats;

/*! \brief 
 *
 * 
 *
*/
class BotWriter : public QObject, public CuDataListener, public CuContextI
{
    Q_OBJECT
public:
    BotWriter(QObject *w, CumbiaPool *cumbia_pool, const CuControlsFactoryPool &fpool, const QString& host);

    virtual ~BotWriter();

    QString target() const;
    CuContext *getContext() const;
    QString host() const;
    bool needsHost() const;
    QString getAppliedHost() const;

public slots:
    void setTarget(const QString& target);

    void write();

signals:
    void newData(const CuData&);

private:
    BotWriterPrivate *d;

    void m_init();
public:
    void onUpdate(const CuData &d);
};

#endif // QUTLABEL_H
