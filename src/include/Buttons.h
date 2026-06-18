#ifndef BUTTONS_H
#define BUTTONS_H

#include <SDL2/SDL.h>

class UIButtons {
private:
    SDL_Rect buttons[7];
    const int NUM_BUTTONS = 7;
    const int BAR_HEIGHT = 70;
    const int MARGIN = 20;
    const int BUTTON_HEIGHT = BAR_HEIGHT - MARGIN * 2;

public:
    UIButtons();
    void render(SDL_Renderer* renderer, int windowWidth);
};

#endif
