/*
 * PATHFINDING SYSTEM - Architecture & Design
 *
 * PURPOSE:
 * Provides pathfinding utilities for agents to navigate from point A to point B.
 * Uses A* algorithm for efficient pathfinding on a grid-based world.
 *
 * DESIGN:
 * - Path: Data structure holding waypoints from start to goal
 * - Pathfinding: Static utility class with pathfinding algorithms
 * - Grid-based world: World is divided into cells for pathfinding
 *
 * HOW TO USE:
 *
 * 1. Find a path from agent to target:
 *    glm::vec2 start = agent->getPosition();
 *    glm::vec2 goal(100, 150);
 *    Path path = Pathfinding::findPath(start, goal, worldWidth, worldHeight);
 *
 * 2. Get next waypoint:
 *    if (!path.waypoints.empty()) {
 *        glm::vec2 nextPoint = path.waypoints.front();
 *        // Move toward nextPoint
 *    }
 *
 * 3. Remove completed waypoint:
 *    path.waypoints.erase(path.waypoints.begin());
 *
 * GRID SYSTEM:
 * - World is divided into cells (GRID_SIZE x GRID_SIZE pixels per cell)
 * - Pathfinding operates on grid, not continuous space
 * - Smaller GRID_SIZE = more precise but slower
 * - Larger GRID_SIZE = faster but less precise
 *
 * LIMITATIONS:
 * - Currently no obstacles (future: add obstacle layer)
 * - A* assumes 8-directional movement (including diagonals)
 * - Grid-based, so paths snap to grid
 */

#ifndef PATHFINDING_H
#define PATHFINDING_H

#include <glm/glm.hpp>
#include <vector>

struct Path {
    std::vector<glm::vec2> waypoints;
    bool found;

    Path() : found(false) {}
};

/*
 * WalkGrid - a domain-tagged, collision-aware grid for pathfinding.
 *
 * Each cell carries an integer "domain" id (the C++ equivalent of the
 * Python prototype's activity "colors"). A cell is walkable when:
 *   - it is in bounds, AND
 *   - its domain is NOT BLOCKED, AND
 *   - either no domain filter is given, or the cell's domain is allowed.
 *
 * This lets an agent restrict its route to the regions that belong to its
 * current activity (e.g. only "learning" cells, plus shared corridors),
 * mirroring search_path(start, end, activity=colors_allowed).
 */
struct WalkGrid {
    static constexpr int BLOCKED = -1;  // impassable cell (wall / collision)
    static constexpr int CORRIDOR = 0;  // shared walkable cell (any activity)

    int cols = 0;
    int rows = 0;
    int cellSize = 10;            // world units per cell
    std::vector<int> domains;     // size cols*rows, row-major

    WalkGrid() = default;
    WalkGrid(int cols, int rows, int cellSize, int fill = CORRIDOR)
        : cols(cols), rows(rows), cellSize(cellSize),
          domains(static_cast<size_t>(cols) * rows, fill) {}

    bool inBounds(glm::ivec2 c) const {
        return c.x >= 0 && c.y >= 0 && c.x < cols && c.y < rows;
    }
    int domainAt(glm::ivec2 c) const { return domains[c.y * cols + c.x]; }
    void setDomain(glm::ivec2 c, int d) { domains[c.y * cols + c.x] = d; }

    glm::ivec2 toGrid(glm::vec2 w) const {
        return glm::ivec2(static_cast<int>(w.x) / cellSize,
                          static_cast<int>(w.y) / cellSize);
    }
    glm::vec2 toWorld(glm::ivec2 c) const {
        return glm::vec2(c.x * cellSize + cellSize * 0.5f,
                         c.y * cellSize + cellSize * 0.5f);
    }
};

class Pathfinding {
private:
    static constexpr int GRID_SIZE = 10;

    struct Node {
        glm::ivec2 pos;
        float g;
        float h;
        float f() const { return g + h; }
        Node* parent;

        Node(glm::ivec2 p) : pos(p), g(0), h(0), parent(nullptr) {}
    };

    static float heuristic(glm::ivec2 a, glm::ivec2 b);
    static glm::ivec2 worldToGrid(glm::vec2 world);
    static glm::vec2 gridToWorld(glm::ivec2 grid);

public:
    // Legacy obstacle-free pathfinding on a bare rectangle.
    static Path findPath(glm::vec2 start, glm::vec2 goal, float worldWidth, float worldHeight);

    // Domain/collision-aware A* on a WalkGrid.
    // allowedDomains: cells an agent may traverse. CORRIDOR is always allowed.
    //                 Empty => every non-BLOCKED cell is allowed.
    static Path findPath(glm::vec2 start, glm::vec2 goal,
                         const WalkGrid& grid,
                         const std::vector<int>& allowedDomains = {});
};

#endif // PATHFINDING_H
