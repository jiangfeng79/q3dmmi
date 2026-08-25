#include "layerParser.h"
#include "geoTransform.h"

#include <math.h>
#include <cstdlib>
#include <ctime>

// ---------------------------------------------------------------------------
// ShapefileLayerParser
// ---------------------------------------------------------------------------
ShapefileLayerParser::ShapefileLayerParser(const QString& a_fileName, const QString& a_layerName)
    : m_fileName(a_fileName)
    , m_layerName(a_layerName)
{
}

ShapefileLayerParser::~ShapefileLayerParser()
{
    freeMemory();
}

void ShapefileLayerParser::freeMemory()
{
    m_shapeFileReader.freeMemory();
    m_dbfFileReader.freeMemory();
}

LayerGeometry ShapefileLayerParser::parse(const Options& a_options)
{
    LayerGeometry geo;
    geo.property = a_options.baseProperty;

    // Read the raw source layer (shapefile + dbf).
    const QString l_qsShpFileName = m_fileName + QString(".shp");
    const QString l_qsDbfFileName = m_fileName + QString(".dbf");
    if (m_layerName.isEmpty())
    {
        m_dbfFileReader.read(l_qsDbfFileName.toStdString().c_str());
        m_shapeFileReader.read(l_qsShpFileName.toStdString().c_str());
    }
    else
    {
        m_dbfFileReader.readLayer(l_qsDbfFileName.toStdString().c_str(), m_layerName.toStdString().c_str());
        m_shapeFileReader.readLayer(l_qsShpFileName.toStdString().c_str(), m_dbfFileReader);
    }

    ShpReader::ShpEntity* l_shapeEntity = m_shapeFileReader.getEntity();
    DBFReader::DBFEntity* l_dbfEntity = m_dbfFileReader.getEntity();

    // Base layer: derive the map center/extent/scale from the data itself.
    if (a_options.isBaseLayer)
    {
        geo.property.centerX = 103.84810;
        geo.property.centerY = (float)WGS84_TO_WGS84WEBMERCATOR(1.35059);
        geo.property.width = (float)(m_shapeFileReader.getShpMaxX() - m_shapeFileReader.getShpMinX());
        geo.property.height = (float)(m_shapeFileReader.getShpMaxY() - m_shapeFileReader.getShpMinY());
        geo.property.scale = 0.02f;
        geo.property.totalNumberOfVertex = 0;
    }

    geo.rings.push_back(0);
    int l_iTotalVertex = 0;
    if (l_shapeEntity)
    {
        for (int n = 0; n < m_shapeFileReader.getNumberOfEntity(); ++n)
            geo.property.totalNumberOfVertex += l_shapeEntity[n].totalVertex;

        geo.vertices.resize(geo.property.totalNumberOfVertex);
        geo.lineIndices.reserve(geo.property.totalNumberOfVertex + geo.rings.size());

        for (int n = 0; n < m_shapeFileReader.getNumberOfEntity(); ++n)
        {
            // Compute the text label position for this entity (WGS84 space).
            if (l_dbfEntity && l_dbfEntity[n].stringValue != NULL)
            {
                double lon, lat;
                float angle = 0.0f;
                if (a_options.useWgs84BuildTransform)
                {
                    lon = ((l_shapeEntity[n].maxX + l_shapeEntity[n].minX) / 2);
                    lat = ((l_shapeEntity[n].maxY + l_shapeEntity[n].minY) / 2);
                }
                else
                {
                    lon = ((l_shapeEntity[n].maxX + l_shapeEntity[n].minX) / 2) / geo.property.mapBuildScale;
                    lat = WGS84WEBMERCATOR_TO_WGS84(((l_shapeEntity[n].maxY + l_shapeEntity[n].minY) / 2) / geo.property.mapBuildScale);
                }
                if (l_shapeEntity[n].type == SHPT_ARC)
                {
                    std::srand(std::time(0));
                    int random_variable = std::rand() % (l_shapeEntity[n].totalVertex);
                    if (random_variable == (l_shapeEntity[n].totalVertex - 1))
                        random_variable--;

                    lon = (l_shapeEntity[n].coordinate[random_variable][0] + l_shapeEntity[n].coordinate[random_variable + 1][0]) / 2 / geo.property.mapBuildScale;
                    lat = WGS84WEBMERCATOR_TO_WGS84((l_shapeEntity[n].coordinate[random_variable][1] + l_shapeEntity[n].coordinate[random_variable + 1][1]) / 2 / geo.property.mapBuildScale);
                    angle = (float)(atan2(-l_shapeEntity[n].coordinate[random_variable + 1][1] + l_shapeEntity[n].coordinate[random_variable][1],
                                          l_shapeEntity[n].coordinate[random_variable + 1][0] - l_shapeEntity[n].coordinate[random_variable][0]) /
                                    M_PI * 180.0);
                }

                Label label;
                label.longitude = lon;
                label.latitude = lat;
                label.angle = angle;
                label.text = l_dbfEntity[n].stringValue;
                geo.labels.push_back(label);
            }

            // Transform the shape vertices into map space.
            l_iTotalVertex += l_shapeEntity[n].totalVertex;
            for (int i = 0; i < l_shapeEntity[n].totalVertex; ++i)
            {
                const int idx = l_iTotalVertex - l_shapeEntity[n].totalVertex + i;
                Vertex v;
                if (a_options.useWgs84BuildTransform)
                {
                    v.x = X_WGS84_BUILD_COORD_TO_MAP_COORD(l_shapeEntity[n].coordinate[i][0], geo.property);
                    v.y = Y_WGS84_BUILD_COORD_TO_MAP_COORD(l_shapeEntity[n].coordinate[i][1], geo.property);
                }
                else
                {
                    v.x = X_METRE_COORD_TO_MAP_COORD(l_shapeEntity[n].coordinate[i][0], geo.property);
                    v.y = Y_METRE_COORD_TO_MAP_COORD(l_shapeEntity[n].coordinate[i][1], geo.property);
                }
                v.z = (float)a_options.layerDepth;
                geo.vertices[idx] = v;
                geo.lineIndices.push_back(static_cast<unsigned int>(idx));

                if (l_shapeEntity[n].isRing[i] == 1)
                {
                    geo.rings.push_back(idx);
                    geo.renderType.push_back(l_shapeEntity[n].type);
                }
            }
            geo.rings.push_back(l_iTotalVertex);
            geo.renderType.push_back(l_shapeEntity[n].type);
            geo.lineIndices.push_back(0xFFFFFFFF); // Primitive Restart Index
        }
    }

    // The raw file data has been fully consumed into renderable geometry, so
    // release it now.
    freeMemory();

    return geo;
}
