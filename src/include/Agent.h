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
    // NOTE: `activity` is a BEHAVIOUR state, never a domain/color name.
    //   {idle, working, offline, standby, walking to a domain}.
    // The domain (color-named space) is tracked separately by targetDomain/roomOf.
    int targetDomain;                      // where agent MUST be (externally set)
    std::string assignedActivity;          // behaviour state on arrival (idle/working/offline/standby)

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
    const std::vector<glm::vec2>& getPath() const { return path; }

    // Setters
    void setPosition(glm::vec2 pos) { position = pos; }
    void setTargetDomain(int domain) {
        // Target changed externally -> flush the old route so the next step
        // replans toward the new domain via the corridor (manual: doel kan elke
        // step veranderen -> herplan).
        if (domain != targetDomain) path.clear();
        targetDomain = domain;
    }
    void connect(int otherId) { connections.push_back(otherId); }

    // Behavior primitives
    void idle();
    void chooseActivity();
    void makeFriends(glm::vec2 meetingPoint);

private:
    glm::vec2 pickPosition() const;
    glm::vec2 pickWanderPosition() const;   // random stand-pos in CURRENT domain (idle wander)
    std::string pickAssignedActivity() const; // draw a behaviour state on domain arrival
    void routeTo(glm::vec2 goal);
    void routeToDomain(int domain);
    std::vector<glm::vec2> bfs(glm::vec2 start, glm::vec2 goal) const;
    std::vector<int> allowedDomains() const;
};

#endif // AGENT_H
