include(/usr/local/cumbia-libs/include/qumbia-tango-controls/qumbia-tango-controls.pri)
include(/usr/local/cumbia-libs/include/qumbia-epics-controls/qumbia-epics-controls.pri)
include(/usr/local/cumbia-libs/include/cumbia-qtcontrols/cumbia-qtcontrols.pri)

INSTALL_ROOT = /usr/local/cumbia-telegram

CUMBIA_TELEGRAM_PLUGIN_PATH=$${INSTALL_ROOT}/lib/plugins

DEFINES += CUMBIA_TELEGRAM_PLUGIN_DIR=\"\\\"$${CUMBIA_TELEGRAM_PLUGIN_PATH}\\\"\" \

# for qwt!
QT += gui

QT += network sql script

QT -= QT_NO_DEBUG_OUTPUT

CONFIG+=link_pkgconfig

CONFIG += debug

TARGET = bin/cumbia-telegram

PKGCONFIG += tango
PKGCONFIG += x11

packagesExist(cumbia) {
    PKGCONFIG += cumbia
}

packagesExist(cumbia-tango) {
    PKGCONFIG += cumbia-tango
}

CONFIG += c++11 console
CONFIG -= app_bundle

# The following define makes your compiler emit warnings if you use
# any Qt feature that has been marked deprecated (the exact warnings
# depend on your compiler). Please consult the documentation of the
# deprecated API in order to know how to port your code away from it.
DEFINES += QT_DEPRECATED_WARNINGS


INCLUDEPATH += src src/lib src/modules

# You can also make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
# You can also select to disable deprecated APIs only up to a certain version of Qt.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

SOURCES += \
        src/main.cpp \
    src/cubotserver.cpp \
    src/cubotlistener.cpp \
    src/botreader.cpp \
    src/cubotsender.cpp \
    src/cumbiasupervisor.cpp \
    src/msgformatter.cpp \
    src/botreadquality.cpp \
    src/auth.cpp \
    src/botcontrolserver.cpp \
    src/botstats.cpp \
    src/modules/botplot.cpp \
    src/modules/botplotgenerator.cpp \
    src/modules/monitorhelper.cpp \
    src/tbotmsgdecoder.cpp \
    src/modules/botmonitor_msgdecoder.cpp \
    src/modules/alias_mod.cpp \
    src/cubotserverevents.cpp \
    src/modules/botmonitor_mod.cpp \
    src/modules/botreader_mod.cpp

HEADERS += \
    src/cubotserver.h \
    src/cubotlistener.h \
    src/tbotmsgdecoder.h \ 
	src/botreader.h \
    src/cubotsender.h \
    src/cumbiasupervisor.h \
    src/msgformatter.h \
    src/botreadquality.h \
    src/auth.h \
    cumbia-telegram-defs.h \
    src/botcontrolserver.h \
    src/botstats.h \
    src/modules/botplot.h \
    src/modules/botplotgenerator.h \
    src/modules/monitorhelper.h \
    src/modules/botmonitor_msgdecoder.h \
    src/modules/alias_mod.h \
    src/cubotserverevents.h \
    src/modules/botmonitor_mod.h \
    src/modules/botreader_mod.h

RESOURCES += \
    cumbia-telegram.qrc

DISTFILES += \
    res/help.html \
    res/help_monitor.html \
    res/help_alerts.html \
    res/help_search.html \
    res/help_host.html \
    CumbiaBot_elettra_token.txt


inc.files = \
    src/tbotmsgdecoder.h \
    src/cubotplugininterface.h \
    src/cubotmodule.h \
    src/cubotvolatileoperation.h \
    src/cubotvolatileoperations.h

message ("install root $${INSTALL_ROOT}")

inc.path = $${INSTALL_ROOT}/include

inst.files = $${TARGET}
inst.path = $${INSTALL_ROOT}/bin

LIBS += -L$${INSTALL_ROOT}/lib -lcumbia-telegram

INSTALLS += inc inst
