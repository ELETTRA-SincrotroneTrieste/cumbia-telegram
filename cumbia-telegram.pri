#

#
#                              ###################################################################
#
#                                                         customization section start
#
isEmpty(INSTALL_ROOT) {
	INSTALL_ROOT = /usr/local/cumbia-telegram
}
isEmpty(CUMBIA_ROOT) {
	CUMBIA_ROOT=/usr/local/cumbia-libs
}
#
SHAREDIR = $${INSTALL_ROOT}/share
DOCDIR = $${SHAREDIR}/doc

# where to look for telegram plugins
CUMBIA_TELEGRAM_PLUGIN_PATH=$${INSTALL_ROOT}/lib/plugins
DEFINES += CUMBIA_TELEGRAM_PLUGIN_DIR=\"\\\"$${CUMBIA_TELEGRAM_PLUGIN_PATH}\\\"\"



MOC_DIR = moc
OBJECTS_DIR = obj

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

DISTFILES += \
    $$PWD/src/d3plots/index.php
