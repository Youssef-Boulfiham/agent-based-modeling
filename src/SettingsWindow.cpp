#include "../include/SettingsWindow.h"
#include "../include/Env.h"
#include "../include/Agent.h"
#include "../include/TextRenderer.h"
#include <string>

// ── layout constants ──
namespace {
    constexpr int FRAME_W      = 420;
    constexpr int FRAME_H      = 520;
    constexpr int PAD          = 20;
    constexpr int TITLE_H      = 40;
    constexpr int ROW_H        = 34;   // main button height
    constexpr int ROW_GAP      = 6;
    constexpr int CLOSE_SZ     = 16;
    constexpr int BACK_W       = 70;
    constexpr int BACK_H       = 24;
}

SettingsWindow::SettingsWindow() {
    windowRect = {0, 0, FRAME_W, FRAME_H};
    closeBtn   = {0, 0, CLOSE_SZ, CLOSE_SZ};
    backBtn    = {0, 0, BACK_W, BACK_H};
}

SettingsWindow::~SettingsWindow() {}

bool SettingsWindow::hit(const SDL_Rect& b, int x, int y) const {
    return x >= b.x && x < b.x + b.w && y >= b.y && y < b.y + b.h;
}

void SettingsWindow::drawButton(SDL_Renderer* r, const SDL_Rect& b,
                                const std::string& label) {
    SDL_SetRenderDrawColor(r, 60, 60, 80, 255);
    SDL_RenderFillRect(r, &b);
    SDL_SetRenderDrawColor(r, 150, 150, 200, 255);
    SDL_RenderDrawRect(r, &b);
    if (!label.empty()) {
        int tw = TextRenderer::width(label, 1);
        int tx = b.x + 10;                        // left-aligned with small inset
        if (b.w < tw + 20) tx = b.x + (b.w - tw) / 2;   // center if tight
        int ty = b.y + (b.h - TextRenderer::LINE_H) / 2;
        TextRenderer::draw(r, tx, ty, label, 1, 210, 210, 230);
    }
}

// Draw the modal frame (dim backdrop + panel box + title + close + optional
// back), and store closeBtn/backBtn/windowRect for hit-testing. Panels call
// this first, then fill their body.
void SettingsWindow::renderFrame(SDL_Renderer* r, int ww, int wh, int w, int h,
                                 const std::string& title, bool withBack) {
    // Dim the whole screen behind the modal.
    SDL_SetRenderDrawBlendMode(r, SDL_BLENDMODE_BLEND);
    SDL_SetRenderDrawColor(r, 0, 0, 0, 140);
    SDL_Rect full = {0, 0, ww, wh};
    SDL_RenderFillRect(r, &full);

    windowRect = {(ww - w) / 2, (wh - h) / 2, w, h};

    // Panel body.
    SDL_SetRenderDrawColor(r, 35, 35, 45, 255);
    SDL_RenderFillRect(r, &windowRect);
    SDL_SetRenderDrawColor(r, 150, 150, 200, 255);
    SDL_RenderDrawRect(r, &windowRect);

    // Title bar separator.
    int titleY = windowRect.y + TITLE_H;
    SDL_RenderDrawLine(r, windowRect.x, titleY, windowRect.x + w, titleY);

    // Title text.
    TextRenderer::draw(r, windowRect.x + PAD,
                       windowRect.y + (TITLE_H - TextRenderer::LINE_H) / 2,
                       title, 1, 230, 230, 255);

    // Close button (X) top-right.
    closeBtn = {windowRect.x + w - CLOSE_SZ - 12,
                windowRect.y + (TITLE_H - CLOSE_SZ) / 2, CLOSE_SZ, CLOSE_SZ};
    SDL_SetRenderDrawColor(r, 180, 120, 120, 255);
    SDL_RenderDrawRect(r, &closeBtn);
    SDL_RenderDrawLine(r, closeBtn.x, closeBtn.y,
                       closeBtn.x + CLOSE_SZ, closeBtn.y + CLOSE_SZ);
    SDL_RenderDrawLine(r, closeBtn.x + CLOSE_SZ, closeBtn.y,
                       closeBtn.x, closeBtn.y + CLOSE_SZ);

    // Back button (sub-panels only), bottom-left.
    if (withBack) {
        backBtn = {windowRect.x + PAD, windowRect.y + h - BACK_H - PAD,
                   BACK_W, BACK_H};
        drawButton(r, backBtn, "< Back");
    } else {
        backBtn = {0, 0, 0, 0};
    }
}

// ── MAIN: 10 category buttons ──
void SettingsWindow::renderMain(SDL_Renderer* r, int ww, int wh) {
    renderFrame(r, ww, wh, FRAME_W, FRAME_H, "Settings", false);

    int x = windowRect.x + PAD;
    int y = windowRect.y + TITLE_H + PAD;
    int w = FRAME_W - PAD * 2;

    for (int i = 0; i < NUM_MAIN_BUTTONS; ++i) {
        mainButtons[i] = {x, y + i * (ROW_H + ROW_GAP), w, ROW_H};
        drawButton(r, mainButtons[i], mainLabels[i]);
    }
}

// ── AGENTS: live agent data from Env (single source of truth) ──
void SettingsWindow::renderAgents(SDL_Renderer* r, int ww, int wh) {
    renderFrame(r, ww, wh, FRAME_W, FRAME_H, "Agent Settings", true);

    int x = windowRect.x + PAD;
    int y = windowRect.y + TITLE_H + PAD;

    if (!world) {
        TextRenderer::draw(r, x, y, "No world bound.", 1, 220, 160, 160);
        return;
    }

    std::string header = "Active agents: " + std::to_string(world->getActiveAgents());
    TextRenderer::draw(r, x, y, header, 1, 230, 230, 255);
    y += TextRenderer::LINE_H + 12;

    // One block per live agent. Reads the SAME Agent objects everything else uses.
    for (Agent* a : world->getAgents()) {
        if (!a) continue;
        glm::vec2 p = a->getPosition();
        std::string line1 = "Agent #" + std::to_string(a->getId());
        std::string line2 = "  pos: (" + std::to_string((int)p.x) + ", "
                          + std::to_string((int)p.y) + ")";
        std::string dname = world->findActivityInDomain(a->getTargetDomain());
        if (dname.empty()) dname = std::to_string(a->getTargetDomain());
        std::string line3 = "  domain: " + dname
                          + "   activity: " + a->getActivity();
        std::string line4 = "  status: " + a->getStatus();

        TextRenderer::draw(r, x, y, line1, 1, 200, 220, 255); y += TextRenderer::LINE_H + 2;
        TextRenderer::draw(r, x, y, line2, 1, 180, 180, 200); y += TextRenderer::LINE_H + 2;
        TextRenderer::draw(r, x, y, line3, 1, 180, 180, 200); y += TextRenderer::LINE_H + 2;
        TextRenderer::draw(r, x, y, line4, 1, 180, 180, 200); y += TextRenderer::LINE_H + 10;
    }
}

// ── WORLD: environment data ──
void SettingsWindow::renderWorld(SDL_Renderer* r, int ww, int wh) {
    renderFrame(r, ww, wh, FRAME_W, FRAME_H, "World Settings", true);

    int x = windowRect.x + PAD;
    int y = windowRect.y + TITLE_H + PAD;

    if (!world) {
        TextRenderer::draw(r, x, y, "No world bound.", 1, 220, 160, 160);
        return;
    }

    std::string w1 = "Width:  " + std::to_string((int)world->getWidth());
    std::string w2 = "Height: " + std::to_string((int)world->getHeight());
    std::string w3 = "Activities: "
                   + std::to_string((int)world->getActivityNames().size());
    TextRenderer::draw(r, x, y, w1, 1, 200, 220, 255); y += TextRenderer::LINE_H + 6;
    TextRenderer::draw(r, x, y, w2, 1, 200, 220, 255); y += TextRenderer::LINE_H + 6;
    TextRenderer::draw(r, x, y, w3, 1, 200, 220, 255); y += TextRenderer::LINE_H + 12;

    for (const std::string& name : world->getActivityNames()) {
        TextRenderer::draw(r, x, y, "  - " + name, 1, 180, 180, 200);
        y += TextRenderer::LINE_H + 2;
    }
}

// ── SIM: simulation runtime data ──
void SettingsWindow::renderSim(SDL_Renderer* r, int ww, int wh) {
    renderFrame(r, ww, wh, FRAME_W, FRAME_H, "Simulation Settings", true);

    int x = windowRect.x + PAD;
    int y = windowRect.y + TITLE_H + PAD;

    if (!world) {
        TextRenderer::draw(r, x, y, "No world bound.", 1, 220, 160, 160);
        return;
    }

    std::string s1 = "Frame: " + std::to_string(world->getFrameCount());
    std::string s2 = std::string("Running: ") + (world->getIsRunning() ? "yes" : "no");
    TextRenderer::draw(r, x, y, s1, 1, 200, 220, 255); y += TextRenderer::LINE_H + 6;
    TextRenderer::draw(r, x, y, s2, 1, 200, 220, 255);
}

// ── dispatch render ──
void SettingsWindow::render(SDL_Renderer* renderer, int ww, int wh) {
    switch (activePanel) {
        case Panel::NONE:   return;
        case Panel::MAIN:   renderMain(renderer, ww, wh);   break;
        case Panel::AGENTS: renderAgents(renderer, ww, wh); break;
        case Panel::WORLD:  renderWorld(renderer, ww, wh);  break;
        case Panel::SIM:    renderSim(renderer, ww, wh);    break;
    }
}

// ── MAIN click: open a sub-panel (kills MAIN) ──
bool SettingsWindow::handleMainClick(int x, int y) {
    for (int i = 0; i < NUM_MAIN_BUTTONS; ++i) {
        if (hit(mainButtons[i], x, y)) {
            switch (i) {
                case 0: activePanel = Panel::AGENTS; break;
                case 1: activePanel = Panel::WORLD;  break;
                case 2: activePanel = Panel::SIM;    break;
                // 3..9 not wired yet — stay on MAIN.
                default: break;
            }
            return true;
        }
    }
    return false;
}

bool SettingsWindow::handleAgentsClick(int x, int y) {
    if (hit(backBtn, x, y)) { activePanel = Panel::MAIN; return true; }
    return false;
}

bool SettingsWindow::handleWorldClick(int x, int y) {
    if (hit(backBtn, x, y)) { activePanel = Panel::MAIN; return true; }
    return false;
}

bool SettingsWindow::handleSimClick(int x, int y) {
    if (hit(backBtn, x, y)) { activePanel = Panel::MAIN; return true; }
    return false;
}

// ── dispatch click ──
bool SettingsWindow::handleClick(int x, int y) {
    if (activePanel == Panel::NONE) return false;

    // Close button is on every panel.
    if (hit(closeBtn, x, y)) { close(); return true; }

    bool consumed = false;
    switch (activePanel) {
        case Panel::MAIN:   consumed = handleMainClick(x, y);   break;
        case Panel::AGENTS: consumed = handleAgentsClick(x, y); break;
        case Panel::WORLD:  consumed = handleWorldClick(x, y);  break;
        case Panel::SIM:    consumed = handleSimClick(x, y);    break;
        default: break;
    }
    if (consumed) return true;

    // Click anywhere inside the panel frame is swallowed (don't leak to sim).
    // Click on the dimmed backdrop closes the overlay.
    if (hit(windowRect, x, y)) return true;
    close();
    return true;
}
