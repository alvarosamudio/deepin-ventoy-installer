#-------------------------------------------------
#
# Project created by QtCreator 2019-06-30T12:53:03
#
#-------------------------------------------------

QT       += core gui

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets network

TARGET = dvtoyinst
TEMPLATE = app

# The following define makes your compiler emit warnings if you use
# any feature of Qt which as been marked as deprecated (the exact warnings
# depend on your compiler). Please consult the documentation of the
# deprecated API in order to know how to port your code away from it.
DEFINES += QT_DEPRECATED_WARNINGS

# You can also make your code fail to compile if you use deprecated APIs.
# In order to do so, uncomment the following line.
# You can also select to disable deprecated APIs only up to a certain version of Qt.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0


SOURCES += main.cpp\
        mainwindow.cpp \
    widget.cpp \
    dvidroparea.cpp \
    utilities.cpp

HEADERS  += mainwindow.h \
    widget.h \
    dvidroparea.h \
    utilities.h

CONFIG += link_pkgconfig
PKGCONFIG += dtkwidget

CONFIG += c++11

FORMS += \
    widget.ui

RESOURCES += \
    res.qrc

TRANSLATIONS += translations/dvtoyinst_zh.ts \
                translations/dvtoyinst_es.ts \
                translations/dvtoyinst_pt.ts \
                translations/dvtoyinst_it.ts \
                translations/dvtoyinst_fr.ts

# Installation
target.path = /usr/bin
desktop.path = /usr/share/applications
desktop.files = dvtoyinst.desktop
icon.path = /usr/share/icons/hicolor/512x512/apps
icon.files = res/dvtoyinst.png

qm.path = /usr/share/deepin-ventoy-installer/translations
qm.files = translations/dvtoyinst_zh.qm \
           translations/dvtoyinst_es.qm \
           translations/dvtoyinst_pt.qm \
           translations/dvtoyinst_it.qm \
           translations/dvtoyinst_fr.qm

INSTALLS += target desktop icon qm
