#include "flightLayerParser.h"

#include <math.h>

#include "geoTransform.h"

// ---------------------------------------------------------------------------
// FlightLayerParser
//
// The input is WGS84 (longitude/latitude), so the WGS84 build transform is
// always used (independent of Options::useWgs84BuildTransform). The base
// property (center/extent/scale) comes from the coastal layer.
// ---------------------------------------------------------------------------

LayerGeometry FlightLayerParser::parse(const Options& a_options)
{
    LayerGeometry geo;
    geo.property = a_options.baseProperty;

    if (m_kind == Trails)
    {
        // Each aircraft's position history becomes a line (SHPT_ARC). The
        // trail is bounded to the most recent kMaxTrailSamples samples.
        const int kMaxTrailSamples = 90;

        // First pass: count the total number of trail vertices.
        int totalVerts = 0;
        for (auto it = m_table.constBegin(); it != m_table.constEnd(); ++it)
        {
            const int n = it.value().history.size();
            if (n < 2)
            {
                continue;
            }
            const int start = qMax(0, n - kMaxTrailSamples);
            totalVerts += (n - start);
        }

        geo.property.totalNumberOfVertex = totalVerts;
        geo.vertices.resize(totalVerts);
        geo.rings.push_back(0);
        geo.lineIndices.reserve(totalVerts + m_table.size());

        int idx = 0;
        for (auto it = m_table.constBegin(); it != m_table.constEnd(); ++it)
        {
            const QList<PositionSample>& hist = it.value().history;
            const int n = hist.size();
            if (n < 2)
            {
                continue;
            }
            const int start = qMax(0, n - kMaxTrailSamples);
            const int count = n - start;

            geo.rings.push_back(idx);
            geo.renderType.push_back(SHPT_ARC);

            for (int i = 0; i < count; ++i)
            {
                Vertex v;
                v.x = X_WGS84_BUILD_COORD_TO_MAP_COORD(hist[start + i].lon, geo.property);
                v.y = Y_WGS84_BUILD_COORD_TO_MAP_COORD(hist[start + i].lat, geo.property);
                v.z = (float)a_options.layerDepth;
                geo.vertices[idx] = v;
                geo.lineIndices.push_back(static_cast<unsigned int>(idx));
                ++idx;
            }

            geo.rings.push_back(idx);
            geo.renderType.push_back(SHPT_ARC);
            geo.lineIndices.push_back(0xFFFFFFFF);  // primitive restart
        }
    }
    else  // Markers
    {
        // A top-down airplane silhouette in local map units, nose pointing +Y
        // (north). Symmetric about the fuselage (Y) axis.
        static const float kPlaneOutline[][2] = {
            {0.00f, 1.00f},   {0.15f, 0.60f},   {0.15f, 0.15f},   {1.10f, -0.20f},  {1.10f, -0.35f},  {0.15f, -0.30f},
            {0.15f, -0.75f},  {0.55f, -0.95f},  {0.55f, -1.05f},  {0.00f, -1.00f},  {-0.55f, -1.05f}, {-0.55f, -0.95f},
            {-0.15f, -0.75f}, {-0.15f, -0.30f}, {-1.10f, -0.35f}, {-1.10f, -0.20f}, {-0.15f, 0.15f},  {-0.15f, 0.60f},
        };
        const int kPlaneVerts = int(sizeof(kPlaneOutline) / sizeof(kPlaneOutline[0]));
        // Half-extent in map units. The projection matrix scales map space by
        // (property.scale * m_scale), so to keep a constant ~8 px screen size
        // (at the default coastal scale of 0.02, where 1 map unit ~ 0.02 px)
        // the map-space size must be INVERSELY proportional to the zoom
        // factor: 8 px = kPlaneSize * 0.02 * m_scale.
        const float kPlaneSize = 400.0f / qMax(m_scale, 1e-6f);

        // Each polygon is drawn as a GL_TRIANGLE_FAN, so the first vertex must
        // be the fan center (the aircraft position) followed by the outline.
        const int totalVerts = m_table.size() * kPlaneVerts;
        geo.property.totalNumberOfVertex = totalVerts;
        geo.vertices.resize(totalVerts);
        geo.rings.push_back(0);

        int idx = 0;
        for (auto it = m_table.constBegin(); it != m_table.constEnd(); ++it)
        {
            const TrackedAircraft& t = it.value();
            const Aircraft& ac = t.latest;

            // Heading (radians, clockwise from north) from the last two samples.
            double heading = 0.0;
            if (t.history.size() >= 2)
            {
                const PositionSample& p0 = t.history[t.history.size() - 2];
                const PositionSample& p1 = t.history[t.history.size() - 1];
                const double dLat = p1.lat - p0.lat;
                const double dLon = (p1.lon - p0.lon) * cos(p0.lat * M_PI / 180.0);
                if (fabs(dLat) > 1e-9 || fabs(dLon) > 1e-9)
                {
                    heading = atan2(dLon, dLat);
                }
            }
            const double cosH = cos(heading);
            const double sinH = sin(heading);

            // Aircraft center in map space.
            const float cx = X_WGS84_BUILD_COORD_TO_MAP_COORD(ac.lon, geo.property);
            const float cy = Y_WGS84_BUILD_COORD_TO_MAP_COORD(ac.lat, geo.property);

            geo.rings.push_back(idx);
            geo.renderType.push_back(SHPT_POLYGON);

            for (int i = 0; i < kPlaneVerts; ++i)
            {
                const float lx = kPlaneOutline[i][0] * kPlaneSize;
                const float ly = kPlaneOutline[i][1] * kPlaneSize;
                // Rotate clockwise by heading (map space: X east, Y north).
                Vertex v;
                v.x = cx + (float)(lx * cosH + ly * sinH);
                v.y = cy + (float)(-lx * sinH + ly * cosH);
                v.z = (float)a_options.layerDepth;
                geo.vertices[idx] = v;
                ++idx;
            }

            geo.rings.push_back(idx);
            geo.renderType.push_back(SHPT_POLYGON);

            // Label: callsign (+ barometric altitude in feet).
            Label label;
            label.longitude = ac.lon;
            label.latitude = ac.lat;
            label.angle = 0.0f;
            QString text = ac.callsign;
            if (ac.altBaro > 0)
            {
                text += " " + QString::number(ac.altBaro);
            }
            label.text = text.toStdString();
            geo.labels.push_back(label);
        }
    }

    return geo;
}
