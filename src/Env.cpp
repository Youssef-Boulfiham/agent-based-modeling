#include "../include/Env.h"
#include <SDL2/SDL_image.h>
#include <random>
#include <algorithm>
#include <iostream>
#include <cctype>

// Random number generation
static std::mt19937 rng(std::random_device{}());

// Domain palette: RGB color on background.png -> domain id + name.
// id 0 = CORRIDOR (the hallway). ids 1..7 = activity domains, named by color.
// Anything not near one of these (white, dark void, anti-aliased edges) -> BLOCKED.
struct DomainColor { int id; const char* name; int r, g, b; };
static const DomainColor DOMAIN_PALETTE[] = {
    {0, "corridor", 204, 193, 173},
    {1, "purple",   176, 127, 201},
    {2, "slate",    122, 138, 160},
    {3, "gold",     201, 161,  75},
    {4, "blue",      91, 143, 201},
    {5, "pink",     201,  96, 142},
    {6, "teal",     108, 192, 168},
    {7, "orange",   201, 138,  75},
};
// Squared-distance threshold for accepting a pixel as a palette color.
// White (255,255,255) is ~13000 from corridor -> rejected (BLOCKED), as wanted.
static const int COLOR_MATCH_MAX_SQ = 60 * 60;

// Name lookup for a domain id (used when registering activities).
static const char* domainName(int id) {
    for (const auto& d : DOMAIN_PALETTE)
        if (d.id == id) return d.name;
    return "domain";
}

// Classify one RGB pixel into a domain id (0..7) or WalkGrid::BLOCKED.
static int classifyColor(int r, int g, int b) {
    int best = WalkGrid::BLOCKED, bestSq = COLOR_MATCH_MAX_SQ + 1;
    for (const auto& d : DOMAIN_PALETTE) {
        int dr = r - d.r, dg = g - d.g, db = b - d.b;
        int sq = dr*dr + dg*dg + db*db;
        if (sq < bestSq) { bestSq = sq; best = d.id; }
    }
    return bestSq <= COLOR_MATCH_MAX_SQ ? best : WalkGrid::BLOCKED;
}

// Build a domain mask live from background.png by sampling each grid cell's
// center pixel and nearest-color matching to the palette. Replaces the stale
// pre-baked CSV so the walkable grid always matches what the user sees.
// targetCols sets grid density; rows derived from image aspect.
static bool loadMaskFromImage(const std::string& path, int targetCols,
                              int& cols, int& rows, std::vector<int>& flat) {
    SDL_Surface* raw = IMG_Load(path.c_str());
    if (!raw) return false;
    SDL_Surface* s = SDL_ConvertSurfaceFormat(raw, SDL_PIXELFORMAT_RGBA32, 0);
    SDL_FreeSurface(raw);
    if (!s) return false;

    const int W = s->w, H = s->h;
    int cell = std::max(1, W / targetCols);
    cols = W / cell;
    rows = H / cell;

    // 1) Full-resolution classification of every pixel.
    std::vector<int>   cls(static_cast<size_t>(W) * H, WalkGrid::BLOCKED);
    std::vector<Uint8> corr(static_cast<size_t>(W) * H, 0); // corridor pixel?
    SDL_LockSurface(s);
    const Uint8* pixels = static_cast<const Uint8*>(s->pixels);
    for (int y = 0; y < H; ++y) {
        const Uint8* row = pixels + y * s->pitch;
        for (int x = 0; x < W; ++x) {
            const Uint8* p = row + x * 4; // RGBA32
            int d = classifyColor(p[0], p[1], p[2]);
            cls[static_cast<size_t>(y) * W + x]  = d;
            corr[static_cast<size_t>(y) * W + x] = (d == WalkGrid::CORRIDOR) ? 1 : 0;
        }
    }
    SDL_UnlockSurface(s);
    SDL_FreeSurface(s);

    // 2) Dilate the corridor mask by CORRIDOR_DILATE px (separable max filter).
    //    The hallways are thin tan lines; without thickening they fragment when
    //    downsampled to the grid and domains become unreachable. R chosen so the
    //    two connectors per domain survive gridding (validated: all 8 reachable).
    //    Scaled with background.png SCALE (16->48): R=18 keeps the dilation reach
    //    at 0.375 logical cells, so the extracted grid is byte-identical to the
    //    old 16px/R=6 render (verified: 0 differing cells).
    const int R = 18;
    auto dilate = [&](std::vector<Uint8>& src) {
        std::vector<Uint8> tmp(src.size(), 0);
        for (int y = 0; y < H; ++y)                 // horizontal pass
            for (int x = 0; x < W; ++x) {
                Uint8 v = 0;
                for (int k = -R; k <= R && !v; ++k) {
                    int xx = x + k;
                    if (xx >= 0 && xx < W) v = src[static_cast<size_t>(y) * W + xx];
                }
                tmp[static_cast<size_t>(y) * W + x] = v;
            }
        for (int y = 0; y < H; ++y)                 // vertical pass
            for (int x = 0; x < W; ++x) {
                Uint8 v = 0;
                for (int k = -R; k <= R && !v; ++k) {
                    int yy = y + k;
                    if (yy >= 0 && yy < H) v = tmp[static_cast<size_t>(yy) * W + x];
                }
                src[static_cast<size_t>(y) * W + x] = v;
            }
    };
    dilate(corr);

    // 3) Downsample to the grid. A cell is CORRIDOR if it holds any (dilated)
    //    corridor pixel; otherwise the majority domain color in the block;
    //    otherwise BLOCKED. Corridor priority keeps the hallway connected.
    flat.assign(static_cast<size_t>(cols) * rows, WalkGrid::BLOCKED);
    for (int gy = 0; gy < rows; ++gy) {
        for (int gx = 0; gx < cols; ++gx) {
            int ys = gy * cell, ye = std::min(H, ys + cell);
            int xs = gx * cell, xe = std::min(W, xs + cell);
            bool hasCorr = false;
            int count[8] = {0,0,0,0,0,0,0,0};
            for (int y = ys; y < ye && !hasCorr; ++y)
                for (int x = xs; x < xe; ++x) {
                    size_t i = static_cast<size_t>(y) * W + x;
                    if (corr[i]) { hasCorr = true; break; }
                    int d = cls[i];
                    if (d >= 1 && d <= 7) ++count[d];
                }
            int val = WalkGrid::BLOCKED;
            if (hasCorr) {
                val = WalkGrid::CORRIDOR;
            } else {
                int bestD = 0, bestN = 0;
                for (int d = 1; d <= 7; ++d)
                    if (count[d] > bestN) { bestN = count[d]; bestD = d; }
                if (bestN > 0) val = bestD;
            }
            flat[gy * cols + gx] = val;
        }
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
    // Extract the walkable grid live from map/background.png so the grid always
    // matches the map the user sees. Try a few relative paths so it works
    // regardless of launch cwd.
    int cols = 0, rows = 0;
    std::vector<int> flat;
    bool loaded = false;

    const char* imgPaths[] = {
        "map/background.png",
        "../map/background.png",
        "../../map/background.png",
    };
    for (const char* p : imgPaths) {
        if (loadMaskFromImage(p, /*targetCols=*/176, cols, rows, flat)) {
            std::cout << "Extracted mask from image: " << p
                      << " (" << cols << "x" << rows << ")\n";
            loaded = true;
            break;
        }
    }

    if (!loaded) {
        std::cerr << "WARNING: map/background.png not found; falling back to empty corridor grid\n";
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
        std::string name = domainName(d);
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

    // Periodic agent-to-agent chatter (every ~20 frames).
    // The agents in the textbox ARE the agents in the environment: the message
    // text is derived from the speaker's REAL state, never made up. The sender
    // reports what it is actually doing this step.
    if (frameCount % 20 == 0 && agents.size() >= 2) {
        std::uniform_int_distribution<int> pick(0, static_cast<int>(agents.size()) - 1);
        int a_idx = pick(rng);
        int b_idx = pick(rng);
        if (a_idx != b_idx && messageLog) {
            Agent* agentA = agents[a_idx];
            Agent* agentB = agents[b_idx];

            int       id     = agentA->getId();
            glm::vec2 pos    = agentA->getPosition();
            int       room   = roomOf(pos);                // domain it is actually IN
            int       target = agentA->getTargetDomain();  // domain it MUST be in
            std::string act  = agentA->getActivity();      // real activity this step

            // A genuine switch: the agent now stands in a domain different from
            // the one it was in the last time it spoke. Read straight from state.
            bool inDomain = room > WalkGrid::CORRIDOR;
            auto prev = lastReportedDomain.find(id);
            bool justSwitched = inDomain && prev != lastReportedDomain.end() &&
                                prev->second != room;

            std::string msg;
            if (justSwitched) {
                msg = std::string("Switched to ") + domainName(room);
            } else if (act == "move to domain") {
                msg = std::string("Moving to ") + domainName(target);
            } else if (act == "working") {
                msg = std::string("Working in ") + domainName(room);
            } else if (act == "idle") {
                msg = std::string("Idle in ") + domainName(room);
            } else if (inDomain) {
                msg = std::string("In ") + domainName(room);
            } else {
                msg = "At (" + std::to_string(static_cast<int>(pos.x)) + "," +
                              std::to_string(static_cast<int>(pos.y)) + ")";
            }

            // Remember the domain this agent just reported (only when in one),
            // so the next switch can be detected against it.
            if (inDomain) lastReportedDomain[id] = room;

            std::string domainStr = inDomain
                ? std::string(domainName(room))
                : "the corridor";

            messageLog->queueAgentChatter(
                "Agent " + std::to_string(id),
                "Agent " + std::to_string(agentB->getId()),
                msg,
                pos,
                domainStr,            // state snapshot: where it ACTUALLY is
                act
            );
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

    // ── GATHER: force every agent to the canteen until all have arrived ──────
    if (phase == Phase::GATHER) {
        bool allArrived = true;
        for (Agent* agent : agents) {
            if (agent->getTargetDomain() != canteenDomain)
                agent->setTargetDomain(canteenDomain);
            if (roomOf(agent->getPosition()) != canteenDomain)
                allArrived = false;
        }
        if (allArrived && !agents.empty()) phase = Phase::SCATTER;
        return; // suppress random reassignment while gathering
    }

    // ── SCATTER: everyone picks one random domain (≠ canteen) once ───────────
    if (phase == Phase::SCATTER) {
        for (Agent* agent : agents) {
            int next;
            do { next = domainList[pick(rng)]; }
            while (next == canteenDomain && domainList.size() > 1);
            agent->setTargetDomain(next);
            nextChangeAt[agent->getId()] = frameCount + 50 + jitter(rng);
        }
        phase = Phase::NORMAL; // afterwards agents keep their scattered domain
        return;
    }

    // NORMAL: no random reassignment. Each agent stays in the domain it was
    // last given (initial domain, or the one picked during the last SCATTER) and
    // only ever switches via the sanity-check gather -> scatter choreography.
}

// Domain whose center is nearest the middle of the map (the "canteen").
int Env::canteenMiddleDomain() const {
    glm::vec2 mid(width * 0.5f, height * 0.5f);
    int best = domainList.empty() ? WalkGrid::CORRIDOR : domainList.front();
    float bestDist = 1e30f;
    for (int d : domainList) {
        glm::vec2 c = domainCenter(d);
        float dx = c.x - mid.x, dy = c.y - mid.y;
        float dist = dx * dx + dy * dy;
        if (dist < bestDist) { bestDist = dist; best = d; }
    }
    return best;
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
        const std::string dirs[] = {"map/", "../map/", "../../map/"};
        for (const auto& d : dirs) {
            std::string path = d + name;
            SDL_Texture* t = IMG_LoadTexture(renderer, path.c_str());
            if (t) { std::cout << "Loaded layer: " << path << "\n"; return t; }
        }
        std::cerr << "WARNING: could not load " << name << "\n";
        return nullptr;
    };
    bgTexture       = tryLoad("background.png");
    envTexture      = tryLoad("enviroment.png");
    overviewTexture = tryLoad("map_overview.png");  // generated; may be absent
}

// Render environment view: background.png behind, enviroment.png on top, agents last.
void Env::clampView() {
    const float frac = 1.0f / static_cast<float>(zoomLevel + 1);
    const float maxOrigin = 1.0f - frac;
    if (viewX < 0.0f) viewX = 0.0f;  if (viewX > maxOrigin) viewX = maxOrigin;
    if (viewY < 0.0f) viewY = 0.0f;  if (viewY > maxOrigin) viewY = maxOrigin;
}

void Env::zoomAt(int dir, int mouseX, int mouseY) {
    int newLevel = zoomLevel + (dir > 0 ? 1 : -1);
    if (newLevel < 0) newLevel = 0;
    if (newLevel > MAX_ZOOM_LEVEL) newLevel = MAX_ZOOM_LEVEL;
    if (newLevel == zoomLevel) return;

    if (envArea.w <= 0 || envArea.h <= 0) { zoomLevel = newLevel; clampView(); return; }

    // Keep the map point under the cursor fixed on screen across the zoom.
    float sx = (mouseX - envArea.x) / static_cast<float>(envArea.w); // 0..1 in view
    float sy = (mouseY - envArea.y) / static_cast<float>(envArea.h);
    if (sx < 0.0f) sx = 0.0f;  if (sx > 1.0f) sx = 1.0f;
    if (sy < 0.0f) sy = 0.0f;  if (sy > 1.0f) sy = 1.0f;

    const float fracOld = 1.0f / static_cast<float>(zoomLevel + 1);
    const float u = viewX + sx * fracOld;   // map-normalized point under cursor
    const float v = viewY + sy * fracOld;

    zoomLevel = newLevel;
    const float fracNew = 1.0f / static_cast<float>(zoomLevel + 1);
    viewX = u - sx * fracNew;
    viewY = v - sy * fracNew;
    clampView();
}

void Env::panByPixels(int dx, int dy) {
    if (envArea.w <= 0 || envArea.h <= 0) return;
    const float frac = 1.0f / static_cast<float>(zoomLevel + 1);
    // Grab the map: dragging right reveals content to the left.
    viewX -= (dx / static_cast<float>(envArea.w)) * frac;
    viewY -= (dy / static_cast<float>(envArea.h)) * frac;
    clampView();
}

void Env::renderEnv(SDL_Renderer* renderer, int x, int y, int width, int height) {
    envArea = {x, y, width, height};

    if (!texturesTried) loadTextures(renderer);

    // Camera: the visible window is the sub-rectangle [viewX, viewX+frac] of the
    // map, scaled up to fill the env area. The SAME transform drives textures
    // (via a source sub-rect) and agents (via world->screen), so they stay
    // calibrated at every zoom level.
    const float frac = 1.0f / static_cast<float>(zoomLevel + 1);

    // Source sub-rect of a full-map texture for the current view.
    auto texSrc = [&](SDL_Texture* t) -> SDL_Rect {
        int tw = 0, th = 0;
        SDL_QueryTexture(t, nullptr, nullptr, &tw, &th);
        return { static_cast<int>(viewX * tw), static_cast<int>(viewY * th),
                 static_cast<int>(tw * frac), static_cast<int>(th * frac) };
    };

    // Void backdrop
    SDL_SetRenderDrawColor(renderer, 16, 16, 16, 255);
    SDL_RenderFillRect(renderer, &envArea);

    // Overview view: draw the annotated overview alone (no bg/env/agents-art mix).
    bool overviewView = (layerView == 2 && overviewTexture);

    // Layer 1: background.png (navigation map / domains) — visible window only.
    if (overviewView) {
        SDL_Rect src = texSrc(overviewTexture);
        SDL_RenderCopy(renderer, overviewTexture, &src, &envArea);
    } else if (bgTexture) {
        SDL_Rect src = texSrc(bgTexture);
        SDL_RenderCopy(renderer, bgTexture, &src, &envArea);
    }

    // Layer 2: enviroment.png (detailed art) on top of the background.
    // Only in enviroment view (0); background (1) and overview (2) skip it.
    if (layerView == 0 && envTexture) {
        SDL_Rect src = texSrc(envTexture);
        SDL_RenderCopy(renderer, envTexture, &src, &envArea);
    }

    // Layer 3: agents. Map grid pixel space -> normalized -> visible window.
    float worldW = static_cast<float>(grid.cols * grid.cellSize);
    float worldH = static_cast<float>(grid.rows * grid.cellSize);
    if (worldW <= 0) worldW = 800.0f;
    if (worldH <= 0) worldH = 600.0f;
    auto toScreenX = [&](float wx) {
        return x + static_cast<int>(((wx / worldW - viewX) / frac) * width);
    };
    auto toScreenY = [&](float wy) {
        return y + static_cast<int>(((wy / worldH - viewY) / frac) * height);
    };

    // Fallback: no bg texture -> draw domains from the mask grid (transformed).
    if (!overviewView && !bgTexture) {
        static const int domainRGB[8][3] = {
            {204,193,173},{176,127,201},{122,138,160},{201,161,75},
            {91,143,201},{201,96,142},{108,192,168},{201,138,75}
        };
        const int gcols = grid.cols, grows = grid.rows;
        if (gcols > 0 && grows > 0) {
            SDL_RenderSetClipRect(renderer, &envArea);
            for (int gy = 0; gy < grows; ++gy)
                for (int gx = 0; gx < gcols; ++gx) {
                    int d = grid.domainAt({gx, gy});
                    if (d == WalkGrid::BLOCKED) continue;
                    const int* rgb = domainRGB[(d >= 0 && d <= 7) ? d : 0];
                    SDL_SetRenderDrawColor(renderer, rgb[0], rgb[1], rgb[2], 255);
                    int sx0 = toScreenX(static_cast<float>(gx * grid.cellSize));
                    int sy0 = toScreenY(static_cast<float>(gy * grid.cellSize));
                    int sx1 = toScreenX(static_cast<float>((gx + 1) * grid.cellSize));
                    int sy1 = toScreenY(static_cast<float>((gy + 1) * grid.cellSize));
                    SDL_Rect r = {sx0, sy0, sx1 - sx0 + 1, sy1 - sy0 + 1};
                    SDL_RenderFillRect(renderer, &r);
                }
            SDL_RenderSetClipRect(renderer, nullptr);
        }
    }

    // Clip agents + paths to the env window so panned-out points don't bleed
    // onto the surrounding panels.
    SDL_RenderSetClipRect(renderer, &envArea);

    // Path overlay: draw each agent's planned route (current pos -> waypoints).
    if (showPaths) {
        SDL_SetRenderDrawColor(renderer, 255, 230, 0, 255); // yellow path lines
        for (const Agent* agent : agents) {
            const std::vector<glm::vec2>& path = agent->getPath();
            if (path.empty()) continue;
            glm::vec2 prev = agent->getPosition();
            for (const glm::vec2& wp : path) {
                SDL_RenderDrawLine(renderer,
                    toScreenX(prev.x), toScreenY(prev.y),
                    toScreenX(wp.x),   toScreenY(wp.y));
                prev = wp;
            }
        }
    }

    for (const Agent* agent : agents) {
        glm::vec2 pos = agent->getPosition();
        int agentX = toScreenX(pos.x);
        int agentY = toScreenY(pos.y);
        int radius = 5;

        SDL_SetRenderDrawColor(renderer, 255, 60, 60, 255);
        for (int i = -radius; i <= radius; ++i)
            for (int j = -radius; j <= radius; ++j)
                if (i*i + j*j <= radius*radius)
                    SDL_RenderDrawPoint(renderer, agentX + i, agentY + j);
    }

    SDL_RenderSetClipRect(renderer, nullptr);

    // Border (drawn after clip reset so it always frames the full env area).
    SDL_SetRenderDrawColor(renderer, 100, 100, 150, 255);
    SDL_RenderDrawRect(renderer, &envArea);
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
    if (overviewTexture) { SDL_DestroyTexture(overviewTexture); overviewTexture = nullptr; }
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
    // Command detection: "sanity check" (case-insensitive) triggers the gather-
    // then-scatter choreography. All agents walk to the canteen; once every one
    // has arrived they each pick a random domain and disperse.
    std::string lower = text;
    for (char& c : lower) c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    if (lower.find("sanity check") != std::string::npos) {
        canteenDomain = 4;            // blue domain — fixed gather point
        phase = Phase::GATHER;
        nextChangeAt.clear();
        // Force every agent to retarget the canteen immediately.
        for (Agent* agent : agents) agent->setTargetDomain(canteenDomain);
    }

    if (!messageLog || agentId < 0 || agentId >= static_cast<int>(agents.size())) return;

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
