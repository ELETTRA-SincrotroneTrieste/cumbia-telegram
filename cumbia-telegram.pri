#
#                              ###################################################################
#
#                                                         customization section start
#

INSTALL_ROOT = /usr/local/cumbia-telegram
CUMBIA_QTCONTROLS_ROOT=/usr/local/cumbia-libs
#
SHAREDIR = $${INSTALL_ROOT}/share
DOCDIR = $${SHAREDIR}/doc

# where to look for telegram plugins
CUMBIA_TELEGRAM_PLUGIN_PATH=$${INSTALL_ROOT}/lib/plugins
DEFINES += CUMBIA_TELEGRAM_PLUGIN_DIR=\"\\\"$${CUMBIA_TELEGRAM_PLUGIN_PATH}\\\"\"


# Qwt is used to generate plot images
# If QWT_HOME is left empty, qwt configuration is taken from pkg-config
# If qwt was built without pkg-config support, please provide a valid
# QWT_HOME pointing to the installation root of the qwt libraries
# Below, the script tries to find qwt installation under /usr/local
#
#
QWT_HOME=


MOC_DIR = moc
OBJECTS_DIR = obj

# DEPENDENCIES
#
# struggle to find Qwt somewhere.
# Hope to find pkgconfig.
# debian/ubuntu's qwt installation and naming are a mess
#
isEmpty(QWT_HOME) {
    exists(/usr/local/qwt-6.1.4) {
        QWT_HOME = /usr/local/qwt-6.1.4
    }
    exists(/usr/local/qwt-6.1.3) {
        QWT_HOME = /usr/local/qwt-6.1.3
    }
    exists(/usr/local/qwt-6.1.2) {
        QWT_HOME = /usr/local/qwt-6.1.2
    }
}

QWT_LIB = qwt

QWT_INCLUDES=$${QWT_HOME}/include

QWT_HOME_USR = /usr
QWT_INCLUDES_USR = $${QWT_HOME_USR}/include/qwt

packagesExist(qwt){
    PKGCONFIG += qwt
    QWT_PKGCONFIG = qwt
    message("cumbia-telegram-app.pro: using pkg-config to configure qwt includes and libraries")
}
else:packagesExist(Qt5Qwt6){
    PKGCONFIG += Qt5Qwt6
    QWT_PKGCONFIG = Qt5Qwt6
    message("cumbia-telegram-app.pro: using pkg-config to configure qwt includes and libraries (Qt5Qwt6)")
} else {
    warning("cumbia-telegram-app.pro: no pkg-config file found")
    warning("cumbia-telegram-app.pro: export PKG_CONFIG_PATH=/usr/path/to/qwt/lib/pkgconfig if you want to enable pkg-config for qwt")
    warning("cumbia-telegram-app.pro: if you build and install qwt from sources, be sure to uncomment/enable ")
    warning("cumbia-telegram-app.pro: QWT_CONFIG     += QwtPkgConfig in qwtconfig.pri qwt project configuration file")
}


#
## mandatory
CONFIG += link_pkgconfig

PKGCONFIG += cumbia \
        cumbia-qtcontrols

## tango support ?
packagesExist(cumbia-tango) {
    PKGCONFIG += cumbia-tango
}
packagesExist(qumbia-tango-controls) {
        PKGCONFIG += qumbia-tango-controls
        DEFINES += HAS_QUMBIA_TANGO_CONTROLS=1
        message("tango support enabled")
}

## epics support?
packagesExist(epics-base) {
    PKGCONFIG += epics-base
}
else {
    message("package epics-base not found")
}
packagesExist(epics-base-linux-x86_64) {
    PKGCONFIG += epics-base-linux-x86_64
}
else {
    message("package epics-base-linux-x86_64 not found")
}

packagesExist(cumbia-epics) {
        PKGCONFIG += cumbia-epics
}
packagesExist(cumbia-epics,epics-base,qumbia-epics-controls) {
        PKGCONFIG += qumbia-epics-controls
        DEFINES += HAS_QUMBIA_EPICS_CONTROLS=1
        message("epics support enabled")
}
else {
    message("epics support disabled")
}



QT -= gui widgets
