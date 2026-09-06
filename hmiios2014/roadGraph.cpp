#include "roadGraph.h"

#include <QFile>
#include <QHash>
#include <QPointF>

#include <algorithm>
#include <cmath>
#include <limits>
#include <queue>

#include "shpReader.h"

// ---------------------------------------------------------------------------
// RoadGraph
// ---------------------------------------------------------------------------

namespace
{
// Snap tolerance for merging road endpoints that share a location. Road
// endpoints from adjacent OSM ways are usually identical, but a small
// tolerance keeps the graph connected across tiny floating-point gaps.
constexpr double kNodeMergeTolerance = 2.0;  // metres

// Spatial-hash cell size for nearest-node lookups (metres).
constexpr double kGridCellSize = 250.0;

// Dijkstra search cap: stop expanding once the accumulated distance exceeds
// this (metres). Bus stops are close together, so a generous cap keeps the
// search fast while still finding routes across the island.
constexpr double kMaxRouteDistance = 200000.0;

// Maximum length (metres) of a single graph edge. Road shapefiles store
// motorways and main roads with sparse, long vertices (often 100-500 m
// between points), so a bus stop on such a road can be far from the nearest
// node. Subdividing long segments keeps nodes close enough for accurate,
// direction-aware snapping and a smooth rendered line.
constexpr double kMaxSegmentLength = 150.0;

// Maximum distance (metres) at which two nodes from different road classes
// are bridged together. At an interchange the motorway and surface-road
// endpoints are close but not identical (separate shapefiles), so a small
// bridge edge connects them. 50 m is wide enough to catch real interchanges
// without creating spurious shortcuts across parallel roads.
constexpr double kBridgeDistance = 50.0;
}  // namespace

bool RoadGraph::build(const QString& mainRoads, const QString& minorRoads, const QString& motorWays)
{
    m_nodes.clear();
    m_segments.clear();
    m_adjacency.clear();
    m_grid.clear();
    m_gridNext.clear();

    addShapefile(mainRoads, 0);
    addShapefile(minorRoads, 1);
    addShapefile(motorWays, 2);

    if (m_segments.empty())
    {
        return false;
    }

    // Build the spatial hash over the nodes.
    m_minX = m_maxX = m_nodes[0].x;
    m_minY = m_maxY = m_nodes[0].y;
    for (const Node& node : m_nodes)
    {
        m_minX = std::min(m_minX, node.x);
        m_maxX = std::max(m_maxX, node.x);
        m_minY = std::min(m_minY, node.y);
        m_maxY = std::max(m_maxY, node.y);
    }

    m_cellSize = kGridCellSize;
    m_gridW = static_cast<int>(std::ceil((m_maxX - m_minX) / m_cellSize)) + 1;
    m_gridH = static_cast<int>(std::ceil((m_maxY - m_minY) / m_cellSize)) + 1;
    m_grid.assign(static_cast<size_t>(m_gridW) * m_gridH, -1);
    m_gridNext.assign(m_nodes.size(), -1);

    for (int i = 0; i < static_cast<int>(m_nodes.size()); ++i)
    {
        int cx = static_cast<int>((m_nodes[i].x - m_minX) / m_cellSize);
        int cy = static_cast<int>((m_nodes[i].y - m_minY) / m_cellSize);
        cx = std::clamp(cx, 0, m_gridW - 1);
        cy = std::clamp(cy, 0, m_gridH - 1);
        const int cell = cy * m_gridW + cx;
        m_gridNext[i] = m_grid[cell];
        m_grid[cell] = i;
    }

    // Bridge nodes from different road classes that are close together (e.g.
    // a motorway endpoint and a surface-road endpoint at an interchange). The
    // shapefiles are separate, so their endpoints don't share exact
    // coordinates; without this, Dijkstra can't route between road classes.
    connectNearbyNodes(kBridgeDistance);

    // Flag minor-road dead ends so snapping avoids them. Runs last, once the
    // final connectivity (including bridges) is known.
    markOpenEnds();

    return true;
}

void RoadGraph::addShapefile(const QString& fileName, int roadClass)
{
    if (!QFile::exists(fileName + ".shp"))
    {
        return;
    }

    ShpReader reader;
    if (reader.read((fileName + ".shp").toStdString().c_str()) != 0)
    {
        return;
    }

    ShpReader::ShpEntity* entity = reader.getEntity();
    const unsigned int count = reader.getNumberOfEntity();
    if (!entity)
    {
        return;
    }

    // Map from a quantised node location to a node index so that shared
    // endpoints across segments are merged into a single node.
    QHash<qint64, int> nodeMap;
    auto quantize = [](double v) -> qint64 { return static_cast<qint64>(std::llround(v / kNodeMergeTolerance)); };

    auto findOrCreateNode = [&](double x, double y) -> int {
        const qint64 key = (quantize(x) << 32) ^ static_cast<qint64>(quantize(y));
        auto it = nodeMap.constFind(key);
        if (it != nodeMap.constEnd())
        {
            return it.value();
        }
        const int idx = static_cast<int>(m_nodes.size());
        m_nodes.push_back(Node{x, y, roadClass});
        nodeMap.insert(key, idx);
        return idx;
    };

    // Add a single graph edge between two nodes.
    auto addEdge = [&](int a, int b, double length) {
        Segment seg;
        seg.a = a;
        seg.b = b;
        seg.length = length;
        m_segments.push_back(seg);

        // Grow the adjacency lists to cover both endpoints. `a` may have a
        // larger index than `b` (when `b` is an already existing node), so
        // size to the maximum of the two.
        const int maxNode = std::max(a, b);
        if (m_adjacency.size() <= static_cast<size_t>(maxNode))
        {
            m_adjacency.resize(maxNode + 1);
        }
        m_adjacency[a].push_back(Edge{b, length});
        m_adjacency[b].push_back(Edge{a, length});
    };

    for (unsigned int n = 0; n < count; ++n)
    {
        const unsigned int totalVertex = entity[n].totalVertex;
        if (totalVertex < 2)
        {
            continue;
        }

        // Walk the arc, adding a graph edge between each consecutive pair of
        // vertices. Long segments (common on motorways and main roads, whose
        // shapefiles are sparsely sampled) are subdivided so the graph has
        // enough nodes for accurate snapping and a smooth rendered line.
        int prevNode = findOrCreateNode(entity[n].coordinate[0][0], entity[n].coordinate[0][1]);
        for (unsigned int i = 1; i < totalVertex; ++i)
        {
            const double x0 = entity[n].coordinate[i - 1][0];
            const double y0 = entity[n].coordinate[i - 1][1];
            const double x1 = entity[n].coordinate[i][0];
            const double y1 = entity[n].coordinate[i][1];

            const double dx = x1 - x0;
            const double dy = y1 - y0;
            const double length = std::sqrt(dx * dx + dy * dy);
            if (length < 1e-6)
            {
                continue;
            }

            const int endNode = findOrCreateNode(x1, y1);
            if (endNode == prevNode)
            {
                continue;
            }

            const int steps = static_cast<int>(std::ceil(length / kMaxSegmentLength));
            if (steps <= 1)
            {
                addEdge(prevNode, endNode, length);
            }
            else
            {
                // Insert intermediate nodes along the segment.
                int curNode = prevNode;
                for (int s = 1; s <= steps; ++s)
                {
                    const double t = static_cast<double>(s) / steps;
                    const double sx = x0 + dx * t;
                    const double sy = y0 + dy * t;
                    const int node = (s == steps) ? endNode : findOrCreateNode(sx, sy);
                    addEdge(curNode, node, length / steps);
                    curNode = node;
                }
            }

            prevNode = endNode;
        }
    }

    reader.freeMemory();
}

void RoadGraph::connectNearbyNodes(double maxDistance)
{
    if (m_nodes.empty())
    {
        return;
    }

    const double maxDistSq = maxDistance * maxDistance;
    const int maxRadiusCells = static_cast<int>(std::ceil(maxDistance / m_cellSize));

    for (int i = 0; i < static_cast<int>(m_nodes.size()); ++i)
    {
        const Node& a = m_nodes[i];

        int cx = static_cast<int>((a.x - m_minX) / m_cellSize);
        int cy = static_cast<int>((a.y - m_minY) / m_cellSize);
        cx = std::clamp(cx, 0, m_gridW - 1);
        cy = std::clamp(cy, 0, m_gridH - 1);

        for (int dy = -maxRadiusCells; dy <= maxRadiusCells; ++dy)
        {
            for (int dx = -maxRadiusCells; dx <= maxRadiusCells; ++dx)
            {
                const int gx = cx + dx;
                const int gy = cy + dy;
                if (gx < 0 || gy < 0 || gx >= m_gridW || gy >= m_gridH)
                {
                    continue;
                }

                for (int j = m_grid[gy * m_gridW + gx]; j != -1; j = m_gridNext[j])
                {
                    // Only consider later nodes to avoid duplicate edges, and
                    // only bridge across road classes (same-class nodes are
                    // already connected by their own segments).
                    if (j <= i || m_nodes[j].roadClass == a.roadClass)
                    {
                        continue;
                    }

                    const double dxNode = m_nodes[j].x - a.x;
                    const double dyNode = m_nodes[j].y - a.y;
                    const double distSq = dxNode * dxNode + dyNode * dyNode;
                    if (distSq > maxDistSq)
                    {
                        continue;
                    }

                    const double dist = std::sqrt(distSq);
                    const int size = static_cast<int>(m_adjacency.size());
                    if (size <= j)
                    {
                        m_adjacency.resize(j + 1);
                    }
                    m_segments.push_back(Segment{i, j, dist});
                    m_adjacency[i].push_back(Edge{j, dist});
                    m_adjacency[j].push_back(Edge{i, dist});
                }
            }
        }
    }
}

void RoadGraph::markOpenEnds()
{
    // A minor-road node with a single connection is an open end (a dead end
    // or cul-de-sac). Buses don't terminate at such roads, so flag them to be
    // excluded from snapping. Must run after the graph and bridge pass are
    // complete so the degree reflects the final connectivity.
    for (int i = 0; i < static_cast<int>(m_nodes.size()); ++i)
    {
        if (m_nodes[i].roadClass == 1 && m_adjacency[i].size() == 1)
        {
            m_nodes[i].openEnd = true;
        }
    }
}

int RoadGraph::nearestNodeInRadius(double x, double y, double maxRadius) const
{
    // Public snapping skips open-end minor nodes (dead ends).
    return nearestNodeInRadius(x, y, maxRadius, true);
}

int RoadGraph::nearestNodeInRadius(double x, double y, double maxRadius, bool excludeOpenEnds) const
{
    if (m_nodes.empty())
    {
        return -1;
    }

    const double maxDistSq = maxRadius * maxRadius;

    int cx = static_cast<int>((x - m_minX) / m_cellSize);
    int cy = static_cast<int>((y - m_minY) / m_cellSize);

    // Expand the search ring by ring, up to the radius that covers
    // maxRadius (plus one cell for safety).
    const int maxRadiusCells = static_cast<int>(std::ceil(maxRadius / m_cellSize)) + 1;
    double bestDist = std::numeric_limits<double>::max();
    int bestNode = -1;

    for (int radius = 0; radius <= maxRadiusCells; ++radius)
    {
        // The closest cell in this ring is at Chebyshev distance `radius` from
        // the query cell, so no node in it can be nearer than (radius - 1) *
        // cellSize. Stop expanding once that lower bound exceeds maxRadius.
        if (radius > 0)
        {
            const double minRingDist = (radius - 1) * m_cellSize;
            if (minRingDist * minRingDist > maxDistSq)
            {
                break;
            }
        }

        for (int oy = -radius; oy <= radius; ++oy)
        {
            for (int ox = -radius; ox <= radius; ++ox)
            {
                // Only visit the ring at this radius (avoid re-checking inner
                // cells), except for radius 0.
                if (radius > 0 && std::abs(ox) != radius && std::abs(oy) != radius)
                {
                    continue;
                }
                const int gx = cx + ox;
                const int gy = cy + oy;
                if (gx < 0 || gx >= m_gridW || gy < 0 || gy >= m_gridH)
                {
                    continue;
                }
                int node = m_grid[gy * m_gridW + gx];
                while (node != -1)
                {
                    if (excludeOpenEnds && m_nodes[node].openEnd)
                    {
                        node = m_gridNext[node];
                        continue;
                    }
                    const double dx = m_nodes[node].x - x;
                    const double dy = m_nodes[node].y - y;
                    const double dist = dx * dx + dy * dy;
                    if (dist < bestDist)
                    {
                        bestDist = dist;
                        bestNode = node;
                    }
                    node = m_gridNext[node];
                }
            }
        }
    }

    return bestNode;
}

int RoadGraph::nearestNode(double x, double y) const
{
    // Unbounded: search the whole grid (the bounding-box diagonal is a safe
    // upper bound on the distance to any node). Open ends are kept here as a
    // last resort so a stop always snaps to *something*.
    return nearestNodeInRadius(x, y, std::hypot(m_maxX - m_minX, m_maxY - m_minY), false);
}

int RoadGraph::snapToward(double x, double y, double dirX, double dirY, double maxRadius) const
{
    if (m_nodes.empty())
    {
        return -1;
    }

    // A degenerate direction: just snap to the nearest node in radius.
    const double dirLen = std::hypot(dirX, dirY);
    if (dirLen < 1e-9)
    {
        return nearestNodeInRadius(x, y, maxRadius);
    }
    const double ux = dirX / dirLen;
    const double uy = dirY / dirLen;

    const double maxDistSq = maxRadius * maxRadius;

    int cx = static_cast<int>((x - m_minX) / m_cellSize);
    int cy = static_cast<int>((y - m_minY) / m_cellSize);
    const int maxRadiusCells = static_cast<int>(std::ceil(maxRadius / m_cellSize)) + 1;

    double bestScore = std::numeric_limits<double>::max();
    int bestNode = -1;

    for (int radius = 0; radius <= maxRadiusCells; ++radius)
    {
        if (radius > 0)
        {
            const double minRingDist = (radius - 1) * m_cellSize;
            if (minRingDist * minRingDist > maxDistSq)
            {
                break;
            }
        }

        for (int oy = -radius; oy <= radius; ++oy)
        {
            for (int ox = -radius; ox <= radius; ++ox)
            {
                if (radius > 0 && std::abs(ox) != radius && std::abs(oy) != radius)
                {
                    continue;
                }
                const int gx = cx + ox;
                const int gy = cy + oy;
                if (gx < 0 || gx >= m_gridW || gy < 0 || gy >= m_gridH)
                {
                    continue;
                }
                int node = m_grid[gy * m_gridW + gx];
                while (node != -1)
                {
                    // Skip open-end minor nodes (dead ends) so a stop snaps to
                    // a through road rather than a cul-de-sac.
                    if (m_nodes[node].openEnd)
                    {
                        node = m_gridNext[node];
                        continue;
                    }
                    const double dx = m_nodes[node].x - x;
                    const double dy = m_nodes[node].y - y;
                    const double distSq = dx * dx + dy * dy;
                    if (distSq <= maxDistSq)
                    {
                        // Score: distance to the point, minus a bonus for how
                        // far ahead of the point the node lies along the travel
                        // direction. Nodes that are close and in the direction
                        // of travel score lowest (best).
                        const double ahead = dx * ux + dy * uy;
                        const double score = std::sqrt(distSq) - 0.5 * ahead;
                        if (score < bestScore)
                        {
                            bestScore = score;
                            bestNode = node;
                        }
                    }
                    node = m_gridNext[node];
                }
            }
        }
    }

    return bestNode;
}

RoadGraph::Polyline RoadGraph::route(double x0, double y0, double x1, double y1) const
{
    Polyline result;
    if (m_nodes.empty())
    {
        return result;
    }

    const int start = nearestNode(x0, y0);
    const int goal = nearestNode(x1, y1);
    if (start < 0 || goal < 0)
    {
        return result;
    }

    // Route between the snapped nodes, then pin the exact endpoints so the
    // polyline starts/ends precisely at the requested points.
    Polyline path = routeBetween(start, goal);
    if (!path.empty())
    {
        path.front() = QPointF(x0, y0);
        path.back() = QPointF(x1, y1);
    }
    return path;
}

RoadGraph::Polyline RoadGraph::routeBetween(int startNode, int goalNode) const
{
    Polyline result;
    if (m_nodes.empty() || startNode < 0 || goalNode < 0 ||
        startNode >= static_cast<int>(m_nodes.size()) || goalNode >= static_cast<int>(m_nodes.size()))
    {
        return result;
    }

    // Dijkstra from the start node to the goal node.
    const int n = static_cast<int>(m_nodes.size());
    std::vector<double> dist(n, std::numeric_limits<double>::max());
    std::vector<int> prev(n, -1);
    std::vector<char> visited(n, 0);

    using HeapItem = std::pair<double, int>;  // (distance, node)
    std::priority_queue<HeapItem, std::vector<HeapItem>, std::greater<HeapItem>> pq;

    dist[startNode] = 0.0;
    pq.push({0.0, startNode});

    while (!pq.empty())
    {
        const auto [d, u] = pq.top();
        pq.pop();
        if (visited[u])
        {
            continue;
        }
        visited[u] = 1;
        if (u == goalNode)
        {
            break;
        }
        if (d > kMaxRouteDistance)
        {
            continue;
        }
        for (const Edge& e : m_adjacency[u])
        {
            const double nd = d + e.weight;
            if (nd < dist[e.to])
            {
                dist[e.to] = nd;
                prev[e.to] = u;
                pq.push({nd, e.to});
            }
        }
    }

    if (prev[goalNode] == -1 && goalNode != startNode)
    {
        // No route found: fall back to a straight segment between the nodes.
        result.push_back(QPointF(m_nodes[startNode].x, m_nodes[startNode].y));
        result.push_back(QPointF(m_nodes[goalNode].x, m_nodes[goalNode].y));
        return result;
    }

    // Reconstruct the node path from goal back to start.
    std::vector<int> path;
    for (int cur = goalNode; cur != -1; cur = prev[cur])
    {
        path.push_back(cur);
        if (cur == startNode)
        {
            break;
        }
    }
    std::reverse(path.begin(), path.end());

    // Emit the polyline: the start node, then the intermediate nodes, then the
    // goal node.
    for (int i = 0; i < static_cast<int>(path.size()); ++i)
    {
        result.push_back(QPointF(m_nodes[path[i]].x, m_nodes[path[i]].y));
    }
    return result;
}
