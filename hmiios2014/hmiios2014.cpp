#include "hmiios2014.h"

#include <QCloseEvent>
#include <QDialog>
#include <QDialogButtonBox>
#include <QFile>
#include <QInputDialog>
#include <QLabel>
#include <QLibraryInfo>
#include <QLineEdit>
#include <QMessageBox>
#include <QVBoxLayout>

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

    // Bus arrival times dock (bottom of the main window by default). It pops
    // up whenever a new stop is queried and hides when there is no arrival
    // data. The dock and its widget come from the .ui file.
    ui.dockBusArrival->hide();
    connect(m_tsd, &TSDWindow::busArrivalSnapshotUpdated, this, &hmiios2014::slot_busArrivalSnapshotUpdated);
    connect(m_tsd, &TSDWindow::busInfoCleared, this, &hmiios2014::slot_busInfoCleared);
    loadConfig();
}

hmiios2014::~hmiios2014() {}


bool hmiios2014::loadConfig()
{
    if (!m_tsd)
    {
        return false;
    }

    // Seed with the current state so any field absent from the JSON keeps its
    // default value.
    m_config.centerX = m_tsd->mapCenterX();
    m_config.centerY = m_tsd->mapCenterY();
    m_config.scale = m_tsd->scaleFactor();
    m_config.rotationAngle = m_tsd->rotationAngle();
    m_config.displayMask = m_tsd->getDisplayMask();
    m_config.autoZoom = m_tsd->getAutoZoom();
    m_config.autoSwing = m_tsd->getAutoSwing();
    m_config.shaderToys = m_tsd->getShaderToys();
    m_config.vsync = m_tsd->vsyncEnabled();
    m_config.mapOpMode = static_cast<int>(m_tsd->mapOpMask());
    m_config.language = m_language;

    if (!AppConfig::load(AppConfig::configPath(), m_config))
    {
        return false;
    }

    // Apply the restored state to the map window.
    m_tsd->setMapCenter(m_config.centerX, m_config.centerY);
    m_tsd->setScaleFactor(m_config.scale);
    m_tsd->setRotationAngle(m_config.rotationAngle);
    m_tsd->setDisplayMask(m_config.displayMask);
    m_tsd->setAutoZoom(m_config.autoZoom);
    m_tsd->setAutoSwing(m_config.autoSwing);
    m_tsd->setShaderToys(m_config.shaderToys);
    m_tsd->setVsyncEnabled(m_config.vsync);
    ui.actionVsync->setChecked(m_config.vsync);

    // Restore the active map tool.
    if (m_config.mapOpMode == static_cast<int>(OpenglWindow::EBL))
    {
        m_tsd->setMapOpMask(OpenglWindow::EBL);
        ui.actionSelect->setChecked(false);
        ui.actionEBL->setChecked(true);
    }
    else
    {
        m_tsd->setMapOpMask(OpenglWindow::PAN);
        ui.actionSelect->setChecked(true);
        ui.actionEBL->setChecked(false);
    }

    // Restore the language.
    if (m_config.language == QStringLiteral("zh"))
    {
        switchTranslator(m_translatorChinese, QStringLiteral(":/hmiios2014/hmiios2014_zh.qm"));
    }
    else
    {
        switchTranslator(m_translatorDefault, QStringLiteral(":/hmiios2014/hmiios2014_en.qm"));
    }

    // Sync the map filter checkboxes with the restored display mask.
    syncMapFilterCheckboxes();

    return true;
}

void hmiios2014::saveConfig()
{
    if (!m_tsd)
    {
        return;
    }

    m_config.centerX = m_tsd->mapCenterX();
    m_config.centerY = m_tsd->mapCenterY();
    m_config.scale = m_tsd->scaleFactor();
    m_config.rotationAngle = m_tsd->rotationAngle();
    m_config.displayMask = m_tsd->getDisplayMask();
    m_config.autoZoom = m_tsd->getAutoZoom();
    m_config.autoSwing = m_tsd->getAutoSwing();
    m_config.shaderToys = m_tsd->getShaderToys();
    m_config.vsync = m_tsd->vsyncEnabled();
    m_config.mapOpMode = static_cast<int>(m_tsd->mapOpMask());
    m_config.language = m_language;
    m_config.windowGeometry = geometry();
    m_config.maximized = isMaximized();

    AppConfig::save(AppConfig::configPath(), m_config);
}

void hmiios2014::closeEvent(QCloseEvent* event)
{
    saveConfig();
    QMainWindow::closeEvent(event);
}

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
    syncMapFilterCheckboxes();
}

void hmiios2014::syncMapFilterCheckboxes()
{
    // The .ui file hard-codes every checkbox as checked, but the real initial
    // display mask (TSDWindow::m_displayMask) has several layers turned off.
    // Sync the checkbox states from the actual mask so the GUI reflects what
    // is really being drawn.
    if (!m_tsd)
    {
        return;
    }

    ui.widgetMapFilter->syncFromMask(m_tsd->getDisplayMask());
}

void hmiios2014::slot_setMapFilter(TSDWindow::DisplayMaskBits layer, int state)
{
    // qWarning() << "slot_setMapFilter" << Qt::hex << m_tsd->getDisplayMask();
    const std::uint64_t layerText = static_cast<std::uint64_t>(layer) << 1;
    if (state == Qt::Unchecked)
    {
        m_tsd->setDisplayMask(layer, false);
        m_tsd->setDisplayMask(static_cast<TSDWindow::DisplayMaskBits>(layerText), false);
    }
    else if (state == Qt::Checked)
    {
        m_tsd->setDisplayMask(layer, true);
        m_tsd->setDisplayMask(static_cast<TSDWindow::DisplayMaskBits>(layerText), true);
    }
    else if (state == Qt::PartiallyChecked)
    {
        m_tsd->setDisplayMask(layer, true);
        m_tsd->setDisplayMask(static_cast<TSDWindow::DisplayMaskBits>(layerText), false);
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
    m_language = languageName;
    ui.statusBar->showMessage(tr("Current Language changed to %1").arg(languageName));
}

void hmiios2014::on_actionDefaultLang_triggered()
{
    const QString languageName = QStringLiteral("en");
    switchTranslator(m_translatorDefault, QStringLiteral(":/hmiios2014/hmiios2014_%1.qm").arg(languageName));
    m_language = languageName;
    ui.statusBar->showMessage(tr("Current Language changed to %1").arg(languageName));
}

QString hmiios2014::getOrPromptAccountKey()
{
    const QString secretFileName = QStringLiteral("datamall.secret");
    QFile secretFile(secretFileName);
    if (secretFile.exists())
    {
        if (secretFile.open(QIODevice::ReadOnly))
        {
            QByteArray base64Data = secretFile.readAll().trimmed();
            secretFile.close();
            QByteArray decodedKey = QByteArray::fromBase64(base64Data);
            QString key = QString::fromUtf8(decodedKey).trimmed();
            if (!key.isEmpty())
            {
                return key;
            }
        }
    }

    QDialog dialog(this);
    dialog.setWindowTitle(tr("LTA DataMall Account Key"));

    auto* layout = new QVBoxLayout(&dialog);
    auto* prompt = new QLabel(tr("Get your free access key:<br>"
                                 "<a href=\"https://datamall.lta.gov.sg/content/datamall/en/request-for-api.html\">"
                                 "LTA DataMall API request</a><br>"
                                 "Enter LTA DataMall Account Key:<br>"),
                              &dialog);
    prompt->setOpenExternalLinks(true);
    prompt->setWordWrap(true);

    auto* accountKeyInput = new QLineEdit(&dialog);
    accountKeyInput->setEchoMode(QLineEdit::Password);
    auto* buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, &dialog);
    connect(buttons, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);

    layout->addWidget(prompt);
    layout->addWidget(accountKeyInput);
    layout->addWidget(buttons);

    if (dialog.exec() == QDialog::Accepted && !accountKeyInput->text().trimmed().isEmpty())
    {
        QString accountKey = accountKeyInput->text().trimmed();
        if (secretFile.open(QIODevice::WriteOnly))
        {
            secretFile.write(accountKey.toUtf8().toBase64());
            secretFile.close();
        }
        return accountKey;
    }

    return QString();
}

void hmiios2014::on_actionBusRoute_triggered()
{
    bool ok = false;
    QString busNo = QInputDialog::getText(this, tr("Bus route"), tr("Enter bus number:"), QLineEdit::Normal,
                                          QStringLiteral("0"), &ok);
    if (ok && !busNo.trimmed().isEmpty())
    {
        QString accountKey = getOrPromptAccountKey();
        if (m_tsd)
        {
            m_tsd->fetchBusRoute(busNo.trimmed(), accountKey);
        }
    }
}

void hmiios2014::on_actionBusTrack_triggered()
{
    bool ok = false;
    QString stopCode = QInputDialog::getText(this, tr("Bus track"), tr("Enter bus stop number:"), QLineEdit::Normal,
                                             QStringLiteral("0"), &ok);
    if (ok && !stopCode.trimmed().isEmpty())
    {
        QString accountKey = getOrPromptAccountKey();
        if (m_tsd)
        {
            m_tsd->trackBusStop(stopCode.trimmed(), accountKey);
        }
    }
}

void hmiios2014::on_actionClearBus_triggered()
{
    if (m_tsd)
    {
        m_tsd->clearBusInfo();
    }
    ui.statusBar->showMessage(tr("Cleared all bus routes and tracks"));
}

void hmiios2014::slot_busArrivalSnapshotUpdated(const BusStopSnapshot& snapshot)
{
    // Populate the table; show the dock only when there is data to display.
    const bool hasData = ui.widgetBusArrival->setSnapshot(snapshot);
    ui.dockBusArrival->setVisible(hasData);
    if (hasData)
    {
        ui.dockBusArrival->setWindowTitle(tr("Bus Arrival Times - Stop %1").arg(snapshot.busStopCode));
        ui.dockBusArrival->raise();
    }
}

void hmiios2014::slot_busInfoCleared()
{
    ui.widgetBusArrival->clear();
    ui.dockBusArrival->setWindowTitle(tr("Bus Arrival Times"));
    ui.dockBusArrival->hide();
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
        ui.widgetBusArrival->retranslate();
    }

    QMainWindow::changeEvent(event);
}
