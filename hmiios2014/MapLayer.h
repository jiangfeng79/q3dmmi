#ifndef MAPLAYER_H
#define MAPLAYER_H

#include <QOpenGLBuffer>
#include <QString>

#include <cstdint>

#include "layerGeometry.h"
#include "layerParser.h"

class MapLayer
{
public:
    MapLayer() : m_bToFill(false), m_parser(nullptr)
    {
        m_VBO_ID[0] = m_VBO_ID[1] = 0;
        m_property.scale = 0.1;
    }

    MapLayer(std::uint64_t id, std::uint64_t textId, bool toFill = false, bool isLive = false)
        : m_id(id), m_text_id(textId), m_bToFill(toFill), m_isLive(isLive), m_parser(nullptr)
    {
        m_VBO_ID[0] = m_VBO_ID[1] = 0;
        m_property.scale = 0.1;
    }

    MapLayer(const char* fileName, std::uint64_t id, std::uint64_t textId, bool toFill = false)
        : m_id(id), m_text_id(textId), m_bToFill(toFill), m_parser(nullptr), m_fileName(fileName)
    {
        m_VBO_ID[0] = m_VBO_ID[1] = 0;
        m_property.scale = 0.1;
    }

    MapLayer(const char* fileName, const char* layerName, std::uint64_t id, std::uint64_t textId,
             bool toFill = false)
        : m_id(id), m_text_id(textId), m_bToFill(toFill), m_parser(nullptr), m_fileName(fileName),
          m_layerName(layerName)
    {
        m_VBO_ID[0] = m_VBO_ID[1] = 0;
        m_property.scale = 0.1;
    }

    ~MapLayer() { delete m_parser; }

    void setParser(LayerParser* parser) { m_parser = parser; }
    LayerParser* parser() const { return m_parser; }

    MapProperty m_property;
    QString m_fileName;
    QString m_layerName;
    LayerParser* m_parser;
    LayerGeometry m_geometry;
    GLuint m_VBO_ID[2];
    std::uint64_t m_id = 0;
    std::uint64_t m_text_id = 0;
    bool m_bToFill;
    bool m_isLive = false;
    bool m_dirty = false;
    float m_lastScale = 0.0f;

    void buildLayer();
    void buildLayer(MapProperty& property, int layerDepth);
};

#endif  // MAPLAYER_H