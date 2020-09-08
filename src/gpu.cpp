#include "gpu.h"
#include "obj_attr.h"
#include <ctime>

#define S_TILE_LEN 32
#define D_TILE_LEN 64

#define PX_IN_TILE_ROW 8
#define PX_IN_TILE_COL 8

u8 five_bits_to_eight(u8);

GPU::GPU() {
    mem = NULL;
    stat = NULL;

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

    lcd_clock = 0;
    current_scanline = 0;

    reset();

    old_clock = clock();
}

GPU::~GPU() {
    std::cout << "GPU:: Shutdown\n";
    SDL_DestroyTexture(texture);
    SDL_DestroyRenderer(renderer);
}

void GPU::reset() {
    // paint the screen black
    SDL_Rect rect{8, 8, SCREEN_WIDTH, SCREEN_HEIGHT};
    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
    SDL_RenderPresent(renderer);
}

// 1 clock cycle of the gpu
void GPU::clock_gpu() {
    lcd_clock++;

    // 4 cycles per pixel
    if (lcd_clock % 4 == 0)
        stat->current_scanline_pixel++;

    // finished hDraw
    if (stat->current_scanline_pixel == SCREEN_WIDTH) {
        stat->in_hBlank = true;
    }

    // completed a scanline
    if (lcd_clock % SCANLINE_CYCLES == 0) {
        stat->current_scanline++;
        if (stat->current_scanline == NUM_SCANLINES)
            stat->current_scanline = 0;

        // go back into hDraw
        stat->in_hBlank = false;

        // reset current scanline pixels
        stat->current_scanline_pixel = 0;
    }

    // finished vDraw
    if (stat->current_scanline == SCREEN_HEIGHT) {
        stat->in_vBlank = true;
    }

    // completed a refresh
    if (lcd_clock == REFRESH_CYCLES) {
        lcd_clock = 0; // restart lcd_clock
        stat->current_scanline = 0;
        stat->current_scanline_pixel = 0;
        stat->in_hBlank = false;
        stat->in_vBlank = false;
        draw();
    }
}

void GPU::draw() {
    // if (!stat->needs_refresh) return;

    //std::cout << "Executing graphics mode: " << (mem->read_u32(REG_DISPCNT) & 0x7) << "\n";
    switch (mem->read_u32_unprotected(REG_DISPCNT) & 0x7) { // bits 0-2 represent video mode
        case 0: draw_mode0(); break;
        case 3: draw_mode3(); break;
        case 4: draw_mode4(); break;
        default: 
            std::cerr << "Error: unknown video mode" << "\n";
            break;
    }

    draw_sprites();
    // double duration;
    // clock_t new_time = std::clock();
    // duration = ( new_time - old_clock ) / (double) CLOCKS_PER_SEC;
    // std::cout << "Refresh took: " << duration << "\n";
    // old_clock = new_time;
    // stat->needs_refresh = false;
}

// video mode 0 - sprite mode
void GPU::draw_mode0() { }

// video mode 3 - bitmap mode
void GPU::draw_mode3() {
    u16 current_pixel; // in mode 3 each pixel uses 2 bytes
    u8 r;
    u8 g;
    u8 b;
    u8 alpha = 255;
    u32 *pixels = new u32[SCREEN_WIDTH * SCREEN_HEIGHT]; // array representing each pixel on the screen

    for (int i = 0; i < SCREEN_WIDTH * SCREEN_HEIGHT; ++i) {
        current_pixel = mem->read_u16_unprotected(MEM_VRAM_START + (2 * i)); // multiply i * 2 b/c each pixel is 2 bytes
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
    u8 pallette_index; // in mode 4 each pixel uses 1 byte 
    u32 current_pixel; // the color located at pallette_ram[pallette_index]
    u8 r;
    u8 g;
    u8 b;
    u8 alpha = 0;
    u32 *pixels = new u32[SCREEN_WIDTH * SCREEN_HEIGHT]; // array representing each pixel on the screen

    for (int i = 0; i < SCREEN_WIDTH * SCREEN_HEIGHT; ++i) {
        pallette_index = mem->read_u8_unprotected(MEM_VRAM_START + i);
        current_pixel = mem->read_u32_unprotected(MEM_PALETTE_RAM_START + (pallette_index * sizeof(u16)));
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

void GPU::draw_sprites() {
    u32 *pixels = new u32[SCREEN_WIDTH * SCREEN_HEIGHT];

    for (int i = 0; i < NUM_OBJS; ++i) {
        draw_sprite(get_attr(i), pixels);
    }

    SDL_UpdateTexture(texture, NULL, pixels, SCREEN_WIDTH * sizeof(u32));
    SDL_RenderClear(renderer);
    SDL_RenderCopy(renderer, texture, NULL, NULL);
    SDL_RenderPresent(renderer);

    delete[] pixels;
}

void GPU::draw_sprite(obj_attr attr, u32 *pixels) {
    if (attr.attr_0._zero == 0) return;
    if (attr.attr_1._one == 0) return;
    if (attr.attr_2._two == 0) return;
    
    int starting_pixel = attr.attr_0.attr.y * SCREEN_WIDTH + attr.attr_1.attr.x;
    
    u32 base_tile_addr = LOWER_SPRITE_BLOCK + (attr.attr_2.attr.tileno * S_TILE_LEN);
    int cur_pixel_index = starting_pixel;

    // get width, height in tiles of sprite
    int width, height;
    switch (attr.size()) {
        case 0x0:
            width = 1;
            height = 1;
            break;
        case 0x1:
            width = 2;
            height = 2;
            break;
        case 0x2:
            width = 4;
            height = 4;
            break;
        case 0x3:
            width = 8;
            height = 8;
            break;
        case 0x4:
            width = 2;
            height = 1;
            break;
        case 0x5:
            width = 4;
            height = 1;
            break;
        case 0x6:
            width = 4;
            height = 2;
            break;
        case 0x7:
            width = 8;
            height = 4;
            break;
        case 0x8:
            width = 1;
            height = 2;
            break;
        case 0x9:
            width = 1;
            height = 4;
            break;
        case 0xA:
            width = 2;
            height = 4;
            break;
        case 0xB:
            width = 4;
            height = 8;
            break;
        default:
            std::cerr << "Error: invalid size for object.\n";
            return;
    }

    bool s_tile = attr.attr_0.attr.a == 0;

    for (int y = 0; y < width; ++y) {
        // add current number of rows to the current pixel index
        cur_pixel_index = starting_pixel + (y * SCREEN_WIDTH * 8); // because each tile is 8 pixels long
        for (int x = 0; x < height; ++x) {
            draw_tile(base_tile_addr, cur_pixel_index, pixels, s_tile);

            // tile offset
            base_tile_addr += s_tile ? S_TILE_LEN : D_TILE_LEN;
            
            // 8 pix / tile row
            cur_pixel_index += PX_IN_TILE_ROW;
        }
    }
}

// fills an obj_attr struct from OAM from the given index (0-127)
obj_attr GPU::get_attr(int index) {
    // each oam entry is 4 u16s,
    u32 base_addr = MEM_OAM_START + index * (4 * sizeof(u16));
    obj_attr attr;
    attr.attr_0._zero = mem->read_u16(base_addr + 0);
    attr.attr_1._one = mem->read_u16(base_addr + 2);
    attr.attr_2._two = mem->read_u16(base_addr + 4);
    return attr;
}

// draws a single 8x8 pixel tile
inline void GPU::draw_tile(int starting_address, int starting_pixel, u32 *pixels, bool s_tile) {
    int cur_pixel_index;
    u32 current_pixel;
    u8 palette_index;
    u8 r;
    u8 g;
    u8 b;
    u8 alpha = 255;

    // draw
    if (s_tile) { // 4 bits / pixel - s-tile
        for (int i = 0; i < S_TILE_LEN; i++) {
            //cur_pixel_index = starting_pixel + cur_y_line + ((tile % 8) * 8) + ((i / 4) * SCREEN_WIDTH) + ((i % 4) * 2);
            cur_pixel_index = starting_pixel  + ((i / 4) * SCREEN_WIDTH) + ((i % 4) * 2);
            palette_index = mem->read_u8_unprotected(starting_address + i);
            
            u8 left_pixel = palette_index & 0xF;
            u8 right_pixel = (palette_index >> 4) & 0xF;
            
            current_pixel = mem->read_u32_unprotected(TILE_PALETTE + left_pixel * sizeof(u16));
            
            r = five_bits_to_eight(current_pixel & 0b11111);
            g = five_bits_to_eight((current_pixel >> 5) & 0b11111);
            b = five_bits_to_eight((current_pixel >> 10) & 0b11111);

            // add left pixel in argb format to pixel array
            if (left_pixel == 0) {
                // sprites use palette index 0 as a transparent pixel
                pixels[cur_pixel_index] = 0;
            } else {
                pixels[cur_pixel_index] = alpha;
                pixels[cur_pixel_index] <<= 8;
                pixels[cur_pixel_index] |= r;
                pixels[cur_pixel_index] <<= 8;
                pixels[cur_pixel_index] |= g;
                pixels[cur_pixel_index] <<= 8;
                pixels[cur_pixel_index] |= b;
            }

            current_pixel = mem->read_u32_unprotected(TILE_PALETTE + right_pixel * sizeof(u16));
            
            r = five_bits_to_eight(current_pixel & 0b11111);
            g = five_bits_to_eight((current_pixel >> 5) & 0b11111);
            b = five_bits_to_eight((current_pixel >> 10) & 0b11111);

            // add right pixel in argb format to pixel array
            if (right_pixel == 0) {
                // sprites use palette index 0 as a transparent pixel
                pixels[cur_pixel_index + 1] = 0;
            } else {
                pixels[cur_pixel_index + 1] = alpha;
                pixels[cur_pixel_index + 1] <<= 8;
                pixels[cur_pixel_index + 1] |= r;
                pixels[cur_pixel_index + 1] <<= 8;
                pixels[cur_pixel_index + 1] |= g;
                pixels[cur_pixel_index + 1] <<= 8;
                pixels[cur_pixel_index + 1] |= b;
            }
        }
    } else { // 8 bits / pixel - d-tile
        for (int i = 0; i < D_TILE_LEN; i++) {
            cur_pixel_index = starting_pixel  + ((i / 8) * SCREEN_WIDTH) + (i % 8);
            palette_index = mem->read_u8_unprotected(starting_address + i);
            current_pixel = mem->read_u32_unprotected(TILE_PALETTE + palette_index * sizeof(u16));
            
            r = five_bits_to_eight(current_pixel & 0b11111);
            g = five_bits_to_eight((current_pixel >> 5) & 0b11111);
            b = five_bits_to_eight((current_pixel >> 10) & 0b11111);

            // add pixel in argb format to pixel array
            if (current_pixel == 0) {
                // sprites use palette index 0 as a transparent pixel
                pixels[cur_pixel_index] = 0;
            } else {
                pixels[cur_pixel_index] = alpha;
                pixels[cur_pixel_index] <<= 8;
                pixels[cur_pixel_index] |= r;
                pixels[cur_pixel_index] <<= 8;
                pixels[cur_pixel_index] |= g;
                pixels[cur_pixel_index] <<= 8;
                pixels[cur_pixel_index] |= b;
            }
        }
    }
    
}

// given a range of 0-31 return a range of 0-255
inline u8 five_bits_to_eight (u8 u5) {
    return u5 << 3;
}