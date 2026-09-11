#include <QSplashScreen>
#include <QStyleFactory>
#include <QSurfaceFormat>
#include <QtWidgets/QApplication>

#include "TSDWindow.h"
#include "hmiios2014.h"

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
    // Set a modern CJK font stack
    QFont font("Microsoft YaHei UI", 10);
    
    // Disable hinting so DirectWrite uses subpixel antialiasing/grayscale instead of pixel snapping
    font.setHintingPreference(QFont::PreferNoHinting);
    font.setStyleStrategy(QFont::PreferAntialias);
    a.setFont(font);

    hmiios2014 w;
    w.resize(1280, 800);
    w.show();
    return a.exec();
}
