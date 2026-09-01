#include "mapFilterWidget.h"

#include <QDebug>

#include "TSDWindow.h"

MapFilterWidget::MapFilterWidget(QWidget* parent) : QWidget(parent)
{
    setupUi(this);
    // setWindowFlags(Qt::WindowCloseButtonHint|Qt::Window);
}

MapFilterWidget::~MapFilterWidget() {}

void MapFilterWidget::on_checkBoxWaterArea_stateChanged(int state)
{
    qDebug() << "on_checkBoxWaterArea_stateChanged" << state;
    emit signal_checkBox_state(TSDWindow::WATER_AREA, state);
}

void MapFilterWidget::on_checkBoxLand_stateChanged(int state)
{
    qDebug() << "on_checkBoxLand_stateChanged" << state;
    emit signal_checkBox_state(TSDWindow::COASTAL, state);
}

void MapFilterWidget::on_checkBoxLandUsage_stateChanged(int state)
{
    qDebug() << "on_checkBoxLandUsage_stateChanged" << state;
    emit signal_checkBox_state(TSDWindow::LAND_USAGE, state);
}
void MapFilterWidget::on_checkBoxBuildings_stateChanged(int state)
{
    qDebug() << "on_checkBoxBuildings_stateChanged" << state;
    emit signal_checkBox_state(TSDWindow::BUILDING, state);
}
void MapFilterWidget::on_checkBoxMotorWays_stateChanged(int state)
{
    qDebug() << "on_checkBoxMotorWays_stateChanged" << state;
    emit signal_checkBox_state(TSDWindow::MOTOR_WAYS, state);
}
void MapFilterWidget::on_checkBoxRailways_stateChanged(int state)
{
    qDebug() << "on_checkBoxRailways_stateChanged" << state;
    emit signal_checkBox_state(TSDWindow::MRT, state);
    emit signal_checkBox_state(TSDWindow::MRT_POINT, state);
}
void MapFilterWidget::on_checkBoxMainRoads_stateChanged(int state)
{
    qDebug() << "on_checkBoxMainRoads_stateChanged" << state;
    emit signal_checkBox_state(TSDWindow::MAIN_ROADS, state);
}
void MapFilterWidget::on_checkBoxMinorRoads_stateChanged(int state)
{
    qDebug() << "on_checkBoxMinorRoads_stateChanged" << state;
    emit signal_checkBox_state(TSDWindow::MINOR_ROADS, state);
}
void MapFilterWidget::on_checkBoxAeroWays_stateChanged(int state)
{
    qDebug() << "on_checkBoxAeroWays_stateChanged" << state;
    emit signal_checkBox_state(TSDWindow::AIR_WAYS, state);
}
void MapFilterWidget::on_checkBoxAmenities_stateChanged(int state)
{
    qDebug() << "on_checkBoxAmenities_stateChanged" << state;
    emit signal_checkBox_state(TSDWindow::AMENITIES, state);
}
void MapFilterWidget::on_checkBoxPlaces_stateChanged(int state)
{
    qDebug() << "on_checkBoxPlaces_stateChanged" << state;
    emit signal_checkBox_state(TSDWindow::PLACES, state);
}

void MapFilterWidget::on_checkBoxManMade_stateChanged(int state)
{
    qDebug() << "on_checkBoxManMade_stateChanged" << state;
    emit signal_checkBox_state(TSDWindow::MAN_MADE, state);
}

void MapFilterWidget::on_checkBoxFlights_stateChanged(int state)
{
    qDebug() << "on_checkBoxFlights_stateChanged" << state;
    // The single "Flights" checkbox toggles both the plane markers (FLIGHTS)
    // and the position trails (FLIGHT_TRAILS).
    emit signal_checkBox_state(TSDWindow::FLIGHTS, state);
    emit signal_checkBox_state(TSDWindow::FLIGHT_TRAILS, state);
}

void MapFilterWidget::on_checkBoxBusRoutes_stateChanged(int state)
{
    qDebug() << "on_checkBoxBusRoutes_stateChanged" << state;
    emit signal_checkBox_state(TSDWindow::BUS_ROUTES, state);
    emit signal_checkBox_state(TSDWindow::BUS_ROUTES2, state);
}

void MapFilterWidget::on_checkBoxBusTracks_stateChanged(int state)
{
    qDebug() << "on_checkBoxBusTracks_stateChanged" << state;
    emit signal_checkBox_state(TSDWindow::BUS_TRACKS, state);
}

void MapFilterWidget::retranslate()
{
    retranslateUi(this);
}
