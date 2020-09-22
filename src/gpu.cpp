#include "gpu.h"
#include "obj_attr.h"
#include <ctime>

#define S_TILE_LEN 32
#define D_TILE_LEN 64

#define PX_IN_TILE_ROW 8
#define PX_IN_TILE_COL 8

// length of each of palette RAM's 16 banks
// in 4bpp mode (s-tiles)
#define PALBANK_LEN 32

u32 u16_to_u32_color(u16);

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

    final_screen = SDL_GetWindowSurface(window);
    original_screen = SDL_CreateRGBSurface(0, SCREEN_WIDTH, SCREEN_HEIGHT, 32, 0, 0, 0, 0);

    lcd_clock = 0;
    current_scanline = 0;

    reset();

    old_clock = clock();
}

GPU::~GPU() {
    std::cout << "GPU:: Shutdown\n";
    SDL_Quit();
}

void GPU::reset() {
    memset(screen_buffer, 0, sizeof(screen_buffer));
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

    // copy pixel buffer over to surface pixels
    if (SDL_MUSTLOCK(final_screen)) SDL_LockSurface(final_screen);

    u32 *screen_pixels = (u32 *) final_screen->pixels;

    for (int y = 0; y < SCREEN_HEIGHT; ++y) {
        for (int x = 0; x < SCREEN_WIDTH; ++x) {
            screen_pixels[y * SCREEN_WIDTH + x] = screen_buffer[y][x];
        }
    }


    if (SDL_MUSTLOCK(final_screen)) SDL_UnlockSurface(final_screen);

    //SDL_BlitSurface(original_screen, NULL, final_screen, NULL);

    // draw final_screen pixels on screen
    SDL_UpdateWindowSurface(window);

    // zero screen buffer for next frame
    memset(screen_buffer, 0, sizeof(screen_buffer));

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

    int i = 0;
    for (int y = 0; y < SCREEN_HEIGHT; ++y) {
        for (int x = 0; x < SCREEN_WIDTH; ++x) {
            current_pixel = mem->read_u16_unprotected(MEM_VRAM_START + (2 * i)); // multiply i * 2 b/c each pixel is 2 bytes
            screen_buffer[y][x] = u16_to_u32_color(current_pixel);
            ++i;
        }
    }
}

// video mode 4 - bitmap mode
void GPU::draw_mode4() {
    u8 pallette_index; // in mode 4 each pixel uses 1 byte 
    u32 current_pixel; // the color located at pallette_ram[pallette_index]

    int i = 0;
    for (int y = 0; y < SCREEN_HEIGHT; ++y) {
        for (int x = 0; x < SCREEN_WIDTH; ++x) {
            pallette_index = mem->read_u8_unprotected(MEM_VRAM_START + i);
            current_pixel = mem->read_u32_unprotected(MEM_PALETTE_RAM_START + (pallette_index * sizeof(u16)));

            // add current pixel in argb format to pixel array
            screen_buffer[y][x] = u16_to_u32_color(current_pixel);
            ++i;
        }
    }
}

void GPU::draw_sprites() {
    for (int i = 0; i < NUM_OBJS; ++i) {
        draw_sprite(get_attr(i));
    }
}

void GPU::draw_sprite(obj_attr attr) {
    // rendering disabled (hidden)
    if (attr.attr_0.attr.d == 1 && attr.attr_0.attr.r == 0) return;

    // use some masking to make x and y fit in screen coordinates
    u16 x = attr.attr_1.attr.x;
    u8 y = attr.attr_0.attr.y;

    u32 base_tile_addr = LOWER_SPRITE_BLOCK + (attr.attr_2.attr.tileno * S_TILE_LEN);

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

    // flips
    bool hor_flip = attr.attr_1.attr.h == 1;
    bool vert_flip = attr.attr_1.attr.v == 1;

    for (int h = 0; h < height; ++h) {
        for (int w = 0; w < width; ++w) {
            // multiply 8 because each tile is 8 pixels wide
            draw_tile(base_tile_addr, x + 8 * w, y + 8 * h, s_tile, attr.attr_2.attr.l);
            // tile offset
            base_tile_addr += s_tile ? S_TILE_LEN : D_TILE_LEN;
        }
    }

    if (hor_flip) {
        u32 temp;
    }

    if (vert_flip) {
        u32 temp;
        for (int h = 0; h < height * 4; ++h) {
            for (int w = 0; w < width * 8; ++w) {
                temp = screen_buffer[y + h][x + w];
                screen_buffer[y + h][x + w] = screen_buffer[(y + height * 8) - h][x + w];
                screen_buffer[(y + height * 8) - h][x + w] = temp;
            }
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
inline void GPU::draw_tile(int starting_address, int start_x, int start_y, bool s_tile, u8 palbank) {
    int cur_pixel_index;
    u32 current_pixel;
    u8 palette_index;
    int x, y;

    // draw
    if (s_tile) { // 4 bits / pixel - s-tile
        for (int i = 0; i < S_TILE_LEN; ++i) {
            x = start_x + (2 * (i % 4));
            y = start_y + (i / 4);

            if (y >= MAX_Y) {
                continue;
            }

            palette_index = mem->read_u8_unprotected(starting_address + i);
            
            u8 left_pixel = palette_index & 0xF;
            u8 right_pixel = (palette_index >> 4) & 0xF;
            
            // multiply by sizeof(u16) because each index in palram represents 2 bytes
            current_pixel = mem->read_u16_unprotected(TILE_PALETTE + left_pixel * sizeof(u16) + (palbank * PALBANK_LEN));
            
            // add left pixel in argb format to pixel array
            if (left_pixel == 0) {
                // sprites use palette index 0 as a transparent pixel
                screen_buffer[y][x] = 0;
            } else {
                screen_buffer[y][x] = u16_to_u32_color(current_pixel);
            }

            // multiply by sizeof(u16) because each index in palram represents 2 bytes
            current_pixel = mem->read_u16_unprotected(TILE_PALETTE + right_pixel * sizeof(u16) + (palbank * PALBANK_LEN));

            // add right pixel in argb format to pixel array
            if (right_pixel == 0) {
                // sprites use palette index 0 as a transparent pixel
                screen_buffer[y][x + 1] = 0;
            } else {
                screen_buffer[y][x + 1] = u16_to_u32_color(current_pixel);
            }
        }


    } else { // 8 bits / pixel - d-tile
        for (int i = 0; i < D_TILE_LEN; i++) {
            x = start_x + (i % 8);
            y = start_y + (i / 8);

            // out of bounds
            if (y >= MAX_Y) {
                continue;
            }

            palette_index = mem->read_u8_unprotected(starting_address + i);
            current_pixel = mem->read_u32_unprotected(TILE_PALETTE + palette_index * sizeof(u16));

            // add pixel in argb format to pixel array
            if (current_pixel == 0) {
                // sprites use palette index 0 as a transparent pixel
                screen_buffer[y][x] = 0;
            } else {
                screen_buffer[y][x] = u16_to_u32_color(current_pixel);
            }
        }
    }
}

// given a range of 0-31 return a range of 0-255
inline u32 u16_to_u32_color (u16 color_u16) {
    u8 r, g, b;
    u32 color = 255; // alpha value
    color <<= 8;
    r = ((color_u16 & 0b11111) << 3);
    color |= r;
    color <<= 8;
    g = (((color_u16 >> 5) & 0b11111) << 3);
    color |= g;
    color <<= 8;
    b = (((color_u16 >> 10) & 0b11111) << 3);
    color |= b; 
    return color;
}