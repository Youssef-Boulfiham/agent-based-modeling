#ifndef BUTTONS_H
#define BUTTONS_H

#include <SDL2/SDL.h>
#include <string>

class Env;
class SettingsWindow;

class UIButtons {
private:
    SDL_Rect buttons[7];
    const int NUM_BUTTONS = 7;
    const int BAR_HEIGHT = 70;
    const int MARGIN = 20;
    const int BUTTON_HEIGHT = BAR_HEIGHT - MARGIN * 2;
    SettingsWindow* settingsWindow;

    const std::string buttonLabels[7] = {
        "Layer",
        "Paths",
        "",
        "",
        "",
        "",
        "Settings"
    };

    // Returns index of the button under (x, y), or -1 if none.
    // Valid after render() has stored the current rects.
    int hitTest(int x, int y) const;

public:
    UIButtons(SettingsWindow* settings);
    void render(SDL_Renderer* renderer, int windowWidth);

    // Handle a left click: dispatch the button's action on the simulation.
    // Returns true if a button consumed the click.
    bool handleClick(int x, int y, Env* sim);

    SettingsWindow* getSettingsWindow() const { return settingsWindow; }
};

#endif
