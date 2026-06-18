#include "../include/System.h"
#include "../include/Env.h"
#include "../include/Buttons.h"
#include "../include/Statistics.h"
#include "../include/ChatBox.h"
#include <iostream>
#include <thread>
#include <chrono>

System::System()
    : window(nullptr), renderer(nullptr), simulation(nullptr),
      uiButtons(nullptr), uiStats(nullptr), uiText(nullptr) {
}

System::~System() {
    shutdown();
}

bool System::initialize() {
    if (!initSDL()) {
        return false;
    }
    initializeSimulation();
    return true;
}

bool System::initSDL() {
    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        std::cerr << "SDL init failed\n";
        return false;
    }

    window = SDL_CreateWindow(
        "Agent-Based Modeling",
        SDL_WINDOWPOS_CENTERED,
        SDL_WINDOWPOS_CENTERED,
        WINDOW_WIDTH,
        WINDOW_HEIGHT,
        SDL_WINDOW_SHOWN
    );

    if (!window) {
        std::cerr << "Window creation failed\n";
        return false;
    }

    renderer = SDL_CreateRenderer(
        window, -1,
        SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC
    );

    if (!renderer) {
        std::cerr << "Renderer creation failed\n";
        return false;
    }

    return true;
}

void System::initializeSimulation() {
    simulation = new Env(WORLD_WIDTH, WORLD_HEIGHT, MAX_AGENTS);
    simulation->initialize();

    uiButtons = new UIButtons();
    uiStats = new Statistics(simulation);
    uiText = new ChatBox();

    std::cout << "Simulation initialized\n";
}

void System::render() {
    SDL_SetRenderDrawColor(renderer, 20, 20, 20, 255);
    SDL_RenderClear(renderer);

    int envX = MARGIN;
    int envY = BUTTON_BAR_HEIGHT + MARGIN;
    int envWidth = WINDOW_WIDTH - RIGHT_PANEL_WIDTH - MARGIN * 3;
    int envHeight = WINDOW_HEIGHT - BUTTON_BAR_HEIGHT - TEXT_PANEL_HEIGHT - MARGIN * 3;

    int statsX = envX + envWidth + MARGIN;
    int statsY = envY;
    int statsHeight = envHeight + TEXT_PANEL_HEIGHT + MARGIN;

    int textX = envX;
    int textY = envY + envHeight + MARGIN;
    int textWidth = envWidth;
    int textHeight = TEXT_PANEL_HEIGHT;

    uiButtons->render(renderer, WINDOW_WIDTH);
    simulation->renderEnv(renderer, envX, envY, envWidth, envHeight);
    uiStats->render(renderer, statsX, statsY, RIGHT_PANEL_WIDTH, statsHeight);
    uiText->render(renderer, textX, textY, textWidth, textHeight);

    SDL_RenderPresent(renderer);
}

void System::handleEvents(bool& running) {
    SDL_Event event;

    while (SDL_PollEvent(&event)) {
        if (event.type == SDL_QUIT) {
            running = false;
        }
        if (event.type == SDL_KEYDOWN) {
            if (event.key.keysym.sym == SDLK_ESCAPE) {
                running = false;
            }
        }
    }
}

void System::run() {
    std::cout << "Starting simulation (close window to exit)\n\n";

    bool running = true;
    int frameCount = 0;

    while (running) {
        handleEvents(running);

        simulation->update(FRAME_TIME);
        simulation->step();
        frameCount++;

        render();

        if (frameCount % 60 == 0) {
            std::cout << "Frame: " << frameCount
                      << " | Agents: " << simulation->getActiveAgents() << "\n";
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(16));
    }

    std::cout << "\nSimulation stopped\n";
}

void System::shutdown() {
    delete uiButtons;
    delete uiStats;
    delete uiText;

    if (renderer) {
        SDL_DestroyRenderer(renderer);
    }
    if (window) {
        SDL_DestroyWindow(window);
    }
    if (simulation) {
        simulation->cleanup();
        delete simulation;
    }

    SDL_Quit();
}
