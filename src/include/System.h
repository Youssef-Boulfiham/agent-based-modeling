#pragma once

#include <SDL2/SDL.h>

class Env;
class UIButtons;
class Statistics;
class ChatBox;
class SettingsWindow;

class System {
public:
    System();
    ~System();

    bool initialize();
    void run();
    void shutdown();

private:
    // Window size — set at runtime from the display (fullscreen-desktop).
    int WINDOW_WIDTH = 1280;
    int WINDOW_HEIGHT = 800;
    // Constants
    static constexpr float WORLD_WIDTH = 800.0f;
    static constexpr float WORLD_HEIGHT = 600.0f;
    static constexpr int MAX_AGENTS = 1;
    // Sim tick interval in real seconds = THE agent-speed knob.
    // Bigger value -> fewer ticks/sec -> slower agents. Smaller -> faster.
    static constexpr float FRAME_TIME = 1.0f / 12.0f;
    static constexpr int MARGIN = 20;
    static constexpr int BUTTON_BAR_HEIGHT = 70;
    static constexpr int RIGHT_PANEL_WIDTH = 300;
    static constexpr int TEXT_PANEL_HEIGHT = 150;

    // SDL
    SDL_Window* window;
    SDL_Renderer* renderer;

    // Simulation
    Env* simulation;

    // UI
    UIButtons* uiButtons;
    Statistics* uiStats;
    ChatBox* uiText;
    SettingsWindow* settingsWindow;

    // Env-window camera drag state (click-and-drag to pan).
    bool panningEnv = false;
    int  lastMouseX = 0, lastMouseY = 0;

    // Helpers
    bool initSDL();
    void initializeSimulation();
    void render();
    void handleEvents(bool& running);
};
