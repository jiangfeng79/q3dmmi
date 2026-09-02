#ifndef LAYERPARSER_H
#define LAYERPARSER_H

#include <QString>

#include "dbfReader.h"
#include "layerGeometry.h"
#include "shpReader.h"

// ---------------------------------------------------------------------------
// A layer parser is coupled to a single input layer and turns it into
// renderable geometry (polygons, lines, points and text labels). The renderer
// only ever sees the LayerGeometry a parser produces, never the raw file
// format, so a new data source can be added by writing a new parser and
// injecting it into a layer.
// ---------------------------------------------------------------------------
class LayerParser
{
public:
    virtual ~LayerParser() = default;

    // Options controlling how the source layer is transformed into map space.
    struct Options
    {
        MapProperty baseProperty;             // base (coastal) layer property, used for coordinate transforms
        int layerDepth = 0;                   // z depth assigned to every vertex
        bool useWgs84BuildTransform = false;  // MAN_MADE / MRT style transform
    };

    // Parse the source layer and return renderable geometry.
    virtual LayerGeometry parse(const Options& a_options) = 0;
    virtual LayerGeometry parseBase()
    {
        Options options;
        return parse(options);
    }

    // Release any cached raw data (e.g. after the GL context is destroyed).
    virtual void freeMemory() = 0;

    virtual void setScale(float) {}
};

// Parser for shapefile + dbf input layers (the default map data source).
class ShapefileLayerParser : public LayerParser
{
public:
    ShapefileLayerParser(const QString& a_fileName, const QString& a_layerName = QString());
    ~ShapefileLayerParser() override;

    LayerGeometry parse(const Options& a_options) override;
    LayerGeometry parseBase() override;
    void freeMemory() override;

private:
    LayerGeometry parseImpl(const Options& options, bool deriveBaseProperty);

    QString m_fileName;
    QString m_layerName;
    // The raw file readers are an implementation detail of this parser.
    ShpReader m_shapeFileReader;
    DBFReader m_dbfFileReader;
};

#endif  // LAYERPARSER_H
