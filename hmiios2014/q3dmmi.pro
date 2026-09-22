QT += core gui widgets network opengl
greaterThan(QT_MAJOR_VERSION, 5): QT += openglwidgets

CONFIG += qt c++20 warn_on

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
    busArrivalWidget.cpp \
    busLayerParser.cpp \
    dbfReader.cpp \
    flightLayerParser.cpp \
    hmiios2014.cpp \
    layerParser.cpp \
    main.cpp \
    mapFilterWidget.cpp \
    shpReader.cpp \
    MapLayer.cpp \
    roadGraph.cpp \
    ../build/generated/appConfig/appConfig.cpp \
    ../build/generated/appConfig/appConfigView.cpp

HEADERS += \
    OpenglWindow.h \
    TSDWindow.h \
    busArrivalWidget.h \
    busLayerParser.h \
    busRoute.h \
    busTracker.h \
    dbfReader.h \
    flightLayerParser.h \
    flightTracker.h \
    geoTransform.h \
    hmiios2014.h \
    layerGeometry.h \
    layerParser.h \
    mapFilterWidget.h \
    mrt.h \
    shpReader.h \
    MapLayer.h \
    WorkerEntry.h \
    roadGraph.h \
    ../build/generated/appConfig/appConfig.h \
    ../build/generated/appConfig/appConfigView.h

FORMS += \
    hmiios2014.ui \
    mapFilter.ui \
    busArrival.ui

TRANSLATIONS = hmiios2014_en.ts \
               hmiios2014_zh.ts

INCLUDEPATH += $$PWD/../../shapelib-1.6.3/ $$PWD/../build/generated/appConfig

APP_CONFIG_DIR = $$clean_path($$PWD/../build/generated/appConfig)
APP_CONFIG_SCHEMA = $$clean_path($$PWD/config.json)
APP_CONFIG_EVENTS = $$clean_path($$PWD/appConfigEvents.cpp)
APP_CONFIG_HEADER = $$APP_CONFIG_DIR/appConfig.h

win32 {
    !exists($$APP_CONFIG_HEADER) {
        # AppVeyor / Visual Studio 2022 builds use cmd.exe, not MSYS2 bash.
        # Let CMake resolve the Visual Studio configuration-specific executable path.
        system("cd /d \"$$PWD/..\" && cmake -S . -B build && cmake --build build --target generateAppConfig --config Release")
    }
    PRE_TARGETDEPS += $$APP_CONFIG_HEADER
}

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target

win32 {
    SHAPELIB_DIR = $$PWD/../../shapelib-1.6.3
    exists($$SHAPELIB_DIR/shapefil.h) {
        INCLUDEPATH += $$SHAPELIB_DIR
        exists($$SHAPELIB_DIR/libshp.a): LIBS += -L$$SHAPELIB_DIR -lshp
        else:exists($$SHAPELIB_DIR/shapelib.lib): LIBS += -L$$SHAPELIB_DIR -lshapelib
        else:exists($$SHAPELIB_DIR/shp.lib): LIBS += -L$$SHAPELIB_DIR -lshp
    }
    LIBS += -lopengl32
}
else:unix {
    exists(/usr/local/include/shapelib/shapefil.h) {
        INCLUDEPATH += /usr/local/include/shapelib
    }
    exists(/usr/local/lib/libshp.so): LIBS += -L/usr/local/lib -lshp
}

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
    vshader.glsl \
    bgfshader.glsl \
    gshader.glsl

RESOURCES += \
    hmiios2014.qrc
