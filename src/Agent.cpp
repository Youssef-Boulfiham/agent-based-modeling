#include "../include/Agent.h"
#include "../include/Env.h"
#include "../include/Pathfinding.h"
#include <algorithm>
#include <random>

// Shared RNG for all agents
static std::mt19937& rng() {
    static std::mt19937 engine(std::random_device{}());
    return engine;
}
static float uniform01() {
    static std::uniform_real_distribution<float> d(0.0f, 1.0f);
    return d(rng());
}
static int randInt(int lo, int hi) {
    std::uniform_int_distribution<int> d(lo, hi);
    return d(rng());
}
template <typename T>
static const T& randChoice(const std::vector<T>& v) {
    return v[randInt(0, static_cast<int>(v.size()) - 1)];
}

Agent::Agent(int id, glm::vec2 startPos, Env* env)
    : id(id), position(startPos), targetDomain(-1), assignedActivity("idle"), world(env) {
    // Start in random activity with random valid position
    if (world) {
        const auto& names = world->getActivityNames();
        if (!names.empty()) {
            activity = randChoice(names);
            const Activity* act = world->findActivity(activity);
            if (act && !act->positions.empty()) {
                positionCurrent = randChoice(act->positions);
                targetDomain = act->domain;
                assignedActivity = activity;
            }
        }
    }
    position = positionCurrent;
    status = "['" + activity + "']";
}

Agent::~Agent() {}

void Agent::step(float /*deltaTime*/) {
    if (!world) return;

    // Walking behavior: domain-driven with corridor routing
    int currentRoom = world->roomOf(positionCurrent);

    // Check: am I in the target domain?
    if (currentRoom != targetDomain) {
        // NOT in target domain -> move_to_domain: route via corridor to domain center
        activity = "move to domain";
        if (path.empty()) {
            routeToDomain(targetDomain);
        }
        // Move one cell toward goal
        if (!path.empty()) {
            positionCurrent = path.front();
            path.erase(path.begin());
            position = positionCurrent;

            // Check if just arrived in target domain
            if (world->roomOf(positionCurrent) == targetDomain) {
                path.clear();
                // Find an activity in this domain
                assignedActivity = world->findActivityInDomain(targetDomain);
                if (assignedActivity.empty()) {
                    assignedActivity = "idle"; // Fallback
                }
            }
        }
        return;
    }

    // In target domain -> execute assigned activity
    activity = assignedActivity;
    if (activity == "working") {
        // Stay put
        path.clear();
        return;
    }

    // idle: wander within current domain
    if (path.empty()) {
        path = bfs(positionCurrent, pickPosition());
    }
    if (!path.empty()) {
        positionCurrent = path.front();
        path.erase(path.begin());
        position = positionCurrent;
    }
}

void Agent::idle() {
    // idle agents wander; others mostly stand still
    if (activity != "idle" && uniform01() < 0.90f) {
        path.assign(randInt(5, 25), positionCurrent);
        status = "['" + activity + "']";
    } else {
        routeTo(pickPosition());
        status = "['" + activity + "']";
    }
}

void Agent::chooseActivity() {
    path.clear();

    const auto& activities = world->getActivities();
    const auto& names = world->getActivityNames();

    // Weighted draw by probability
    float total = 0.0f;
    for (const auto& name : names) {
        total += activities.at(name).probability;
    }
    if (total <= 0.0f) {
        idle();
        return;
    }

    float r = uniform01() * total;
    std::string next = names.back();
    float acc = 0.0f;
    for (const auto& name : names) {
        acc += activities.at(name).probability;
        if (r <= acc) {
            next = name;
            break;
        }
    }

    std::string previous = activity;
    activity = next;

    if (activity == previous) {
        idle();  // Already here
        return;
    }

    // Route to new activity
    glm::vec2 goal = pickPosition();
    routeTo(goal);
    status = "['" + previous + "', 'black', '" + activity + "']";
}

void Agent::makeFriends(glm::vec2 meetingPoint) {
    path.clear();
    routeTo(meetingPoint);
    const int dwellUntil = 450;
    while (static_cast<int>(path.size()) < dwellUntil) {
        path.push_back(meetingPoint);
    }
    status = "['" + activity + "', 'friends']";
}

glm::vec2 Agent::pickPosition() const {
    const Activity* act = world->findActivity(activity);
    if (!act || act->positions.empty()) return positionCurrent;

    // idle: fully random
    if (activity == "idle") return randChoice(act->positions);

    // Otherwise bias toward nearby points (Manhattan distance)
    std::vector<glm::vec2> sorted = act->positions;
    std::sort(sorted.begin(), sorted.end(),
        [this](const glm::vec2& a, const glm::vec2& b) {
            float da = std::abs(a.x - positionCurrent.x) + std::abs(a.y - positionCurrent.y);
            float db = std::abs(b.x - positionCurrent.x) + std::abs(b.y - positionCurrent.y);
            return da < db;
        });

    size_t half = sorted.size() / 2;
    if (half > 0 && uniform01() < 0.75f) {
        return sorted[randInt(0, static_cast<int>(half) - 1)];
    }
    return randChoice(sorted);
}

std::vector<int> Agent::allowedDomains() const {
    std::vector<int> allowed;
    const Activity* act = world->findActivity(activity);
    if (act) allowed.push_back(act->domain);
    return allowed;
}

void Agent::routeTo(glm::vec2 goal) {
    const WalkGrid& grid = world->getWalkGrid();
    Path p = Pathfinding::findPath(positionCurrent, goal, grid, allowedDomains());
    if (p.found) {
        path.insert(path.end(), p.waypoints.begin(), p.waypoints.end());
    } else {
        path.assign(randInt(5, 15), positionCurrent);
    }
}

void Agent::routeToDomain(int domain) {
    glm::vec2 goal = world->domainCenter(domain);
    const WalkGrid& grid = world->getWalkGrid();
    // Allow corridor + target domain
    std::vector<int> allowed = {domain};
    Path p = Pathfinding::findPath(positionCurrent, goal, grid, allowed);
    if (p.found) {
        path.insert(path.end(), p.waypoints.begin(), p.waypoints.end());
    } else {
        // Fallback: stay put or wander locally
        path.clear();
    }
}

// BFS path within current domain, for idle wandering
std::vector<glm::vec2> Agent::bfs(glm::vec2 start, glm::vec2 goal) const {
    if (!world) return {};

    const WalkGrid& grid = world->getWalkGrid();
    int currentDomain = world->roomOf(start);

    // Allow corridor + current domain
    std::vector<int> allowed = {currentDomain};
    Path p = Pathfinding::findPath(start, goal, grid, allowed);

    return p.found ? p.waypoints : std::vector<glm::vec2>();
}
