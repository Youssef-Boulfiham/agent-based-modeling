#include "../include/Buttons.h"

UIButtons::UIButtons() {}

void UIButtons::render(SDL_Renderer* renderer, int windowWidth) {
    const int BUTTON_MARGIN = 10;
    const int BUTTON_WIDTH = (windowWidth - MARGIN * 2 - BUTTON_MARGIN * (NUM_BUTTONS - 1)) / NUM_BUTTONS;

    for (int i = 0; i < NUM_BUTTONS; ++i) {
        int btnX = MARGIN + i * (BUTTON_WIDTH + BUTTON_MARGIN);
        int btnY = MARGIN;

        buttons[i] = {btnX, btnY, BUTTON_WIDTH, BUTTON_HEIGHT};

        SDL_SetRenderDrawColor(renderer, 60, 60, 80, 255);
        SDL_RenderFillRect(renderer, &buttons[i]);

        SDL_SetRenderDrawColor(renderer, 150, 150, 200, 255);
        SDL_RenderDrawRect(renderer, &buttons[i]);
    }
}

int UIButtons::hitTest(int x, int y) const {
    for (int i = 0; i < NUM_BUTTONS; ++i) {
        const SDL_Rect& b = buttons[i];
        if (x >= b.x && x < b.x + b.w && y >= b.y && y < b.y + b.h)
            return i;
    }
    return -1;
}
