#ifndef HMIIOS2014_H
#define HMIIOS2014_H

#include <QDebug>
#include <QKeyEvent>
#include <QMainWindow>
#include <QTranslator>

#include "TSDWindow.h"
#include "mapFilterWidget.h"
#include "ui_hmiios2014.h"

class hmiios2014 : public QMainWindow
{
    Q_OBJECT

public:
    hmiios2014(QWidget* parent = 0);
    ~hmiios2014();
    void setStatusBarText(QString text);
    void setTsdWindow(TSDWindow* a_tsd);  // {m_tsd = a_tsd;}
    // the comms object
    // HmiiosComms UIQueue; //("168.99.88.80");

private:
    Ui::hmiios2014Class ui;
    QActionGroup* m_mapOpActionGroup;
    TSDWindow* m_tsd;
    QWidget* m_tsdContainer;
    // MapFilterWidget * m_mapFilterWidget;

    QTranslator m_translatorChinese;  // contains the translations for this application
    QTranslator m_translatorDefault;  // contains the translations for qt

    QString getOrPromptAccountKey();

signals:
    void signal_widget_resize(QRect rect);

protected:
    bool eventFilter(QObject* obj, QEvent* ev) override;
    void keyPressEvent(QKeyEvent* event) override;
    void keyReleaseEvent(QKeyEvent* event) override;
    bool forwardTsdKeyEvent(QEvent* event) const;
    void switchTranslator(QTranslator& translator, const QString& filename);
    void changeEvent(QEvent* event) override;

private slots:
    void slot_setFps(int a_iFps);
    void on_actionSelect_triggered();
    void on_actionEBL_triggered();
    void on_actionMapLayerFilter_triggered();
    void on_actionCenterMap_triggered();
    void on_actionAutoZoom_triggered();
    void on_actionAutoSwing_triggered();
    void on_actionShaderToys_triggered();
    void on_actionVsync_triggered();
    void on_actionChineseLang_triggered();
    void on_actionDefaultLang_triggered();
    void on_actionBusRoute_triggered();
    void on_actionBusTrack_triggered();
    void on_actionClearBus_triggered();
    void slot_setMapFilter(TSDWindow::DisplayMaskBits layer_id, int state);
};

#endif  // HMIIOS2014_H
