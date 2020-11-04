#include <ctime>

#include "gpu.h"

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

GPU::GPU()
{
    mem = NULL;
    stat = NULL;

    if (SDL_Init(SDL_INIT_VIDEO) < 0)
    {
        std::cerr << "Could not initialize GPU" << "\n";
        exit(2);
    }

    window = SDL_CreateWindow("discovery", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, SCREEN_WIDTH, SCREEN_HEIGHT, 0);
    if (window == NULL)
    {
        std::cerr << "Could not create window" << "\n";
        exit(2);
    }

    final_screen = SDL_GetWindowSurface(window);
    original_screen = SDL_CreateRGBSurface(0, SCREEN_WIDTH, SCREEN_HEIGHT, 32, 0, 0, 0, 0);

    reset();
}

GPU::~GPU()
{
    std::cout << "GPU:: Shutdown\n";
    SDL_Quit();
}

void GPU::reset()
{
    cycles = 0;
    scanline = 0;
    memset(screen_buffer, 0, sizeof(screen_buffer));
    old_clock = clock();
}

// 1 clock cycle of the gpu
void GPU::cycle()
{
    cycles++;

    // start HBlank
    if (cycles == HDRAW)
    {
        stat->dispstat.in_hBlank = true;

        // fire HBlank interrupt if necessary
        if (stat->dispstat.hbi)
            std::cout << "Firing HBlank interrupt\n";

        // check for DMA HBlank requests
        for (int i = 0; i < 4; ++i)
        {
            if (mem->dma[i].enable && mem->dma[i].mode == 2) // start at HBLANK
            {
                mem->_dma(i);
                //std::cout << "DMA" << i << " HBLANK\n";
            }
        }
        
        // start VBlank
        if (scanline == VDRAW)
        {
            draw();
            stat->dispstat.in_vBlank = true;

            // fire Vblank interrupt if necessary
            if (stat->dispstat.vbi)
                std::cout << "Firing VBlank interrupt\n";

            // check for DMA VBLANK requests
            for (int i = 0; i < 4; ++i)
            {
                if (mem->dma[i].enable && mem->dma[i].mode == 1) // start at VBLANK
                    mem->_dma(i);
            }
        }
    }

    // completed HBlank
    else if (cycles == HDRAW + HBLANK)
    {
        // completed full refresh
        if (scanline == VDRAW + VBLANK)
        {
            stat->dispstat.in_vBlank = false;
            scanline = 0;
            stat->scanline = 0;
        }

        else
        {
            scanline++;
            stat->scanline++;
        }

        // scanline has reached trigger value
        if (scanline == stat->dispstat.vct)
        {
            // set trigger status
            stat->dispstat.vcs = 1;
            
            // interrupt is triggered if requested
            if (stat->dispstat.vci)
                std::cout << "Firing VCount interrupt\n";
        }
        
        // scanline is not equal to trigger value, reset this bit
        else
        {
            stat->dispstat.vcs = 0;
        }
        

        cycles = 0;
        stat->dispstat.in_hBlank = false;
    }
}

void GPU::draw()
{
    //std::cout << "Executing graphics mode: " << (int) (stat->dispcnt.mode) << "\n";
    switch (stat->dispcnt.mode)
    {
        case 0: draw_mode0(); break;
        case 1: draw_mode1(); break;
        case 3: draw_mode3(); break;
        case 4: draw_mode4(); break;
        default: 
            std::cerr << "Error: unknown video mode" << "\n";
            break;
    }

    // sprites enabled
    if (stat->dispcnt.obj_enabled)
    {
        update_attr();
        draw_sprites();
    }

    // copy pixel buffer over to surface pixels
    if (SDL_MUSTLOCK(final_screen))
        SDL_LockSurface(final_screen);

    u32 *screen_pixels = (u32 *) final_screen->pixels;
    for (int y = 0; y < SCREEN_HEIGHT; ++y)
        for (int x = 0; x < SCREEN_WIDTH; ++x)
            screen_pixels[y * SCREEN_WIDTH + x] = screen_buffer[y][x];

    if (SDL_MUSTLOCK(final_screen))
        SDL_UnlockSurface(final_screen);

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
}

// video mode 0 - tile mode
// can draw bg0 - bg3 all regular
void GPU::draw_mode0()
{
    for (int priority = 3; priority >= 0; --priority) // draw highest priority first, lower priorities drawn on top
        for (int i = 0; i <= 3; ++i) // bg0 - bg3
            if (stat->bg_cnt[i].enabled && stat->bg_cnt[i].priority == priority)
            {
                //std::cout << "drawing bg: " << i << "\n";
                draw_reg_background(i);
            }
}

// video mode 1 - tile mode
// can draw bg0, bg1 regular and bg 2 affine
void GPU::draw_mode1()
{
    for (int priority = 3; priority >= 0; --priority) // draw highest priority first, lower priorities drawn on top
        for (int i = 0; i <= 2; ++i) // bg0 - bg2
            if (stat->bg_cnt[i].enabled && stat->bg_cnt[i].priority == priority)
            {
                switch (i)
                {
                    case 0:
                    case 1:
                        draw_reg_background(i);
                        break;
                    case 2:
                        draw_affine_background(i);
                        break;
                    default: // should never happen
                        std::cerr << "Error: trying to draw invalid background in mode 1: " << i << "\n";
                }
            }
}

// video mode 3 - bitmap mode
// mode 3 straight up uses 2 bytes to represent each pixel in aRBG format, no palette used
void GPU::draw_mode3()
{
    u16 current_pixel; // in mode 3 each pixel uses 2 bytes

    int i = 0;
    for (int y = 0; y < SCREEN_HEIGHT; ++y)
    {
        for (int x = 0; x < SCREEN_WIDTH; ++x)
        {
            current_pixel = mem->read_u16_unprotected(MEM_VRAM_START + (2 * i)); // multiply i * 2 b/c each pixel is 2 bytes
            screen_buffer[y][x] = u16_to_u32_color(current_pixel);
            ++i;
        }
    }
}

// video mode 4 - bitmap mode
void GPU::draw_mode4()
{
    u8 palette_index; // in mode 4 each pixel uses 1 byte 
    u16 color;        // the color located at pallette_ram[palette_index]
    
    // TODO - page flip
    std::cout << (int) stat->dispcnt.ps << "\n";
    int i = 0;
    for (int y = 0; y < SCREEN_HEIGHT; ++y)
    {
        for (int x = 0; x < SCREEN_WIDTH; ++x)
        {
            palette_index = mem->read_u8_unprotected(MEM_VRAM_START + i);
            // multiply by sizeof(u16) because each entry in palram is 2 bytes
            color = mem->read_u16_unprotected(MEM_PALETTE_RAM_START + (palette_index * sizeof(u16)));
            // add current pixel in argb format to pixel array
            screen_buffer[y][x] = u16_to_u32_color(color);
            ++i;
        }
    }
}

// draws regular background
// bg - background index (0-3)
void GPU::draw_reg_background(int bg)
{
    // initial address of background tileset
    u32 tileset_address = MEM_VRAM_START + CHARBLOCK_LEN * stat->bg_cnt[bg].cbb;

    // initial address of background tilemap
    u32 start_tilemap_address = MEM_VRAM_START + SCREENBLOCK_LEN * stat->bg_cnt[bg].sbb;
    u32 tilemap_address = start_tilemap_address;

    // get width / height (in tiles) of background
    int width, height;
    switch(stat->bg_cnt[bg].size)
    {
        case 0:
            width  = 32;
            height = 32;
        break;
        
        case 1:
            width  = 64;
            height = 32;
        break;

        case 2:
            width  = 32;
            height = 64;
        break;

        case 3:
            width  = 64;
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

    // TODO - yikes...
    for (int ssy = 0; ssy < (height / 32); ++ssy)
    {
        for (int ssx = 0; ssx < (width / 32); ++ssx)
        {
            tilemap_address = start_tilemap_address + ssx * SCREENBLOCK_LEN + (2 * ssy * SCREENBLOCK_LEN);
            for (int h = 0; h < TILES_PER_SCREENBLOCK; ++h)
            {
                for (int w = 0; w < TILES_PER_SCREENBLOCK; ++w)
                {
                    screen_entry = mem->read_u16_unprotected(tilemap_address);
                    tilemap_index = screen_entry & 0x3FF; // bits 9-0 

                    if (stat->bg_cnt[bg].color_mode == 0) // s-tile (4bpp)
                    {
                        palbank = screen_entry >> 12 & 0xF; // bits F - C
                        cur_screenblock = tileset_address + S_SCREENENTRY_DEPTH * tilemap_index;

                        for (int i = 0; i < S_TILE_LEN; ++i)
                        {
                            // 256 bc each screenblock is 32 tiles, 32 * 8 = 256
                            x = ssx * 256 + w * PX_IN_TILE_ROW + 2 * (i % 4); // s-tiles get left/right px in one read 
                            y = ssy * 256 + h * PX_IN_TILE_COL + (i / 4);
                            palette_index = mem->read_u8_unprotected(cur_screenblock + i);
                            
                            u8 left_pixel = palette_index & 0xF;
                            u8 right_pixel = (palette_index >> 4) & 0xF;

                            // add left, right pixel to screen buffer
                            // pixel value 0 is transparent, so only draw if not 0
                            if (left_pixel != 0)
                            {
                                // multiply by sizeof(u16) because each entry in palram is 2 bytes
                                color = mem->read_u16_unprotected(MEM_PALETTE_RAM_START + left_pixel * sizeof(u16) + (palbank * PALBANK_LEN));
                                map[y][x] = u16_to_u32_color(color);
                            }

                            // pixel value 0 is transparent, so only draw if not 0
                            if (right_pixel != 0)
                            {
                                // multiply by sizeof(u16) because each entry in palram is 2 bytes
                                color = mem->read_u16_unprotected(MEM_PALETTE_RAM_START + right_pixel * sizeof(u16) + (palbank * PALBANK_LEN));
                                map[y][x + 1] = u16_to_u32_color(color);
                            }
                        }

                    }
                    
                    else // d-tile (8bpp)
                    {
                        cur_screenblock = tileset_address + D_SCREENENTRY_DEPTH * tilemap_index;

                        for (int i = 0; i < D_TILE_LEN; i++)
                        {

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
    switch (bg)
    {
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
    for (int y = 0; y < SCREEN_HEIGHT; ++y)
    {
        for (int x = 0; x < SCREEN_WIDTH; ++x)
        {
            // modulo mapsize to allow wrapping
            if (map[(y + voff) % (height * PX_IN_TILE_COL)][(x + hoff) % (width * PX_IN_TILE_ROW)] != 0)
                screen_buffer[y][x] = map[(y + voff) % (height * PX_IN_TILE_COL)][(x + hoff) % (width * PX_IN_TILE_ROW)];
        }
    }
}

void GPU::draw_affine_background(int bg)
{
    // initial address of background tileset
    u32 tileset_address = MEM_VRAM_START + CHARBLOCK_LEN * stat->bg_cnt[bg].cbb;

    // initial address of background tilemap
    u32 start_tilemap_address = MEM_VRAM_START + SCREENBLOCK_LEN * stat->bg_cnt[bg].sbb;
    u32 tilemap_address = start_tilemap_address;

    // get width / height (in tiles) of background
    int width, height;
    switch(stat->bg_cnt[bg].size)
    {
        case 0:
            width  = 16;
            height = 16;
        break;
        
        case 1:
            width  = 32;
            height = 32;
        break;

        case 2:
            width  = 64;
            height = 64;
        break;

        case 3:
            width  = 128;
            height = 128;
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

    // TODO - yikes...
    for (int ssy = 0; ssy < (height / 32); ++ssy)
    {
        for (int ssx = 0; ssx < (width / 32); ++ssx)
        {
            tilemap_address = start_tilemap_address + ssx * SCREENBLOCK_LEN + (2 * ssy * SCREENBLOCK_LEN);
            for (int h = 0; h < TILES_PER_SCREENBLOCK; ++h)
            {
                for (int w = 0; w < TILES_PER_SCREENBLOCK; ++w)
                {
                    screen_entry = mem->read_u16_unprotected(tilemap_address);
                    tilemap_index = screen_entry & 0x3FF; // bits 9-0 

                    if (stat->bg_cnt[bg].color_mode == 0) // s-tile (4bpp)
                    {
                        palbank = screen_entry >> 12 & 0xF; // bits F - C
                        cur_screenblock = tileset_address + S_SCREENENTRY_DEPTH * tilemap_index;

                        for (int i = 0; i < S_TILE_LEN; ++i)
                        {
                            // 256 bc each screenblock is 32 tiles, 32 * 8 = 256
                            x = ssx * 256 + w * PX_IN_TILE_ROW + 2 * (i % 4); // s-tiles get left/right px in one read 
                            y = ssy * 256 + h * PX_IN_TILE_COL + (i / 4);
                            palette_index = mem->read_u8_unprotected(cur_screenblock + i);
                            
                            u8 left_pixel = palette_index & 0xF;
                            u8 right_pixel = (palette_index >> 4) & 0xF;

                            // add left, right pixel to screen buffer
                            // pixel value 0 is transparent, so only draw if not 0
                            if (left_pixel != 0)
                            {
                                // multiply by sizeof(u16) because each entry in palram is 2 bytes
                                color = mem->read_u16_unprotected(MEM_PALETTE_RAM_START + left_pixel * sizeof(u16) + (palbank * PALBANK_LEN));
                                map[y][x] = u16_to_u32_color(color);
                            }

                            // pixel value 0 is transparent, so only draw if not 0
                            if (right_pixel != 0)
                            {
                                // multiply by sizeof(u16) because each entry in palram is 2 bytes
                                color = mem->read_u16_unprotected(MEM_PALETTE_RAM_START + right_pixel * sizeof(u16) + (palbank * PALBANK_LEN));
                                map[y][x + 1] = u16_to_u32_color(color);
                            }
                        }

                    }
                    
                    else // d-tile (8bpp)
                    {
                        cur_screenblock = tileset_address + D_SCREENENTRY_DEPTH * tilemap_index;

                        for (int i = 0; i < D_TILE_LEN; i++)
                        {

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

    // copy area of map that screen is over into screen buffer
    for (int y = 0; y < SCREEN_HEIGHT; ++y)
    {
        for (int x = 0; x < SCREEN_WIDTH; ++x)
        {
            // modulo mapsize to allow wrapping
            //if (map[(y + voff) % (height * PX_IN_TILE_COL)][(x + hoff) % (width * PX_IN_TILE_ROW)] != 0)
                screen_buffer[y][x] = map[y][x];//map[(y) % (height * PX_IN_TILE_COL)][(x) % (width * PX_IN_TILE_ROW)];
        }
    }
}

void GPU::draw_sprites()
{
    // make sure sprites are 1D mode, I haven't added 2D mode compatibility yet
    // if (stat->dispcnt.obj_map_mode == 0)
    //     std::cout << "Caution: sprite mode is 2D\n";

    obj_attr attr;
    // TODO - priority
    for (int i = NUM_OBJS - 1; i >= 0; --i)
    {
        attr = objs[i];
        switch (attr.attr_0.attr.om)
        {
            case 0x0: // normal rendering
                draw_regular_sprite(attr);
                break;
            case 0x1: // affine & affine double
            case 0x3:
                draw_affine_sprite(attr);
            case 0x2: // hidden
                continue;
        }
    }
}

void GPU::draw_regular_sprite(obj_attr attr)
{
    // x, y coordinate of top left of sprite
    u16 x0 = attr.attr_1.attr.x;
    u8  y0 = attr.attr_0.attr.y;

    // std::cout << "x: " << (int) x << "\n";
    // std::cout << "y: " <<  (int) y << "\n";

    // TODO - wat is this
    u32 base_tile_addr = LOWER_SPRITE_BLOCK + (attr.attr_2.attr.tileno * S_TILE_LEN);
    u32 tile_ptr = base_tile_addr;

    // get width, height in tiles of sprite
    int width, height;
    switch (attr.size())
    {
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

    u16 color;
    u8 palette_index; // nth entry in palram
    u8 palbank = attr.attr_2.attr.l;
    u16 x;
    u8 y;

    //draw sprite tile by tile
    for (int h = 0; h < height; ++h)
    {
        for (int w = 0; w < width; ++w)
        {
            if (s_tile) // 4 bits / pixel - s-tile
            {
                for (int i = 0; i < S_TILE_LEN; ++i)
                {
                    palette_index = mem->read_u8_unprotected(base_tile_addr + i);
                    x = x0 + w * PX_IN_TILE_ROW + 2 * (i % 4); // s-tiles get left/right px in one read 
                    y = y0 + h * PX_IN_TILE_COL + (i / 4);

                    u8 left_pixel = palette_index & 0xF;
                    u8 right_pixel = (palette_index >> 4) & 0xF;
                    
                    // add left, right pixel to screen buffer
                    // pixel value 0 is transparent, so only draw if not 0
                    if (left_pixel != 0)
                    {
                        // multiply by sizeof(u16) because each entry in palram is 2 bytes
                        color = mem->read_u16_unprotected(SPRITE_PALETTE + left_pixel * sizeof(u16) + (palbank * PALBANK_LEN));
                        screen_buffer[y][x] = u16_to_u32_color(color);
                    }

                    // pixel value 0 is transparent, so only draw if not 0
                    if (right_pixel != 0)
                    {
                        // multiply by sizeof(u16) because each entry in palram is 2 bytes
                        color = mem->read_u16_unprotected(SPRITE_PALETTE + right_pixel * sizeof(u16) + (palbank * PALBANK_LEN));
                        screen_buffer[y][x + 1] = u16_to_u32_color(color);
                    }
                }


            }
            
            // 8 bits / pixel - d-tile
            else
            {
                for (int i = 0; i < D_TILE_LEN; i++)
                {

                    palette_index = mem->read_u8_unprotected(base_tile_addr + i);

                    // pixel value 0 is transparent, so only draw if not 0
                    if (palette_index == 0)
                        continue; 

                    x = x0 + w * PX_IN_TILE_ROW + (i % 8);
                    y = y0 + h * PX_IN_TILE_COL + (i / 8);

                    // multiply by sizeof(u16) because each entry in palram is 2 bytes
                    color = mem->read_u32_unprotected(SPRITE_PALETTE + palette_index * sizeof(u16));

                    screen_buffer[y][x] = u16_to_u32_color(color);
                }
            }

            // increment start address for tile
            base_tile_addr += s_tile ? S_TILE_LEN : D_TILE_LEN;
        }
    }

    // horizontal flip
    if (attr.attr_1.attr.h)
    {
        u32 temp;
        for (int h = 0; h < height * 8; ++h)
        {
            for (int w = 0; w < width * 4; ++w) 
            {
                temp = screen_buffer[y0 + h][x0 + w];
                screen_buffer[y0 + h][x0 + w] = screen_buffer[y0 + h][(x0 + width * 8) - w];
                screen_buffer[y0 + h][(x0 + width * 8) - w] = temp;
            }
        }
    }

    // vertical flip
    if (attr.attr_1.attr.v) 
    {
        u32 temp;
        for (int h = 0; h < height * 4; ++h)
        {
            for (int w = 0; w < width * 8; ++w)
            {
                temp = screen_buffer[y0 + h][x0 + w];
                screen_buffer[y0 + h][x0 + w] = screen_buffer[(y0 + height * 8) - h][x0 + w];
                screen_buffer[(y0 + height * 8) - h][x0 + w] = temp;
            }
        }
    }

    // mosaic
    if (attr.attr_0.attr.m)
    {
        u16 mosaic = mem->read_u16_unprotected(REG_MOSAIC);
        
        // horizontal / vertical stretch
        // add one to get range 1 - 16
        u8 wm = ((mosaic >>  8) & 0xF) + 1; // bits 8 - B
        u8 hm = ((mosaic >> 12) & 0xF) + 1; // bits C - F

        u32 temp;
        for (int y = 0; y < height * PX_IN_TILE_COL; y += hm)
        {
            for (int x = 0; x < width * PX_IN_TILE_ROW; x += wm)
            {
                //std::cout << (int) x << "\n";
                temp = screen_buffer[y0 + y][x0 + x];

                // fill wm more x values with the top left
                for (int xi = 0; xi < wm; ++xi)
                {
                    // make sure it doesn't go past sprite's bounds
                    //if (x0 + x + xi < (x0 + width * PX_IN_TILE_ROW))
                    //std::cout << (int) x0 + x + xi << "\n";
                        screen_buffer[y0 + y][x0 + x + xi] = temp;
                }
                    

                // fill hm more y values with the top left    
                for (int yi = 0; yi < hm; ++yi)
                {
                    //std::cout << (int) y0 + y + yi << "\n";
                   //if (y0 + y + yi < (y0 + height * PX_IN_TILE_COL))
                        screen_buffer[y0 + y + yi][x0 + x] = temp;
                }
            }
            //std::cout << "\n";
        }
    }

    //std::cout << "\n\n";
}

void GPU::draw_affine_sprite(obj_attr attr)
{
    // get affine matrix
    u8 aff_index = (attr.attr_1.attr.v << 4) | (attr.attr_1.attr.h << 3) | (attr.attr_1.attr.f);
    u32 oam_addr = MEM_OAM_START + aff_index * 32; // each affine entry is 32 bytes across

    // transform P matrix from 8.8f to float
    // P = [pa pb]
    //     [pc pd]
    float pa = (s16) mem->read_u16(oam_addr +  0x6) / 256.0; 
    float pb = (s16) mem->read_u16(oam_addr +  0xE) / 256.0;
    float pc = (s16) mem->read_u16(oam_addr + 0x16) / 256.0;
    float pd = (s16) mem->read_u16(oam_addr + 0x1E) / 256.0;

    u32 base_tile_addr = LOWER_SPRITE_BLOCK + (attr.attr_2.attr.tileno * S_TILE_LEN);
    bool s_tile = attr.attr_0.attr.a == 0;

    // get width, height in tiles of sprite
    int width, height;
    switch (attr.size())
    {
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

    // coordinates of corners of sprite
    u16 left_x   = attr.attr_1.attr.x;
    u8  top_y    = attr.attr_0.attr.y;

    // temporary buffer to hold texture
    u32 sprite[height * PX_IN_TILE_COL][width * PX_IN_TILE_ROW] = {0};

    u16 color;
    u8 palette_index; // nth entry in palram
    u8 palbank = attr.attr_2.attr.l;

    // x, y coordinate of sprite before rot/scale (1) and after (2)
    u16 x1, x2;
    u8  y1, y2;

    // draw sprite tile by tile
    for (int h = 0; h < height; ++h)
    {
        for (int w = 0; w < width; ++w)
        {
            if (s_tile) // 4 bits / pixel - s-tile
            {
                for (int i = 0; i < S_TILE_LEN; ++i)
                {
                    palette_index = mem->read_u8_unprotected(base_tile_addr + i);
                    x1 = w * PX_IN_TILE_ROW + 2 * (i % 4); // s-tiles get left/right px in one read 
                    y1 = h * PX_IN_TILE_COL + (i / 4);
                    
                    u8 left_pixel = palette_index & 0xF;
                    u8 right_pixel = (palette_index >> 4) & 0xF;
                    
                    // add left, right pixel to screen buffer
                    // pixel value 0 is transparent, so only draw if not 0
                    if (left_pixel != 0)
                    {
                        // multiply by sizeof(u16) because each entry in palram is 2 bytes
                        color = mem->read_u16_unprotected(SPRITE_PALETTE + left_pixel * sizeof(u16) + (palbank * PALBANK_LEN));
                        sprite[y1][x1] = u16_to_u32_color(color);
                    }

                    // pixel value 0 is transparent, so only draw if not 0
                    if (right_pixel != 0)
                    {
                        // multiply by sizeof(u16) because each entry in palram is 2 bytes
                        color = mem->read_u16_unprotected(SPRITE_PALETTE + right_pixel * sizeof(u16) + (palbank * PALBANK_LEN));
                        sprite[y1][x1 + 1] = u16_to_u32_color(color);
                    }
                }
            }
            
            // 8 bits / pixel - d-tile
            else
            {
                for (int i = 0; i < D_TILE_LEN; i++)
                {
                    palette_index = mem->read_u8_unprotected(base_tile_addr + i);

                    // pixel value 0 is transparent, so only draw if not 0
                    if (palette_index == 0)
                        continue; 

                    x1 = w * PX_IN_TILE_ROW + (i % 8);
                    y1 = h * PX_IN_TILE_COL + (i / 8);

                    // multiply by sizeof(u16) because each entry in palram is 2 bytes
                    color = mem->read_u32_unprotected(SPRITE_PALETTE + palette_index * sizeof(u16));

                    sprite[y1][x1] = u16_to_u32_color(color);
                }
            }

            // increment start address for tile
            base_tile_addr += s_tile ? S_TILE_LEN : D_TILE_LEN;
        }
    }

    // x, y coordinate of rot / scale center of texture (center of sprite)
    u16 x0 = width * 4;
    u8  y0 = height * 4;
    
    // max value sprite can have in screen space
    u16 max_screen_x = width  * PX_IN_TILE_ROW; 
    u8 max_screen_y = height * PX_IN_TILE_COL;

    int hwidth  = width  * PX_IN_TILE_ROW; 
    int hheight = width  * PX_IN_TILE_ROW; 

    // double flag set
    if (attr.attr_0.attr.om == 3)
    {
        hwidth *= 2;
        hheight *= 2;
    }

    //std::cout << hwidth << " " << hheight << "\n";
    // map texture from texture space to screen space using P
    for (int yy = 0; yy < hheight; ++yy)
    {
        for (int xx = 0; xx < hwidth; ++xx)
        {
            //std::cout << xx << " " << yy << "\n";
            y1 = yy + top_y;
            x1 = xx + left_x;

            // get new position of pixel after rot / scale
            x2 = pa * (xx - x0) + pb * (yy - y0) + x0;
            y2 = pc * (xx - x0) + pd * (yy - y0) + y0;

            // transfomation put pixel out of bounds
            if ((x2 >= hwidth) || (y2 >= hheight))
                continue;
            
            // skip 0 pixel value (transparent)
            if (sprite[y2][x2] != 0)
                screen_buffer[y1][x1] = sprite[y2][x2];
        }
    }

    //exit(0);
}

// fills the objs data structure every frame an object needs to be drawn
void GPU::update_attr()
{
    u32 address = MEM_OAM_START;
    // loop through all 128 objects
    for (int i = 0; i < NUM_OBJS; ++i)
    {
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

// given a 16 bit GBA color, make it a 32 bit SDL color
inline u32 u16_to_u32_color (u16 color_u16)
{
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




// attempt at trying to draw a sprite not tile by tile
// for (u8 hh = 0; hh < height * PX_IN_TILE_COL; ++hh)
// {
//     tile_ptr = base_tile_addr + hh * PX_IN_TILE_COL * width;
//     std::cout << std::hex << tile_ptr << "\n";
//     for (u16 ww = 0; ww < width * PX_IN_TILE_COL; ++ww)
//     {
//         //std::cout << "x: " << ww << " " << "y: " << (int) hh << "\n";
//         palette_index = mem->read_u8_unprotected(tile_ptr + (ww / 8) * 32);
//         // x = start_x + w * PX_IN_TILE_ROW + 2 * (i % 4); // s-tiles get left/right px in one read 
//         // y = start_y + h * PX_IN_TILE_COL + (i / 4);

//         u8 left_pixel = palette_index & 0xF;
//         u8 right_pixel = (palette_index >> 4) & 0xF;
        
//         // add left, right pixel to screen buffer
//         // pixel value 0 is transparent, so only draw if not 0
//         if (left_pixel != 0)
//         {
//             // multiply by sizeof(u16) because each entry in palram is 2 bytes
//             color = mem->read_u16_unprotected(SPRITE_PALETTE + left_pixel * sizeof(u16) + (palbank * PALBANK_LEN));
//             screen_buffer[y0 + hh][x0 + ww] = u16_to_u32_color(color);
//         }

//         // pixel value 0 is transparent, so only draw if not 0
//         if (right_pixel != 0)
//         {
//             // multiply by sizeof(u16) because each entry in palram is 2 bytes
//             color = mem->read_u16_unprotected(SPRITE_PALETTE + right_pixel * sizeof(u16) + (palbank * PALBANK_LEN));
//             screen_buffer[y0 + hh][x0 + ww + 1] = u16_to_u32_color(color);
//         }

//         ww++;

//         //base_tile_addr += s_tile ? S_TILE_LEN : D_TILE_LEN;
//     }
// }