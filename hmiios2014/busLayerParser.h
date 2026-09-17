#ifndef BUSLAYERPARSER_H
#define BUSLAYERPARSER_H

#include <QList>
#include <vector>

#include <atomic>
#include <memory>

#include "busRoute.h"
#include "busTracker.h"
#include "layerParser.h"
#include "roadGraph.h"

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

    // Thread-safe input setters. The values are published as immutable heap
    // objects behind a std::atomic pointer (release store), so parse() can
    // read them from a worker thread without a lock while the UI thread
    // updates them. See the "Thread-safety" note below.
    void setRoutes(const QList<BusRoute>& routes) { publishRoutes(routes); }
    void setSnapshot(const BusStopSnapshot& snapshot) { publishSnapshot(snapshot); }
    void setScale(float a_scale) { m_scale.store(a_scale, std::memory_order_relaxed); }

    // Optional road network used by RouteLines to route between consecutive
    // bus stops along real roads instead of straight lines. When null (or
    // invalid), RouteLines falls back to connecting stops directly.
    void setRoadGraph(const RoadGraph* a_roadGraph) { m_roadGraph = a_roadGraph; }

    LayerGeometry parse(const Options& a_options) override;
    void freeMemory() override {}

    // Populated by the Vehicles parse(); used to draw per-bus HUD labels.
    // Published atomically so the UI thread can read it while a worker thread
    // is rebuilding it. Returns a copy (the list is small and TrackedBusInfo
    // holds COW QStrings) so the caller is never left with a dangling
    // reference if a concurrent publish replaces the data.
    std::vector<TrackedBusInfo> getBusInfos() const
    {
        const auto p = m_busInfos.load(std::memory_order_acquire);
        return p ? *p : std::vector<TrackedBusInfo>{};
    }

private:
    // Build the RouteLines geometry for the given routes, routing between
    // consecutive stops along the road network when available.
    LayerGeometry buildRouteLines(const Options& a_options,
                                  const std::shared_ptr<const QList<BusRoute>>& routes) const;

    // Emit the vertices of a Web-Mercator polyline into map space, skipping
    // the first point (which the caller has already emitted). Each vertex gets
    // a sequential line index so the renderer draws it as a continuous strip.
    int appendPolylineVertices(LayerGeometry& geo, const RoadGraph::Polyline& path,
                                int layerDepth, const MapProperty& property) const;

    // ------------------------------------------------------------------
    // Thread-safety
    //
    // parse() may run on a worker thread (MapLayer::buildLayer) while the UI
    // thread calls setRoutes()/setSnapshot()/setScale(). To stay lock-free we
    // publish each input as an immutable heap object behind a std::atomic
    // pointer: the writer builds a fresh object and stores its pointer with a
    // release store; the reader loads the pointer with an acquire load and
    // copies out what it needs. The old object is deleted only after the
    // reader has released its reference (refcounted shared_ptr), so a reader
    // never observes a half-updated value and never frees memory it still
    // uses.
    // ------------------------------------------------------------------
    void publishRoutes(const QList<BusRoute>& routes);
    void publishSnapshot(const BusStopSnapshot& snapshot);
    void publishBusInfos(std::vector<TrackedBusInfo> infos);

    Kind m_kind;

    // Immutable input snapshots, published atomically (see note above).
    std::atomic<std::shared_ptr<const QList<BusRoute>>> m_routes{std::make_shared<const QList<BusRoute>>()};
    std::atomic<std::shared_ptr<const BusStopSnapshot>> m_snapshot{std::make_shared<const BusStopSnapshot>()};
    std::atomic<float> m_scale{1.0f};
    const RoadGraph* m_roadGraph = nullptr;  // set once before any parse()

    // Populated by the Vehicles parse(); published atomically for the UI.
    std::atomic<std::shared_ptr<const std::vector<TrackedBusInfo>>> m_busInfos{
        std::make_shared<const std::vector<TrackedBusInfo>>()};

    // Cached road-routed geometry for RouteLines, keyed by the stop sequence.
    // The road graph is static, so a given route's geometry only changes when
    // its stops change; caching avoids re-running Dijkstra on every rebuild.
    // Only ever touched by a single parse() call at a time (see note above).
    mutable QString m_routeCacheKey;
    mutable LayerGeometry m_routeCache;
};

#endif  // BUSLAYERPARSER_H
