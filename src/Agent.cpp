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

// Agent's own grid level: each step advances this many underlying path cells.
// The A* path is contiguous (neighbour cells), so a stride of 4 still follows
// the walkable route — the agent never leaves the path / jumps off-map.
static constexpr int STEP_CELLS = 4;

Agent::Agent(int id, glm::vec2 startPos, Env* env)
    : id(id), position(startPos), activity("idle"), targetDomain(-1),
      assignedActivity("idle"), world(env) {
    // Start standing in a random domain with a valid position. The domain is
    // the (color-named) space; `activity` stays a behaviour state, never a name.
    if (world) {
        const auto& names = world->getActivityNames();
        if (!names.empty()) {
            const std::string& dname = randChoice(names);   // a DOMAIN (color) name
            const Activity* act = world->findActivity(dname);
            if (act && !act->positions.empty()) {
                positionCurrent = randChoice(act->positions);
                targetDomain = act->domain;
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
        // NOT in target domain -> route via corridor to domain center.
        // Activity is the en-route behaviour state, NOT a domain name.
        activity = "walking to a domain";
        if (path.empty()) {
            routeToDomain(targetDomain);
        }
        // Advance STEP_CELLS cells toward goal (slowed by 40%). Position and
        // path stay in sync: each iteration pops the next contiguous waypoint.
        if (!path.empty() && uniform01() < 0.6f) {
            for (int s = 0; s < STEP_CELLS && !path.empty(); ++s) {
                positionCurrent = path.front();
                path.erase(path.begin());
                position = positionCurrent;

                // On arrival, draw a behaviour state for this domain visit.
                if (world->roomOf(positionCurrent) == targetDomain) {
                    path.clear();
                    assignedActivity = pickAssignedActivity();
                    break;
                }
            }
        }
        return;
    }

    // In target domain -> execute the assigned behaviour state.
    // {idle, working, offline, standby} — for now all four wander the room
    // (idle behaviour per the walking manual: stroll within the domain).
    activity = assignedActivity;

    if (path.empty()) {
        path = bfs(positionCurrent, pickWanderPosition());
    }
    if (!path.empty() && uniform01() < 0.6f) {
        for (int s = 0; s < STEP_CELLS && !path.empty(); ++s) {
            positionCurrent = path.front();
            path.erase(path.begin());
            position = positionCurrent;
        }
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

// Random stand-position inside the domain the agent is CURRENTLY in.
// Keyed off the domain (roomOf), never the activity string — so idle wander
// works for any behaviour state. Idle = fully random per the walking manual.
glm::vec2 Agent::pickWanderPosition() const {
    int d = world->roomOf(positionCurrent);
    std::string dname = world->findActivityInDomain(d);
    if (dname.empty()) return positionCurrent;
    const Activity* act = world->findActivity(dname);
    if (!act || act->positions.empty()) return positionCurrent;
    return randChoice(act->positions);
}

// Draw a behaviour state when the agent reaches its target domain.
// working/offline/standby currently behave like idle (wander the room).
std::string Agent::pickAssignedActivity() const {
    static const char* states[] = {"idle", "working", "offline", "standby"};
    return states[randInt(0, 3)];
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
    // Route over any walkable (non-BLOCKED) cell — matches the sandbox reference
    // bfs() which has no domain filter. A restricted {domain} filter traps an
    // agent in the interior of its current domain (its own cells aren't allowed),
    // leaving it frozen in "walking to a domain". Empty filter => all walkable.
    std::vector<int> allowed = {};
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
