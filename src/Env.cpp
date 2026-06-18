#include "../include/Env.h"
#include <SDL2/SDL_image.h>
#include <random>
#include <algorithm>
#include <iostream>
#include <fstream>
#include <sstream>

// Random number generation
static std::mt19937 rng(std::random_device{}());

// Load a domain mask CSV (row-major, values: -1 BLOCKED, 0 CORRIDOR, 1..N domain).
// Returns true on success; fills out cols/rows and the flat domain vector.
static bool loadMaskCSV(const std::string& path, int& cols, int& rows,
                        std::vector<int>& flat) {
    std::ifstream f(path);
    if (!f.is_open()) return false;
    flat.clear();
    rows = 0;
    cols = 0;
    std::string line;
    while (std::getline(f, line)) {
        if (line.empty()) continue;
        std::stringstream ss(line);
        std::string cell;
        int c = 0;
        while (std::getline(ss, cell, ',')) {
            flat.push_back(std::stoi(cell));
            ++c;
        }
        if (cols == 0) cols = c;
        ++rows;
    }
    return cols > 0 && rows > 0;
}

// Constructor: Initialize environment with given dimensions
Env::Env(float w, float h, int maxAgents)
    : width(w), height(h), deltaTime(0.016f),
      frameCount(0), isRunning(false), maxAgents(maxAgents),
      activeAgents(0), messageLog(nullptr) {
    agents.reserve(maxAgents);
    messageLog = new MessageLog();
}

// Destructor: Clean up all agents
Env::~Env() {
    cleanup();
}

// Build the walkable map layer: a corridor grid with four activity zones
// carved into it. Each zone is a domain region plus a set of stand positions.
void Env::buildWorld() {
    // Load the domain mask exported from the input map (data/input -> data/output).
    // Try a few relative paths so it works regardless of launch cwd.
    const char* candidates[] = {
        "data/output/mask_v13_seed1.csv",
        "../data/output/mask_v13_seed1.csv",
        "../../data/output/mask_v13_seed1.csv",
    };

    int cols = 0, rows = 0;
    std::vector<int> flat;
    bool loaded = false;
    for (const char* p : candidates) {
        if (loadMaskCSV(p, cols, rows, flat)) {
            std::cout << "Loaded mask: " << p << " (" << cols << "x" << rows << ")\n";
            loaded = true;
            break;
        }
    }

    if (!loaded) {
        std::cerr << "WARNING: mask CSV not found; falling back to empty corridor grid\n";
        int c = static_cast<int>(width) / 10, r = static_cast<int>(height) / 10;
        grid = WalkGrid(c, r, 10, WalkGrid::CORRIDOR);
        domainList.clear();
        return;
    }

    // Fit the mask grid into the world rectangle (cellSize chosen from world size).
    int cell = std::max(1, std::min(static_cast<int>(width) / cols,
                                    static_cast<int>(height) / rows));
    grid = WalkGrid(cols, rows, cell, WalkGrid::CORRIDOR);

    // Copy mask values into the grid and collect stand positions per domain.
    std::unordered_map<int, std::vector<glm::vec2>> positionsByDomain;
    int maxDomain = 0;
    for (int y = 0; y < rows; ++y) {
        for (int x = 0; x < cols; ++x) {
            int v = flat[y * cols + x];
            glm::ivec2 c(x, y);
            grid.setDomain(c, v);
            if (v > 0) {
                maxDomain = std::max(maxDomain, v);
                // sample every 3rd cell as a valid stand position
                if (x % 3 == 0 && y % 3 == 0)
                    positionsByDomain[v].push_back(grid.toWorld(c));
            }
        }
    }

    // Register one activity per domain (loop behaviour treats activity as separate;
    // here each domain gets a neutral activity name so agents have stand positions).
    domainList.clear();
    for (int d = 1; d <= maxDomain; ++d) {
        auto& pos = positionsByDomain[d];
        if (pos.empty()) continue;
        std::string name = "domain_" + std::to_string(d);
        addActivity(name, d, 1.0f / maxDomain, std::move(pos));
        domainList.push_back(d);
    }
}

// Initialize the environment and spawn initial agents
void Env::initialize() {
    buildWorld();

    int initialPopulation = maxAgents;
    for (int i = 0; i < initialPopulation; ++i) {
        addAgent(glm::vec2(0, 0));  // Position will be set by Agent constructor
    }

    isRunning = true;
    frameCount = 0;

    std::cout << "Environment initialized with " << activeAgents << " agents\n";
}

// Main simulation step
void Env::step() {
    if (!isRunning) return;

    // Process highest-priority messages from queue
    if (messageLog) messageLog->processQueue();

    controlAgentDomains();   // external controller: assign target domains
    updateAgents();
    activeAgents = agents.size();
    frameCount++;

    // Periodic agent-to-agent chatter (every ~20 frames)
    if (frameCount % 20 == 0 && agents.size() >= 2) {
        std::uniform_int_distribution<int> pick(0, static_cast<int>(agents.size()) - 1);
        int a_idx = pick(rng);
        int b_idx = pick(rng);
        if (a_idx != b_idx) {
            Agent* agentA = agents[a_idx];
            Agent* agentB = agents[b_idx];
            std::vector<std::string> msgs = {
                "Moving to domain",
                "Status update",
                "At position",
                "Switching domain",
                "Queue depth: " + std::to_string(static_cast<int>(frameCount % 10))
            };
            std::uniform_int_distribution<int> msg_pick(0, static_cast<int>(msgs.size()) - 1);
            std::string msg = msgs[msg_pick(rng)];

            if (messageLog) {
                messageLog->queueAgentChatter(
                    "Agent " + std::to_string(agentA->getId()),
                    "Agent " + std::to_string(agentB->getId()),
                    msg,
                    agentA->getPosition(),
                    "Domain " + std::to_string(agentA->getTargetDomain()),
                    agentA->getActivity()
                );
            }
        }
    }
}

// External domain controller (mirrors testcase externalDomainController).
// Per spec, the target domain is set EXTERNALLY (not by the agent). Each agent
// gets a new random target domain at least every 50 steps. The agent only
// reacts — it never chooses its own target.
void Env::controlAgentDomains() {
    if (domainList.empty()) return;

    static std::uniform_int_distribution<int> jitter(0, 39);
    std::uniform_int_distribution<int> pick(0, static_cast<int>(domainList.size()) - 1);

    for (Agent* agent : agents) {
        int aid = agent->getId();
        auto it = nextChangeAt.find(aid);
        if (it == nextChangeAt.end() || frameCount >= it->second) {
            int current = agent->getTargetDomain();
            int next;
            do { next = domainList[pick(rng)]; } while (next == current && domainList.size() > 1);
            agent->setTargetDomain(next);
            nextChangeAt[aid] = frameCount + 50 + jitter(rng);
        }
    }
}

// Update physics and behavior for all agents
void Env::update(float dt) {
    // Only stores the time delta; agents are advanced once in step().
    deltaTime = dt;
}

// Render simulation (placeholder for UI integration)
void Env::render() {
    // This will be called by the frontend visualization
    // For now, just output statistics every 60 frames
    if (frameCount % 60 == 0) {
        std::cout << "Frame: " << frameCount
                  << " | Agents: " << activeAgents << "\n";
    }
}

// Load the two visual layers once. Tries several relative paths (launch cwd varies).
void Env::loadTextures(SDL_Renderer* renderer) {
    texturesTried = true;
    auto tryLoad = [&](const char* name) -> SDL_Texture* {
        const std::string dirs[] = {"data/input/", "../data/input/", "../../data/input/"};
        for (const auto& d : dirs) {
            std::string path = d + name;
            SDL_Texture* t = IMG_LoadTexture(renderer, path.c_str());
            if (t) { std::cout << "Loaded layer: " << path << "\n"; return t; }
        }
        std::cerr << "WARNING: could not load " << name << "\n";
        return nullptr;
    };
    bgTexture  = tryLoad("background.png");
    envTexture = tryLoad("enviroment.png");
}

// Render environment view: background.png behind, enviroment.png on top, agents last.
void Env::renderEnv(SDL_Renderer* renderer, int x, int y, int width, int height) {
    envArea = {x, y, width, height};

    if (!texturesTried) loadTextures(renderer);

    // Void backdrop
    SDL_SetRenderDrawColor(renderer, 16, 16, 16, 255);
    SDL_RenderFillRect(renderer, &envArea);

    // Layer 1: background.png (navigation map / domains) — stretched to env area.
    if (bgTexture) {
        SDL_RenderCopy(renderer, bgTexture, nullptr, &envArea);
    } else {
        // Fallback: draw domains from the mask grid.
        static const int domainRGB[8][3] = {
            {204,193,173},{176,127,201},{122,138,160},{201,161,75},
            {91,143,201},{201,96,142},{108,192,168},{201,138,75}
        };
        const int gcols = grid.cols, grows = grid.rows;
        if (gcols > 0 && grows > 0) {
            float cw = static_cast<float>(width) / gcols;
            float ch = static_cast<float>(height) / grows;
            for (int gy = 0; gy < grows; ++gy)
                for (int gx = 0; gx < gcols; ++gx) {
                    int d = grid.domainAt({gx, gy});
                    if (d == WalkGrid::BLOCKED) continue;
                    const int* rgb = domainRGB[(d >= 0 && d <= 7) ? d : 0];
                    SDL_SetRenderDrawColor(renderer, rgb[0], rgb[1], rgb[2], 255);
                    SDL_Rect r = {x + static_cast<int>(gx*cw), y + static_cast<int>(gy*ch),
                                  static_cast<int>(cw+1), static_cast<int>(ch+1)};
                    SDL_RenderFillRect(renderer, &r);
                }
        }
    }

    // Layer 2: enviroment.png (detailed art) on top of the background.
    if (envTexture) {
        SDL_RenderCopy(renderer, envTexture, nullptr, &envArea);
    }

    // Border
    SDL_SetRenderDrawColor(renderer, 100, 100, 150, 255);
    SDL_RenderDrawRect(renderer, &envArea);

    // Layer 3: agents. Scale grid pixel space into the env area.
    float worldW = static_cast<float>(grid.cols * grid.cellSize);
    float worldH = static_cast<float>(grid.rows * grid.cellSize);
    if (worldW <= 0) worldW = 800.0f;
    if (worldH <= 0) worldH = 600.0f;
    float scaleX = static_cast<float>(width) / worldW;
    float scaleY = static_cast<float>(height) / worldH;

    for (const Agent* agent : agents) {
        glm::vec2 pos = agent->getPosition();
        int agentX = x + static_cast<int>(pos.x * scaleX);
        int agentY = y + static_cast<int>(pos.y * scaleY);
        int radius = 5;

        SDL_SetRenderDrawColor(renderer, 255, 60, 60, 255);
        for (int i = -radius; i <= radius; ++i)
            for (int j = -radius; j <= radius; ++j)
                if (i*i + j*j <= radius*radius)
                    SDL_RenderDrawPoint(renderer, agentX + i, agentY + j);
    }
}

// Clean up all agents
void Env::cleanup() {
    for (Agent* agent : agents) {
        delete agent;
    }
    agents.clear();
    activeAgents = 0;
    isRunning = false;

    if (messageLog) { delete messageLog; messageLog = nullptr; }
    if (bgTexture)  { SDL_DestroyTexture(bgTexture);  bgTexture = nullptr; }
    if (envTexture) { SDL_DestroyTexture(envTexture); envTexture = nullptr; }
}

// Add a new agent to the environment
void Env::addAgent(glm::vec2 position) {
    if (activeAgents < maxAgents) {
        Agent* newAgent = new Agent(activeAgents, position, this);
        agents.push_back(newAgent);
        activeAgents++;
    }
}

// Remove agent at given index
void Env::removeAgent(int index) {
    if (index >= 0 && index < agents.size()) {
        delete agents[index];
        agents.erase(agents.begin() + index);
        activeAgents--;
    }
}

// Update all agents
void Env::updateAgents() {
    for (Agent* agent : agents) {
        agent->step(deltaTime);
    }
}

// Determine which domain a world position is in
// Returns domain id of the activity that owns this position, or WalkGrid::CORRIDOR if in corridor
int Env::roomOf(glm::vec2 worldPos) const {
    glm::ivec2 gridPos = grid.toGrid(worldPos);
    if (!grid.inBounds(gridPos)) return WalkGrid::BLOCKED;
    return grid.domainAt(gridPos);
}

// Find center of a domain by averaging all positions of activities in that domain
glm::vec2 Env::domainCenter(int domain) const {
    float sumX = 0.0f, sumY = 0.0f;
    int count = 0;
    for (const auto& pair : activities) {
        if (pair.second.domain == domain) {
            for (const auto& pos : pair.second.positions) {
                sumX += pos.x;
                sumY += pos.y;
                count++;
            }
        }
    }
    if (count == 0) return glm::vec2(0, 0);
    return glm::vec2(sumX / count, sumY / count);
}

// Find an activity that belongs to the given domain
std::string Env::findActivityInDomain(int domain) const {
    for (const auto& name : activityNames) {
        const auto& act = activities.at(name);
        if (act.domain == domain) return name;
    }
    return ""; // No activity in domain
}


// Queue user input message
void Env::queueUserInput(const std::string& text, int agentId) {
    if (!messageLog || agentId < 0 || agentId >= agents.size()) return;

    Agent* agent = agents[agentId];
    messageLog->queueUserInput(
        "user",
        "Agent " + std::to_string(agentId),
        text,
        agent->getPosition(),
        "Domain " + std::to_string(agent->getTargetDomain()),
        agent->getActivity()
    );
}
