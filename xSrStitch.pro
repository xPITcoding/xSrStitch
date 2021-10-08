QT       += core gui charts concurrent

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += c++11

# You can make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

SOURCES += \
    alglibinternal.cpp \
    alglibmisc.cpp \
    ap.cpp \
    baumwidget.cpp \
    chartdialog.cpp \
    fasttransforms.cpp \
    fmdialog.cpp \
    linalg.cpp \
    main.cpp \
    predialog.cpp \
    selectdirdialog.cpp \
    tools.cpp \
    xsrstitch.cpp

HEADERS += \
    alglibinternal.h \
    alglibmisc.h \
    ap.h \
    baumwidget.h \
    chartdialog.h \
    fasttransforms.h \
    fmdialog.h \
    linalg.h \
    predialog.h \
    selectdirdialog.h \
    stdafx.h \
    tools.h \
    xsrstitch.h

FORMS += \
    chartdialog.ui \
    fmdialog.ui \
    predialog.ui \
    selectdirdialog.ui \
    xsrstitch.ui

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target


RESOURCES += \
    data.qrc

win32:CONFIG(release, debug|release): LIBS += -L$$PWD/../../CODING_SOFTWARE/tiff-4.3.0/build/libtiff/release/ -ltiff
else:win32:CONFIG(debug, debug|release): LIBS += -L$$PWD/../../CODING_SOFTWARE/tiff-4.3.0/build/libtiff/debug/ -ltiffd
else:unix: LIBS += -L$$PWD/../../CODING_SOFTWARE/tiff-4.3.0/build/libtiff/ -ltiffd

INCLUDEPATH += $$PWD/../../CODING_SOFTWARE/tiff-4.3.0/libtiff
DEPENDPATH += $$PWD/../../CODING_SOFTWARE/tiff-4.3.0/libtiff

INCLUDEPATH += $$PWD/../../CODING_SOFTWARE/tiff-4.3.0/build/libtiff

INCLUDEPATH += "C:\Program Files (x86)\Windows Kits\10\Include\10.0.19041.0\ucrt"
