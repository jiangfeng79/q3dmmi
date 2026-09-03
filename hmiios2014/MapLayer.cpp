#include "MapLayer.h"

#include <shapefil.h>

#include "TSDWindow.h"

MapLayer::MapLayer(std::uint64_t id, std::uint64_t textId, LayerParser* parser, const std::uint64_t& displayMask,
                   TSDWindow& window, FillMode fillMode)
    : m_parser(parser), m_id(id), m_textId(textId), m_displayMask(displayMask), m_window(window), m_fillMode(fillMode)
{
    m_VBO_ID[0] = m_VBO_ID[1] = 0;
    m_property.scale = 0.1f;
}

MapLayer::MapLayer(const char* fileName, std::uint64_t id, std::uint64_t textId, LayerParser* parser,
                   const std::uint64_t& displayMask, TSDWindow& window, FillMode fillMode)
    : m_fileName(fileName),
      m_parser(parser),
      m_id(id),
      m_textId(textId),
      m_displayMask(displayMask),
      m_window(window),
      m_fillMode(fillMode)
{
    m_VBO_ID[0] = m_VBO_ID[1] = 0;
    m_property.scale = 0.1f;
}

MapLayer::~MapLayer()
{
    delete m_parser;
}

void MapLayer::buildLayer(const MapProperty& baseProperty, int layerDepth)
{
    if (!m_parser)
        return;
    LayerParser::Options options;
    options.baseProperty = baseProperty;
    options.layerDepth = layerDepth;
    options.useWgs84BuildTransform = (m_id == TSDWindow::MAN_MADE || m_id == TSDWindow::MRT);
    m_geometry = m_parser->parse(options);
    m_property = m_geometry.property;
    uploadGeometry(GL_STATIC_DRAW);
}

void MapLayer::uploadGeometry(GLenum usage)
{
    if (m_geometry.vertices.empty())
        return;
    if (!m_VBO_ID[0])
        m_window.glGenBuffers(1, &m_VBO_ID[0]);
    m_window.glBindBuffer(GL_ARRAY_BUFFER, m_VBO_ID[0]);
    m_window.glBufferData(GL_ARRAY_BUFFER, m_geometry.vertices.size() * sizeof(Vertex), m_geometry.vertices.data(),
                          usage);
    m_window.glBindBuffer(GL_ARRAY_BUFFER, 0);
    if (!m_geometry.lineIndices.empty())
    {
        if (!m_VBO_ID[1])
            m_window.glGenBuffers(1, &m_VBO_ID[1]);
        m_window.glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_VBO_ID[1]);
        m_window.glBufferData(GL_ELEMENT_ARRAY_BUFFER, m_geometry.lineIndices.size() * sizeof(GLuint),
                              m_geometry.lineIndices.data(), usage);
        m_window.glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
    }
}

void MapLayer::releaseGpuResources()
{
    if (m_VBO_ID[0] || m_VBO_ID[1])
        m_window.glDeleteBuffers(2, m_VBO_ID);
    m_VBO_ID[0] = m_VBO_ID[1] = 0;
    if (m_parser)
        m_parser->freeMemory();
    m_geometry.clear();
}

void MapLayer::draw(const MapLayerRenderContext& context, bool linePass) const
{
    if (!isVisible())
        return;
    if (linePass)
    {
        drawLines(context);
        return;
    }
    drawPrimitive(context);
}

void MapLayer::drawPrimitive(const MapLayerRenderContext& context) const
{
    if (!m_VBO_ID[0] || m_geometry.renderType.empty())
        return;

    const int type = m_geometry.renderType.front();
    if (type != SHPT_POINT && type != SHPT_POLYGON)
        return;

    if (type == SHPT_POLYGON && m_fillMode != MapLayer::FillMode::None)
    {
        m_window.glClear(GL_STENCIL_BUFFER_BIT);
        m_window.glClearStencil(0);
        m_window.glEnable(GL_STENCIL_TEST);

        if (m_id == TSDWindow::DisplayMaskBits::WATER_AREA)
        {
            drawRingsToStencil(context);
            drawRingsToColor(context);
        }
        else
        {
            drawRingFilled(context);
        }

        m_window.glDisable(GL_STENCIL_TEST);
    }
    else
    {
        m_window.glBindBuffer(GL_ARRAY_BUFFER, m_VBO_ID[0]);
        m_window.glVertexAttribPointer(context.positionAttribute, 3, GL_FLOAT, GL_FALSE, 0, nullptr);
        m_window.glEnableVertexAttribArray(context.positionAttribute);

        m_window.glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_VBO_ID[1]);
        m_window.glEnable(GL_PRIMITIVE_RESTART);
        m_window.glPrimitiveRestartIndex(0xFFFFFFFF);
        if (type == SHPT_POINT)
        {
            m_window.glPointSize(7.0f);  // Set the point size, adjust as needed
            m_window.glDrawElements(GL_POINTS, m_geometry.lineIndices.size(), GL_UNSIGNED_INT, nullptr);
        }
        else
        {
            m_window.glDrawElements(GL_LINE_STRIP, m_geometry.lineIndices.size(), GL_UNSIGNED_INT, nullptr);
        }
        m_window.glDisable(GL_PRIMITIVE_RESTART);
        m_window.glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
        m_window.glDisableVertexAttribArray(context.positionAttribute);
        m_window.glBindBuffer(GL_ARRAY_BUFFER, 0);
    }
}

void MapLayer::drawLines(const MapLayerRenderContext& context) const
{
    if (!m_VBO_ID[0] || !m_VBO_ID[1] || m_geometry.renderType.empty() || m_geometry.renderType.front() != SHPT_ARC)
        return;

    m_window.glBindBuffer(GL_ARRAY_BUFFER, m_VBO_ID[0]);
    m_window.glVertexAttribPointer(context.positionAttribute, 3, GL_FLOAT, GL_FALSE, 0, nullptr);
    m_window.glEnableVertexAttribArray(context.positionAttribute);
    m_window.glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_VBO_ID[1]);
    m_window.glEnable(GL_PRIMITIVE_RESTART);
    m_window.glPrimitiveRestartIndex(0xFFFFFFFF);
    m_window.glDrawElements(GL_LINE_STRIP, m_geometry.lineIndices.size(), GL_UNSIGNED_INT, nullptr);
    m_window.glDisable(GL_PRIMITIVE_RESTART);
    m_window.glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
    m_window.glDisableVertexAttribArray(context.positionAttribute);
    m_window.glBindBuffer(GL_ARRAY_BUFFER, 0);
}

void MapLayer::drawText(const MapLayerRenderContext& context) const
{
    if (!(m_textId & m_displayMask))
        return;
    for (const Label& label : m_geometry.labels)
    {
        const QPointF point = context.wgs84ToScreen(label.longitude, label.latitude);
        if (point.x() > 0 && point.x() < context.width * context.retinaScale && point.y() > 0
            && point.y() < context.height * context.retinaScale && !label.text.empty())
        {
            const QString text = QString::fromStdString(label.text);
            if (label.angle == 0.0f)
                context.renderText(static_cast<int>(point.x()), static_cast<int>(point.y()), text);
            else
                context.renderTextWithAngle(static_cast<int>(point.x()), static_cast<int>(point.y()), text,
                                            label.angle);
        }
    }
}

void MapLayer::drawPolygonRing(const MapLayerRenderContext& context, int index) const
{
    const int vertexCount = m_geometry.rings[index + 1] - m_geometry.rings[index];
    if (vertexCount < 3)
        return;

    m_window.glBindBuffer(GL_ARRAY_BUFFER, m_VBO_ID[0]);
    m_window.glVertexAttribPointer(
        context.positionAttribute, 3, GL_FLOAT, GL_FALSE, 0,
        reinterpret_cast<void*>(static_cast<intptr_t>(m_geometry.rings[index] * sizeof(Vertex))));
    m_window.glEnableVertexAttribArray(context.positionAttribute);
    m_window.glDrawArrays(GL_TRIANGLE_FAN, 0, vertexCount);
    m_window.glDisableVertexAttribArray(context.positionAttribute);
    m_window.glBindBuffer(GL_ARRAY_BUFFER, 0);
}

void MapLayer::drawRingsToStencil(const MapLayerRenderContext& context) const
{
    m_window.glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
    m_window.glStencilFunc(GL_ALWAYS, 1, 1);
    m_window.glStencilOp(GL_KEEP, GL_INVERT, GL_INVERT);

    for (int index = 0; index < static_cast<int>(m_geometry.rings.size()) - 1; ++index)
        if (m_geometry.renderType[index] == SHPT_POLYGON)
            drawPolygonRing(context, index);
}

void MapLayer::drawRingsToColor(const MapLayerRenderContext& context) const
{
    m_window.glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
    m_window.glStencilFunc(GL_EQUAL, 1, 1);
    m_window.glStencilOp(GL_KEEP, GL_KEEP, GL_KEEP);

    for (int index = 0; index < static_cast<int>(m_geometry.rings.size()) - 1; ++index)
        if (m_geometry.renderType[index] == SHPT_POLYGON)
            drawPolygonRing(context, index);
}

void MapLayer::drawRingFilled(const MapLayerRenderContext& context) const
{
    for (int index = 0; index < static_cast<int>(m_geometry.rings.size()) - 1; ++index)
    {
        if (m_geometry.renderType[index] != SHPT_POLYGON)
            continue;

        m_window.glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
        m_window.glStencilFunc(GL_ALWAYS, 1, 1);
        m_window.glStencilOp(GL_KEEP, GL_INVERT, GL_INVERT);
        drawPolygonRing(context, index);

        m_window.glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
        m_window.glStencilFunc(GL_EQUAL, 1, 1);
        m_window.glStencilOp(GL_KEEP, GL_KEEP, GL_KEEP);
        drawPolygonRing(context, index);
    }
}

BaseMapLayer::BaseMapLayer(const char* fileName, std::uint64_t id, std::uint64_t textId, LayerParser* parser,
                           const std::uint64_t& displayMask, TSDWindow& window)
    : MapLayer(fileName, id, textId, parser, displayMask, window, MapLayer::FillMode::Fill)
{
}

void BaseMapLayer::drawFilled(const MapLayerRenderContext& context) const
{
    if (!m_VBO_ID[0] || m_geometry.rings.size() < 2 || m_geometry.renderType.empty())
        return;

    m_window.glClear(GL_STENCIL_BUFFER_BIT);
    m_window.glClearStencil(0);
    m_window.glEnable(GL_STENCIL_TEST);
    drawRingFilled(context);
    m_window.glDisable(GL_STENCIL_TEST);
}

void BaseMapLayer::buildLayer(const MapProperty& baseProperty, int layerDepth)
{
    if (!m_parser)
        return;
    if (m_id == TSDWindow::DisplayMaskBits::COASTAL)
    {
        m_geometry = m_parser->parseBase();
        m_property = m_geometry.property;
        uploadGeometry(GL_STATIC_DRAW);
        return;
    }

    MapLayer::buildLayer(baseProperty, layerDepth);
}

void BaseMapLayer::draw(const MapLayerRenderContext& context, bool linePass) const
{
    if (isVisible() && !linePass)
    {
        drawFilled(context);
    }
}

void StaticMapLayer::setGeometry(LayerGeometry geometry)
{
    m_geometry = std::move(geometry);
    m_property = m_geometry.property;
}

LiveMapLayer::LiveMapLayer(std::uint64_t id, std::uint64_t textId, LayerParser* parser,
                           const std::uint64_t& displayMask, TSDWindow& window, LabelStyle labelStyle,
                           FillMode fillMode)
    : MapLayer(id, textId, parser, displayMask, window, fillMode), m_labelStyle(labelStyle)
{
}

void LiveMapLayer::drawFilled(const MapLayerRenderContext& context) const
{
    if (!m_VBO_ID[0] || m_geometry.rings.size() < 2 || m_geometry.renderType.empty())
        return;

    m_window.glClear(GL_STENCIL_BUFFER_BIT);
    m_window.glClearStencil(0);
    m_window.glEnable(GL_STENCIL_TEST);
    drawRingFilled(context);
    m_window.glDisable(GL_STENCIL_TEST);
}

void LiveMapLayer::rebuild(const MapProperty& baseProperty, float scale)
{
    if (!m_parser)
        return;

    if (m_lastScale != scale)
    {
        m_lastScale = scale;
        m_parser->setScale(scale);
        m_dirty = true;
    }

    if (!m_dirty)
        return;

    LayerParser::Options options;
    options.baseProperty = baseProperty;
    options.useWgs84BuildTransform = true;
    m_geometry = m_parser->parse(options);
    m_property = m_geometry.property;
    uploadGeometry(GL_DYNAMIC_DRAW);
    m_dirty = false;
}

void LiveMapLayer::drawText(const MapLayerRenderContext& context) const
{
    if (m_labelStyle != LabelStyle::Flight)
    {
        MapLayer::drawText(context);
        return;
    }

    if (!(m_textId & m_displayMask))
        return;

    for (const Label& label : m_geometry.labels)
    {
        const QPointF point = context.wgs84ToScreen(label.longitude, label.latitude);
        if (point.x() <= 0 || point.x() >= context.width * context.retinaScale || point.y() <= 0
            || point.y() >= context.height * context.retinaScale || label.text.empty())
            continue;

        const QString full = QString::fromStdString(label.text);
        const int separator = full.indexOf(' ');
        const QString callsign = separator >= 0 ? full.left(separator) : full;
        const QString altitude = separator >= 0 ? full.mid(separator + 1) : QString();
        const int lineHeight = 12 * context.retinaScale;
        const int left = static_cast<int>(point.x()) + 10 * context.retinaScale;
        const int top = static_cast<int>(point.y()) - 30;

        context.renderText(left + static_cast<int>(context.textWidth(callsign) / 2), top, callsign);
        if (!altitude.isEmpty())
            context.renderText(left + static_cast<int>(context.textWidth(altitude) / 2), top + lineHeight, altitude);
    }
}