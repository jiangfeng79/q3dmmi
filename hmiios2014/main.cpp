#include "hmiios2014.h"
#include <QtWidgets/QApplication>
#include <QSplashScreen>
#include "TSDWindow.h"
#include <QStyleFactory>
#include <QSurfaceFormat>

hmiios2014* G_P_MAINWINDOW;
int main(int argc, char* argv[])
{
    QSurfaceFormat format;
    format.setRenderableType(QSurfaceFormat::OpenGL);
    format.setProfile(QSurfaceFormat::CoreProfile);
    format.setVersion(3, 3);
    format.setSamples(4);
    format.setDepthBufferSize(24);
    format.setStencilBufferSize(8);
    QSurfaceFormat::setDefaultFormat(format);

    QApplication a(argc, argv);
    hmiios2014 w;
    w.resize(1280, 800);
    w.show();
    return a.exec();
}
