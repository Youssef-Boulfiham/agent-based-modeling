#include "../include/Pathfinding.h"
#include <cmath>
#include <algorithm>
#include <queue>
#include <set>

float Pathfinding::heuristic(glm::ivec2 a, glm::ivec2 b) {
    float dx = a.x - b.x;
    float dy = a.y - b.y;
    return std::sqrt(dx * dx + dy * dy);
}

glm::ivec2 Pathfinding::worldToGrid(glm::vec2 world) {
    return glm::ivec2(world.x / GRID_SIZE, world.y / GRID_SIZE);
}

glm::vec2 Pathfinding::gridToWorld(glm::ivec2 grid) {
    return glm::vec2(grid.x * GRID_SIZE, grid.y * GRID_SIZE);
}

Path Pathfinding::findPath(glm::vec2 start, glm::vec2 goal, float worldWidth, float worldHeight) {
    Path path;

    glm::ivec2 startGrid = worldToGrid(start);
    glm::ivec2 goalGrid = worldToGrid(goal);

    if (startGrid == goalGrid) {
        path.waypoints.push_back(goal);
        path.found = true;
        return path;
    }

    auto openSet = std::vector<Node*>();
    auto closedSet = std::vector<glm::ivec2>();

    Node* startNode = new Node(startGrid);
    startNode->g = 0;
    startNode->h = heuristic(startGrid, goalGrid);
    openSet.push_back(startNode);

    const glm::ivec2 directions[] = {
        {0, 1}, {1, 0}, {0, -1}, {-1, 0},
        {1, 1}, {1, -1}, {-1, 1}, {-1, -1}
    };

    Node* current = nullptr;

    while (!openSet.empty()) {
        // Find node with lowest f score
        auto minIt = std::min_element(openSet.begin(), openSet.end(),
            [](Node* a, Node* b) { return a->f() < b->f(); });

        current = *minIt;
        openSet.erase(minIt);

        if (current->pos == goalGrid) {
            path.found = true;

            // Reconstruct path
            while (current) {
                path.waypoints.insert(path.waypoints.begin(), gridToWorld(current->pos));
                current = current->parent;
            }

            // Cleanup
            for (auto node : openSet) delete node;

            return path;
        }

        closedSet.push_back(current->pos);

        for (const auto& dir : directions) {
            glm::ivec2 neighbor = current->pos + dir;

            // Bounds check
            if (neighbor.x < 0 || neighbor.y < 0 ||
                neighbor.x >= worldWidth / GRID_SIZE ||
                neighbor.y >= worldHeight / GRID_SIZE) {
                continue;
            }

            // Check if in closed set
            if (std::find(closedSet.begin(), closedSet.end(), neighbor) != closedSet.end()) {
                continue;
            }

            float moveCost = (dir.x != 0 && dir.y != 0) ? 1.414f : 1.0f;
            float tentativeG = current->g + moveCost;

            // Check if neighbor already in open set
            auto existingIt = std::find_if(openSet.begin(), openSet.end(),
                [neighbor](Node* n) { return n->pos == neighbor; });

            if (existingIt == openSet.end()) {
                Node* newNode = new Node(neighbor);
                newNode->g = tentativeG;
                newNode->h = heuristic(neighbor, goalGrid);
                newNode->parent = current;
                openSet.push_back(newNode);
            } else {
                if (tentativeG < (*existingIt)->g) {
                    (*existingIt)->g = tentativeG;
                    (*existingIt)->parent = current;
                }
            }
        }
    }

    // No path found
    path.found = false;
    for (auto node : openSet) delete node;

    return path;
}

Path Pathfinding::findPath(glm::vec2 start, glm::vec2 goal,
                           const WalkGrid& grid,
                           const std::vector<int>& allowedDomains) {
    Path path;
    if (grid.cols == 0 || grid.rows == 0) return path;

    // A cell is passable if not blocked and (no filter | corridor | allowed).
    auto passable = [&](glm::ivec2 c) -> bool {
        if (!grid.inBounds(c)) return false;
        int d = grid.domainAt(c);
        if (d == WalkGrid::BLOCKED) return false;
        if (allowedDomains.empty() || d == WalkGrid::CORRIDOR) return true;
        return std::find(allowedDomains.begin(), allowedDomains.end(), d)
               != allowedDomains.end();
    };

    glm::ivec2 startGrid = grid.toGrid(start);
    glm::ivec2 goalGrid = grid.toGrid(goal);

    // Snap an unreachable start/goal onto the grid as best we can.
    if (!grid.inBounds(startGrid) || !grid.inBounds(goalGrid)) return path;

    if (startGrid == goalGrid) {
        path.waypoints.push_back(goal);
        path.found = true;
        return path;
    }

    std::vector<Node*> openSet;
    std::vector<glm::ivec2> closedSet;

    Node* startNode = new Node(startGrid);
    startNode->g = 0;
    startNode->h = heuristic(startGrid, goalGrid);
    openSet.push_back(startNode);

    const glm::ivec2 directions[] = {
        {0, 1}, {1, 0}, {0, -1}, {-1, 0},
        {1, 1}, {1, -1}, {-1, 1}, {-1, -1}
    };

    Node* current = nullptr;
    std::vector<Node*> allNodes; // own every node for cleanup
    allNodes.push_back(startNode);

    while (!openSet.empty()) {
        auto minIt = std::min_element(openSet.begin(), openSet.end(),
            [](Node* a, Node* b) { return a->f() < b->f(); });
        current = *minIt;
        openSet.erase(minIt);

        if (current->pos == goalGrid) {
            path.found = true;
            while (current) {
                path.waypoints.insert(path.waypoints.begin(),
                                      grid.toWorld(current->pos));
                current = current->parent;
            }
            // Land exactly on the requested goal.
            if (!path.waypoints.empty()) path.waypoints.back() = goal;
            for (auto n : allNodes) delete n;
            return path;
        }

        closedSet.push_back(current->pos);

        for (const auto& dir : directions) {
            glm::ivec2 neighbor = current->pos + dir;
            if (!passable(neighbor)) continue;

            // Disallow diagonal squeezes between two blocked cells.
            if (dir.x != 0 && dir.y != 0) {
                if (!passable({current->pos.x + dir.x, current->pos.y}) &&
                    !passable({current->pos.x, current->pos.y + dir.y}))
                    continue;
            }

            if (std::find(closedSet.begin(), closedSet.end(), neighbor)
                != closedSet.end())
                continue;

            float moveCost = (dir.x != 0 && dir.y != 0) ? 1.414f : 1.0f;
            float tentativeG = current->g + moveCost;

            auto existingIt = std::find_if(openSet.begin(), openSet.end(),
                [neighbor](Node* n) { return n->pos == neighbor; });

            if (existingIt == openSet.end()) {
                Node* newNode = new Node(neighbor);
                newNode->g = tentativeG;
                newNode->h = heuristic(neighbor, goalGrid);
                newNode->parent = current;
                openSet.push_back(newNode);
                allNodes.push_back(newNode);
            } else if (tentativeG < (*existingIt)->g) {
                (*existingIt)->g = tentativeG;
                (*existingIt)->parent = current;
            }
        }
    }

    path.found = false;
    for (auto n : allNodes) delete n;
    return path;
}
