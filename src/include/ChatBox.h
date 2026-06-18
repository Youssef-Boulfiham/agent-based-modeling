#ifndef CHATBOX_H
#define CHATBOX_H

#include <SDL2/SDL.h>

class ChatBox {
private:
    SDL_Rect panelArea;

public:
    ChatBox();
    void render(SDL_Renderer* renderer, int x, int y, int width, int height);
};

#endif
