#include "busLayerParser.h"
#include <math.h>
#include <algorithm>
#include "geoTransform.h"

// ---------------------------------------------------------------------------
// BusLayerParser
//
// Converts bus routes and live vehicle positions into map geometry using
// WGS84 build coordinate transforms. Each Kind produces exactly one
// primitive type in LayerGeometry, matching what the generic renderer
// expects:
//   RouteLines -> SHPT_ARC rings + lineIndices, drawn via drawLayerLines().
//   RouteStops -> SHPT_POINT ring + lineIndices, drawn via drawLayer().
//   Vehicles   -> SHPT_POLYGON rings (no indices), drawn via drawLayerAndFill().
// ---------------------------------------------------------------------------

static Vertex makeVertex(double lon, double lat, int depth, const MapProperty& property)
{
    Vertex v;
    v.x = X_WGS84_BUILD_COORD_TO_MAP_COORD(lon, property);
    v.y = Y_WGS84_BUILD_COORD_TO_MAP_COORD(lat, property);
    v.z = static_cast<float>(depth);
    return v;
}

LayerGeometry BusLayerParser::parse(const Options& a_options)
{
    LayerGeometry geo;
    geo.property = a_options.baseProperty;

    if (m_kind == RouteLines)
    {
        return buildRouteLines(a_options);
    }
    else if (m_kind == RouteStops)
    {
        if (m_routes.isEmpty())
        {
            return geo;
        }

        geo.rings.push_back(0);
        int idx = 0;
        for (const BusRoute& route : m_routes)
        {
            for (const RouteStop& rstop : route.stops)
            {
                if (rstop.stop.latitude == 0.0 && rstop.stop.longitude == 0.0)
                {
                    continue;
                }
                geo.vertices.push_back(
                    makeVertex(rstop.stop.longitude, rstop.stop.latitude, a_options.layerDepth, geo.property));
                geo.lineIndices.push_back(static_cast<unsigned int>(idx));
                geo.lineIndices.push_back(0xFFFFFFFF);  // each stop is its own point primitive
                ++idx;
            }
        }

        if (idx > 0)
        {
            geo.rings.push_back(idx);
            geo.renderType.push_back(SHPT_POINT);
        }
        geo.property.totalNumberOfVertex = static_cast<int>(geo.vertices.size());
    }
    else  // Vehicles and vehicle windshields
    {
        m_busInfos.clear();

        for (const BusService& service : m_snapshot.services)
        {
            auto addBus = [this, &service](const ArrivalBus& bus, const QString& labelPrefix) {
                if (bus.latitude == 0.0 && bus.longitude == 0.0)
                {
                    return;
                }
                TrackedBusInfo info;
                info.serviceNo = service.serviceNo;
                info.bus = bus;
                info.labelPrefix = labelPrefix;
                m_busInfos.push_back(info);
            };

            addBus(service.nextBus, QStringLiteral("Next"));
            addBus(service.nextBus2, QStringLiteral("2nd"));
            addBus(service.nextBus3, QStringLiteral("3rd"));
        }

        if (m_busInfos.empty())
        {
            return geo;
        }

        // Maintain a crisp ~20px x 36px screen size regardless of zoom.
        const float currentScale = qMax(geo.property.scale * m_scale, 1e-6f);
        const float busW = 6.0f / currentScale;
        const float busH = 10.8f / currentScale;

        // 2D top-down bus outline (nose pointing +Y/North), closed as a fan.
        static const float kOutline[][2] = {
            { 0.00f,  1.00f }, { 0.50f,  0.95f }, { 0.85f,  0.75f }, { 0.85f,  0.50f },
            { 0.85f, -0.75f }, { 0.85f, -0.90f }, { 0.50f, -1.00f }, { 0.00f, -1.00f },
            {-0.50f, -1.00f }, {-0.85f, -0.90f }, {-0.85f, -0.75f }, {-0.85f,  0.50f },
            {-0.85f,  0.75f }, {-0.50f,  0.95f },
        };
        const int kOutlineVerts = static_cast<int>(sizeof(kOutline) / sizeof(kOutline[0]));
        static const float kWindshield[][2] = {
            {-0.55f, 0.80f}, {-0.25f, 0.87f}, {0.00f, 0.89f}, {0.25f, 0.87f}, {0.55f, 0.80f},
            {0.62f, 0.68f}, {0.50f, 0.51f}, {0.00f, 0.47f}, {-0.50f, 0.51f}, {-0.62f, 0.68f},
        };
        const int kWindshieldVerts = static_cast<int>(sizeof(kWindshield) / sizeof(kWindshield[0]));

        geo.rings.push_back(0);
        int idx = 0;
        for (const auto& info : m_busInfos)
        {
            float cx = X_WGS84_BUILD_COORD_TO_MAP_COORD(info.bus.longitude, geo.property);
            float cy = Y_WGS84_BUILD_COORD_TO_MAP_COORD(info.bus.latitude, geo.property);

            // Heading (radians, clockwise from north) from the tracker's
            // snapshot-to-snapshot comparison.
            const double heading = info.bus.heading;
            const double cosH = cos(heading);
            const double sinH = sin(heading);

            auto transformPoint = [cx, cy, busW, busH, cosH, sinH](float localX, float localY) {
                float scaledVx = localX * busW;
                float scaledVy = localY * busH;
                // Rotate clockwise by heading (map space: X east, Y north).
                return Vertex{cx + (float)(scaledVx * cosH + scaledVy * sinH),
                              cy + (float)(-scaledVx * sinH + scaledVy * cosH), 0.0f};
            };

            const float (*shape)[2] = m_kind == VehicleWindshields ? kWindshield : kOutline;
            const int shapeVerts = m_kind == VehicleWindshields ? kWindshieldVerts : kOutlineVerts;

            // Fan: center vertex + outline, closed by repeating the first outline vertex.
            Vertex center{cx, cy, static_cast<float>(a_options.layerDepth)};
            geo.vertices.push_back(center);
            ++idx;
            for (int v = 0; v < shapeVerts; ++v)
            {
                Vertex p = transformPoint(shape[v][0], shape[v][1]);
                p.z = static_cast<float>(a_options.layerDepth);
                geo.vertices.push_back(p);
                ++idx;
            }
            Vertex closing = transformPoint(shape[0][0], shape[0][1]);
            closing.z = static_cast<float>(a_options.layerDepth);
            geo.vertices.push_back(closing);
            ++idx;

            geo.rings.push_back(idx);
            geo.renderType.push_back(SHPT_POLYGON);

            // Label: service number, position in queue, ETA, load and type.
            // Only the main vehicle layer emits labels (the windshield layer
            // reuses the same geometry pass and would otherwise duplicate them).
            if (m_kind == Vehicles)
            {
                Label label;
                label.longitude = info.bus.longitude;
                label.latitude = info.bus.latitude;
                label.angle = 0.0f;

                int mins = -1;
                if (!info.bus.estimatedArrival.isEmpty())
                {
                    const QDateTime dt = QDateTime::fromString(info.bus.estimatedArrival, Qt::ISODate);
                    if (dt.isValid())
                    {
                        const qint64 secs = QDateTime::currentDateTime().secsTo(dt);
                        mins = secs > 0 ? static_cast<int>(secs / 60) : 0;
                    }
                }
                const QString eta = (mins >= 0) ? QString("%1m").arg(mins) : QStringLiteral("Arr");
                const QString loadStr = info.bus.load.isEmpty() ? QStringLiteral("SEA") : info.bus.load;
                const QString typeStr = info.bus.type.isEmpty() ? QStringLiteral("SD") : info.bus.type;
                label.text = QString("Svc %1 (%2): %3 [%4,%5]")
                                 .arg(info.serviceNo)
                                 .arg(info.labelPrefix)
                                 .arg(eta)
                                 .arg(loadStr)
                                 .arg(typeStr)
                                 .toStdString();
                geo.labels.push_back(label);
            }
        }

        geo.property.totalNumberOfVertex = static_cast<int>(geo.vertices.size());
    }

    return geo;
}

// ---------------------------------------------------------------------------
// RouteLines: route between consecutive bus stops along the road network.
// ---------------------------------------------------------------------------

int BusLayerParser::appendPolylineVertices(LayerGeometry& geo, const RoadGraph::Polyline& path,
                                            int layerDepth, const MapProperty& property) const
{
    int added = 0;
    // Convert the Web Mercator points into map-space vertices. The first point
    // is the previous stop, which the caller has already emitted, so it is
    // skipped to avoid a duplicate (zero-length) segment.
    for (size_t i = 1; i < path.size(); ++i)
    {
        const QPointF& p = path[i];
        Vertex v;
        v.x = (float)(p.x() - property.centerX * property.mapBuildScale);
        v.y = (float)(p.y() - property.centerY * property.mapBuildScale);
        v.z = static_cast<float>(layerDepth);
        geo.vertices.push_back(v);
        geo.lineIndices.push_back(static_cast<unsigned int>(geo.vertices.size() - 1));

        ++added;
    }

    return added;
}

LayerGeometry BusLayerParser::buildRouteLines(const Options& a_options) const
{
    LayerGeometry geo;
    geo.property = a_options.baseProperty;

    if (m_routes.isEmpty())
    {
        return geo;
    }

    // The road graph is static, so the routed geometry only depends on the
    // set of routes. Cache it and reuse across rebuilds (which happen on every
    // zoom change) to avoid re-running Dijkstra each time.
    QString cacheKey;
    for (const BusRoute& route : m_routes)
    {
        cacheKey += route.serviceNo;
        cacheKey += QString::number(route.direction);
        for (const RouteStop& rstop : route.stops)
        {
            cacheKey += QString::number(rstop.stop.longitude, 'f', 6);
            cacheKey += QString::number(rstop.stop.latitude, 'f', 6);
        }
        cacheKey += QLatin1Char('|');
    }
    if (cacheKey == m_routeCacheKey && !m_routeCache.vertices.empty())
    {
        return m_routeCache;
    }

    const bool useRoads = m_roadGraph && m_roadGraph->isValid();
    // How far (metres) a bus stop may be from a road vertex and still be
    // considered "on" that road for snapping purposes.
    const double snapRadius = 200.0;

    // Convert a WGS84 position to Web Mercator metres (the road graph's space).
    auto toMercator = [&](double lon, double lat) -> QPointF {
        return QPointF(lon * geo.property.mapBuildScale,
                       WGS84_TO_WGS84WEBMERCATOR(lat) * geo.property.mapBuildScale);
    };

    geo.rings.push_back(0);
    int idx = 0;
    for (const BusRoute& route : m_routes)
    {
        // Collect the valid stops (non-zero coordinates) for this route.
        QList<const RouteStop*> valid;
        for (const RouteStop& rstop : route.stops)
        {
            if (rstop.stop.latitude == 0.0 && rstop.stop.longitude == 0.0)
            {
                continue;
            }
            valid.append(&rstop);
        }

        // Snap each stop to a road node, biased toward the direction of
        // travel. Looking up to 2 stops ahead gives a stable heading so a stop
        // on a divided road snaps to the carriageway the bus is heading down
        // rather than the opposite one.
        QList<int> nodes(valid.size(), -1);
        if (useRoads)
        {
            for (int i = 0; i < valid.size(); ++i)
            {
                const QPointF p = toMercator(valid[i]->stop.longitude, valid[i]->stop.latitude);
                double dx = 0.0, dy = 0.0;
                if (i + 2 < valid.size())
                {
                    const QPointF q = toMercator(valid[i + 2]->stop.longitude, valid[i + 2]->stop.latitude);
                    dx = q.x() - p.x();
                    dy = q.y() - p.y();
                }
                else if (i + 1 < valid.size())
                {
                    const QPointF q = toMercator(valid[i + 1]->stop.longitude, valid[i + 1]->stop.latitude);
                    dx = q.x() - p.x();
                    dy = q.y() - p.y();
                }
                else if (i - 1 >= 0)
                {
                    const QPointF q = toMercator(valid[i - 1]->stop.longitude, valid[i - 1]->stop.latitude);
                    dx = p.x() - q.x();
                    dy = p.y() - q.y();
                }
                int node = m_roadGraph->snapToward(p.x(), p.y(), dx, dy, snapRadius);
                if (node < 0)
                {
                    node = m_roadGraph->nearestNode(p.x(), p.y());
                }
                nodes[i] = node;
            }
        }

        int start = idx;
        for (int i = 0; i < valid.size(); ++i)
        {
            const RouteStop& rstop = *valid[i];

            if (i == 0)
            {
                // First stop of the route: emit it directly.
                geo.vertices.push_back(
                    makeVertex(rstop.stop.longitude, rstop.stop.latitude, a_options.layerDepth, geo.property));
                geo.lineIndices.push_back(static_cast<unsigned int>(idx));
                ++idx;
            }
            else
            {
                // Route from the previous stop to this one along the road
                // network (or straight line as a fallback).
                const QPointF p0 = toMercator(valid[i - 1]->stop.longitude, valid[i - 1]->stop.latitude);
                const QPointF p1 = toMercator(rstop.stop.longitude, rstop.stop.latitude);
                RoadGraph::Polyline path;
                if (useRoads && nodes[i - 1] >= 0 && nodes[i] >= 0)
                {
                    path = m_roadGraph->routeBetween(nodes[i - 1], nodes[i]);
                    if (path.size() >= 2)
                    {
                        // Pin the endpoints to the actual stop positions so the
                        // line passes through every stop marker.
                        path.front() = p0;
                        path.back() = p1;
                    }
                }
                if (path.size() < 2)
                {
                    path = {p0, p1};
                }

                idx += appendPolylineVertices(geo, path, a_options.layerDepth, geo.property);
            }

            // Label: direction, stop description and bus stop code.
            Label label;
            label.longitude = rstop.stop.longitude;
            label.latitude = rstop.stop.latitude;
            label.angle = 0.0f;
            const QString desc =
                rstop.stop.description.isEmpty() ? rstop.stop.busStopCode : rstop.stop.description;
            label.text =
                QString("D%1: %2 (%3)").arg(route.direction).arg(desc, rstop.stop.busStopCode).toStdString();
            geo.labels.push_back(label);
        }

        if (valid.size() >= 2)
        {
            geo.rings.push_back(idx);
            geo.renderType.push_back(SHPT_ARC);
            geo.lineIndices.push_back(0xFFFFFFFF);  // primitive restart between directions
        }
        else
        {
            // Not enough valid points for a line strip; drop what was pushed.
            geo.vertices.resize(start);
            geo.lineIndices.resize(start);
            idx = start;
        }
    }

    geo.property.totalNumberOfVertex = static_cast<int>(geo.vertices.size());

    m_routeCacheKey = cacheKey;
    m_routeCache = geo;
    return geo;
}
