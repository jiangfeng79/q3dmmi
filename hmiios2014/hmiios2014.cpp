#include "hmiios2014.h"

#include <QCloseEvent>
#include <QDialog>
#include <QDialogButtonBox>
#include <QDockWidget>
#include <QFile>
#include <QInputDialog>
#include <QLabel>
#include <QLibraryInfo>
#include <QLineEdit>
#include <QMessageBox>
#include <QMoveEvent>
#include <QResizeEvent>
#include <QTimer>
#include <QVBoxLayout>

#include <limits>

#include "appConfigView.h"

hmiios2014::hmiios2014(QWidget* parent) : QMainWindow(parent), m_tsd(nullptr)
{
    ui.setupUi(this);
    resize(1280, 800);

    disconnect(ui.actionFullscreen, nullptr, this, nullptr);
    disconnect(ui.actionNormalscreen, nullptr, this, nullptr);
    connect(ui.actionFullscreen, &QAction::triggered, this, [this] { setFullscreenRequested(true); });
    connect(ui.actionNormalscreen, &QAction::triggered, this, [this] { setFullscreenRequested(false); });

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

    m_configObserverId = m_config.addObserver(
        [this](AppConfig::Field field, const AppConfig::Data& config) { applyConfigChange(field, config); });
    loadConfig();
    QTimer::singleShot(0, this, [this] {
        m_configReady = true;
        syncWindowConfig();
    });
    connect(m_tsd, &OpenglWindow::cameraChanged, this, &hmiios2014::syncCameraConfig);

    auto* configDock = new QDockWidget(tr("Configuration Database"), this);
    configDock->setObjectName(QStringLiteral("configurationDatabaseDock"));
    configDock->setWidget(new AppConfigView(m_config, configDock));
    addDockWidget(Qt::RightDockWidgetArea, configDock);
    ui.menuDebug->addAction(configDock->toggleViewAction());
    configDock->hide();
}

hmiios2014::~hmiios2014()
{
    m_config.removeObserver(m_configObserverId);
}

AppConfig::Data hmiios2014::currentConfig() const
{
    AppConfig::Data config = m_config.data();
    if (!m_tsd)
    {
        return config;
    }

    auto setValue = [&config](const QString& group, const QString& name, const QString& value) {
        for (AppConfig::Field field : AppConfig::fields())
        {
            if (AppConfig::fieldGroup(field) == group && AppConfig::fieldName(field) == name)
            {
                AppConfig::setFieldValue(config, field, value);
                return;
            }
        }
    };
    setValue(QStringLiteral("camera"), QStringLiteral("centerX"), QString::number(m_tsd->mapCenterX()));
    setValue(QStringLiteral("camera"), QStringLiteral("centerY"), QString::number(m_tsd->mapCenterY()));
    setValue(QStringLiteral("camera"), QStringLiteral("scale"), QString::number(m_tsd->scaleFactor()));
    setValue(QStringLiteral("camera"), QStringLiteral("rotationAngle"), QString::number(m_tsd->rotationAngle()));
    setValue(QStringLiteral("display"), QStringLiteral("mask"),
             QStringLiteral("0x%1").arg(m_tsd->getDisplayMask(), 16, 16, QLatin1Char('0')));
    setValue(QStringLiteral("display"), QStringLiteral("autoZoom"), m_tsd->getAutoZoom() ? QStringLiteral("true") : QStringLiteral("false"));
    setValue(QStringLiteral("display"), QStringLiteral("autoSwing"), m_tsd->getAutoSwing() ? QStringLiteral("true") : QStringLiteral("false"));
    setValue(QStringLiteral("display"), QStringLiteral("shaderToys"), m_tsd->getShaderToys() ? QStringLiteral("true") : QStringLiteral("false"));
    setValue(QStringLiteral("display"), QStringLiteral("vsync"), m_tsd->vsyncEnabled() ? QStringLiteral("true") : QStringLiteral("false"));
    setValue(QStringLiteral("display"), QStringLiteral("mapOpMode"), QString::number(m_tsd->mapOpMask()));
    setValue(QStringLiteral("app"), QStringLiteral("language"), m_language);
    if (m_fullscreenRequested)
    {
        const QRect savedGeometry = normalGeometry();
        setValue(QStringLiteral("app"), QStringLiteral("windowGeometry"),
                 QStringLiteral("%1,%2,%3,%4")
                     .arg(savedGeometry.x()).arg(savedGeometry.y()).arg(savedGeometry.width()).arg(savedGeometry.height()));
        setValue(QStringLiteral("app"), QStringLiteral("maximized"), QStringLiteral("false"));
        setValue(QStringLiteral("app"), QStringLiteral("fullscreen"), QStringLiteral("true"));
    }
    else
    {
        const QRect savedGeometry = isMaximized() ? normalGeometry() : geometry();
        setValue(QStringLiteral("app"), QStringLiteral("windowGeometry"),
                 QStringLiteral("%1,%2,%3,%4")
                     .arg(savedGeometry.x()).arg(savedGeometry.y()).arg(savedGeometry.width()).arg(savedGeometry.height()));
        setValue(QStringLiteral("app"), QStringLiteral("maximized"), isMaximized() ? QStringLiteral("true") : QStringLiteral("false"));
        setValue(QStringLiteral("app"), QStringLiteral("fullscreen"), QStringLiteral("false"));
    }
    return config;
}

QString hmiios2014::configValue(const AppConfig::Data& config, const QString& group, const QString& name) const
{
    for (AppConfig::Field field : AppConfig::fields())
    {
        if (AppConfig::fieldGroup(field) == group && AppConfig::fieldName(field) == name)
        {
            return AppConfig::fieldValue(field, config);
        }
    }
    return {};
}

void hmiios2014::updateConfigValue(const QString& group, const QString& name, const QString& value)
{
    AppConfig::Data config = m_config.data();
    for (AppConfig::Field field : AppConfig::fields())
    {
        if (AppConfig::fieldGroup(field) == group && AppConfig::fieldName(field) == name)
        {
            if (AppConfig::setFieldValue(config, field, value))
            {
                m_config.replace(config);
            }
            return;
        }
    }
}

void hmiios2014::syncCameraConfig()
{
    if (!m_configReady || !m_tsd)
    {
        return;
    }

    AppConfig::Data config = m_config.data();
    for (AppConfig::Field field : AppConfig::fields())
    {
        if (AppConfig::fieldGroup(field) != QStringLiteral("camera"))
        {
            continue;
        }

        switch (field)
        {
        case AppConfig::Field::CenterX:
            AppConfig::setFieldValue(config, field, QString::number(m_tsd->mapCenterX(), 'g', std::numeric_limits<float>::max_digits10));
            break;
        case AppConfig::Field::CenterY:
            AppConfig::setFieldValue(config, field, QString::number(m_tsd->mapCenterY(), 'g', std::numeric_limits<float>::max_digits10));
            break;
        case AppConfig::Field::Scale:
            AppConfig::setFieldValue(config, field, QString::number(m_tsd->scaleFactor(), 'g', std::numeric_limits<float>::max_digits10));
            break;
        case AppConfig::Field::RotationAngle:
            AppConfig::setFieldValue(config, field, QString::number(m_tsd->rotationAngle(), 'g', std::numeric_limits<double>::max_digits10));
            break;
        default:
            break;
        }
    }
    m_config.replace(config);
}

void hmiios2014::syncWindowConfig()
{
    if (!m_configReady || m_syncingWindowConfig)
    {
        return;
    }

    m_syncingWindowConfig = true;
    const QRect savedGeometry = (isMaximized() || m_fullscreenRequested) ? normalGeometry() : geometry();
    updateConfigValue(QStringLiteral("app"), QStringLiteral("windowGeometry"),
                      QStringLiteral("%1,%2,%3,%4")
                          .arg(savedGeometry.x()).arg(savedGeometry.y())
                          .arg(savedGeometry.width()).arg(savedGeometry.height()));
    updateConfigValue(QStringLiteral("app"), QStringLiteral("maximized"),
                      isMaximized() ? QStringLiteral("true") : QStringLiteral("false"));
    updateConfigValue(QStringLiteral("app"), QStringLiteral("fullscreen"),
                      m_fullscreenRequested ? QStringLiteral("true") : QStringLiteral("false"));
    m_syncingWindowConfig = false;
}

void hmiios2014::setFullscreenRequested(bool fullscreen)
{
    m_fullscreenRequested = fullscreen;
    if (m_configReady)
    {
        updateConfigValue(QStringLiteral("app"), QStringLiteral("fullscreen"),
                          fullscreen ? QStringLiteral("true") : QStringLiteral("false"));
    }

    if (fullscreen)
    {
        showFullScreen();
    }
    else
    {
        showNormal();
    }
}

void hmiios2014::applyConfigChange(AppConfig::Field field, const AppConfig::Data& config)
{
    if (!m_tsd)
    {
        return;
    }

    const QString value = AppConfig::fieldValue(field, config);
    switch (field)
    {
    case AppConfig::Field::CenterX:
    case AppConfig::Field::CenterY:
        m_tsd->setMapCenter(configValue(config, QStringLiteral("camera"), QStringLiteral("centerX")).toFloat(),
                            configValue(config, QStringLiteral("camera"), QStringLiteral("centerY")).toFloat());
        break;
    case AppConfig::Field::Scale:
        m_tsd->setScaleFactor(value.toFloat());
        break;
    case AppConfig::Field::RotationAngle:
        m_tsd->setRotationAngle(value.toDouble());
        break;
    case AppConfig::Field::DisplayMask:
        m_tsd->setDisplayMask(value.toULongLong(nullptr, 0));
        syncMapFilterCheckboxes();
        break;
    case AppConfig::Field::AutoZoom:
        m_tsd->setAutoZoom(value == QStringLiteral("true"));
        break;
    case AppConfig::Field::AutoSwing:
        m_tsd->setAutoSwing(value == QStringLiteral("true"));
        break;
    case AppConfig::Field::ShaderToys:
        m_tsd->setShaderToys(value == QStringLiteral("true"));
        break;
    case AppConfig::Field::Vsync:
    {
        const bool enabled = value == QStringLiteral("true");
        m_tsd->setVsyncEnabled(enabled);
        ui.actionVsync->setChecked(enabled);
        break;
    }
    case AppConfig::Field::MapOpMode:
        if (value.toInt() == static_cast<int>(OpenglWindow::EBL))
        {
            m_tsd->setMapOpMask(OpenglWindow::EBL);
            ui.actionEBL->setChecked(true);
        }
        else
        {
            m_tsd->setMapOpMask(OpenglWindow::PAN);
            ui.actionSelect->setChecked(true);
        }
        break;
    case AppConfig::Field::Fullscreen:
        if (m_syncingWindowConfig)
        {
            return;
        }
        m_fullscreenRequested = value == QStringLiteral("true");
        if (m_fullscreenRequested)
        {
            QTimer::singleShot(0, this, [this] {
                if (configValue(m_config.data(), QStringLiteral("app"), QStringLiteral("fullscreen")) ==
                    QStringLiteral("true"))
                {
                    showFullScreen();
                }
            });
        }
        else if (isFullScreen())
        {
            showNormal();
        }
        break;
    case AppConfig::Field::Language:
        m_language = value;
        if (value == QStringLiteral("zh"))
        {
            switchTranslator(m_translatorChinese, QStringLiteral(":/hmiios2014/hmiios2014_zh.qm"));
        }
        else
        {
            switchTranslator(m_translatorDefault, QStringLiteral(":/hmiios2014/hmiios2014_en.qm"));
        }
        break;
    case AppConfig::Field::WindowGeometry:
        if (m_syncingWindowConfig)
        {
            return;
        }
        {
            const QStringList parts = value.split(QLatin1Char(','));
            if (parts.size() == 4)
            {
                const QRect geometry(parts[0].toInt(), parts[1].toInt(), parts[2].toInt(), parts[3].toInt());
                if (geometry.isValid())
                {
                    setGeometry(geometry);
                }
            }
        }
        break;
    case AppConfig::Field::Maximized:
        if (m_syncingWindowConfig)
        {
            return;
        }
        if (value == QStringLiteral("true"))
        {
            QTimer::singleShot(0, this, [this] {
                if (configValue(m_config.data(), QStringLiteral("app"), QStringLiteral("maximized")) ==
                    QStringLiteral("true"))
                {
                    showMaximized();
                }
            });
        }
        else if (isMaximized())
        {
            showNormal();
        }
        break;
    default:
        break;
    }
}

bool hmiios2014::loadConfig()
{
    if (!m_tsd)
    {
        return false;
    }

    if (m_config.load(AppConfig::configPath()))
    {
        return true;
    }

    // On first launch, publish the defaults generated from the source
    // config.json even though no persisted runtime file exists yet.
    for (AppConfig::Field field : AppConfig::fields())
    {
        applyConfigChange(field, m_config.data());
    }
    return false;
}

void hmiios2014::saveConfig()
{
    if (!m_tsd)
    {
        return;
    }

    m_config.replace(currentConfig());
    m_config.save(AppConfig::configPath());
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

bool hmiios2014::forwardTsdKeyEvent(QEvent* event)
{
    if (!m_tsd)
    {
        return false;
    }

    if (event->type() == QEvent::KeyPress)
    {
        const auto* keyEvent = static_cast<QKeyEvent*>(event);
        if (keyEvent->key() == Qt::Key_F)
        {
            setFullscreenRequested(!m_fullscreenRequested);
            event->accept();
            return true;
        }
        if (keyEvent->key() == Qt::Key_Escape && m_fullscreenRequested)
        {
            setFullscreenRequested(false);
            event->accept();
            return true;
        }
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

void hmiios2014::moveEvent(QMoveEvent* event)
{
    QMainWindow::moveEvent(event);
    syncWindowConfig();
}

void hmiios2014::resizeEvent(QResizeEvent* event)
{
    QMainWindow::resizeEvent(event);
    syncWindowConfig();
}

void hmiios2014::on_actionSelect_triggered()
{
    qDebug() << "action pan triggered";
    updateConfigValue(QStringLiteral("display"), QStringLiteral("mapOpMode"), QString::number(TSDWindow::PAN));
}

void hmiios2014::on_actionEBL_triggered()
{
    qDebug() << "action EBL triggered";
    updateConfigValue(QStringLiteral("display"), QStringLiteral("mapOpMode"), QString::number(TSDWindow::EBL));
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
    updateConfigValue(QStringLiteral("display"), QStringLiteral("mask"),
                      QStringLiteral("0x%1").arg(m_tsd->getDisplayMask(), 16, 16, QLatin1Char('0')));
}

void hmiios2014::on_actionAutoZoom_triggered()
{
    const bool enabled = configValue(m_config.data(), QStringLiteral("display"), QStringLiteral("autoZoom")) == QStringLiteral("true");
    updateConfigValue(QStringLiteral("display"), QStringLiteral("autoZoom"), enabled ? QStringLiteral("false") : QStringLiteral("true"));
}

void hmiios2014::on_actionAutoSwing_triggered()
{
    const bool enabled = configValue(m_config.data(), QStringLiteral("display"), QStringLiteral("autoSwing")) == QStringLiteral("true");
    updateConfigValue(QStringLiteral("display"), QStringLiteral("autoSwing"), enabled ? QStringLiteral("false") : QStringLiteral("true"));
}

void hmiios2014::on_actionShaderToys_triggered()
{
    const bool enabled = configValue(m_config.data(), QStringLiteral("display"), QStringLiteral("shaderToys")) == QStringLiteral("true");
    updateConfigValue(QStringLiteral("display"), QStringLiteral("shaderToys"), enabled ? QStringLiteral("false") : QStringLiteral("true"));
}

void hmiios2014::on_actionVsync_triggered()
{
    if (!m_tsd)
    {
        return;
    }

    const bool enabled = configValue(m_config.data(), QStringLiteral("display"), QStringLiteral("vsync")) == QStringLiteral("true");
    updateConfigValue(QStringLiteral("display"), QStringLiteral("vsync"), enabled ? QStringLiteral("false") : QStringLiteral("true"));
}

void hmiios2014::on_actionChineseLang_triggered()
{
    const QString languageName = QStringLiteral("zh");
    updateConfigValue(QStringLiteral("app"), QStringLiteral("language"), languageName);
    ui.statusBar->showMessage(tr("Current Language changed to %1").arg(languageName));
}

void hmiios2014::on_actionDefaultLang_triggered()
{
    const QString languageName = QStringLiteral("en");
    updateConfigValue(QStringLiteral("app"), QStringLiteral("language"), languageName);
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
        if (auto* configDock = findChild<QDockWidget*>(QStringLiteral("configurationDatabaseDock")))
        {
            configDock->setWindowTitle(tr("Configuration Database"));
        }
    }

    QMainWindow::changeEvent(event);

    if (event && event->type() == QEvent::WindowStateChange)
    {
        syncWindowConfig();
    }
}
