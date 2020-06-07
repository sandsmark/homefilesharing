#-------------------------------------------------
#
# Project created by QtCreator 2018-04-22T21:08:39
#
#-------------------------------------------------

QT       += core gui network widgets

TARGET = homefilesharing
TEMPLATE = app

LIBS += -lcrypto

linux {
    LIBS +=  -lX11 -lXtst
    QT += x11extras
}

# The following define makes your compiler emit warnings if you use
# any feature of Qt which has been marked as deprecated (the exact warnings
# depend on your compiler). Please consult the documentation of the
# deprecated API in order to know how to port your code away from it.
DEFINES += QT_DEPRECATED_WARNINGS

# You can also make your code fail to compile if you use deprecated APIs.
# In order to do so, uncomment the following line.
# You can also select to disable deprecated APIs only up to a certain version of Qt.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0


RESOURCES += icons/icons.qrc

SOURCES += \
        main.cpp \
        machinelist.cpp \
    connection.cpp \
    connectionhandler.cpp \
    randomart.cpp \
    connectdialog.cpp \
    mainwindow.cpp \
    transferdialog.cpp

HEADERS += \
        machinelist.h \
    connection.h \
    connectionhandler.h \
    common.h \
    randomart.h \
    connectdialog.h \
    mainwindow.h \
    host.h \
    transferdialog.h
