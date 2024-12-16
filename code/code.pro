QT       += core gui sql

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets printsupport

CONFIG += c++11

# You can make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

SOURCES += \
    databasemanager.cpp \
    main.cpp \
    mainwindow.cpp\
    neureset.cpp\
    agent.cpp \
    qcustomplot.cpp \
    siteinfo.cpp

HEADERS += \
    databasemanager.h \
    defs.h \
    mainwindow.h\
    neureset.h\
    agent.h\
    qcustomplot.h\
    defs.h \
    siteinfo.h

FORMS += \
    mainwindow.ui

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target

RESOURCES += \
    rsc.qrc

DISTFILES +=
