#include "mainwindow.h"
#include "ui_mainwindow.h"

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    m_mapOpActionGroup = new QActionGroup(ui->subToolBar);
    m_mapOpActionGroup->setExclusive(true);
    m_mapOpActionGroup->addAction(ui->actionSelect);
    m_mapOpActionGroup->addAction(ui->actionEBL);
    ui->subToolBar->addActions(m_mapOpActionGroup->actions());
    ui->actionSelect->setChecked(true);

    m_mapFilterWidget = ui->widgetMapFilter;//new MapFilterWidget(this);

    //ui->widget->installEventFilter(this);
    //installEventFilter(this);

    QSurfaceFormat format;
    //format.setDepthBufferSize( 24 );
    //format.setSwapBehavior(QSurfaceFormat::TripleBuffer);
    //format.setMajorVersion( 2 );
    //format.setMinorVersion( 1 );
    //format.setProfile( QSurfaceFormat::CoreProfile );
    format.setSamples(16 );

    m_tsd = new TSDWindow();
    m_tsd->setFormat(format);
    //tsdWindow.setFlags(Qt::FramelessWindowHint);
    m_tsd->setAnimating(true);
    //tsdWindow.setParent(w.windowHandle());
    //    tsdWindow.resize(800,600); // must hv it or else shader failure
    //tsdWindow.show();


    QWidget *centralWidget = QWidget::createWindowContainer(m_tsd);
    setTsdWindow(m_tsd);
    setCentralWidget(centralWidget);

    //QObject::connect(&w, SIGNAL(signal_widget_resize(QRect)), &tsdWindow, SLOT(resetGeometry(QRect)));
    QObject::connect(m_tsd, SIGNAL(signal_setFps(int)),this, SLOT(slot_setFps(int)));
}

MainWindow::~MainWindow()
{
    delete ui;
}


/*
bool MainWindow::eventFilter(QObject *watched, QEvent* e)
{
    if ((watched == ui->widget || watched == this) && (e->type() == QEvent::Move || e->type() == QEvent::Show || e->type() == QEvent::Resize))
    {
        QRect widgetRect = ui->widget->geometry();

        widgetRect.moveTopLeft(ui->widget->parentWidget()->mapTo(this,widgetRect.topLeft()));
        emit signal_widget_resize(widgetRect);
    }
    return QWidget::eventFilter(watched, e);
}
*/
void MainWindow::slot_setFps(int a_iFps)
{
    ui->statusBar->showMessage(QString("Fps: %1").arg(a_iFps));
}

void MainWindow::on_actionSelect_triggered()
{
    qDebug() << "action pan triggered";
    if(m_tsd)
        m_tsd->setMapOpMask(TSDWindow::PAN);
}

void MainWindow::on_actionEBL_triggered()
{
    qDebug() << "action EBL triggered";
        if(m_tsd)
        m_tsd->setMapOpMask(TSDWindow::EBL);
}

void MainWindow::on_actionCenterMap_triggered()
{
        if(m_tsd)
        m_tsd->centerMap();

}

void MainWindow::setTsdWindow(TSDWindow * a_tsd)
{
    m_tsd = a_tsd;
    connect(m_mapFilterWidget, SIGNAL(signal_checkBox_state(TSDWindow::DisplayMaskBits, int)), this, SLOT(slot_setMapFilter(TSDWindow::DisplayMaskBits, int)));

}

void MainWindow::slot_setMapFilter(TSDWindow::DisplayMaskBits layer, int state)
{
    int l_iLayerText = ((int)layer)<<1;
    if(state == Qt::Unchecked)
    {
        m_tsd->setDisplayMask(layer, false);
        m_tsd->setDisplayMask((TSDWindow::DisplayMaskBits)l_iLayerText, false);

    }else if(state == Qt::Checked)
    {
        m_tsd->setDisplayMask(layer, true);
        m_tsd->setDisplayMask((TSDWindow::DisplayMaskBits)l_iLayerText, true);
    }else if(state == Qt::PartiallyChecked)
    {
        m_tsd->setDisplayMask(layer, true);
        m_tsd->setDisplayMask((TSDWindow::DisplayMaskBits)l_iLayerText, false);
    }
}

void MainWindow::on_actionAutoZoom_triggered()
{
    if(m_tsd)
    {
        if(m_tsd->getAutoZoom())
            m_tsd->setAutoZoom(false);
        else
            m_tsd->setAutoZoom(true);
    }
}

void MainWindow::on_actionMapLayerFilter_triggered()
{
    qDebug() << "on_actionMapLayerFilter_triggered";
    m_mapFilterWidget->show();
    m_mapFilterWidget->raise();
    m_mapFilterWidget->activateWindow();
}
