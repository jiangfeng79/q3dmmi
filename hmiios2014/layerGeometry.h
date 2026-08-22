#ifndef LAYERGEOMETRY_H
#define LAYERGEOMETRY_H

#include <vector>
#include <string>

// ---------------------------------------------------------------------------
// Renderable geometry produced by a layer parser.
//
// A parser turns a raw input layer (shapefile + dbf, or anything else) into a
// flat, render-ready description: polygons, lines and points, plus optional
// text labels. The renderer consumes this without knowing anything about the
// source file format.
// ---------------------------------------------------------------------------

// A single vertex in map space (x, y, depth).
struct Vertex
{
    float x;
    float y;
    float z;
};

// One contiguous run of vertices that is drawn as a single primitive.
// For polygons this is a ring (drawn as a triangle fan / line strip); for
// lines it is a polyline; for points each vertex is a point.
struct GeometryRing
{
    std::vector<Vertex> vertices;
    int type; // source shape type (SHPT_*), used to pick the draw primitive
};

// A text label positioned in WGS84 (longitude, latitude) space.
struct Label
{
    double longitude;
    double latitude;
    float angle; // degrees; 0 = no rotation
    std::string text;
};

// Per-layer map properties (center, extent, scale, ...).
struct MapProperty
{
    MapProperty()
        : scale(0.0f)
        , centerX(0.0f)
        , centerY(0.0f)
        , width(0.0f)
        , height(0.0f)
        , mapBuildScale(111319.4907777778f)
        , totalNumberOfVertex(0)
    {
    }

    float scale;
    float centerX, centerY;
    float width, height;
    float mapBuildScale;
    int totalNumberOfVertex;
};

// The complete renderable output of a single layer.
struct LayerGeometry
{
    MapProperty property;

    // Flat vertex buffer (x, y, z) for all geometry, in draw order.
    std::vector<Vertex> vertices;

    // Ring boundaries: ring i spans vertices [rings[i], rings[i + 1]).
    std::vector<int> rings;
    std::vector<int> renderType; // SHPT_* type per ring

    // Text labels for the layer.
    std::vector<Label> labels;

    void clear()
    {
        vertices.clear();
        rings.clear();
        renderType.clear();
        labels.clear();
        property.totalNumberOfVertex = 0;
    }
};

#endif // LAYERGEOMETRY_H
