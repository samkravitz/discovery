#include "gpu.h"

GPU::GPU() {
    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        std::cerr << "Could not initialize GPU" << "\n";
        exit(2);
    }

    window = SDL_CreateWindow("discovery", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, SCREEN_WIDTH, SCREEN_HEIGHT, 0);
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
    SDL_Rect rect{8, 8, SCREEN_WIDTH, SCREEN_HEIGHT};
    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
    SDL_RenderPresent(renderer);
}

void GPU::draw(u32 dispcnt, u8 *vram) {
    switch (dispcnt & 0x7) { // bits 0-2 represent video mode
        case 3: draw_mode3(vram); break;
        default: 
            std::cerr << "Error: unknown video mode" << "\n";
            break;
    }
}

void GPU::draw_mode3(u8 *vram) {

}

void GPU::draw_pixel(int x, int y) {
    SDL_RenderDrawPoint(renderer, x, y); //Renders on middle of screen.
    SDL_RenderPresent(renderer);
}