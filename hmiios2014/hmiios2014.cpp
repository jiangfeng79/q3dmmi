#include "hmiios2014.h"

#include <QLibraryInfo>
#include <QMessageBox>

hmiios2014::hmiios2014(QWidget* parent) : QMainWindow(parent), m_tsd(nullptr)
{
    ui.setupUi(this);

    // ui.widget->installEventFilter(this);
    // installEventFilter(this);

    m_mapOpActionGroup = new QActionGroup(ui.subToolBar);
    m_mapOpActionGroup->setExclusive(true);
    m_mapOpActionGroup->addAction(ui.actionSelect);
    m_mapOpActionGroup->addAction(ui.actionEBL);
    ui.subToolBar->addActions(m_mapOpActionGroup->actions());
    ui.actionSelect->setChecked(true);

    // m_mapFilterWidget = ui.widgetMapFilter;

    // Use the app-wide default surface format defined in main().
    QSurfaceFormat format = QSurfaceFormat::defaultFormat();
    m_tsd = new TSDWindow();
    m_tsd->setFormat(format);
    m_tsd->setAnimating(true);

    auto* centralWidget = QWidget::createWindowContainer(m_tsd, this);
    centralWidget->setFocusPolicy(Qt::StrongFocus);
    centralWidget->setFocus();
    centralWidget->installEventFilter(this);
    m_tsd->requestActivate();
    m_tsdContainer = centralWidget;
    setTsdWindow(m_tsd);
    setCentralWidget(centralWidget);

    connect(m_tsd, &TSDWindow::signal_setFps, this, &hmiios2014::slot_setFps);
}

hmiios2014::~hmiios2014() {}

void hmiios2014::slot_setFps(int a_iFps)
{
    ui.statusBar->showMessage(QString("Fps: %1").arg(a_iFps));
}

bool hmiios2014::forwardTsdKeyEvent(QEvent* event) const
{
    if (!m_tsd)
    {
        return false;
    }

    QCoreApplication::sendEvent(m_tsd, event);
    return event->isAccepted();
}

bool hmiios2014::eventFilter(QObject* obj, QEvent* ev)
{
    if (obj == m_tsdContainer && (ev->type() == QEvent::KeyPress || ev->type() == QEvent::KeyRelease))
    {
        return forwardTsdKeyEvent(ev);
    }

    return QMainWindow::eventFilter(obj, ev);
}

void hmiios2014::keyPressEvent(QKeyEvent* event)
{
    if (forwardTsdKeyEvent(event))
    {
        return;
    }

    QMainWindow::keyPressEvent(event);
}

void hmiios2014::keyReleaseEvent(QKeyEvent* event)
{
    if (forwardTsdKeyEvent(event))
    {
        return;
    }

    QMainWindow::keyReleaseEvent(event);
}

void hmiios2014::on_actionSelect_triggered()
{
    qDebug() << "action pan triggered";
    if (m_tsd)
    {
        m_tsd->setMapOpMask(TSDWindow::PAN);
    }
}

void hmiios2014::on_actionEBL_triggered()
{
    qDebug() << "action EBL triggered";
    if (m_tsd)
    {
        m_tsd->setMapOpMask(TSDWindow::EBL);
    }
}

void hmiios2014::on_actionMapLayerFilter_triggered()
{
    qDebug() << "on_actionMapLayerFilter_triggered";
    if (ui.dockWidget->isHidden())
    {
        ui.dockWidget->show();
    }
    else
    {
        ui.dockWidget->hide();
    }
}

void hmiios2014::on_actionCenterMap_triggered()
{
    if (m_tsd)
    {
        m_tsd->centerMap();
    }
}

void hmiios2014::setTsdWindow(TSDWindow* a_tsd)
{
    m_tsd = a_tsd;
    if (!m_tsd)
    {
        return;
    }

    connect(ui.widgetMapFilter, &MapFilterWidget::signal_checkBox_state, this, &hmiios2014::slot_setMapFilter);
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
        m_tsd->setAutoZoom(!m_tsd->getAutoZoom());
    }
}

void hmiios2014::on_actionAutoSwing_triggered()
{
    if (m_tsd)
    {
        m_tsd->setAutoSwing(!m_tsd->getAutoSwing());
    }
}

void hmiios2014::on_actionShaderToys_triggered()
{
    if (m_tsd)
    {
        m_tsd->setShaderToys(!m_tsd->getShaderToys());
    }
}

void hmiios2014::on_actionVsync_triggered()
{
    if (!m_tsd)
    {
        return;
    }

    // toggleVsync() flips the internal state, so sync the menu check state
    // from the window afterwards.
    m_tsd->toggleVsync();
    ui.actionVsync->setChecked(m_tsd->vsyncEnabled());
}

void hmiios2014::on_actionChineseLang_triggered()
{
    const QString languageName = QStringLiteral("zh");
    switchTranslator(m_translatorChinese, QStringLiteral(":/hmiios2014/hmiios2014_%1.qm").arg(languageName));
    ui.statusBar->showMessage(tr("Current Language changed to %1").arg(languageName));
}

void hmiios2014::on_actionDefaultLang_triggered()
{
    const QString languageName = QStringLiteral("en");
    switchTranslator(m_translatorDefault, QStringLiteral(":/hmiios2014/hmiios2014_%1.qm").arg(languageName));
    ui.statusBar->showMessage(tr("Current Language changed to %1").arg(languageName));
}

void hmiios2014::switchTranslator(QTranslator& translator, const QString& filename)
{
    // remove the old translator
    qApp->removeTranslator(&translator);

    // load the new translator
    if (translator.load(filename))
    {
        qApp->installTranslator(&translator);
    }
}

void hmiios2014::changeEvent(QEvent* event)
{
    if (event && event->type() == QEvent::LanguageChange)
    {
        ui.retranslateUi(this);
        ui.widgetMapFilter->retranslate();
    }

    QMainWindow::changeEvent(event);
}
