#ifndef ENV_H
#define ENV_H

#include "Agent.h"
#include "Pathfinding.h"
#include "MessageLog.h"
#include <SDL2/SDL.h>
#include <string>
#include <vector>
#include <unordered_map>
#include <glm/glm.hpp>

struct Activity {
    int domain = WalkGrid::CORRIDOR;
    float probability = 0.0f;
    std::vector<glm::vec2> positions;
};

class Env {
private:
    float width;
    float height;
    float deltaTime;
    int frameCount;
    bool isRunning;

    std::vector<Agent*> agents;
    int maxAgents;
    int activeAgents;

    WalkGrid grid;
    std::unordered_map<std::string, Activity> activities;
    std::vector<std::string> activityNames;

    SDL_Rect envArea;

    // Visual layers (loaded lazily on first render).
    //   background.png  -> behind  (the navigation map / domains)
    //   enviroment.png  -> on top of background (detailed art)
    //   agents          -> drawn last
    SDL_Texture* bgTexture = nullptr;
    SDL_Texture* envTexture = nullptr;
    bool texturesTried = false;

    // Layer view toggle (first top-left button switches it).
    //   true  -> environment view: background + enviroment art on top
    //   false -> background view:  background layer only
    bool showEnvLayer = true;
    // Path overlay toggle (second top-left button): draw each agent's planned
    // route so you can see where they are going and debug walkable-area issues.
    bool showPaths = false;

    void buildWorld();
    void loadTextures(SDL_Renderer* renderer);

    // External domain controller: per spec, target domain is set externally and
    // re-checked each step. Reassigns each agent a new target domain at intervals.
    void controlAgentDomains();
    std::vector<int> domainList;            // domains available for assignment
    std::unordered_map<int, int> nextChangeAt; // agentId -> frame of next domain change

    // "sanity check" choreography. NORMAL = per-agent random reassign (default).
    // GATHER = all agents forced to the canteen (geometric-middle domain) until
    // every agent has arrived. SCATTER = each agent picks one random domain once,
    // then we fall back to NORMAL.
    enum class Phase { NORMAL, GATHER, SCATTER };
    Phase phase = Phase::NORMAL;
    int canteenDomain = WalkGrid::CORRIDOR; // gather target, chosen when GATHER begins
    int canteenMiddleDomain() const;        // domain whose center is nearest map middle

    // Message logging and priority queue
    MessageLog* messageLog;
    std::unordered_map<int, int> lastReportedDomain; // agentId -> domain at its last chatter

public:
    Env(float w, float h, int maxAgents);
    ~Env();

    void initialize();
    void step();
    void update(float dt);
    void render();
    void renderEnv(SDL_Renderer* renderer, int x, int y, int width, int height);
    void cleanup();

    // Switch between environment layer and background layer.
    void toggleLayer() { showEnvLayer = !showEnvLayer; }

    // Show/hide agent path overlay.
    void togglePaths() { showPaths = !showPaths; }

    void addAgent(glm::vec2 position);
    void removeAgent(int index);
    void updateAgents();

    // World/activity query interface for agents
    const WalkGrid& getWalkGrid() const { return grid; }
    const std::unordered_map<std::string, Activity>& getActivities() const { return activities; }
    const std::vector<std::string>& getActivityNames() const { return activityNames; }
    const Activity* findActivity(const std::string& name) const {
        auto it = activities.find(name);
        return it == activities.end() ? nullptr : &it->second;
    }

    void addActivity(const std::string& name, int domain, float probability,
                     std::vector<glm::vec2> positions) {
        activities[name] = Activity{domain, probability, std::move(positions)};
        activityNames.push_back(name);
    }

    // Walking behavior helper: determine which domain a world position is in
    // Returns the domain id, or WalkGrid::CORRIDOR (-1 or 0) if not in any activity domain
    int roomOf(glm::vec2 worldPos) const;

    // Walking behavior helper: find center of domain
    glm::vec2 domainCenter(int domain) const;

    // Walking behavior helper: find an activity name in the given domain
    std::string findActivityInDomain(int domain) const;

    float getWidth() const { return width; }
    float getHeight() const { return height; }
    int getFrameCount() const { return frameCount; }
    int getActiveAgents() const { return activeAgents; }
    const std::vector<Agent*>& getAgents() const { return agents; }
    bool getIsRunning() const { return isRunning; }

    void setRunning(bool running) { isRunning = running; }
    void setDeltaTime(float dt) { deltaTime = dt; }

    // Message logging interface
    MessageLog* getMessageLog() const { return messageLog; }
    void queueUserInput(const std::string& text, int agentId);
};

#endif // ENV_H
