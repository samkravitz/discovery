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

#define CHARBLOCK_LEN             0x4000
#define SCREENBLOCK_LEN           0x800
#define S_SCREENENTRY_DEPTH       32    // s-tile SE depth
#define D_SCREENENTRY_DEPTH       64    // d-tile SE depth
#define TILES_PER_SCREENBLOCK     32

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

    i = 0;
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
        if (i > 10)
            draw();
        i++;
    }
}

void GPU::draw() {
    // std::cout << "Executing graphics mode: " << (mem->read_u32(REG_DISPCNT) & 0x7) << "\n";
    // std::cout << "Executing graphics mode: " << (int) stat->reg_dispcnt.obj_enabled << "\n";
    switch (stat->reg_dispcnt.mode) { // bits 0-2 represent video mode
        case 0: draw_mode0(); break;
        case 3: draw_mode3(); break;
        case 4: draw_mode4(); break;
        default: 
            std::cerr << "Error: unknown video mode" << "\n";
            break;
    }

    // sprites enabled
    if (stat->reg_dispcnt.obj_enabled)
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

    double duration;
    clock_t new_time = std::clock();
    duration = ( new_time - old_clock ) / (double) CLOCKS_PER_SEC;
    std::cout << "Refresh took: " << duration << "\n";
    old_clock = new_time;
    stat->needs_refresh = false;
}

// video mode 0 - tile mode
// can draw bg0-bg3 all regular
void GPU::draw_mode0() {
    for (int priority = 3; priority >= 0; --priority) // draw highest priority first, lower priorities drawn on top
        for (int i = 0; i < 3; ++i) // bg0-bg3
            if (stat->bg_cnt[i].enabled && stat->bg_cnt[i].priority == priority)
                draw_reg_background(i);
}

// video mode 3 - bitmap mode
// mode 3 straight up uses 2 bytes to represent each pixel in aRBG format, no palette used
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
    u8 palette_index; // in mode 4 each pixel uses 1 byte 
    u32 color;        // the color located at pallette_ram[palette_index]

    int i = 0;
    for (int y = 0; y < SCREEN_HEIGHT; ++y) {
        for (int x = 0; x < SCREEN_WIDTH; ++x) {
            palette_index = mem->read_u8_unprotected(MEM_VRAM_START + i);
            // multiply by sizeof(u16) because each entry in palram is 2 bytes
            color = mem->read_u32_unprotected(MEM_PALETTE_RAM_START + (palette_index * sizeof(u16)));

            // add current pixel in argb format to pixel array
            screen_buffer[y][x] = u16_to_u32_color(color);
            ++i;
        }
    }
}

// draws regular background
// bg - background index (0-3)
void GPU::draw_reg_background(int bg) {
    // initial address of background tileset
    u32 tileset_address = MEM_VRAM_START + CHARBLOCK_LEN * stat->bg_cnt[bg].cbb;

    // initial address of background tilemap
    u32 start_tilemap_address = MEM_VRAM_START + SCREENBLOCK_LEN * stat->bg_cnt[bg].sbb;
    u32 tilemap_address = start_tilemap_address;

    // get width / height (in tiles) of background
    int width, height;
    switch(stat->bg_cnt[bg].size) {
        case 0:
            width = 32;
            height = 32;
        break;
        
        case 1:
            width = 64;
            height = 32;
        break;

        case 2:
            width = 32;
            height = 64;
        break;

        case 3:
            width = 64;
            height = 64;
        break;
    }

    // entire map (bigger than screen)
    u32 map[height * PX_IN_TILE_COL][width * PX_IN_TILE_ROW] = {0};

    u16 screen_entry;    // contains tile index, flipping flags, and balbank for s-tiles
    u16 tilemap_index;   // index # of current tile
    u32 cur_screenblock; // address of palette entries of current tile
    u16 color;           // arbg color of current pixel
    u8 palette_index;    // index of entry in palette memory
    u8 palbank;          // palette bank (for s-tiles)
    int x, y;            // (x, y) coordinate of current pixel;

    for (int ssy = 0; ssy < (height / 32); ++ssy) {
        for (int ssx = 0; ssx < (width / 32); ++ssx) {
            tilemap_address = start_tilemap_address + ssx * SCREENBLOCK_LEN + (2 * ssy * SCREENBLOCK_LEN);
            for (int h = 0; h < TILES_PER_SCREENBLOCK; ++h) {
                for (int w = 0; w < TILES_PER_SCREENBLOCK; ++w) {
                    screen_entry = mem->read_u16_unprotected(tilemap_address);
                    tilemap_index = screen_entry & 0x3FF; // bits 9-0 

                    if (stat->bg_cnt[bg].color_mode == 0) { // s-tile (4bpp)
                        palbank = screen_entry >> 12 & 0xF; // bits F - C
                        cur_screenblock = tileset_address + S_SCREENENTRY_DEPTH * tilemap_index;

                        for (int i = 0; i < S_TILE_LEN; ++i) {
                            // 256 bc each screenblock is 32 tiles, 32 * 8 = 256
                            x = ssx * 256 + w * PX_IN_TILE_ROW + 2 * (i % 4); // s-tiles get left/right px in one read 
                            y = ssy * 256 + h * PX_IN_TILE_COL + (i / 4);
                            palette_index = mem->read_u8_unprotected(cur_screenblock + i);
                            
                            u8 left_pixel = palette_index & 0xF;
                            u8 right_pixel = (palette_index >> 4) & 0xF;

                            // add left, right pixel to screen buffer
                            // pixel value 0 is transparent, so only draw if not 0
                            if (left_pixel != 0) {
                                // multiply by sizeof(u16) because each entry in palram is 2 bytes
                                color = mem->read_u16_unprotected(MEM_PALETTE_RAM_START + left_pixel * sizeof(u16) + (palbank * PALBANK_LEN));
                                map[y][x] = u16_to_u32_color(color);
                            }

                            // pixel value 0 is transparent, so only draw if not 0
                            if (right_pixel != 0) {
                                // multiply by sizeof(u16) because each entry in palram is 2 bytes
                                color = mem->read_u16_unprotected(MEM_PALETTE_RAM_START + right_pixel * sizeof(u16) + (palbank * PALBANK_LEN));
                                map[y][x + 1] = u16_to_u32_color(color);
                            }
                        }

                    } else { // d-tile (8bpp)

                        cur_screenblock = tileset_address + D_SCREENENTRY_DEPTH * tilemap_index;

                        for (int i = 0; i < D_TILE_LEN; i++) {

                            palette_index = mem->read_u8_unprotected(cur_screenblock + i);

                            // pixel value 0 is transparent, so only draw if not 0
                            if (palette_index == 0)
                                continue; 

                            // 256 bc each screenblock is 32 tiles, 32 * 8 = 256
                            x = ssx * 256 + w * PX_IN_TILE_ROW + (i % 8);
                            y = ssy * 256 + h * PX_IN_TILE_COL + (i / 8);

                            // multiply by sizeof(u16) because each entry in palram is 2 bytes
                            color = mem->read_u32_unprotected(SPRITE_PALETTE + palette_index * sizeof(u16));

                            screen_buffer[y][x] = u16_to_u32_color(color);
                        }
                    }

                    tilemap_address += 2; // each tile is 2 bytes long

                }
            }
        }
    }

    // get vertical & horizontal offset
    u16 voff, hoff;
    switch (bg) {
        case 0:
            voff = mem->read_u16_unprotected(REG_BG0VOFS);
            hoff = mem->read_u16_unprotected(REG_BG0HOFS);
        break;

        case 1:
            voff = mem->read_u16_unprotected(REG_BG1VOFS);
            hoff = mem->read_u16_unprotected(REG_BG1HOFS);
        break;

        case 2:
            voff = mem->read_u16_unprotected(REG_BG2VOFS);
            hoff = mem->read_u16_unprotected(REG_BG2HOFS);
        break;

        case 3:
            voff = mem->read_u16_unprotected(REG_BG3VOFS);
            hoff = mem->read_u16_unprotected(REG_BG3HOFS);
        break;
    }

    // copy area of map that screen is over into screen buffer
    for (int y = 0; y < SCREEN_HEIGHT; ++y) {
        for (int x = 0; x < SCREEN_WIDTH; ++x) {
            // modulo mapsize to allow wrapping
            if (map[(y + voff) % (height * PX_IN_TILE_COL)][(x + hoff) % (width * PX_IN_TILE_ROW)] != 0)
                screen_buffer[y][x] = map[(y + voff) % (height * PX_IN_TILE_COL)][(x + hoff) % (width * PX_IN_TILE_ROW)];
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

    // std::cout << "x: " << (int) x << "\n";
    // std::cout << "y: " <<  (int) y << "\n";

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
            draw_sprite_tile(base_tile_addr, x + 8 * w, y + 8 * h, s_tile, attr.attr_2.attr.l);
            // tile offset
            base_tile_addr += s_tile ? S_TILE_LEN : D_TILE_LEN;
        }
    }

    // horizontal flip
    if (hor_flip) {
        u32 temp;
        for (int h = 0; h < height * 8; ++h) {
            for (int w = 0; w < width * 4; ++w) {
                temp = screen_buffer[y + h][x + w];
                screen_buffer[y + h][x + w] = screen_buffer[y + h][(x + width * 8) - w];
                screen_buffer[y + h][(x + width * 8) - w] = temp;
            }
        }
    }

    // vertical flip
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

// draws a single 8x8 pixel tile
inline void GPU::draw_sprite_tile(int starting_address, u16 start_x, u8 start_y, bool s_tile, u8 palbank) {
    u16 color;
    u8 palette_index; // nth entry in palram
    int x, y;

    // draw
    if (s_tile) { // 4 bits / pixel - s-tile
        for (int i = 0; i < S_TILE_LEN; ++i) {
            palette_index = mem->read_u8_unprotected(starting_address + i);

            x = start_x + (2 * (i % 4));
            y = start_y + (i / 4);

            u8 left_pixel = palette_index & 0xF;
            u8 right_pixel = (palette_index >> 4) & 0xF;
            
            // add left, right pixel to screen buffer
            // pixel value 0 is transparent, so only draw if not 0
            if (left_pixel != 0) {
                // multiply by sizeof(u16) because each entry in palram is 2 bytes
                color = mem->read_u16_unprotected(SPRITE_PALETTE + left_pixel * sizeof(u16) + (palbank * PALBANK_LEN));
                screen_buffer[y][x] = u16_to_u32_color(color);
            }

            // pixel value 0 is transparent, so only draw if not 0
            if (right_pixel != 0) {
                // multiply by sizeof(u16) because each entry in palram is 2 bytes
                color = mem->read_u16_unprotected(SPRITE_PALETTE + right_pixel * sizeof(u16) + (palbank * PALBANK_LEN));
                screen_buffer[y][x + 1] = u16_to_u32_color(color);
            }
        }


    } else { // 8 bits / pixel - d-tile
        for (int i = 0; i < D_TILE_LEN; i++) {

            palette_index = mem->read_u8_unprotected(starting_address + i);

            // pixel value 0 is transparent, so only draw if not 0
            if (palette_index == 0)
                continue; 

            x = start_x + (i % 8);
            y = start_y + (i / 8);

            // multiply by sizeof(u16) because each entry in palram is 2 bytes
            color = mem->read_u32_unprotected(SPRITE_PALETTE + palette_index * sizeof(u16));

            screen_buffer[y][x] = u16_to_u32_color(color);
        }
    }
}

// fills the objs data structure every frame an object needs to be drawn
void GPU::update_attr() {
    u32 address = MEM_OAM_START;
    for (int i = 0; i < 128; ++i) { // loop through all 128 objects
        // attr 0
        objs[i].attr_0._zero = mem->read_u16(address);
        address += sizeof(u16);

        // attr 1
        objs[i].attr_1._one = mem->read_u16(address);
        address += sizeof(u16);

        // attr 2
        objs[i].attr_2._two = mem->read_u16(address);
        address += sizeof(u16);

        // fill
        objs[i].fill = mem->read_u16(address);
        address += sizeof(u16);
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

// given a 16 bit GBA color, make it a 32 bit SDL color
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