QT       += core widgets gui opengl

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += qt c++11

# The following define makes your compiler emit warnings if you use
# any Qt feature that has been marked deprecated (the exact warnings
# depend on your compiler). Please consult the documentation of the
# deprecated API in order to know how to port your code away from it.
DEFINES += QT_DEPRECATED_WARNINGS

# You can also make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
# You can also select to disable deprecated APIs only up to a certain version of Qt.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0
TARGET = q3dmmi
TEMPLATE = app

SOURCES += \
    OpenglWindow.cpp \
    TSDWindow.cpp \
    dbfReader.cpp \
    hmiios2014.cpp \
    main.cpp \
    mapFilterWidget.cpp \
    shpReader.cpp

HEADERS += \
    OpenglWindow.h \
    TSDWindow.h \
    dbfReader.h \
    hmiios2014.h \
    mapFilterWidget.h \
    mrt.h \
    shpReader.h

FORMS += \
    hmiios2014.ui \
    mapFilter.ui

win32:INCLUDEPATH += $$PWD/../../shapelib-1.3.0/
else: INCLUDEPATH += $$PWD/../../shapelib-1.5.0/

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target

win32:CONFIG(release, debug|release): LIBS += -L$$PWD/../../shapelib-1.3.0/ -lshapelib opengl32.lib
else:win32:CONFIG(debug, debug|release): LIBS += -L$$PWD/../../shapelib-1.3.0/ -lshapelib opengl32.lib
else:unix: LIBS += -L/usr/local/lib -lshp

DISTFILES += \
    fshader.glsl \
    hmiios2014.aps \
    hmiios2014.exe \
    hmiios2014.ico \
    hmiios2014.qph \
    hmiios2014_en.qm \
    hmiios2014_en.ts \
    hmiios2014_zh.qm \
    hmiios2014_zh.ts \
    q3dmmi.pro.user \
    vshader.glsl

RESOURCES += \
    hmiios2014.qrc
