#ifndef STATISTICS_H
#define STATISTICS_H

#include <SDL2/SDL.h>
#include "Env.h"

class Statistics {
private:
    Env* simulation;
    SDL_Rect panelArea;

public:
    Statistics(Env* sim);
    void render(SDL_Renderer* renderer, int x, int y, int width, int height);
};

#endif
