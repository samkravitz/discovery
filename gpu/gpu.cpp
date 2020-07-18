#include "gpu.h"

GPU::GPU() {
    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        std::cerr << "Could not initialize GPU" << "\n";
        exit(2);
    }

    window = SDL_CreateWindow("discovery", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 240, 160, 0);
    if (window == NULL) {
        std::cerr << "Could not create window" << "\n";
        exit(2);
    }

    surface = SDL_GetWindowSurface(window);
    renderer = SDL_CreateRenderer(window, -1, 0);

    reset();
}

GPU::~GPU() {
    
}

void GPU::reset() {
    // paint the screen black
    SDL_Rect rect{8, 8, 240, 160};
    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
    SDL_RenderPresent(renderer);
}

void GPU::draw_pixel(int x, int y) {
    SDL_RenderDrawPoint(renderer, x, y); //Renders on middle of screen.
    SDL_RenderPresent(renderer);
}