#include "hmiios2014.h"
#include <QMessageBox>
#include <QLibraryInfo>

hmiios2014::hmiios2014(QWidget* parent)
    : QMainWindow(parent), m_tsd(nullptr)
{
    ui.setupUi(this);

    m_translatorChinese.load("hmiios2014_zh.qm"); // contains the translations for this application
    m_translatorDefault.load("hmiios2014_en.qm"); // contains the translations for qt

    //ui.widget->installEventFilter(this);
    //installEventFilter(this);

    m_mapOpActionGroup = new QActionGroup(ui.subToolBar);
    m_mapOpActionGroup->setExclusive(true);
    m_mapOpActionGroup->addAction(ui.actionSelect);
    m_mapOpActionGroup->addAction(ui.actionEBL);
    ui.subToolBar->addActions(m_mapOpActionGroup->actions());
    ui.actionSelect->setChecked(true);

    //m_mapFilterWidget = ui.widgetMapFilter;

    QSurfaceFormat format;
    format.setSamples(16);
    format.setStencilBufferSize(8);

    m_tsd = new TSDWindow();
    m_tsd->setFormat(format);
    m_tsd->setAnimating(true);


    QWidget* centralWidget = QWidget::createWindowContainer(m_tsd);
    setTsdWindow(m_tsd);
    setCentralWidget(centralWidget);

    QObject::connect(m_tsd, SIGNAL(signal_setFps(int)), this, SLOT(slot_setFps(int)));
}

hmiios2014::~hmiios2014()
{
}

void hmiios2014::slot_setFps(int a_iFps)
{
    ui.statusBar->showMessage(QString("Fps: %1").arg(a_iFps));
}

void hmiios2014::on_actionSelect_triggered()
{
    qDebug() << "action pan triggered";
    if (m_tsd)
        m_tsd->setMapOpMask(TSDWindow::PAN);
}

void hmiios2014::on_actionEBL_triggered()
{
    qDebug() << "action EBL triggered";
    if (m_tsd)
        m_tsd->setMapOpMask(TSDWindow::EBL);
}

void hmiios2014::on_actionMapLayerFilter_triggered()
{
    qDebug() << "on_actionMapLayerFilter_triggered";
    if (ui.dockWidget->isHidden())
        ui.dockWidget->show();
    else
        ui.dockWidget->hide();
}

void hmiios2014::on_actionCenterMap_triggered()
{
    if (m_tsd)
        m_tsd->centerMap();
}

void hmiios2014::setTsdWindow(TSDWindow* a_tsd)
{
    m_tsd = a_tsd;
    connect(ui.widgetMapFilter, SIGNAL(signal_checkBox_state(TSDWindow::DisplayMaskBits, int)), this, SLOT(slot_setMapFilter(TSDWindow::DisplayMaskBits, int)));

}

void hmiios2014::slot_setMapFilter(TSDWindow::DisplayMaskBits layer, int state)
{
    int l_iLayerText = ((int)layer) << 1;
    if (state == Qt::Unchecked)
    {
        m_tsd->setDisplayMask(layer, false);
        m_tsd->setDisplayMask((TSDWindow::DisplayMaskBits)l_iLayerText, false);

    }
    else if (state == Qt::Checked)
    {
        m_tsd->setDisplayMask(layer, true);
        m_tsd->setDisplayMask((TSDWindow::DisplayMaskBits)l_iLayerText, true);
    }
    else if (state == Qt::PartiallyChecked)
    {
        m_tsd->setDisplayMask(layer, true);
        m_tsd->setDisplayMask((TSDWindow::DisplayMaskBits)l_iLayerText, false);
    }
}

void hmiios2014::on_actionAutoZoom_triggered()
{
    if (m_tsd)
    {
        if (m_tsd->getAutoZoom())
            m_tsd->setAutoZoom(false);
        else
            m_tsd->setAutoZoom(true);
    }
}

void hmiios2014::on_actionAutoSwing_triggered()
{
    if (m_tsd)
    {
        if (m_tsd->getAutoSwing())
            m_tsd->setAutoSwing(false);
        else
            m_tsd->setAutoSwing(true);
    }
}

void hmiios2014::on_actionChineseLang_triggered()
{
    //QString locale = QLocale::system().name();
    //QLocale::setDefault(locale);
    QString chineselanguageName = "zh";

    switchTranslator(m_translatorDefault, QString(":/hmiios2014/hmiios2014_%1.qm").arg(chineselanguageName));
    ui.statusBar->showMessage(tr("Current Language changed to %1").arg(chineselanguageName));
}

void hmiios2014::on_actionDefaultLang_triggered()
{
    //QMessageBox::information(this, "Lang test",tr("good"));
    QString englishlanguageName = "en";

    switchTranslator(m_translatorChinese, QString(":/hmiios2014/hmiios2014_%1.qm").arg(englishlanguageName));
    ui.statusBar->showMessage(tr("Current Language changed to %1").arg(englishlanguageName));
}

void hmiios2014::switchTranslator(QTranslator& translator, const QString& filename)
{
    // remove the old translator
    qApp->removeTranslator(&translator);

    // load the new translator
    if (translator.load(filename))
        qApp->installTranslator(&translator);
}

void hmiios2014::changeEvent(QEvent* event)
{
    if (0 != event) {
        switch (event->type()) {
            // this event is send if a translator is loaded
        case QEvent::LanguageChange:
            ui.retranslateUi(this);
            ui.widgetMapFilter->retranslate();
            break;

            /*
            // this event is send, if the system, language changes
            case QEvent::LocaleChange:
            {
            QLocale locale = QLocale("en");
            QLocale::setDefault(locale);
            QString languageName = QLocale::languageToString(locale.language());
            switchTranslator(m_translatorDefault, QString("hmiios2014_%1.ts").arg(languageName));
            ui.statusBar->showMessage(tr("Current Language changed to %1").arg(languageName));
            }
            break;
            */
        }
    }
    QMainWindow::changeEvent(event);
}
