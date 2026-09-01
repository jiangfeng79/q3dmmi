#include "busLayerParser.h"
#include <math.h>
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

// Nearest-segment heading lookup: finds the route segment (preferring the
// bus's own service number) closest to (busMx, busMy) and returns its unit
// direction, used to orient the vehicle polygon along the road.
static void findBusHeading(const QList<BusRoute>& routes, const MapProperty& property, const QString& serviceNo,
                           const QString& originCode, const QString& destinationCode, float busMx, float busMy,
                           float& outDirX, float& outDirY)
{
    outDirX = 0.0f;
    outDirY = 1.0f;

    float minDistSq = 1e30f;
    bool foundSegment = false;

    auto searchRoutes = [&](bool matchService, bool matchOrigin, bool matchDestination) {
        for (const BusRoute& route : routes)
        {
            if (matchService && !serviceNo.isEmpty() && route.serviceNo.compare(serviceNo, Qt::CaseInsensitive) != 0)
            {
                continue;
            }

            const auto& stops = route.stops;
            if (stops.isEmpty() ||
                (matchOrigin && (!originCode.isEmpty() && stops.first().stop.busStopCode != originCode)) ||
                (matchDestination &&
                 (!destinationCode.isEmpty() && stops.last().stop.busStopCode != destinationCode)))
            {
                continue;
            }
            for (int i = 0; i < stops.size() - 1; ++i)
            {
                const auto& s1 = stops[i].stop;
                const auto& s2 = stops[i + 1].stop;
                if ((s1.latitude == 0.0 && s1.longitude == 0.0) || (s2.latitude == 0.0 && s2.longitude == 0.0))
                {
                    continue;
                }

                float x1 = X_WGS84_BUILD_COORD_TO_MAP_COORD(s1.longitude, property);
                float y1 = Y_WGS84_BUILD_COORD_TO_MAP_COORD(s1.latitude, property);
                float x2 = X_WGS84_BUILD_COORD_TO_MAP_COORD(s2.longitude, property);
                float y2 = Y_WGS84_BUILD_COORD_TO_MAP_COORD(s2.latitude, property);

                float vx = x2 - x1;
                float vy = y2 - y1;
                float wx = busMx - x1;
                float wy = busMy - y1;

                float c1 = wx * vx + wy * vy;
                float c2 = vx * vx + vy * vy;
                if (c2 < 1e-12f)
                {
                    continue;
                }

                float t = qBound(0.0f, c1 / c2, 1.0f);
                float projX = x1 + t * vx;
                float projY = y1 + t * vy;
                float dx = busMx - projX;
                float dy = busMy - projY;
                float distSq = dx * dx + dy * dy;

                if (distSq < minDistSq)
                {
                    minDistSq = distSq;
                    float segLen = sqrtf(c2);
                    if (segLen > 1e-6f)
                    {
                        outDirX = vx / segLen;
                        outDirY = vy / segLen;
                        foundSegment = true;
                    }
                }
            }
        }
    };

    searchRoutes(true, true, true);
    if (!foundSegment)
    {
        searchRoutes(true, false, true);
    }
    if (!foundSegment)
    {
        searchRoutes(true, true, false);
    }
    if (!foundSegment)
    {
        searchRoutes(true, false, false);
    }
    if (!foundSegment)
    {
        searchRoutes(false, false, false);
    }
}

LayerGeometry BusLayerParser::parse(const Options& a_options)
{
    LayerGeometry geo;
    geo.property = a_options.baseProperty;

    if (m_kind == RouteLines)
    {
        if (m_routes.isEmpty())
        {
            return geo;
        }

        geo.rings.push_back(0);
        int idx = 0;
        for (const BusRoute& route : m_routes)
        {
            int start = idx;
            for (const RouteStop& rstop : route.stops)
            {
                if (rstop.stop.latitude == 0.0 && rstop.stop.longitude == 0.0)
                {
                    continue;
                }
                geo.vertices.push_back(
                    makeVertex(rstop.stop.longitude, rstop.stop.latitude, a_options.layerDepth, geo.property));
                geo.lineIndices.push_back(static_cast<unsigned int>(idx));
                ++idx;
            }

            if (idx - start >= 2)
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

            float dirX = 0.0f, dirY = 1.0f;
            findBusHeading(m_routes, geo.property, info.serviceNo, info.bus.originCode, info.bus.destinationCode, cx, cy,
                           dirX, dirY);

            auto transformPoint = [cx, cy, busW, busH, dirX, dirY](float localX, float localY) {
                float scaledVx = localX * busW;
                float scaledVy = localY * busH;
                return Vertex{cx + (scaledVx * dirY + scaledVy * dirX), cy + (-scaledVx * dirX + scaledVy * dirY),
                              0.0f};
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
        }

        geo.property.totalNumberOfVertex = static_cast<int>(geo.vertices.size());
    }

    return geo;
}
