#include "../include/System.h"
#include "../include/Env.h"
#include "../include/Buttons.h"
#include "../include/Statistics.h"
#include "../include/ChatBox.h"
#include "../include/SettingsWindow.h"
#include <SDL2/SDL_image.h>
#include <iostream>
#include <thread>
#include <chrono>

System::System()
    : window(nullptr), renderer(nullptr), simulation(nullptr),
      uiButtons(nullptr), uiStats(nullptr), uiText(nullptr), settingsWindow(nullptr) {
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

    if (!(IMG_Init(IMG_INIT_PNG) & IMG_INIT_PNG)) {
        std::cerr << "SDL_image PNG init failed: " << IMG_GetError() << "\n";
        return false;
    }

    window = SDL_CreateWindow(
        "Agent-Based Modeling",
        SDL_WINDOWPOS_CENTERED,
        SDL_WINDOWPOS_CENTERED,
        WINDOW_WIDTH,
        WINDOW_HEIGHT,
        SDL_WINDOW_SHOWN | SDL_WINDOW_FULLSCREEN_DESKTOP
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

    // Fill the whole screen: use the actual window size (points == pixels with
    // no HighDPI, so mouse hit-testing stays correct).
    SDL_GetWindowSize(window, &WINDOW_WIDTH, &WINDOW_HEIGHT);

    return true;
}

void System::initializeSimulation() {
    simulation = new Env(WORLD_WIDTH, WORLD_HEIGHT, MAX_AGENTS);
    simulation->initialize();

    settingsWindow = new SettingsWindow();
    settingsWindow->setWorld(simulation);   // bind single source of truth
    uiButtons = new UIButtons(settingsWindow);
    uiStats = new Statistics(simulation);
    uiText = new ChatBox();
    uiText->setWorld(simulation);

    SDL_StartTextInput();   // enable SDL_TEXTINPUT events for the chat box

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
    settingsWindow->render(renderer, WINDOW_WIDTH, WINDOW_HEIGHT);

    SDL_RenderPresent(renderer);
}

void System::handleEvents(bool& running) {
    SDL_Event event;

    while (SDL_PollEvent(&event)) {
        // System-level events handled here; everything else (text input, editing
        // keys, mouse, wheel) is owned by the ChatBox.
        if (event.type == SDL_QUIT ||
            (event.type == SDL_KEYDOWN && event.key.keysym.sym == SDLK_ESCAPE)) {
            running = false;
            continue;
        }

        // Arrows pan and 1/2 zoom the env camera — UNLESS the chat input is
        // focused, in which case the keys go to the ChatBox at the bottom.
        if (event.type == SDL_KEYDOWN && !uiText->isInputFocused()) {
            int dx = 0, dy = 0;
            switch (event.key.keysym.sym) {
                case SDLK_LEFT:  dx = -1; break;
                case SDLK_RIGHT: dx = +1; break;
                case SDLK_UP:    dy = -1; break;
                case SDLK_DOWN:  dy = +1; break;
                default: break;
            }
            if (dx != 0 || dy != 0) {
                simulation->panStep(dx, dy);
                continue;   // consume: don't pass the arrow to the ChatBox
            }

            // 1 = zoom in, 2 = zoom out, focused on the env center.
            int zdir = 0;
            if (event.key.keysym.sym == SDLK_1) zdir = +1;
            if (event.key.keysym.sym == SDLK_2) zdir = -1;
            if (zdir != 0) {
                SDL_Rect e = simulation->getEnvArea();
                simulation->zoomAt(zdir, e.x + e.w / 2, e.y + e.h / 2);
                continue;   // consume: don't type "1"/"2" into the ChatBox
            }
        }

        // Top-bar buttons get first dibs on a left click.
        if (event.type == SDL_MOUSEBUTTONDOWN &&
            event.button.button == SDL_BUTTON_LEFT &&
            uiButtons->handleClick(event.button.x, event.button.y, simulation))
            continue;

        // Settings window gets next dibs if open.
        if (event.type == SDL_MOUSEBUTTONDOWN &&
            event.button.button == SDL_BUTTON_LEFT &&
            settingsWindow->handleClick(event.button.x, event.button.y))
            continue;

        // Env window owns scroll-to-zoom and click-and-drag pan when the
        // pointer is over it. Everything else falls through to the ChatBox.
        SDL_Rect env = simulation->getEnvArea();
        auto inEnv = [&](int px, int py) {
            return px >= env.x && px < env.x + env.w &&
                   py >= env.y && py < env.y + env.h;
        };

        if (event.type == SDL_MOUSEWHEEL) {
            int mx, my;
            SDL_GetMouseState(&mx, &my);
            if (inEnv(mx, my)) {
                float dy = event.wheel.preciseY != 0.0f
                             ? event.wheel.preciseY
                             : static_cast<float>(event.wheel.y);
                if (event.wheel.direction == SDL_MOUSEWHEEL_FLIPPED) dy = -dy;
                if (dy != 0.0f) simulation->zoomAt(dy > 0 ? +1 : -1, mx, my);
                uiText->blurInput();   // interacting with env releases the input
                continue;   // consume: don't scroll the ChatBox
            }
        }

        if (event.type == SDL_MOUSEBUTTONDOWN &&
            event.button.button == SDL_BUTTON_LEFT &&
            inEnv(event.button.x, event.button.y)) {
            panningEnv = true;
            uiText->blurInput();   // clicking the env releases the input
            lastMouseX = event.button.x;
            lastMouseY = event.button.y;
            continue;
        }
        if (event.type == SDL_MOUSEMOTION && panningEnv) {
            simulation->panByPixels(event.motion.x - lastMouseX,
                                    event.motion.y - lastMouseY);
            lastMouseX = event.motion.x;
            lastMouseY = event.motion.y;
            continue;
        }
        if (event.type == SDL_MOUSEBUTTONUP &&
            event.button.button == SDL_BUTTON_LEFT && panningEnv) {
            panningEnv = false;
            continue;
        }

        uiText->handleEvent(event);
    }
}

void System::run() {
    std::cout << "Starting simulation (close window to exit)\n\n";

    bool running = true;
    int frameCount = 0;

    // Fixed-timestep loop. The sim advances exactly one tick per FRAME_TIME of
    // REAL time — independent of how long rendering or pathfinding takes that
    // frame. FRAME_TIME is the single knob: it sets agent speed for all time.
    // Without this, speed = framerate, so heavy frames (e.g. mass A* on a sanity
    // check) made agents crawl while light frames made them zip.
    using clock = std::chrono::steady_clock;
    auto previous = clock::now();
    double accumulator = 0.0;

    while (running) {
        handleEvents(running);

        auto now = clock::now();
        double frameSeconds = std::chrono::duration<double>(now - previous).count();
        previous = now;
        // Clamp to stop a spiral of death after a long stall (debugger, drag).
        if (frameSeconds > 0.25) frameSeconds = 0.25;
        accumulator += frameSeconds;

        while (accumulator >= FRAME_TIME) {
            simulation->update(FRAME_TIME);
            simulation->step();
            accumulator -= FRAME_TIME;
            frameCount++;

            if (frameCount % 60 == 0) {
                std::cout << "Frame: " << frameCount
                          << " | Agents: " << simulation->getActiveAgents() << "\n";
            }
        }

        render();

        // Yield so the loop does not busy-spin at 100% CPU between ticks.
        std::this_thread::sleep_for(std::chrono::milliseconds(2));
    }

    std::cout << "\nSimulation stopped\n";
}

void System::shutdown() {
    SDL_StopTextInput();

    delete settingsWindow;
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

    IMG_Quit();
    SDL_Quit();
}
