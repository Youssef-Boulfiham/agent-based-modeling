#pragma once

#include <SDL2/SDL.h>

class Env;
class UIButtons;
class Statistics;
class ChatBox;

class System {
public:
    System();
    ~System();

    bool initialize();
    void run();
    void shutdown();

private:
    // Constants
    static constexpr int WINDOW_WIDTH = 1280;
    static constexpr int WINDOW_HEIGHT = 800;
    static constexpr float WORLD_WIDTH = 800.0f;
    static constexpr float WORLD_HEIGHT = 600.0f;
    static constexpr int MAX_AGENTS = 10;
    static constexpr float FRAME_TIME = 1.0f / 60.0f;
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

    // Helpers
    bool initSDL();
    void initializeSimulation();
    void render();
    void handleEvents(bool& running);
};
