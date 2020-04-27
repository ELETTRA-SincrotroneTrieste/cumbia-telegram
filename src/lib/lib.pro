include(../../cumbia-telegram.pri)

# for qwt!
QT -= gui

QT += network sql script

QT -= QT_NO_DEBUG_OUTPUT

CONFIG+=link_pkgconfig

CONFIG += debug

TEMPLATE = lib

PKGCONFIG += tango
PKGCONFIG -= x11

VERSION_HEX = 0x010000
VERSION = 1.0.0
VER_MAJ = 1
VER_MIN = 0
VER_PAT = 0

TARGET = cumbia-telegram

# The following define makes your compiler emit warnings if you use
# any Qt feature that has been marked deprecated (the exact warnings
# depend on your compiler). Please consult the documentation of the
# deprecated API in order to know how to port your code away from it.
DEFINES += QT_DEPRECATED_WARNINGS


# You can also make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
# You can also select to disable deprecated APIs only up to a certain version of Qt.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0


UNIX:INCLUDEPATH += .
# to find cumbia-telegram-defs.h
INCLUDEPATH += ../.. ..

SOURCES += \
    botdb.cpp \
    optiondesc.cpp \
    tbotmsg.cpp \
    formulahelper.cpp \
    botconfig.cpp \
    cuformulaparsehelper.cpp \
    cubotvolatileoperation.cpp \
    cubotvolatileoperations.cpp \
    historyentry.cpp \
    cubotmodule.cpp \
    botreadquality.cpp \
    botreader.cpp \
    botwriter.cpp \
    datamsgformatter.cpp \
    cumbiasupervisor.cpp \
    botplot.cpp \
    botplotgenerator.cpp \
    generic_msgformatter.cpp \
    hostentry.cpp

HEADERS += \
# this must be installed also
    ../../cumbia-telegram-defs.h \
    cubotmodule.h \
    cubotplugininterface.h \
    botdb.h \
    optiondesc.h \
    tbotmsg.h \
    formulahelper.h \
    botconfig.h \
    cuformulaparsehelper.h \
    cubotvolatileoperation.h \
    cubotvolatileoperations.h \
    historyentry.h \
    botreadquality.h \
    botreader.h \
    botwriter.h \
    datamsgformatter.h \
    cumbiasupervisor.h \
    botplot.h \
    botplotgenerator.h \
    generic_msgformatter.h \
    hostentry.h

inc.files = $${HEADERS}


INCLUDEPATH += $${QWT_INCLUDES} \
$${QWT_INCLUDES_USR}

message ("install root $${INSTALL_ROOT}")
doc.commands = \
doxygen \
Doxyfile;

doc.files = doc/*

QMAKE_EXTRA_TARGETS += doc


inc.path = $${INSTALL_ROOT}/include
target.path = $${INSTALL_ROOT}/lib
doc.path = $${DOCDIR}/cumbia-telegram-lib

INSTALLS += inc target doc
