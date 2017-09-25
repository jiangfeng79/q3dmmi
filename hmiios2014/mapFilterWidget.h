#ifndef MAPFILTERWIDGET_H
#define MAPFILTERWIDGET_H

#include <QWidget>
#include <QToolBar>
#include "ui_mapFilter.h"
#include "TSDWindow.h"

class MapFilterWidget : public QWidget, Ui::MapFilterWidget
{
	Q_OBJECT

public:
	MapFilterWidget(QWidget *parent);
	~MapFilterWidget();
	void retranslate();

private:

signals :
	void signal_checkBox_state(TSDWindow::DisplayMaskBits layer, int state);

private slots:
	void on_checkBoxWaterArea_stateChanged(int state);
	void on_checkBoxLand_stateChanged(int state);
	void on_checkBoxLandUsage_stateChanged(int state);
	void on_checkBoxBuildings_stateChanged(int state);
	void on_checkBoxMotorWays_stateChanged(int state);
	void on_checkBoxRailways_stateChanged(int state);
	void on_checkBoxMainRoads_stateChanged(int state);
	void on_checkBoxMinorRoads_stateChanged(int state);
	void on_checkBoxAeroWays_stateChanged(int state);
	void on_checkBoxAmenities_stateChanged(int state);
	void on_checkBoxPlaces_stateChanged(int state);
	void on_checkBoxManMade_stateChanged(int state);
};

#endif // MAPFILTERWIDGET_H
