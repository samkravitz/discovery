#include "gpu.h"

u8 five_bits_to_eight(u8);

GPU::GPU() {
    mem = NULL;

    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        std::cerr << "Could not initialize GPU" << "\n";
        exit(2);
    }

    window = SDL_CreateWindow("discovery", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, SCREEN_WIDTH, SCREEN_HEIGHT, 0);
    if (window == NULL) {
        std::cerr << "Could not create window" << "\n";
        exit(2);
    }

    renderer = SDL_CreateRenderer(window, -1, 0);
    texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STATIC, SCREEN_WIDTH, SCREEN_HEIGHT);

    reset();
}

GPU::~GPU() {
    delete mem;
    SDL_DestroyTexture(texture);
    SDL_DestroyRenderer(renderer);
}

void GPU::reset() {
    // paint the screen black
    SDL_Rect rect{8, 8, SCREEN_WIDTH, SCREEN_HEIGHT};
    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
    SDL_RenderPresent(renderer);
}

void GPU::draw() {
    std::cout << "Executing graphics mode: " << (mem->read_u32(REG_DISPCNT) & 0x7) << "\n";
    switch (mem->read_u32(REG_DISPCNT) & 0x7) { // bits 0-2 represent video mode
        case 3: draw_mode3(); break;
        case 4: draw_mode4(); break;
        default: 
            std::cerr << "Error: unknown video mode" << "\n";
            break;
    }
}

// video mode 3 - bitmap mode
void GPU::draw_mode3() {
    u16 current_pixel; // in mode 3 each pixel uses 2 bytes
    u8 r;
    u8 g;
    u8 b;
    u8 alpha = 255;
    u32 *pixels = new u32[SCREEN_WIDTH * SCREEN_HEIGHT]; // array representing each pixel on the screen

    for (int i = 0; i < SCREEN_WIDTH * SCREEN_HEIGHT; ++i) {
        current_pixel = mem->read_u16(MEM_VRAM_START + (2 * i)); // multiply i * 2 b/c each pixel is 2 bytes
        r = five_bits_to_eight(current_pixel & 0b11111);
        g = five_bits_to_eight((current_pixel >> 5) & 0b11111);
        b = five_bits_to_eight((current_pixel >> 10) & 0b11111);

        // add current pixel in argb format to pixel array
        pixels[i] = alpha;
        pixels[i] <<= 8;
        pixels[i] |= r;
        pixels[i] <<= 8;
        pixels[i] |= g;
        pixels[i] <<= 8;
        pixels[i] |= b;
    }   

    SDL_UpdateTexture(texture, NULL, pixels, SCREEN_WIDTH * sizeof(u32));
    SDL_RenderClear(renderer);
    SDL_RenderCopy(renderer, texture, NULL, NULL);
    SDL_RenderPresent(renderer);

    delete[] pixels;
}

// video mode 4 - bitmap mode
void GPU::draw_mode4() {
    // u8 current_pixel; // in mode 3 each pixel uses 2 bytes
    // u8 r;
    // u8 g;
    // u8 b;
    // u8 alpha = 0;
    // u32 *pixels = new u32[SCREEN_WIDTH * SCREEN_HEIGHT]; // array representing each pixel on the screen

    // for (int i = 0; i < SCREEN_WIDTH * SCREEN_HEIGHT; ++i) {
    //     current_pixel = mem->read_u16(MEM_VRAM_START + (2 * i)); // multiply i * 2 b/c each pixel is 2 bytes
    //     r = five_bits_to_eight(current_pixel & 0b11111);
    //     g = five_bits_to_eight((current_pixel >> 5) & 0b11111);
    //     b = five_bits_to_eight((current_pixel >> 10) & 0b11111);

    //     // add current pixel in argb format to pixel array
    //     pixels[i] = alpha;
    //     pixels[i] <<= 8;
    //     pixels[i] |= b;
    //     pixels[i] <<= 8;
    //     pixels[i] |= g;
    //     pixels[i] <<= 8;
    //     pixels[i] |= r;
    // }   

    // SDL_UpdateTexture(texture, NULL, pixels, SCREEN_WIDTH * sizeof(u32));
    // SDL_RenderClear(renderer);
    // SDL_RenderCopy(renderer, texture, NULL, NULL);
    // SDL_RenderPresent(renderer);

    // delete[] pixels;
    std::cout << "Video mode 4!\n";
}

void GPU::draw_pixel(int x, int y) {
    SDL_RenderDrawPoint(renderer, x, y); //Renders on middle of screen.
    SDL_RenderPresent(renderer);
}

// given a range of 0-31 return a range of 0-255
inline u8 five_bits_to_eight (u8 u5) {
    return u5 * 255 / 32;
}