include(../../cumbia-telegram.pri)

# for qwt!
QT -= gui

QT += network sql script

QT -= QT_NO_DEBUG_OUTPUT

CONFIG+=link_pkgconfig

CONFIG += debug

TARGET = ../../bin/cumbia-telegram

PKGCONFIG += tango
#PKGCONFIG += x11

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

# ../.. for cumbia-telegram-defs.h
#
INCLUDEPATH += ../lib .. modules ../..

# You can also make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
# You can also select to disable deprecated APIs only up to a certain version of Qt.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

SOURCES += \
        main.cpp \
    cubotserver.cpp \
    cubotlistener.cpp \
    cubotsender.cpp \
    auth.cpp \
    botcontrolserver.cpp \
    botstats.cpp \
    modules/monitorhelper.cpp \
   # tbotmsgdecoder.cpp \
    modules/botmonitor_msgdecoder.cpp \
    modules/alias_mod.cpp \
    cubotserverevents.cpp \
    modules/botmonitor_mod.cpp \
    modules/botreader_mod.cpp \
    modules/botwriter_mod.cpp \
    modules/moduleutils.cpp \
    modules/botmonitor_mod_msgformatter.cpp \
    modules/host_mod.cpp \
    modules/help_mod.cpp \
    cubotmsgtracker.cpp \
    modules/settings_mod.cpp

HEADERS += \
    ../cumbia-telegram-doc.h \
    ../../cumbia-telegram-defs.h \
    cubotserver.h \
    cubotlistener.h \
 #   tbotmsgdecoder.h \
    cubotsender.h \
    auth.h \
    botcontrolserver.h \
    botstats.h \
    cubotserverevents.h \
    cubotmsgtracker.h \
    modules/monitorhelper.h \
    modules/botmonitor_msgdecoder.h \
    modules/alias_mod.h \
    modules/botmonitor_mod.h \
    modules/botreader_mod.h \
    modules/botwriter_mod.h \
    modules/moduleutils.h \
    modules/botmonitor_mod_msgformatter.h \
    modules/host_mod.h \
    modules/help_mod.h \
    modules/settings_mod.h

RESOURCES += \
    cumbia-telegram.qrc

DISTFILES += \
    res/help.html \
    res/help_monitor.html \
    res/help_search.html \
    res/help_host.html \
    CumbiaBot_elettra_token.txt \
    res/help_alias.html


message ("install root $${INSTALL_ROOT}")

inst.files = $${TARGET}
inst.path = $${INSTALL_ROOT}/bin

doc.commands = \
doxygen \
Doxyfile;
doc.files = doc/*
doc.path = $${DOCDIR}/cumbia-telegram-app

QMAKE_EXTRA_TARGETS += doc

LIBS += -L../lib  -lcumbia-telegram -L$${INSTALL_ROOT}/lib

INSTALLS += inst doc
