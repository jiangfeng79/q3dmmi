#ifndef MAPLAYER_H
#define MAPLAYER_H

#include <QPointF>
#include <QOpenGLBuffer>
#include <QString>

#include <cstdint>
#include <functional>

#include "layerGeometry.h"
#include "layerParser.h"

class TSDWindow;

struct MapLayerRenderContext
{
    GLuint positionAttribute;
    qreal retinaScale;
    int width;
    int height;
    std::function<QPointF(double, double)> wgs84ToScreen;
    std::function<void(int, int, const QString&)> renderText;
    std::function<void(int, int, const QString&, float)> renderTextWithAngle;
    std::function<qreal(const QString&)> textWidth;
};

class MapLayer
{
public:
    enum class FillMode
    {
        None,
        Fill,
        Substract
    };

    MapLayer(std::uint64_t id, std::uint64_t textId, LayerParser* parser, const std::uint64_t& displayMask,
             TSDWindow& window, FillMode fillMode = FillMode::None);
    MapLayer(const char* fileName, std::uint64_t id, std::uint64_t textId, LayerParser* parser,
             const std::uint64_t& displayMask, TSDWindow& window, FillMode fillMode = FillMode::None);
    virtual ~MapLayer();

    MapLayer(const MapLayer&) = delete;
    MapLayer& operator=(const MapLayer&) = delete;

    virtual void buildLayer(const MapProperty& baseProperty, int layerDepth = 0);
    virtual void draw(const MapLayerRenderContext& context, bool linePass = false) const;
    virtual void drawText(const MapLayerRenderContext& context) const;
    void releaseGpuResources();

    LayerParser* parser() const { return m_parser; }
    std::uint64_t id() const { return m_id; }
    std::uint64_t textId() const { return m_textId; }
    bool isVisible() const { return m_id & m_displayMask; }

    MapProperty m_property;
    QString m_fileName;
    QString m_layerName;
    LayerParser* m_parser;
    LayerGeometry m_geometry;
    GLuint m_VBO_ID[2];

protected:
    void uploadGeometry(GLenum usage);
    void drawPrimitive(const MapLayerRenderContext& context) const;
    void drawLines(const MapLayerRenderContext& context) const;

    void drawPolygonRing(const MapLayerRenderContext& context, int index) const;
    void drawRingsToStencil(const MapLayerRenderContext& context) const;
    void drawRingsToColor(const MapLayerRenderContext& context) const;
    void drawRingFilled(const MapLayerRenderContext& context) const;

    std::uint64_t m_id;
    std::uint64_t m_textId;
    const std::uint64_t& m_displayMask;
    TSDWindow& m_window;
    FillMode m_fillMode;
};

class BaseMapLayer : public MapLayer
{
public:
    BaseMapLayer(const char* fileName, std::uint64_t id, std::uint64_t textId,
             LayerParser* parser, const std::uint64_t& displayMask, TSDWindow& window, FillMode fillMode = FillMode::Fill);
    void buildLayer(const MapProperty& baseProperty, int layerDepth = 0) override;
    void draw(const MapLayerRenderContext& context, bool linePass = false) const override;

private:
    void drawFilled(const MapLayerRenderContext& context) const;
};

class StaticMapLayer : public MapLayer
{
public:
    using MapLayer::MapLayer;

    void setGeometry(LayerGeometry geometry);
};

class LiveMapLayer : public MapLayer
{
public:
    enum class LabelStyle
    {
        Default,
        Flight,
        Bus
    };

    LiveMapLayer(std::uint64_t id, std::uint64_t textId, LayerParser* parser, const std::uint64_t& displayMask,
                 TSDWindow& window, LabelStyle labelStyle = LabelStyle::Default, FillMode fillMode = FillMode::None);

    void rebuild(const MapProperty& baseProperty, float scale);
    void drawText(const MapLayerRenderContext& context) const override;
    void markDirty() { m_dirty = true; }
    bool isDirty() const { return m_dirty; }

private:
    void drawFilled(const MapLayerRenderContext& context) const;
    bool m_dirty = false;
    float m_lastScale = 0.0f;
    LabelStyle m_labelStyle;
};

#endif  // MAPLAYER_H