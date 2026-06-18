#include "../include/Env.h"
#include <random>
#include <algorithm>
#include <iostream>

// Random number generation
static std::mt19937 rng(std::random_device{}());

// Constructor: Initialize environment with given dimensions
Env::Env(float w, float h, int maxAgents)
    : width(w), height(h), deltaTime(0.016f),
      frameCount(0), isRunning(false), maxAgents(maxAgents),
      activeAgents(0) {
    agents.reserve(maxAgents);
}

// Destructor: Clean up all agents
Env::~Env() {
    cleanup();
}

// Build the walkable map layer: a corridor grid with four activity zones
// carved into it. Each zone is a domain region plus a set of stand positions.
void Env::buildWorld() {
    const int cell = 10;
    int cols = static_cast<int>(width) / cell;
    int rows = static_cast<int>(height) / cell;
    grid = WalkGrid(cols, rows, cell, WalkGrid::CORRIDOR);

    // Activity domains (must be != CORRIDOR/BLOCKED).
    enum Domain { ALFA = 1, BRAVO = 2, CHARLIE = 3 };

    // Paint a rectangular zone [cx0,cx1) x [cy0,cy1) and collect stand points.
    auto paint = [&](int cx0, int cy0, int cx1, int cy1, int domain,
                     std::vector<glm::vec2>& out) {
        for (int y = cy0; y < cy1; ++y)
            for (int x = cx0; x < cx1; ++x) {
                glm::ivec2 c(x, y);
                grid.setDomain(c, domain);
                // sample every 2nd cell as a valid stand position
                if (x % 2 == 0 && y % 2 == 0)
                    out.push_back(grid.toWorld(c));
            }
    };

    std::vector<glm::vec2> idle, working, learning;
    int mx = cols / 2, my = rows / 2;     // map midpoints
    paint(0,      0,      mx - 2, my - 2, ALFA,    idle);     // top-left
    paint(mx + 2, 0,      cols,   my - 2, BRAVO,   working);  // top-right
    paint(0,      my + 2, mx - 2, rows,   CHARLIE, learning); // bottom-left
    // the 2-cell gap between zones stays CORRIDOR -> shared, always walkable

    addActivity("idle",     ALFA,    0.30f, std::move(idle));
    addActivity("working",  BRAVO,   0.35f, std::move(working));
    addActivity("learning", CHARLIE, 0.35f, std::move(learning));
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

    updateAgents();
    activeAgents = agents.size();
    frameCount++;
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

// Render environment view with agents
void Env::renderEnv(SDL_Renderer* renderer, int x, int y, int width, int height) {
    envArea = {x, y, width, height};

    SDL_SetRenderDrawColor(renderer, 40, 40, 50, 255);
    SDL_RenderFillRect(renderer, &envArea);
    SDL_SetRenderDrawColor(renderer, 100, 100, 150, 255);
    SDL_RenderDrawRect(renderer, &envArea);

    float scaleX = static_cast<float>(width) / 800.0f;
    float scaleY = static_cast<float>(height) / 600.0f;

    for (const Agent* agent : agents) {
        glm::vec2 pos = agent->getPosition();
        int agentX = x + static_cast<int>(pos.x * scaleX);
        int agentY = y + static_cast<int>(pos.y * scaleY);
        int radius = 6;

        SDL_SetRenderDrawColor(renderer, 100, 200, 255, 255);
        for (int i = -radius; i <= radius; ++i) {
            for (int j = -radius; j <= radius; ++j) {
                if (i*i + j*j <= radius*radius) {
                    SDL_RenderDrawPoint(renderer, agentX + i, agentY + j);
                }
            }
        }
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

