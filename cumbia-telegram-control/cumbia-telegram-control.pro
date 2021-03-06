QT -= gui

QT += network sql

TARGET = bin/cumbia-telegram-control

CONFIG += c++11 console
CONFIG -= app_bundle

CONFIG += debug

# The following define makes your compiler emit warnings if you use
# any Qt feature that has been marked deprecated (the exact warnings
# depend on your compiler). Please consult the documentation of the
# deprecated API in order to know how to port your code away from it.
DEFINES += QT_DEPRECATED_WARNINGS

# You can also make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
# You can also select to disable deprecated APIs only up to a certain version of Qt.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

SOURCES += \
        src/main.cpp \
    src/cutelegramctrl.cpp \
    src/dbctrl.cpp

# Default rules for deployment.

!isEmpty(INSTALL_ROOT) {
	target.path=$${INSTALL_ROOT}/bin
	INSTALLS += target
} else {

message("-")
message("cumbia-telegram-control: if you want to install the application please")
message("specify INSTALL_ROOT. The binary will be installed under $INSTALL_ROOT/bin")
message("-")
}

HEADERS += \
    src/cutelegramctrl.h \
    src/dbctrl.h
    ../cumbia-telegram-defs.h

INCLUDEPATH += src
