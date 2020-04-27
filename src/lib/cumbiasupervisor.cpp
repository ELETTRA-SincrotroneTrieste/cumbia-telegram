#include "cumbiasupervisor.h"
#include <cumbiapool.h>
#include <cuthreadfactoryimpl.h>
#include <qthreadseventbridgefactory.h>

#ifdef HAS_QUMBIA_TANGO_CONTROLS // set by cumbia-telegram.pri
#include <cumbiatango.h>
#include <cutango-world.h>
#include <cutcontrolsreader.h>
#include <cutcontrolswriter.h>
#endif

#ifdef HAS_QUMBIA_EPICS_CONTROLS // set by cumbia-telegram.pri
#include <cumbiaepics.h>
#include <cuepcontrolsreader.h>
#include <cuepcontrolswriter.h>
#include <cuepics-world.h>
#include <cuepreadoptions.h>
#endif

#include <cupluginloader.h>
#include <cuformulaplugininterface.h>
#include <QPluginLoader>

#include <QStringList>

CumbiaSupervisor::CumbiaSupervisor()
{
    cu_pool = nullptr;
}

void CumbiaSupervisor::setup()
{
    if(cu_pool) {
        perr("CumbiaSupervisor.setup: cumbia already setup");
    }
    else {
        cu_pool = new CumbiaPool();
        // setup Cumbia pool and register cumbia implementations for tango and epics
        // the following patterns have been taken from the older implementation of CuFormulaParseHelper
        // CuFormulaParseHelper newer version makes use of the patterns defined in CuControlsFactoryPool
        // from this method, so that duplicate patterns for matching sources are avoided
        //
        //    const char* tg_host_pattern_p = "(?:[A-Za-z0-9\\.\\-_]+:[\\d]+/){0,1}";
        //    const char* tg_pattern_p = "[A-Za-z0-9\\.\\-_:]+";
        //    const char* ep_pattern = "[A-Za-z0-9\\.\\-_]+:[A-Za-z0-9\\.\\-_]+";

        // Tango attribute patterns
        // (?:[A-Za-z0-9]+\:\d+)/[A-Za-z0-9_\.\-]+/[A-Za-z0-9_\.\-]+/[A-Za-z0-9_\.\-]+/[A-Za-z0-9_\.\-]+
        const char *h_p = "(?:[A-Za-z0-9\\.\\-_]+\\:\\d+)"; // host pattern e.g. hokuto:20000
        const char *t_p = "[A-Za-z0-9_\\.\\-]+"; // t_p tango pattern
        const char *t_args = "(?:\\([A-Za-z0-9,_\\-\\.\\s]+\\)){0,1}";   // optional args, e.g. (0,100)
        QString a_p = QString("%1/%1/%1/%1%2").arg(t_p).arg(t_args); // a_p  attribute pattern
        QString c_p = QString("%1/%1/%1\\->%1%2").arg(t_p).arg(t_args); // c_p command pattern
        QString h_a_p = QString("%1/%2/%2/%2/%2%3").arg(h_p).arg(t_p).arg(t_args);
        QString h_c_p = QString("%1/%2/%2/%2\\->%2%3").arg(h_p).arg(t_p).arg(t_args); // host + command + args
        std::vector<std::string> tg_patterns;
        tg_patterns.push_back(h_a_p.toStdString());
        tg_patterns.push_back(a_p.toStdString());
        tg_patterns.push_back(c_p.toStdString());
        tg_patterns.push_back(h_c_p.toStdString());

#ifdef HAS_QUMBIA_TANGO_CONTROLS
        CumbiaTango* cuta = new CumbiaTango(new CuThreadFactoryImpl(), new QThreadsEventBridgeFactory());
        cu_pool->registerCumbiaImpl("tango", cuta);
        ctrl_factory_pool.registerImpl("tango", CuTWriterFactory());  // register Tango writer implementation
        ctrl_factory_pool.registerImpl("tango", CuTReaderFactory());  // register Tango reader implementation
        ctrl_factory_pool.setSrcPatterns("tango", tg_patterns);
        cu_pool->setSrcPatterns("tango", tg_patterns);
#endif

#ifdef HAS_QUMBIA_EPICS_CONTROLS
        // do not allow host:20000/sys/tg_test/1/double_scalar
        // force at least one letter after ":"
        // unescaped
        // [A-Za-z0-9_\-\.]+:[A-Za-z_]+[A-Za-z_0-9\-:]*
        std::string ep_pattern = std::string("[A-Za-z0-9_\\-\\.]+:[A-Za-z_]+[A-Za-z_0-9\\-:]*");
        std::vector<std::string> ep_patterns;
        ep_patterns.push_back(ep_pattern);
        CumbiaEpics* cuep = new CumbiaEpics(new CuThreadFactoryImpl(), new QThreadsEventBridgeFactory());
        cu_pool->registerCumbiaImpl("epics", cuep);
        // m_ctrl_factory_pool  is in this example a private member of type CuControlsFactoryPool
        ctrl_factory_pool.registerImpl("epics", CuEpReaderFactory());   // register EPICS reader implementation
        ctrl_factory_pool.registerImpl("epics", CuEpWriterFactory());   // register EPICS writer implementation
        ctrl_factory_pool.setSrcPatterns("epics", ep_patterns);
        cu_pool->setSrcPatterns("epics", ep_patterns);
#endif

        // formulas

        CuPluginLoader plulo;
        // empty default path: CuPluginLoader will use CUMBIA_QTCONTROLS_PLUGIN_DIR
        // defined in cumbia-qtcontrols.pri
        QString plupath = plulo.getPluginAbsoluteFilePath(QString(), "cuformula-plugin.so");
        QPluginLoader pluginLoader(plupath);
        QObject *plugin = pluginLoader.instance();
        if (plugin){
            m_formulaPlu = qobject_cast<CuFormulaPluginI *>(plugin);
            if(!m_formulaPlu)
                perr("Failed to load formula plugin");
            else {
                printf("\e[1;32m* \e[0minitializing formula plugin...");
                m_formulaPlu->initialize(cu_pool, this->ctrl_factory_pool);
                printf("\t[\e[1;32mdone\e[0m]\n");
            }
        }
        else {
            perr("failed to load plugin loader under path %s: %s", qstoc(plupath), qstoc(pluginLoader.errorString()));
        }

    }
}

void CumbiaSupervisor::dispose()
{
    if(cu_pool) {
        Cumbia *cumb =  cu_pool->get("tango");
        printf("deleting cumbia tango %p in dispose()\n", cumb);
        if(cumb) delete cumb;
        cumb =  cu_pool->get("epics");
        if(cumb) delete cumb;
    }
    delete cu_pool;
    cu_pool = nullptr;
}

CuFormulaPluginI *CumbiaSupervisor::formulaPlugin() const
{
    return  m_formulaPlu;
}

