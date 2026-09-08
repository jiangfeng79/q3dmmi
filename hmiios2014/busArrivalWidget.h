#ifndef BUSARRIVALWIDGET_H
#define BUSARRIVALWIDGET_H

#include <QWidget>

#include "busTracker.h"
#include "ui_busArrival.h"

// ---------------------------------------------------------------------------
// BusArrivalWidget
//
// Shows the live bus arrival times for a single bus stop. The table has one
// row per bus service and one column per arriving bus (1st / 2nd / 3rd), with
// each cell holding that bus's estimated arrival time in a friendly format.
//
// This widget lives inside a QDockWidget (dockBusArrival) in hmiios2014.ui,
// so the dock provides the floating/docking behaviour. The dock is hidden
// when there is no arrival data and shown whenever a new snapshot arrives.
// ---------------------------------------------------------------------------
class BusArrivalWidget : public QWidget, Ui::BusArrivalWidget
{
    Q_OBJECT

public:
    explicit BusArrivalWidget(QWidget* parent = nullptr);
    ~BusArrivalWidget() override = default;

    // Populate the table from a snapshot. Returns true if there is data to
    // show (i.e. the dock should be visible).
    bool setSnapshot(const BusStopSnapshot& snapshot);

    // Clear the table (e.g. when bus info is cleared).
    void clear();

    // Re-apply translations (called on language change).
    void retranslate();

private:
    // Format an LTA estimated-arrival string (ISO 8601) into a friendly
    // "HH:MM" time, or "-" when empty.
    static QString formatArrivalTime(const QString& estimatedArrival);
};

#endif  // BUSARRIVALWIDGET_H
