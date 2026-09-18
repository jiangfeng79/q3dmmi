#ifndef MAPLAYER_H
#define MAPLAYER_H

#include <QPointF>
#include <QOpenGLBuffer>
#include <QString>
#include <QObject>

#include <atomic>
#include <cstdint>
#include <functional>
#include <memory>

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

class MapLayer : public QObject
{
    Q_OBJECT

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

    // The current geometry, published atomically (see the note below). The
    // draw path reads this on the render thread while a worker thread may be
    // publishing a new one, so it is an immutable heap object behind a
    // std::atomic pointer rather than a mutable member.
    //
    // Returns the shared_ptr by value so the caller holds a reference for as
    // long as it uses the geometry; this keeps the object alive even if a
    // concurrent publish replaces it.
    std::shared_ptr<const LayerGeometry> geometry() const
    {
        return m_geometry.load(std::memory_order_acquire);
    }

    MapProperty m_property;
    QString m_fileName;
    QString m_layerName;
    LayerParser* m_parser;
    GLuint m_VBO_ID[2];

protected:
    void uploadGeometry(GLenum usage);
    void drawPrimitive(const MapLayerRenderContext& context) const;
    void drawLines(const MapLayerRenderContext& context) const;

    void drawPolygonRing(const MapLayerRenderContext& context, int index) const;
    void drawRingsToStencil(const MapLayerRenderContext& context) const;
    void drawRingsToColor(const MapLayerRenderContext& context) const;
    void drawRingFilled(const MapLayerRenderContext& context) const;

    // ------------------------------------------------------------------
    // Geometry double-buffering (thread-safety)
    //
    // buildLayer() parses on a worker thread and then publishes the result
    // back on the render thread, while draw() reads the geometry every frame.
    // To avoid a data race we keep the geometry as an immutable heap object
    // behind a std::atomic pointer: publishGeometry() builds a fresh object
    // and stores its pointer with a release store; geometry() loads it with
    // an acquire load. The old object is freed only once no reader holds a
    // reference (shared_ptr refcount), so a reader never sees a half-built
    // geometry and never frees memory it is still using.
    // ------------------------------------------------------------------
    void publishGeometry(LayerGeometry geometry);

    std::uint64_t m_id;
    std::uint64_t m_textId;
    const std::uint64_t& m_displayMask;
    TSDWindow& m_window;
    FillMode m_fillMode;

    std::atomic<std::shared_ptr<const LayerGeometry>> m_geometry{
        std::make_shared<const LayerGeometry>()};
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
    // Bumps the input generation so an in-flight parse can detect that its
    // inputs (routes / snapshot / scale) changed and discard a stale result.
    void markDirty() { m_dirty = true; ++m_generation; }
    bool isDirty() const { return m_dirty; }

private:
    void drawFilled(const MapLayerRenderContext& context) const;
    bool m_dirty = false;
    // True while a worker-thread parse is running. Atomic because the worker
    // thread clears it and the render thread reads it in rebuild().
    std::atomic<bool> m_rebuildInFlight{false};
    float m_lastScale = 0.0f;
    // Monotonic counter bumped by markDirty(). A parse captures the value at
    // start and discards its result if it has advanced (inputs changed).
    // Atomic because the render thread bumps it while a worker reads it.
    std::atomic<std::uint64_t> m_generation{0};
    LabelStyle m_labelStyle;
};

#endif  // MAPLAYER_H