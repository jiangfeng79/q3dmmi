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
