#ifndef AGENT_H
#define AGENT_H

#include <glm/glm.hpp>
#include <string>
#include <vector>

// Forward declaration to avoid circular dependency
struct WalkGrid;

class Agent {
protected:
    int id;
    glm::vec2 position;

    // Rational behavior state (agent walks a path, chooses activities, meets others)
    std::string activity;
    std::string status;
    std::vector<glm::vec2> path;           // waypoints to follow (front = next)
    glm::vec2 positionCurrent;             // last cell stood on
    std::vector<int> connections;          // ids of connected agents

    // Walking behavior (domain-driven)
    int targetDomain;                      // where agent MUST be (externally set)
    std::string assignedActivity;          // idle or working (assigned when reaching domain)

    // Shared world context (passed at construction, read-only)
    class Env* world;

public:
    Agent(int id, glm::vec2 startPos, Env* env);
    virtual ~Agent();

    void step(float deltaTime);

    // Getters
    int getId() const { return id; }
    glm::vec2 getPosition() const { return position; }
    const std::string& getActivity() const { return activity; }
    const std::string& getStatus() const { return status; }
    bool isBusy() const { return !path.empty(); }
    int getTargetDomain() const { return targetDomain; }

    // Setters
    void setPosition(glm::vec2 pos) { position = pos; }
    void setTargetDomain(int domain) { targetDomain = domain; }
    void connect(int otherId) { connections.push_back(otherId); }

    // Behavior primitives
    void idle();
    void chooseActivity();
    void makeFriends(glm::vec2 meetingPoint);

private:
    glm::vec2 pickPosition() const;
    void routeTo(glm::vec2 goal);
    void routeToDomain(int domain);
    std::vector<glm::vec2> bfs(glm::vec2 start, glm::vec2 goal) const;
    std::vector<int> allowedDomains() const;
};

#endif // AGENT_H
