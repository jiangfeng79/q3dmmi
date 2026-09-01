#ifndef BUSLAYERPARSER_H
#define BUSLAYERPARSER_H

#include <QList>
#include <vector>

#include "busRoute.h"
#include "busTracker.h"
#include "layerParser.h"

// ---------------------------------------------------------------------------
// BusLayerParser
//
// Turns bus routes and live bus arrival snapshots into renderable map
// geometry, following the same convention as every other MapLayer: points,
// lines and polygons are plain LayerGeometry rings that the generic renderer
// (drawLayer / drawLayerLines / drawLayerAndFill) already knows how to draw.
// Three kinds are supported, each producing exactly one primitive type:
//   - RouteLines: stop sequences per direction as line strips (SHPT_ARC),
//                 drawn with the thickened line shader (like flight trails).
//   - RouteStops: individual stop nodes as points (SHPT_POINT).
//   - Vehicles:   arriving bus positions as filled polygons (SHPT_POLYGON),
//                 oriented along their route heading (like flight markers).
// ---------------------------------------------------------------------------
class BusLayerParser : public LayerParser
{
public:
    enum Kind
    {
        RouteLines,
        RouteStops,
        Vehicles,
        VehicleWindshields
    };

    struct TrackedBusInfo
    {
        QString serviceNo;
        ArrivalBus bus;
        QString labelPrefix;
    };

    explicit BusLayerParser(Kind a_kind) : m_kind(a_kind) {}
    ~BusLayerParser() override = default;

    void setRoutes(const QList<BusRoute>& routes) { m_routes = routes; }
    void setSnapshot(const BusStopSnapshot& snapshot) { m_snapshot = snapshot; }
    void setScale(float a_scale) override { m_scale = a_scale; }

    LayerGeometry parse(const Options& a_options) override;
    void freeMemory() override {}

    // Populated by the Vehicles parse(); used to draw per-bus HUD labels.
    const std::vector<TrackedBusInfo>& getBusInfos() const { return m_busInfos; }

private:
    Kind m_kind;
    QList<BusRoute> m_routes;
    BusStopSnapshot m_snapshot;
    float m_scale = 1.0f;

    std::vector<TrackedBusInfo> m_busInfos;
};

#endif  // BUSLAYERPARSER_H
