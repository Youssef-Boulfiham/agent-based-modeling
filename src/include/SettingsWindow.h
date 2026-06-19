#ifndef SETTINGSWINDOW_H
#define SETTINGSWINDOW_H

#include <SDL2/SDL.h>
#include <string>
#include <vector>

class Env;

// ──────────────────────────────────────────────────────────────────────────
// SettingsWindow — the in-game settings overlay.
//
// DESIGN (read this before adding panels):
//   * ONE class, MANY panels, ONE data source. There is never a "window on a
//     window". A single `Panel` enum holds the active panel; switching panels
//     REPLACES the current one (the old panel stops rendering). Pressing a
//     button on the MAIN panel kills MAIN and opens the target panel; each
//     sub-panel has a "< Back" button that returns to MAIN.
//   * `world` (Env*) is the SINGLE SOURCE OF TRUTH. Every panel reads/writes
//     the SAME Env / Agent objects the simulation, the dev view, and the
//     ChatBox use. No panel keeps its own copy of game state — it edits the
//     live objects directly, so all views stay in sync (one connected data
//     pipeline). When you add a panel, bind it to `world`, never to a snapshot.
// ──────────────────────────────────────────────────────────────────────────
class SettingsWindow {
public:
    // Each value is one full-screen-modal panel. Add new panels here, then
    // give them a button on MAIN and a render*/handle* method pair below.
    enum class Panel {
        NONE,      // closed
        MAIN,      // landing page: 10 category buttons
        AGENTS,    // agent settings (bound to Env's live agents)
        WORLD,     // world / environment settings
        SIM        // simulation settings
    };

private:
    Panel activePanel = Panel::NONE;
    Env*  world = nullptr;             // single source of truth (not owned)

    SDL_Rect windowRect;              // current panel frame (recomputed on render)
    SDL_Rect closeBtn;                // X button (top-right)
    SDL_Rect backBtn;                 // "< Back" button (sub-panels only)

    // MAIN panel: the 10 category buttons, recomputed each render for hit-test.
    static constexpr int NUM_MAIN_BUTTONS = 10;
    SDL_Rect mainButtons[NUM_MAIN_BUTTONS];
    const std::string mainLabels[NUM_MAIN_BUTTONS] = {
        "Agents",
        "World",
        "Simulation",
        "Pathfinding",
        "Activities",
        "Rendering",
        "Camera",
        "Chat / Messages",
        "Logging",
        "About"
    };

    // ── panel renderers ──
    void renderFrame(SDL_Renderer* r, int ww, int wh, int w, int h,
                     const std::string& title, bool withBack);
    void renderMain(SDL_Renderer* r, int ww, int wh);
    void renderAgents(SDL_Renderer* r, int ww, int wh);
    void renderWorld(SDL_Renderer* r, int ww, int wh);
    void renderSim(SDL_Renderer* r, int ww, int wh);

    // ── panel click handlers (return true if the click was consumed) ──
    bool handleMainClick(int x, int y);
    bool handleAgentsClick(int x, int y);
    bool handleWorldClick(int x, int y);
    bool handleSimClick(int x, int y);

    // Shared helpers.
    bool hit(const SDL_Rect& b, int x, int y) const;
    void drawButton(SDL_Renderer* r, const SDL_Rect& b, const std::string& label);

public:
    SettingsWindow();
    ~SettingsWindow();

    // Bind the single source of truth. Call once at setup.
    void setWorld(Env* w) { world = w; }

    void open()  { activePanel = Panel::MAIN; }   // top-bar Settings button
    void close() { activePanel = Panel::NONE; }
    bool getIsOpen() const { return activePanel != Panel::NONE; }

    void render(SDL_Renderer* renderer, int windowWidth, int windowHeight);

    // Returns true if the click landed inside the overlay (consumed). The
    // System checks this BEFORE forwarding to the env/chatbox, so an open panel
    // swallows clicks instead of leaking them to the simulation underneath.
    bool handleClick(int x, int y);
};

#endif
