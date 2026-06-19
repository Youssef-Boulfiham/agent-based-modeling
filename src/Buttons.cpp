#include "../include/Buttons.h"
#include "../include/Env.h"
#include "../include/SettingsWindow.h"
#include "../include/TextRenderer.h"

UIButtons::UIButtons(SettingsWindow* settings)
    : settingsWindow(settings) {}

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

        // Draw button label
        if (!buttonLabels[i].empty()) {
            int textWidth = TextRenderer::width(buttonLabels[i], 1);
            int textX = btnX + (BUTTON_WIDTH - textWidth) / 2;
            int textY = btnY + (BUTTON_HEIGHT - TextRenderer::LINE_H) / 2;
            TextRenderer::draw(renderer, textX, textY, buttonLabels[i], 1, 200, 200, 220);
        }
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

// Map each button to its simulation action. Returns true if a button was hit.
bool UIButtons::handleClick(int x, int y, Env* sim) {
    if (!sim) return false;
    switch (hitTest(x, y)) {
        case 0: sim->toggleLayer(); return true;      // env / background layer
        case 1: sim->togglePaths(); return true;      // agent path overlay
        case 6: settingsWindow->open(); return true;  // settings (rightmost button)
        default: return false;
    }
}
