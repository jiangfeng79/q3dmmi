#ifndef FLIGHTLAYERPARSER_H
#define FLIGHTLAYERPARSER_H

#include <QHash>

#include "flightTracker.h"
#include "layerParser.h"

// ---------------------------------------------------------------------------
// FlightLayerParser
//
// Turns the live airflight tracking table (QHash<QString, TrackedAircraft>)
// into renderable map geometry. Two kinds are produced:
//   - Trails:  each aircraft's position history becomes a line (SHPT_ARC).
//   - Markers: each aircraft's current position becomes a polygon
//              (SHPT_POLYGON) plane silhouette oriented by its heading, plus
//              a callsign label.
//
// The input is WGS84 (longitude/latitude), so the WGS84 build transform is
// always used (independent of Options::useWgs84BuildTransform).
// ---------------------------------------------------------------------------
class FlightLayerParser : public LayerParser
{
public:
    enum Kind
    {
        Trails,
        Markers
    };

    explicit FlightLayerParser(Kind a_kind) : m_kind(a_kind) {}
    ~FlightLayerParser() override = default;

    // Update the tracking table consumed by the next parse().
    void setTable(const QHash<QString, TrackedAircraft>& a_table) { m_table = a_table; }
    void setScale(const float a_scale) { m_scale = a_scale; }

    LayerGeometry parse(const Options& a_options) override;
    void freeMemory() override {}

private:
    Kind m_kind;
    QHash<QString, TrackedAircraft> m_table;
    float m_scale = 1;
};

#endif  // FLIGHTLAYERPARSER_H
