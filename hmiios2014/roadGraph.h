#ifndef ROADGRAPH_H
#define ROADGRAPH_H

#include <QHash>
#include <QList>
#include <QPointF>
#include <QVector>

#include <cstdint>
#include <vector>

// ---------------------------------------------------------------------------
// RoadGraph
//
// A small road-network graph built from the OSM road shapefiles
// (main roads, minor roads and motorways). It lets the bus-route parser snap
// each bus stop onto the nearest road and then route between consecutive
// stops along the actual road network instead of drawing a straight line.
//
// The road shapefiles are stored in Web Mercator metres (EPSG:3785), so the
// graph works entirely in that space. Bus stops arrive in WGS84 and are
// converted to the same space before being snapped.
//
// The graph is built once (lazily) and cached; it is immutable afterwards.
// ---------------------------------------------------------------------------
class RoadGraph
{
public:
    // A single road segment (a shapefile arc) in Web Mercator metres.
    struct Segment
    {
        int a = -1;  // endpoint node index
        int b = -1;  // endpoint node index
        double length = 0.0;
    };

    // A routed path between two snapped points, as a polyline in Web Mercator
    // metres. Empty when no route could be found.
    using Polyline = std::vector<QPointF>;

    // Build the graph from the three road shapefiles. Returns false (and
    // leaves the graph empty) if the files cannot be read.
    bool build(const QString& mainRoads, const QString& minorRoads, const QString& motorWays);

    bool isValid() const { return !m_segments.empty(); }

    // Snap a Web Mercator point to the nearest road vertex. Returns the node
    // index, or -1 if the graph is empty.
    int nearestNode(double x, double y) const;

    // Snap a Web Mercator point to the nearest road vertex within `maxRadius`
    // metres. Returns the node index, or -1 if no node is that close (or the
    // graph is empty). Used to snap a bus stop to the road it actually sits on
    // rather than an arbitrary nearby road. Open-end minor nodes (dead ends)
    // are skipped so routes stay on through roads.
    int nearestNodeInRadius(double x, double y, double maxRadius) const;

    // Snap a Web Mercator point to a road vertex within `maxRadius` metres,
    // biased toward the direction of travel (dirX, dirY). Among the candidate
    // nodes, the one that is both close to the point and lying ahead in the
    // travel direction is preferred, so a stop on a divided road snaps to the
    // carriageway the bus is heading down rather than the opposite one.
    // Returns the node index, or -1 if no node is that close (or the graph is
    // empty). A degenerate direction falls back to the plain nearest node.
    int snapToward(double x, double y, double dirX, double dirY, double maxRadius) const;

    // Route between two Web Mercator points along the road network. The
    // returned polyline starts at (x0, y0) and ends at (x1, y1); intermediate
    // points follow the road. Falls back to a straight segment when no route
    // exists.
    Polyline route(double x0, double y0, double x1, double y1) const;

    // Route between two already-snapped road nodes. The returned polyline
    // starts at the start node and ends at the goal node, following the road
    // network. Falls back to a straight segment when no route exists.
    Polyline routeBetween(int startNode, int goalNode) const;

private:
    // Add one shapefile (Web Mercator metres) to the graph. `roadClass` tags
    // every node created from this file so the bridge pass can tell which
    // road class a node belongs to.
    void addShapefile(const QString& fileName, int roadClass);

    // Connect nearby nodes that belong to different road classes (e.g. a
    // motorway endpoint and a surface-road endpoint at an interchange). The
    // shapefiles are separate, so their endpoints don't share exact
    // coordinates; this pass bridges the gap so Dijkstra can route between
    // road classes.
    void connectNearbyNodes(double maxDistance);

    // Flag minor-road nodes that are "open ends" (dead ends / cul-de-sacs,
    // i.e. a single connection). Buses don't terminate at dead-end minor
    // roads, so these nodes are excluded from snapping to keep routes on
    // through roads. Must run after the graph (and bridge pass) is complete.
    void markOpenEnds();

    // Nearest-node search with an explicit choice of whether to skip open-end
    // minor nodes. The public nearestNodeInRadius() skips them; the unbounded
    // nearestNode() keeps them as a last resort.
    int nearestNodeInRadius(double x, double y, double maxRadius, bool excludeOpenEnds) const;

    // A graph node (a road endpoint) in Web Mercator metres.
    struct Node
    {
        double x = 0.0;
        double y = 0.0;
        int roadClass = 0;  // which shapefile this node came from
        bool openEnd = false;  // minor-road dead end, excluded from snapping
    };

    // Adjacency list: node index -> list of (neighbour node, edge length).
    struct Edge
    {
        int to = -1;
        double weight = 0.0;
    };

    std::vector<Node> m_nodes;
    std::vector<Segment> m_segments;
    std::vector<std::vector<Edge>> m_adjacency;

    // Spatial hash for nearest-node lookups.
    double m_minX = 0.0, m_minY = 0.0;
    double m_maxX = 0.0, m_maxY = 0.0;
    double m_cellSize = 1.0;
    int m_gridW = 0, m_gridH = 0;
    std::vector<int> m_grid;  // cell -> first node index; -1 if empty
    std::vector<int> m_gridNext;  // node -> next node in the same cell
};

#endif  // ROADGRAPH_H
