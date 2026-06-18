#include "../include/ChatBox.h"

ChatBox::ChatBox() {}

void ChatBox::render(SDL_Renderer* renderer, int x, int y, int width, int height) {
    panelArea = {x, y, width, height};

    SDL_SetRenderDrawColor(renderer, 40, 40, 50, 255);
    SDL_RenderFillRect(renderer, &panelArea);
    SDL_SetRenderDrawColor(renderer, 100, 100, 150, 255);
    SDL_RenderDrawRect(renderer, &panelArea);
}
