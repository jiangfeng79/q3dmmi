#include "hmiios2014.h"
#include <QtWidgets/QApplication>
#include <QSplashScreen>
#include "TSDWindow.h"
#include <QStyleFactory>
hmiios2014 * G_P_MAINWINDOW;
int main(int argc, char *argv[])
{
	QApplication a(argc, argv);
	hmiios2014 w;
	w.resize(1280, 800);
	w.show();
	return a.exec();
}
