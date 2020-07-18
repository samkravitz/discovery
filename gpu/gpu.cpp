#include "gpu.h"
//#include <SDL2/SDL.h>

GPU::GPU() {
    init();
}

GPU::~GPU() {

}

void GPU::init() {
    SDL_CreateWindowAndRenderer(800, 600, 0, &window, &renderer);
}

void GPU::draw_pixel(int x, int y) {
    SDL_RenderDrawPoint(renderer, x, y); //Renders on middle of screen.
    SDL_RenderPresent(renderer);
}