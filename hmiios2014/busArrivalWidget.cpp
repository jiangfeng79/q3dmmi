#include "busArrivalWidget.h"

#include <QDateTime>
#include <QHeaderView>

BusArrivalWidget::BusArrivalWidget(QWidget* parent) : QWidget(parent)
{
    setupUi(this);

    tableArrivals->horizontalHeader()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
    tableArrivals->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Stretch);
    tableArrivals->horizontalHeader()->setSectionResizeMode(2, QHeaderView::Stretch);
    tableArrivals->horizontalHeader()->setSectionResizeMode(3, QHeaderView::Stretch);
    tableArrivals->verticalHeader()->setVisible(false);
}

bool BusArrivalWidget::setSnapshot(const BusStopSnapshot& snapshot)
{
    if (snapshot.services.isEmpty())
    {
        clear();
        return false;
    }

    tableArrivals->setRowCount(snapshot.services.size());
    for (int row = 0; row < snapshot.services.size(); ++row)
    {
        const BusService& service = snapshot.services.at(row);

        auto* serviceItem = new QTableWidgetItem(service.serviceNo);
        serviceItem->setTextAlignment(Qt::AlignCenter);
        tableArrivals->setItem(row, 0, serviceItem);

        const ArrivalBus buses[] = {service.nextBus, service.nextBus2, service.nextBus3};
        for (int col = 0; col < 3; ++col)
        {
            auto* item = new QTableWidgetItem(formatArrivalTime(buses[col].estimatedArrival));
            item->setTextAlignment(Qt::AlignCenter);
            tableArrivals->setItem(row, col + 1, item);
        }
    }

    return true;
}

void BusArrivalWidget::clear()
{
    tableArrivals->setRowCount(0);
}

void BusArrivalWidget::retranslate()
{
    retranslateUi(this);
}

QString BusArrivalWidget::formatArrivalTime(const QString& estimatedArrival)
{
    // LTA returns ISO 8601 (e.g. "2026-09-08T14:35:00+08:00"). Show a compact
    // local "HH:MM" plus a relative "in X min" so the ETA is obvious at a
    // glance; fall back to the raw string if it can't be parsed.
    const QDateTime dt = QDateTime::fromString(estimatedArrival, Qt::ISODate);
    if (dt.isValid())
    {
        const QString time = dt.toString("HH:mm");
        const qint64 seconds = QDateTime::currentDateTime().secsTo(dt);
        if (seconds < 0)
        {
            return time + QStringLiteral(" (now)");
        }
        const int minutes = static_cast<int>((seconds + 59) / 60);  // round up
        return time + QStringLiteral(" (in %1 min)").arg(minutes);
    }
    return estimatedArrival.isEmpty() ? QStringLiteral("-") : estimatedArrival;
}
